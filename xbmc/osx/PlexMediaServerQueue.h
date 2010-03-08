#pragma once

#include <string>
#include <queue>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>

#include "PlexDirectory.h"

using namespace std;
using namespace boost;
using namespace DIRECTORY;

class PlexMediaServerQueue : public CThread
{
 public:
  
  PlexMediaServerQueue();
  virtual void Process();
  virtual void StopThread();
  
  /// Events, send to media server who owns the item.
  void onPlayingStarted(const string& identifier, const string& rootURL, const string& key, bool fullScreen);
  void onPlayingPaused(const string& identifier, const string& rootURL, const string& key, bool isPaused);
  void onPlayingProgress(const string& identifier, const string& rootURL, const string& key, int ms);
  void onPlayingStopped(const string& identifier, const string& rootURL, const string& key, int ms);
  void onViewed(const string& identifier, const string& rootURL, const string& key);
  void onViewModeChanged(const string& identifier, const string& rootURL, const string& key, int viewMode);
  
  /// These go to local media server only.
  void onIdleStart();
  void onIdleStop();
  void onQuit();
  
  /// Commands.
  void onMarkedViewed(const string& identifier, const string& parentURL, const string& key);
  void onMarkedUnviewed(const string& identifier, const string& parentURL, const string& key);
  
  void onRate(const string& identifier, const string& parentURL, const string& key, float rating)
  {
    string url = "/:/rate?key=%s&identifier=%s&rating=%d";
    url = CPlexDirectory::ProcessUrl(parentURL, url, false);
  }
  
 protected:
  
  void enqueue(const string& url)
  {
    m_mutex.lock();
    m_queue.push(url);
    m_mutex.unlock();
    m_condition.notify_one();
  }
  
 private:
  
  queue<string> m_queue;
  condition     m_condition;
  mutex         m_mutex;
};
