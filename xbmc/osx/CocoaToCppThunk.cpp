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

void Cocoa_AutodetectRemotePlexSources(const char* hostName, const char* hostLabel)
{
  CStdString path;
  
  if (!Cocoa_AreHostsEqual(hostName, "localhost") && g_guiSettings.GetBool("servers.remoteautosource"))
  {
    path.Format("plex://%s/music/", hostName);
    CUtil::AutodetectPlexSources(path, g_settings.m_musicSources, hostLabel, true);
    
    path.Format("plex://%s/video/", hostName);
    CUtil::AutodetectPlexSources(path, g_settings.m_videoSources, hostLabel, true);
    
    path.Format("plex://%s/photos/", hostName);
    CUtil::AutodetectPlexSources(path, g_settings.m_pictureSources, hostLabel, true);
    
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES);
    m_gWindowManager.SendThreadMessage(msg);
  }
}

void Cocoa_RemoveRemotePlexSources(const char* hostName)
{
  CStdString path;
  if (!Cocoa_AreHostsEqual(hostName, "localhost"))
  {
    path.Format("plex://%s/music/", hostName);
    CUtil::RemovePlexSources(path, g_settings.m_musicSources);
    
    path.Format("plex://%s/video/", hostName);
    CUtil::RemovePlexSources(path, g_settings.m_videoSources);
    
    path.Format("plex://%s/photos/", hostName);
    CUtil::RemovePlexSources(path, g_settings.m_pictureSources);
    
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_UPDATE_SOURCES);
    m_gWindowManager.SendThreadMessage(msg);
  }
}