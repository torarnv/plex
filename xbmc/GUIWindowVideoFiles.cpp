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
#include "GUIWindowVideoFiles.h"
#include "Util.h"
#include "Picture.h"
#include "utils/IMDB.h"
#include "utils/GUIInfoManager.h"
#include "GUIWindowVideoInfo.h"
#include "PlayListFactory.h"
#include "Application.h"
#include "NfoFile.h"
#include "PlayListPlayer.h"
#include "GUIPassword.h"
#include "GUIDialogMediaSource.h"
#include "GUIDialogContentSettings.h"
#include "GUIDialogVideoScan.h"
#include "FileSystem/MultiPathDirectory.h"
#include "utils/RegExp.h"
#include "GUIWindowManager.h"
#include "GUIDialogOK.h"
#include "GUIDialogYesNo.h"
#include "FileSystem/File.h"
#include "PlayList.h"
#include "CocoaUtils.h"

using namespace std;
using namespace MEDIA_DETECT;
using namespace XFILE;
using namespace PLAYLIST;
using namespace VIDEO;

#define CONTROL_LIST              50
#define CONTROL_THUMBS            51

#define CONTROL_PLAY_DVD           6
#define CONTROL_STACK              7
#define CONTROL_BTNSCAN            8
#define CONTROL_IMDB               9
#define CONTROL_BTNPLAYLISTS      13

CGUIWindowVideoFiles::CGUIWindowVideoFiles()
: CGUIWindowVideoBase(WINDOW_VIDEO_FILES, "MyVideo.xml")
{
  m_stackingAvailable = true;
}

CGUIWindowVideoFiles::~CGUIWindowVideoFiles()
{
}

bool CGUIWindowVideoFiles::OnAction(const CAction &action)
{
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
  return CGUIWindowVideoBase::OnAction(action);
}

