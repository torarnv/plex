#pragma once

#include <string>
#include <queue>
#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/lexical_cast.hpp>

#include "FileItem.h"
#include "Thread.h"
#include "PlexDirectory.h"
#include "StackDirectory.h"

using namespace std;
using namespace boost;
using namespace DIRECTORY;
using boost::lexical_cast;

class PlexMediaServerQueue : public CThread
{
 public:
  
  PlexMediaServerQueue();
  virtual ~PlexMediaServerQueue() { StopThread(); m_mutex.lock(); m_mutex.unlock(); }
  virtual void Process();
  virtual void StopThread();
  
  /// Events, send to media server who owns the item.
  void onPlayingStarted(const string& identifier, const string& rootURL, const string& key, bool fullScreen);
  void onPlayingPaused(const string& identifier, const string& rootURL, const string& key, bool isPaused);
  void onPlayingStopped(const string& identifier, const string& rootURL, const string& key, int ms);
  
  /// View mode changed.
  void onViewModeChanged(const string& identifier, const string& rootURL, const string& viewGroup, int viewMode, int sortMode, int sortAsc)
  {
    if (identifier.size() > 0 && viewGroup.size() > 0)
    {
      string url = "/:/viewChange";
      url = CPlexDirectory::ProcessUrl(rootURL, url, false);
      url += "?identifier=" + identifier + 
             "&viewGroup=" + viewGroup + 
             "&viewMode=" + lexical_cast<string>(viewMode) + 
             "&sortMode=" + lexical_cast<string>(sortMode) +
             "&sortAsc=" + lexical_cast<string>(sortAsc);
    
      enqueue(url);
    }
  }
  
  /// Stream selection.
  void onStreamSelected(const CFileItemPtr& item, int partID, int subtitleStreamID, int audioStreamID)
  { 
    if (partID > 0)
    {
      string url = "/library/parts/" + boost::lexical_cast<string>(partID);
      url = buildUrl(item, url);
      
      if (subtitleStreamID != -1)
      {
        url += "?subtitleStreamID=";
        if (subtitleStreamID != 0)
          url += boost::lexical_cast<string>(subtitleStreamID);
      }
      else if (audioStreamID != -1)
      {
        url += "?audioStreamID=";
        if (audioStreamID != 0)
          url += boost::lexical_cast<string>(audioStreamID);
      }
      
      enqueue(url, "PUT");
    }
  }
  
  /// Play progress.
  void onPlayingProgress(const CFileItemPtr& item, int ms)
  { enqueue("progress", item, "&time=" + lexical_cast<string>(ms)); }
  
  /// Clear playing progress.
  void onClearPlayingProgress(const CFileItemPtr& item)
  { enqueue("progress", item, "&time=-1"); }
  
  /// Notify of a viewed item (a scrobble).
  void onViewed(const CFileItemPtr& item, bool force=false)
  {
    if (m_allowScrobble || force)
      enqueue("scrobble", item);
    
    m_allowScrobble = false;
  }

  /// Notify of an un-view.
  void onUnviewed(const CFileItemPtr& item)
  { enqueue("unscrobble", item); }

  /// Notify of a rate.
  void onRate(const CFileItemPtr& item, float rating)
  { enqueue("rate", item, "&rating=" + lexical_cast<string>(rating)); }
  
  /// These go to local media server only.
  void onIdleStart();
  void onIdleStop();
  void onQuit();
  
  /// Return the singleton.
  static PlexMediaServerQueue& Get() 
  { return g_plexMediaServerQueue; }

  /// Mark to allow the next scrobble that occurs.
  void allowScrobble()
  { m_allowScrobble = true; }
  
 protected:

  void enqueue(const string& url, const string& verb="GET")
  {
    m_mutex.lock();
    m_queue.push(pair<string, string>(verb, url));
    m_mutex.unlock();
    m_condition.notify_one();
  }
  
  void enqueue(const string& verb, const CFileItemPtr& item, const string& options="")
  {
    if (item->HasProperty("ratingKey"))
    {
      // Build the URL.
      string url = "/:/" + verb;
      url = buildUrl(item, url);
      url += "?key=" + item->GetProperty("ratingKey");
      url += "&identifier=" + item->GetProperty("pluginIdentifier");
      url += options;
      
      // Queue it up!
      enqueue(url);
    }
  }
  
  string buildUrl(const CFileItemPtr& item, const string& url)
  {
    CStdString path = item->m_strPath;
    if (item->IsStack())
    {
      CStackDirectory stack;
      path = stack.GetFirstStackedFile(path);
    }
    
    // Build the URL.
    return CPlexDirectory::ProcessUrl(path, url, false);
  }
  
 private:
  
  queue<pair<string, string> > m_queue;
  condition     m_condition;
  mutex         m_mutex;
  bool          m_allowScrobble;
  
  static PlexMediaServerQueue g_plexMediaServerQueue;
};
