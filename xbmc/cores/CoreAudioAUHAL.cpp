/*
 *  CoreAudioAUHAL.cpp
 *  Plex
 *
 *  Created by Ryan Walklin on 9/6/08.
 *  Copyright 2008 Ryan Walklin. All rights reserved.
 *  Based on PortAudioDirectSound.cpp/ALSADirectSound.cpp from XBMC
 *  CoreAudio HAL interface code from VLC (www.videolan.org)
 *
 */

#ifdef __APPLE__
/*
 * XBoxMediaPlayer
 * Copyright (c) 2002 d7o3g4q and RUNTiME
 * Portions Copyright (c) by the authors of ffmpeg and xvid
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Founda	tion; either version 2 of the License, or
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

#include <CoreAudio/AudioHardware.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <CoreServices/Components.k.h>

#include "stdafx.h"
#include "CoreAudioAUHAL.h"
#include "Settings.h"
#include "AudioDecoder.h"
#include "ac3encoder.h"

bool CoreAudioAUHAL::s_lastPlayWasSpdif = false;

struct CoreAudioDeviceParameters
{
  /* AUHAL specific */
	AudioDeviceID				device_id;
    Component                   au_component;   /* The Audiocomponent we use */
	AudioUnit                   au_unit;        /* The AudioUnit we use */
	PaUtilRingBuffer*			outputBuffer;
	void*						outputBufferData;
	int							hardwareFrameLatency;
	bool						b_digital;      /* Are we running in digital mode? */

	/* CoreAudio SPDIF mode specific */
  AudioDeviceIOProcID			sInputIOProcID;
  pid_t                       i_hog_pid;      /* The keep the pid of our hog status */
  AudioStreamID               i_stream_id;    /* The StreamID that has a cac3 streamformat */
  int                         i_stream_index; /* The index of i_stream_id in an AudioBufferList */
  AudioStreamBasicDescription stream_format;  /* The format we changed the stream to */
  AudioStreamBasicDescription sfmt_revert;    /* The original format of the stream */
  bool                  b_revert;       /* Wether we need to revert the stream format */
  bool                  b_changed_mixing;/* Wether we need to set the mixing mode back */
	
  // AC3 Encoder 	
  bool					m_bEncodeAC3;
  AC3Encoder			m_ac3encoder;
	uint8_t				rawSampleBuffer[AC3_SAMPLES_PER_FRAME * SPDIF_SAMPLESIZE/8 * 6]; // 6 channels = 12 bytes/sample

};

/*****************************************************************************
 * Open: open macosx audio output
 *****************************************************************************/
