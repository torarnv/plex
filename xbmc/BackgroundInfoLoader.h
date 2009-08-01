#pragma once

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

#include "utils/Thread.h"
#include "IProgressCallback.h"
#include "utils/CriticalSection.h"
#include "FileItem.h"

#include <vector>
#include "boost/shared_ptr.hpp"
#include "boost/foreach.hpp"

class CFileItem; 
typedef boost::shared_ptr<CFileItem> CFileItemPtr;
class CFileItemList;
class CBackgroundRunnerGroup;

class IBackgroundLoaderObserver
{
public:
  virtual ~IBackgroundLoaderObserver() {}
  virtual void OnItemLoaded(CFileItem* pItem) = 0;
};

class CBackgroundInfoLoader
{
public:
  CBackgroundInfoLoader(int nThreads=-1, int pauseBetweenLoadsInMS=0);
  virtual ~CBackgroundInfoLoader();

  void Load(CFileItemList& items);
  bool IsLoading();
  void SetObserver(IBackgroundLoaderObserver* pObserver);
  void SetProgressCallback(IProgressCallback* pCallback);
  virtual bool LoadItem(CFileItem* pItem) { return false; };

  void StopThread(); // will actually stop all worker threads.
  void StopAsync();  // will ask loader to stop as soon as possible, but not block

  void SetNumOfWorkers(int nThreads); // -1 means auto compute num of required threads
  
  IProgressCallback* GetProgressCallback() { return m_pProgressCallback; }
  IBackgroundLoaderObserver* GetObserver() { return m_pObserver; }

  void OnLoaderFinished();
  virtual void OnLoaderStart() {};
  virtual void OnLoaderFinish() {};
  
protected:

  CFileItemList *m_pVecItems;
  std::vector<CFileItemPtr> m_vecItems; // FileItemList would delete the items and we only want to keep a reference.
  CCriticalSection m_lock;

  bool m_bRunning;
  volatile bool m_bStop;
  int  m_nRequestedThreads;
  int  m_pauseBetweenLoadsInMS;

  IBackgroundLoaderObserver* m_pObserver;
  IProgressCallback* m_pProgressCallback;

  std::vector<CThread *> m_workers;
  
  CBackgroundRunnerGroup* m_workerGroup;
};

typedef boost::shared_ptr<CFileItemList> CFileItemListPtr;
typedef boost::shared_ptr<CCriticalSection> CCriticalSectionPtr;

/////////////////////////////////////////////////////////////////////////////
class CBackgroundRunnerGroup;
class CBackgroundRunner : public CThread
{
 public:
  
  CBackgroundRunner(CBackgroundRunnerGroup& group)
    : m_group(group)
    , m_bStop(false)
  {
    SetName("Background Runner");
    Create(true);
  }
  
  virtual ~CBackgroundRunner() {} 
  
  void Stop()
  {
    m_bStop = true;
  }
  
  virtual void Process();
  
 private:
  
  CBackgroundRunnerGroup& m_group;
  volatile bool           m_bStop;
};

/////////////////////////////////////////////////////////////////////////////
class CBackgroundInfoLoader;
class CBackgroundRunnerGroup
{
 public:
  CBackgroundRunnerGroup(CBackgroundInfoLoader* loader, CFileItemList& items, int numThreads, int msBetweenLoads)
    : m_msBetweenLoads(msBetweenLoads)
    , m_nActiveThreads(numThreads)
    , m_loader(loader)
    , m_stopped(false)
  {
    if (items.Size() > 0)
    {
      // Copy the list.
      for (int n=0; n<items.Size(); n++)
        m_vecItems.push_back(items[n]);
      
      // Notify of the start.
      m_loader->OnLoaderStart();
      
      // Start the threads.
      for (int i=0; i<numThreads; i++)
      {
        CBackgroundRunner* runner = new CBackgroundRunner(*this);
        m_runners.push_back(runner);
      }
    }
  }
  
  ~CBackgroundRunnerGroup()
  {
    CSingleLock lock(m_lock);
  }
  
  void Stop()
  {
    CSingleLock lock(m_lock);
    m_stopped = true;
    
    BOOST_FOREACH(CBackgroundRunner* runner, m_runners)
      runner->Stop();
  }
  
  void Detach()
  {
    CSingleLock lock(m_lock);
    m_loader = 0;
  }
  
  CFileItemPtr PopItem()
  {
    // Pull the first item off the stack.
    CFileItemPtr pItem;
    
    CSingleLock lock(m_lock);
    std::vector<CFileItemPtr>::iterator iter = m_vecItems.begin();
    if (iter != m_vecItems.end())
    {
      pItem = *iter;
      m_vecItems.erase(iter);
    }

    return pItem;
  }
  
  bool LoadItem(CFileItemPtr& item)
  {
    EnterCriticalSection(m_lock);
    if (m_loader && m_stopped == false)
    {
      if (m_loader->GetProgressCallback() == 0 || m_loader->GetProgressCallback()->Abort() == false)
      {
        CBackgroundInfoLoader* loader = m_loader;

        // Leave the critical section to load the item.
        LeaveCriticalSection(m_lock);
        bool ret = loader->LoadItem(item.get());
        EnterCriticalSection(m_lock);
        
        if (ret)
        {
          // Notify the observer.
          NotifyObserver(item);
          
          // Pause if it was requested.
          if (m_msBetweenLoads > 0)
            ::usleep(m_msBetweenLoads*1000);
        }
      }
    }
    
    LeaveCriticalSection(m_lock);
    return false;
  }
  
  void NotifyObserver(CFileItemPtr& item)
  {
    CSingleLock lock(m_lock);
    if (m_stopped == false && m_loader->GetObserver())
      m_loader->GetObserver()->OnItemLoaded(item.get());
  }
  
  void WorkerDone()
  {
    EnterCriticalSection(m_lock);
    
    // If we're the last one on, turn the lights off.
    if (--m_nActiveThreads == 0)
    {
      if (m_loader)
        m_loader->OnLoaderFinished();
      
      LeaveCriticalSection(m_lock);
      delete this;
    }
    else
    {
      LeaveCriticalSection(m_lock);
    }
  }
  
  int GetNumActiveThreads() 
  { 
    return m_nActiveThreads; 
  }
  
  CBackgroundInfoLoader*           m_loader;
  int                              m_msBetweenLoads;
  int                              m_nActiveThreads;
  std::vector<CFileItemPtr>        m_vecItems;
  CCriticalSection                 m_lock;
  std::vector<CBackgroundRunner* > m_runners;
  bool                             m_stopped;
};
