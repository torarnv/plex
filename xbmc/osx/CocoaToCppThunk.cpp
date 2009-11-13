/*
 *  CocoaToCppThunk.cpp
 *  XBMC
 *
 *  Created by Elan Feingold on 2/27/2008.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */
#include "stdafx.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "AppleRemoteKeys.h"
#include "CocoaToCppThunk.h"
#include "HTTP.h"
#include "GUIDialogUtils.h"
#include "Util.h"
#include "Settings.h"
#include "CocoaUtilsPlus.h"
#include "GUIWindowManager.h"
#include "Thread.h"
#include "GUISettings.h"

void Cocoa_OnAppleRemoteKey(void* application, AppleRemoteEventIdentifier event, bool pressedDown, unsigned int count)
{
  CApplication* pApp = (CApplication* )application;
  printf("Remote Key: %d, down=%d, count=%d\n", event, pressedDown, count);
  
  switch (event)
  {
  case kRemoteButtonPlay:
    if (count >= 2)
    {
      CKey key(KEY_BUTTON_WHITE);
      pApp->OnKey(key);
    }
    else
    {
      CKey key(KEY_BUTTON_A);
      pApp->OnKey(key);
    }
    break;

  case kRemoteButtonVolume_Plus:
  {
    CKey key(KEY_BUTTON_DPAD_UP);
    pApp->OnKey(key);
  }
  break;

  case kRemoteButtonVolume_Minus:
  {
    CKey key(KEY_BUTTON_DPAD_DOWN);
    pApp->OnKey(key);
  }
  break;

  case kRemoteButtonRight:
  {
    CKey key(KEY_BUTTON_DPAD_RIGHT);
    pApp->OnKey(key);
  }
  break;

  case kRemoteButtonLeft:
  {
    CKey key(KEY_BUTTON_DPAD_LEFT);
    pApp->OnKey(key);
  }
  break;
  case kRemoteButtonRight_Hold:
  case kRemoteButtonLeft_Hold:
  case kRemoteButtonVolume_Plus_Hold:
  case kRemoteButtonVolume_Minus_Hold:
    /* simulate an event as long as the user holds the button */
    //b_remote_button_hold = pressedDown;
    //if( pressedDown )
    //{
    //    NSNumber* buttonIdentifierNumber = [NSNumber numberWithInt: buttonIdentifier];
    //    [self performSelector:@selector(executeHoldActionForRemoteButton:)
    //               withObject:buttonIdentifierNumber];
    //}
    break;
    
  case kRemoteButtonMenu:
  {
    if (pApp->IsPlayingFullScreenVideo() == false)
    {
      CKey key(KEY_BUTTON_B);
      pApp->OnKey(key);
    }
    else
    {
      CKey key(KEY_BUTTON_START);
      pApp->OnKey(key);
    }
  }
  break;
  
  case kRemoteButtonMenu_Hold:
  {
    if (pApp->IsPlayingFullScreenVideo() == true)
    {
      CKey key(KEY_BUTTON_B);
      pApp->OnKey(key);
    }
  }
  break;
    
  default:
    /* Add here whatever you want other buttons to do */
    break;
  }
}

void Cocoa_DownloadFile(const char* remoteFile, const char* localFile)
{
  using namespace std;
  CHTTP http;
  string fLocal = localFile;
  string fRemote = remoteFile;
  http.Download(fRemote, fLocal);
}

void Cocoa_CPPUpdateProgressDialog()
{
  ThreadMessage tMsg = {TMSG_GUI_UPDATE_COCOA_DIALOGS};
  g_application.getApplicationMessenger().SendMessage(tMsg);
}

#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include "CriticalSection.h"
#include "SingleLock.h"

class HostSources
{
 public:
  VECSOURCES videoSources;
  VECSOURCES musicSources;
  VECSOURCES pictureSources;
};
typedef boost::shared_ptr<HostSources> HostSourcesPtr;

////////////////////////////////////////////
class PlexSourceScanner : public CThread
{
 public:
 
