/*
 * PlexDirectory.cpp
 *
 *  Created on: Oct 4, 2008
 *      Author: Elan Feingold
 */
#include <boost/lexical_cast.hpp>

#include "stdafx.h"
#include "Album.h"
#include "PlexDirectory.h"
#include "DirectoryCache.h"
#include "Util.h"
#include "FileCurl.h"
#include "utils/HttpHeader.h"
#include "VideoInfoTag.h"
#include "MusicInfoTag.h"
#include "GUIWindowManager.h"
#include "GUIDialogProgress.h"
#include "URL.h"
#include "FileItem.h"

using namespace std;
using namespace XFILE;
using namespace DIRECTORY;

#define MASTER_PLEX_MEDIA_SERVER "http://localhost:32400"

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexDirectory::CPlexDirectory()
  : m_bStop(false)
  , m_bSuccess(true)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexDirectory::~CPlexDirectory()
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CPlexDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CStdString strRoot = strPath;
  if (CUtil::HasSlashAtEnd(strRoot) && strRoot != "plex://")
    strRoot.Delete(strRoot.size() - 1);
  
  // See if it's cached.
  if (g_directoryCache.GetDirectory(strRoot, items))
    return true;

  strRoot.Replace(" ", "%20");

  // Start the download thread running.
  m_url = strRoot;
  //CThread::Create(false, 0);
  Process();

  // Now display progress, look for cancel.
  CGUIDialogProgress* dlgProgress = 0;
  
#if 0
  int time = GetTickCount();
  
  while (m_downloadEvent.WaitMSec(100) == false)
  {
    // If enough time has passed, display the dialog.
    if (GetTickCount() - time > 3000)
    {
      dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
      if (dlgProgress)
      {
        dlgProgress->ShowProgressBar(false);
        dlgProgress->SetHeading(260);
        dlgProgress->SetLine(0, 14003);
        dlgProgress->SetLine(1, "");
        dlgProgress->SetLine(2, "");
        dlgProgress->StartModal();
      }
    }

    if (dlgProgress)
    {
      if (dlgProgress->IsCanceled())
      {
        printf("Cancelling.....\n");
        StopThread();
      }
      
      dlgProgress->Progress();
    }
  }
#endif
  
  // Parse returned xml.
  TiXmlDocument doc;
  doc.Parse(m_data.c_str());

  TiXmlElement* root = doc.RootElement();
  if(root == 0)
  {
    CLog::Log(LOGERROR, "%s - Unable to parse xml\n%s", __FUNCTION__, m_data.c_str());
    if (dlgProgress) dlgProgress->Close();
    return false;
  }

  // Walk the parsed tree.
  string strFileLabel = "%N - %T"; 
  string strDirLabel = "%B";
  
  Parse(m_url, root, items, strFileLabel, strDirLabel);
  items.AddSortMethod(SORT_METHOD_NONE, 552, LABEL_MASKS(strFileLabel, "%D", strDirLabel, "%Y"));

  CFileItemList vecCacheItems;
  g_directoryCache.ClearDirectory(strRoot);
  for( int i = 0; i <items.Size(); i++ )
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->IsParentFolder())
      vecCacheItems.Add(pItem);
  }
  
  //g_directoryCache.SetDirectory(strRoot, vecCacheItems);
  if (dlgProgress) dlgProgress->Close();
  
  return true;
}

class PlexMediaNode
{
 public:
   static PlexMediaNode* Create(const string& name);
   
   CFileItemPtr BuildFileItem(const CURL& url, TiXmlElement& el, string& strFileLabel, string& strDirLabel)
   {
     CFileItemPtr pItem(new CFileItem());
     pItem->m_bIsFolder = true;
     
     url.GetURL(pItem->m_strPath);
     if (pItem->m_strPath[pItem->m_strPath.size()-1] != '/')
       pItem->m_strPath += "/";
     pItem->m_strPath += el.Attribute("key");
     
     // Let subclass finish.
     DoBuildFileItem(pItem, el);
     
     // Make sure we have the trailing slash.
     if (pItem->m_bIsFolder == true && pItem->m_strPath[pItem->m_strPath.size()-1] != '/')
       pItem->m_strPath += "/";
     
     return pItem;
   }
   
   virtual void DoBuildFileItem(CFileItemPtr& pItem, TiXmlElement& el) = 0;
   virtual void ComputeLabels(const string& strPath, string& strFileLabel, string& strDirLabel)
   {
     strFileLabel = "%N - %T";
     strDirLabel = "%B";
     
     if (strPath.find("/Albums") != -1)
       strDirLabel = "%B - %A";
     else if (strPath.find("/Recently%20Played/By%20Artist") != -1)
       strDirLabel = "%B";
     else if (strPath.find("/Decades/") != -1 || strPath.find("/Recently%20Added") != -1 || strPath.find("/Recently%20Played/") != -1 || strPath.find("/Genre/") != -1)
       strDirLabel = "%A - %B";
   }
};

