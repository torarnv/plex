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
#include "GUIWindowVideoBase.h"
#include "Util.h"
#include "utils/IMDB.h"
#include "utils/RegExp.h"
#include "utils/GUIInfoManager.h"
#include "GUIWindowVideoInfo.h"
#include "GUIDialogFileBrowser.h"
#include "GUIDialogSmartPlaylistEditor.h"
#include "GUIDialogProgress.h"
#include "PlayListFactory.h"
#include "Application.h"
#include "NfoFile.h"
#include "Picture.h"
#include "utils/fstrcmp.h"
#include "PlayListPlayer.h"
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
#include "GUIFontManager.h"
#endif
#include "GUIPassword.h"
#include "FileSystem/ZipManager.h"
#include "FileSystem/StackDirectory.h"
#include "GUIDialogFileStacking.h"
#include "GUIDialogMediaSource.h"
#include "GUIWindowFileManager.h"
#include "PartyModeManager.h"
#include "GUIWindowManager.h"
#include "GUIDialogOK.h"
#include "GUIDialogSelect.h"
#include "GUIDialogKeyboard.h"
#include "FileSystem/Directory.h"
#include "PlayList.h"
#include "PlexMediaServerQueue.h"
#include "SkinInfo.h"

using namespace std;
using namespace XFILE;
using namespace DIRECTORY;
using namespace MEDIA_DETECT;
using namespace PLAYLIST;
using namespace VIDEO;

#define CONTROL_BTNVIEWASICONS     2
#define CONTROL_BTNSORTBY          3
#define CONTROL_BTNSORTASC         4
#define CONTROL_BTNTYPE            5
#define CONTROL_LIST              50
#define CONTROL_THUMBS            51
#define CONTROL_BIGLIST           52
#define CONTROL_LABELFILES        12

#define CONTROL_PLAY_DVD           6
#define CONTROL_STACK              7
#define CONTROL_BTNSCAN            8
#define CONTROL_IMDB               9

CGUIWindowVideoBase::CGUIWindowVideoBase(DWORD dwID, const CStdString &xmlFile)
    : CGUIMediaWindow(dwID, xmlFile)
{
  m_thumbLoader.SetObserver(this);
}

CGUIWindowVideoBase::~CGUIWindowVideoBase()
{
}

bool CGUIWindowVideoBase::OnAction(const CAction &action)
{
  if (action.wID == ACTION_SHOW_PLAYLIST)
  {
    OutputDebugString("activate guiwindowvideoplaylist!\n");
    m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    return true;
  }
  if (action.wID == ACTION_SCAN_ITEM)
    return OnContextButton(m_viewControl.GetSelectedItem(),CONTEXT_BUTTON_SCAN);

  return CGUIMediaWindow::OnAction(action);
}

bool CGUIWindowVideoBase::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    if (m_thumbLoader.IsLoading())
      //m_thumbLoader.StopThread();
    m_database.Close();
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      m_database.Open();

      m_dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);

      // save current window, unless the current window is the video playlist window
      if (GetID() != WINDOW_VIDEO_PLAYLIST && (DWORD)g_stSettings.m_iVideoStartWindow != GetID())
      {
        g_stSettings.m_iVideoStartWindow = GetID();
        g_settings.Save();
      }

      return CGUIMediaWindow::OnMessage(message);
    }
    break;

  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_PLAY_DVD)
      {
        // play movie...
        CUtil::PlayDVD();
      }
      else if (iControl == CONTROL_BTNTYPE)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_BTNTYPE);
        m_gWindowManager.SendMessage(msg);

        int nSelected = msg.GetParam1();
        int nNewWindow = WINDOW_VIDEO_FILES;
        switch (nSelected)
        {
        case 0:  // Movies
          nNewWindow = WINDOW_VIDEO_FILES;
          break;
        case 1:  // Library
          nNewWindow = WINDOW_VIDEO_NAV;
          break;
        }

        if ((DWORD) nNewWindow != GetID())
        {
          g_stSettings.m_iVideoStartWindow = nNewWindow;
          g_settings.Save();
          m_gWindowManager.ChangeActiveWindow(nNewWindow);
          CGUIMessage msg2(GUI_MSG_SETFOCUS, nNewWindow, CONTROL_BTNTYPE);
          g_graphicsContext.SendMessage(msg2);
        }

        return true;
      }
      else if (m_viewControl.HasControl(iControl))  // list/thumb control
      {
        // get selected item
        int iItem = m_viewControl.GetSelectedItem();
        int iAction = message.GetParam1();

        // iItem is checked for validity inside these routines
        if (iAction == ACTION_QUEUE_ITEM || iAction == ACTION_MOUSE_MIDDLE_CLICK)
        {
          OnQueueItem(iItem);
          return true;
        }
        else if (iAction == ACTION_SHOW_INFO)
        {
          SScraperInfo info;
          CStdString strDir;
          if (iItem < 0 || iItem >= m_vecItems->Size())
            return false;
          
          CFileItemPtr item = m_vecItems->Get(iItem);

          // Don't show the Info dialog for Plex paths
          if (CUtil::IsPlexMediaServer(item->m_strPath) && item->HasProperty("type") == false)
            return false;
          
          OnInfo(item,info);

          return true;
        }
        else if (iAction == ACTION_PLAYER_PLAY && !g_application.IsPlayingVideo())
        {
          OnResumeItem(iItem);
          return true;
        }
        else if (iAction == ACTION_DELETE_ITEM)
        {
          // is delete allowed?
          // must be at the title window
          if (GetID() == WINDOW_VIDEO_NAV)
            OnDeleteItem(iItem);

          // or be at the files window and have file deletion enabled
          else if (GetID() == WINDOW_VIDEO_FILES && g_guiSettings.GetBool("filelists.allowfiledeletion"))
            OnDeleteItem(iItem);

          // or be at the video playlists location
          else if (m_vecItems->m_strPath.Equals("special://videoplaylists/"))
            OnDeleteItem(iItem);
          else
            return false;
          
          return true;
        }
      }
    }
    break;
  }
  return CGUIMediaWindow::OnMessage(message);
}