CoreAudioAUHAL::CoreAudioAUHAL(const CStdString& strName, const char *strCodec, int channels, unsigned int sampleRate, int bitsPerSample, bool passthrough, bool isMusic, int packetSize)
  : m_bIsInitialized(false)
{
  OSStatus                err = noErr;
  UInt32                  i_param_size = 0;
	
	/* Allocate structure */
	deviceParameters = (CoreAudioDeviceParameters*)calloc(sizeof(CoreAudioDeviceParameters), 1);
	if (!deviceParameters) return;
	
	if (g_audioConfig.HasDigitalOutput() && // disable encoder for legacy SPDIF devices for now
		channels > 2 &&
		!passthrough &&
		(sampleRate == SPDIF_SAMPLERATE44 || sampleRate == SPDIF_SAMPLERATE48))
	{
		// Enable AC3 passthrough for digital devices
		int mpeg_remapping = 0;
		if (strCodec == "AAC" /*|| strCodec == "DTS"*/) 
		  mpeg_remapping = 1; // DTS uses MPEG channel mapping
		
		if (ac3encoderInit(&deviceParameters->m_ac3encoder, channels, sampleRate, bitsPerSample, mpeg_remapping) == -1)
		{
			m_bIsInitialized = false;
			return;
		}
		else
		{
			deviceParameters->m_bEncodeAC3 = true;
		}
		
	}
	else
	{
		deviceParameters->m_bEncodeAC3 = false;
	}

	deviceParameters->b_digital = (passthrough || deviceParameters->m_bEncodeAC3) && g_audioConfig.HasDigitalOutput();
	deviceParameters->i_hog_pid = -1;
	deviceParameters->i_stream_index = -1;
	
	CLog::Log(LOGNOTICE, "Asked to create device:   [%s]", strName.c_str());
	CLog::Log(LOGNOTICE, "Device should be digital: [%d]\n", deviceParameters->b_digital);
	CLog::Log(LOGNOTICE, "CoreAudio S/PDIF mode:    [%d]\n", g_audioConfig.UseDigitalOutput());
	CLog::Log(LOGNOTICE, "Music mode:               [%d]\n", isMusic);
	CLog::Log(LOGNOTICE, "Channels:                 [%d]\n", channels);
	CLog::Log(LOGNOTICE, "Sample Rate:              [%d]\n", sampleRate);
	CLog::Log(LOGNOTICE, "BitsPerSample:            [%d]\n", bitsPerSample);
	CLog::Log(LOGNOTICE, "PacketSize:               [%d]\n", packetSize);
	
	m_dwPacketSize = packetSize;
	
	m_bIsMusic = isMusic;

	// Build a list of devices.
	deviceArray = PlexAudioDevices::FindAll();

	if (!deviceArray->getSelectedDevice())
	{
	  CLog::Log(LOGERROR, "There is no selected device, very strange, there are %d available ones.", deviceArray->getDevices().size());
	  return;
	}
	
	i_param_size = sizeof(deviceParameters->i_hog_pid);
	AudioObjectPropertyAddress propertyAOPA;
	propertyAOPA.mScope = kAudioDevicePropertyScopeOutput;
	propertyAOPA.mElement = kAudioObjectPropertyElementMaster;	
	propertyAOPA.mSelector = kAudioDevicePropertyHogMode;
	
	if (AudioObjectHasProperty(deviceParameters->i_hog_pid, &propertyAOPA))
	{
		err = AudioObjectGetPropertyData(deviceParameters->i_hog_pid, &propertyAOPA, 0, NULL, &i_param_size, &deviceParameters->i_hog_pid);
		if( err != noErr )
		{
			/* This is not a fatal error. Some drivers simply don't support this property */
			CLog::Log(LOGINFO, "Could not check whether device is hogged: %4.4s", (char *)&err );
			deviceParameters->i_hog_pid = -1;
		}
	}
		
	if( deviceParameters->i_hog_pid != -1 && deviceParameters->i_hog_pid != getpid() )
	{
		CLog::Log(LOGERROR, "Selected audio device is exclusively in use by another program.");
		return;
	}

	deviceParameters->device_id = deviceArray->getSelectedDevice()->getDeviceID();

	/* Check for Digital mode or Analog output mode */
	if (deviceParameters->b_digital)
	{
	  if (OpenSPDIF(deviceParameters, strName, channels, sampleRate, bitsPerSample, packetSize))
	  {
		m_dwPacketSize = AC3_SPDIF_FRAME_SIZE;
	    m_bIsInitialized = true;
	    return;
	  }
	}
	else if (g_audioConfig.ForcedDigital() && (deviceParameters->m_bEncodeAC3 || passthrough))
	{
		if (OpenPCM(deviceParameters, strName, SPDIF_CHANNELS, sampleRate, SPDIF_SAMPLESIZE, packetSize))
		{
			m_bIsInitialized = true;
			return;
		}
	}
	else
	{
	  if (OpenPCM(deviceParameters, strName, channels, sampleRate, bitsPerSample, packetSize))
	  {
	    m_bIsInitialized = true;
	    return;
	  }
	}

error:
  free(deviceParameters);
  m_bIsInitialized = false;
  return;
}