class PlexMediaDirectory : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, TiXmlElement& el)
  {
    pItem->SetLabel(el.Attribute("name"));
  }
};

class PlexMediaObject : public PlexMediaNode
{  
  virtual void DoBuildFileItem(CFileItemPtr& pItem, TiXmlElement& el) {}
};

class PlexMediaArtist : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, TiXmlElement& el)
  {
    pItem->SetLabel(el.Attribute("artist"));
    
    string path = el.Attribute("thumb");
    
    CURL url(pItem->m_strPath);
    url.SetProtocol("http");
    url.SetFileName(path.substr(1));
    url.SetPort(32400);
    
    CStdString theURL;
    url.GetURL(theURL);
    
    pItem->SetThumbnailImage(theURL);
  }
};

class PlexMediaAlbum : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, TiXmlElement& el) 
  {
    CAlbum album;
    
    album.strLabel = el.Attribute("label");
    album.idAlbum = boost::lexical_cast<int>(el.Attribute("key"));
    album.strAlbum = el.Attribute("album");
    album.strArtist = el.Attribute("artist");
    album.strGenre = el.Attribute("genre");
    album.iYear = boost::lexical_cast<int>(el.Attribute("year"));
    
    // Construct the thumbnail request.
    CURL url(pItem->m_strPath);
    url.SetProtocol("http");
    
    string path = el.Attribute("thumb");
    url.SetFileName(path.substr(1));
    url.SetPort(32400);
    
    CStdString theURL;
    url.GetURL(theURL);
    album.thumbURL = theURL;
    
    CFileItemPtr newItem(new CFileItem(pItem->m_strPath, album));
    pItem = newItem;
  }
};

class PlexMediaPodcast : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, TiXmlElement& el)
  {
    CAlbum album;
    
    album.strLabel = el.Attribute("title");
    album.idAlbum = boost::lexical_cast<int>(el.Attribute("key"));
    album.strAlbum = el.Attribute("title");
    
    if (strlen(el.Attribute("year")) > 0)
      album.iYear = boost::lexical_cast<int>(el.Attribute("year"));
    
    // Construct the thumbnail request.
    CURL url(pItem->m_strPath);
    url.SetProtocol("http");
    url.SetPort(32400);
    
    string path = el.Attribute("thumb");
    url.SetFileName(path.substr(1));
    
    CStdString theURL;
    url.GetURL(theURL);
    album.thumbURL = theURL;
    
    CFileItemPtr newItem(new CFileItem(pItem->m_strPath, album));
    pItem = newItem;
  }
};

class PlexMediaPlaylist : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, TiXmlElement& el)
  {
    pItem->SetLabel(el.Attribute("title"));
  }
};

class PlexMediaGenre : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, TiXmlElement& el)
  {
    pItem->SetLabel(el.Attribute("genre"));
  }
};

class PlexMediaTrack : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, TiXmlElement& el)
  {
    pItem->m_bIsFolder = false;
    
    CSong song;
    song.strTitle = (el.Attribute("track"));
    song.strArtist = el.Attribute("artist");
    song.strAlbum = el.Attribute("album");
    song.iDuration = boost::lexical_cast<int>(el.Attribute("totalTime"))/1000;
    song.iTrack = boost::lexical_cast<int>(el.Attribute("index"));
    song.strFileName = pItem->m_strPath;
        
    // Thumbnail.
    CURL url(pItem->m_strPath);
    url.SetProtocol("http");
    url.SetPort(32400);
    string path = el.Attribute("thumb");
    url.SetFileName(path.substr(1));
    CStdString theURL;
    url.GetURL(song.strThumb);
    
    // Replace the item.
    CFileItemPtr newItem(new CFileItem(song));
    newItem->m_strPath = pItem->m_strPath;
    pItem = newItem;
    
    // Path to the track.
    CURL url2(pItem->m_strPath);
    url2.SetProtocol("http");
    url2.SetPort(32400);
    url2.GetURL(pItem->m_strPath);
  }
  
  virtual void ComputeLabels(const string& strPath, string& strFileLabel, string& strDirLabel)
  {
    strDirLabel = "%B";
    
    if (strPath.find("/Artists/") != -1 || strPath.find("/Genre/") != -1 || strPath.find("/Albums/") != -1 || 
        strPath.find("/Recently%20Played/By%20Album/") != -1 || strPath.find("Recently%20Played/By%20Artist") != -1 ||
        strPath.find("/Recently%20Added/") != -1)
      strFileLabel = "%N - %T";
    else if (strPath.find("/Compilations/") != -1)
      strFileLabel = "%N - %A - %T";
    else if (strPath.find("/Tracks/") != -1)
      strFileLabel = "%T - %A";
    else if (strPath.find("/Podcasts/") != -1)
      strFileLabel = "%T";
    else
      strFileLabel = "%A - %T";
  }
  
 private:
};

