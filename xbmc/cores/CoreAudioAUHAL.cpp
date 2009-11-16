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

#define SAFE(expr) {  \
    err = expr;       \
    if (err != noErr) \
      CLog::Log(LOGERROR, "CoreAudio Error (%s) [%4.4s]", #expr, (char *)&err); }

#define SAFE_WARN(expr) {  \
    err = expr;            \
    if (err != noErr)      \
      CLog::Log(LOGWARNING, "CoreAudio Warning (%s) [%4.4s]", #expr, (char *)&err); }

#define SAFE_RETURN(expr) {  \
    err = expr;              \
    if (err != noErr)        \
    {                        \
      CLog::Log(LOGERROR, "CoreAudio Error (%s) [%4.4s]", #expr, (char *)&err); \
      return false;          \
    } }

bool CoreAudioAUHAL::s_lastPlayWasSpdif = false;

struct CoreAudioDeviceParameters
{
  // AUHAL specific.
	AudioDeviceID				device_id;
	Component           au_component;   /* The Audio component we use */
	AudioUnit           au_unit;        /* The AudioUnit we use */
	PaUtilRingBuffer*		outputBuffer;
	void*						    outputBufferData;
	int							    hardwareFrameLatency;
	bool						    b_digital;      /* Are we running in digital mode? */

	/* CoreAudio SPDIF mode specific */
  AudioDeviceIOProcID			    sInputIOProcID;
  AudioStreamID               i_stream_id;    /* The StreamID that has a cac3 streamformat */
  int                         i_stream_index; /* The index of i_stream_id in an AudioBufferList */
  AudioStreamBasicDescription stream_format;  /* The format we changed the stream to */
  AudioStreamBasicDescription sfmt_revert;    /* The original format of the stream */
  bool                        b_revert;       /* Whether we need to revert the stream format */
  bool					              hardwareReady; // set once the hardware stream format is set
  	
  // AC3 Encoder 	
  bool					m_bEncodeAC3;
  AC3Encoder		m_ac3encoder;
	uint8_t				rawSampleBuffer[AC3_SAMPLES_PER_FRAME * SPDIF_SAMPLESIZE/8 * 6]; // 6 channels = 12 bytes/sample

	volatile bool m_bInitializationComplete;
};

/*****************************************************************************
 * Open: open macosx audio output
 *****************************************************************************/
CoreAudioAUHAL::CoreAudioAUHAL(const CStdString& strName, const char *strCodec, int channels, unsigned int sampleRate, int bitsPerSample, bool passthrough, bool isMusic, int packetSize)
  : m_bIsInitialized(false)
{
  OSStatus                err = noErr;
  UInt32                  i_param_size = 0;
	
	// Allocate structure.
	deviceParameters = (CoreAudioDeviceParameters*)calloc(sizeof(CoreAudioDeviceParameters), 1);
	if (!deviceParameters) return;
	
	if (g_audioConfig.HasDigitalOutput() && // disable encoder for legacy SPDIF devices for now
		channels > 2 &&
		!passthrough &&
		(sampleRate == SPDIF_SAMPLERATE44 || sampleRate == SPDIF_SAMPLERATE48))
	{
		// Enable AC3 passthrough for digital devices
		int mpeg_remapping = 0;
		if (strCodec == "AAC" || strCodec == "DTS") 
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
	  CLog::Log(LOGERROR, "There is no selected device, very strange, there are %ld available ones.", deviceArray->getDevices().size());
	  return;
	}
	
	deviceParameters->device_id = deviceArray->getSelectedDevice()->getDeviceID();

	i_param_size = sizeof(pid_t);
	AudioObjectPropertyAddress propertyAOPA;
	propertyAOPA.mScope = kAudioDevicePropertyScopeOutput;
	propertyAOPA.mElement = kAudioObjectPropertyElementMaster;	
	propertyAOPA.mSelector = kAudioDevicePropertyHogMode;
	
	pid_t currentHogMode = -1;
	if (AudioObjectHasProperty(deviceParameters->device_id, &propertyAOPA))
		SAFE_WARN(AudioObjectGetPropertyData(deviceParameters->device_id, &propertyAOPA, 0, NULL, &i_param_size, &currentHogMode));
		
	if (currentHogMode != -1 && currentHogMode != getpid())
	{
		CLog::Log(LOGERROR, "Selected audio device is exclusively in use by another program.");
		return;
	}

	// Check for Digital mode or Analog output mode.
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
	
	OSStatus err = noErr;
	AudioObjectPropertyAddress propertyAOPA;
	propertyAOPA.mScope = kAudioDevicePropertyScopeOutput;
	propertyAOPA.mElement = kAudioObjectPropertyElementMaster;
	
	if(deviceParameters->au_unit)
	{
	  SAFE(AudioOutputUnitStop(deviceParameters->au_unit));
	  SAFE(AudioUnitUninitialize(deviceParameters->au_unit));
		SAFE(CloseComponent(deviceParameters->au_unit));
      
		deviceParameters->au_unit = NULL;
	}
		
	if (deviceParameters->b_digital)
	{
	  CLog::Log(LOGINFO, "Stopping Core Audio device.");
		
	  // Stop device.
	  SAFE(AudioDeviceStop(deviceParameters->device_id, (AudioDeviceIOProc)RenderCallbackSPDIF));
		
	  // Remove IOProc callback.
		SAFE(AudioDeviceDestroyIOProcID(deviceParameters->device_id, deviceParameters->sInputIOProcID));

		if (deviceParameters->b_revert)
		{
			CLog::Log(LOGDEBUG, "Reverting CoreAudio stream mode");
			AudioStreamChangeFormat(deviceParameters, deviceParameters->i_stream_id, deviceParameters->sfmt_revert);
			
			// Now we have to wait for it to finish deinitializing.
			for (int i=0; i<40 && deviceParameters->m_bInitializationComplete == true; i++)
			  Sleep(50);
		}
	}
	
	 if (deviceParameters->m_bEncodeAC3)
	    ac3encoderFinalise(&deviceParameters->m_ac3encoder);
		
	free(deviceParameters->outputBuffer);
	free(deviceParameters->outputBufferData);
		
	m_bIsInitialized = false;
	return S_OK;
}
												
DWORD CoreAudioAUHAL::GetSpace()
{
	DWORD fakeCeiling, bufferDataSize = PaUtil_GetRingBufferReadAvailable(deviceParameters->outputBuffer);
	
	if (m_bIsMusic)
		fakeCeiling = PACKET_SIZE / deviceParameters->stream_format.mChannelsPerFrame;
	else
		fakeCeiling = deviceParameters->stream_format.mSampleRate * CA_BUFFER_FACTOR;
	
	if (bufferDataSize < fakeCeiling)
		return fakeCeiling - bufferDataSize;
	else
		return 0;
}

float CoreAudioAUHAL::GetHardwareLatency()
{
	float latency = ((float)deviceParameters->hardwareFrameLatency / deviceParameters->stream_format.mSampleRate);
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
}

int CoreAudioAUHAL::WriteStream(uint8_t *sampleBuffer, uint32_t samplesToWrite)
{
	if (sampleBuffer == NULL || samplesToWrite == 0)
		return 0;
	
	// dump frames unless the stream is active
	if (!deviceParameters->b_digital || deviceParameters->hardwareReady)
		return PaUtil_WriteRingBuffer(deviceParameters->outputBuffer, sampleBuffer, samplesToWrite);
	else 
	  return samplesToWrite;
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
	
	// Lets go find our Component.
	ComponentDescription desc;
	desc.componentType = kAudioUnitType_Output;
	desc.componentSubType = kAudioUnitSubType_HALOutput;
	desc.componentManufacturer = kAudioUnitManufacturer_Apple;
	desc.componentFlags = 0;
	desc.componentFlagsMask = 0;
	
	deviceParameters->au_component = FindNextComponent(NULL, &desc);
	if (deviceParameters->au_component == NULL)
	{
		CLog::Log(LOGERROR, "we cannot find our HAL component");
		return false;
	}
	
	SAFE_RETURN(OpenAComponent(deviceParameters->au_component, &deviceParameters->au_unit));
	AudioDeviceID selectedDeviceID = deviceArray->getSelectedDevice()->getDeviceID();
    
	// Set the device we will use for this output unit.
  SAFE_RETURN(AudioUnitSetProperty(deviceParameters->au_unit, kAudioOutputUnitProperty_CurrentDevice, kAudioUnitScope_Input, 0, &selectedDeviceID, sizeof(AudioDeviceID)));

  // Get the current format.
  i_param_size = sizeof(AudioStreamBasicDescription);
  SAFE_RETURN(AudioUnitGetProperty(deviceParameters->au_unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &deviceParameters->sfmt_revert, &i_param_size));
  CLog::Log(LOGINFO, STREAM_FORMAT_MSG("Current format is: ", deviceParameters->sfmt_revert));
    
  // Set up the format to be used.
  DeviceFormat.mSampleRate = sampleRate;
  DeviceFormat.mFormatID = kAudioFormatLinearPCM;
  DeviceFormat.mFormatFlags = (bitsPerSample == 32 ? kLinearPCMFormatFlagIsFloat : kLinearPCMFormatFlagIsSignedInteger);
  DeviceFormat.mBitsPerChannel = bitsPerSample;
  DeviceFormat.mChannelsPerFrame = channels;

  // Calculate frame sizes and stuff.
  DeviceFormat.mFramesPerPacket = 1;
  DeviceFormat.mBytesPerFrame = DeviceFormat.mBitsPerChannel/8 * DeviceFormat.mChannelsPerFrame;
  DeviceFormat.mBytesPerPacket = DeviceFormat.mBytesPerFrame * DeviceFormat.mFramesPerPacket;

  // Set the desired format.
  i_param_size = sizeof(AudioStreamBasicDescription);
  SAFE_RETURN(AudioUnitSetProperty(deviceParameters->au_unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &DeviceFormat, i_param_size));
	CLog::Log(LOGINFO, STREAM_FORMAT_MSG("We set the AU format: ", DeviceFormat));

  // Retrieve actual format.
	SAFE_RETURN(AudioUnitGetProperty(deviceParameters->au_unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &deviceParameters->stream_format, &i_param_size));
	CLog::Log(LOGINFO, STREAM_FORMAT_MSG("The actual set AU format is ", DeviceFormat));

  // set the IOproc callback.
	input.inputProc = (AURenderCallback) RenderCallbackAnalog;
	input.inputProcRefCon = deviceParameters;

	SAFE_RETURN(AudioUnitSetProperty(deviceParameters->au_unit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Global, 0, &input, sizeof(input)));

	// AU initialize.
  SAFE_RETURN(AudioUnitInitialize(deviceParameters->au_unit));

	// Get AU hardware buffer size
	uint32_t audioDeviceLatency = 0;
	uint32_t audioDeviceBufferFrameSize = 0;
	uint32_t audioDeviceSafetyOffset = 0;

	// Get the latency.
	deviceParameters->hardwareFrameLatency = 0;
	i_param_size = sizeof(uint32_t);
	SAFE_RETURN(AudioUnitGetProperty(deviceParameters->au_unit, kAudioDevicePropertyLatency, kAudioUnitScope_Global, 0, &audioDeviceLatency, &i_param_size));
	deviceParameters->hardwareFrameLatency += audioDeviceLatency;

	// Get the buffer frame size.
	SAFE_RETURN(AudioUnitGetProperty(deviceParameters->au_unit, kAudioDevicePropertyBufferFrameSize, kAudioUnitScope_Global, 0, &audioDeviceBufferFrameSize, &i_param_size));
	deviceParameters->hardwareFrameLatency += audioDeviceBufferFrameSize;

	// Get the safety offset.
	SAFE_RETURN(AudioUnitGetProperty(deviceParameters->au_unit, kAudioDevicePropertySafetyOffset, kAudioUnitScope_Global, 0, &audioDeviceSafetyOffset, &i_param_size));
	deviceParameters->hardwareFrameLatency += audioDeviceSafetyOffset;

	CLog::Log(LOGINFO, "Hardware latency: %i frames (%.2f msec @ %.0fHz)", deviceParameters->hardwareFrameLatency,
			  (float)deviceParameters->hardwareFrameLatency / deviceParameters->stream_format.mSampleRate * 1000,
			  deviceParameters->stream_format.mSampleRate);

	// Initialise the CoreAudio sink buffer
	uint32_t framecount = 1;
	while(framecount <= deviceParameters->stream_format.mSampleRate) // ensure power of 2
		framecount <<= 1;
	
	deviceParameters->outputBuffer = (PaUtilRingBuffer *)malloc(sizeof(PaUtilRingBuffer));
	deviceParameters->outputBufferData = malloc(framecount * deviceParameters->stream_format.mBytesPerFrame);

	PaUtil_InitializeRingBuffer(deviceParameters->outputBuffer,
								deviceParameters->stream_format.mBytesPerFrame,
								framecount, deviceParameters->outputBufferData);

	// Start the AU.
	SAFE_RETURN(AudioOutputUnitStart(deviceParameters->au_unit));

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
		framesToWrite = framesAvailable;
	
	int currentPos = framesToWrite * deviceParameters->stream_format.mBytesPerFrame;
	int underrunLength = (inNumberFrames - framesToWrite) * deviceParameters->stream_format.mBytesPerFrame;

	// write as many frames as possible from buffer
	PaUtil_ReadRingBuffer(deviceParameters->outputBuffer, ioData->mBuffers[0].mData, framesToWrite);

	// write silence to any remainder
	if (underrunLength > 0)
		memset((void *)((uint8_t *)(ioData->mBuffers[0].mData)+currentPos), 0, underrunLength);

	return noErr;
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

	CLog::Log(LOGINFO, "Opening in SPDIF mode.");
	
	// We're digital.
	deviceParameters->hardwareReady = false;
	s_lastPlayWasSpdif = true;
    
	// Retrieve all the output streams.
	propertyAOPA.mSelector = kAudioDevicePropertyStreams;
	if (!AudioObjectHasProperty(deviceParameters->device_id, &propertyAOPA))
	{
		CLog::Log(LOGERROR, "Device 0x%lx does not support streams", deviceParameters->device_id);
		return false;
	}

	SAFE_RETURN(AudioObjectGetPropertyDataSize(deviceParameters->device_id, &propertyAOPA, 0, NULL, &propertySize));
	if (propertySize == 0)
	{
		CLog::Log(LOGERROR, "Device 0x%lx has no output stream", deviceParameters->device_id);
		return false;
	}
	
	AudioStreamID* pStreams = (AudioStreamID *)malloc(propertySize);
	
	SAFE_RETURN(AudioObjectGetPropertyData(deviceParameters->device_id, &propertyAOPA, 0, NULL, &propertySize, pStreams));
	int numStreams = propertySize / sizeof(AudioStreamID);
	CLog::Log(LOGINFO, "There are %d output streams.", numStreams);
	
	// Iterate through streams and find digital outputs.
	propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal; // streams only have global scope
	propertyAOPA.mSelector = kAudioStreamPropertyAvailablePhysicalFormats;
	
	for (int i=0; i<numStreams; i++)
	{
		AudioStreamID currentStreamID = pStreams[i];
		if (AudioObjectHasProperty(currentStreamID, &propertyAOPA))
		{
			SAFE(AudioObjectGetPropertyDataSize(currentStreamID, &propertyAOPA, 0, NULL, &propertySize));
			if (err || propertySize == 0)
			{
				CLog::Log(LOGERROR, "Unable to read output streams for device 0x%lx", deviceParameters->device_id);
				break;
			}
			
			AudioStreamRangedDescription *pASRDs = (AudioStreamRangedDescription *)malloc(propertySize);
			int streamDescriptionCount = propertySize / sizeof(AudioStreamRangedDescription);
			
			SAFE(AudioObjectGetPropertyData(currentStreamID, &propertyAOPA, 0, NULL, &propertySize, pASRDs));
			if (err)
			{
				CLog::Log(LOGERROR, "Unable to read output streams for 0x%lx", deviceParameters->device_id);
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
					CLog::Log(LOGINFO, "Found digital interface at stream ID 0x%lx for device 0x%lx", currentStreamID, deviceParameters->device_id);
					memcpy(&deviceParameters->stream_format, &currentASRD.mFormat, sizeof(AudioStreamBasicDescription));
					
					if (deviceParameters->stream_format.mSampleRate == kAudioStreamAnyRate) 
					  deviceParameters->stream_format.mSampleRate = 48000;
					
					deviceParameters->i_stream_id = currentStreamID;
					deviceParameters->i_stream_index = i;
					break;
				}
			}
			free(pASRDs);
		}
	}
	
	free(pStreams);
	
	if (deviceParameters->i_stream_id == 0)
	{
		CLog::Log(LOGERROR, "Device 0x%lx has no digital interfaces", deviceParameters->device_id);
		return false;
	}
	
	// Store current format
	propertySize = sizeof(AudioStreamBasicDescription);
	propertyAOPA.mScope = kAudioDevicePropertyScopeOutput;
	propertyAOPA.mSelector = kAudioStreamPropertyPhysicalFormat;

	SAFE(AudioObjectGetPropertyData(deviceParameters->i_stream_id, &propertyAOPA, 0, NULL, &propertySize, &deviceParameters->sfmt_revert));
	CLog::Log(LOGINFO, STREAM_FORMAT_MSG("original stream format: ", deviceParameters->sfmt_revert ) );
	
	// Add stream listener.
	propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal;
	propertyAOPA.mSelector = kAudioStreamPropertyPhysicalFormat;
	SAFE_RETURN(AudioObjectAddPropertyListener(deviceParameters->i_stream_id, &propertyAOPA, HardwareStreamListener, deviceParameters));

	if (!AudioStreamChangeFormat(deviceParameters, deviceParameters->i_stream_id, deviceParameters->stream_format))
	  return false;

  // We need to wait for this to complete.
  for (int i=0; i<40 && deviceParameters->m_bInitializationComplete == false; i++)
    Sleep(50);
	
	deviceParameters->b_revert = true;

	// Get device hardware buffer size
	uint32_t audioDeviceLatency, audioStreamLatency, audioDeviceBufferFrameSize, audioDeviceSafetyOffset;
	deviceParameters->hardwareFrameLatency = 0;
	propertySize = sizeof(uint32_t);

	propertyAOPA.mScope = kAudioDevicePropertyScopeOutput;
	propertyAOPA.mSelector = kAudioDevicePropertyLatency;
	SAFE(AudioObjectGetPropertyData(deviceParameters->device_id, &propertyAOPA, 0, NULL, &propertySize, &audioDeviceLatency));
	if (err == noErr) deviceParameters->hardwareFrameLatency += audioDeviceLatency;
	
	propertyAOPA.mSelector = kAudioDevicePropertyBufferFrameSize;
	SAFE(AudioObjectGetPropertyData(deviceParameters->device_id, &propertyAOPA, 0, NULL, &propertySize, &audioDeviceBufferFrameSize));
	if (err == noErr) deviceParameters->hardwareFrameLatency += audioDeviceBufferFrameSize;
	
	propertyAOPA.mSelector = kAudioDevicePropertySafetyOffset;
	SAFE(AudioObjectGetPropertyData(deviceParameters->device_id, &propertyAOPA, 0, NULL, &propertySize, &audioDeviceSafetyOffset));
	if (err == noErr) deviceParameters->hardwareFrameLatency += audioDeviceSafetyOffset;
	
	propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal; // streams only have global scope
	propertyAOPA.mSelector = kAudioStreamPropertyLatency;
	SAFE(AudioObjectGetPropertyData(deviceParameters->i_stream_id, &propertyAOPA, 0, NULL, &propertySize, &audioStreamLatency));
	if (err == noErr) deviceParameters->hardwareFrameLatency += audioStreamLatency;
	
	CLog::Log(LOGINFO, "Hardware latency: %i frames (%.2f msec @ %.0fHz)", deviceParameters->hardwareFrameLatency,
			  (float)deviceParameters->hardwareFrameLatency / deviceParameters->stream_format.mSampleRate * 1000,
			  deviceParameters->stream_format.mSampleRate);

	// Initialise the CoreAudio sink buffer.
	uint32_t framecount = 1;
	while(framecount <= deviceParameters->stream_format.mSampleRate) // ensure power of 2
		framecount <<= 1;

	deviceParameters->outputBuffer = (PaUtilRingBuffer *)malloc(sizeof(PaUtilRingBuffer));
	deviceParameters->outputBufferData = calloc(1, framecount * channels * bitsPerSample/8); // use uncompressed size if encoding ac3

	PaUtil_InitializeRingBuffer(deviceParameters->outputBuffer, channels * bitsPerSample/8, framecount, deviceParameters->outputBufferData);
	
	// Add IOProc callback.
	SAFE_RETURN(AudioDeviceCreateIOProcID(deviceParameters->device_id, (AudioDeviceIOProc)RenderCallbackSPDIF, deviceParameters, &deviceParameters->sInputIOProcID));
	
  // Start device.
	deviceParameters->hardwareReady = true;

  SAFE(AudioDeviceStart(deviceParameters->device_id, (AudioDeviceIOProc)RenderCallbackSPDIF));
  if (err != noErr)
    SAFE(AudioDeviceDestroyIOProcID(deviceParameters->device_id, (AudioDeviceIOProc)RenderCallbackSPDIF));

	return true;
}

/*****************************************************************************
 * AudioStreamChangeFormat: Change i_stream_id to change_format
 *****************************************************************************/
int CoreAudioAUHAL::AudioStreamChangeFormat(CoreAudioDeviceParameters *deviceParameters, AudioStreamID i_stream_id, AudioStreamBasicDescription change_format)
{
  OSStatus err = noErr;
	AudioObjectPropertyAddress propertyAOPA;
	propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal;
	propertyAOPA.mElement = kAudioObjectPropertyElementMaster;	
	propertyAOPA.mSelector = kAudioStreamPropertyPhysicalFormat;
	UInt32 propertySize = sizeof(AudioStreamBasicDescription);

	CLog::Log(LOGINFO, STREAM_FORMAT_MSG( "setting stream format: ", change_format ));
	
	CSingleLock lock(m_cs); // acquire lock

	// change the format.
	SAFE_RETURN(AudioObjectSetPropertyData(deviceParameters->i_stream_id, &propertyAOPA, 0, NULL, propertySize, &change_format));
	
	return true;
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
			ac3encoderEncodePCM(&deviceParameters->m_ac3encoder, (uint8_t *)&deviceParameters->rawSampleBuffer, (uint8_t *)BUFFER.mData, framesToWrite)/*)*/;
		}
		else
		{
		  PaUtil_ReadRingBuffer(deviceParameters->outputBuffer, BUFFER.mData, framesToWrite);
		}
	}
	