HRESULT CoreAudioAUHAL::Deinitialize()
{
  // Don't allow double deinitialization.
  if (m_bIsInitialized == false)
    return S_OK;
  
	CLog::Log(LOGDEBUG,"CoreAudioAUHAL::Deinitialize");
	
	OSStatus            err = noErr;
    UInt32              i_param_size = 0;
	AudioObjectPropertyAddress propertyAOPA;
	propertyAOPA.mScope = kAudioDevicePropertyScopeOutput;
	propertyAOPA.mElement = kAudioObjectPropertyElementMaster;
	
    if(deviceParameters->au_unit)
    {
        AudioOutputUnitStop( deviceParameters->au_unit );
        AudioUnitUninitialize( deviceParameters->au_unit);
		CloseComponent( deviceParameters->au_unit );

        deviceParameters->au_unit = NULL;
    }
	
	if (deviceParameters->m_bEncodeAC3)
	{
		ac3encoderFinalise(&deviceParameters->m_ac3encoder);
	}
	
	
	
    if( deviceParameters->b_digital )
    {
      printf("Stopping CoreAudio device.\n");
      
        /* Stop device */
        err = AudioDeviceStop( deviceParameters->device_id,
							  (AudioDeviceIOProc)RenderCallbackSPDIF );
        if( err != noErr )
        {
			CLog::Log(LOGERROR, "AudioDeviceStop failed: [%4.4s]", (char *)&err );
        }
		
        /* Remove IOProc callback */
		err = AudioDeviceDestroyIOProcID(deviceParameters->device_id, deviceParameters->sInputIOProcID);
		
        if( err != noErr )
        {
            CLog::Log(LOGERROR, "AudioDeviceRemoveIOProc failed: [%4.4s]", (char *)&err );
        }
		
        #warning fix listener
    //err = AudioHardwareRemovePropertyListener( kAudioHardwarePropertyDevices,
	//										  HardwareListener );
	}
	
    if( err != noErr )
    {
		CLog::Log(LOGERROR, "AudioHardwareRemovePropertyListener failed: [%4.4s]", (char *)&err );
    }
	
    if( deviceParameters->i_hog_pid == getpid() )
    {
        deviceParameters->i_hog_pid = -1;
        i_param_size = sizeof( deviceParameters->i_hog_pid );
		propertyAOPA.mSelector = kAudioDevicePropertyHogMode;
		err = AudioObjectSetPropertyData(deviceParameters->device_id, &propertyAOPA, 0, NULL, i_param_size, &deviceParameters->i_hog_pid);
        if( err != noErr ) CLog::Log(LOGERROR, "Could not release hogmode: [%4.4s]", (char *)&err );
    }

    // Revert the stream format *after* we've set all the parameters, as doing it before seems to 
    // result in a hang under some circumstances, an apparent deadlock in CoreAudio between handing
    // the stream format change and setting parameters. 
    //
    if (deviceParameters->b_digital && deviceParameters->b_revert)
    {
      printf("Reverting CoreAudio stream mode\n");
      AudioStreamChangeFormat(deviceParameters, deviceParameters->i_stream_id, deviceParameters->sfmt_revert);
    }
	
	free(deviceParameters->outputBuffer);
	free(deviceParameters->outputBufferData);
	
  m_bIsInitialized = false;
	return S_OK;
}

DWORD CoreAudioAUHAL::GetSpace()
{
	DWORD fakeCeiling, bufferDataSize = PaUtil_GetRingBufferReadAvailable(deviceParameters->outputBuffer);
	
	if (m_bIsMusic)
	{
		fakeCeiling = PACKET_SIZE / deviceParameters->stream_format.mChannelsPerFrame;
	}
	else
	{
		fakeCeiling = deviceParameters->stream_format.mSampleRate * CA_BUFFER_FACTOR;
	}
	
	if (bufferDataSize < fakeCeiling)
	{
		return fakeCeiling - bufferDataSize;
	}
	else
	{
		return 0;
	}
}

float CoreAudioAUHAL::GetHardwareLatency()
{
	float latency = /*CA_BUFFER_FACTOR +*/ ((float)deviceParameters->hardwareFrameLatency / deviceParameters->stream_format.mSampleRate);
	if (deviceParameters->b_digital)
		latency += 0.032;
	//if (m_bEncodeAC3)
	//	latency += 0.064;
	//latency += 0.225;
	return latency;
}

AudioStreamBasicDescription* CoreAudioAUHAL::GetStreamDescription()
{
	return &deviceParameters->stream_format;
};

int CoreAudioAUHAL::WriteStream(uint8_t *sampleBuffer, uint32_t samplesToWrite)
{
	if (sampleBuffer == NULL || samplesToWrite == 0)
	{
		return 0;
	}
	return PaUtil_WriteRingBuffer(deviceParameters->outputBuffer, sampleBuffer, samplesToWrite);
}

void CoreAudioAUHAL::Flush()
{
	//CSingleLock lock(m_cs); // acquire lock
	
	//PaUtil_FlushRingBuffer( deviceParameters->outputBuffer );
	if (deviceParameters->m_bEncodeAC3)
	{
	//	ac3encoder_flush(&m_ac3encoder);
	}
}

#pragma mark Analog (PCM)

/*****************************************************************************
 * Open: open and setup a HAL AudioUnit to do analog (multichannel) audio output
 *****************************************************************************/