void CGUIWindowVideoBase::UpdateButtons()
{
  // Remove labels from the window selection
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), CONTROL_BTNTYPE);
  g_graphicsContext.SendMessage(msg);

  // Add labels to the window selection
  CStdString strItem = g_localizeStrings.Get(744); // Files
  CGUIMessage msg2(GUI_MSG_LABEL_ADD, GetID(), CONTROL_BTNTYPE);
  msg2.SetLabel(strItem);
  g_graphicsContext.SendMessage(msg2);

  strItem = g_localizeStrings.Get(14022); // Library
  msg2.SetLabel(strItem);
  g_graphicsContext.SendMessage(msg2);

  // Select the current window as default item
  int nWindow = g_stSettings.m_iVideoStartWindow-WINDOW_VIDEO_FILES;
  CONTROL_SELECT_ITEM(CONTROL_BTNTYPE, nWindow);

  // disable scan and manual imdb controls if internet lookups are disabled
  if (g_guiSettings.GetBool("network.enableinternet"))
  {
    CONTROL_ENABLE(CONTROL_BTNSCAN);
    CONTROL_ENABLE(CONTROL_IMDB);
  }
  else
  {
    CONTROL_DISABLE(CONTROL_BTNSCAN);
    CONTROL_DISABLE(CONTROL_IMDB);
  }

  CGUIMediaWindow::UpdateButtons();
}

void CGUIWindowVideoBase::OnInfo(const CFileItemPtr& item, const SScraperInfo& info)
{
  if (!item) 
    return;
  
  bool modified = ShowIMDB(item, info);
  if (modified)
  {
    int itemNumber = m_viewControl.GetSelectedItem();
    Update(m_vecItems->m_strPath);
    m_viewControl.SetSelectedItem(itemNumber);
  }
}

bool CGUIWindowVideoBase::ShowIMDB(const CFileItemPtr& item, const SScraperInfo& info)
{
  CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
  CGUIWindowVideoInfo* pDlgInfo = (CGUIWindowVideoInfo*)m_gWindowManager.GetWindow(WINDOW_VIDEO_INFO);

  if (!pDlgProgress) return false;
  if (!pDlgSelect) return false;
  if (!pDlgInfo) return false;

  CUtil::ClearCache();

  pDlgInfo->SetMovie(item);
  pDlgInfo->DoModal();
  
  if (pDlgInfo->NeedRefresh() == false)
    return false;
  
  return true;
}

bool CGUIWindowVideoBase::IsCorrectDiskInDrive(const CStdString& strFileName, const CStdString& strDVDLabel)
{
  CDetectDVDMedia::WaitMediaReady();
  CCdInfo* pCdInfo = CDetectDVDMedia::GetCdInfo();
  if (pCdInfo == NULL) 
    return false;
  if (!CFile::Exists(strFileName)) 
    return false;
  CStdString label = pCdInfo->GetDiscLabel().TrimRight(" ");
  int iLabelCD = label.GetLength();
  int iLabelDB = strDVDLabel.GetLength();
  if (iLabelDB < iLabelCD) 
    return false;
  CStdString dbLabel = strDVDLabel.Left(iLabelCD);
  
  return (dbLabel == label);
}

bool CGUIWindowVideoBase::CheckMovie(const CStdString& strFileName)
{
  if (!m_database.HasMovieInfo(strFileName)) 
    return true;

  CVideoInfoTag movieDetails;
  m_database.GetMovieInfo(strFileName, movieDetails);
  CFileItem movieFile(movieDetails.m_strFileNameAndPath, false);
  if (!movieFile.IsOnDVD()) 
    return true;
  CGUIDialogOK *pDlgOK = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
  if (!pDlgOK) 
    return true;
  while (1)
  {
//    if (IsCorrectDiskInDrive(strFileName, movieDetails.m_strDVDLabel))
 //   {
      return true;
 //   }
    pDlgOK->SetHeading( 428);
    pDlgOK->SetLine( 0, 429 );
//    pDlgOK->SetLine( 1, movieDetails.m_strDVDLabel );
    pDlgOK->SetLine( 2, "" );
    pDlgOK->DoModal();
    if (!pDlgOK->IsConfirmed())
    {
      break;
    }
  }
  return false;
}

void CGUIWindowVideoBase::OnQueueItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems->Size() ) return ;

  // we take a copy so that we can alter the queue state
  CFileItemPtr item(new CFileItem(*m_vecItems->Get(iItem)));
  if (item->IsRAR() || item->IsZIP())
    return;

  //  Allow queuing of unqueueable items
  //  when we try to queue them directly
  if (!item->CanQueue())
    item->SetCanQueue(true);

  CFileItemList queuedItems;
  AddItemToPlayList(item, queuedItems);
  // if party mode, add items but DONT start playing
  if (g_partyModeManager.IsEnabled(PARTYMODECONTEXT_VIDEO))
  {
    g_partyModeManager.AddUserSongs(queuedItems, false);
    return;
  }

  g_playlistPlayer.Add(PLAYLIST_VIDEO, queuedItems);
  // video does not auto play on queue like music
  m_viewControl.SetSelectedItem(iItem + 1);
}