bool CGUIWindowVideoFiles::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
    {
      // check for valid quickpath parameter
      CStdStringArray params;
      StringUtils::SplitString(message.GetStringParam(), ",", params);
      bool returning = params.size() > 1 && params[1] == "return";
      
      // check for a passed destination path
      CStdString strDestination = params.size() ? params[0] : "";
      if (!strDestination.IsEmpty())
      {
        message.SetStringParam("");
        g_stSettings.m_iVideoStartWindow = GetID();
        CLog::Log(LOGINFO, "Attempting to %s to: %s", returning ? "return" : "quickpath", strDestination.c_str());
        // reset directory path, as we have effectively cleared it here
        m_history.ClearPathHistory();
      }

      // is this the first time accessing this window?
      // a quickpath overrides the a default parameter
      if ((m_vecItems->m_strPath == "?" || m_vecItems->m_strPath == "") && strDestination.IsEmpty())
      {
        m_vecItems->m_strPath = strDestination = g_settings.m_defaultVideoSource;
        CLog::Log(LOGINFO, "Attempting to default to: %s", strDestination.c_str());
      }

      // try to open the destination path
      if (!strDestination.IsEmpty())
      {
        // open root
        if (strDestination.Equals("$ROOT"))
        {
          m_vecItems->m_strPath = "";
          CLog::Log(LOGINFO, "  Success! Opening root listing.");
        }
        // open playlists location
        else if (strDestination.Equals("$PLAYLISTS"))
        {
          m_vecItems->m_strPath = "special://videoplaylists/";
          CLog::Log(LOGINFO, "  Success! Opening destination path: %s", m_vecItems->m_strPath.c_str());
        }
        else
        {
          // default parameters if the jump fails
          m_vecItems->m_strPath = "";

          bool bIsSourceName = false;

          SetupShares();
          VECSOURCES shares;
          m_rootDir.GetSources(shares);
          int iIndex = CUtil::GetMatchingSource(strDestination, shares, bIsSourceName);
          if (iIndex > -1)
          {
            bool bDoStuff = true;
            if (iIndex < shares.size() && shares[iIndex].m_iHasLock == 2)
            {
              CFileItem item(shares[iIndex]);
              if (!g_passwordManager.IsItemUnlocked(&item,"video"))
              {
                m_vecItems->m_strPath = ""; // no u don't
                bDoStuff = false;
                CLog::Log(LOGINFO, "  Failure! Failed to unlock destination path: %s", strDestination.c_str());
              }
            }
            // set current directory to matching share
            if (bDoStuff)
            {
              if (bIsSourceName)
                m_vecItems->m_strPath=shares[iIndex].strPath;
              else
                m_vecItems->m_strPath=strDestination;
              CLog::Log(LOGINFO, "  Success! Opened destination path: %s", strDestination.c_str());
            }
          }
          else
          {
            CLog::Log(LOGWARNING, "  Failed! Destination parameter (%s) does not match a valid source!", strDestination.c_str());
            m_vecItems->m_strPath = strDestination;
            CUtil::RemoveSlashAtEnd(m_vecItems->m_strPath);
          }
        }

        // check for network up
        if (CUtil::IsRemote(m_vecItems->m_strPath) && !WaitForNetwork())
          m_vecItems->m_strPath.Empty();

        SetHistoryForPath(m_vecItems->m_strPath);
      }

      if (!CGUIWindowVideoBase::OnMessage(message))
        return false;
      
      if (message.GetParam1() != WINDOW_INVALID)
      { // first time to this window - make sure we set the root path
        m_startDirectory = returning ? m_vecItems->m_strPath : "";
      }
      
      return true;
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      //if (iControl == CONTROL_BTNSCAN)
      //{
      //  OnScan();
     // }
      /*else*/ if (iControl == CONTROL_STACK)
      {
        // toggle between the following states:
        //   0 : no stacking
        //   1 : stacking
        g_stSettings.m_iMyVideoStack++;

        if (g_stSettings.m_iMyVideoStack > STACK_SIMPLE)
          g_stSettings.m_iMyVideoStack = STACK_NONE;

        g_settings.Save();
        UpdateButtons();
        Update( m_vecItems->m_strPath );
      }
      else if (iControl == CONTROL_BTNPLAYLISTS)
      {
        if (!m_vecItems->m_strPath.Equals("special://videoplaylists/"))
        {
          CStdString strParent = m_vecItems->m_strPath;
          UpdateButtons();
          Update("special://videoplaylists/");
        }
      }
      // list/thumb panel
      else if (m_viewControl.HasControl(iControl))
      {
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        const CFileItemPtr pItem = m_vecItems->Get(iItem);

        // use play button to add folders of items to temp playlist
        if (iAction == ACTION_PLAYER_PLAY && pItem->m_bIsFolder && !pItem->IsParentFolder())
        {
          if (pItem->IsDVD())
            return CAutorun::PlayDisc();

          if (pItem->m_bIsShareOrDrive)
            return false;
          // if playback is paused or playback speed != 1, return
          if (g_application.IsPlayingVideo())
          {
            if (g_application.m_pPlayer->IsPaused()) return false;
            if (g_application.GetPlaySpeed() != 1) return false;
          }
          // not playing, or playback speed == 1
          // queue folder or playlist into temp playlist and play
          if ((pItem->m_bIsFolder && !pItem->IsParentFolder()) || pItem->IsPlayList())
            PlayItem(iItem);
          return true;
        }
      }
    }
  }
  return CGUIWindowVideoBase::OnMessage(message);
}


void CGUIWindowVideoFiles::UpdateButtons()
{
  CGUIWindowVideoBase::UpdateButtons();
  const CGUIControl *stack = GetControl(CONTROL_STACK);
  if (stack)
  {
    if (m_stackingAvailable)
    {
      CONTROL_ENABLE(CONTROL_STACK)
      if (stack->GetControlType() == CGUIControl::GUICONTROL_RADIO)
      {
        SET_CONTROL_SELECTED(GetID(), CONTROL_STACK, g_stSettings.m_iMyVideoStack == STACK_SIMPLE);
        SET_CONTROL_LABEL(CONTROL_STACK, 14000);  // Stack
      }
      else
      {
        SET_CONTROL_LABEL(CONTROL_STACK, g_stSettings.m_iMyVideoStack + 14000);
      }
    }
    else
    {
      if (stack->GetControlType() == CGUIControl::GUICONTROL_RADIO)
      {
        SET_CONTROL_LABEL(CONTROL_STACK, 14000);  // Stack
      }

      CONTROL_DISABLE(CONTROL_STACK)
    }
  }
}