int CoreAudioAUHAL::OpenPCM(struct CoreAudioDeviceParameters *deviceParameters, const CStdString& strName, int channels, float sampleRate, int bitsPerSample, int packetSize)
{
    OSStatus                    err = noErr;
    UInt32                      i_param_size = 0;
    AudioStreamBasicDescription DeviceFormat;
    AURenderCallbackStruct      input;

    // We're non-digital.
    s_lastPlayWasSpdif = false;
	
	/* Lets go find our Component */
	ComponentDescription        desc;
	desc.componentType = kAudioUnitType_Output;
	desc.componentSubType = kAudioUnitSubType_HALOutput;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;
	
	deviceParameters->au_component = FindNextComponent( NULL, &desc );
	if(deviceParameters->au_component == NULL)
	{
		CLog::Log(LOGERROR, "we cannot find our HAL component");
		return false;
	}
	
	err = OpenAComponent( deviceParameters->au_component, &deviceParameters->au_unit );
	if( err != noErr )
	{
		CLog::Log(LOGERROR, "we cannot open our HAL component");
		return false;
	}
    
    AudioDeviceID selectedDeviceID = deviceArray->getSelectedDevice()->getDeviceID();
    
    /* Set the device we will use for this output unit */
    err = AudioUnitSetProperty(deviceParameters->au_unit,
							   kAudioOutputUnitProperty_CurrentDevice,
							   kAudioUnitScope_Input,
							   0,
							   &selectedDeviceID,
							   sizeof(AudioDeviceID));

    if( err != noErr )
    {
		CLog::Log(LOGERROR, "we cannot select the audio device");
        return false;
    }

    /* Get the current format */
    i_param_size = sizeof(AudioStreamBasicDescription);

    err = AudioUnitGetProperty(deviceParameters->au_unit,
							   kAudioUnitProperty_StreamFormat,
							   kAudioUnitScope_Input,
							   0,
							   &deviceParameters->sfmt_revert,
							   &i_param_size );

    if( err != noErr ) return false;
    else CLog::Log(LOGINFO, STREAM_FORMAT_MSG("current format is: ", deviceParameters->sfmt_revert) );
    
    /* Set up the format to be used */
    DeviceFormat.mSampleRate = sampleRate;
    DeviceFormat.mFormatID = kAudioFormatLinearPCM;
    DeviceFormat.mFormatFlags = (bitsPerSample == 32 ? kLinearPCMFormatFlagIsFloat : kLinearPCMFormatFlagIsSignedInteger);
    DeviceFormat.mBitsPerChannel = bitsPerSample;
    DeviceFormat.mChannelsPerFrame = channels;

    /* Calculate framesizes and stuff */
    DeviceFormat.mFramesPerPacket = 1;
    DeviceFormat.mBytesPerFrame = DeviceFormat.mBitsPerChannel/8 * DeviceFormat.mChannelsPerFrame;
    DeviceFormat.mBytesPerPacket = DeviceFormat.mBytesPerFrame * DeviceFormat.mFramesPerPacket;

    /* Set the desired format */
    i_param_size = sizeof(AudioStreamBasicDescription);
    err = AudioUnitSetProperty(deviceParameters->au_unit,
									   kAudioUnitProperty_StreamFormat,
									   kAudioUnitScope_Input,
									   0,
									   &DeviceFormat,
									   i_param_size );
	if( err != noErr ) return false;

	CLog::Log(LOGINFO, STREAM_FORMAT_MSG( "we set the AU format: " , DeviceFormat ) );

    /* Retrieve actual format */
    err = AudioUnitGetProperty(deviceParameters->au_unit,
									   kAudioUnitProperty_StreamFormat,
									   kAudioUnitScope_Input,
									   0,
									   &deviceParameters->stream_format,
									   &i_param_size );
	if( err != noErr ) return false;

	CLog::Log(LOGINFO, STREAM_FORMAT_MSG( "the actual set AU format is " , DeviceFormat ) );

    /* set the IOproc callback */
	input.inputProc = (AURenderCallback) RenderCallbackAnalog;
	input.inputProcRefCon = deviceParameters;

    err = AudioUnitSetProperty(deviceParameters->au_unit,
									   kAudioUnitProperty_SetRenderCallback,
									   kAudioUnitScope_Global,
									   0, &input, sizeof(input));
	if( err != noErr ) return false;


    /* AU initialize */
    err = AudioUnitInitialize(deviceParameters->au_unit);
	if( err != noErr ) return false;

	// Get AU hardware buffer size

	uint32_t audioDeviceLatency, audioDeviceBufferFrameSize, audioDeviceSafetyOffset;
	deviceParameters->hardwareFrameLatency = 0;
	
	i_param_size = sizeof(uint32_t);

	err = AudioUnitGetProperty(deviceParameters->au_unit,
									   kAudioDevicePropertyLatency,
									   kAudioUnitScope_Global,
									   0,
									   &audioDeviceLatency,
							   &i_param_size );
	if( err != noErr ) return false;

	deviceParameters->hardwareFrameLatency += audioDeviceLatency;

	err = AudioUnitGetProperty(deviceParameters->au_unit,
									   kAudioDevicePropertyBufferFrameSize,
									   kAudioUnitScope_Global,
									   0,
									   &audioDeviceBufferFrameSize,
									   &i_param_size );
	if( err != noErr ) return false;

	deviceParameters->hardwareFrameLatency += audioDeviceBufferFrameSize;

	err = AudioUnitGetProperty(deviceParameters->au_unit,
									   kAudioDevicePropertySafetyOffset,
									   kAudioUnitScope_Global,
									   0,
									   &audioDeviceSafetyOffset,
									   &i_param_size );
	if( err != noErr ) return false;
	
	deviceParameters->hardwareFrameLatency += audioDeviceSafetyOffset;

	CLog::Log(LOGINFO, "Hardware latency: %i frames (%.2f msec @ %.0fHz)", deviceParameters->hardwareFrameLatency,
			  (float)deviceParameters->hardwareFrameLatency / deviceParameters->stream_format.mSampleRate * 1000,
			  deviceParameters->stream_format.mSampleRate);

	// initialise the CoreAudio sink buffer
	uint32_t framecount = 1;
	while(framecount <= deviceParameters->stream_format.mSampleRate) // ensure power of 2
	{
		framecount <<= 1;
	}
	deviceParameters->outputBuffer = (PaUtilRingBuffer *)malloc(sizeof(PaUtilRingBuffer));
	deviceParameters->outputBufferData = malloc(framecount * deviceParameters->stream_format.mBytesPerFrame);

	PaUtil_InitializeRingBuffer(deviceParameters->outputBuffer,
								deviceParameters->stream_format.mBytesPerFrame,
								framecount, deviceParameters->outputBufferData);


    /* Start the AU */
    err = AudioOutputUnitStart(deviceParameters->au_unit);
	if( err != noErr ) return false;
    return true;
}

