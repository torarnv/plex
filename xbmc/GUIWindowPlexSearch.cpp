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
#include <boost/thread/mutex.hpp>

#include "stdafx.h"

#include "Application.h"
#include "FileItem.h"
#include "GUIBaseContainer.h"
#include "GUIDialogContextMenu.h"
#include "GUIWindowManager.h"
#include "GUIInfoManager.h"
#include "GUILabelControl.h"
#include "GUIWindowPlexSearch.h"
#include "PlexDirectory.h"
#include "Settings.h"
#include "Util.h"

#define CTL_LABEL_EDIT       310
#define CTL_BUTTON_BACKSPACE 8
#define CTL_BUTTON_CLEAR     10
#define CTL_BUTTON_SPACE     32

#define SEARCH_DELAY         750

using namespace boost;
using namespace DIRECTORY;

class PlexSearchWorker;
typedef boost::shared_ptr<PlexSearchWorker> PlexSearchWorkerPtr;

class PlexSearchWorker : public CThread
{
 public:
  
  virtual ~PlexSearchWorker()
  {
    printf("Whacking search worker: %d.\n", m_id);
  }
  
  void Process()
  {
    printf("Processing search request in thread.\n");
    
    if (m_cancelled == false)
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
      dir.GetDirectory(path, m_results);
    }
      
    // If we haven't been cancelled, send them back.
    if (m_cancelled == false)
    {
      // Notify the main menu.
      CGUIMessage msg2(GUI_MSG_SEARCH_HELPER_COMPLETE, WINDOW_PLEX_SEARCH, 300, m_id);
      m_gWindowManager.SendThreadMessage(msg2);
    }
    else
    {
      // Get rid of myself.
      Delete(m_id);
    }
  }
  
  void Cancel()
  {
    m_cancelled = true;
  }
  
  CFileItemList& GetResults()
  {
    return m_results;
  }
  
  int GetID()
  {
    return m_id;
  }
  
  static int PendingWorkers()
  {
    return g_pendingWorkers.size();
  }
  
  static PlexSearchWorkerPtr Construct(const string& url, const string& query)
  {
    mutex::scoped_lock lk(g_mutex);
    
    PlexSearchWorkerPtr worker = PlexSearchWorkerPtr(new PlexSearchWorker(url, query));
    g_pendingWorkers[worker->GetID()] = worker;
    worker->Create(false);
    
    return worker;
  }
  
  static PlexSearchWorkerPtr Find(int id)
  {
    mutex::scoped_lock lk(g_mutex);
    
    if (g_pendingWorkers.find(id) != g_pendingWorkers.end())
      return g_pendingWorkers[id];
    
    return PlexSearchWorkerPtr();
  }
  
  static void Delete(int id)
  {
    mutex::scoped_lock lk(g_mutex);
    
    if (g_pendingWorkers.find(id) != g_pendingWorkers.end())
      g_pendingWorkers.erase(id);
  }
  
  static void CancelPending()
  {
    mutex::scoped_lock lk(g_mutex);
    
    typedef pair<int, PlexSearchWorkerPtr> int_worker_pair;
    BOOST_FOREACH(int_worker_pair pair, g_pendingWorkers)
      pair.second->Cancel();
  }
  
 protected:
  
  PlexSearchWorker(const string& url, const string& query)
    : m_id(g_workerID++)
    , m_url(url)
    , m_query(query)
    , m_cancelled(false)
  {
  }
  
 private:
  
  int           m_id;
  string        m_url;
  string        m_query;
  bool          m_cancelled;
  CFileItemList m_results;
  
  /// Keeps track of the last worker ID.
  static int g_workerID;

  /// Keeps track of pending workers.
  static map<int, PlexSearchWorkerPtr> g_pendingWorkers;
  
  /// Protects the map.
  static mutex g_mutex;
};

