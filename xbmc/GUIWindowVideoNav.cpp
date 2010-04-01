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
#include "GUIWindowVideoNav.h"
#include "GUIWindowVideoFiles.h"
#include "utils/GUIInfoManager.h"
#include "Util.h"
#include "utils/RegExp.h"
#include "PlayListM3U.h"
#include "PlayListPlayer.h"
#include "GUIPassword.h"
#include "GUILabelControl.h"
#include "GUIDialogFileBrowser.h"
#include "Picture.h"
#include "PlayListFactory.h"
#include "GUIFontManager.h"
#include "GUIDialogOK.h"
#include "PartyModeManager.h"
#include "MusicDatabase.h"
#include "GUIWindowManager.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogSelect.h"
#include "GUIDialogKeyboard.h"
#include "FileSystem/Directory.h"
#include "FileSystem/File.h"
#include "FileItem.h"
#include "Application.h"
#include "CocoaUtils.h"

using namespace XFILE;
using namespace DIRECTORY;
using namespace std;

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_BTNTYPE            5
#define CONTROL_BTNSEARCH          8
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
#define CONTROL_BIGLIST           52
#define CONTROL_LABELFILES        12

#define CONTROL_BTN_FILTER        19
#define CONTROL_BTNSHOWMODE       10
#define CONTROL_BTNSHOWALL        14
#define CONTROL_UNLOCK            11

#define CONTROL_FILTER            15
#define CONTROL_BTNPARTYMODE      16
#define CONTROL_BTNFLATTEN        17
#define CONTROL_LABELEMPTY        18

CGUIWindowVideoNav::CGUIWindowVideoNav(void)
    : CGUIWindowVideoBase(WINDOW_VIDEO_NAV, "MyVideoNav.xml")
{
  m_vecItems->m_strPath = "?";
  m_bDisplayEmptyDatabaseMessage = false;
  m_thumbLoader.SetObserver(this);
  m_unfilteredItems = new CFileItemList;
}

CGUIWindowVideoNav::~CGUIWindowVideoNav(void)
{
  delete m_unfilteredItems;
}

bool CGUIWindowVideoNav::OnAction(const CAction &action)
{
  if (action.wID == ACTION_PREVIOUS_MENU)
    Cocoa_SetBackgroundMusicThemeId(NULL);

  if (action.wID == ACTION_PARENT_DIR)
  {
    if (g_advancedSettings.m_bUseEvilB && m_vecItems->m_strPath == m_startDirectory)
    {
      m_history.ClearPathHistory();
      m_vecItems->m_strPath = "?";
      m_gWindowManager.PreviousWindow();
      return true;
    }
  }
  if (action.wID == ACTION_TOGGLE_WATCHED)
  {
    CFileItemPtr pItem = m_vecItems->Get(m_viewControl.GetSelectedItem());
    if (pItem->GetVideoInfoTag()->m_playCount == 0)
      return OnContextButton(m_viewControl.GetSelectedItem(),CONTEXT_BUTTON_MARK_WATCHED);
    if (pItem->GetVideoInfoTag()->m_playCount > 0)
      return OnContextButton(m_viewControl.GetSelectedItem(),CONTEXT_BUTTON_MARK_UNWATCHED);
  }
  return CGUIWindowVideoBase::OnAction(action);
}

