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

#include "stdafx.h"
#include "GUIViewStateVideo.h"
#include "PlayListPlayer.h"
#include "FileSystem/PluginDirectory.h"
#include "GUIBaseContainer.h"
#include "VideoDatabase.h"
#include "Settings.h"
#include "FileItem.h"
#include "Util.h"
#include "CocoaUtilsPlus.h"
#include "PlexDirectory.h"

using namespace DIRECTORY;

CStdString CGUIViewStateWindowVideo::GetLockType()
{
  return "video";
}

bool CGUIViewStateWindowVideo::UnrollArchives()
{
  return g_guiSettings.GetBool("filelists.unrollarchives");
}

CStdString CGUIViewStateWindowVideo::GetExtensions()
{
  return g_stSettings.m_videoExtensions;
}

int CGUIViewStateWindowVideo::GetPlaylist()
{
  return PLAYLIST_VIDEO;
}

CGUIViewStateWindowVideoFiles::CGUIViewStateWindowVideoFiles(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS()); // Preformated
    AddSortMethod(SORT_METHOD_DRIVE_TYPE, 564, LABEL_MASKS()); // Preformated
    SetSortMethod(SORT_METHOD_LABEL);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SORT_ORDER_ASC);
  }
  else
  {
    if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
      AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // FileName, Size | Foldername, empty
    else
      AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // FileName, Size | Foldername, empty
    AddSortMethod(SORT_METHOD_SIZE, 553, LABEL_MASKS("%L", "%I", "%L", "%I"));  // FileName, Size | Foldername, Size
    AddSortMethod(SORT_METHOD_DATE, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // FileName, Date | Foldername, Date
    AddSortMethod(SORT_METHOD_FILE, 561, LABEL_MASKS("%L", "%I", "%L", ""));  // Filename, Size | FolderName, empty
    SetSortMethod((SORT_METHOD)g_guiSettings.GetInt("myvideos.sortmethod"));

    SetViewAsControl(g_guiSettings.GetInt("myvideos.viewmode"));

    SetSortOrder((SORT_ORDER)g_guiSettings.GetInt("myvideos.sortorder"));
  }
  LoadViewState(items.m_strPath, WINDOW_VIDEO_FILES);
}

void CGUIViewStateWindowVideoFiles::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_VIDEO_FILES);
}

VECSOURCES& CGUIViewStateWindowVideoFiles::GetSources()
{
  bool bIsSourceName = true;
  // plugins share
  if (CPluginDirectory::HasPlugins("video"))
  {
    CMediaSource share;
    share.strName = g_localizeStrings.Get(1037);
    share.strPath = "plugin://video/";
    if (CUtil::GetMatchingSource(share.strName, g_settings.m_videoSources, bIsSourceName) < 0)
      g_settings.m_videoSources.push_back(share);
  }
  return g_settings.m_videoSources; 
}

CGUIViewStateWindowVideoNav::CGUIViewStateWindowVideoNav(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  if (items.IsVirtualDirectoryRoot())
  {
    AddSortMethod(SORT_METHOD_NONE, 551, LABEL_MASKS("%F", "%I", "%L", ""));  // Filename, Size | Foldername, empty
    SetSortMethod(SORT_METHOD_NONE);

    SetViewAsControl(DEFAULT_VIEW_LIST);

    SetSortOrder(SORT_ORDER_NONE);
  }
  {
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%F", "%D", "%L", ""));  // Filename, Duration | Foldername, empty
    SetSortMethod(SORT_METHOD_LABEL);

    SetViewAsControl(DEFAULT_VIEW_LIST);

  }
  LoadViewState(items.m_strPath, WINDOW_VIDEO_NAV);
}

void CGUIViewStateWindowVideoNav::SaveViewState()
{
}

