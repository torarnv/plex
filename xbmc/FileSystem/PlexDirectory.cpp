/*
 * PlexDirectory.cpp
 *
 *  Created on: Oct 4, 2008
 *      Author: Elan Feingold
 */
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/case_conv.hpp>

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
#include "GUIViewState.h"
#include "GUIDialogOK.h"

using namespace std;
using namespace XFILE;
using namespace DIRECTORY;

#define MASTER_PLEX_MEDIA_SERVER "http://localhost:32400"

///////////////////////////////////////////////////////////////////////////////////////////////////
CPlexDirectory::CPlexDirectory()
  : m_bStop(false)
  , m_bSuccess(true)
{
  m_timeout = 300;
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
  
  strRoot.Replace(" ", "%20");

  // Start the download thread running.
  printf("PlexDirectory::GetDirectory(%s)\n", strRoot.c_str());
  m_url = strRoot;
  CThread::Create(false, 0);

  // Now display progress, look for cancel.
  CGUIDialogProgress* dlgProgress = 0;
  
  int time = GetTickCount();
  
  while (m_downloadEvent.WaitMSec(100) == false)
  {
    // If enough time has passed, display the dialog.
    if (GetTickCount() - time > 1000)
    {
      dlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
      if (dlgProgress)
      {
        dlgProgress->ShowProgressBar(false);
        dlgProgress->SetHeading(40203);
        dlgProgress->SetLine(0, 40204);
        dlgProgress->SetLine(1, "");
        dlgProgress->SetLine(2, "");
        dlgProgress->StartModal();
      }
    }

    if (dlgProgress)
    {
      dlgProgress->Progress();
      if (dlgProgress->IsCanceled())
      {
        items.m_wasListingCancelled = true;
        m_http.Cancel();
        StopThread();
      }
    }
  }
  
  if (dlgProgress) 
    dlgProgress->Close();
  
  // Wait for the thread to exit.
  WaitForThreadExit(INFINITE);
  
  // See if we suceeded.
  if (m_bSuccess == false)
    return false;
  
  // Parse returned xml.
  TiXmlDocument doc;
  doc.Parse(m_data.c_str());

  TiXmlElement* root = doc.RootElement();
  if(root == 0)
  {
    CLog::Log(LOGERROR, "%s - Unable to parse xml\n%s", __FUNCTION__, m_data.c_str());
    
    return false;
  }
  
  // Get the fanart.
  const char* fanart = root->Attribute("art");
  string strFanart;
  if (fanart && strlen(fanart) > 0)
    strFanart = ProcessUrl(strPath, fanart, false);

  // Walk the parsed tree.
  string strFileLabel = "%N - %T"; 
  string strDirLabel = "%B";
  string strSecondDirLabel = "%Y";
  
  Parse(m_url, root, items, strFileLabel, strDirLabel, strSecondDirLabel);
  items.AddSortMethod(SORT_METHOD_NONE, 552, LABEL_MASKS(strFileLabel, "%D", strDirLabel, strSecondDirLabel));
  
  // Set the window titles
  const char* title1 = root->Attribute("title1");
  const char* title2 = root->Attribute("title2");

  if (title1 && strlen(title1) > 0)
    items.SetFirstTitle(title1);
  if (title2 && strlen(title2) > 0)
    items.SetSecondTitle(title2);

  // Set fanart on items.
  for (int i=0; i<items.Size(); i++)
  {
    CFileItemPtr pItem = items[i];
    
    if (strFanart.size() > 0)
      pItem->SetQuickFanart(strFanart);
      
    // Make sure sort label is lower case.
    string sortLabel = pItem->GetLabel();
    boost::to_lower(sortLabel);
    pItem->SetSortLabel(sortLabel);
  }
  
  // Set fanart on directory.
  if (strFanart.size() > 0)
    items.SetQuickFanart(strFanart);
    
  // Set the view mode.
  const char* viewmode = root->Attribute("viewmode");
  if (viewmode && strlen(viewmode) > 0)
  {
    CGUIViewState* viewState = CGUIViewState::GetViewState(0, items);
    viewState->SaveViewAsControl(atoi(viewmode));
  }
  
  // Set the content label.
  const char* content = root->Attribute("content");
  if (content && strlen(content) > 0)
  {
    items.SetContent(content);
  }
  
  // Check for dialog message attributes
  CStdString strHeader = "";
  CStdString strMessage = "";
  const char* header = root->Attribute("header");
  if (header && strlen(header) > 0)
  {
    strHeader = header;
    const char* message = root->Attribute("message");
    if (message && strlen(message) > 0) strMessage = message;
    CGUIDialogOK::ShowAndGetInput(strHeader, strMessage, "", "");
    
    // If the container has no child items, return to the previous directory
    if (items.Size() == 0)
      return false;
  }
  
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
     
     string src = el.Attribute("key");
     CStdString parentPath;
     url.GetURL(parentPath);
     
     // Compute the new path.
     pItem->m_strPath = CPlexDirectory::ProcessUrl(parentPath, src, true);
     
     // Let subclass finish.
     DoBuildFileItem(pItem, string(parentPath), el);
     
     try
     {
       // Thumb.
       const char* thumb = el.Attribute("thumb"); 
       if (thumb && strlen(thumb) > 0)
       {
         string strThumb = CPlexDirectory::ProcessUrl(parentPath, thumb, false);
         pItem->SetThumbnailImage(strThumb);
       }
       
       // Fanart.
       const char* fanart = el.Attribute("art");
       string strFanart;
       if (fanart && strlen(fanart) > 0)
       {
         strFanart = CPlexDirectory::ProcessUrl(parentPath, fanart, false);
         pItem->SetQuickFanart(strFanart);
       } 
     }
     catch (...)
     {
       printf("ERROR: Exception setting directory thumbnail.\n");
     }
     
     // Make sure we have the trailing slash.
     if (pItem->m_bIsFolder == true && pItem->m_strPath[pItem->m_strPath.size()-1] != '/')
       pItem->m_strPath += "/";
     
     return pItem;
   }
   
   virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el) = 0;
   virtual void ComputeLabels(const string& strPath, string& strFileLabel, string& strDirLabel, string& strSecondDirLabel)
   {
     strFileLabel = "%N - %T";
     strDirLabel = "%B";
     strSecondDirLabel = "%Y";
     
     if (strPath.find("/Albums") != -1)
       strDirLabel = "%B - %A";
     else if (strPath.find("/Recently%20Played/By%20Artist") != -1 || strPath.find("/Most%20Played/By%20Artist") != -1)
       strDirLabel = "%B";
     else if (strPath.find("/Decades/") != -1 || strPath.find("/Recently%20Added") != -1 || strPath.find("/Most%20Played") != -1 || strPath.find("/Recently%20Played/") != -1 || strPath.find("/Genre/") != -1)
       strDirLabel = "%A - %B";
   }
};

