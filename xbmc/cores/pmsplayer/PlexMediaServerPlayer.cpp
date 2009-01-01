/*
 *      Copyright (C) 2008-2009 Plex
 *      http://www.plexapp.com
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
 
#include "stdafx.h"
#include "PlexMediaServerPlayer.h"
#include "FileItem.h"
#include "GUIFontManager.h"
#include "GUITextLayout.h"
#include "Application.h"
#include "Settings.h"
#include "VideoRenderers/RenderManager.h"
#include "VideoRenderers/RenderFlags.h"
#include "URL.h"
#include "Util.h"
#include "VideoInfoTag.h"
#include "utils/SystemInfo.h"

#include <vector>
#include <set>

CPlexMediaServerPlayer::CPlexMediaServerPlayer(IPlayerCallback& callback)
    : IPlayer(callback),
      CThread()
{
  m_paused = false;
  m_playing = false;
  m_clock = 0;
  m_lastTime = 0;
  m_speed = 1;
  m_height = 0;
  m_width = 0;
  m_cropTop = 0;
  m_cropBottom = 0;
  m_cropLeft = 0;
  m_cropRight = 0;
  m_totalTime = 0;
  m_pDlgCache = NULL;
}

CPlexMediaServerPlayer::~CPlexMediaServerPlayer()
{
  CloseFile();
}

bool CPlexMediaServerPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  printf("Opening [%s]\n", file.m_strPath.c_str());
  
  if (m_pDlgCache)
    m_pDlgCache->Close();

  m_pDlgCache = new CDlgCache(0, g_localizeStrings.Get(10214), file.GetLabel());
  
  CStdString strActualPath = file.m_strPath;
  
  //m_dll.FlashSetCrop(m_handle, m_cropTop, m_cropBottom, m_cropLeft, m_cropRight);
  //m_dll.FlashSetCallback(m_handle, this);
  //m_dll.FlashOpen(m_handle, params.size(), (char **)argn, (char **)argv))
  
  if (file.HasVideoInfoTag())
  {
    std::vector<CStdString> tokens;
    CUtil::Tokenize(file.GetVideoInfoTag()->m_strRuntime, tokens, ":");
    int nHours = 0;
    int nMinutes = 0;
    int nSeconds = 0;
    
    if (tokens.size() == 2)
    {
      nMinutes = atoi(tokens[0].c_str());
      nSeconds = atoi(tokens[1].c_str());
    }
    else if (tokens.size() == 3)
    {
      nHours = atoi(tokens[0].c_str());
      nMinutes = atoi(tokens[1].c_str());
      nSeconds = atoi(tokens[2].c_str());
    }
    m_totalTime = (nHours * 3600) + (nMinutes * 60) + nSeconds;
  }
  
  Create();
  if (options.starttime > 0)
    SeekTime((__int64)(options.starttime * 1000));

  m_playing = true;
  return true;
}

bool CPlexMediaServerPlayer::CloseFile()
{
  int locks = ExitCriticalSection(g_graphicsContext);  
  StopThread();
  if (locks > 0)
    RestoreCriticalSection(g_graphicsContext, locks);
  
  //if (m_handle)
  //  m_dll.FlashClose(m_handle);
  //m_handle = NULL;
    
  if (m_pDlgCache)
    m_pDlgCache->Close();
  m_pDlgCache = NULL;
  
  return true;
}

bool CPlexMediaServerPlayer::IsPlaying() const
{
  return m_playing;
}

void CPlexMediaServerPlayer::Process()
{
  m_clock = 0;
  m_lastTime = timeGetTime();

  while (!m_bStop)
  {
    if (m_playing)
    {      
      if (!m_paused)
        m_clock += (timeGetTime() - m_lastTime)*m_speed;
      m_lastTime = timeGetTime();      
    }
    else
      Sleep(100);

    //m_dll.FlashUpdate(m_handle);
  }
  m_playing = false;
  if (m_bStop)
    m_callback.OnPlayBackEnded();
}

void CPlexMediaServerPlayer::Pause()
{
  //if (m_paused)
  //  m_dll.FlashPlay(m_handle);
  //else
  //  m_dll.FlashPause(m_handle);
  
  m_paused = !m_paused;
}

bool CPlexMediaServerPlayer::IsPaused() const
{
  return m_paused;
}

bool CPlexMediaServerPlayer::HasVideo() const
{
  return true;
}

bool CPlexMediaServerPlayer::HasAudio() const
{
  return true;
}

bool CPlexMediaServerPlayer::CanSeek()
{
  return true;
}

void CPlexMediaServerPlayer::Seek(bool bPlus, bool bLargeStep)
{
  if (bLargeStep)
  {
    //m_dll.FlashBigStep(m_handle, !bPlus);
  }
  else 
  {
    //m_dll.FlashSmallStep(m_handle, !bPlus);
  }
}

void CPlexMediaServerPlayer::GetAudioInfo(CStdString& strAudioInfo)
{
  strAudioInfo = "CPlexMediaServerPlayer";
}

void CPlexMediaServerPlayer::GetVideoInfo(CStdString& strVideoInfo)
{
  strVideoInfo.Format("Width: %d, height: %d", m_width, m_height);
}

void CPlexMediaServerPlayer::GetGeneralInfo(CStdString& strGeneralInfo)
{
  strGeneralInfo = "CPlexMediaServerPlayer";
}

void CPlexMediaServerPlayer::SetVolume(long nVolume) 
{
  int nPct = (int)fabs(((float)(nVolume - VOLUME_MINIMUM) / (float)(VOLUME_MAXIMUM - VOLUME_MINIMUM)) * 100.0);
  //m_dll.FlashSetVolume(m_handle, nPct);
}

void CPlexMediaServerPlayer::SeekPercentage(float iPercent)
{
  __int64 iTotalMsec = GetTotalTime() * 1000;
  __int64 iTime = (__int64)(iTotalMsec * iPercent / 100);
  SeekTime(iTime);
}

float CPlexMediaServerPlayer::GetPercentage()
{
  return m_pct ; 
}

// This is how much audio is delayed to video, we count the opposite in the dvdplayer.
void CPlexMediaServerPlayer::SetAVDelay(float fValue)
{
}

float CPlexMediaServerPlayer::GetAVDelay()
{
  return 0.0f;
}

void CPlexMediaServerPlayer::SeekTime(__int64 iTime)
{
//  m_clock = iTime;
  CStdString caption = g_localizeStrings.Get(52050); 
  CStdString description = g_localizeStrings.Get(52051); 
  g_application.m_guiDialogKaiToast.QueueNotification(caption, description);
  
  return;
}

// return the time in milliseconds
__int64 CPlexMediaServerPlayer::GetTime()
{
  return m_clock;
}

// return length in seconds.. this should be changed to return in milleseconds throughout xbmc
int CPlexMediaServerPlayer::GetTotalTime()
{
  return m_totalTime;
}

void CPlexMediaServerPlayer::ToFFRW(int iSpeed)
{
  m_speed = iSpeed;
}

void CPlexMediaServerPlayer::Render()
{
  if (!m_playing)
    return;
  
  //m_dll.FlashLockImage(m_handle);
  //int nHeight = m_dll.FlashGetHeight(m_handle)  - m_cropTop - m_cropBottom;
  //int nWidth = m_dll.FlashGetWidth(m_handle)  - m_cropLeft - m_cropRight;
  //g_renderManager.SetRGB32Image((const char*) m_dll.FlashGetImage(m_handle), nHeight, nWidth, nWidth*4);
  //m_dll.FlashUnlockImage(m_handle);
  g_application.NewFrame();
}

void CPlexMediaServerPlayer::OnPlaybackEnded()
{
  if (m_pDlgCache)
    m_pDlgCache->Close();
  m_pDlgCache = NULL; 

  ThreadMessage tMsg = {TMSG_MEDIA_STOP};
  g_application.getApplicationMessenger().SendMessage(tMsg, false);
}

void CPlexMediaServerPlayer::OnPlaybackStarted()
{
  CSingleLock lock(g_graphicsContext);

  m_playing = true;
  m_callback.OnPlayBackStarted();    
  
  try
  {
    g_renderManager.PreInit();
    //m_dll.FlashLockImage(m_handle);
    //int nHeight = m_dll.FlashGetHeight(m_handle) - m_cropTop - m_cropBottom;
    //int nWidth = m_dll.FlashGetWidth(m_handle) - m_cropLeft - m_cropRight;
    //g_renderManager.Configure(nWidth, nHeight, nWidth, nHeight, 30.0f, CONF_FLAGS_FULLSCREEN);
    //g_renderManager.SetRGB32Image((const char*) m_dll.FlashGetImage(m_handle), nHeight, nWidth, nWidth*4);
    //m_dll.FlashSetDestPitch(m_handle, nWidth * 4);
    //m_dll.FlashUnlockImage(m_handle);
  }
  catch(...)
  {
    CLog::Log(LOGERROR,"%s - Exception thrown on open", __FUNCTION__);
  }  
  
  if (m_pDlgCache)
    m_pDlgCache->Close();
  m_pDlgCache = NULL;
  
}

void CPlexMediaServerPlayer::OnNewFrame() 
{
  Render();
}

void CPlexMediaServerPlayer::OnPaused()
{
  m_paused = true;
}

void CPlexMediaServerPlayer::OnResumed() 
{
  m_paused = false;
}

void CPlexMediaServerPlayer::OnProgress(int nPct) 
{
  m_pct = nPct;
}