void CGUIWindowVideoBase::AddItemToPlayList(const CFileItemPtr &pItem, CFileItemList &queuedItems)
{
  if (!pItem->CanQueue() || pItem->IsRAR() || pItem->IsZIP() || pItem->IsParentFolder()) // no zip/rar enques thank you!
    return;

  if (pItem->m_bIsFolder)
  {
    if (pItem->IsParentFolder()) 
      return;

    // Check if we add a locked share
    if ( pItem->m_bIsShareOrDrive )
    {
      CFileItem item = *pItem;
      if ( !g_passwordManager.IsItemUnlocked( &item, "video" ) )
        return;
    }

    // recursive
    CFileItemList items;
    GetDirectory(pItem->m_strPath, items);
    FormatAndSort(items);

    for (int i = 0; i < items.Size(); ++i)
    {
      if (items[i]->m_bIsFolder)
      {
        CStdString strPath = items[i]->m_strPath;
        if (CUtil::HasSlashAtEnd(strPath))
          strPath.erase(strPath.size()-1);
        strPath.ToLower();
        if (strPath.size() > 6)
        {
          CStdString strSub = strPath.substr(strPath.size()-6);
          if (strPath.Mid(strPath.size()-6).Equals("sample")) // skip sample folders
            continue;
        }
      }
      AddItemToPlayList(items[i], queuedItems);
    }
  }
  else
  {
    // just an item
    if (pItem->IsPlayList())
    {
      auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(*pItem));
      if (pPlayList.get())
      {
        // load it
        if (!pPlayList->Load(pItem->m_strPath))
        {
          CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
          return; //hmmm unable to load playlist?
        }

        CPlayList playlist = *pPlayList;
        for (int i = 0; i < (int)playlist.size(); ++i)
        {
          AddItemToPlayList(playlist[i], queuedItems);
        }
        return;
      }
    }
    else if(pItem->IsInternetStream())
    { // just queue the internet stream, it will be expanded on play
      queuedItems.Add(pItem);
    }
    else if (pItem->IsVideoDb())
    { // this case is needed unless we allow IsVideo() to return true for videodb items,
      // but then we have issues with playlists of videodb items
      CFileItemPtr item(new CFileItem(*pItem->GetVideoInfoTag()));
      queuedItems.Add(item);
    }
    else if (!pItem->IsNFO() && pItem->IsVideo())
    {
      queuedItems.Add(pItem);
    }
  }
}

int  CGUIWindowVideoBase::GetResumeItemOffset(const CFileItem *item)
{
  if (item->HasProperty("viewOffset"))
    return boost::lexical_cast<int>(item->GetProperty("viewOffset")) * 75 / 1000;
  
  return 0;
  
#ifdef FUCKING_DOOMED_CODE
  m_database.Open();
  long startoffset = 0;

  if (item->IsStack() && !g_guiSettings.GetBool("myvideos.treatstackasfile") )
  {
    CStdStringArray movies;
    GetStackedFiles(item->m_strPath, movies);

    /* check if any of the stacked files have a resume bookmark */
    for (unsigned i = 0; i<movies.size();i++)
    {
      CBookmark bookmark;
      if (m_database.GetResumeBookMark(movies[i], bookmark))
      {
        startoffset = (long)(bookmark.timeInSeconds*75);
        startoffset += 0x10000000 * (i+1); /* store file number in here */
        break;
      }
    }
  }
  else if (!item->IsNFO() && !item->IsPlayList())
  {
    CBookmark bookmark;
    CStdString strPath = item->m_strPath;
    if (item->IsVideoDb() && item->HasVideoInfoTag())
      strPath = item->GetVideoInfoTag()->m_strFileNameAndPath;

    if (m_database.GetResumeBookMark(strPath, bookmark))
      startoffset = (long)(bookmark.timeInSeconds*75);
  }
  m_database.Close();
  
  return startoffset;
#endif
}

bool CGUIWindowVideoBase::OnClick(int iItem)
{
  if (g_guiSettings.GetInt("myvideos.resumeautomatically") != RESUME_NO)
    OnResumeItem(iItem);
  else
    return CGUIMediaWindow::OnClick(iItem);

  return true;
}

void CGUIWindowVideoBase::OnRestartItem(int iItem)
{
  CGUIMediaWindow::OnClick(iItem);
}

void CGUIWindowVideoBase::OnResumeItem(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems->Size()) return;
  CFileItemPtr item = m_vecItems->Get(iItem);
  
  // we always resume the movie if the user doesn't want us to ask
  bool resumeItem = g_guiSettings.GetInt("myvideos.resumeautomatically") != RESUME_ASK;
  if (!item->m_bIsFolder && !resumeItem)
  {
    // See if we have a view offset.
    if (item->HasProperty("viewOffset"))
    { 
      float seconds = boost::lexical_cast<int>(item->GetProperty("viewOffset")) / 1000.0f;
      
      // prompt user whether they wish to resume
      vector<CStdString> choices;
      CStdString resumeString, time;
      StringUtils::SecondsToTimeString(seconds, time);
      resumeString.Format(g_localizeStrings.Get(12022).c_str(), time.c_str());
      choices.push_back(resumeString);
      choices.push_back(g_localizeStrings.Get(12021)); // start from the beginning
      int retVal = CGUIDialogContextMenu::ShowAndGetChoice(choices, GetContextPosition());
      if (!retVal)
        return; // don't do anything
      resumeItem = (retVal == 1);
    }
  }
  
  if (resumeItem)
    item->m_lStartOffset = STARTOFFSET_RESUME;
  
  CGUIMediaWindow::OnClick(iItem);
}

