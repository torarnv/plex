/*
 * PlexMediaServerScrobbler.h
 *
 *  Copyright (C) 2008 XBMC-Fork   
 *
 *  Created on: May 5, 2009
 *      Author: Elan Feingold
 */

#pragma once

#include <boost/shared_ptr.hpp>
#include <string>
#include <queue>

#include <boost/lexical_cast.hpp>
#include "CriticalSection.h"
#include "utils/Thread.h"

using namespace std;

class CPlexMediaServerScrobbler : public CThread
{
 public:
  CPlexMediaServerScrobbler();
  virtual ~CPlexMediaServerScrobbler();
  
  /// Get singleton instance.
  static CPlexMediaServerScrobbler* Get() 
  {
    if (g_instance == 0)
      g_instance = new CPlexMediaServerScrobbler();
    
    return g_instance; 
  }
  
  /// Shut down.
  static void Shutdown()
  {
    if (g_instance)
     {
       delete g_instance;
       g_instance = 0;
     }
  }
  
  /// Allow queuing a play.
  void AllowPlay()
  {
    CSingleLock lock(m_lock);
    m_acceptPlay = true;
  }
  
  /// Notify of a play.
  void AddPlay(const string& url)
  {
    CSingleLock lock(m_lock);
    if (m_acceptPlay)
    {
      printf("Queuing play of %s\n", url.c_str());
      m_queue.push(ScrobbleActionPtr(new ScrobbleAction(PLAY, url)));
      m_acceptPlay = false;
      SetEvent(m_hWorkerEvent);
    }
  }
  
  /// Notify of a rating (0-100).
  void SetRating(const string& url, int rating)
  {
    CSingleLock lock(m_lock);
    m_queue.push(ScrobbleActionPtr(new ScrobbleAction(RATE, url, boost::lexical_cast<string>(rating))));
    SetEvent(m_hWorkerEvent);
  }
  
 protected:
   
  enum ScrobbleActionVerb
  {
    PLAY,
    RATE
  };
  
  struct ScrobbleAction
  {
    ScrobbleAction(ScrobbleActionVerb verb, const string& url, const string& param="")
      : verb(verb)
      , url(url)
      , param(param) {}
    
    string getVerb()
    {
      switch (verb)
      {
      case PLAY: return "play";
      case RATE: return "rate";
      default: return "?";
      }
    }
    
    ScrobbleActionVerb verb;
    string url;
    string param;
  };
  typedef boost::shared_ptr<ScrobbleAction> ScrobbleActionPtr;
   
  void QueueAction();
  virtual void Process();

 private:
  static CPlexMediaServerScrobbler* g_instance;
   
  bool                     m_acceptPlay;
  HANDLE                   m_hWorkerEvent;
  CCriticalSection         m_lock;
  queue<ScrobbleActionPtr> m_queue;
};

