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
#include "CocoaUtilsPlus.h"
#include "PlexSourceScanner.h"
#include "Util.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"

map<string, HostSourcesPtr> CPlexSourceScanner::g_hostSourcesMap;
CCriticalSection CPlexSourceScanner::g_lock;

void CPlexSourceScanner::Process()
{
  CStdString path;
  
  { // Make sure any existing entry is removed.
    CSingleLock lock(g_lock);
    g_hostSourcesMap.erase(m_host);
  }
  
  if (m_host.find("members.mac.com") != -1)
  {
    CLog::Log(LOGINFO, "Skipping MobileMe address: %s\n", m_host.c_str());
  }
  else if (Cocoa_IsHostLocal(m_host) == false)
  {
    // Create a new entry.
    HostSourcesPtr sources = HostSourcesPtr(new HostSources());
    CLog::Log(LOGINFO, "Scanning remote server: %s\n", m_host.c_str());
    
    // Scan the server.
    path.Format("plex://%s/music/", m_host);
    CUtil::AutodetectPlexSources(path, sources->musicSources, m_hostLabel, true);
    
    path.Format("plex://%s/video/", m_host);
    CUtil::AutodetectPlexSources(path, sources->videoSources, m_hostLabel, true);
    
    path.Format("plex://%s/photos/", m_host);
    CUtil::AutodetectPlexSources(path, sources->pictureSources, m_hostLabel, true);
    
    { // Add the entry to the map.
      CSingleLock lock(g_lock);
      g_hostSourcesMap[m_host] = sources;
    }
    
    // Notify the UI.
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_REMOTE_SOURCES);
    m_gWindowManager.SendThreadMessage(msg);
  
    CLog::Log(LOGINFO, "Scanning host %s is complete.\n", m_host.c_str());
  }
  else
  {
    CLog::Log(LOGINFO, "Ignoring local server: %s\n", m_host.c_str());
  }
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
  CLog::Log(LOGINFO, "Notifying remote remove host on %s\n", host.c_str());
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_REMOTE_SOURCES);
  m_gWindowManager.SendThreadMessage(msg);
  
  CGUIMessage msg2(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES);
  m_gWindowManager.SendThreadMessage(msg2);
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