/*****************************************************************************
 * RenderCallbackAnalog: This function is called everytime the AudioUnit wants
 * us to provide some more audio data.
 * Don't print anything during normal playback, calling blocking function from
 * this callback is not allowed.
 *****************************************************************************/
OSStatus CoreAudioAUHAL::RenderCallbackAnalog(struct CoreAudioDeviceParameters *deviceParameters,
									  int *ioActionFlags,
									  const AudioTimeStamp *inTimeStamp,
									  unsigned int inBusNumber,
									  unsigned int inNumberFrames,
									  AudioBufferList *ioData )
{
    // initial calc
	int framesToWrite = inNumberFrames;
	int framesAvailable = PaUtil_GetRingBufferReadAvailable(deviceParameters->outputBuffer);
	
	if (framesToWrite > framesAvailable)
	{
		framesToWrite = framesAvailable;
	}
	
	int currentPos = framesToWrite * deviceParameters->stream_format.mBytesPerFrame;
	int underrunLength = (inNumberFrames - framesToWrite) * deviceParameters->stream_format.mBytesPerFrame;

	// write as many frames as possible from buffer
	PaUtil_ReadRingBuffer(deviceParameters->outputBuffer, ioData->mBuffers[0].mData, framesToWrite);
	// write silence to any remainder
	if (underrunLength > 0)
	{
		memset((void *)((uint8_t *)(ioData->mBuffers[0].mData)+currentPos), 0, underrunLength);
	}

    return( noErr );
}

#pragma mark Digital (SPDIF)

/*****************************************************************************
 * Setup a encoded digital stream (SPDIF)
 *****************************************************************************/
int CoreAudioAUHAL::OpenSPDIF(struct CoreAudioDeviceParameters *deviceParameters, const CStdString& strName, int channels, float sampleRate, int bitsPerSample, int packetSize)

