/*
 *  QTPlayer.h
 *  Plex
 *
 *  Created by James Clarke on 10/10/2008.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "IPlayer.h"
#include "Thread.h"


class CFileItem;
class QuickTimeWrapper;

class QTPlayer : public IPlayer, public CThread
{
public:
  QTPlayer(IPlayerCallback &callback);
  virtual ~QTPlayer();
  
  static void InitQuickTime();
  
  virtual void RegisterAudioCallback(IAudioCallback *pCallback) {}
  virtual void UnRegisterAudioCallback() {}
  virtual bool OpenFile(const CFileItem &file, const CPlayerOptions &options);
  virtual bool QueueNextFile(const CFileItem &file) { return false; }
  virtual void OnNothingToQueueNotify() {}
  virtual bool CloseFile();
  virtual bool IsPlaying() const;
  virtual void Pause();
  virtual bool IsPaused() const;
  virtual bool HasVideo() const { return false; }
  virtual bool HasAudio() const { return true; }
  virtual void ToggleFrameDrop() {}
  virtual bool CanSeek() { return true; }
  virtual void Seek(bool bPlus = true, bool bLargeStep = false) {}
  virtual void SeekPercentage(float fPercent = 0.0f) {}
  virtual float GetPercentage() { return 0.0f;}
  virtual void SetVolume(long nVolume);
  virtual void SetDynamicRangeCompression(long drc) {}
  virtual void GetAudioInfo(CStdString &strAudioInfo) {}
  virtual void GetVideoInfo(CStdString &strVideoInfo) {}
  virtual void GetGeneralInfo(CStdString &strVideoInfo) {}
  virtual void Update(bool bPauseDrawing = false) {}
  virtual void GetVideoRect(RECT &srcRect, RECT &destRect) {}
  virtual void GetVideoAspectRatio(float &fAR) {}
//  virtual void SeekTime(__int64 iTime = 0);
  virtual __int64 GetTime();
  virtual int  GetTotalTime();
  virtual void ToFFRW(int iSpeed = 1);
  
protected:
  virtual void Process();
  
private:
  QuickTimeWrapper *m_qtFile;
  CFileItem*        m_currentFile;
  bool m_bStopPlaying;
  
};