VECSOURCES& CGUIViewStateWindowVideoNav::GetSources()
{
  //  Setup shares we want to have
  m_sources.clear();
  //  Musicdb shares
  CFileItemList items;
  CDirectory::GetDirectory("videodb://", items);
  for (int i=0; i<items.Size(); ++i)
  {
    CFileItemPtr item=items[i];
    CMediaSource share;
    share.strName=item->GetLabel();
    share.strPath = item->m_strPath;
    share.m_strThumbnailImage= item->HasThumbnail() ? item->GetThumbnailImage() : "defaultFolderBig.png";
    share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
    m_sources.push_back(share);
  }

  //  Playlists share
  CMediaSource share;
  share.strName=g_localizeStrings.Get(136); // Playlists
  share.strPath = "special://videoplaylists/";
  share.m_strThumbnailImage="defaultFolderBig.png";
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
  m_sources.push_back(share);

  // plugins share
  if (CPluginDirectory::HasPlugins("video"))
  {
    share.strName = g_localizeStrings.Get(1037);
    share.strPath = "plugin://video/";
    m_sources.push_back(share);
  }

  return CGUIViewStateWindowVideo::GetSources();
}

bool CGUIViewStateWindowVideoNav::AutoPlayNextItem()
{
  return false;
}

CGUIViewStateWindowVideoPlaylist::CGUIViewStateWindowVideoPlaylist(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  AddSortMethod(SORT_METHOD_NONE, 551, LABEL_MASKS("%L", "", "%L", ""));  // Label, "" | Label, empty
  SetSortMethod(SORT_METHOD_NONE);

  SetViewAsControl(DEFAULT_VIEW_LIST);

  SetSortOrder(SORT_ORDER_NONE);

  LoadViewState(items.m_strPath, WINDOW_VIDEO_PLAYLIST);
}

void CGUIViewStateWindowVideoPlaylist::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_VIDEO_PLAYLIST);
}

bool CGUIViewStateWindowVideoPlaylist::HideExtensions()
{
  return true;
}

bool CGUIViewStateWindowVideoPlaylist::HideParentDirItems()
{
  return true;
}

VECSOURCES& CGUIViewStateWindowVideoPlaylist::GetSources()
{
  m_sources.clear();
  //  Playlist share
  CMediaSource share;
  share.strPath= "playlistvideo://";
  share.m_strThumbnailImage="defaultFolderBig.png";
  share.m_iDriveType = CMediaSource::SOURCE_TYPE_LOCAL;
  m_sources.push_back(share);

  return CGUIViewStateWindowVideo::GetSources();
}


CGUIViewStateVideoMovies::CGUIViewStateVideoMovies(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%T", "%R"));  // Filename, Duration | Foldername, empty
  else
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%T", "%R"));  // Filename, Duration | Foldername, empty
  AddSortMethod(SORT_METHOD_VIDEO_RATING, 563, LABEL_MASKS("%T", "%R"));  // Filename, Duration | Foldername, empty
  AddSortMethod(SORT_METHOD_YEAR,562, LABEL_MASKS("%T", "%Y"));

  if (items.IsSmartPlayList())
    AddSortMethod(SORT_METHOD_PLAYLIST_ORDER, 559, LABEL_MASKS("%T", "%R"));

  SetSortMethod(g_stSettings.m_viewStateVideoNavTitles.m_sortMethod);

  SetViewAsControl(g_stSettings.m_viewStateVideoNavTitles.m_viewMode);

  SetSortOrder(g_stSettings.m_viewStateVideoNavTitles.m_sortOrder);
}

void CGUIViewStateVideoMovies::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_VIDEO_NAV, g_stSettings.m_viewStateVideoNavTitles);
}


