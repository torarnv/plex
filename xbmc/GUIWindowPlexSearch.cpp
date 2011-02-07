/*
 *      Copyright (C) 2011 Plex
 *      http://www.plexapp.com
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include "stdafx.h"

#include "Application.h"
#include "FileItem.h"
#include "GUIBaseContainer.h"
#include "GUIWindowManager.h"
#include "GUIInfoManager.h"
#include "GUILabelControl.h"
#include "GUIWindowPlexSearch.h"
#include "PlexDirectory.h"
#include "Settings.h"
#include "Util.h"

#define CTL_LABEL_EDIT       310
#define CTL_BUTTON_BACKSPACE 8

#define SEARCH_DELAY         500

using namespace DIRECTORY;

class PlexSearchWorker : public CThread
{
 public:
  
  PlexSearchWorker(const string& url, const string& query)
    : m_url(url)
    , m_query(query)
    , m_cancelled(false)
    , m_results(new CFileItemList())
  {
  }
  
  void Process()
  {
    // Escape the query.
    CStdString query = m_query;
    CUtil::URLEncode(query);
    
    // Get the results.
    CPlexDirectory dir;
    CStdString path = CStdString(m_url);

    // Strip tailing slash.
    if (path[path.size()-1] == '/')
      path = path.substr(0, path.size()-1);
    
    // Add the query parameter.
    if (path.Find("?") == string::npos)
      path = path + "?query=" + query;
    else
      path = path + "&query=" + query;
    
    printf("Running query for %s [%s]\n", path.c_str(), m_query.c_str());
    dir.GetDirectory(path, *m_results);
    
    // If we haven't been cancelled, send them back.
    if (m_cancelled == false)
    {
      // Notify the main menu.
      CGUIMessage msg2(GUI_MSG_SEARCH_HELPER_COMPLETE, WINDOW_PLEX_SEARCH, 300, 0, 0, m_results);
      m_gWindowManager.SendThreadMessage(msg2);
    }
    else
    {
      // Whack it, nobody wants it.
      delete m_results;
    }
  }
  
  void Cancel()
  {
    m_cancelled = true;
  }
  
  CFileItemList& GetResults()
  {
    return *m_results;
  }
  
 private:
  
  string m_url;
  string m_query;
  bool   m_cancelled;
  
  CFileItemList* m_results;
};

///////////////////////////////////////////////////////////////////////////////
CGUIWindowPlexSearch::CGUIWindowPlexSearch() 
  : CGUIWindow(WINDOW_PLEX_SEARCH, "PlexSearch.xml")
  , m_lastSearchUpdate(0)
  , m_resetOnNextResults(false)
  , m_resultSelected(false)
  , m_searchWorker(0)
{
  // Initialize results lists.
  m_categoryResults[PLEX_METADATA_MOVIE] = Group(kVIDEO_LOADER);
  m_categoryResults[PLEX_METADATA_SHOW] = Group(kVIDEO_LOADER);
  m_categoryResults[PLEX_METADATA_EPISODE] = Group(kVIDEO_LOADER);
  m_categoryResults[PLEX_METADATA_ARTIST] = Group(kMUSIC_LOADER);
  m_categoryResults[PLEX_METADATA_ALBUM] = Group(kMUSIC_LOADER);
  m_categoryResults[PLEX_METADATA_TRACK] = Group(kMUSIC_LOADER);
  m_categoryResults[PLEX_METADATA_CLIP] = Group(kVIDEO_LOADER);
}

///////////////////////////////////////////////////////////////////////////////
CGUIWindowPlexSearch::~CGUIWindowPlexSearch()
{
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::OnInitWindow()
{
  CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));
  if (pEdit)
    pEdit->ShowCursor();

  if (m_resultSelected == false)
  {
    Reset();
    m_strEdit = "";
    UpdateLabel();
  }
  
  m_resultSelected = false;
}

///////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexSearch::OnAction(const CAction &action)
{
  CStdString strAction = action.strAction;
  strAction = strAction.ToLower();
  
  if (action.wID == ACTION_PREVIOUS_MENU)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }
  else if (action.wID == ACTION_PARENT_DIR)
  {
    Backspace();
    return true;
  }
  else
  {
    // Input from the keyboard.
    switch (action.unicode)
    {
    case 8:   // backspace
      Backspace();
      break;
    case 27:  // escape
      Close();
      break;
      
    default:
      if (CGUIWindow::OnAction(action) == false) 
        Character(action.unicode);
    }
    return true;
  }
  
  return false;
}

///////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexSearch::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_SEARCH_HELPER_COMPLETE:
  {
    CFileItemListPtr results = CFileItemListPtr((CFileItemList* )message.GetLPVOID());
    
    if (m_resetOnNextResults)
    {
      Reset();
      m_resetOnNextResults = false;
    }
    
    // If we have any additional providers, run them in parallel.
    vector<CFileItemPtr>& providers = results->GetProviders();
    BOOST_FOREACH(CFileItemPtr& provider, providers)
    {
      // Convert back to utf8.
      CStdString search;
      g_charsetConverter.wToUTF8(m_strEdit, search);

      // Create a new worker.
      PlexSearchWorker* worker = new PlexSearchWorker(provider->m_strPath.c_str(), search);
      worker->Create(true);
    }
      
    // Put the items in the right category.
    for (int i=0; i<results->Size(); i++)
    {
      // Get the item and the type.
      CFileItemPtr item = results->Get(i);
      int type = boost::lexical_cast<int>(item->GetProperty("typeNumber"));
      
      // Add it to the correct "bucket".
      if (m_categoryResults.find(type) != m_categoryResults.end())
        m_categoryResults[type].list->Add(item);
    }

    // Bind all the lists.
    BOOST_FOREACH(int_list_pair pair, m_categoryResults)
    {
      int controlID = 9000 + pair.first;
      CGUIBaseContainer* control = (CGUIBaseContainer* )GetControl(controlID);
      if (control && pair.second.list->Size() > 0)
      {
        CGUIMessage msg(GUI_MSG_LABEL_BIND, CTL_LABEL_EDIT, controlID, 0, 0, pair.second.list.get());
        OnMessage(msg);
        
        SET_CONTROL_VISIBLE(controlID);
        SET_CONTROL_VISIBLE(controlID-1000);
      }
    }
    
    // Get thumbs and then reset results.
    BOOST_FOREACH(int_list_pair pair, m_categoryResults)
    {
      pair.second.loader->Load(*pair.second.list.get());
      pair.second.list->Clear();
    }
  }
  break;
  
  case GUI_MSG_WINDOW_DEINIT:
  {
    if (m_resultSelected == false)
    {
      if (m_videoThumbLoader.IsLoading())
        m_videoThumbLoader.StopThread();
      
      if (m_musicThumbLoader.IsLoading())
        m_musicThumbLoader.StopThread();
    }
  }
  break;
  
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();
    OnClickButton(iControl);
    return true;
  }
  break;
  }
  
  return CGUIWindow::OnMessage(message);
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::Render()
{
  if (m_lastSearchUpdate && m_lastSearchUpdate + SEARCH_DELAY < timeGetTime())
    UpdateLabel();
  
  CGUIWindow::Render();
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::Character(WCHAR ch)
{
  if (!ch) 
    return;
  
  m_strEdit.Insert(GetCursorPos(), ch);
  UpdateLabel();
  MoveCursor(1);
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::Backspace()
{
  int iPos = GetCursorPos();
  if (iPos > 0)
  {
    m_strEdit.erase(iPos - 1, 1);
    MoveCursor(-1);
    UpdateLabel();
  }
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::UpdateLabel()
{
  CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));
  if (pEdit)
  {
    // Convert back to utf8.
    CStdString utf8Edit;
    g_charsetConverter.wToUTF8(m_strEdit, utf8Edit);
    pEdit->SetLabel(utf8Edit);
    
    // Send off a search message if it's been SEARCH_DELAY since last search.
    DWORD now = timeGetTime();
    if (!m_lastSearchUpdate || m_lastSearchUpdate + SEARCH_DELAY >= now)
      m_lastSearchUpdate = now;
    
    if (m_lastSearchUpdate + SEARCH_DELAY < now)
    {
      m_lastSearchUpdate = 0;
      StartSearch(utf8Edit);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::Reset()
{
  // Reset results.
  printf("Resetting results.\n");
  BOOST_FOREACH(int_list_pair pair, m_categoryResults)
  {
    int controlID = 9000 + pair.first;
    CGUIBaseContainer* control = (CGUIBaseContainer* )GetControl(controlID);
    if (control)
    {
      CGUIMessage msg(GUI_MSG_LABEL_RESET, CTL_LABEL_EDIT, controlID);
      OnMessage(msg);
      
      SET_CONTROL_HIDDEN(controlID);
      SET_CONTROL_HIDDEN(controlID-1000);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::StartSearch(const string& search)
{
  if (search.empty())
  {
    // Reset results.
    Reset();
  }
  else
  {
    // Issue the first canonical search result to the local media server.
    m_searchWorker = new PlexSearchWorker("http://localhost:32400/search", search);
    m_searchWorker->Create(false);
    m_resetOnNextResults = true; 
  }
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::MoveCursor(int iAmount)
{
  CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));
  if (pEdit)
    pEdit->SetCursorPos(pEdit->GetCursorPos() + iAmount);
}

///////////////////////////////////////////////////////////////////////////////
int CGUIWindowPlexSearch::GetCursorPos() const
{
  const CGUILabelControl* pEdit = (const CGUILabelControl*)GetControl(CTL_LABEL_EDIT);
  if (pEdit)
    return pEdit->GetCursorPos();

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
char CGUIWindowPlexSearch::GetCharacter(int iButton)
{
  if (iButton >= 65 && iButton <= 90)
  {
    // It's a letter.
    return 'a' + (iButton-65);
  }
  else if (iButton >= 91 && iButton <= 100)
  {
    // It's a number.
    return '0' + (iButton-91);
  }
  
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::OnClickButton(int iButtonControl)
{
  printf("OnClickButton.\n");
  
  if (iButtonControl == CTL_BUTTON_BACKSPACE)
  {
    Backspace();
  }
  else
  {
    char ch = GetCharacter(iButtonControl);
    if (ch != 0)
    {
      Character(ch);
    }
    else
    {
      const CGUIControl* control = GetControl(iButtonControl);
      if (control->IsContainer())
      {
        const CGUIBaseContainer* container = (CGUIBaseContainer* )control;
        CGUIListItemPtr item = container->GetListItem(0);
       
        if (item)
        {
          printf("Playing %s\n", item->GetLabel().c_str());
          m_resultSelected = true;
          g_application.PlayFile(*(CFileItem* )item.get());
        }
        else
        {
          printf("No selected item: %d\n");
        }
      }
      else
      {
        printf("Not a container: %d\n", iButtonControl);
      }
    }
  }
}