// Static initialization.
int PlexSearchWorker::g_workerID = 0;
mutex PlexSearchWorker::g_mutex;
map<int, PlexSearchWorkerPtr> PlexSearchWorker::g_pendingWorkers;

///////////////////////////////////////////////////////////////////////////////
CGUIWindowPlexSearch::CGUIWindowPlexSearch() 
  : CGUIWindow(WINDOW_PLEX_SEARCH, "PlexSearch.xml")
  , m_lastSearchUpdate(0)
  , m_resetOnNextResults(false)
  , m_selectedContainerID(-1)
  , m_selectedItem(-1)
{
  // Initialize results lists.
  m_categoryResults[PLEX_METADATA_MOVIE] = Group(kVIDEO_LOADER);
  m_categoryResults[PLEX_METADATA_SHOW] = Group(kVIDEO_LOADER);
  m_categoryResults[PLEX_METADATA_EPISODE] = Group(kVIDEO_LOADER);
  m_categoryResults[PLEX_METADATA_ARTIST] = Group(kMUSIC_LOADER);
  m_categoryResults[PLEX_METADATA_ALBUM] = Group(kMUSIC_LOADER);
  m_categoryResults[PLEX_METADATA_TRACK] = Group(kMUSIC_LOADER);
  m_categoryResults[PLEX_METADATA_PERSON] = Group(kVIDEO_LOADER);
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

  if (m_selectedItem != -1)
  {
    // Convert back to utf8.
    CStdString utf8Edit;
    g_charsetConverter.wToUTF8(m_strEdit, utf8Edit);
    pEdit->SetLabel(utf8Edit);
    pEdit->SetCursorPos(utf8Edit.size());

    // Bind the lists.
    Bind();
    
    // Select group.
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), m_selectedContainerID);
    OnMessage(msg);

    // Select item.
    CGUIMessage msg2(GUI_MSG_ITEM_SELECT, GetID(), m_selectedContainerID, m_selectedItem);
    OnMessage(msg2);
    
    m_selectedContainerID = -1;
    m_selectedItem = -1;
  }
  else
  {
    // Reset the view.
    Reset();
    m_strEdit = "";
    UpdateLabel();
    
    // Select button.
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), 65);
    OnMessage(msg);
  }
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
  else if (action.wID == ACTION_MOVE_LEFT || action.wID == ACTION_MOVE_RIGHT ||
           action.wID == ACTION_MOVE_UP   || action.wID == ACTION_MOVE_DOWN  ||
           action.wID == ACTION_PAGE_UP   || action.wID == ACTION_PAGE_DOWN  ||
           action.wID == ACTION_HOME      || action.wID == ACTION_END        ||
           action.wID == ACTION_SELECT_ITEM)
  {
    // Reset search time.
    m_lastSearchUpdate = 0;
    
    // Allow cursor keys to work.
    return CGUIWindow::OnAction(action);
  }
  else
  {
    // Input from the keyboard.
    switch (action.unicode)
    {
    case 8:  // backspace.
      Backspace();
      break;
    case 27: // escape.
      Close();
      break;
    case 0:
      return false;
    case 13: // return.
      return CGUIWindow::OnAction(action);
      break;
      
    default:
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
    PlexSearchWorkerPtr worker = PlexSearchWorker::Find(message.GetParam1());
    if (worker)
    {
      printf("Processing results from worker: %d.\n", worker->GetID());
      CFileItemList& results = worker->GetResults();

      int lastFocusedList = -1;
      if (m_resetOnNextResults)
      {
        // Get the last focused list.
        lastFocusedList = GetFocusedControlID();
        if (lastFocusedList < 9000)
          lastFocusedList = -1;
        
        Reset();
        m_resetOnNextResults = false;
      }
      
      // If we have any additional providers, run them in parallel.
      vector<CFileItemPtr>& providers = results.GetProviders();
      BOOST_FOREACH(CFileItemPtr& provider, providers)
      {
        // Convert back to utf8.
        CStdString search;
        g_charsetConverter.wToUTF8(m_strEdit, search);
        
        // Create a new worker.
        PlexSearchWorker::Construct(provider->m_strPath.c_str(), search);
      }
      
      // Put the items in the right category.
      for (int i=0; i<results.Size(); i++)
      {
        // Get the item and the type.
        CFileItemPtr item = results.Get(i);
        int type = boost::lexical_cast<int>(item->GetProperty("typeNumber"));
        
        // Add it to the correct "bucket".
        if (m_categoryResults.find(type) != m_categoryResults.end())
          m_categoryResults[type].list->Add(item);
        else
          printf("Warning, skipping item with category %d.\n", type);
      }
      
      // Bind all the lists.
      int firstListWithStuff = -1;
      BOOST_FOREACH(int_list_pair pair, m_categoryResults)
      {
        int controlID = 9000 + pair.first;
        CGUIBaseContainer* control = (CGUIBaseContainer* )GetControl(controlID);
        if (control && pair.second.list->Size() > 0)
        {
          if (control->GetRows() != pair.second.list->Size())
          {
            // Save selected item.
            int selectedItem = control->GetSelectedItem();
            
            // Set the list.
            CGUIMessage msg(GUI_MSG_LABEL_BIND, CTL_LABEL_EDIT, controlID, 0, 0, pair.second.list.get());
            OnMessage(msg);
            
            // Restore selected item.
            CONTROL_SELECT_ITEM(control->GetID(), selectedItem);
            
            // Make sure it's visible.
            SET_CONTROL_VISIBLE(controlID);
            SET_CONTROL_VISIBLE(controlID-2000);
          }
            
          if (firstListWithStuff == -1)
            firstListWithStuff = controlID;
        }
      }
      
      // If we lost focus, restore it.
      if (lastFocusedList > 0)
      {
        CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), firstListWithStuff != -1 ? firstListWithStuff : 70);
        OnMessage(msg);
      }
      
      // Get thumbs.
      BOOST_FOREACH(int_list_pair pair, m_categoryResults)
        pair.second.loader->Load(*pair.second.list.get());
      
      // Whack it.
      PlexSearchWorker::Delete(message.GetParam1());
    }
  }
  break;
  
  case GUI_MSG_WINDOW_DEINIT:
  {
    if (m_videoThumbLoader.IsLoading())
      m_videoThumbLoader.StopThread();
    
    if (m_musicThumbLoader.IsLoading())
      m_musicThumbLoader.StopThread();
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
void CGUIWindowPlexSearch::Bind()
{
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
      SET_CONTROL_VISIBLE(controlID-2000);
    }
    else
    {
      SET_CONTROL_HIDDEN(controlID);
      SET_CONTROL_HIDDEN(controlID-2000);
    }
  }
  
  // Get thumbs.
  BOOST_FOREACH(int_list_pair pair, m_categoryResults)
    pair.second.loader->Load(*pair.second.list.get());
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
      SET_CONTROL_HIDDEN(controlID-2000);
    }
  }
  
  // Fix focus if needed.
  if (GetFocusedControlID() >= 9000)
  {
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), 70);
    OnMessage(msg);
  }
  
  // Reset results.
  BOOST_FOREACH(int_list_pair pair, m_categoryResults)
    pair.second.list->Clear();
}

