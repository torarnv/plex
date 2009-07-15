/*
 *  CoreAudioPlexSupport.cpp
 *  Plex
 *
 *  Created by Ryan Walklin on 9/7/08.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include <boost/foreach.hpp>

#include "CoreAudioPlexSupport.h"
#include "stdafx.h"
#include "Settings.h"
#include "log.h"

#define SAFELY(expr)     \
  if (err == noErr)      \
  {                      \
    err = (expr);        \
    if (err != noErr)    \
      printf("PlexAudioDevice: Error %4.4s doing %s\n", (char* )&err, #expr); \
  }

PlexAudioDevicePtr PlexAudioDevices::g_selectedDevice;

///////////////////////////////////////////////////////////////////////////////
PlexAudioDevice::PlexAudioDevice(AudioDeviceID deviceID)
  : m_deviceID(deviceID)
  , m_isValid(false)
  , m_supportsDigital(false)
{
  UInt32   paramSize = 0;
  OSStatus err = noErr;

  // Retrieve the length of the device name.
  SAFELY(AudioDeviceGetPropertyInfo(deviceID, 0, false, kAudioDevicePropertyDeviceName, &paramSize, NULL));
  if (err == noErr)
  {
    // Retrieve the name of the device.
    char* pStrName = new char[paramSize];
    pStrName[0] = '\0';
    
    SAFELY(AudioDeviceGetProperty(deviceID, 0, false, kAudioDevicePropertyDeviceName, &paramSize, pStrName));
    if (err == noErr)
    {
      m_deviceName = pStrName;
      
      // See if the device is writable (can output).
      m_hasOutput = computeHasOutput();
      
      // If the device does have output, see if it supports digital.
      if (m_hasOutput)
        m_supportsDigital = computeDeviceSupportsDigital();
      
      m_isValid = true;
    }
    
    delete[] pStrName;
  }
}

///////////////////////////////////////////////////////////////////////////////
bool PlexAudioDevice::computeHasOutput()
{
  UInt32  dataSize = 0;
  Boolean isWritable = false;

  AudioDeviceGetPropertyInfo(m_deviceID, 0, FALSE, kAudioDevicePropertyStreams, &dataSize, &isWritable);
  if (dataSize == 0)
    return false;
  
  return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PlexAudioDevice::computeDeviceSupportsDigital()
{
  bool ret = false;
  
  OSStatus err = noErr;
  UInt32   paramSize = 0;
    
  // Retrieve all the output streams.
  SAFELY(AudioDeviceGetPropertyInfo(m_deviceID, 0, FALSE, kAudioDevicePropertyStreams, &paramSize, NULL));
  if (err == noErr)
  {
    int numStreams = paramSize / sizeof(AudioStreamID);
    AudioStreamID* pStreams = (AudioStreamID *)malloc(paramSize);

    SAFELY(AudioDeviceGetProperty(m_deviceID, 0, FALSE, kAudioDevicePropertyStreams, &paramSize, pStreams));
    if (err == noErr)
    {
      for (int i=0; i<numStreams && ret == false; i++)
      {
        if (computeStreamSupportsDigital(pStreams[i]))
            ret = true;
      }
    }

    free(pStreams);
  }
  
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
bool PlexAudioDevice::computeStreamSupportsDigital(AudioStreamID streamID)
{
  bool ret = false;

  OSStatus err = noErr;
  UInt32   paramSize = 0;
  
  // Retrieve all the stream formats supported by each output stream.
  SAFELY(AudioStreamGetPropertyInfo(streamID, 0, kAudioStreamPropertyPhysicalFormats, &paramSize, NULL));
  if (err == noErr)
  {
    int numFormats = paramSize / sizeof(AudioStreamBasicDescription);
    AudioStreamBasicDescription* pFormatList = (AudioStreamBasicDescription *)malloc(paramSize);

    SAFELY(AudioStreamGetProperty(streamID, 0, kAudioStreamPropertyPhysicalFormats, &paramSize, pFormatList));
    if (err == noErr)
    {
      for(int i=0; i<numFormats && ret == false; i++)
      {
        if (pFormatList[i].mFormatID == 'IAC3' || pFormatList[i].mFormatID == kAudioFormat60958AC3)
          ret = true;
      }
    }
    
    free(pFormatList);
  }

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
void PlexAudioDevice::setDefault()
{
  OSStatus err = noErr;
  SAFELY(AudioHardwareSetProperty(kAudioHardwarePropertyDefaultOutputDevice, sizeof (UInt32), &m_deviceID));
}

///////////////////////////////////////////////////////////////////////////////
void PlexAudioDevice::reset()
{
  OSStatus err = noErr;
  UInt32   paramSize = 0;
  AudioStreamBasicDescription deviceFormat;

  // Set up the basic default format.
  deviceFormat.mSampleRate = 48000.0;
  deviceFormat.mFormatID = kAudioFormatLinearPCM;
  deviceFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
  deviceFormat.mBitsPerChannel = 16;
  deviceFormat.mChannelsPerFrame = 2;
  deviceFormat.mFramesPerPacket = 1;
  deviceFormat.mBytesPerFrame = 4;
  deviceFormat.mBytesPerPacket = 4;
  deviceFormat.mReserved = 0;
  
  // Retrieve all the output streams.
  SAFELY(AudioDeviceGetPropertyInfo(m_deviceID, 0, FALSE, kAudioDevicePropertyStreams, &paramSize, NULL));
  if (err == noErr)
  {
    int numStreams = paramSize / sizeof(AudioStreamID);
    AudioStreamID* pStreams = (AudioStreamID *)malloc(paramSize);

    SAFELY(AudioDeviceGetProperty(m_deviceID, 0, FALSE, kAudioDevicePropertyStreams, &paramSize, pStreams));
    if (err == noErr)
    {
      for (int i=0; i<numStreams; i++)
      {
        // Change the format.
        SAFELY(AudioStreamSetProperty(pStreams[i], 0, 0, kAudioStreamPropertyPhysicalFormat, sizeof(AudioStreamBasicDescription), &deviceFormat));
      }
    }

    free(pStreams);
  }
}

///////////////////////////////////////////////////////////////////////////////
bool PlexAudioDevice::isAlive()
{
  OSStatus err = noErr;
  UInt32   alive = 1;
  UInt32   paramSize = 0;
  
  paramSize = sizeof(alive);
  SAFELY(AudioDeviceGetProperty(m_deviceID, 0, FALSE, kAudioDevicePropertyDeviceIsAlive, &paramSize, &alive));

  return (alive != 0);
}

///////////////////////////////////////////////////////////////////////////////
float PlexAudioDevice::getVolume()
{
  float         ret = 0;
  OSStatus      err = noErr;
  UInt32        paramSize;
  
  // Try to get master volume (channel 0)
  paramSize = sizeof(ret);
  if (AudioDeviceGetProperty(m_deviceID, 0, FALSE, kAudioDevicePropertyVolumeDecibels, &paramSize, &ret) == noErr)
    return ret;
    
  // Otherwise, try separate channels, first getting channel numbers.
  UInt32 channels[2];
  paramSize = sizeof(channels);
  SAFELY(AudioDeviceGetProperty(m_deviceID, 0, FALSE, kAudioDevicePropertyPreferredChannelsForStereo, &paramSize, &channels));
  
  float volume[2];
  paramSize = sizeof(float);
  SAFELY(AudioDeviceGetProperty(m_deviceID, channels[0], FALSE, kAudioDevicePropertyVolumeDecibels, &paramSize, &volume[0]));
  SAFELY(AudioDeviceGetProperty(m_deviceID, channels[1], FALSE, kAudioDevicePropertyVolumeDecibels, &paramSize, &volume[1]));
  
  // See if it's muted.
  int muted = 0;
  paramSize = sizeof(muted);
  SAFELY(AudioDeviceGetProperty(m_deviceID, 0, FALSE, kAudioDevicePropertyMute, &paramSize, &muted));
  
  // If it's muted pretend the volume is at minimum.
  if (muted == 1)
    ret = -60.0;
  else
    ret = ((volume[0]+volume[1])/2.00)*60.0/65.0;
  
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
void PlexAudioDevice::setVolume(float vol)
{
  UInt32   paramSize = 0;
  OSStatus err = noErr;
  Boolean  canSet = false;
  
  // Convert from Plex's [-60, 0] to [-65, 0].
  vol = (65.0/60.0) * vol;
      
  // Make sure we're not muted if we're setting volume.
  if (vol > -65.0)
  {
    int muted = 0;
    paramSize = sizeof(muted);
    SAFELY(AudioDeviceSetProperty(m_deviceID, NULL, 0, false, kAudioDevicePropertyMute, paramSize, &muted));
  }
  
  // Try to set master-channel (0) volume.
  paramSize = sizeof(canSet);
  err = AudioDeviceGetPropertyInfo(m_deviceID, 0, FALSE, kAudioDevicePropertyVolumeDecibels, &paramSize, &canSet);
  if (err == noErr && canSet == true) 
  {
    paramSize = sizeof(vol);
    SAFELY(AudioDeviceSetProperty(m_deviceID, NULL, 0, false, kAudioDevicePropertyVolumeDecibels, paramSize, &vol));
    return;
  }

  // Otherwise, try separate channels.
  UInt32 channels[2];
  err = noErr;
  paramSize = sizeof(channels);

  SAFELY(AudioDeviceGetProperty(m_deviceID, 0, false, kAudioDevicePropertyPreferredChannelsForStereo, &paramSize, &channels));
    
  // Set volume
  paramSize = sizeof(vol);
  SAFELY(AudioDeviceSetProperty(m_deviceID, 0, channels[0], false, kAudioDevicePropertyVolumeDecibels, paramSize, &vol));
  SAFELY(AudioDeviceSetProperty(m_deviceID, 0, channels[1], false, kAudioDevicePropertyVolumeDecibels, paramSize, &vol));
}

///////////////////////////////////////////////////////////////////////////////
PlexAudioDevicesPtr PlexAudioDevices::FindAll()
{
	OSStatus          err = noErr;
	UInt32            paramSize = 0;
	PlexAudioDevices* audioDevices = new PlexAudioDevices();
	
	// Get number of devices.
  SAFELY(AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices, &paramSize, NULL));
  if (err == noErr)
  {
    int totalDeviceCount = paramSize / sizeof(AudioDeviceID);
    if (totalDeviceCount > 0)
    {
      CLog::Log(LOGDEBUG, "System has %ld device(s)", totalDeviceCount);

      // Allocate the device ID array and retreive them.
      AudioDeviceID* pDevices = (AudioDeviceID* )malloc(paramSize);
      SAFELY(AudioHardwareGetProperty( kAudioHardwarePropertyDevices, &paramSize, pDevices));
      if (err == noErr)
      {
        AudioDeviceID defaultDeviceID = 0;

        // Find the ID of the default Device.
        paramSize = sizeof(AudioDeviceID);
        SAFELY(AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &paramSize, &defaultDeviceID));
        if (err == noErr)
        {
          for (int i=0; i<totalDeviceCount; i++)
          {
            PlexAudioDevicePtr audioDevice = PlexAudioDevicePtr(new PlexAudioDevice(pDevices[i]));
            
            // Add valid devices that support output.
            if (audioDevice->isValid() && audioDevice->hasOutput())
            {
              audioDevices->m_audioDevices.push_back(audioDevice);
  
              // Set the default device.
              if (defaultDeviceID == pDevices[i])
                audioDevices->m_defaultDevice = audioDevice;
               
              // Set the selected device.
              if (g_guiSettings.GetSetting("audiooutput.audiodevice") && g_guiSettings.GetString("audiooutput.audiodevice") == audioDevice->getName())
                audioDevices->g_selectedDevice = audioDevice;
            }
          }
           
          // If we haven't selected any devices, select the default one.
          if (!audioDevices->g_selectedDevice)
            audioDevices->g_selectedDevice = audioDevices->m_defaultDevice;
          
          // Check if the selected device is alive and usable.
          if (audioDevices->g_selectedDevice->isAlive() == false)
          {
            CLog::Log(LOGWARNING, "Selected audio device is not alive, switching to default device.");
            audioDevices->g_selectedDevice = audioDevices->m_defaultDevice;
          }
        }
      }
      
      free(pDevices);
    }
  }
  
  // FIXME? Attach a Listener so that we are notified of a change in the Device setup.
  // AudioHardwareAddPropertyListener( kAudioHardwarePropertyDevices, HardwareListener, (void *)p_aout);

  return PlexAudioDevicesPtr(audioDevices);
}

///////////////////////////////////////////////////////////////////////////////
PlexAudioDevicePtr PlexAudioDevices::FindByName(const string& audioDeviceName)
{
  PlexAudioDevicesPtr allDevices = FindAll();
  
  // Find the matching device.
  BOOST_FOREACH(PlexAudioDevicePtr device, allDevices->getDevices())
    if (device->getName() == audioDeviceName)
      return device;
  
  return PlexAudioDevicePtr();
}

///////////////////////////////////////////////////////////////////////////////
PlexAudioDevicePtr PlexAudioDevices::FindDefault()
{
  PlexAudioDevice* ret = 0;
  AudioDeviceID    defaultDeviceID = 0;
  UInt32           paramSize = 0;
  OSStatus         err = noErr;

  // Find the ID of the default Device.
  paramSize = sizeof(AudioDeviceID);
  SAFELY(AudioHardwareGetProperty(kAudioHardwarePropertyDefaultOutputDevice, &paramSize, &defaultDeviceID));
  if (err == noErr)
    ret = new PlexAudioDevice(defaultDeviceID);

  return PlexAudioDevicePtr(ret);
}