bool CGUIWindowVideoFiles::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  if (!CGUIWindowVideoBase::GetDirectory(strDirectory, items))
    return false;

  SScraperInfo info2;

  m_stackingAvailable = true;

  if (items.GetContent().IsEmpty())
    items.SetContent("files");

  if (m_database.GetScraperForPath(strDirectory,info2) && info2.strContent.Equals("tvshows"))
  { 
    // Don't stack in TV directories.
    m_stackingAvailable = false;
  }
  else if (strDirectory.Left(7) == "plex://")
  {
    // Don't stack PMS directories.
    m_stackingAvailable = false;
  }
  else if (!items.IsStack() && g_stSettings.m_iMyVideoStack != STACK_NONE)
  {
    items.Stack();
  }

  items.SetThumbnailImage("");
  items.SetVideoThumb();
  
  // See if we need to turn on/off theme music.
  string themeURL = items.GetProperty("theme");
  if (themeURL.size() > 0)
    Cocoa_SetBackgroundMusicThemeId(themeURL.c_str());
  else
    Cocoa_SetBackgroundMusicThemeId(NULL);

  return true;
}

void CGUIWindowVideoFiles::OnPrepareFileItems(CFileItemList &items)
{
  CGUIWindowVideoBase::OnPrepareFileItems(items);
  if (g_guiSettings.GetBool("filelists.cleanfilenames"))
    items.CleanFileNames();
}

bool CGUIWindowVideoFiles::OnClick(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems->Size() ) return true;
  CFileItemPtr pItem = m_vecItems->Get(iItem);
  CStdString strExtension;
  CUtil::GetExtension(pItem->m_strPath, strExtension);

  if (strcmpi(strExtension.c_str(), ".nfo") == 0) // WTF??
  {
    SScraperInfo info;
    info.strPath = "imdb.xml";
    info.strContent = "movies";
    info.strTitle = "IMDb";
    OnInfo(pItem,info);
    return true;
  }

  return CGUIWindowVideoBase::OnClick(iItem);
}

bool CGUIWindowVideoFiles::OnPlayMedia(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems->Size() ) return false;
  CFileItemPtr pItem = m_vecItems->Get(iItem);

  if (pItem->IsDVD())
    return CAutorun::PlayDisc();

  if (pItem->m_bIsShareOrDrive)
    return false;

  if (pItem->m_strPath == "add" && pItem->GetLabel() == g_localizeStrings.Get(1026)) // 'add source button' in empty root
  {
    if (CGUIDialogMediaSource::ShowAndAddMediaSource("video"))
    {
      Update("");
      return true;
    }
    return false;
  }
  else
  {
    AddFileToDatabase(pItem.get());

    return CGUIWindowVideoBase::OnPlayMedia(iItem);
  }
}

void CGUIWindowVideoFiles::AddFileToDatabase(const CFileItem* pItem)
{
  if (!pItem->IsVideo()) return ;
  if ( pItem->IsNFO()) return ;
  if ( pItem->IsPlayList()) return ;

  /* subtitles are assumed not to exist here, if we need it  */
  /* player should update this when it figures out if it has */
  m_database.AddFile(pItem->m_strPath);

  if (pItem->IsStack())
  { // get stacked items
    // TODO: This should be removed as soon as we no longer need the individual
    // files for saving settings etc.
    vector<CStdString> movies;
    GetStackedFiles(pItem->m_strPath, movies);
    for (unsigned int i = 0; i < movies.size(); i++)
    {
      CFileItem item(movies[i], false);
      m_database.AddFile(item.m_strPath);
    }
  }
}

void CGUIWindowVideoFiles::OnUnAssignContent(int iItem)
{
  bool bCanceled;
  if (CGUIDialogYesNo::ShowAndGetInput(20375,20340,20341,20022,bCanceled))
  {
    m_database.RemoveContentForPath(m_vecItems->Get(iItem)->m_strPath,m_dlgProgress);
    CUtil::DeleteVideoDatabaseDirectoryCache();
  }
  else
  {
    if (!bCanceled)
    {
      SScraperInfo info;
      SScanSettings settings;
      m_database.SetScraperForPath(m_vecItems->Get(iItem)->m_strPath,info,settings);
    }
  }
}