void CGUIWindowVideoBase::GetContextButtons(int itemNumber, CContextButtons &buttons)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);

  bool includeStandardContextButtons = true;
  // contextual buttons
  if (item)
  {
    includeStandardContextButtons = item->m_includeStandardContextItems;
    if (!item->IsParentFolder() && includeStandardContextButtons)
    {
#if 0
      CStdString path(item->m_strPath);
      if (CUtil::IsStack(path))
      {
        vector<long> times;
        if (m_database.GetStackTimes(path,times))
          buttons.Add(CONTEXT_BUTTON_PLAY_PART, 20324);
      }
#endif

      if (GetID() != WINDOW_VIDEO_NAV || (!m_vecItems->m_strPath.IsEmpty() && 
         !item->m_strPath.Left(19).Equals("newsmartplaylist://")))
      {
        buttons.Add(CONTEXT_BUTTON_QUEUE_ITEM, 13347);      // Add to Playlist
      }

      // allow a folder to be ad-hoc queued and played by the default player
      if (item->m_bIsFolder || (item->IsPlayList() && 
         !g_advancedSettings.m_playlistAsFolders))
      {
        buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 208);
      }
      else
      { // get players
        VECPLAYERCORES vecCores;
        if (item->IsVideoDb())
        {
          CFileItem item2;
          item2.m_strPath = item->GetVideoInfoTag()->m_strFileNameAndPath;
          CPlayerCoreFactory::GetPlayers(item2, vecCores);
        }
        else
          CPlayerCoreFactory::GetPlayers(*item, vecCores);
        if (vecCores.size() > 1)
          buttons.Add(CONTEXT_BUTTON_PLAY_WITH, 15213);
      }
      if (item->IsSmartPlayList())
      {
        buttons.Add(CONTEXT_BUTTON_PLAY_PARTYMODE, 15216); // Play in Partymode
      }

      // if autoresume is enabled then add restart video button
      // check to see if the Resume Video button is applicable
      if (GetResumeItemOffset(item.get()) > 0)
      {
        if (g_guiSettings.GetInt("myvideos.resumeautomatically") == RESUME_YES)
          buttons.Add(CONTEXT_BUTTON_RESTART_ITEM, 20132);    // Restart Video
        if (g_guiSettings.GetInt("myvideos.resumeautomatically") == RESUME_NO)
          buttons.Add(CONTEXT_BUTTON_RESUME_ITEM, 13381);     // Resume Video
      }
      if (item->IsSmartPlayList() || m_vecItems->IsSmartPlayList())
        buttons.Add(CONTEXT_BUTTON_EDIT_SMART_PLAYLIST, 586);
    }
  }
  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
}

void CGUIWindowVideoBase::GetNonContextButtons(int itemNumber, CContextButtons &buttons)
{
  if (g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO).size() > 0)
    buttons.Add(CONTEXT_BUTTON_NOW_PLAYING, 559);
}

bool CGUIWindowVideoBase::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item;
  if (itemNumber >= 0 && itemNumber < m_vecItems->Size())
    item = m_vecItems->Get(itemNumber);
  switch (button)
  {
  case CONTEXT_BUTTON_PLAY_PART:
    {
      CFileItemList items;
      CStdString path(item->m_strPath);
      if (item->IsVideoDb())
        path = item->GetVideoInfoTag()->m_strFileNameAndPath;

      CDirectory::GetDirectory(path,items);
      CGUIDialogFileStacking* dlg = (CGUIDialogFileStacking*)m_gWindowManager.GetWindow(WINDOW_DIALOG_FILESTACKING);
      if (!dlg) return true;
      dlg->SetNumberOfFiles(items.Size());
      dlg->DoModal();
      int btn2 = dlg->GetSelectedFile();
      if (btn2 > 0)
      {
        if (btn2 > 1)
        {
          vector<long> times;
          if (m_database.GetStackTimes(path,times))
            item->m_lStartOffset = times[btn2-2]*75; // wtf?
        }
        else
          item->m_lStartOffset = 0;

        // call CGUIMediaWindow::OnClick() as otherwise autoresume will kick in
        CGUIMediaWindow::OnClick(itemNumber);
      }
      return true;
    }
  case CONTEXT_BUTTON_QUEUE_ITEM:
    OnQueueItem(itemNumber);
    return true;

  case CONTEXT_BUTTON_PLAY_ITEM:
    PlayItem(itemNumber);
    return true;

  case CONTEXT_BUTTON_PLAY_WITH:
    {   
      VECPLAYERCORES vecCores;
      if (item->IsVideoDb())
      {
        CFileItem item2(*item->GetVideoInfoTag());
        CPlayerCoreFactory::GetPlayers(item2, vecCores);
      }
      else
        CPlayerCoreFactory::GetPlayers(*item, vecCores);
      g_application.m_eForcedNextPlayer = CPlayerCoreFactory::SelectPlayerDialog(vecCores);
      if (g_application.m_eForcedNextPlayer != EPC_NONE)
        OnClick(itemNumber);
      return true;
    }

  case CONTEXT_BUTTON_PLAY_PARTYMODE:
    g_partyModeManager.Enable(PARTYMODECONTEXT_VIDEO, m_vecItems->Get(itemNumber)->m_strPath);
    return true;

  case CONTEXT_BUTTON_RESTART_ITEM:
    OnRestartItem(itemNumber);
    return true;

  case CONTEXT_BUTTON_RESUME_ITEM:
    OnResumeItem(itemNumber);
    return true;

  case CONTEXT_BUTTON_NOW_PLAYING:
    m_gWindowManager.ActivateWindow(WINDOW_VIDEO_PLAYLIST);
    return true;

  case CONTEXT_BUTTON_INFO:
    {
      SScraperInfo info;
      OnInfo(item, info);
      return true;
    }
  case CONTEXT_BUTTON_STOP_SCANNING:
    {
      return true;
    }
  case CONTEXT_BUTTON_SCAN:
  case CONTEXT_BUTTON_UPDATE_TVSHOW:
    {
      if( !item)
        return false;
      SScraperInfo info;
      SScanSettings settings;
      GetScraperForItem(item.get(), info, settings);
      CStdString strPath = item->m_strPath;
      if (item->IsVideoDb() && (!item->m_bIsFolder || item->GetVideoInfoTag()->m_strPath.IsEmpty()))
        return false;

      if (item->IsVideoDb())
        strPath = item->GetVideoInfoTag()->m_strPath;

      if (info.strContent.IsEmpty())
        return false;

      if (item->m_bIsFolder)
      {
        m_database.SetPathHash(strPath,""); // to force scan
        OnScan(strPath,info,settings);
      }
      else
        OnInfo(item, info);

      return true;
    }
  case CONTEXT_BUTTON_DELETE:
    OnDeleteItem(itemNumber);
    return true;
  case CONTEXT_BUTTON_EDIT_SMART_PLAYLIST:
    {
      CStdString playlist = m_vecItems->Get(itemNumber)->IsSmartPlayList() ? m_vecItems->Get(itemNumber)->m_strPath : m_vecItems->m_strPath; // save path as activatewindow will destroy our items
      if (CGUIDialogSmartPlaylistEditor::EditPlaylist(playlist, "video"))
      { // need to update
        m_vecItems->RemoveDiscCache();
        Update(m_vecItems->m_strPath);
      }
      return true;
    }
  case CONTEXT_BUTTON_RENAME:
    OnRenameItem(itemNumber);
    return true;
  default:
    break;
  }
  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

