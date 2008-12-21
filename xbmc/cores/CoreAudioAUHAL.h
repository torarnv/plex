/*
 *  CoreAudioAUHAL.h
 *  Plex
 *
 *  Created by Ryan Walklin on 9/6/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

/*
 * XBoxMediaPlayer
 * Copyright (c) 2002 d7o3g4q and RUNTiME
 * Portions Copyright (c) by the authors of ffmpeg and xvid
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __COREAUDIO_AUHAL_H__
#define __COREAUDIO_AUHAL_H__

#include "pa_ringbuffer.h"
#include "CoreAudioPlexSupport.h"
#include "stdafx.h"
#include "XBAudioConfig.h"
#include "utils/CriticalSection.h"

#define CA_BUFFER_FACTOR 0.05

class CoreAudioAUHAL
	{
	public:
		CoreAudioAUHAL(const CStdString& strName, int channels, unsigned int sampleRate, int bitsPerSample, bool isDigital, bool useCoreAudio, bool isMusic, int packetSize);
		virtual HRESULT Deinitialize();
		virtual DWORD GetSpace();
		virtual float GetHardwareLatency();
		virtual AudioStreamBasicDescription* GetStreamDescription();
		virtual int WriteStream(uint8_t *sampleBuffer, uint32_t samplesToWrite);

		// Addpackets - make ringbuffer private

		
	private:
		virtual int OpenPCM(struct CoreAudioDeviceParameters *deviceParameters, const CStdString& strName, int channels, float sampleRate, int bitsPerSample, bool isDigital, bool useCoreAudio, int packetSize);
		static OSStatus RenderCallbackAnalog(struct CoreAudioDeviceParameters *deviceParameters,
											 int *ioActionFlags,
											 const AudioTimeStamp *inTimeStamp,
											 unsigned int inBusNummer,
											 unsigned int inNumberFrames,
											 AudioBufferList *ioData );
		virtual int OpenSPDIF(struct CoreAudioDeviceParameters *deviceParameters, const CStdString& strName, int channels, float sampleRate, int bitsPerSample, bool isDigital, bool useCoreAudio, int packetSize);
		virtual int AudioStreamChangeFormat(CoreAudioDeviceParameters *deviceParameters, AudioStreamID i_stream_id, AudioStreamBasicDescription change_format);
		static OSStatus RenderCallbackSPDIF(AudioDeviceID inDevice,
											const AudioTimeStamp * inNow,
											const void * inInputData,
											const AudioTimeStamp * inInputTime,
											AudioBufferList * outOutputData,
											const AudioTimeStamp * inOutputTime,
											void * threadGlobals );
		static OSStatus StreamListener( AudioStreamID inStream,
									   UInt32 inChannel,
									   AudioDevicePropertyID inPropertyID,
									   void * inClientData );
		
		static PlexAudioDevicesPtr GetDeviceArray(); // could make this public interface to current device
		
		PlexAudioDevicesPtr deviceArray;
		struct CoreAudioDeviceParameters* deviceParameters;
		
		bool m_bIsMusic;
		bool m_bIsInitialized;
		CCriticalSection    m_cs;
	};
		
#endif

