#pragma once

/*
 * PlexDirectory.h
 *
 *  Created on: Oct 4, 2008
 *      Author: Elan Feingold
 */
#include "IDirectory.h"

class CURL;
class TiXmlElement;

namespace DIRECTORY
{
class CPlexDirectory : public IDirectory
{
 public:
  CPlexDirectory();
  virtual ~CPlexDirectory();
  
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  
 protected:
   
  bool Parse(const CURL& url, TiXmlElement* root, CFileItemList &items); 
};

}