void CGUIWindowVideoBase::GetStackedFiles(const CStdString &strFilePath1, vector<CStdString> &movies)
{
  CStdString strFilePath = strFilePath1;  // we're gonna be altering it

  movies.clear();

  CURL url(strFilePath);
  if (url.GetProtocol() == "stack")
  {
    CStackDirectory dir;
    CFileItemList items;
    dir.GetDirectory(strFilePath, items);
    for (int i = 0; i < items.Size(); ++i)
      movies.push_back(items[i]->m_strPath);
  }
  if (movies.empty())
    movies.push_back(strFilePath);
}

bool CGUIWindowVideoBase::OnPlayMedia(int iItem)
{
  if ( iItem < 0 || iItem >= (int)m_vecItems->Size() ) 
    return false;
  
  CFileItemPtr pItem = m_vecItems->Get(iItem);
  
  // If there is more than one media item, allow picking which one.
  if (pItem->m_mediaItems.size() > 1 && g_guiSettings.GetBool("videoplayer.alternatemedia") == true)
  {
    CFileItemList fileItems;
    vector<CStdString> items;
    CPlexDirectory mediaChoices;
    
    for (int i=0; i < pItem->m_mediaItems.size(); i++)
    {
      CFileItemPtr item = pItem->m_mediaItems[i];
      
      CStdString label;
      CStdString videoCodec = item->GetProperty("mediaTag-videoCodec").ToUpper();
      CStdString videoRes = item->GetProperty("mediaTag-videoResolution").ToUpper();
      
      if (videoCodec.size() == 0 && videoRes.size() == 0)
      {
        label = "Unknown";
      }
      else
      {
        if (isnumber(videoRes[0]))
          videoRes += "p";
      
        label += videoRes;
        label += " " + videoCodec;
      }
      
      items.push_back(label);
    }
    
    int choice = CGUIDialogContextMenu::ShowAndGetChoice(items, GetContextPosition());
    if (choice > 0)
    {
      // Steal the resume offset and mode.
      long offset = pItem->m_lStartOffset;
      CStdString resumeTime = pItem->GetProperty("viewOffset");
      
      // Copy over the selected item.
      pItem = pItem->m_mediaItems[choice-1];
      
      // Store what we need to resume.
      pItem->m_lStartOffset = offset;
      pItem->SetProperty("viewOffset", resumeTime);
    }
    else
    {
      return false;
    }
  }
  
  if (pItem->IsPlexMediaServer())
  {
    CFileItemList fileItems;
    vector<CStdString> items;
    // Probe for a possible Plex directory. CPlexDirectory will parse
    // the headers and not read the body unless the content-type is
    // correct, so we will not end up reading a lot of extra data
    // in the normal case where the item is a valid video source.
    CPlexDirectory plexDir(true, false);
    if (plexDir.GetDirectory(pItem->m_strPath, fileItems))
    {
      // Probing succeeded. The item is a Plex directory and should not
      // be forwarded to the DVDPlayer, as the player will not be able
      // to determine the input format and will fail to play the item.
      CFileItemPtr plexDirItem(new CFileItem(pItem->m_strPath, true));
      return CGUIMediaWindow::OnClick(plexDirItem, iItem);
    }
  }

  // party mode
  if (g_partyModeManager.IsEnabled(PARTYMODECONTEXT_VIDEO))
  {
    CPlayList playlistTemp;
    playlistTemp.Add(pItem);
    g_partyModeManager.AddUserSongs(playlistTemp, true);
    return true;
  }

  // Reset Playlistplayer, playback started now does
  // not use the playlistplayer.
  g_playlistPlayer.Reset();
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);

  if (pItem->m_strPath == "add" && pItem->GetLabel() == g_localizeStrings.Get(1026)) // 'add source button' in empty root
  {
    if (CGUIDialogMediaSource::ShowAndAddMediaSource("video"))
    {
      Update("");
      return true;
    }
    return false;
  }

  CFileItem item(*pItem);
  if (pItem->IsVideoDb())
  {
    item = CFileItem(*pItem->GetVideoInfoTag());
    item.m_lStartOffset = pItem->m_lStartOffset;
  }

  PlayMovie(&item);

  return true;
}