  virtual void Process()
  {
    CStdString path;
    
    { // Make sure any existing entry is removed.
      CSingleLock lock(g_lock);
      g_hostSourcesMap.erase(m_host);
    }
    
    // Create a new entry.
    HostSourcesPtr sources = HostSourcesPtr(new HostSources());
    printf("Scanning host: %s\n", m_host.c_str());
    
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
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES);
    m_gWindowManager.SendThreadMessage(msg);
    
    printf("Scanning host %s is complete.\n", m_host.c_str());
  }
  
  static void ScanHost(const string& host, const string& hostLabel)
  {
    new PlexSourceScanner(host, hostLabel);
  }

  static void RemoveHost(const string& host)
  {
    { // Remove the entry from the map.
      CSingleLock lock(g_lock);
      g_hostSourcesMap.erase(host);
    }
    
    // Notify the UI.
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES);
    m_gWindowManager.SendThreadMessage(msg);
  }
  
  static void MergeSources(VECSOURCES& videoSources, VECSOURCES& musicSources, VECSOURCES& pictureSources)
  {
    // Remove remote sources...TBD...
    
    // Now add the current remote sources in.
    typedef pair<string, HostSourcesPtr> StringSourcesPair;
    BOOST_FOREACH(StringSourcesPair pair, g_hostSourcesMap)
    {
      MergeSource(videoSources, pair.second->videoSources);
      MergeSource(musicSources, pair.second->musicSources);
      MergeSource(pictureSources, pair.second->pictureSources);
    }
  }
  
 protected:
  
  static void MergeSource(VECSOURCES& sources, VECSOURCES& remoteSources)
  {
    BOOST_FOREACH(CMediaSource source, sources)
    {
      // If the source doesn't already exist, add it.
    }
  }
  
  PlexSourceScanner(const string& host, const string& hostLabel)
    : m_host(host)
    , m_hostLabel(hostLabel)
  {
    Create(true);
  }
 
 private:
  
  string m_host;
  string m_hostLabel;
  
  static map<string, HostSourcesPtr> g_hostSourcesMap;
  static CCriticalSection g_lock;
};

map<string, HostSourcesPtr> PlexSourceScanner::g_hostSourcesMap;
CCriticalSection PlexSourceScanner::g_lock;

void Cocoa_AutodetectRemotePlexSources(const char* hostName, const char* hostLabel)
{
  CStdString path;
  
  if (!Cocoa_AreHostsEqual(hostName, "localhost") && g_guiSettings.GetBool("servers.remoteautosource"))
  {
    PlexSourceScanner::ScanHost(hostName, hostLabel);
    
#if 0
    path.Format("plex://%s/music/", hostName);
    CUtil::AutodetectPlexSources(path, g_settings.m_musicSources, hostLabel, true);
    
    path.Format("plex://%s/video/", hostName);
    CUtil::AutodetectPlexSources(path, g_settings.m_videoSources, hostLabel, true);
    
    path.Format("plex://%s/photos/", hostName);
    CUtil::AutodetectPlexSources(path, g_settings.m_pictureSources, hostLabel, true);
    
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES);
    m_gWindowManager.SendThreadMessage(msg);
#endif
  }
}

void Cocoa_RemoveRemotePlexSources(const char* hostName)
{
  CStdString path;
  if (!Cocoa_AreHostsEqual(hostName, "localhost"))
  {
    PlexSourceScanner::RemoveHost(hostName);
    
#if 0
    path.Format("plex://%s/music/", hostName);
    CUtil::RemovePlexSources(path, g_settings.m_musicSources);
    
    path.Format("plex://%s/video/", hostName);
    CUtil::RemovePlexSources(path, g_settings.m_videoSources);
    
    path.Format("plex://%s/photos/", hostName);
    CUtil::RemovePlexSources(path, g_settings.m_pictureSources);
    
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES);
    m_gWindowManager.SendThreadMessage(msg);
#endif
  }
}
