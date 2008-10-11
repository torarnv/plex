/*
 *  QTPlayer.mm
 *  Plex
 *
 *  Created by James Clarke on 10/10/2008.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "stdafx.h"
#include "QuickTimeWrapper.h"
#include "QTPlayer.h"
#include "FileItem.h"
#include "Settings.h"

using namespace XFILE;


QTPlayer::QTPlayer(IPlayerCallback &callback) : IPlayer(callback)
{
  m_qtFile = new QuickTimeWrapper();
  m_bStopPlaying = false;
}

QTPlayer::~QTPlayer()
{
  CloseFile();
  delete m_qtFile;
  m_qtFile = NULL;
}

void QTPlayer::InitQuickTime()
{
  QuickTimeWrapper::InitQuickTime();
}

bool QTPlayer::OpenFile(const CFileItem &file, const CPlayerOptions &options)
{
  bool bResult = m_qtFile->OpenFile(file.m_strPath);
  SetVolume(g_stSettings.m_nVolumeLevel);
  m_callback.OnPlayBackStarted();
  if (ThreadHandle() == NULL)
    Create();
  return bResult;
}

bool QTPlayer::CloseFile()
{
  m_bStopPlaying = true;
  m_bStop = true;
  StopThread();
  bool bResult = m_qtFile->CloseFile();
  return bResult;
}

bool QTPlayer::IsPlaying() const
{
  return m_qtFile->IsPlaying();
}

void QTPlayer::Pause()
{
  m_qtFile->Pause();
}

bool QTPlayer::IsPaused() const
{
  return m_qtFile->IsPaused();
}

void QTPlayer::SetVolume(long nVolume)
{
  short vol = (1.0f + (nVolume / 6000.0f)) * 256.0f;
  m_qtFile->SetVolume(vol);
}

/*
void QTPlayer::SeekTime(__int64 iTime)
{
  if (!CanSeek()) return;
  m_qtFile->SeekTime(iTime);
}
 */

__int64 QTPlayer::GetTime()
{
  return m_qtFile->GetTime() * 1000;
}

int QTPlayer::GetTotalTime()
{
  return m_qtFile->GetTotalTime();
}

void QTPlayer::ToFFRW(int iSpeed)
{
  m_qtFile->SetRate(iSpeed);
}

void QTPlayer::Process()
{
  CLog::Log(LOGDEBUG, "QTPlayer: Thread started");
    
  do
  {
    Sleep(100);
  }
  while (!m_qtFile->IsPlayerDone() && !m_bStopPlaying);//(!m_bStopPlaying && m_bIsPlaying && !m_bStop);
    
    CLog::Log(LOGINFO, "QTPlayer: End of playback reached");
    if (!m_bStopPlaying && !m_bStop)
      m_callback.OnPlayBackEnded();

  CLog::Log(LOGDEBUG, "QTPlayer: Thread end");
}
