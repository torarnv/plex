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
    CLog::Log(LOGERROR, "PlexAudioDevice: Error %4.4s doing %s", (char* )&err, #expr); \
  }

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
    SAFELY(AudioDeviceGetProperty(deviceID, 0, false, kAudioDevicePropertyDeviceName, &paramSize, pStrName));
    if (err == noErr)
    {
      CLog::Log(LOGDEBUG, "DevID: %p DevName: %s", deviceID, pStrName);
      m_deviceName = pStrName;
      
      // See if the device is writable (can output).
      m_hasOutput = computeHasOutput();
      
      // If the device does have output, see if it supports digital.
      if (m_hasOutput)
        m_supportsDigital = computeDeviceSupportsDigital();
      else
        CLog::Log(LOGDEBUG, "Skipping input-only device %s", m_deviceName.c_str());
      
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
        CLog::Log(LOGDEBUG, STREAM_FORMAT_MSG(" * Supported format: ", pFormatList[i]));
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
                audioDevices->m_selectedDevice = audioDevice;
            }
          }
           
          // If we haven't selected any devices, select the default one.
          if (!audioDevices->m_selectedDevice)
            audioDevices->m_selectedDevice = audioDevices->m_defaultDevice;
          
          // Check if the selected device is alive and usable.
          if (audioDevices->m_selectedDevice->isAlive() == false)
          {
            CLog::Log(LOGWARNING, "Selected audio device [%s] is not alive, switching to default device", audioDevices->m_selectedDevice->getName().c_str());
            audioDevices->m_selectedDevice = audioDevices->m_defaultDevice;
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

#if 0

/**
 * Return high precision date
 *
 * Use a 1 MHz clock when possible, or 1 kHz
 *
 * Beware ! It doesn't reflect the actual date (since epoch), but can be the machine's uptime or anything (when monotonic clock is used)
 */
mtime_t CoreAudioPlexSupport::mdate()
{
    mtime_t res;

#if defined (HAVE_CLOCK_NANOSLEEP)
    struct timespec ts;

    /* Try to use POSIX monotonic clock if available */
    if( clock_gettime( CLOCK_MONOTONIC, &ts ) == EINVAL )
	/* Run-time fallback to real-time clock (always available) */
        (void)clock_gettime( CLOCK_REALTIME, &ts );

    res = ((mtime_t)ts.tv_sec * (mtime_t)1000000)
	+ (mtime_t)(ts.tv_nsec / 1000);

#elif defined( HAVE_KERNEL_OS_H )
    res = real_time_clock_usecs();

#elif defined( WIN32 ) || defined( UNDER_CE )
    /* We don't need the real date, just the value of a high precision timer */
    static mtime_t freq = INT64_C(-1);

    if( freq == INT64_C(-1) )
    {
        /* Extract from the Tcl source code:
         * (http://www.cs.man.ac.uk/fellowsd-bin/TIP/7.html)
         *
         * Some hardware abstraction layers use the CPU clock
         * in place of the real-time clock as a performance counter
         * reference.  This results in:
         *    - inconsistent results among the processors on
         *      multi-processor systems.
         *    - unpredictable changes in performance counter frequency
         *      on "gearshift" processors such as Transmeta and
         *      SpeedStep.
         * There seems to be no way to test whether the performance
         * counter is reliable, but a useful heuristic is that
         * if its frequency is 1.193182 MHz or 3.579545 MHz, it's
         * derived from a colorburst crystal and is therefore
         * the RTC rather than the TSC.  If it's anything else, we
         * presume that the performance counter is unreliable.
         */
        LARGE_INTEGER buf;

        freq = ( QueryPerformanceFrequency( &buf ) &&
				(buf.QuadPart == INT64_C(1193182) || buf.QuadPart == INT64_C(3579545) ) )
		? buf.QuadPart : 0;

#if defined( WIN32 )
        /* on windows 2000, XP and Vista detect if there are two
		 cores there - that makes QueryPerformanceFrequency in
		 any case not trustable?
		 (may also be true, for single cores with adaptive
		 CPU frequency and active power management?)
		 */
        HINSTANCE h_Kernel32 = LoadLibrary(_T("kernel32.dll"));
        if(h_Kernel32)
        {
            void WINAPI (*pf_GetSystemInfo)(LPSYSTEM_INFO);
            pf_GetSystemInfo = (void WINAPI (*)(LPSYSTEM_INFO))
			GetProcAddress(h_Kernel32, _T("GetSystemInfo"));
            if(pf_GetSystemInfo)
            {
				SYSTEM_INFO system_info;
				pf_GetSystemInfo(&system_info);
				if(system_info.dwNumberOfProcessors > 1)
					freq = 0;
            }
            FreeLibrary(h_Kernel32);
        }
#endif
    }

    if( freq != 0 )
    {
        LARGE_INTEGER counter;
        QueryPerformanceCounter (&counter);

        /* Convert to from (1/freq) to microsecond resolution */
        /* We need to split the division to avoid 63-bits overflow */
        lldiv_t d = lldiv (counter.QuadPart, freq);

        res = (d.quot * 1000000) + ((d.rem * 1000000) / freq);
    }
    else
    {
        /* Fallback on timeGetTime() which has a millisecond resolution
         * (actually, best case is about 5 ms resolution)
         * timeGetTime() only returns a DWORD thus will wrap after
         * about 49.7 days so we try to detect the wrapping. */

        static CRITICAL_SECTION date_lock;
        static mtime_t i_previous_time = INT64_C(-1);
        static int i_wrap_counts = -1;

        if( i_wrap_counts == -1 )
        {
            /* Initialization */
#if defined( WIN32 )
            i_previous_time = INT64_C(1000) * timeGetTime();
#else
            i_previous_time = INT64_C(1000) * GetTickCount();
#endif
            InitializeCriticalSection( &date_lock );
            i_wrap_counts = 0;
        }

        EnterCriticalSection( &date_lock );
#if defined( WIN32 )
        res = INT64_C(1000) *
		(i_wrap_counts * INT64_C(0x100000000) + timeGetTime());
#else
        res = INT64_C(1000) *
		(i_wrap_counts * INT64_C(0x100000000) + GetTickCount());
#endif
        if( i_previous_time > res )
        {
            /* Counter wrapped */
            i_wrap_counts++;
            res += INT64_C(0x100000000) * 1000;
        }
        i_previous_time = res;
        LeaveCriticalSection( &date_lock );
    }
#else
    struct timeval tv_date;

    /* gettimeofday() cannot fail given &tv_date is a valid address */
    (void)gettimeofday( &tv_date, NULL );
    res = (mtime_t) tv_date.tv_sec * 1000000 + (mtime_t) tv_date.tv_usec;
#endif

    return res;
}
#endif