bool CGUIWindowVideoNav::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_RESET:
    m_vecItems->m_strPath = "?";
    break;
  case GUI_MSG_WINDOW_DEINIT:
    if (m_thumbLoader.IsLoading())
      m_thumbLoader.StopThread();
    break;
  case GUI_MSG_WINDOW_INIT:
    {
/* We don't want to show Autosourced items (ie removable pendrives, memorycards) in Library mode */
      m_rootDir.AllowNonLocalSources(false);
      // check for valid quickpath parameter
      CStdStringArray params;
      StringUtils::SplitString(message.GetStringParam(), ",", params);
      bool returning = params.size() > 1 && params[1] == "return";

      CStdString strDestination = params.size() ? params[0] : "";
      if (!strDestination.IsEmpty())
      {
        message.SetStringParam("");
        CLog::Log(LOGINFO, "Attempting to %s to: %s", returning ? "return" : "quickpath", strDestination.c_str());
      }

      // is this the first time the window is opened?
      if (m_vecItems->m_strPath == "?" && strDestination.IsEmpty())
      {
        strDestination = g_settings.m_defaultVideoLibSource;
        m_vecItems->m_strPath = strDestination;
        CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
      }

      CStdString destPath;
      if (!strDestination.IsEmpty())
      {
        if (strDestination.Equals("$ROOT") || strDestination.Equals("Root"))
          destPath = "";
        else if (strDestination.Equals("MovieGenres"))
          destPath = "videodb://1/1/";
        else if (strDestination.Equals("MovieTitles"))
          destPath = "videodb://1/2/";
        else if (strDestination.Equals("MovieYears"))
          destPath = "videodb://1/3/";
        else if (strDestination.Equals("MovieActors"))
          destPath = "videodb://1/4/";
        else if (strDestination.Equals("MovieDirectors"))
          destPath = "videodb://1/5/";
        else if (strDestination.Equals("MovieStudios"))
          destPath = "videodb://1/6/";
        else if (strDestination.Equals("Movies"))
          destPath = "videodb://1/";
        else if (strDestination.Equals("TvShowGenres"))
          destPath = "videodb://2/1/";
        else if (strDestination.Equals("TvShowTitles"))
          destPath = "videodb://2/2/";
        else if (strDestination.Equals("TvShowYears"))
          destPath = "videodb://2/3/";
        else if (strDestination.Equals("TvShowActors"))
          destPath = "videodb://2/4/";
        else if (strDestination.Equals("TvShows"))
          destPath = "videodb://2/";
        else if (strDestination.Equals("MusicVideoGenres"))
          destPath = "videodb://3/1/";
        else if (strDestination.Equals("MusicVideoTitles"))
          destPath = "videodb://3/2/";
        else if (strDestination.Equals("MusicVideoYears"))
          destPath = "videodb://3/3/";
        else if (strDestination.Equals("MusicVideoArtists"))
          destPath = "videodb://3/4/";
        else if (strDestination.Equals("MusicVideoDirectors"))
          destPath = "videodb://3/5/";
        else if (strDestination.Equals("MusicVideoStudios"))
          destPath = "videodb://3/6/";
        else if (strDestination.Equals("MusicVideos"))
          destPath = "videodb://3/";
        else if (strDestination.Equals("RecentlyAddedMovies"))
          destPath = "videodb://4/";
        else if (strDestination.Equals("RecentlyAddedEpisodes"))
          destPath = "videodb://5/";
        else if (strDestination.Equals("RecentlyAddedMusicVideos"))
          destPath = "videodb://6/";
        else if (strDestination.Equals("Playlists"))
          destPath = "special://videoplaylists/";
        else if (strDestination.Equals("Plugins"))
          destPath = "plugin://video/";
        else
        {
          CLog::Log(LOGWARNING, "Warning, destination parameter (%s) may not be valid", strDestination.c_str());
          destPath = strDestination;
        }
        if (!returning || m_vecItems->m_strPath.Left(destPath.GetLength()) != destPath)
        { // we're not returning to the same path, so set our directory to the requested path
          m_vecItems->m_strPath = destPath;
        }
        SetHistoryForPath(m_vecItems->m_strPath);
      }

      DisplayEmptyDatabaseMessage(false); // reset message state

      if (!CGUIWindowVideoBase::OnMessage(message))
        return false;

      if (message.GetParam1() != WINDOW_INVALID)
      { // first time to this window - make sure we set the root path
        m_startDirectory = returning ? destPath : "";
      }

      //  base class has opened the database, do our check
      m_database.Open();
      DisplayEmptyDatabaseMessage(!m_database.HasContent());

      if (m_bDisplayEmptyDatabaseMessage)
      {
        // no library - make sure we focus on a known control, and default to the root.
        SET_CONTROL_FOCUS(CONTROL_BTNTYPE, 0);
        m_vecItems->m_strPath = "";
        SetHistoryForPath("");
        Update("");
      }

      m_database.Close();
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTNPARTYMODE)
      {
        if (g_partyModeManager.IsEnabled())
          g_partyModeManager.Disable();
        else
        {
          if (!g_partyModeManager.Enable(PARTYMODECONTEXT_VIDEO))
          {
            SET_CONTROL_SELECTED(GetID(),CONTROL_BTNPARTYMODE,false);
            return false;
          }

          // Playlist directory is the root of the playlist window
          if (m_guiState.get()) m_guiState->SetPlaylistDirectory("playlistvideo://");

          return true;
        }
        UpdateButtons();
      }

      if (iControl == CONTROL_BTNSEARCH)
      {
        OnSearch();
      }
      else if (iControl == CONTROL_BTN_FILTER)
      {
        if (m_filter.IsEmpty())
          CGUIDialogKeyboard::ShowAndGetFilter(m_filter, false);
        else
        {
          m_filter.Empty();
          OnFilterItems();
        }
        return true;
      }
      else if (iControl == CONTROL_BTNSHOWMODE)
      {
        g_stSettings.m_iMyVideoWatchMode++;
        if (g_stSettings.m_iMyVideoWatchMode > VIDEO_SHOW_WATCHED)
          g_stSettings.m_iMyVideoWatchMode = VIDEO_SHOW_ALL;
        g_settings.Save();
        // TODO: Can we perhaps filter this directly?  Probably not for some of the more complicated views,
        //       but for those perhaps we can just display them all, and only filter when we get a list
        //       of actual videos?
        Update(m_vecItems->m_strPath);
        return true;
      }
      else if (iControl == CONTROL_BTNFLATTEN)
      {
        g_stSettings.m_bMyVideoNavFlatten = !g_stSettings.m_bMyVideoNavFlatten;
        g_settings.Save();
        CUtil::DeleteVideoDatabaseDirectoryCache();
        SetupShares();
        Update("");
        return true;
      }
      else if (iControl == CONTROL_BTNSHOWALL)
      {
        if (g_stSettings.m_iMyVideoWatchMode == VIDEO_SHOW_ALL)
          g_stSettings.m_iMyVideoWatchMode = VIDEO_SHOW_UNWATCHED;
        else
          g_stSettings.m_iMyVideoWatchMode = VIDEO_SHOW_ALL;
        g_settings.Save();
        // TODO: Can we perhaps filter this directly?  Probably not for some of the more complicated views,
        //       but for those perhaps we can just display them all, and only filter when we get a list
        //       of actual videos?
        Update(m_vecItems->m_strPath);
        return true;
      }
    }
    break;
    // update the display
    case GUI_MSG_SCAN_FINISHED:
    case GUI_MSG_REFRESH_THUMBS:
    {
      Update(m_vecItems->m_strPath);
    }
    break;

  case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_FILTER_ITEMS && IsActive())
      {
        if (message.GetParam2() == 1)  // append
          m_filter += message.GetStringParam();
        else if (message.GetParam2() == 2) // delete
        {
          if (m_filter.size())
            m_filter.erase(m_filter.end() - 1);
        }
        else
          m_filter = message.GetStringParam();
        OnFilterItems();
      }
    }
    break;
  }
  return CGUIWindowVideoBase::OnMessage(message);
}

