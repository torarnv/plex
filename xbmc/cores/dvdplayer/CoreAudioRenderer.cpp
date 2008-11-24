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
	ac3_framebuffer = NULL;
	m_bIsMusic = bIsMusic;
	
	m_dwPacketSize = iChannels*(uiBitsPerSample/8)*256;
	m_dwNumPackets = 16;
	
	if (g_audioConfig.UseDigitalOutput() &&
		iChannels > 2 &&
		!bPassthrough)
	{
		// Enable AC3 passthrough for digital devices
		int mpeg_remapping = 0;
		if (strAudioCodec == "AAC" || strAudioCodec == "DTS") mpeg_remapping = 1; // DTS uses MPEG channel mapping
		ac3encoder_init(&m_ac3encoder, iChannels, uiSamplesPerSec, uiBitsPerSample, mpeg_remapping);
		m_bEncodeAC3 = true;
		m_bPassthrough = true;
		ac3_framebuffer = (unsigned char *)calloc(m_dwPacketSize, 1);
	}
	else
	{
		m_bEncodeAC3 = false;
	}
	
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
	
	if (g_audioConfig.UseDigitalOutput() &&
		iChannels > 2 &&
		!m_bPassthrough)
	{
		// Use AC3 encoder
		audioUnit = new CoreAudioAUHAL(device,
									   SPDIF_CHANNELS,
									   SPDIF_SAMPLERATE,
									   SPDIF_SAMPLESIZE,
									   true,
									   g_audioConfig.HasDigitalOutput(),
									   false,
									   SPDIF_SAMPLE_BYTES*512);
	}
	else
	{
		audioUnit = new CoreAudioAUHAL(device,
									   m_uiChannels,
									   m_uiSamplesPerSec,
									   m_uiBitsPerSample,
									   m_bPassthrough,
									   g_audioConfig.HasDigitalOutput(),
									   false,
									   m_dwPacketSize);
	}
	
	m_bCanPause = false;
	m_bIsAllocated = true;
	
	
}

//***********************************************************************************************
CoreAudioRenderer::~CoreAudioRenderer()
{
	CLog::Log(LOGDEBUG,"CoreAudioAUHAL() dtor");
	
	Deinitialize();
	
	if (m_bEncodeAC3)
	{
		ac3encoder_free(&m_ac3encoder);
	}
	if (ac3_framebuffer != NULL)
	{
		free(ac3_framebuffer);
		ac3_framebuffer = NULL;
	}
#warning free device array
	
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
	//CLog::Log(LOGDEBUG, "Flushing %i bytes from buffer", PaUtil_GetRingBufferReadAvailable(deviceParameters->outputBuffer));
#warning disabled
	//rb_clear(deviceParameters->outputBuffer);
	
	if (m_bEncodeAC3)
	{
		ac3encoder_flush(&m_ac3encoder);
	}
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
#warning Stop stream
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
	
	int samplesPassedIn, inputByteFactor, outputByteFactor;
	uint8_t *pcmPtr = data;
	
	if (m_bEncodeAC3) // use the raw PCM channel count to get the number of samples to play
	{
		inputByteFactor = ac3encoder_channelcount(&m_ac3encoder) * m_uiBitsPerSample/8;
		outputByteFactor = SPDIF_SAMPLE_BYTES;
	}
	else // the PCM input and stream output should match
	{
		inputByteFactor = m_uiChannels * m_uiBitsPerSample/8;
		outputByteFactor = inputByteFactor;
	}
	samplesPassedIn = len / inputByteFactor;
	
	// Find out how much space we have available and clip to the amount we got passed in.
 	DWORD samplesToWrite = GetSpace();
	if (samplesToWrite == 0) return samplesToWrite;
	
	if (samplesToWrite > samplesPassedIn)
	{
		samplesToWrite = samplesPassedIn;
	}
	
	if (m_bEncodeAC3)
	{
		int ac3_frame_count = 0;
		if ((ac3_frame_count = ac3encoder_write_samples(&m_ac3encoder, pcmPtr, samplesToWrite)) == 0)
		{
			CLog::Log(LOGERROR, "AC3 output buffer underrun");
			return 0;
		}
		else
		{
			int buffer_sample_readcount = -1;
			if ((buffer_sample_readcount = ac3encoder_get_encoded_samples(&m_ac3encoder, ac3_framebuffer, samplesToWrite)) != samplesToWrite)
			{
				CLog::Log(LOGERROR, "AC3 output buffer underrun");
			}
			else
			{
				audioUnit->WriteStream(ac3_framebuffer, samplesToWrite);
			}
		}
	}
	else
	{
		// Handle volume de-amplification.
		if (!m_bPassthrough)
			m_amp.DeAmplifyInt16((short *)pcmPtr, samplesToWrite * m_uiChannels);
		
		// Write data to the stream.
		audioUnit->WriteStream(pcmPtr, samplesToWrite);
	}
	return samplesToWrite * inputByteFactor;
	
}

//***********************************************************************************************
FLOAT CoreAudioRenderer::GetDelay()
{
	FLOAT delay = audioUnit->GetHardwareLatency();
	
	
	if (m_bEncodeAC3)
		delay += 0.032;
	
	return delay;
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