class PlexMediaDirectory : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    pItem->SetLabel(el.Attribute("name"));
    
    CVideoInfoTag tag;
    tag.m_strTitle = pItem->GetLabel();
    
    // Summary.
    const char* summary = el.Attribute("summary");
    if (summary)
    {
      pItem->SetProperty("description", summary);
      tag.m_strPlot = tag.m_strPlotOutline = summary;
    }
    
    CFileItemPtr newItem(new CFileItem(tag));
    newItem->m_bIsFolder = true;
    newItem->m_strPath = pItem->m_strPath;
    newItem->SetProperty("description", pItem->GetProperty("description"));
    pItem = newItem;
    
    // Check for search directories
    const char* search = el.Attribute("search");
    const char* prompt = el.Attribute("prompt");
    
    if (search && strlen(search) > 0 && prompt)
    {
      string strSearch = search;
      if (strSearch == "1")
      {
        pItem->m_bIsSearchDir = true;
        pItem->m_strSearchPrompt = prompt;
      }
    }
    
    // Check for popup menus
    const char* popup = el.Attribute("popup");
    if (popup && strlen(popup) > 0)
    {
      string strPopup = popup;
      if (strPopup == "1")
        pItem->m_bIsPopupMenuItem = true;
    }
  }
};

class PlexMediaObject : public PlexMediaNode
{  
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el) {}
};

class PlexMediaArtist : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    pItem->SetLabel(el.Attribute("artist"));
  }
};

