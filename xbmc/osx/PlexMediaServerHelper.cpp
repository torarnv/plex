/*
 *  Created on: Nov 8, 2008
 *      Author: Elan Feingold
 */
#include "PlexMediaServerHelper.h"
#include "PlatformDefs.h"
#include "log.h"
#include "system.h"
#include "Settings.h"

PlexMediaServerHelper* PlexMediaServerHelper::_theInstance = 0;

/////////////////////////////////////////////////////////////////////////////
bool PlexMediaServerHelper::DoConfigure(int& mode, bool& alwaysRunning, bool& errorStarting)
{
  bool changed = false;
  
  mode = g_guiSettings.GetInt("plexmediaserver.mode");
  alwaysRunning = g_guiSettings.GetInt("plexmediaserver.alwayson");
  
  return changed;
}

/////////////////////////////////////////////////////////////////////////////
string PlexMediaServerHelper::GetConfigString()
{
  return "";
}
