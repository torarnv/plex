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

CPlexDirectory::CPlexDirectory()
{
}

CPlexDirectory::~CPlexDirectory()
{
}

bool CPlexDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CStdString strRoot = strPath;
  if (CUtil::HasSlashAtEnd(strRoot) && strRoot != "plex://")
    strRoot.Delete(strRoot.size() - 1);
  
  // See if it's cached.
  if (g_directoryCache.GetDirectory(strRoot, items))
    return true;

#if 0
  // Show progress dialog.
  CGUIDialogProgress* dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (dlgProgress)
  {
    dlgProgress->ShowProgressBar(false);
    dlgProgress->SetHeading(260);
    dlgProgress->SetLine(0, 14003);
    dlgProgress->SetLine(1, "");
    dlgProgress->SetLine(2, "");
    dlgProgress->StartModal();
  }
#endif

  strRoot.Replace(" ", "%20");
  CURL url(strRoot);
  CStdString protocol = url.GetProtocol();
  url.SetProtocol("http");

  CFileCurl http;
  //http.SetContentEncoding("deflate");
  if (http.Open(url, false) == false) 
  {
    CLog::Log(LOGERROR, "%s - Unable to get Plex Media Server directory", __FUNCTION__);
    //if (dlgProgress) dlgProgress->Close();
    return false;
  }

  // Restore protocol.
  url.SetProtocol(protocol);

  CStdString content = http.GetContent();
  if (content.Equals("text/xml;charset=utf-8") == false)
  {
    CLog::Log(LOGERROR, "%s - Invalid content type %s", __FUNCTION__, content.c_str());
    //if (dlgProgress) dlgProgress->Close();
    return false;
  }
  
  int size_read = 0;  
  int size_total = (int)http.GetLength();
  int data_size = 0;

  CStdString data;
  data.reserve(size_total);
  printf("Content-Length was %d bytes\n", http.GetLength());
  
  // Read response from server into string buffer.
  char buffer[4096];
  while ((size_read = http.Read(buffer, sizeof(buffer)-1)) > 0)
  {
    buffer[size_read] = 0;
    data += buffer;
    data_size += size_read;

#if 0
    dlgProgress->Progress();
    if (dlgProgress->IsCanceled())
    {
      dlgProgress->Close();
      return false;
    }
#endif
  }

  // Parse returned xml.
  TiXmlDocument doc;
  doc.Parse(data.c_str());

  TiXmlElement* root = doc.RootElement();
  if(root == 0)
  {
    CLog::Log(LOGERROR, "%s - Unable to parse xml\n%s", __FUNCTION__, data.c_str());
    //if (dlgProgress) dlgProgress->Close();
    return false;
  }

  // Walk the parsed tree.
  Parse(url, root, items);

  CFileItemList vecCacheItems;
  g_directoryCache.ClearDirectory(strRoot);
  for( int i = 0; i <items.Size(); i++ )
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->IsParentFolder())
      vecCacheItems.Add(pItem);
  }
  
  g_directoryCache.SetDirectory(strRoot, vecCacheItems);

  //if (dlgProgress) dlgProgress->Close();
  
  items.AddSortMethod(SORT_METHOD_NONE, 552, LABEL_MASKS("%T", "%D"));
  
  return true;
}

class PlexMediaNode
{
 public:
   static PlexMediaNode* Create(const string& name);
   
   CFileItemPtr BuildFileItem(const CURL& url, TiXmlElement& el)
   {
     CFileItemPtr pItem(new CFileItem());
     pItem->m_bIsFolder = true;
     
     url.GetURL(pItem->m_strPath);
     if (pItem->m_strPath[pItem->m_strPath.size()-1] != '/')
       pItem->m_strPath += "/";
     
     pItem->m_strPath += el.Attribute("key");
     //printf("Key: %s\n", pItem->m_strPath.c_str());
     
     // Let subclass finish.
     DoBuildFileItem(pItem, el);
     
     return pItem;
   }
   
   virtual void DoBuildFileItem(CFileItemPtr& pItem, TiXmlElement& el) = 0;
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
  }
};

class PlexMediaAlbum : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, TiXmlElement& el) 
  {
    CAlbum album;
    
    if (pItem->m_strPath.Find("/Artists/") != -1)
      album.strLabel = el.Attribute("album");
    else
      album.strLabel = el.Attribute("artist") + string(" - ") + el.Attribute("album");
    
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
    
    CStdString theURL;
    url.GetURL(theURL);
    album.thumbURL = theURL;
    
    CFileItemPtr newItem(new CFileItem(pItem->m_strPath, album));
    pItem = newItem;
    
    pItem->SetLabel(album.strLabel);
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
    //album.iYear = boost::lexical_cast<int>(el.Attribute("year"));
    
    // Construct the thumbnail request.
    CURL url(pItem->m_strPath);
    url.SetProtocol("http");
    
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
    
    if (pItem->m_strPath.Find("/Artists/") != -1)
      song.strTitle = (el.Attribute("track"));
    else
      song.strTitle = (el.Attribute("artist") + string(" - ") + el.Attribute("track"));

    song.strArtist = el.Attribute("artist");
    song.strAlbum = el.Attribute("album");
    song.iDuration = boost::lexical_cast<int>(el.Attribute("totalTime"))/1000;
    song.iTrack = boost::lexical_cast<int>(el.Attribute("index"));
        
    // Thumbnail.
    CURL url(pItem->m_strPath);
    url.SetProtocol("http");
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
    url2.GetURL(pItem->m_strPath);
  }
};

PlexMediaNode* PlexMediaNode::Create(const string& name)
{
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
  else
    printf("ERROR: Unknown class [%s]\n", name.c_str());
  
  return 0;
}
  
bool CPlexDirectory::Parse(const CURL& url, TiXmlElement* root, CFileItemList &items)
{
  for (TiXmlElement* element = root->FirstChildElement(); element; element=element->NextSiblingElement())
  {
    PlexMediaNode* mediaNode = PlexMediaNode::Create(element->Value());
    items.Add(mediaNode->BuildFileItem(url, *element));
  }
  
  return true;
}
