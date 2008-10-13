/*
 *  QuickTimeWrapper.h
 *  Plex
 *
 *  Created by James Clarke on 10/10/2008.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include <QuickTime/QuickTime.h>

class QuickTimeWrapper
{
public:
  QuickTimeWrapper();
  virtual ~QuickTimeWrapper();
  
  static void InitQuickTime();
  
  virtual bool OpenFile(const CStdString &fileName);
  virtual bool CloseFile();
  virtual void Pause();
  virtual bool IsPlaying() { return m_bIsPlaying; }
  virtual bool IsPaused() { return m_bIsPaused; }
  virtual bool IsPlayerDone();
  virtual void SetVolume(short sVolume);
  //virtual void SeekTime(__int64 iTime = 0);
  virtual __int64 GetTime();
  virtual int  GetTotalTime();
  virtual void SetRate(int iRate = 1);
  
private:
  Movie m_qtMovie;
  bool m_bIsPlaying;
  bool m_bIsPaused;
  
};