class PlexMediaRoll : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, TiXmlElement& el)
  {
    pItem->SetLabel(el.Attribute("label"));
  }
};

class PlexMediaPhotoAlbum : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, TiXmlElement& el)
  {
    pItem->SetLabel(el.Attribute("label"));
  }
};

class PlexMediaPhotoKeyword : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, TiXmlElement& el)
  {
    pItem->SetLabel(el.Attribute("label"));
  }
};

class PlexMediaPhoto : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, TiXmlElement& el)
  {
    pItem->m_bIsFolder = false;
    pItem->SetLabel(el.Attribute("label"));
    
    // Thumbnail.
    CURL url(pItem->m_strPath);
    url.SetProtocol("http");
    url.SetPort(32400);
    string path = el.Attribute("thumb");
    url.SetFileName(path.substr(1));
    CStdString thumbPath;
    url.GetURL(thumbPath);

    pItem->SetThumbnailImage(thumbPath);
    
    // Path to the photo.
    CURL url2(pItem->m_strPath);
    url2.SetProtocol("http");
    url2.SetPort(32400);
    url2.GetURL(pItem->m_strPath);
  }
};

///////////////////////////////////////////////////////////////////////////////////////////////////
PlexMediaNode* PlexMediaNode::Create(const string& name)
{
  // FIXME: Move to using factory pattern.
  if (name == "Directory")
    return new PlexMediaDirectory();
  else if (name == "Artist")
    return new PlexMediaArtist();
  else if (name == "Album")
    return new PlexMediaAlbum();
  else if (name == "Playlist")
    return new PlexMediaPlaylist();
  else if (name == "Podcast")
    return new PlexMediaPodcast();
  else if (name == "Track")
    return new PlexMediaTrack();
  else if (name == "Genre")
    return new PlexMediaGenre();
  else if (name == "Roll")
    return new PlexMediaRoll();
  else if (name == "Photo")
    return new PlexMediaPhoto();
  else if (name == "PhotoAlbum")
    return new PlexMediaPhotoAlbum();
  else if (name == "PhotoKeyword")
    return new PlexMediaPhotoKeyword();
  else
    printf("ERROR: Unknown class [%s]\n", name.c_str());
  
  return 0;
}
  
///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexDirectory::Parse(const CURL& url, TiXmlElement* root, CFileItemList &items, string& strFileLabel, string& strDirLabel)
{
  PlexMediaNode* mediaNode = 0;
  
  for (TiXmlElement* element = root->FirstChildElement(); element; element=element->NextSiblingElement())
  {
    mediaNode = PlexMediaNode::Create(element->Value());
    items.Add(mediaNode->BuildFileItem(url, *element, strFileLabel, strDirLabel));
  }
  
  if (mediaNode != 0)
  {
    CStdString strURL;
    url.GetURL(strURL);
    mediaNode->ComputeLabels(strURL, strFileLabel, strDirLabel);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexDirectory::Process()
{
  CURL url(m_url);
  CStdString protocol = url.GetProtocol();
  url.SetProtocol("http");
  url.SetPort(32400);  

  CFileCurl http;
  //http.SetContentEncoding("deflate");
  if (http.Open(url, false) == false) 
  {
    CLog::Log(LOGERROR, "%s - Unable to get Plex Media Server directory", __FUNCTION__);
    m_bSuccess = false;
    m_downloadEvent.Set();
  }

  // Restore protocol.
  url.SetProtocol(protocol);

  CStdString content = http.GetContent();
  if (content.Equals("text/xml;charset=utf-8") == false)
  {
    CLog::Log(LOGERROR, "%s - Invalid content type %s", __FUNCTION__, content.c_str());
    m_bSuccess = false;
    m_downloadEvent.Set();
  }
  
  int size_read = 0;  
  int size_total = (int)http.GetLength();
  int data_size = 0;

  m_data.reserve(size_total);
  printf("Content-Length was %d bytes\n", http.GetLength());
  
  // Read response from server into string buffer.
  char buffer[4096];
  while (m_bStop == false && (size_read = http.Read(buffer, sizeof(buffer)-1)) > 0)
  {
    buffer[size_read] = 0;
    m_data += buffer;
    data_size += size_read;
  }
  
  // If we didn't get it all, we failed.
  if (m_data.size() != size_total)
    m_bSuccess = false;
  
  m_downloadEvent.Set();
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexDirectory::OnExit()
{
  m_bStop = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexDirectory::StopThread()
{
  m_bStop = true;
  CThread::StopThread();
}
