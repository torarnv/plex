/*
 *  QuickTimeWrapper.cpp
 *  Plex
 *
 *  Created by James Clarke on 10/10/2008.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "stdafx.h"
#include "QuickTimeWrapper.h"
#include "log.h"

QuickTimeWrapper::QuickTimeWrapper()
{
  CLog::Log(LOGDEBUG, "QuickTimeWrapper::QuickTimeWrapper()");
  m_qtMovie = NULL;
  m_bIsPlaying = false;
  m_bIsPaused = false;
}

QuickTimeWrapper::~QuickTimeWrapper()
{
  CLog::Log(LOGDEBUG, "QuickTimeWrapper::~QuickTimeWrapper()");
}

void QuickTimeWrapper::InitQuickTime()
{
  EnterMovies();
}

bool QuickTimeWrapper::OpenFile(const CStdString &fileName)
{
  CLog::Log(LOGDEBUG, "QuickTimeWrapper::OpenFile(%s)", fileName.c_str());
  OSErr error;
  Handle dataRef;
  OSType dataRefType;
  CFStringRef filePath;
  CFURLRef fileLocation;
  
  filePath = CFStringCreateWithCString(kCFAllocatorDefault, fileName.c_str(), kCFStringEncodingUTF8);
  
  if (fileName.Find("http://") == 0)
    fileLocation = CFURLCreateWithString(kCFAllocatorDefault, filePath, NULL);
  else
    fileLocation = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, filePath, kCFURLPOSIXPathStyle, false);
  
  error = QTNewDataReferenceFromCFURL(fileLocation, 0, &dataRef, &dataRefType);
  if (error != 0)
  {
    CLog::Log(LOGERROR, "Error creating URL from %s\n", fileName.c_str());
    return false;
  }

  short fileID = movieInDataForkResID;
  error = NewMovieFromDataRef(&m_qtMovie, 0, &fileID, dataRef, dataRefType);
  if (error != 0) 
  {
    CLog::Log(LOGERROR, "Error creating movie from data ref\n");
    return false;
  }
  
  StartMovie(m_qtMovie);
  m_bIsPlaying = true;
  m_bIsPaused = false;
  return true;
}

bool QuickTimeWrapper::CloseFile()
{
  CLog::Log(LOGDEBUG, "QuickTimeWrapper::CloseFile()");
  if (!m_bIsPaused)
  {
    StopMovie(m_qtMovie);
    m_bIsPaused = false;;
  }
  m_bIsPlaying = false;
  if (m_qtMovie != NULL)
  {
    DisposeMovie(m_qtMovie);
    m_qtMovie = NULL;
  }
  return true;
}

void QuickTimeWrapper::Pause()
{
  CLog::Log(LOGDEBUG, "QuickTimeWrapper::Pause()");
  if (!m_bIsPaused)
    StopMovie(m_qtMovie);
  else
    StartMovie(m_qtMovie);
  m_bIsPaused = !m_bIsPaused;
}

bool QuickTimeWrapper::IsPlayerDone()
{
  if ((m_qtMovie == NULL) || (!m_bIsPlaying)) return true;
  return IsMovieDone(m_qtMovie);
}

void QuickTimeWrapper::SetVolume(short sVolume)
{
  SetMovieVolume(m_qtMovie, sVolume);
}

/*
void QuickTimeWrapper::SeekTime(__int64 iTime)
{
  SetMovieTimeValue(m_qtMovie, iTime * GetMovieTimeScale(m_qtMovie));
}
 */

__int64 QuickTimeWrapper::GetTime()
{
  if ((m_qtMovie == NULL) || (!m_bIsPlaying)) return 0;
  return GetMovieTime(m_qtMovie, NULL) / GetMovieTimeScale(m_qtMovie);
}

int QuickTimeWrapper::GetTotalTime()
{
  if ((m_qtMovie == NULL) || (!m_bIsPlaying)) return 0;
  return GetMovieDuration(m_qtMovie) / GetMovieTimeScale(m_qtMovie);
}

void QuickTimeWrapper::SetRate(int iRate)
{
  SetMovieRate(m_qtMovie, IntToFixed(iRate));
}
