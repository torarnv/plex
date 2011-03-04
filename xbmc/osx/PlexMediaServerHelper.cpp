/*
 *  Created on: Nov 8, 2008
 *      Author: Elan Feingold
 */
#include "File.h"
#include "PlexMediaServerHelper.h"
#include "PlatformDefs.h"
#include "log.h"
#include "system.h"
#include "Settings.h"

using namespace XFILE;

PlexMediaServerHelper* PlexMediaServerHelper::_theInstance = 0;

/////////////////////////////////////////////////////////////////////////////
bool PlexMediaServerHelper::DoConfigure(int& mode, bool& alwaysRunning, bool& errorStarting)
{
  bool changed = false;
  
  mode = g_guiSettings.GetInt("plexmediaserver.mode");
  alwaysRunning = g_guiSettings.GetBool("plexmediaserver.alwayson");
  
  return changed;
}

/////////////////////////////////////////////////////////////////////////////
string PlexMediaServerHelper::GetConfigString()
{
  return "";
}

/////////////////////////////////////////////////////////////////////////////
void PlexMediaServerHelper::InstallLatestVersion(const string& dstDir)
{
  string src = dstDir + "/Plex Media Server.app/Contents/Resources/Plex Plug-in Installer.app";
  string dst = dstDir + "/Plex Plug-in Installer.app";
  
  printf("From [%s] to [%s]\n", src.c_str(), dst.c_str());
  
  // Move over the latest plug-in installer.
  string rsync = "/usr/bin/rsync --delete -a \"" + src + "/\" \"" + dst + "\"";
  system(rsync.c_str());
}

#define FLASH_INFO_PLIST "/Library/Internet Plug-Ins/Flash Player.plugin/Contents/Info.plist"

/////////////////////////////////////////////////////////////////////////////
void PlexMediaServerHelper::RestartIfOldFlash()
{
  if (m_checkedForOldFlash == false)
  {
    CStdString file = FLASH_INFO_PLIST;
    bool exists = CFile::Exists(file);
    
    // Assume the worst.
    m_oldFlash = true;
    
    if (exists == false)
    {
      // Check the user version.
      file = getenv("HOME") + string(FLASH_INFO_PLIST);
      exists = CFile::Exists(file);
    }
      
    // If we found the file, then read it.
    if (exists)
    {
      string data = ReadFile(file);
      if (data.find("Flash 10.2") != string::npos)
        m_oldFlash = false;
        
      printf("Found the file!\n");
    }
    else
    {
      // If it doesn't exist, no need to restart!
      m_oldFlash = false;
    }
    
    m_checkedForOldFlash = true;
  }
  
  // Now see if we need to restart.
  if (m_oldFlash)
    Restart();
  else
    CLog::Log(LOGNOTICE, "Not restarting PMS because we're running new Flash.");
}