void CGUIWindowVideoBase::PlayMovie(const CFileItem *item)
{
  CFileItemList movieList;
  int selectedFile = 1;
  long startoffset = item->m_lStartOffset;

  if (item->IsStack() && !g_guiSettings.GetBool("myvideos.treatstackasfile"))
  {
    CStdStringArray movies;
    GetStackedFiles(item->m_strPath, movies);

    if (item->m_lStartOffset == STARTOFFSET_RESUME)
    {
      startoffset = GetResumeItemOffset(item);

      if (startoffset & 0xF0000000) /* file is specified as a flag */
      {
        selectedFile = (startoffset>>28);
        startoffset = startoffset & ~0xF0000000;
      }
      else
      {
        /* attempt to start on a specific time in a stack */
        /* if we are lucky, we might have stored timings for */
        /* this stack at some point */

        m_database.Open();

        /* figure out what file this time offset is */
        vector<long> times;
        m_database.GetStackTimes(item->m_strPath, times);
        long totaltime = 0;
        for (unsigned i = 0; i < times.size(); i++)
        {
          totaltime += times[i]*75;
          if (startoffset < totaltime )
          {
            selectedFile = i+1;
            startoffset -= totaltime - times[i]*75; /* rebase agains selected file */
            break;
          }
        }
        m_database.Close();
      }
    }
    else
    { // show file stacking dialog
      CGUIDialogFileStacking* dlg = (CGUIDialogFileStacking*)m_gWindowManager.GetWindow(WINDOW_DIALOG_FILESTACKING);
      if (dlg)
      {
        dlg->SetNumberOfFiles(movies.size());
        dlg->DoModal();
        selectedFile = dlg->GetSelectedFile();
        if (selectedFile < 1) 
          return;
      }
    }
    // add to our movie list
    for (unsigned int i = 0; i < movies.size(); i++)
    {
      CFileItemPtr movieItem(new CFileItem(movies[i], false));
      movieList.Add(movieItem);
    }
  }
  else
  {
    CFileItemPtr movieItem(new CFileItem(*item));
    movieList.Add(movieItem);
  }

  g_playlistPlayer.Reset();
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
  CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_VIDEO);
  playlist.Clear();
  for (int i = selectedFile - 1; i < (int)movieList.Size(); ++i)
  {
    if (i == selectedFile - 1)
      movieList[i]->m_lStartOffset = startoffset;
    playlist.Add(movieList[i]);
  }

  if(m_thumbLoader.IsLoading())
    m_thumbLoader.StopAsync();

  // play movie...
  g_playlistPlayer.Play(0);

  if(!g_application.IsPlayingVideo())
    m_thumbLoader.Load(*m_vecItems);
}

void CGUIWindowVideoBase::OnDeleteItem(int iItem)
{
  if ( iItem < 0 || iItem >= m_vecItems->Size()) 
    return;

  CFileItemPtr item = m_vecItems->Get(iItem);
  // HACK: stacked files need to be treated as folders in order to be deleted
  if (item->IsStack())
    item->m_bIsFolder = true;
  if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].getLockMode() != LOCK_MODE_EVERYONE &&
      g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].filesLocked())
  {
    if (!g_passwordManager.IsMasterLockUnlocked(true))
      return;
  }
  
  if (!CGUIWindowFileManager::DeleteItem(item.get()))
    return;
  
  Update(m_vecItems->m_strPath);
  m_viewControl.SetSelectedItem(iItem);
}

void CGUIWindowVideoBase::MarkUnWatched(const CFileItemPtr &item)
{
  PlexMediaServerQueue::Get().onUnviewed(item);
  
  item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_UNWATCHED);
  if (item->GetVideoInfoTag())
    item->GetVideoInfoTag()->m_playCount = 0;
  
  // Fix numbers.
  if (item->HasProperty("watchedepisodes"))
  {
    int watched = boost::lexical_cast<int>(item->GetProperty("watchedepisodes"));
    int unwatched = boost::lexical_cast<int>(item->GetProperty("unwatchedepisodes"));
    
    item->SetEpisodeData(watched+unwatched, 0);
  }
}

//Add Mark a Title as watched
void CGUIWindowVideoBase::MarkWatched(const CFileItemPtr &item)
{
  PlexMediaServerQueue::Get().onViewed(item, true);

  // Change the item.
  item->SetOverlayImage(CGUIListItem::ICON_OVERLAY_WATCHED);
  if (item->GetVideoInfoTag())
    item->GetVideoInfoTag()->m_playCount++;
  
  // Fix numbers.
  if (item->HasProperty("watchedepisodes"))
  {
    int watched = boost::lexical_cast<int>(item->GetProperty("watchedepisodes"));
    int unwatched = boost::lexical_cast<int>(item->GetProperty("unwatchedepisodes"));

    item->SetEpisodeData(watched+unwatched, watched+unwatched);
  }
}

//Add change a title's name
void CGUIWindowVideoBase::UpdateVideoTitle(const CFileItem* pItem)
{
  CVideoInfoTag detail;
  CVideoDatabase database;
  database.Open();

  VIDEODB_CONTENT_TYPE iType=VIDEODB_CONTENT_MOVIES;
  if (pItem->HasVideoInfoTag() && (!pItem->GetVideoInfoTag()->m_strShowTitle.IsEmpty() || 
      pItem->GetVideoInfoTag()->m_iEpisode > 0))
  {
    iType = VIDEODB_CONTENT_TVSHOWS;
  }
  if (pItem->HasVideoInfoTag() && pItem->GetVideoInfoTag()->m_iSeason > -1 && !pItem->m_bIsFolder)
    iType = VIDEODB_CONTENT_EPISODES;
  if (pItem->HasVideoInfoTag() && !pItem->GetVideoInfoTag()->m_strArtist.IsEmpty())
    iType = VIDEODB_CONTENT_MUSICVIDEOS;

  if (iType == VIDEODB_CONTENT_MOVIES)
    database.GetMovieInfo("", detail, pItem->GetVideoInfoTag()->m_iDbId);
  if (iType == VIDEODB_CONTENT_EPISODES)
    database.GetEpisodeInfo(pItem->m_strPath,detail,pItem->GetVideoInfoTag()->m_iDbId);
  if (iType == VIDEODB_CONTENT_TVSHOWS)
    database.GetTvShowInfo(pItem->GetVideoInfoTag()->m_strFileNameAndPath,detail,pItem->GetVideoInfoTag()->m_iDbId);
  if (iType == VIDEODB_CONTENT_MUSICVIDEOS)
    database.GetMusicVideoInfo(pItem->GetVideoInfoTag()->m_strFileNameAndPath,detail,pItem->GetVideoInfoTag()->m_iDbId);

  CStdString strInput;
  strInput = detail.m_strTitle;

  //Get the new title
  if (!CGUIDialogKeyboard::ShowAndGetInput(strInput, g_localizeStrings.Get(16105), false)) 
    return;

  database.UpdateMovieTitle(pItem->GetVideoInfoTag()->m_iDbId, strInput, iType);
}

