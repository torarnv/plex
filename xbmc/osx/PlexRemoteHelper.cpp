/*
 *  Created on: Nov 8, 2008
 *      Author: Elan Feingold
 */
#include <mach-o/dyld.h>
 
#include "PlexRemoteHelper.h"
#include "PlatformDefs.h"
#include "log.h"
#include "system.h"
#include "Settings.h"

#define SOFA_CONTROL_PROGRAM "Sofa Control"

PlexRemoteHelper* PlexRemoteHelper::_theInstance = 0;

/////////////////////////////////////////////////////////////////////////////
bool PlexRemoteHelper::DoConfigure(int& mode, bool& alwaysRunning, bool& errorStarting)
{
  bool changed = false;

  int oldDelay = m_sequenceDelay;
  int oldSecureInput = m_secureInput;
  int oldMode = mode;
  
  mode = g_guiSettings.GetInt("appleremote.mode");
  alwaysRunning = g_guiSettings.GetInt("appleremote.alwayson");
  m_sequenceDelay = g_guiSettings.GetInt("appleremote.sequencetime");
  m_secureInput = g_guiSettings.GetBool("appleremote.secureinput");

  // Don't let it enable if sofa control is running.
  if (IsSofaControlRunning())
  {
    // If we were starting then remember error.
    if (oldMode == MODE_DISABLED && mode != MODE_DISABLED)
      errorStarting = true;

    mode = MODE_DISABLED;
    g_guiSettings.SetInt("appleremote.mode", mode);
  }
  
  // See if anything changed.
  if (oldDelay != m_sequenceDelay || oldSecureInput != m_secureInput)
    changed = true;
  
  return changed;
}

/////////////////////////////////////////////////////////////////////////////
string PlexRemoteHelper::GetConfigString()
{
  // Build a new config string.
  std::string strConfig;
  if (GetMode() == APPLE_REMOTE_UNIVERSAL)
    strConfig = "--universal ";

  // Delay.
  char strDelay[64];
  sprintf(strDelay, "--timeout %d ", m_sequenceDelay);
  strConfig += strDelay;

  // Secure input.
  char strSecure[64];
  sprintf(strSecure, "--secureInput %d ", m_secureInput ? 1 : 0);
  strConfig += strSecure;
  
  // Find out where we're running from.
  char     given_path[2*MAXPATHLEN];
  uint32_t path_size = 2*MAXPATHLEN;

  int result = _NSGetExecutablePath(given_path, &path_size);
  if (result == 0)
  {
    char real_path[2*MAXPATHLEN];
    if (realpath(given_path, real_path) != NULL)
    {
      // Move backwards out to the application.
      for (int x=0; x<4; x++)
      {
        for (int n=strlen(real_path)-1; real_path[n] != '/'; n--)
          real_path[n] = '\0';
      
        real_path[strlen(real_path)-1] = '\0';
      }
    }
    
    strConfig += "--appLocation \"";
    strConfig += real_path;
    strConfig += "\"";
  }
  
  return strConfig + "\n";
}

/////////////////////////////////////////////////////////////////////////////
bool PlexRemoteHelper::IsSofaControlRunning()
{
  // Check for a "Sofa Control" process running.
  return GetProcessPid(SOFA_CONTROL_PROGRAM) != -1;
}