CStdString CGUIWindowVideoNav::GetQuickpathName(const CStdString& strPath) const
{
  if (strPath.Equals("videodb://1/1/"))
    return "MovieGenres";
  else if (strPath.Equals("videodb://1/2/"))
    return "MovieTitles";
  else if (strPath.Equals("videodb://1/3/"))
    return "MovieYears";
  else if (strPath.Equals("videodb://1/4/"))
    return "MovieActors";
  else if (strPath.Equals("videodb://1/5/"))
    return "MovieDirectors";
  else if (strPath.Equals("videodb://1/"))
    return "Movies";
  else if (strPath.Equals("videodb://2/1/"))
    return "TvShowGenres";
  else if (strPath.Equals("videodb://2/2/"))
    return "TvShowTitles";
  else if (strPath.Equals("videodb://2/3/"))
    return "TvShowYears";
  else if (strPath.Equals("videodb://2/4/"))
    return "TvShowActors";
  else if (strPath.Equals("videodb://2/"))
    return "TvShows";
  else if (strPath.Equals("videodb://3/1/"))
    return "MusicVideoGenres";
  else if (strPath.Equals("videodb://3/2/"))
    return "MusicVideoTitles";
  else if (strPath.Equals("videodb://3/3/"))
    return "MusicVideoYears";
  else if (strPath.Equals("videodb://3/4/"))
    return "MusicVideoArtists";
  else if (strPath.Equals("videodb://3/5/"))
    return "MusicVideoDirectors";
  else if (strPath.Equals("videodb://3/"))
    return "MusicVideos";
  else if (strPath.Equals("videodb://4/"))
    return "RecentlyAddedMovies";
  else if (strPath.Equals("videodb://5/"))
    return "RecentlyAddedEpisodes";
  else if (strPath.Equals("videodb://6/"))
    return "RecentlyAddedMusicVideos";
  else if (strPath.Equals("special://videoplaylists/"))
    return "Playlists";
  else
  {
    CLog::Log(LOGERROR, "  CGUIWindowVideoNav::GetQuickpathName: Unknown parameter (%s)", strPath.c_str());
    return strPath;
  }
}

bool CGUIWindowVideoNav::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (m_bDisplayEmptyDatabaseMessage)
    return true;

  CFileItem directory(strDirectory, true);

  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  m_rootDir.SetCacheDirectory(false);
  items.ClearProperties();

  bool bResult = CGUIWindowVideoBase::GetDirectory(strDirectory, items);

  m_filter.Empty();
  return bResult;
}

void CGUIWindowVideoNav::UpdateButtons()
{
  CGUIWindowVideoBase::UpdateButtons();
}