CGUIViewStateVideoMusicVideos::CGUIViewStateVideoMusicVideos(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 556, LABEL_MASKS("%T", "%Y"));  // Filename, Duration | Foldername, empty
  else
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%T", "%Y"));  // Filename, Duration | Foldername, empty
  AddSortMethod(SORT_METHOD_YEAR,562, LABEL_MASKS("%T", "%Y"));
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
  {
    AddSortMethod(SORT_METHOD_ARTIST_IGNORE_THE,557, LABEL_MASKS("%A - %T", "%Y"));
    AddSortMethod(SORT_METHOD_ALBUM_IGNORE_THE,558, LABEL_MASKS("%B - %T", "%Y"));
  }
  else
  {
    AddSortMethod(SORT_METHOD_ARTIST,557, LABEL_MASKS("%A - %T", "%Y"));
    AddSortMethod(SORT_METHOD_ALBUM,558, LABEL_MASKS("%B - %T", "%Y"));
  }
  
  SetSortMethod(g_stSettings.m_viewStateVideoNavMusicVideos.m_sortMethod);

  SetViewAsControl(g_stSettings.m_viewStateVideoNavMusicVideos.m_viewMode);

  SetSortOrder(g_stSettings.m_viewStateVideoNavMusicVideos.m_sortOrder);
}

void CGUIViewStateVideoMusicVideos::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_VIDEO_NAV, g_stSettings.m_viewStateVideoNavMusicVideos);
}


CGUIViewStateVideoTVShows::CGUIViewStateVideoTVShows(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%L", "%M", "%L", "%M"));  // Filename, Duration | Foldername, empty
  else
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%L", "%M", "%L", "%M"));  // Filename, Duration | Foldername, empty

  AddSortMethod(SORT_METHOD_YEAR,562,LABEL_MASKS("%L","%Y","%L","%Y"));

  if (items.IsSmartPlayList())
    AddSortMethod(SORT_METHOD_PLAYLIST_ORDER, 559, LABEL_MASKS("%L", "%M", "%L", "%M"));

  SetSortMethod(g_stSettings.m_viewStateVideoNavTvShows.m_sortMethod);

  SetViewAsControl(g_stSettings.m_viewStateVideoNavTvShows.m_viewMode);

  SetSortOrder(g_stSettings.m_viewStateVideoNavTvShows.m_sortOrder);
}

void CGUIViewStateVideoTVShows::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_VIDEO_NAV, g_stSettings.m_viewStateVideoNavTvShows);
}


CGUIViewStateVideoEpisodes::CGUIViewStateVideoEpisodes(const CFileItemList& items) : CGUIViewStateWindowVideo(items)
{
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting"))
    AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%T","%R"));  // Filename, Duration | Foldername, empty
  else
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%T", "%R"));  // Filename, Duration | Foldername, empty
  if (0)//params.GetSeason() > -1)
  {
    AddSortMethod(SORT_METHOD_VIDEO_RATING, 563, LABEL_MASKS("%E. %T", "%R"));  // Filename, Duration | Foldername, empty
    AddSortMethod(SORT_METHOD_EPISODE,20359,LABEL_MASKS("%E. %T","%R"));
    AddSortMethod(SORT_METHOD_PRODUCTIONCODE,20368,LABEL_MASKS("%E. %T","%P", "%E. %T","%P"));
    AddSortMethod(SORT_METHOD_DATE,552,LABEL_MASKS("%E. %T","%J","E. %T","%J"));
  }
  else
  {
    AddSortMethod(SORT_METHOD_VIDEO_RATING, 563, LABEL_MASKS("%H. %T", "%R"));  // Filename, Duration | Foldername, empty
    AddSortMethod(SORT_METHOD_EPISODE,20359,LABEL_MASKS("%H. %T","%R"));
    AddSortMethod(SORT_METHOD_PRODUCTIONCODE,20368,LABEL_MASKS("%H. %T","%P", "%H. %T","%P"));
    AddSortMethod(SORT_METHOD_DATE,552,LABEL_MASKS("%H. %T","%J","%H. %T","%J"));
  }

  SetSortMethod(g_stSettings.m_viewStateVideoNavEpisodes.m_sortMethod);

  SetViewAsControl(g_stSettings.m_viewStateVideoNavEpisodes.m_viewMode);

  SetSortOrder(g_stSettings.m_viewStateVideoNavEpisodes.m_sortOrder);
}

void CGUIViewStateVideoEpisodes::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, WINDOW_VIDEO_NAV, g_stSettings.m_viewStateVideoNavEpisodes);
}