#undef BUFFER
	
	return noErr;
}

OSStatus CoreAudioAUHAL::HardwareStreamListener(AudioObjectID inObjectID,
										UInt32        inNumberAddresses,
										const AudioObjectPropertyAddress inAddresses[],
										void* inClientData)
{	
	CoreAudioDeviceParameters* deviceParameters = (CoreAudioDeviceParameters* )inClientData;
	OSStatus err = noErr;
	
	for (int i=0; i<inNumberAddresses; i++)
	{
		if (inAddresses[i].mSelector == kAudioStreamPropertyPhysicalFormat)
		{
			// Hardware physical format has changed.
			AudioStreamBasicDescription actual_format;
			UInt32 propertySize = sizeof(AudioStreamBasicDescription);
			SAFE(AudioObjectGetPropertyData(deviceParameters->i_stream_id, &inAddresses[i], 0, NULL, &propertySize, &actual_format));
			
			CLog::Log(LOGINFO, STREAM_FORMAT_MSG("HardwareStreamListener: Hardware format changed to ", actual_format));
			
			if (deviceParameters->hardwareReady == false)
			{
				// Mark us as having initialized.
				deviceParameters->m_bInitializationComplete = true;
			}
			else if (deviceParameters->hardwareReady == true && deviceParameters->b_revert == true)
			{
        deviceParameters->hardwareReady = false;
        CLog::Log(LOGDEBUG, "Reverting CoreAudio mix mode");
        
        // Remove the listener.
			  AudioObjectPropertyAddress propertyAOPA;
			  propertyAOPA.mElement = kAudioObjectPropertyElementMaster;  
			  propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal;
			  propertyAOPA.mSelector = kAudioStreamPropertyPhysicalFormat;
			  SAFE(AudioObjectRemovePropertyListener(deviceParameters->i_stream_id, &propertyAOPA, HardwareStreamListener, deviceParameters));
			    
        UInt32 mixable = 1;
        UInt32 i_param_size = sizeof(UInt32);
        
        // Set mixable to true.
        propertyAOPA.mScope = kAudioDevicePropertyScopeOutput;
        propertyAOPA.mSelector = kAudioDevicePropertySupportsMixing;
        if (AudioObjectHasProperty(deviceParameters->device_id, &propertyAOPA))
          SAFE(AudioObjectSetPropertyData(deviceParameters->device_id, &propertyAOPA, 0, NULL, i_param_size, &mixable));

        // Revert hog mode.
        CLog::Log(LOGINFO, "Reverting CoreAudio hog mode\n");
        propertyAOPA.mSelector = kAudioDevicePropertyHogMode;
        pid_t currentHogMode;
        i_param_size = sizeof(pid_t);
        
        if (AudioObjectHasProperty(deviceParameters->device_id, &propertyAOPA))
        {
          SAFE(AudioObjectGetPropertyData(deviceParameters->device_id, &propertyAOPA, 0, NULL, &i_param_size, &currentHogMode));
          
          if (currentHogMode == getpid())
            SAFE(AudioObjectSetPropertyData(deviceParameters->device_id, &propertyAOPA, 0, NULL, i_param_size, &currentHogMode));
        }
      
        // Mark us as having deinitialized.
        deviceParameters->m_bInitializationComplete = false;
			}
		}
	}
	
	return noErr;
}

#endif