/// \brief Search for genres, artists and albums with search string \e strSearch in the musicdatabase and return the found \e items
/// \param strSearch The search string
/// \param items Items Found
void CGUIWindowVideoNav::DoSearch(const CStdString& strSearch, CFileItemList& items)
{
  // get matching genres
  CFileItemList tempItems;

  m_database.GetMovieGenresByName(strSearch, tempItems);
  if (tempItems.Size())
  {
    CStdString strGenre = g_localizeStrings.Get(515); // Genre
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + strGenre + " - "+g_localizeStrings.Get(20342)+"] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetTvShowGenresByName(strSearch, tempItems);
  if (tempItems.Size())
  {
    CStdString strGenre = g_localizeStrings.Get(515); // Genre
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + strGenre + " - "+g_localizeStrings.Get(20343)+"] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetMusicVideoGenresByName(strSearch, tempItems);
  if (tempItems.Size())
  {
    CStdString strGenre = g_localizeStrings.Get(515); // Genre
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + strGenre + " - "+g_localizeStrings.Get(20389)+"] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetMusicVideoGenresByName(strSearch, tempItems);
  if (tempItems.Size())
  {
    CStdString strGenre = g_localizeStrings.Get(515); // Genre
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + strGenre + " - "+g_localizeStrings.Get(20389)+"] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetMovieActorsByName(strSearch, tempItems);
  if (tempItems.Size())
  {
    CStdString strActor = g_localizeStrings.Get(20337); // Actor
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + strActor + " - "+g_localizeStrings.Get(20342)+"] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetTvShowsActorsByName(strSearch, tempItems);
  if (tempItems.Size())
  {
    CStdString strActor = g_localizeStrings.Get(20337); // Actor
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + strActor + " - "+g_localizeStrings.Get(20343)+"] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetMusicVideoArtistsByName(strSearch, tempItems);
  if (tempItems.Size())
  {
    CStdString strActor = g_localizeStrings.Get(557); // Artist
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + strActor + " - "+g_localizeStrings.Get(20389)+"] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetMovieDirectorsByName(strSearch, tempItems);
  if (tempItems.Size())
  {
    CStdString strMovie = g_localizeStrings.Get(20339); // Director
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + strMovie + " - "+g_localizeStrings.Get(20342)+"] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetTvShowsDirectorsByName(strSearch, tempItems);
  if (tempItems.Size())
  {
    CStdString strMovie = g_localizeStrings.Get(20339); // Director
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + strMovie + " - "+g_localizeStrings.Get(20343)+"] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetMusicVideoDirectorsByName(strSearch, tempItems);
  if (tempItems.Size())
  {
    CStdString strMovie = g_localizeStrings.Get(20339); // Director
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + strMovie + " - "+g_localizeStrings.Get(20389)+"] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetMoviesByName(strSearch, tempItems);

  if (tempItems.Size())
  {
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + g_localizeStrings.Get(20338) + "] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetEpisodesByName(strSearch, tempItems);

  if (tempItems.Size())
  {
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + g_localizeStrings.Get(20359) + "] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetMusicVideosByName(strSearch, tempItems);

  if (tempItems.Size())
  {
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + g_localizeStrings.Get(20391) + "] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetMusicVideosByAlbum(strSearch, tempItems);

  if (tempItems.Size())
  {
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + g_localizeStrings.Get(558) + "] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }

  tempItems.Clear();
  m_database.GetEpisodesByPlot(strSearch, tempItems);

  if (tempItems.Size())
  {
    for (int i = 0; i < (int)tempItems.Size(); i++)
    {
      tempItems[i]->SetLabel("[" + g_localizeStrings.Get(20365) + "] " + tempItems[i]->GetLabel());
    }
    items.Append(tempItems);
  }
}

void CGUIWindowVideoNav::PlayItem(int iItem)
{
  // unlike additemtoplaylist, we need to check the items here
  // before calling it since the current playlist will be stopped
  // and cleared!

  // root is not allowed
  if (m_vecItems->IsVirtualDirectoryRoot())
    return;

  CGUIWindowVideoBase::PlayItem(iItem);
}

void CGUIWindowVideoNav::OnWindowLoaded()
{
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  const CGUIControl *pList = GetControl(CONTROL_LIST);
  if (pList && !GetControl(CONTROL_LABELEMPTY))
  {
    CLabelInfo info;
    info.align = XBFONT_CENTER_X | XBFONT_CENTER_Y;
    info.font = g_fontManager.GetFont("font13");
    info.textColor = 0xffffffff;
    CGUILabelControl *pLabel = new CGUILabelControl(GetID(),CONTROL_LABELEMPTY,pList->GetXPosition(),pList->GetYPosition(),pList->GetWidth(),pList->GetHeight(),info,false,false);
    pLabel->SetAnimations(pList->GetAnimations());
    Add(pLabel);
  }
#endif
  CGUIWindowVideoBase::OnWindowLoaded();
}

void CGUIWindowVideoNav::DisplayEmptyDatabaseMessage(bool bDisplay)
{
  m_bDisplayEmptyDatabaseMessage = bDisplay;
}

void CGUIWindowVideoNav::Render()
{
  if (m_bDisplayEmptyDatabaseMessage)
  {
    SET_CONTROL_LABEL(CONTROL_LABELEMPTY,g_localizeStrings.Get(745)+'\n'+g_localizeStrings.Get(746))
  }
  else
  {
    SET_CONTROL_LABEL(CONTROL_LABELEMPTY,"")
  }
  CGUIWindowVideoBase::Render();
}

void CGUIWindowVideoNav::OnInfo(const CFileItemPtr& pItem, const SScraperInfo& info)
{
}

void CGUIWindowVideoNav::OnDeleteItem(int iItem)
{
  if (iItem < 0 || iItem >= (int)m_vecItems->Size()) return;

  if (m_vecItems->IsPluginFolder())
    return;

  if (m_vecItems->m_strPath.Equals("special://videoplaylists/"))
  {
    CGUIWindowVideoBase::OnDeleteItem(iItem);
    return;
  }

  CFileItemPtr pItem = m_vecItems->Get(iItem);
  if (!DeleteItem(pItem.get()))
    return;

  CStdString strDeletePath;
  if (pItem->m_bIsFolder)
    strDeletePath=pItem->GetVideoInfoTag()->m_strPath;
  else
    strDeletePath=pItem->GetVideoInfoTag()->m_strFileNameAndPath;

  if (g_guiSettings.GetBool("filelists.allowfiledeletion") &&
      CUtil::SupportsFileOperations(strDeletePath))
  {
    pItem->m_strPath = strDeletePath;
    CGUIWindowVideoBase::OnDeleteItem(iItem);
  }

  CUtil::DeleteVideoDatabaseDirectoryCache();

  DisplayEmptyDatabaseMessage(!m_database.HasContent());
  Update( m_vecItems->m_strPath );
  m_viewControl.SetSelectedItem(iItem);
}

bool CGUIWindowVideoNav::DeleteItem(CFileItem* pItem)
{
  VIDEODB_CONTENT_TYPE iType=VIDEODB_CONTENT_MOVIES;
  if (pItem->HasVideoInfoTag() && !pItem->GetVideoInfoTag()->m_strShowTitle.IsEmpty())
    iType = VIDEODB_CONTENT_TVSHOWS;
  if (pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_iSeason > -1 && !pItem->m_bIsFolder)
    iType = VIDEODB_CONTENT_EPISODES;
  if (pItem->HasVideoInfoTag() && !pItem->GetVideoInfoTag()->m_strArtist.IsEmpty())
    iType = VIDEODB_CONTENT_MUSICVIDEOS;

  CGUIDialogYesNo* pDialog = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (!pDialog)
    return false;
  if (iType == VIDEODB_CONTENT_MOVIES)
    pDialog->SetHeading(432);
  if (iType == VIDEODB_CONTENT_EPISODES)
    pDialog->SetHeading(20362);
  if (iType == VIDEODB_CONTENT_TVSHOWS)
    pDialog->SetHeading(20363);
  if (iType == VIDEODB_CONTENT_MUSICVIDEOS)
    pDialog->SetHeading(20392);
  CStdString strLine;
  strLine.Format(g_localizeStrings.Get(433),pItem->GetLabel());
  pDialog->SetLine(0, strLine);
  pDialog->SetLine(1, "");
  pDialog->SetLine(2, "");;
  pDialog->DoModal();
  if (!pDialog->IsConfirmed())
    return false;

  CStdString path;
  CVideoDatabase database;
  database.Open();

  database.GetFilePathById(pItem->GetVideoInfoTag()->m_iDbId, path, iType);
  if (path.IsEmpty()) 
    return false;
  if (iType == VIDEODB_CONTENT_MOVIES)
    database.DeleteMovie(path);
  if (iType == VIDEODB_CONTENT_EPISODES)
    database.DeleteEpisode(path);
  if (iType == VIDEODB_CONTENT_TVSHOWS)
    database.DeleteTvShow(path);
  if (iType == VIDEODB_CONTENT_MUSICVIDEOS)
    database.DeleteMusicVideo(path);

  if (iType == VIDEODB_CONTENT_TVSHOWS)
    database.SetPathHash(path,"");
  else
  {
    CStdString strDirectory;
    CUtil::GetDirectory(path,strDirectory);
    database.SetPathHash(strDirectory,"");
  }

  // delete the cached thumb for this item (it will regenerate if it is a user thumb)
  CStdString thumb(pItem->GetCachedVideoThumb());
  CFile::Delete(thumb);

  return true;
}

void CGUIWindowVideoNav::OnFinalizeFileItems(CFileItemList& items)
{
}

void CGUIWindowVideoNav::ClearFileItems()
{
  m_viewControl.Clear();
  m_vecItems->Clear();
  m_unfilteredItems->Clear();
}

void CGUIWindowVideoNav::OnFilterItems()
{
  CStdString currentItem;
  int item = m_viewControl.GetSelectedItem();
  if (item >= 0)
    currentItem = m_vecItems->Get(item)->m_strPath;

  m_viewControl.Clear();

  FilterItems(*m_vecItems);

  // and update our view control + buttons
  m_viewControl.SetItems(*m_vecItems);
  m_viewControl.SetSelectedItem(currentItem);
  UpdateButtons();
}

void CGUIWindowVideoNav::FilterItems(CFileItemList &items)
{
}

void CGUIWindowVideoNav::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
}

bool CGUIWindowVideoNav::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  switch (button)
  {
  case CONTEXT_BUTTON_SET_DEFAULT:
    g_settings.m_defaultVideoLibSource = GetQuickpathName(item->m_strPath);
    g_settings.Save();
    return true;

  case CONTEXT_BUTTON_CLEAR_DEFAULT:
    g_settings.m_defaultVideoLibSource.Empty();
    g_settings.Save();
    return true;

  case CONTEXT_BUTTON_MARK_WATCHED:
    // If we're about to hide this item, select the next one
    if (g_stSettings.m_iMyVideoWatchMode == VIDEO_SHOW_UNWATCHED)
      m_viewControl.SetSelectedItem((itemNumber+1) % m_vecItems->Size());
    MarkWatched(item);
    CUtil::DeleteVideoDatabaseDirectoryCache();
    Update(m_vecItems->m_strPath);
    return true;

  case CONTEXT_BUTTON_MARK_UNWATCHED:
    {
      MarkUnWatched(item);
      CUtil::DeleteVideoDatabaseDirectoryCache();
      Update(m_vecItems->m_strPath);
    }
    return true;

  case CONTEXT_BUTTON_EDIT:
    UpdateVideoTitle(item.get());
    CUtil::DeleteVideoDatabaseDirectoryCache();
    Update(m_vecItems->m_strPath);
    return true;

  case CONTEXT_BUTTON_SET_SEASON_THUMB:
  case CONTEXT_BUTTON_SET_ACTOR_THUMB:
  case CONTEXT_BUTTON_SET_ARTIST_THUMB:
  case CONTEXT_BUTTON_SET_PLUGIN_THUMB:
    {
      // Grab the thumbnails from the web
      CStdString strPath;
      CFileItemList items;
      CUtil::AddFileToFolder(g_advancedSettings.m_cachePath,"imdbthumbs",strPath);
      CUtil::WipeDir(strPath);
      DIRECTORY::CDirectory::Create(strPath);
      CFileItemPtr noneitem(new CFileItem("thumb://None", false));
      int i=1;
      CVideoInfoTag tag;
      if (button != CONTEXT_BUTTON_SET_ARTIST_THUMB &&
          button != CONTEXT_BUTTON_SET_PLUGIN_THUMB)
      {
        if (button == CONTEXT_BUTTON_SET_SEASON_THUMB)
          m_database.GetTvShowInfo("",tag,m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_iDbId);
        else
          tag = *m_vecItems->Get(itemNumber)->GetVideoInfoTag();
        for (vector<CScraperUrl::SUrlEntry>::iterator iter=tag.m_strPictureURL.m_url.begin();iter != tag.m_strPictureURL.m_url.end();++iter)
        {
          if ((iter->m_type != CScraperUrl::URL_TYPE_SEASON ||
               iter->m_season != m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_iSeason) &&
               button == CONTEXT_BUTTON_SET_SEASON_THUMB)
          {
            continue;
          }
          CStdString thumbFromWeb;
          CStdString strLabel;
          strLabel.Format("imdbthumb%i.jpg",i);
          CUtil::AddFileToFolder(strPath, strLabel, thumbFromWeb);
          if (CScraperUrl::DownloadThumbnail(thumbFromWeb,*iter))
          {
            CStdString strItemPath;
            strItemPath.Format("thumb://IMDb%i",i++);
            CFileItemPtr item(new CFileItem(strItemPath, false));
            item->SetThumbnailImage(thumbFromWeb);
            CStdString strLabel;
            item->SetLabel(g_localizeStrings.Get(20015));
            items.Add(item);
          }
        }
      }
      CStdString cachedThumb = m_vecItems->Get(itemNumber)->GetCachedSeasonThumb();
      if (button == CONTEXT_BUTTON_SET_ACTOR_THUMB)
        cachedThumb = m_vecItems->Get(itemNumber)->GetCachedActorThumb();
      if (button == CONTEXT_BUTTON_SET_ARTIST_THUMB)
        cachedThumb = m_vecItems->Get(itemNumber)->GetCachedArtistThumb();
      if (button == CONTEXT_BUTTON_SET_SEASON_THUMB)
      {
        CFileItemList tbnItems;
        CDirectory::GetDirectory(tag.m_strPath,tbnItems,".tbn");
        CStdString strExpression;
        strExpression.Format("season[ ._-](0?%i)\\.tbn",m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_iSeason);
        CRegExp reg;
        if (reg.RegComp(strExpression.c_str()))
        {
          for (int j=0;j<tbnItems.Size();++j)
          {
            CStdString strCheck = CUtil::GetFileName(tbnItems[j]->m_strPath);
            strCheck.ToLower();
            if (reg.RegFind(strCheck.c_str()) > -1)
            {
              CFileItemPtr item(new CFileItem("thumb://Local", false));
              item->SetThumbnailImage(cachedThumb); // this doesn't look right to me - perhaps tbnItems[j]->m_strPath?
              item->SetLabel(g_localizeStrings.Get(20017));
              items.Add(item);
              break;
            }
          }
        }
        noneitem->SetThumbnailImage("DefaultFolderBig.png");
        noneitem->SetLabel(g_localizeStrings.Get(20018));
      }
      if (button == CONTEXT_BUTTON_SET_PLUGIN_THUMB)
      {
        strPath = m_vecItems->Get(itemNumber)->m_strPath;
        strPath.Replace("plugin://video/","U:\\plugins\\video\\");
        strPath.Replace("/","\\");
        CFileItem item(strPath,true);
        cachedThumb = item.GetCachedProgramThumb();
      }
      if (CFile::Exists(cachedThumb))
      {
        CFileItemPtr item(new CFileItem("thumb://Current", false));
        item->SetThumbnailImage(cachedThumb);
        item->SetLabel(g_localizeStrings.Get(20016));
        items.Add(item);
      }
      else
      {
        noneitem->SetThumbnailImage("DefaultFolderBig.png");
        noneitem->SetLabel(g_localizeStrings.Get(20018));
      }

      if (button == CONTEXT_BUTTON_SET_PLUGIN_THUMB)
      {
        if (items.Size() == 0)
        {
          CFileItem item2(strPath,false);
          CUtil::AddFileToFolder(strPath,"default.py",item2.m_strPath);
          if (CFile::Exists(item2.GetCachedProgramThumb()))
          {
            CFileItemPtr item(new CFileItem("thumb://Current", false));
            item->SetThumbnailImage(item2.GetCachedProgramThumb());
            item->SetLabel(g_localizeStrings.Get(20016));
            items.Add(item);
          }
          else
          {
            noneitem->SetThumbnailImage("DefaultFolderBig.png");
            noneitem->SetLabel(g_localizeStrings.Get(20018));
          }
        }
        CStdString strThumb;
        CUtil::AddFileToFolder(strPath,"folder.jpg",strThumb);
        if (CFile::Exists(strThumb))
        {
          CFileItemPtr item(new CFileItem(strThumb,false));
          item->SetThumbnailImage(strThumb);
          item->SetLabel(g_localizeStrings.Get(20017));
          items.Add(item);
        }
        else
        {
          noneitem->SetThumbnailImage("DefaultFolderBig.png");
          noneitem->SetLabel(g_localizeStrings.Get(20018));
        }
        CUtil::AddFileToFolder(strPath,"default.tbn",strThumb);
        if (CFile::Exists(strThumb))
        {
          CFileItemPtr item(new CFileItem(strThumb,false));
          item->SetThumbnailImage(strThumb);
          item->SetLabel(g_localizeStrings.Get(20017));
          items.Add(item);
        }
        else
        {
          noneitem->SetThumbnailImage("DefaultFolderBig.png");
          noneitem->SetLabel(g_localizeStrings.Get(20018));
        }
      }
      if (button == CONTEXT_BUTTON_SET_ARTIST_THUMB)
      {
        CStdString picturePath;

        CStdString strPath = m_vecItems->Get(itemNumber)->m_strPath;
        CUtil::RemoveSlashAtEnd(strPath);

        int nPos=strPath.ReverseFind("/");
        if (nPos>-1)
        {
          //  try to guess where the user should start
          //  browsing for the artist thumb
          CMusicDatabase database;
          database.Open();
          CFileItemList albums;
          long idArtist=database.GetArtistByName(m_vecItems->Get(itemNumber)->GetLabel());
          CStdString path;
          database.GetArtistPath(idArtist, picturePath);
        }

        CStdString strThumb;
        CUtil::AddFileToFolder(picturePath,"folder.jpg",strThumb);
        if (XFILE::CFile::Exists(strThumb))
        {
          CFileItemPtr pItem(new CFileItem(strThumb,false));
          pItem->SetLabel(g_localizeStrings.Get(20017));
          pItem->SetThumbnailImage(strThumb);
          items.Add(pItem);
        }
        else
        {
          noneitem->SetThumbnailImage("DefaultArtistBig.png");
          noneitem->SetLabel(g_localizeStrings.Get(20018));
        }
      }

      if (button == CONTEXT_BUTTON_SET_ACTOR_THUMB)
      {
        CStdString picturePath;
        CStdString strThumb;
        CUtil::AddFileToFolder(picturePath,"folder.jpg",strThumb);
        if (XFILE::CFile::Exists(strThumb))
        {
          CFileItemPtr pItem(new CFileItem(strThumb,false));
          pItem->SetLabel(g_localizeStrings.Get(20017));
          pItem->SetThumbnailImage(strThumb);
          items.Add(pItem);
        }
        else
        {
          noneitem->SetThumbnailImage("DefaultActorBig.png");
          noneitem->SetLabel(g_localizeStrings.Get(20018));
        }
      }

      items.Add(noneitem);

      CStdString result;
      if (!CGUIDialogFileBrowser::ShowAndGetImage(items, g_settings.m_videoSources,
                                                  g_localizeStrings.Get(20019), result))
      {
        return false;   // user cancelled
      }

      if (result == "thumb://Current")
        return false;   // user chose the one they have

      // delete the thumbnail if that's what the user wants, else overwrite with the
      // new thumbnail
      if (result.Mid(0,12) == "thumb://IMDb")
      {
        CStdString strFile;
        CUtil::AddFileToFolder(strPath,"imdbthumb"+result.Mid(12)+".jpg",strFile);
        if (CFile::Exists(strFile))
          CFile::Cache(strFile, cachedThumb);
        else
          result = "thumb://None";
      }
      if (result == "thumb://None")
      {
        CFile::Delete(cachedThumb);
        if (button == CONTEXT_BUTTON_SET_PLUGIN_THUMB)
        {
          CPicture picture;
          picture.CacheSkinImage("DefaultFolderBig.png",cachedThumb);
          CFileItem item2(strPath,false);
          CUtil::AddFileToFolder(strPath,"default.py",item2.m_strPath);
          CFile::Delete(item2.GetCachedProgramThumb());
        }
      }
      else
        CFile::Cache(result,cachedThumb);

      CUtil::DeleteVideoDatabaseDirectoryCache();
      CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
      g_graphicsContext.SendMessage(msg);
      Update(m_vecItems->m_strPath);

      return true;
    }
  case CONTEXT_BUTTON_UPDATE_LIBRARY:
    {
      return true;
    }
  case CONTEXT_BUTTON_UNLINK_MOVIE:
    {
      OnLinkMovieToTvShow(itemNumber, true);
      Update(m_vecItems->m_strPath);
      return true;
    }
  case CONTEXT_BUTTON_LINK_MOVIE:
    {
      OnLinkMovieToTvShow(itemNumber, false);
      return true;
    }
  case CONTEXT_BUTTON_GO_TO_ARTIST:
    {
      CStdString strPath;
      CMusicDatabase database;
      database.Open();
      strPath.Format("musicdb://2/%ld/",database.GetArtistByName(m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_strArtist));
      m_gWindowManager.ActivateWindow(WINDOW_MUSIC_NAV,strPath);
      return true;
    }
  case CONTEXT_BUTTON_GO_TO_ALBUM:
    {
      CStdString strPath;
      CMusicDatabase database;
      database.Open();
      strPath.Format("musicdb://3/%ld/",database.GetAlbumByName(m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_strAlbum));
      m_gWindowManager.ActivateWindow(WINDOW_MUSIC_NAV,strPath);
      return true;
    }
  case CONTEXT_BUTTON_PLAY_OTHER:
    {
      CMusicDatabase database;
      database.Open();
      CSong song;
      if (database.GetSongById(database.GetSongByArtistAndAlbumAndTitle(m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_strArtist,m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_strAlbum,
                                                                        m_vecItems->Get(itemNumber)->GetVideoInfoTag()->m_strTitle),
                                                                        song))
      {
        g_application.getApplicationMessenger().PlayFile(song);
      }
      return true;
    }

  case CONTEXT_BUTTON_UNLINK_BOOKMARK:
    {
      m_database.Open();
      m_database.DeleteBookMarkForEpisode(*m_vecItems->Get(itemNumber)->GetVideoInfoTag());
      m_database.Close();
      CUtil::DeleteVideoDatabaseDirectoryCache();
      Update(m_vecItems->m_strPath);
      return true;
    }

  default:
    break;

  }
  return CGUIWindowVideoBase::OnContextButton(itemNumber, button);
}

void CGUIWindowVideoNav::OnLinkMovieToTvShow(int itemnumber, bool bRemove)
{
}