void CGUIWindowVideoFiles::OnAssignContent(int iItem, int iFound, SScraperInfo& info, SScanSettings& settings)
{
  if (!g_guiSettings.GetBool("videolibrary.enabled"))
    return;

  CFileItemPtr item = m_vecItems->Get(iItem);
  bool bScan=true;
  if (iFound == 0)
  {
    m_database.GetScraperForPath(item->m_strPath,info, settings,iFound);
  }
  SScraperInfo info2 = info;
  SScanSettings settings2 = settings;
  
  if (CGUIDialogContentSettings::Show(info2, settings2, bScan))
  {
    if((info2.strContent.IsEmpty() || info2.strContent.Equals("None")) && 
      (!info.strContent.IsEmpty() && !info.strContent.Equals("None")))
    {
      OnUnAssignContent(iItem);
    }

    m_database.Open();
    m_database.SetScraperForPath(item->m_strPath,info2,settings2);
    m_database.Close();

    if (bScan)
    {
      GetScraperForItem(item.get(),info2,settings2);
      OnScan(item->m_strPath,info2,settings2);
    }
  }
}

void CGUIWindowVideoFiles::GetStackedDirectory(const CStdString &strPath, CFileItemList &items)
{
  items.Clear();
  m_rootDir.GetDirectory(strPath, items);
  items.Stack();
}

void CGUIWindowVideoFiles::LoadPlayList(const CStdString& strPlayList)
{
  // load a playlist like .m3u, .pls
  // first get correct factory to load playlist
  auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(strPlayList));
  if ( NULL != pPlayList.get())
  {
    // load it
    if (!pPlayList->Load(strPlayList))
    {
      CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
      return ; //hmmm unable to load playlist?
    }
  }

  int iSize = pPlayList->size();
  if (g_application.ProcessAndStartPlaylist(strPlayList, *pPlayList, PLAYLIST_VIDEO))
  {
    if (m_guiState.get())
      m_guiState->SetPlaylistDirectory("playlistvideo://");
    // activate the playlist window if its not activated yet
    if (GetID() == (DWORD) m_gWindowManager.GetActiveWindow() && iSize > 1)
    {
      m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    }
  }
}

void CGUIWindowVideoFiles::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  
  bool includeStandardContextButtons = true;
  if (item)
  {
    includeStandardContextButtons = item->m_includeStandardContextItems;
    
    // The info button.
    if (m_vecItems->GetContent() == "movies")
      buttons.Add(CONTEXT_BUTTON_INFO, 13346);
    else if (m_vecItems->GetContent() == "tvshows")
      buttons.Add(CONTEXT_BUTTON_INFO, 20351);
    else if (m_vecItems->GetContent() == "episodes")
      buttons.Add(CONTEXT_BUTTON_INFO, 20352);
    
    if (m_vecItems->IsVirtualDirectoryRoot() == false)
      CGUIWindowVideoBase::GetContextButtons(itemNumber, buttons);
  }

  if (includeStandardContextButtons)
    CGUIWindowVideoBase::GetNonContextButtons(itemNumber, buttons);
}

bool CGUIWindowVideoFiles::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  if ( m_vecItems->IsVirtualDirectoryRoot() && item)
  {
    CMediaSource *share = CGUIDialogContextMenu::GetShare("video", item.get());
    if (CGUIDialogContextMenu::OnContextButton("video", share, button))
    {
      if (button == CONTEXT_BUTTON_REMOVE_SOURCE)
        OnUnAssignContent(itemNumber);
      Update("");
      return true;
    }
  }

  switch (button)
  {
  case CONTEXT_BUTTON_SWITCH_MEDIA:
    CGUIDialogContextMenu::SwitchMedia("video", m_vecItems->m_strPath);
    return true;

  case CONTEXT_BUTTON_SET_CONTENT:
    {
      SScraperInfo info;
      SScanSettings settings;
      if (item->HasVideoInfoTag())  // files view shouldn't need this check I think?
        m_database.GetScraperForPath(item->GetVideoInfoTag()->m_strPath, info, settings);
      else
        m_database.GetScraperForPath(item->m_strPath, info, settings);
      CScraperParser parser;
      if (parser.Load(_P("q:\\system\\scrapers\\video\\"+info.strPath)))
        info.strTitle = parser.GetName();
      OnAssignContent(itemNumber,0, info, settings);
      return true;
    }

  case CONTEXT_BUTTON_ADD_TO_LIBRARY:
    AddToDatabase(itemNumber);
    return true;

  default:
    break;
  }
  return CGUIWindowVideoBase::OnContextButton(itemNumber, button);
}

void CGUIWindowVideoFiles::OnQueueItem(int iItem)
{
  CGUIWindowVideoBase::OnQueueItem(iItem);
}