///////////////////////////////////////////////////////////////////////////////
void CGUIWindowPlexSearch::StartSearch(const string& search)
{
  // Cancel pending requests.
  PlexSearchWorker::CancelPending();
  
  if (search.empty())
  {
    // Reset the results, we're not searching for anything at the moment.
    Reset();
  }
  else
  {
    // Issue the root of the new search, and note that when we receive results, clear out the old ones.
    PlexSearchWorker::Construct("http://localhost:32400/search", search);
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
    return 'A' + (iButton-65);
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
  if (iButtonControl == CTL_BUTTON_BACKSPACE)
  {
    Backspace();
  }
  else if (iButtonControl == CTL_BUTTON_CLEAR)
  {
    CGUILabelControl* pEdit = ((CGUILabelControl*)GetControl(CTL_LABEL_EDIT));
    if (pEdit)
    {
      m_strEdit = "";
      UpdateLabel();
      
      // Convert back to utf8.
      CStdString utf8Edit;
      g_charsetConverter.wToUTF8(m_strEdit, utf8Edit);
      pEdit->SetLabel(utf8Edit);
      
      // Reset cursor position.
      pEdit->SetCursorPos(0);
    }
  }
  else if (iButtonControl == CTL_BUTTON_SPACE)
  {
    Character(' ');
  }
  else
  {
    char ch = GetCharacter(iButtonControl);
    if (ch != 0)
    {
      // A keyboard button was pressed.
      Character(ch);
    }
    else
    {
      // Let's see if we're asked to play something.
      const CGUIControl* control = GetControl(iButtonControl);
      if (control->IsContainer())
      {
        CGUIBaseContainer* container = (CGUIBaseContainer* )control;
        CGUIListItemPtr item = container->GetListItem(0);
       
        if (item)
        {
          CFileItem* file = (CFileItem* )item.get();
          printf("Playing %s\n", item->GetLabel().c_str());
          
          // Save state.
          m_selectedContainerID = container->GetID();
          m_selectedItem = container->GetSelectedItem();
          
          // Now see what to do with it.
          string type = item->GetProperty("type");
          if (type == "show" || type == "person")
          {
            m_gWindowManager.ActivateWindow(WINDOW_VIDEO_FILES, file->m_strPath + ",return");
          }
          else if (type == "artist" || type == "album")
          {
            m_gWindowManager.ActivateWindow(WINDOW_MUSIC_FILES, file->m_strPath + ",return");
          }
          else if (type == "track")
          {
            // Get album.
            CFileItemList  fileItems;
            CPlexDirectory plexDir;
            plexDir.GetDirectory(file->GetProperty("parentPath"), fileItems);
            int itemIndex = -1;

            for (int i=0; i < fileItems.Size(); ++i)
            {
              CFileItemPtr fileItem = fileItems[i];
              if (fileItem->GetProperty("unprocessedKey") == file->GetProperty("unprocessedKey"))
              {
                itemIndex = i;
                break;
              }
            }
            
            g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
            g_playlistPlayer.Reset();
            g_playlistPlayer.Add(PLAYLIST_MUSIC, fileItems);
            g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
            g_playlistPlayer.Play(itemIndex);
          }
          else
          {
            bool resumeItem = false;
            
            if (!item->m_bIsFolder && item->HasProperty("viewOffset")) 
            {
              // Oh my god. Copy and paste code. We need a superclass which manages media.
              float seconds = boost::lexical_cast<int>(item->GetProperty("viewOffset")) / 1000.0f;
                
              vector<CStdString> choices;
              CStdString resumeString, time;
              StringUtils::SecondsToTimeString(seconds, time);
              resumeString.Format(g_localizeStrings.Get(12022).c_str(), time.c_str());
              choices.push_back(resumeString);
              choices.push_back(g_localizeStrings.Get(12021));
              int retVal = CGUIDialogContextMenu::ShowAndGetChoice(choices, CPoint(200, 100));
              if (!retVal)
                return;
              
              resumeItem = (retVal == 1);
              printf("RESUME ITEM: %d\n", resumeItem);
            }
            
            CFileItem* file = (CFileItem* )item.get();
            if (resumeItem)
              file->m_lStartOffset = STARTOFFSET_RESUME;
            else
              file->m_lStartOffset = 0;
            
            g_application.PlayFile(*(CFileItem* )item.get());
          }
        }
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
bool CGUIWindowPlexSearch::InProgress()
{
  return (PlexSearchWorker::PendingWorkers() > 0);
}