{
	OSStatus                err = noErr;
    UInt32                  propertySize = 0;
    AudioObjectPropertyAddress propertyAOPA;
	propertyAOPA.mScope = kAudioDevicePropertyScopeOutput;
	propertyAOPA.mElement = kAudioObjectPropertyElementMaster;	

    // We're digital.
    s_lastPlayWasSpdif = true;
    
    /* Start doing the SPDIF setup proces */
    deviceParameters->b_changed_mixing = false;

    /* Hog the device */
    propertySize = sizeof(deviceParameters->i_hog_pid);
    deviceParameters->i_hog_pid = getpid();

	propertyAOPA.mSelector = kAudioDevicePropertyHogMode;
	
	err = AudioObjectSetPropertyData(deviceParameters->device_id, &propertyAOPA, 0, NULL, propertySize, &deviceParameters->i_hog_pid);
	
	if( err != noErr )
	{
		CLog::Log(LOGWARNING, "Failed to set hogmode: [%4.4s]", (char *)&err);
	}
	
	// Retrieve all the output streams.
	propertyAOPA.mSelector = kAudioDevicePropertyStreams;
	
	if (!AudioObjectHasProperty(deviceParameters->device_id, &propertyAOPA))
	{
		CLog::Log(LOGERROR, "Device 0x%0x does not support streams", deviceParameters->device_id);
		return false;
	}

	err = AudioObjectGetPropertyDataSize(deviceParameters->device_id, &propertyAOPA, 0, NULL, &propertySize);
	
	if (err || !propertySize)
	{
		CLog::Log(LOGERROR, "Device 0x%0x has no output stream", deviceParameters->device_id);
		return false;
	}
	
	AudioStreamID* pStreams = (AudioStreamID *)malloc(propertySize);
	
	err = AudioObjectGetPropertyData(deviceParameters->device_id, &propertyAOPA, 0, NULL, &propertySize, pStreams);
	
	int numStreams = propertySize / sizeof(AudioStreamID);
	
	// iterate through streams and find digital outs
	propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal; // streams only have global scope
	propertyAOPA.mSelector = kAudioStreamPropertyAvailablePhysicalFormats;
	
	for (int i=0; i<numStreams; i++)
	{
		AudioStreamID currentStreamID = pStreams[i];
		if (AudioObjectHasProperty(currentStreamID, &propertyAOPA))
		{
			err = AudioObjectGetPropertyDataSize(currentStreamID, &propertyAOPA, 0, NULL, &propertySize);
			
			if (err || propertySize == 0)
			{
				CLog::Log(LOGERROR, "Unable to read output streams for device 0x%0x", deviceParameters->device_id);
				break;
			}
			
			AudioStreamRangedDescription *pASRDs = (AudioStreamRangedDescription *)malloc(propertySize);
			int streamDescriptionCount = propertySize / sizeof(AudioStreamRangedDescription);
			
			err = AudioObjectGetPropertyData(currentStreamID, &propertyAOPA, 0, NULL, &propertySize, pASRDs);
			if (err)
			{
				CLog::Log(LOGERROR, "Unable to read output streams for 0x%0x", deviceParameters->device_id);
				break;
			}
			
			for (int j=0; j<streamDescriptionCount; j++)
			{
				AudioStreamRangedDescription currentASRD = pASRDs[j];
				if (currentASRD.mFormat.mFormatID == kAudioFormat60958AC3 &&
					(currentASRD.mFormat.mSampleRate == 48000 || 
					 (currentASRD.mFormat.mSampleRate == kAudioStreamAnyRate &&
					  currentASRD.mSampleRateRange.mMinimum <= 48000 && 
					  currentASRD.mSampleRateRange.mMaximum >= 48000)))
				{
					CLog::Log(LOGINFO, "Found digital interface at stream ID 0x%0x for device 0x%0x", currentStreamID, deviceParameters->device_id);
					memcpy(&deviceParameters->stream_format, &currentASRD.mFormat, sizeof(AudioStreamBasicDescription));
					if (deviceParameters->stream_format.mSampleRate == kAudioStreamAnyRate) deviceParameters->stream_format.mSampleRate = 48000;
					deviceParameters->i_stream_id = currentStreamID;
					deviceParameters->i_stream_index = i;
					break;
				}
			}
			free(pASRDs);
		}
	}
	
	if (deviceParameters->i_stream_id == 0)
	{
		CLog::Log(LOGERROR, "%@ (0x%0x) has no digital interfaces", deviceParameters->device_id);
	}
	free(pStreams);
	
	// store current format
	propertySize = sizeof(AudioStreamBasicDescription);
	propertyAOPA.mSelector = kAudioStreamPropertyPhysicalFormat;
	err = AudioObjectGetPropertyData(deviceParameters->i_stream_id, &propertyAOPA, 0, NULL, &propertySize, &deviceParameters->sfmt_revert);
	
	CLog::Log(LOGINFO, STREAM_FORMAT_MSG("original stream format: ", deviceParameters->sfmt_revert ) );

    if( !AudioStreamChangeFormat(deviceParameters, deviceParameters->i_stream_id, deviceParameters->stream_format))
        return false;
	deviceParameters->b_revert = true;

	// Get device hardware buffer size

	uint32_t audioDeviceLatency, audioStreamLatency, audioDeviceBufferFrameSize, audioDeviceSafetyOffset;
	deviceParameters->hardwareFrameLatency = 0;
	propertySize = sizeof(uint32_t);

	propertyAOPA.mSelector = kAudioDevicePropertyLatency;
	err = AudioObjectGetPropertyData(deviceParameters->device_id, &propertyAOPA, 0, NULL, &propertySize, &audioDeviceLatency);
	if (err == noErr) deviceParameters->hardwareFrameLatency += audioDeviceLatency;
	
	propertyAOPA.mSelector = kAudioDevicePropertyBufferFrameSize;
	err = AudioObjectGetPropertyData(deviceParameters->device_id, &propertyAOPA, 0, NULL, &propertySize, &audioDeviceBufferFrameSize);
	if (err == noErr) deviceParameters->hardwareFrameLatency += audioDeviceBufferFrameSize;
	
	propertyAOPA.mSelector = kAudioDevicePropertySafetyOffset;
	err = AudioObjectGetPropertyData(deviceParameters->device_id, &propertyAOPA, 0, NULL, &propertySize, &audioDeviceSafetyOffset);
	if (err == noErr) deviceParameters->hardwareFrameLatency += audioDeviceSafetyOffset;
	
	propertyAOPA.mSelector = kAudioStreamPropertyLatency;
	err = AudioObjectGetPropertyData(deviceParameters->i_stream_id, &propertyAOPA, 0, NULL, &propertySize, &audioStreamLatency);
	if (err == noErr) deviceParameters->hardwareFrameLatency += audioStreamLatency;
	
	CLog::Log(LOGINFO, "Hardware latency: %i frames (%.2f msec @ %.0fHz)", deviceParameters->hardwareFrameLatency,
			  (float)deviceParameters->hardwareFrameLatency / deviceParameters->stream_format.mSampleRate * 1000,
			  deviceParameters->stream_format.mSampleRate);

  	// initialise the CoreAudio sink buffer
	uint32_t framecount = 1;
	while(framecount <= deviceParameters->stream_format.mSampleRate) // ensure power of 2
	{
		framecount <<= 1;
	}

#warning free
	deviceParameters->outputBuffer = (PaUtilRingBuffer *)malloc(sizeof(PaUtilRingBuffer));
	deviceParameters->outputBufferData = calloc(1, framecount * channels * bitsPerSample/8); // use uncompressed size if encoding ac3

	PaUtil_InitializeRingBuffer(deviceParameters->outputBuffer, channels * bitsPerSample/8, framecount, deviceParameters->outputBufferData);
	/* Add IOProc callback */
	err = AudioDeviceCreateIOProcID(deviceParameters->device_id,
									(AudioDeviceIOProc)RenderCallbackSPDIF,
									deviceParameters,
									&deviceParameters->sInputIOProcID);
    if( err != noErr )
    {
		CLog::Log(LOGERROR, "AudioDeviceAddIOProcID failed: [%4.4s]", (char *)&err );
        return false;
    }

    /* Start device */
    err = AudioDeviceStart(deviceParameters->device_id, (AudioDeviceIOProc)RenderCallbackSPDIF );
    if( err != noErr )
    {
		CLog::Log(LOGERROR, "AudioDeviceStart failed: [%4.4s]", (char *)&err );

        err = AudioDeviceDestroyIOProcID(deviceParameters->device_id,
										 (AudioDeviceIOProc)RenderCallbackSPDIF);
        if( err != noErr )
        {
			CLog::Log(LOGERROR, "AudioDeviceRemoveIOProc failed: [%4.4s]", (char *)&err );
        }
        return false;
    }

    return true;
}

