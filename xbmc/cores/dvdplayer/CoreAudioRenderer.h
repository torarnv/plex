/*
 *  CoreAudioRenderer.h
 *  Plex
 *
 *  Created by Ryan Walklin on 11/19/08.
 *  Copyright 2008 Ryan Walklin. All rights reserved.
 *  Based on PortAudioDirectSound.cpp/ALSADirectSound.cpp from XBMC
 *
 */
#include "stdafx.h"
#include "../mplayer/IDirectSoundRenderer.h"
#include "../../utils/PCMAmplifier.h"


extern "C" {
#include "ac3encoder.h"
};
#include "CoreAudioAUHAL.h"

class CoreAudioRenderer : public IDirectSoundRenderer
	{
	public:
		virtual void UnRegisterAudioCallback();
		virtual void RegisterAudioCallback(IAudioCallback* pCallback);
		virtual DWORD GetChunkLen();
		virtual FLOAT GetDelay();
		CoreAudioRenderer(IAudioCallback* pCallback, int iChannels, unsigned int uiSamplesPerSec, unsigned int uiBitsPerSample, bool bResample, const char* strAudioCodec = "", bool bIsMusic=false, bool bPassthrough = false);
		virtual ~CoreAudioRenderer();
		
		virtual DWORD AddPackets(unsigned char* data, DWORD len);
		virtual DWORD GetSpace();
		virtual HRESULT Deinitialize();
		virtual HRESULT Pause();
		virtual HRESULT Stop();
		virtual HRESULT Resume();
		
		virtual LONG GetMinimumVolume() const;
		virtual LONG GetMaximumVolume() const;
		virtual LONG GetCurrentVolume() const;
		virtual void Mute(bool bMute);
		virtual HRESULT SetCurrentVolume(LONG nVolume);
		virtual int SetPlaySpeed(int iSpeed);
		virtual void WaitCompletion();
		virtual void SwitchChannels(int iAudioStream, bool bAudioOnAllSpeakers);
		virtual void Flush();
		
		bool IsValid();
		
		CPCMAmplifier *Amplifier() { return &m_amp; }
		
	private:
				
		CPCMAmplifier 	m_amp;
		CoreAudioAUHAL *audioUnit;
		
		LONG m_nCurrentVolume;
		DWORD m_dwPacketSize;
		DWORD m_dwNumPackets;
		bool m_bPause;
		bool m_bIsAllocated;
		bool m_bCanPause;
		bool m_bIsMusic;
		
		unsigned int m_uiSamplesPerSec;
		unsigned int m_uiBitsPerSample;
		unsigned int m_uiChannels;
		
		bool m_bPassthrough;
		
		bool m_bEncodeAC3;
		AC3Encoder m_ac3encoder;
		unsigned char* ac3_framebuffer;
	};

