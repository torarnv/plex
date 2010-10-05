/*
 *  PlexSourceScanner.cpp
 *  Plex
 *
 *  Created by Elan Feingold & James Clarke on 13/11/2009.
 *  Copyright 2009 Plex Development Team. All rights reserved.
 *
 */
#include "stdafx.h"
#include "Log.h"
#include "FileItem.h"
#include "CocoaUtilsPlus.h"
#include "PlexDirectory.h"
#include "PlexSourceScanner.h"
#include "Util.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"

map<string, HostSourcesPtr> CPlexSourceScanner::g_hostSourcesMap;
CCriticalSection CPlexSourceScanner::g_lock;
int CPlexSourceScanner::g_activeScannerCount = 0;

using namespace DIRECTORY; 

void CPlexSourceScanner::Process()
{
  CStdString path;
  
  CLog::Log(LOGNOTICE, "Plex Source Scanner starting...", m_host.c_str());
  
  { // Make sure any existing entry is removed.
    CSingleLock lock(g_lock);
    g_hostSourcesMap.erase(m_host);
    g_activeScannerCount++;
  }
  
  if (m_host.find("members.mac.com") != -1)
  {
    CLog::Log(LOGWARNING, "Skipping MobileMe address: %s", m_host.c_str());
  }
  else
  {
    // Compute the real host label (empty for local server).
    string realHostLabel = m_hostLabel;
    bool onlyShared = true;

    // Act a bit differently if we're talking to a local server.
    if (Cocoa_IsHostLocal(m_host) == true)
    {
      realHostLabel = "";
      onlyShared = false;
    }
    
    // Create a new entry.
    HostSourcesPtr sources = HostSourcesPtr(new HostSources());
    CLog::Log(LOGNOTICE, "Scanning remote server: %s", m_host.c_str());
    
    // Scan the server.
    path.Format("plex://%s/music/", m_host);
    CUtil::AutodetectPlexSources(path, sources->musicSources, realHostLabel, onlyShared);
    
    path.Format("plex://%s/video/", m_host);
    CUtil::AutodetectPlexSources(path, sources->videoSources, realHostLabel, onlyShared);
    
    path.Format("plex://%s/photos/", m_host);
    CUtil::AutodetectPlexSources(path, sources->pictureSources, realHostLabel, onlyShared);

    // Library sections.
    path.Format("plex://%s/library/sections", m_host);
    CPlexDirectory plexDir(true, false);
    plexDir.SetTimeout(5);
    sources->librarySections.ClearItems();
    plexDir.GetDirectory(path, sources->librarySections);
    
    // Edit for friendly name.
    for (int i=0; i<sources->librarySections.Size(); i++)
    {
      CFileItemPtr item = sources->librarySections[i];
      item->SetLabel2(m_hostLabel);
      CLog::Log(LOGNOTICE, " -> Section '%s' found.", item->GetLabel().c_str());
    }
    
    { // Add the entry to the map.
      CSingleLock lock(g_lock);
      g_hostSourcesMap[m_host] = sources;
    }
    
    // Notify the UI.
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_REMOTE_SOURCES);
    m_gWindowManager.SendThreadMessage(msg);
    
    // Notify the main menu.
    CGUIMessage msg2(GUI_MSG_UPDATE_MAIN_MENU, WINDOW_HOME, 300);
    m_gWindowManager.SendThreadMessage(msg2);
  
    CLog::Log(LOGNOTICE, "Scanning host %s is complete.", m_host.c_str());
  }
  
  CSingleLock lock(g_lock);
  g_activeScannerCount--;
  
  CLog::Log(LOGNOTICE, "Plex Source Scanner finished for host %s (%d left)", m_host.c_str(), g_activeScannerCount);
}

void CPlexSourceScanner::ScanHost(const string& host, const string& hostLabel)
{
  new CPlexSourceScanner(host, hostLabel);
}

void CPlexSourceScanner::RemoveHost(const string& host)
{
  { // Remove the entry from the map in case it was remote.
    CSingleLock lock(g_lock);
    g_hostSourcesMap.erase(host);
  }
  
  // Notify the UI.
  CLog::Log(LOGNOTICE, "Notifying remote remove host on %s", host.c_str());
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_REMOTE_SOURCES);
  m_gWindowManager.SendThreadMessage(msg);
  
  CGUIMessage msg2(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES);
  m_gWindowManager.SendThreadMessage(msg2);
  
  CGUIMessage msg3(GUI_MSG_UPDATE_MAIN_MENU, WINDOW_HOME, 300);
  m_gWindowManager.SendThreadMessage(msg3);
}

void CPlexSourceScanner::MergeSourcesForWindow(int windowId)
{
  CSingleLock lock(g_lock);
  switch (windowId) 
  {
    case WINDOW_MUSIC_FILES:
      BOOST_FOREACH(StringSourcesPair pair, g_hostSourcesMap)
        MergeSource(g_settings.m_musicSources, pair.second->musicSources);
      CheckForRemovedSources(g_settings.m_musicSources, windowId);
      break;
      
    case WINDOW_PICTURES:
      BOOST_FOREACH(StringSourcesPair pair, g_hostSourcesMap)
        MergeSource(g_settings.m_pictureSources, pair.second->pictureSources);
      CheckForRemovedSources(g_settings.m_pictureSources, windowId);
      break;
      
    case WINDOW_VIDEO_FILES:
      BOOST_FOREACH(StringSourcesPair pair, g_hostSourcesMap)
        MergeSource(g_settings.m_videoSources, pair.second->videoSources);
      CheckForRemovedSources(g_settings.m_videoSources, windowId);
      break;
      
    default:
      break;   
  }
}

void CPlexSourceScanner::MergeSource(VECSOURCES& sources, VECSOURCES& remoteSources)
{
  BOOST_FOREACH(CMediaSource source, remoteSources)
  {
    // If the source doesn't already exist, add it.
    bool bIsSourceName = true;
    if (CUtil::GetMatchingSource(source.strName, sources, bIsSourceName) < 0)
    {
      source.m_autoDetected = true;
      sources.push_back(source);
    }
  }
}

void CPlexSourceScanner::CheckForRemovedSources(VECSOURCES& sources, int windowId)
{
  VECSOURCES::iterator iterSources = sources.begin();
  while (iterSources != sources.end()) 
  {
    CMediaSource source = *iterSources;
    bool bFound = true;
    if (source.m_autoDetected)
    {
      bool bIsSourceName = true;
      bFound = false;
      BOOST_FOREACH(StringSourcesPair pair, g_hostSourcesMap)
      {
        VECSOURCES remoteSources;
        switch (windowId) 
        {
          case WINDOW_MUSIC_FILES:
            remoteSources = pair.second->musicSources;
            break;
          case WINDOW_PICTURES:
            remoteSources = pair.second->pictureSources;
            break;
          case WINDOW_VIDEO_FILES:
            remoteSources = pair.second->videoSources;
            break;
          default:
            return;
        }
        
        if (CUtil::GetMatchingSource(source.strName, remoteSources, bIsSourceName) >= 0)
          bFound = true;
      }
    }
    
    if (!bFound)
      sources.erase(iterSources);
    else
      ++iterSources;
  }  
}