void CGUIWindowVideoBase::LoadPlayList(const CStdString& strPlayList, int iPlayList /* = PLAYLIST_VIDEO */)
{
  // if partymode is active, we disable it
  if (g_partyModeManager.IsEnabled())
    g_partyModeManager.Disable();

  // load a playlist like .m3u, .pls
  // first get correct factory to load playlist
  auto_ptr<CPlayList> pPlayList (CPlayListFactory::Create(strPlayList));
  if (pPlayList.get())
  {
    // load it
    if (!pPlayList->Load(strPlayList))
    {
      CGUIDialogOK::ShowAndGetInput(6, 0, 477, 0);
      return; //hmmm unable to load playlist?
    }
  }

  if (g_application.ProcessAndStartPlaylist(strPlayList, *pPlayList, iPlayList))
  {
    if (m_guiState.get())
      m_guiState->SetPlaylistDirectory("playlistvideo://");
  }
}

void CGUIWindowVideoBase::PlayItem(int iItem)
{
  // restrictions should be placed in the appropiate window code
  // only call the base code if the item passes since this clears
  // the currently playing temp playlist

  const CFileItemPtr pItem = m_vecItems->Get(iItem);
  // if its a folder, build a temp playlist
  if (pItem->m_bIsFolder)
  {
    // take a copy so we can alter the queue state
    CFileItemPtr item(new CFileItem(*m_vecItems->Get(iItem)));

    //  Allow queuing of unqueueable items
    //  when we try to queue them directly
    if (!item->CanQueue())
      item->SetCanQueue(true);

    // skip ".."
    if (item->IsParentFolder())
      return;

    // recursively add items to list
    CFileItemList queuedItems;
    AddItemToPlayList(item, queuedItems);

    g_playlistPlayer.ClearPlaylist(PLAYLIST_VIDEO);
    g_playlistPlayer.Reset();
    g_playlistPlayer.Add(PLAYLIST_VIDEO, queuedItems);
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_VIDEO);
    g_playlistPlayer.Play();
  }
  else if (pItem->IsPlayList())
  {
    // load the playlist the old way
    LoadPlayList(pItem->m_strPath, PLAYLIST_VIDEO);
  }
  else
  {
    // single item, play it
    OnClick(iItem);
  }
}

bool CGUIWindowVideoBase::Update(const CStdString &strDirectory)
{
  if (m_thumbLoader.IsLoading())
    m_thumbLoader.StopThread();

  if (!CGUIMediaWindow::Update(strDirectory))
    return false;

  m_thumbLoader.Load(*m_vecItems);

  return true;
}

bool CGUIWindowVideoBase::GetDirectory(const CStdString &strDirectory, CFileItemList &items)
{
  bool bResult = CGUIMediaWindow::GetDirectory(strDirectory,items);
 
  // add in the "New Playlist" item if we're in the playlists folder
  if (items.m_strPath == "special://videoplaylists/" && !items.Contains("newplaylist://"))
  {
    CFileItemPtr newPlaylist(new CFileItem(g_settings.GetUserDataItem("PartyMode-Video.xsp"),false));
    newPlaylist->SetLabel(g_localizeStrings.Get(16035));
    newPlaylist->SetLabelPreformated(true);
    newPlaylist->m_bIsFolder = true;
    items.Add(newPlaylist);

/*    newPlaylist.reset(new CFileItem("newplaylist://", false));
    newPlaylist->SetLabel(g_localizeStrings.Get(525));
    newPlaylist->SetLabelPreformated(true);
    items.Add(newPlaylist);
*/
    newPlaylist.reset(new CFileItem("newsmartplaylist://video", false));
    newPlaylist->SetLabel(g_localizeStrings.Get(21437));  // "new smart playlist..."
    newPlaylist->SetLabelPreformated(true);
    items.Add(newPlaylist);
  }

  return bResult;
}

void CGUIWindowVideoBase::OnPrepareFileItems(CFileItemList &items)
{
  if (!items.m_strPath.Equals("plugin://video/"))
    items.SetCachedVideoThumbs();
}

