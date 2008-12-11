#pragma once

/*
 * PlexDirectory.h
 *
 *  Created on: Oct 4, 2008
 *      Author: Elan Feingold
 */
#include <string>

#include "IDirectory.h"
#include "Thread.h"

class CURL;
class TiXmlElement;
using namespace std;

namespace DIRECTORY
{
class CPlexDirectory : public IDirectory, 
                       public CThread
{
 public:
  CPlexDirectory();
  virtual ~CPlexDirectory();
  
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  
  static string ProcessUrl(const string& parent, const string& url);
  
 protected:
   
  virtual void Process();
  virtual void OnExit();
  virtual void StopThread();
  
  void Parse(const CURL& url, TiXmlElement* root, CFileItemList &items, string& strFileLabel, string& strDirLabel, string& strSecondDirLabel);
  
  CEvent     m_downloadEvent;
  bool       m_bStop;
  
  CStdString m_url;
  CStdString m_data;
  bool       m_bSuccess;
};

}
