#pragma once
/*
 *  PlexSourceScanner.h
 *  Plex
 *
 *  Created by Elan Feingold & James Clarke on 13/11/2009.
 *  Copyright 2009 Plex Development Team. All rights reserved.
 *
 */

#include <map>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

#include "CriticalSection.h"
#include "FileItem.h"
#include "SingleLock.h"
#include "Settings.h"
#include "Thread.h"

class HostSources
{
 public:

  VECSOURCES    videoSources;
  VECSOURCES    musicSources;
  VECSOURCES    pictureSources;
  CFileItemList librarySections;
};

typedef boost::shared_ptr<HostSources> HostSourcesPtr;
typedef pair<string, HostSourcesPtr> StringSourcesPair;

////////////////////////////////////////////
class CPlexSourceScanner : public CThread
{
public:
  
  virtual void Process();
  
  static void ScanHost(const string& host, const string& hostLabel);
  static void RemoveHost(const string& host);
  
  static void MergeSourcesForWindow(int windowId);
  
  static map<string, HostSourcesPtr>& GetMap() { return g_hostSourcesMap; }
  
protected:
  
  static void MergeSource(VECSOURCES& sources, VECSOURCES& remoteSources);
  static void CheckForRemovedSources(VECSOURCES& sources, int windowId);
  
  CPlexSourceScanner(const string& host, const string& hostLabel)
  : m_host(host)
  , m_hostLabel(hostLabel)
  {
    Create(true);
  }
  
private:
  
  string m_host;
  string m_hostLabel;
  
  static map<string, HostSourcesPtr> g_hostSourcesMap;
  static CCriticalSection g_lock;
};
