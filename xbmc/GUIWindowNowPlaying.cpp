/*
 *      Copyright (C) 2009 Plex
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
#include "stdafx.h"
#include "FileItem.h"
#include "GUIWindowManager.h"
#include "GUIInfoManager.h"
#include "PlayListPlayer.h"
#include "PlayList.h"

#include "GUIWindowNowPlaying.h"

#define NOW_PLAYING_FLIP_TIME 120

using namespace PLAYLIST;

CGUIWindowNowPlaying::CGUIWindowNowPlaying() 
  : CGUIWindow(WINDOW_NOW_PLAYING, "NowPlaying.xml")
  , m_thumbLoader(1, 200)
{
}

CGUIWindowNowPlaying::~CGUIWindowNowPlaying()
{
}

bool CGUIWindowNowPlaying::OnAction(const CAction &action)
{
  CStdString strAction = action.strAction;
  strAction = strAction.ToLower();
  
  if (action.wID == ACTION_PREVIOUS_MENU || action.wID == ACTION_PARENT_DIR)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }
  else if (action.wID == ACTION_CONTEXT_MENU || action.wID == ACTION_SHOW_INFO)
  {
    return true;
  }
  else if (action.wID == ACTION_SHOW_GUI)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }
  else if (strAction == "activatewindow(playercontrols)")
  {
    return true;
  }
  
  return false;
}

bool CGUIWindowNowPlaying::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_thumbLoader.IsLoading())
        m_thumbLoader.StopThread();
      m_flipTimer.Stop();
    }
    break;

    case GUI_MSG_WINDOW_INIT:
    {
      CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
      CFileItemList list;
      
      for (int i=0; i<playlist.size(); i++)
        list.Add(playlist[i]);
      
      m_thumbLoader.Load(list);
      
      g_infoManager.m_nowPlayingFlipped = false;
      m_flipTimer.StartZero();
    }
    break;
  }
  
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowNowPlaying::Render()
{
  if (m_flipTimer.GetElapsedSeconds() >= NOW_PLAYING_FLIP_TIME)
  {
    g_infoManager.m_nowPlayingFlipped = !g_infoManager.m_nowPlayingFlipped;
    m_flipTimer.Reset();
  }
  CGUIWindow::Render();
}