class PlexMediaAlbum : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el) 
  {
    CAlbum album;
    
    album.strLabel = el.Attribute("label");
    album.idAlbum = boost::lexical_cast<int>(el.Attribute("key"));
    album.strAlbum = el.Attribute("album");
    album.strArtist = el.Attribute("artist");
    album.strGenre = el.Attribute("genre");
    album.iYear = boost::lexical_cast<int>(el.Attribute("year"));
        
    CFileItemPtr newItem(new CFileItem(pItem->m_strPath, album));
    pItem = newItem;
  }
};

class PlexMediaPodcast : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    CAlbum album;
    
    album.strLabel = el.Attribute("title");
    album.idAlbum = boost::lexical_cast<int>(el.Attribute("key"));
    album.strAlbum = el.Attribute("title");
    
    if (strlen(el.Attribute("year")) > 0)
      album.iYear = boost::lexical_cast<int>(el.Attribute("year"));
        
    CFileItemPtr newItem(new CFileItem(pItem->m_strPath, album));
    pItem = newItem;
  }
};

class PlexMediaPlaylist : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    pItem->SetLabel(el.Attribute("title"));
  }
};

class PlexMediaGenre : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    pItem->SetLabel(el.Attribute("genre"));
  }
};

class PlexMediaVideo : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    pItem->m_bIsFolder = false;
          
    CURL parentURL(parentPath);
    CVideoInfoTag videoInfo;
    videoInfo.m_strTitle = el.Attribute("title");
    videoInfo.m_strPlot = videoInfo.m_strPlotOutline = el.Attribute("summary");
    
    const char* year = el.Attribute("year");
    if (year)
        videoInfo.m_iYear = boost::lexical_cast<int>(year);
      
    string duration = el.Attribute("duration");
    if (duration.size() > 0)
    {
      int seconds = boost::lexical_cast<int>(duration)/1000;
      int hours = seconds/3600;
      int minutes = (seconds / 60) % 60;
      seconds = seconds % 60;

      CStdString std;
      if (hours > 0)
        std.Format("%d:%02d:%02d", hours, minutes, seconds);
      else
        std.Format(":%d:%02d", minutes, seconds);
      
      videoInfo.m_strRuntime = std;
    }
    
#if 0
    // NOT YET, messes up display in file mode.
    
    // TV show information.
    const char* season = el.Attribute("season");
    if (season)
      videoInfo.m_iSeason = boost::lexical_cast<int>(season);

    const char* episode = el.Attribute("episode");
    if (episode)
      videoInfo.m_iEpisode = boost::lexical_cast<int>(episode);
      
    const char* show = el.Attribute("show");
    if (show)
      videoInfo.m_strShowTitle = show;
#endif
        
    // Path to the track itself.
    CURL url2(pItem->m_strPath);
    if (url2.GetProtocol() == "plex" && url2.GetFileName().Find("video/:/") == -1)
    {
      url2.SetProtocol("http");
      url2.SetPort(32400);
      url2.GetURL(pItem->m_strPath);
    }
    
    videoInfo.m_strFile = pItem->m_strPath;
    videoInfo.m_strFileNameAndPath = pItem->m_strPath;
    
    CFileItemPtr newItem(new CFileItem(videoInfo));
    newItem->m_bIsFolder = false;
    newItem->m_strPath = pItem->m_strPath;
    pItem = newItem;
  }
  
  virtual void ComputeLabels(const string& strPath, string& strFileLabel, string& strDirLabel, string& strSecondDirLabel)
  {
    strDirLabel = "%B";
    strFileLabel = "%K";
    strSecondDirLabel = "%D";
  }
};

class PlexMediaTrack : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    pItem->m_bIsFolder = false;
    
    CSong song;
    song.strTitle = (el.Attribute("track"));
    song.strArtist = el.Attribute("artist");
    song.strAlbum = el.Attribute("album");
    song.strFileName = pItem->m_strPath;

    const char* totalTime = el.Attribute("totalTime");
    if (totalTime && strlen(totalTime) > 0)
      song.iDuration = boost::lexical_cast<int>(totalTime)/1000;
    
    const char* trackNumber = el.Attribute("index");
    if (trackNumber && strlen(trackNumber) > 0)
      song.iTrack = boost::lexical_cast<int>(trackNumber);
    
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
  
  virtual void ComputeLabels(const string& strPath, string& strFileLabel, string& strDirLabel, string& strSecondDirLabel)
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
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    pItem->SetLabel(el.Attribute("label"));
  }
};