void CGUIWindowVideoBase::AddToDatabase(int iItem)
{
  if (iItem < 0 || iItem >= m_vecItems->Size()) 
    return;
  
  CFileItemPtr pItem = m_vecItems->Get(iItem);
  if (pItem->IsParentFolder() || pItem->m_bIsFolder) 
    return;

  bool bGotXml = false;
  CVideoInfoTag movie;
  movie.Reset();

  // look for matching xml file first
  CStdString strXml = pItem->m_strPath + ".xml";
  if (pItem->IsStack())
  {
    // for a stack, use the first file in the stack
    CStackDirectory stack;
    strXml = stack.GetFirstStackedFile(pItem->m_strPath) + ".xml";
  }
  CStdString strCache = "Z:\\" + CUtil::GetFileName(strXml);
  CUtil::GetFatXQualifiedPath(strCache);
  if (CFile::Exists(strXml))
  {
    bGotXml = true;
    CLog::Log(LOGDEBUG,"%s: found matching xml file:[%s]", __FUNCTION__, strXml.c_str());
    CFile::Cache(strXml, strCache);
    CIMDB imdb;
    if (!imdb.LoadXML(strCache, movie, false))
    {
      CLog::Log(LOGERROR,"%s: Could not parse info in file:[%s]", __FUNCTION__, strXml.c_str());
      bGotXml = false;
    }
  }

  // prompt for data
  if (!bGotXml)
  {
    // enter a new title
    CStdString strTitle = pItem->GetLabel();
    if (!CGUIDialogKeyboard::ShowAndGetInput(strTitle, g_localizeStrings.Get(528), false)) // Enter Title
      return;

    // pick genre
    CGUIDialogSelect* pSelect = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
    if (!pSelect)
      return;

    pSelect->SetHeading(530); // Select Genre
    pSelect->Reset();
    CFileItemList items;
    if (!CDirectory::GetDirectory("videodb://1/1/", items))
      return;
    pSelect->SetItems(&items);
    pSelect->EnableButton(true);
    pSelect->SetButtonLabel(531); // New Genre
    pSelect->DoModal();
    CStdString strGenre;
    int iSelected = pSelect->GetSelectedLabel();
    if (iSelected >= 0)
      strGenre = items[iSelected]->GetLabel();
    else if (!pSelect->IsButtonPressed())
      return;

    // enter new genre string
    if (strGenre.IsEmpty())
    {
      strGenre = g_localizeStrings.Get(532); // Manual Addition
      if (!CGUIDialogKeyboard::ShowAndGetInput(strGenre, g_localizeStrings.Get(533), false)) // Enter Genre
        return; // user backed out
      if (strGenre.IsEmpty())
        return; // no genre string
    }

    // set movie info
    movie.m_strTitle = strTitle;
    movie.m_strGenre = strGenre;
  }

  // everything is ok, so add to database
  m_database.Open();
  long lMovieId = m_database.AddMovie(pItem->m_strPath);
  movie.m_strIMDBNumber.Format("xx%08i", lMovieId);
  m_database.SetDetailsForMovie(pItem->m_strPath, movie);
  m_database.Close();

  // done...
  CGUIDialogOK *pDialog = (CGUIDialogOK*)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
  if (pDialog)
  {
    pDialog->SetHeading(20177); // Done
    pDialog->SetLine(0, movie.m_strTitle);
    pDialog->SetLine(1, movie.m_strGenre);
    pDialog->SetLine(2, movie.m_strIMDBNumber);
    pDialog->SetLine(3, "");
    pDialog->DoModal();
  }

  // library view cache needs to be cleared
  CUtil::DeleteVideoDatabaseDirectoryCache();
}

/// \brief Search the current directory for a string got from the virtual keyboard
void CGUIWindowVideoBase::OnSearch()
{
  CStdString strSearch;
  if (!CGUIDialogKeyboard::ShowAndGetInput(strSearch, g_localizeStrings.Get(16017), false))
    return ;

  strSearch.ToLower();
  if (m_dlgProgress)
  {
    m_dlgProgress->SetHeading(194);
    m_dlgProgress->SetLine(0, strSearch);
    m_dlgProgress->SetLine(1, "");
    m_dlgProgress->SetLine(2, "");
    m_dlgProgress->StartModal();
    m_dlgProgress->Progress();
  }
  CFileItemList items;
  DoSearch(strSearch, items);

  if (items.Size())
  {
    CGUIDialogSelect* pDlgSelect = (CGUIDialogSelect*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SELECT);
    pDlgSelect->Reset();
    pDlgSelect->SetHeading(283);
    items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);

    for (int i = 0; i < (int)items.Size(); i++)
    {
      CFileItemPtr pItem = items[i];
      pDlgSelect->Add(pItem->GetLabel());
    }

    pDlgSelect->DoModal();

    int iItem = pDlgSelect->GetSelectedLabel();
    if (iItem < 0)
    {
      if (m_dlgProgress) 
        m_dlgProgress->Close();
      
      return ;
    }

    CFileItemPtr pSelItem = items[iItem];

    OnSearchItemFound(pSelItem.get());

    if (m_dlgProgress) 
      m_dlgProgress->Close();
  }
  else
  {
    if (m_dlgProgress) 
      m_dlgProgress->Close();
    
    CGUIDialogOK::ShowAndGetInput(194, 284, 0, 0);
  }
}

/// \brief React on the selected search item
/// \param pItem Search result item
void CGUIWindowVideoBase::OnSearchItemFound(const CFileItem* pSelItem)
{
  if (pSelItem->m_bIsFolder)
  {
    CStdString strPath = pSelItem->m_strPath;
    CStdString strParentPath;
    CUtil::GetParentPath(strPath, strParentPath);

    Update(strParentPath);

    SetHistoryForPath(strParentPath);

    strPath = pSelItem->m_strPath;
    CURL url(strPath);
    if (pSelItem->IsSmb() && !CUtil::HasSlashAtEnd(strPath))
      strPath += "/";

    for (int i = 0; i < m_vecItems->Size(); i++)
    {
      CFileItemPtr pItem = m_vecItems->Get(i);
      if (pItem->m_strPath == strPath)
      {
        m_viewControl.SetSelectedItem(i);
        break;
      }
    }
  }
  else
  {
    CStdString strPath;
    CUtil::GetDirectory(pSelItem->m_strPath, strPath);

    Update(strPath);

    SetHistoryForPath(strPath);

    for (int i = 0; i < (int)m_vecItems->Size(); i++)
    {
      CFileItemPtr pItem = m_vecItems->Get(i);
      if (pItem->m_strPath == pSelItem->m_strPath)
      {
        m_viewControl.SetSelectedItem(i);
        break;
      }
    }
  }
  m_viewControl.SetFocused();
}

int CGUIWindowVideoBase::GetScraperForItem(CFileItem *item, SScraperInfo &info, SScanSettings& settings)
{
  if (!item) 
    return 0;
  
  int found = 0;
  if (item->HasVideoInfoTag())  // files view shouldn't need this check I think?
    m_database.GetScraperForPath(item->GetVideoInfoTag()->m_strPath,info,settings,found);
  else
    m_database.GetScraperForPath(item->m_strPath,info,settings,found);
  CScraperParser parser;
  if (parser.Load("q:\\system\\scrapers\\video\\"+info.strPath))
    info.strTitle = parser.GetName();
  
  return found;
}

void CGUIWindowVideoBase::OnScan(const CStdString& strPath, const SScraperInfo& info, const SScanSettings& settings)
{
}
