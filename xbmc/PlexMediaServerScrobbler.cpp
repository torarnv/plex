/*
 * PlexMediaServerScrobbler.cpp
 *
 *  Copyright (C) 2008 XBMC-Fork   
 *
 *  Created on: May 5, 2009
 *      Author: elan
 */

#include <boost/regex.hpp>
#include "stdafx.h"
#include "PlexMediaServerScrobbler.h"
#include "HTTP.h"
#include "URL.h"
#include "Util.h"

using namespace boost;

CPlexMediaServerScrobbler* CPlexMediaServerScrobbler::g_instance = 0;

///////////////////////////////////////////////////////////////////////////////
CPlexMediaServerScrobbler::CPlexMediaServerScrobbler()
  : m_acceptPlay(false)
{
  m_hWorkerEvent = CreateEvent(NULL, false, false, NULL);
  Create();
  SetEvent(m_hWorkerEvent);
}

///////////////////////////////////////////////////////////////////////////////
CPlexMediaServerScrobbler::~CPlexMediaServerScrobbler()
{
  CloseHandle(m_hWorkerEvent);
  StopThread();
  CLog::Log(LOGINFO,"Plex Media Server scrobbler destroyed");
}

///////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerScrobbler::Process()
{
  CLog::Log(LOGINFO,"Plex Media Server scrobbler running");
  
  while (!m_bStop)
  {
    // Wait for an event.
    WaitForSingleObject(m_hWorkerEvent, INFINITE);
    if (m_bStop)
      break;
    
    for (bool done=false; done == false; )
    {
      // Pull items off and scrobble.
      ::EnterCriticalSection(m_lock);
      if (m_queue.size() > 0)
      {
        // Process and action.
        ScrobbleActionPtr action = m_queue.front();
        m_queue.pop();
        ::LeaveCriticalSection(m_lock);
        
        CURL url(action->url);
        if (url.GetProtocol() == "http" && url.GetPort() == 32400)
        {
          url.SetPort(80);
          url.SetProtocol("plex");
        }
        
        CStdString processedURL;
        url.GetURL(processedURL);
        
        if (url.GetProtocol() == "plex")
        {
          printf("Scrobbling: %s\n", processedURL.c_str());
          
          // If we have something like this: plex://localhost/music/iTunes/Artists/OST/58514/58486.mp3
          // Then we want to hit plex://localhost/:/scrobble?key=Artists/OST/58514/58486.mp3&prefix=audio/iTunes
          //
          regex expression("(plex://[^/]+/)([^/]+/[^/]+)/(.*)"); 
          cmatch what; 
          if (regex_match(processedURL.c_str(), what, expression))
          {
            CStdString base = (string)what[1];
            CStdString prefix = "/" + (string)what[2];
            CStdString key = (string)what[3];
            
            CUtil::URLEncode(prefix);
            CUtil::URLEncode(key);
            
            // Build the final URL.
            CStdString url = base;
            url += ":/scrobble?key=";
            url += key;
            url += "&prefix=";
            url += prefix;
            url += "&verb=";
            url += action->getVerb();
            
            if (action->param.size() > 0)
            {
              url += "&param=";
              url += action->param;
            }
            
            CURL theURL(url);
                
            theURL.SetProtocol("http");
            theURL.SetPort(32400);
            theURL.GetURL(url);

            // Make the request.
            printf("Making request to %s\n", url.c_str());
            CHTTP http;
            http.Open(url, "GET", 0);
          }
        }        
      }
      else
      {
        // Nothing to process.
        ::LeaveCriticalSection(m_lock);
        done = true;
      }
    }
  }
  
  CLog::Log(LOGINFO,"Plex Media Server scrobbler thread terminated");
}