class PlexMediaPhotoAlbum : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    pItem->SetLabel(el.Attribute("label"));
  }
};

class PlexMediaPhotoKeyword : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    pItem->SetLabel(el.Attribute("label"));
  }
};

class PlexMediaPhoto : public PlexMediaNode
{
  virtual void DoBuildFileItem(CFileItemPtr& pItem, const string& parentPath, TiXmlElement& el)
  {
    pItem->m_bIsFolder = false;
    
    // FIXME: Shouldn't have to ways to get this.
    if (el.Attribute("title"))
      pItem->SetLabel(el.Attribute("title"));
    else
      pItem->SetLabel(el.Attribute("label"));
    
    // Summary.
    const char* summary = el.Attribute("summary");
    if  (summary)
      pItem->SetProperty("description", summary);
    
    // Path to the photo.
    pItem->m_strPath = CPlexDirectory::ProcessUrl(parentPath, el.Attribute("key"), false);
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
  else if (name == "Video")
    return new PlexMediaVideo();
  else
    printf("ERROR: Unknown class [%s]\n", name.c_str());
  
  return 0;
}
  
///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexDirectory::Parse(const CURL& url, TiXmlElement* root, CFileItemList &items, string& strFileLabel, string& strDirLabel, string& strSecondDirLabel)
{
  PlexMediaNode* mediaNode = 0;
  
  for (TiXmlElement* element = root->FirstChildElement(); element; element=element->NextSiblingElement())
  {
    mediaNode = PlexMediaNode::Create(element->Value());
    items.Add(mediaNode->BuildFileItem(url, *element));
  }
  
  if (mediaNode != 0)
  {
    CStdString strURL;
    url.GetURL(strURL);
    mediaNode->ComputeLabels(strURL, strFileLabel, strDirLabel, strSecondDirLabel);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CPlexDirectory::Process()
{
  CURL url(m_url);
  CStdString protocol = url.GetProtocol();
  url.SetProtocol("http");
  url.SetPort(32400);  

  m_http.SetTimeout(m_timeout);
  if (m_http.Open(url, false) == false) 
  {
    CLog::Log(LOGERROR, "%s - Unable to get Plex Media Server directory", __FUNCTION__);
    m_bSuccess = false;
    m_downloadEvent.Set();
    return;
  }

  // Restore protocol.
  url.SetProtocol(protocol);

  CStdString content = m_http.GetContent();
  if (content.Equals("text/xml;charset=utf-8") == false && content.Equals("application/xml") == false)
  {
    CLog::Log(LOGERROR, "%s - Invalid content type %s", __FUNCTION__, content.c_str());
    m_bSuccess = false;
    m_downloadEvent.Set();
  }
  else
  {
    int size_read = 0;  
    int size_total = (int)m_http.GetLength();
    int data_size = 0;
  
    m_data.reserve(size_total);
    printf("Content-Length was %d bytes\n", size_total);
    
    // Read response from server into string buffer.
    char buffer[4096];
    while (m_bStop == false && (size_read = m_http.Read(buffer, sizeof(buffer)-1)) > 0)
    {
      buffer[size_read] = 0;
      m_data += buffer;
      data_size += size_read;
    }
    
    // If we didn't get it all, we failed.
    if (m_data.size() != size_total)
      m_bSuccess = false;
  }

  m_http.Close();
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

///////////////////////////////////////////////////////////////////////////////////////////////////
string CPlexDirectory::ProcessUrl(const string& parent, const string& url, bool isDirectory)
{
  CURL theURL(parent);
  
  // Files use plain HTTP.
  if (isDirectory == false)
  {
    theURL.SetProtocol("http");
    theURL.SetPort(32400);
  }

  if (url.find("://") != -1)
  {
    // It's got its own protocol, so leave it alone.
    return url;
  }
  else if (url.find("/") == 0)
  {
    // Absolute off parent.
    theURL.SetFileName(url.substr(1));
  }
  else
  {
    // Relative off parent.
    string path = theURL.GetFileName();
    if (path[path.size() -1] != '/')
      path += '/';
    
    theURL.SetFileName(path + url);
  }
  
  CStdString newURL;
  theURL.GetURL(newURL);
  return newURL;
}

