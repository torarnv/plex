#pragma once

/*
 * PlexDirectory.h
 *
 *  Created on: Oct 4, 2008
 *      Author: Elan Feingold
 */
#include "IDirectory.h"
#include "Thread.h"

class CURL;
class TiXmlElement;

namespace DIRECTORY
{
class CPlexDirectory : public IDirectory, 
                       public CThread
{
 public:
  CPlexDirectory();
  virtual ~CPlexDirectory();
  
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  
 protected:
   
  virtual void Process();
  virtual void OnExit();
  virtual void StopThread();
   
  bool Parse(const CURL& url, TiXmlElement* root, CFileItemList &items);
  
  CEvent     m_downloadEvent;
  bool       m_bStop;
  
  CStdString m_url;
  CStdString m_data;
  bool       m_bSuccess;
};

}
