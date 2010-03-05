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

#include <boost/foreach.hpp>

#include "stdafx.h"
#include "FileSystem/StackDirectory.h"
#include "ThumbLoader.h"
#include "Util.h"
#include "URL.h"
#include "Picture.h"
#include "FileSystem/File.h"
#include "FileItem.h"
#include "Settings.h"


#include "cores/dvdplayer/DVDFileInfo.h"

using namespace XFILE;
using namespace DIRECTORY;

CVideoThumbLoader::CVideoThumbLoader() 
{  
}

CVideoThumbLoader::~CVideoThumbLoader()
{
  StopThread();
}

void CVideoThumbLoader::OnLoaderStart() 
{
}

void CVideoThumbLoader::OnLoaderFinish() 
{
}

bool CVideoThumbLoader::ExtractThumb(const CStdString &strPath, const CStdString &strTarget)
{
  if (!g_guiSettings.GetBool("myvideos.autothumb"))
    return false;

  if (CUtil::IsMythTV(strPath)
  ||  CUtil::IsUPnP(strPath)
  ||  CUtil::IsTuxBox(strPath)
  ||  CUtil::IsDAAP(strPath))
    return false;

  if (CUtil::IsRemote(strPath) && !CUtil::IsOnLAN(strPath))
    return false;

  CLog::Log(LOGDEBUG,"%s - trying to extract thumb from video file %s", __FUNCTION__, strPath.c_str());
  return CDVDFileInfo::ExtractThumb(strPath, strTarget);
}

bool CVideoThumbLoader::LoadItem(CFileItem* pItem)
{
 
  if (pItem->m_bIsShareOrDrive) return true;
  CStdString cachedThumb(pItem->GetCachedVideoThumb());

  if (!pItem->HasThumbnail())
  {
    pItem->SetUserVideoThumb();
    if (!CFile::Exists(cachedThumb))
    {
      CStdString strPath, strFileName;
      CUtil::Split(cachedThumb, strPath, strFileName);
       
      // create unique thumb for auto generated thumbs
      cachedThumb = strPath + "auto-" + strFileName;
      if (pItem->IsVideo() && !pItem->IsInternetStream() && !pItem->IsPlayList() && !CFile::Exists(cachedThumb))
      {
        if (pItem->IsStack())
        {
          CStackDirectory stack;
          CVideoThumbLoader::ExtractThumb(stack.GetFirstStackedFile(pItem->m_strPath), cachedThumb);
        }
        else
        {
          CVideoThumbLoader::ExtractThumb(pItem->m_strPath, cachedThumb);
        }
      }
  
      if (CFile::Exists(cachedThumb))
      {
        pItem->SetProperty("HasAutoThumb", "1");
        pItem->SetProperty("AutoThumbImage", cachedThumb);
        pItem->SetThumbnailImage(cachedThumb);
      }
    }
  }
  else
  {
    // look for remote thumbs
    CStdString thumb(pItem->GetThumbnailImage());
    if (!CURL::IsFileOnly(thumb) && !CUtil::IsHD(thumb))
    {      
      if(CFile::Exists(cachedThumb))
          pItem->SetThumbnailImage(cachedThumb);
      else
      {
        CPicture pic;
        if(pic.DoCreateThumbnail(thumb, cachedThumb))
          pItem->SetThumbnailImage(cachedThumb);
        else
          pItem->SetThumbnailImage("");
      }
    }  
  }

  if (!pItem->HasProperty("fanart_image"))
  {
    pItem->CacheFanart();
    
    if (pItem->GetQuickFanart().size() > 0)
    {
      if (CFile::Exists(pItem->GetCachedPlexMediaServerFanart()))
        pItem->SetProperty("fanart_image", pItem->GetCachedPlexMediaServerFanart());
    }
    else
    {
      if (CFile::Exists(pItem->GetCachedFanart()))
        pItem->SetProperty("fanart_image", pItem->GetCachedFanart());
    }
  }
  
  // Walk through properties and see if there are any image resources to be loaded.
  std::map<CStdString, CStdString, CGUIListItem::icompare>& properties = pItem->GetPropertyDict();
  typedef pair<CStdString, CStdString> PropertyPair;
  BOOST_FOREACH(PropertyPair pair, properties)
  {
    if (pair.first.substr(0, 6) == "cache$")
    {
      string name = pair.first.substr(6);
      string url = pair.second;
      
      string localFile = CFileItem::GetCachedPlexMediaServerThumb(url);
      if (CFile::Exists(localFile) == false)
      {
        CPicture pic;
        if (pic.DoCreateThumbnail(url, localFile))
          pItem->SetProperty(name, localFile);
        else
          printf("%s Failure downloading %s\n", name.c_str(), url.c_str());
      }
      else
      {
        pItem->SetProperty(name, localFile);
      }
      
      if (pair.first.Find("movieStudio") != -1)
        printf("movieStudio => %s\n", pItem->GetProperty(name).c_str());
    }
  }
  
  return true;
}

CProgramThumbLoader::CProgramThumbLoader()
{
}

CProgramThumbLoader::~CProgramThumbLoader()
{
}

bool CProgramThumbLoader::LoadItem(CFileItem *pItem)
{
  if (pItem->m_bIsShareOrDrive) return true;
  if (!pItem->HasThumbnail())
    pItem->SetUserProgramThumb();
  else
  {
    // look for remote thumbs
    CStdString thumb(pItem->GetThumbnailImage());
    if (!CURL::IsFileOnly(thumb) && !CUtil::IsHD(thumb))
    {
      CStdString cachedThumb(pItem->GetCachedProgramThumb());
      if(CFile::Exists(cachedThumb))
        pItem->SetThumbnailImage(cachedThumb);
      else
      {
        CPicture pic;
        if(pic.DoCreateThumbnail(thumb, cachedThumb))
          pItem->SetThumbnailImage(cachedThumb);
        else
          pItem->SetThumbnailImage("");
      }
    }  
  }
  
  if (!pItem->HasProperty("fanart_image"))
  {
    pItem->CacheFanart();
    if (CFile::Exists(pItem->GetCachedFanart()))
      pItem->SetProperty("fanart_image",pItem->GetCachedFanart());
  }
  
  return true;
}

CMusicThumbLoader::CMusicThumbLoader(int numThreads, int pauseBetweenLoads)
  : CBackgroundInfoLoader(numThreads, pauseBetweenLoads)
{
}

CMusicThumbLoader::~CMusicThumbLoader()
{
}

bool CMusicThumbLoader::LoadItem(CFileItem* pItem)
{ 
  if (pItem->m_bIsShareOrDrive) return true;
  if (!pItem->HasThumbnail())
  {
    pItem->SetUserMusicThumb();
  }
  else
  {
    // look for remote thumbs
    CStdString thumb(pItem->GetThumbnailImage());
    if (!CURL::IsFileOnly(thumb) && !CUtil::IsHD(thumb))
    {
      CStdString cachedThumb(pItem->GetCachedMusicThumb());
      if(CFile::Exists(cachedThumb))
      {
        pItem->SetThumbnailImage(cachedThumb);
      }
      else
      {
        CPicture pic;
        if(pic.DoCreateThumbnail(thumb, cachedThumb))
          pItem->SetThumbnailImage(cachedThumb);
        else
          pItem->SetThumbnailImage("");
      }
    }  
  }
  
  if (!pItem->HasProperty("fanart_image"))
  {
    pItem->CacheFanart();
    if (CFile::Exists(pItem->GetCachedProgramFanart()))
      pItem->SetProperty("fanart_image",pItem->GetCachedProgramFanart());
  }
 
  return true;
}

