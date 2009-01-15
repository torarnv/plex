/*
 *  CoreAudioRenderer.cpp
 *  Plex
 *
 *  Created by Ryan Walklin on 11/19/08.
 *  Copyright 2008 Ryan Walklin. All rights reserved.
 *  Based on PortAudioDirectSound.cpp/ALSADirectSound.cpp from XBMC
 *
 */

#include "CoreAudioRenderer.h"
#include "AudioContext.h"
#include "XBAudioConfig.h"
#include "CoreAudioAUHAL.h"

CoreAudioRenderer::CoreAudioRenderer(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, const char* strAudioCodec, bool bIsMusic, bool bPassthrough)
{
	CLog::Log(LOGDEBUG,"CoreAudioRenderer::CoreAudioRenderer - opening device");
	
	if (iChannels == 0)
		iChannels = 2;
	bool bAudioOnAllSpeakers = false;
	g_audioContext.SetupSpeakerConfig(iChannels, bAudioOnAllSpeakers, bIsMusic);
	g_audioContext.SetActiveDevice(CAudioContext::DIRECTSOUND_DEVICE);
	
	m_bPause = false;
	m_bCanPause = false;
	m_bIsAllocated = false;
	m_bIsMusic = bIsMusic;
	
	m_dwPacketSize = (int)((float)iChannels*(uiBitsPerSample/8)* uiSamplesPerSec * CA_BUFFER_FACTOR / 5); // Pass 20% of the buffer at a time
	if (uiSamplesPerSec < 25000)
	{
	  // Use small buffer for low samplerates.
	  m_dwPacketSize /= 5; 
	}
	m_dwNumPackets = 16;
	
	// set the stream parameters
	m_uiChannels = iChannels;
	m_uiSamplesPerSec = uiSamplesPerSec;
	m_uiBitsPerSample = uiBitsPerSample;
	m_bPassthrough = bPassthrough;
	
	m_nCurrentVolume = g_stSettings.m_nVolumeLevel;
	if (!m_bPassthrough)
		m_amp.SetVolume(m_nCurrentVolume);
	
	/* Open the device */
	CStdString device = g_guiSettings.GetString("audiooutput.audiodevice");
	
	CLog::Log(LOGINFO, "Asked to open device: [%s]\n", device.c_str());
	
	audioUnit = new CoreAudioAUHAL(device,
								   strAudioCodec,
								   m_uiChannels,
								   m_uiSamplesPerSec,
								   m_uiBitsPerSample,
								   m_bPassthrough,
								   false,
								   m_dwPacketSize);
	
	m_bCanPause = false;
	m_bIsAllocated = true;
	
	
}

//***********************************************************************************************
CoreAudioRenderer::~CoreAudioRenderer()
{
	CLog::Log(LOGDEBUG,"CoreAudioAUHAL() dtor");
	
	Deinitialize();
	
	m_bIsAllocated = false;
	
 	CLog::Log(LOGDEBUG,"CoreAudioAUHAL::Deinitialize - set active");
	g_audioContext.SetActiveDevice(CAudioContext::DEFAULT_DEVICE);
	
	
}

bool CoreAudioRenderer::IsValid()
{
	return (audioUnit != NULL && m_bIsAllocated);
}

HRESULT CoreAudioRenderer::Deinitialize()
{
	audioUnit->Deinitialize();
	
	return S_OK;
}

//***********************************************************************************************
void CoreAudioRenderer::Flush()
{
#warning disabled
	//rb_clear(deviceParameters->outputBuffer);
	audioUnit->Flush();
}

//***********************************************************************************************
HRESULT CoreAudioRenderer::Pause()
{
	if (m_bPause)
		return S_OK;
	
	m_bPause = true;
	Flush();
	
	return S_OK;
}

//***********************************************************************************************
HRESULT CoreAudioRenderer::Resume()
{
	// If we are not pause, stream might not be prepared to start flush will do this for us
	if (!m_bPause)
		Flush();
	
	m_bPause = false;
	return S_OK;
}

//***********************************************************************************************
HRESULT CoreAudioRenderer::Stop()
{
	return S_OK;
}

//***********************************************************************************************
LONG CoreAudioRenderer::GetMinimumVolume() const
{
	return -60;
}

//***********************************************************************************************
LONG CoreAudioRenderer::GetMaximumVolume() const
{
	return 60;
}

//***********************************************************************************************
LONG CoreAudioRenderer::GetCurrentVolume() const
{
	return m_nCurrentVolume;
}

//***********************************************************************************************
void CoreAudioRenderer::Mute(bool bMute)
{
	if (!m_bIsAllocated) return;
	
	if (bMute)
		SetCurrentVolume(GetMinimumVolume());
	else
		SetCurrentVolume(m_nCurrentVolume);
	
}

//***********************************************************************************************
HRESULT CoreAudioRenderer::SetCurrentVolume(LONG nVolume)
{
	if (!m_bIsAllocated || m_bPassthrough) return -1;
	m_nCurrentVolume = nVolume;
	m_amp.SetVolume(nVolume);
	return S_OK;
}


//***********************************************************************************************
DWORD CoreAudioRenderer::GetSpace()
{
	return audioUnit->GetSpace();
}

//***********************************************************************************************
DWORD CoreAudioRenderer::AddPackets(unsigned char *data, DWORD len)
{
	// sanity
	if (!audioUnit && !IsValid())
	{
		CLog::Log(LOGERROR, "No audio output unit found!");
		return -1;
	}
	
	if (len == 0) return len;
	
	int samplesPassedIn, byteFactor;
	uint8_t *pcmPtr = data;
	
	byteFactor = m_uiChannels * m_uiBitsPerSample/8;
	samplesPassedIn = len / byteFactor;
	
	// Find out how much space we have available and clip to the amount we got passed in.
 	DWORD samplesToWrite = GetSpace();
	if (samplesToWrite == 0) return samplesToWrite;
	
	if (samplesToWrite > samplesPassedIn)
	{
		samplesToWrite = samplesPassedIn;
	}
	if (!m_bPassthrough)
		m_amp.DeAmplifyInt16((int16_t *)pcmPtr, samplesToWrite * m_uiChannels, g_guiSettings.GetBool("audiooutput.normalisevolume"), true);

	audioUnit->WriteStream(pcmPtr, samplesToWrite);
	
	return samplesToWrite * byteFactor;
	
}

//***********************************************************************************************
FLOAT CoreAudioRenderer::GetDelay()
{
	return audioUnit->GetHardwareLatency();
}

//***********************************************************************************************
DWORD CoreAudioRenderer::GetChunkLen()
{
	return m_dwPacketSize;
}
//***********************************************************************************************
int CoreAudioRenderer::SetPlaySpeed(int iSpeed)
{
	return 0;
}

void CoreAudioRenderer::RegisterAudioCallback(IAudioCallback *pCallback)
{
	
}

void CoreAudioRenderer::UnRegisterAudioCallback()
{
	
}

void CoreAudioRenderer::WaitCompletion()
{
	
}

void CoreAudioRenderer::SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers)
{
    return ;
}