/*****************************************************************************
 * AudioStreamChangeFormat: Change i_stream_id to change_format
 *****************************************************************************/
int CoreAudioAUHAL::AudioStreamChangeFormat(CoreAudioDeviceParameters *deviceParameters, AudioStreamID i_stream_id, AudioStreamBasicDescription change_format)
{
    OSStatus            status = noErr;
	AudioObjectPropertyAddress propertyAOPA;
	propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal;
	propertyAOPA.mElement = kAudioObjectPropertyElementMaster;	
	propertyAOPA.mSelector = kAudioStreamPropertyPhysicalFormat;
	UInt32 propertySize = sizeof(AudioStreamBasicDescription);
    int i;

    CLog::Log(LOGINFO, STREAM_FORMAT_MSG( "setting stream format: ", change_format ));
	
	CSingleLock lock(m_cs); // acquire lock

    /* change the format */
	status = AudioObjectSetPropertyData(deviceParameters->i_stream_id, &propertyAOPA, 0, NULL, propertySize, &change_format);
	
    if(status != noErr)
    {
		CLog::Log(LOGERROR, "could not revert the stream format: [%4.4s]", (char *)&status);
        return false;
    }
	
    /* The AudioStreamSetProperty is not only asynchronious (requiring the locks)
     * it is also not atomic in its behaviour.
     * Therefore we check 5 times before we really give up.*/
    for( i = 0; i < 5; i++ )
    {
        AudioStreamBasicDescription actual_format;
		usleep(20);
        status = AudioObjectGetPropertyData(deviceParameters->i_stream_id, &propertyAOPA, 0, NULL, &propertySize, &actual_format);
		
		CLog::Log(LOGINFO, STREAM_FORMAT_MSG("Active format: ", actual_format));
        if( actual_format.mSampleRate == change_format.mSampleRate &&
		   actual_format.mFormatID == change_format.mFormatID &&
		   actual_format.mFramesPerPacket == change_format.mFramesPerPacket )
        {
            /* The right format is now active */
            return true;
        }
        /* We need to check again */
    }
	
    return false;
}

