/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
#include <vector>

#include "stdafx.h"
#include "FileSystem/File.h"
#include "GUIWindowHome.h"
#include "GUIDialogTimer.h"
#include "GUIDialogYesNo.h"
#include "GUIWindowManager.h"
#include "AlarmClock.h"
#include "GUIBaseContainer.h"
#include "FileItem.h"
#include "PlexSourceScanner.h"

#define MAIN_MENU         300
#define POWER_MENU        407

#define QUIT_ITEM         111
#define SLEEP_ITEM        112
#define SHUTDOWN_ITEM     113

using namespace std;
using namespace XFILE;

CGUIWindowHome::CGUIWindowHome(void) 
    : CGUIWindow(WINDOW_HOME, "Home.xml")
    , m_lastSelectedItem(-1)
{
}

CGUIWindowHome::~CGUIWindowHome(void)
{
}

bool CGUIWindowHome::OnAction(const CAction &action)
{
  if (action.wID == ACTION_CONTEXT_MENU)
  {
    return OnPopupMenu();
  }
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowHome::OnPopupMenu()
{
  int controlId = GetFocusedControl()->GetID();
  if (controlId == MAIN_MENU || controlId == POWER_MENU)
  {
    CGUIBaseContainer* pControl = (CGUIBaseContainer*)GetFocusedControl();
    CGUIListItemPtr pItem = pControl->GetListItem(pControl->GetSelectedItem());
    int itemId = pControl->GetSelectedItemID();

    float menuOffset = 0;
    int iHeading, iInfo, iAlreadySetMsg;
    CStdString sAlarmName, sAction;

    if (controlId == POWER_MENU) menuOffset = 180;
      
    switch (itemId) {
      case QUIT_ITEM:
        iHeading = 40300;
        iInfo = 40310;
        iAlreadySetMsg = 40320;
        sAlarmName = "plex_quit_timer";
        sAction = "quit";
        break;
      case SLEEP_ITEM:       
        iHeading = 40301;
        iInfo = 40311;
        iAlreadySetMsg = 40321;
        sAlarmName = "plex_sleep_timer";
        sAction = "sleepsystem";
        break;
      case SHUTDOWN_ITEM:
        iHeading = 40302;
        iInfo = 40312;
        iAlreadySetMsg = 40322;
        sAlarmName = "plex_shutdown_timer";
        sAction = "shutdownsystem";
        break;
      default:
        return false;
        break;
    }

    // Check to see if any timers already exist
    if (!CheckTimer("plex_quit_timer", sAlarmName, 40325, 40315, iAlreadySetMsg) ||
        !CheckTimer("plex_sleep_timer", sAlarmName, 40325, 40316, iAlreadySetMsg) ||
        !CheckTimer("plex_shutdown_timer", sAlarmName, 40325, 40317, iAlreadySetMsg))
      return false;
    

    int iTime;
    if (g_alarmClock.hasAlarm(sAlarmName))
    {
      iTime = (int)(g_alarmClock.GetRemaining(sAlarmName)/60);
      iHeading += 5; // Change the title to "Change" not "Set".
    }
    else
    {
      iTime = 0;
    }
    
    iTime = CGUIDialogTimer::ShowAndGetInput(iHeading, iInfo, iTime);
    
    // Dialog cancelled
    if (iTime == -1) return false;
    
    // If the alarm's already been set, cancel it
    if (g_alarmClock.hasAlarm(sAlarmName))
        g_alarmClock.stop(sAlarmName, false);

    // Start a new alarm
    if (iTime > 0)
        g_alarmClock.start(sAlarmName, iTime*60, sAction, false);
      
    // Focus the main menu again
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), MAIN_MENU);
    if(OwningCriticalSection(g_graphicsContext))
      CGUIWindow::OnMessage(msg);
    else
      m_gWindowManager.SendThreadMessage(msg, m_dwWindowId);
    
    return true;
    
    if (pControl->GetSelectedItemID() == 1)
    {
      g_alarmClock.start ("plex_quit_timer", 5, "ShutDown", false);      
    }
  }
  return false;
}

bool CGUIWindowHome::CheckTimer(const CStdString& strExisting, const CStdString& strNew, int title, int line1, int line2)
{
  bool bReturn;
  if (g_alarmClock.hasAlarm(strExisting) && strExisting != strNew)
    {
    if (CGUIDialogYesNo::ShowAndGetInput(title, line1, line2, 0, bReturn) == false)
    {
      return false;
    }
    else
    {
      g_alarmClock.stop(strExisting, false);  
      return true;
    }
  }
  else
    return true;
}

typedef pair<string, HostSourcesPtr> string_sources_pair;

static bool compare(CFileItemPtr first, CFileItemPtr second)
{
  return first->GetLabel().compare(second->GetLabel());
}

bool CGUIWindowHome::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() ==  GUI_MSG_WINDOW_DEINIT)
  {
    CGUIBaseContainer* control = (CGUIBaseContainer* )GetControl(MAIN_MENU);
    m_lastSelectedItem = control->GetSelectedItem();
  }
  
  bool ret = CGUIWindow::OnMessage(message);
  
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_INIT:
  case GUI_MSG_WINDOW_RESET:
  case GUI_MSG_UPDATE_MAIN_MENU:
  {
    // This will be our new list.
    vector<CGUIListItemPtr> newList;
    
    // Get the old list.
    CGUIBaseContainer* control = (CGUIBaseContainer* )GetControl(MAIN_MENU);
    if (control)
    {
      vector<CGUIListItemPtr>& oldList = control->GetStaticItems();
      
      // First collect all the real items.
      BOOST_FOREACH(CGUIListItemPtr item, oldList)
      {
        if (item->HasProperty("plex") == false)
          newList.push_back(item);
      }
      
      // Now collect all the added items.
      map<string, HostSourcesPtr>& map = CPlexSourceScanner::GetMap();
      list<CFileItemPtr> newItems;

      BOOST_FOREACH(string_sources_pair nameSource, map)
      {
        for (int i=0; i<nameSource.second->librarySections.Size(); i++)
          newItems.push_back(nameSource.second->librarySections[i]);
      }

      // Now sort them according to name.
      newItems.sort(compare);

      // Now add the new ones.
      int id = 1000;
      BOOST_FOREACH(CFileItemPtr item, newItems)
      {
        CFileItemPtr newItem = CFileItemPtr(new CFileItem(item->GetLabel()));
        newItem->SetProperty("plex", "1");
        newItem->m_strPath = "Plex.ActivateWindow(MyVideoFiles," + item->m_strPath + ",return)";
        newItem->m_idepth = 0;
        newItem->SetQuickFanart(item->GetQuickFanart());
        newItem->m_iprogramCount = id++;

        // Load and set fanart.
        newItem->CacheFanart();
        if (CFile::Exists(newItem->GetCachedProgramFanart()))
          newItem->SetProperty("fanart_image", newItem->GetCachedProgramFanart());

        newList.push_back(newItem);
      }

      // Replace 'em.
      control->GetStaticItems().clear();
      control->GetStaticItems().assign(newList.begin(), newList.end());
      
      // See if we have a selected item and restore it if we do.
      if (m_lastSelectedItem >= 0)
      {
        CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), control->GetID(), m_lastSelectedItem+1, 0);
        m_gWindowManager.SendThreadMessage(msg);
      }
    }
  }
  break;
  }
  
  return ret;
}





