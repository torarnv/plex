#pragma once

/*
 *  CoreAudioPlexSupport.h
 *  Plex
 *
 *  Created by Ryan Walklin on 9/7/08.
 *  Copyright 2008 Ryan Walklin. All rights reserved.
 *
 */

#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>
#include <CoreAudio/AudioHardware.h>

using namespace std;

#define STREAM_FORMAT_MSG( pre, sfm ) \
  pre "[%ld][%4.4s][%ld][%ld][%ld][%ld][%ld][%ld]", \
  (UInt32)sfm.mSampleRate, (char *)&sfm.mFormatID, \
  sfm.mFormatFlags, sfm.mBytesPerPacket, \
  sfm.mFramesPerPacket, sfm.mBytesPerFrame, \
  sfm.mChannelsPerFrame, sfm.mBitsPerChannel

#define STREAM_FORMAT_MSG_FULL( pre, sfm ) \
  pre ":\nsamplerate: [%ld]\nFormatID: [%4.4s]\nFormatFlags: [%ld]\nBytesPerPacket: [%ld]\nFramesPerPacket: [%ld]\nBytesPerFrame: [%ld]\nChannelsPerFrame: [%ld]\nBitsPerChannel[%ld]", \
  (UInt32)sfm.mSampleRate, (char *)&sfm.mFormatID, \
  sfm.mFormatFlags, sfm.mBytesPerPacket, \
  sfm.mFramesPerPacket, sfm.mBytesPerFrame, \
  sfm.mChannelsPerFrame, sfm.mBitsPerChannel

///////////////////////////////////////////////////////////////////////////////
class PlexAudioDevice 
{
  friend class PlexAudioDevices;
  
 public:
  
  PlexAudioDevice(AudioDeviceID deviceID);
  ~PlexAudioDevice() {}
   
  bool          isValid()         const { return m_isValid; }
  const string& getName()         const { return m_deviceName; }
  bool          supportsDigital() const { return m_supportsDigital; }
  bool          hasOutput()       const { return m_hasOutput; }
  AudioDeviceID getDeviceID()     const { return m_deviceID; }
  bool          isAlive();
  
  /// Set the device to be the default.
  void setDefault();
  
 private:
  
  bool computeHasOutput();
  bool computeDeviceSupportsDigital();
  bool computeStreamSupportsDigital(AudioStreamID streamID);
   
  bool          m_hasOutput;
  bool          m_isValid;
  string        m_deviceName;
	AudioDeviceID m_deviceID;
	bool          m_supportsDigital;
};

typedef boost::shared_ptr<PlexAudioDevice> PlexAudioDevicePtr;

///////////////////////////////////////////////////////////////////////////////
class PlexAudioDevices
{
 public:
   
   PlexAudioDevicePtr          getDefaultDevice()  { return m_defaultDevice; }
   PlexAudioDevicePtr          getSelectedDevice() { return m_selectedDevice; }
   vector<PlexAudioDevicePtr>& getDevices()        { return m_audioDevices; }
   
   /// Get all devices.
   static boost::shared_ptr<PlexAudioDevices> FindAll();
   
   /// Look up ID by name.
   static PlexAudioDevicePtr FindByName(const string& audioDeviceName);
   
   /// Get the default device.
   static PlexAudioDevicePtr FindDefault();
   
 protected:
   
  PlexAudioDevices() {}

 private:
   
  vector<PlexAudioDevicePtr> m_audioDevices;
  PlexAudioDevicePtr         m_defaultDevice;
  PlexAudioDevicePtr         m_selectedDevice;
};

typedef boost::shared_ptr<PlexAudioDevices> PlexAudioDevicesPtr;

typedef int64_t mtime_t;

