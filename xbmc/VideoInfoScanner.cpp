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
#include "VideoInfoScanner.h"
#include "FileSystem/DirectoryCache.h"
#include "Util.h"
#include "NfoFile.h"
#include "utils/RegExp.h"
#include "utils/md5.h"
#include "Picture.h"
#include "FileSystem/StackDirectory.h"
#include "xbox/XKGeneral.h"
#include "utils/IMDB.h"
#include "utils/GUIInfoManager.h"
#include "FileSystem/File.h"
#include "GUIDialogProgress.h"
#include "Settings.h"
#include "FileItem.h"
#include "CocoaUtils.h"

#define REGEXSAMPLEFILE "[-\\._ ](sample|trailer)[-\\._ ]"

using namespace std;
using namespace DIRECTORY;
using namespace XFILE;

namespace VIDEO 
{

  CVideoInfoScanner::CVideoInfoScanner()
  {
    m_bRunning = false;
    m_pObserver = NULL;
    m_bCanInterrupt = false;
    m_currentItem=0;
    m_itemCount=0;
    m_bClean=false;
  }

  CVideoInfoScanner::~CVideoInfoScanner()
  {
  }

  void CVideoInfoScanner::Process()
  {
  }

  void CVideoInfoScanner::Start(const CStdString& strDirectory, const SScraperInfo& info, const SScanSettings& settings, bool bUpdateAll)
  {
  }

  bool CVideoInfoScanner::IsScanning()
  {
    return m_bRunning;
  }

  void CVideoInfoScanner::Stop()
  {
    StopThread();
  }

  void CVideoInfoScanner::SetObserver(IVideoInfoScannerObserver* pObserver)
  {
    m_pObserver = pObserver;
  }

  bool CVideoInfoScanner::DoScan(const CStdString& strDirectory, SScanSettings settings)
  {
    return false;
  }

  bool CVideoInfoScanner::RetrieveVideoInfo(CFileItemList& items, bool bDirNames, const SScraperInfo& info, bool bRefresh, CScraperUrl* pURL, CGUIDialogProgress* pDlgProgress)
  {
    return true;
  }

  // This function is run by another thread
  void CVideoInfoScanner::Run()
  {
  }

  // Recurse through all folders we scan and count files
  int CVideoInfoScanner::CountFiles(const CStdString& strPath)
  {
    return 0;
  }

  void CVideoInfoScanner::EnumerateSeriesFolder(const CFileItem* item, IMDB_EPISODELIST& episodeList)
  {
  }

  long CVideoInfoScanner::AddMovieAndGetThumb(CFileItem *pItem, const CStdString &content, CVideoInfoTag &movieDetails, long idShow, bool bApplyToDir /*=false*/, CGUIDialogProgress* pDialog /* = NULL */)
  {
    return 0;
  }

  void CVideoInfoScanner::OnProcessSeriesFolder(IMDB_EPISODELIST& episodes, IMDB_EPISODELIST& files, long lShowId, CIMDB& IMDB, const CStdString& strShowTitle, CGUIDialogProgress* pDlgProgress /* = NULL */)
  {
  }

  CStdString CVideoInfoScanner::GetnfoFile(CFileItem *item, bool bGrabAny)
  {
    return "";
  }

  long CVideoInfoScanner::GetIMDBDetails(CFileItem *pItem, CScraperUrl &url, const SScraperInfo& info, bool bUseDirNames, CGUIDialogProgress* pDialog /* = NULL */)
  {
    return -1;
  }

  void CVideoInfoScanner::ApplyIMDBThumbToFolder(const CStdString &folder, const CStdString &imdbThumb)
  {
  }

  int CVideoInfoScanner::GetPathHash(const CFileItemList &items, CStdString &hash)
  {
  }

  void CVideoInfoScanner::FetchSeasonThumbs(long lTvShowId)
  {
  }

  void CVideoInfoScanner::FetchActorThumbs(const vector<SActorInfo>& actors)
  {
  }

  CVideoInfoScanner::NFOResult CVideoInfoScanner::CheckForNFOFile(CFileItem* pItem, bool bGrabAny, SScraperInfo& info, CGUIDialogProgress* pDlgProgress, CScraperUrl& scrUrl)
  {
    return NO_NFO;
  }
}

