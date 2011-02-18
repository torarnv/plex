#pragma once

/*
 * PlexDirectory.h
 *
 *  Created on: Oct 4, 2008
 *      Author: Elan Feingold
 */
#include <string>

#include "FileCurl.h"
#include "IDirectory.h"
#include "Thread.h"

class CURL;
class TiXmlElement;
using namespace std;
using namespace XFILE;

#define PLEX_METADATA_MOVIE   1
#define PLEX_METADATA_SHOW    2
#define PLEX_METADATA_EPISODE 4
#define PLEX_METADATA_TRAILER 5
#define PLEX_METADATA_PERSON  7
#define PLEX_METADATA_ARTIST  8
#define PLEX_METADATA_ALBUM   9
#define PLEX_METADATA_TRACK   10
#define PLEX_MEDATATA_PICTURE 11
#define PLEX_METADATA_CLIP    12
#define PLEX_METADATA_PERSON  13

namespace DIRECTORY
{
class CPlexDirectory : public IDirectory, 
                       public CThread
{
 public:
  CPlexDirectory(bool parseResults=true, bool displayDialog=true);
  virtual ~CPlexDirectory();
  
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual DIR_CACHE_TYPE GetCacheType(const CStdString &strPath) const { return m_dirCacheType; };
  static string ProcessUrl(const string& parent, const string& url, bool isDirectory);
  virtual void SetTimeout(int timeout) { m_timeout = timeout; }
  
  string GetData() { return m_data; } 
  
  static string ProcessMediaElement(const string& parentPath, const char* mediaURL, int maxAge, bool local);
  static string BuildImageURL(const string& parentURL, const string& imageURL, bool local);
  
 protected:
  
  virtual void Process();
  virtual void OnExit();
  virtual void StopThread();
  
  void Parse(const CURL& url, TiXmlElement* root, CFileItemList &items, string& strFileLabel, string& strSecondFileLabel, string& strDirLabel, string& strSecondDirLabel, bool isLocal);
  void ParseTags(TiXmlElement* element, const CFileItemPtr& item, const string& name);
  
  CEvent     m_downloadEvent;
  bool       m_bStop;
  
  CStdString m_url;
  CStdString m_data;
  bool       m_bSuccess;
  bool       m_bParseResults;
  int        m_timeout;
  CFileCurl  m_http;
  DIR_CACHE_TYPE m_dirCacheType;
};

}