/*****************************************************************************
 * RenderCallbackSPDIF: callback for SPDIF audio output
 *****************************************************************************/
OSStatus CoreAudioAUHAL::RenderCallbackSPDIF(AudioDeviceID inDevice,
                                    const AudioTimeStamp * inNow,
                                    const void * inInputData,
                                    const AudioTimeStamp * inInputTime,
                                    AudioBufferList * outOutputData,
                                    const AudioTimeStamp * inOutputTime,
                                    void * threadGlobals )
{
    CoreAudioDeviceParameters *deviceParameters = (CoreAudioDeviceParameters *)threadGlobals;

#define BUFFER outOutputData->mBuffers[deviceParameters->i_stream_index]
	int framesToWrite = BUFFER.mDataByteSize / (deviceParameters->stream_format.mChannelsPerFrame * deviceParameters->stream_format.mBitsPerChannel/8);
	//fprintf(stderr, "Frames requested: %i (%i bit, %i channels in, %i channels out)\n", 
	//		framesToWrite, 
	//		deviceParameters->stream_format.mBitsPerChannel,
	//		deviceParameters->m_ac3encoder.m_aftenContext.channels,
	//		deviceParameters->stream_format.mChannelsPerFrame);

	if (framesToWrite > PaUtil_GetRingBufferReadAvailable(deviceParameters->outputBuffer))
	{
		// we can't write a frame, send null frame
        memset(BUFFER.mData, 0, BUFFER.mDataByteSize);
    }
	else
	{
		// write a frame
		if (deviceParameters->m_bEncodeAC3)
		{
			PaUtil_ReadRingBuffer(deviceParameters->outputBuffer, deviceParameters->rawSampleBuffer, framesToWrite);
			/*fprintf(stderr, "Got %i frames from encoder\n", */ac3encoderEncodePCM(&deviceParameters->m_ac3encoder, (uint8_t *)&deviceParameters->rawSampleBuffer, (uint8_t *)BUFFER.mData, framesToWrite)/*)*/;

		}
		else PaUtil_ReadRingBuffer(deviceParameters->outputBuffer, BUFFER.mData, framesToWrite);
	}
#undef BUFFER
    return( noErr );
}

#endif

