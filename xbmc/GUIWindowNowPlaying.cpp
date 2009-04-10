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

#include "stdafx.h"
#include "GUIWindowManager.h"

#include "GUIWindowNowPlaying.h"

CGUIWindowNowPlaying::CGUIWindowNowPlaying(void) : CGUIWindow(WINDOW_NOW_PLAYING, "NowPlaying.xml")
{
}

CGUIWindowNowPlaying::~CGUIWindowNowPlaying(void)
{
}

bool CGUIWindowNowPlaying::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PREVIOUS_MENU || action.wID == ACTION_PARENT_DIR)
  {
    m_gWindowManager.PreviousWindow();
    return true;
  }
  return false;
}
