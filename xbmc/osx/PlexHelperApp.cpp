#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <assert.h>
#include <mach-o/dyld.h>
#include <errno.h>
#include <signal.h>
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

#include "PlexHelperApp.h"

#include "CocoaUtils.h"
#include "PlatformDefs.h"
#include "log.h"
#include "system.h"
#include "Settings.h"
#include "Util.h"
#include "XFileUtils.h"

#define RESOURCES_DIR "/Library/Application Support/Plex"

static int GetBSDProcessList(kinfo_proc **procList, size_t *procCount);

/////////////////////////////////////////////////////////////////////////////
PlexHelperApp::PlexHelperApp()
  : m_errorStarting(false)
  , m_alwaysOn(false)
{
}

/////////////////////////////////////////////////////////////////////////////
void PlexHelperApp::Initialize()
{
  CStdString homePath;
  CUtil::GetHomePath(homePath);

  // Compute the helper filename.
  m_helperFile = string(homePath) + string("/") + GetHelperBinaryName();
  
  // Compute the local (pristine) launch agent filename.
  m_launchAgentLocalFile = string(homePath) + string("/") + GetPlistName();
  
  // Compute the install path for the launch agent.
  m_launchAgentInstallFile = getenv("HOME");
  m_launchAgentInstallFile += "/Library/LaunchAgents/" + GetPlistName();
  
  // Compute the configuration file name.
  m_configFile = getenv("HOME");
  m_configFile += RESOURCES_DIR "/" + GetHelperBinaryName() + ".conf";
  
  // This is where we install the helper.
  m_helperInstalledFile = getenv("HOME");
  m_helperInstalledFile += RESOURCES_DIR "/" + GetHelperBinaryName();
  
  // This is where we store the installed version of helper.
  m_helperInstalledVersionFile = getenv("HOME");
  m_helperInstalledVersionFile += RESOURCES_DIR "/" + GetHelperBinaryName() + ".version"; 
}

/////////////////////////////////////////////////////////////////////////////
bool PlexHelperApp::EnsureLatestHelperInstalled()
{
  std::string appVersion = Cocoa_GetAppVersion();
  std::string installedVersion = GetInstalledHelperVersion();

  if (/* appVersion.length() > 0 && */ appVersion != installedVersion ||
      ::access(m_helperInstalledFile.c_str(), R_OK) != 0)
  {
    CLog::Log(LOGNOTICE, "Detected change in %s helper version, it was '%s,' and application version was '%s'.\n", 
              GetHelperBinaryName().c_str(), installedVersion.c_str(), appVersion.c_str());

    // Whack old helper.
    DeleteFile(m_helperInstalledFile.c_str());

    // Copy helper and ensure executable.
    CopyFile(m_helperFile.c_str(), m_helperInstalledFile.c_str(), FALSE);
    chmod(m_helperInstalledFile.c_str(), S_IRWXU | S_IRGRP | S_IROTH);

    // Write version file.
    WriteFile(m_helperInstalledVersionFile.c_str(), appVersion);
    return true;
  }

  return false;
}

/////////////////////////////////////////////////////////////////////////////
void PlexHelperApp::Start()
{
  if (GetProcessPid(GetHelperBinaryName()) == -1)
  {
    string cmd = "\"" + m_helperInstalledFile + "\" -x &";
    CLog::Log(LOGNOTICE, "Asking %s to start [%s]", GetHelperBinaryName().c_str(), cmd.c_str());
    system(cmd.c_str());
  }
}

/////////////////////////////////////////////////////////////////////////////
void PlexHelperApp::Stop(bool hup)
{
  // Kill the process.
  int pid = GetProcessPid(GetHelperBinaryName());
  if (pid != -1)
  {
    CLog::Log(LOGNOTICE, "Asking %s to %s.", GetHelperBinaryName().c_str(), hup ? "reconfigure" : "stop");
    kill(pid, (hup ? SIGHUP : SIGKILL));
  }
  else
  {
    CLog::Log(LOGNOTICE, "%s is not running.", GetHelperBinaryName().c_str());
  }
}

/////////////////////////////////////////////////////////////////////////////
void PlexHelperApp::Configure()
{
  int oldMode = m_mode;
  int oldAlwaysOn = m_alwaysOn;

  // Assume success.
  m_errorStarting = false;
  
  // Ask the subclass to do its configuration.
  bool changed = DoConfigure(m_mode, m_alwaysOn, m_errorStarting);

  // New configuration.
  if (oldMode != m_mode || changed)
  {
    // Write the new configuration.
    WriteFile(m_configFile.c_str(), GetConfigString());

    // If process is running, kill -HUP to have it reload settings.
    Stop(TRUE);
  }

  // Make sure latest helper is installed.
  if (EnsureLatestHelperInstalled() == true)
  {
    CLog::Log(LOGNOTICE, "Version of %s changed, uninstalling and stopping.", GetHelperBinaryName().c_str());

    // Things changed. Uninstall and stop.
    Uninstall();
    Stop();

    // Make sure we reinstall/start if needed.
    oldMode = MODE_DISABLED;
    oldAlwaysOn = false;
  }

  // Turning off?
  if (oldMode != MODE_DISABLED && m_mode == MODE_DISABLED)
  {
    Stop();
    Uninstall();
  }

  // Turning on.
  if (oldMode == MODE_DISABLED && m_mode != MODE_DISABLED)
    Start();

  // Installation/uninstallation.
  if (oldAlwaysOn == false && m_alwaysOn == true)
    Install();
  if (oldAlwaysOn == true && m_alwaysOn == false)
    Uninstall();
}

/////////////////////////////////////////////////////////////////////////////
std::string PlexHelperApp::GetInstalledHelperVersion()
{
  if (::access(m_helperInstalledVersionFile.c_str(), R_OK) == 0)
    return ReadFile(m_helperInstalledVersionFile.c_str());
  else
    return "???";
}

/////////////////////////////////////////////////////////////////////////////
void PlexHelperApp::Install()
{
  if (::access(m_launchAgentLocalFile.c_str(), R_OK) == 0 && // Launch agent template exists
      ::access(m_launchAgentInstallFile.c_str(), R_OK) != 0) // Not already installed.
  {
    // Make sure directory exists.
    string strDir = getenv("HOME");
    strDir += "/Library/LaunchAgents";
    CreateDirectory(strDir.c_str(), NULL);

    // Load template.
    string plistData = ReadFile(m_launchAgentLocalFile.c_str());

    if (plistData != "")
    {
      CLog::Log(LOGNOTICE, "Installing %s at %s", GetHelperBinaryName().c_str(), m_launchAgentInstallFile.c_str());

      // Replace it in the file.
      int start = plistData.find("${PATH}");
      plistData.replace(start, 7, m_helperInstalledFile.c_str(), m_helperInstalledFile.length());

      // Install it.
      WriteFile(m_launchAgentInstallFile.c_str(), plistData);

      // Load it.
      string cmd = "/bin/launchctl load ";
      cmd += m_launchAgentInstallFile;
      system(cmd.c_str());
    }
    else
    {
      CLog::Log(LOGERROR, "Unable to install \"%s\". Launch agent template (%s) is corrupted.",
                GetHelperBinaryName().c_str(), m_launchAgentLocalFile.c_str());
    }
  }
  else
  {
    if (::access(m_launchAgentLocalFile.c_str(), R_OK) != 0)
      CLog::Log(LOGERROR, "Unable to install \"%s\". Unable to find launch agent template (%s).",
                 GetHelperBinaryName().c_str(), m_launchAgentLocalFile.c_str());
  }
}

/////////////////////////////////////////////////////////////////////////////
void PlexHelperApp::Uninstall()
{
  if (::access(m_launchAgentInstallFile.c_str(), R_OK) == 0)
  {
    CLog::Log(LOGNOTICE, "Uninstalling %s from %s.", GetHelperBinaryName().c_str(), m_launchAgentInstallFile.c_str());

    // Call the unloader.
    string cmd = "/bin/launchctl unload ";
    cmd += m_launchAgentInstallFile;
    system(cmd.c_str());

    // Remove the plist file.
    DeleteFile(m_launchAgentInstallFile.c_str());
  }
  else
  {
    CLog::Log(LOGNOTICE, "Asked to uninstall %s, but it wasn't installed.", GetHelperBinaryName().c_str());
  }
}

/////////////////////////////////////////////////////////////////////////////
std::string PlexHelperApp::ReadFile(const string& fileName)
{
  ifstream is;
  is.open(fileName.c_str());
  if (!is.is_open())
    return "";

  // Get length of file:
  is.seekg (0, ios::end);
  int length = is.tellg();
  is.seekg (0, ios::beg);

  // Allocate memory:
  char* buffer = new char [length+1];

  // Read data as a block:
  is.read(buffer,length);
  is.close();
  buffer[length] = '\0';

  std::string ret = buffer;
  delete[] buffer;
  return ret;
}

/////////////////////////////////////////////////////////////////////////////
void PlexHelperApp::WriteFile(const string& fileName, const std::string& data)
{
  ofstream out(fileName.c_str());
  if (!out)
  {
    CLog::Log(LOGERROR, "PlexHelper: Unable to open file '%s'", fileName);
  }
  else
  {
    // Write new configuration.
    out << data;
    out.flush();
    out.close();
  }
}

/////////////////////////////////////////////////////////////////////////////
extern "C" int GetProcessPid(const char* processName)
{
  return PlexHelperApp::GetProcessPid(processName);
}

/////////////////////////////////////////////////////////////////////////////
int PlexHelperApp::GetProcessPid(const string& strProgram)
{
  kinfo_proc* mylist;
  size_t mycount = 0;
  int ret = -1;

  GetBSDProcessList(&mylist, &mycount);
  for (size_t k = 0; k < mycount && ret == -1; k++)
  {
    kinfo_proc *proc = NULL;
    proc = &mylist[k];
    
    if (strProgram == proc->kp_proc.p_comm)
    {
      //if (ignorePid == 0 || ignorePid != proc->kp_proc.p_pid)
      ret = proc->kp_proc.p_pid;
    }

  }

  free (mylist);

  return ret;
}

typedef struct kinfo_proc kinfo_proc;

// Returns a list of all BSD processes on the system.  This routine
// allocates the list and puts it in *procList and a count of the
// number of entries in *procCount.  You are responsible for freeing
// this list (use "free" from System framework).
// On success, the function returns 0.
// On error, the function returns a BSD errno value.
//
static int GetBSDProcessList(kinfo_proc **procList, size_t *procCount)
{
  int err;
  kinfo_proc * result;
  bool done;
  static const int name[] =
  { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };

  // Declaring name as const requires us to cast it when passing it to
  // sysctl because the prototype doesn't include the const modifier.
  size_t length;

  assert(procList != NULL);
  assert(procCount != NULL);

  *procCount = 0;

  // We start by calling sysctl with result == NULL and length == 0.
  // That will succeed, and set length to the appropriate length.
  // We then allocate a buffer of that size and call sysctl again
  // with that buffer.  If that succeeds, we're done.  If that fails
  // with ENOMEM, we have to throw away our buffer and loop.  Note
  // that the loop causes use to call sysctl with NULL again; this
  // is necessary because the ENOMEM failure case sets length to
  // the amount of data returned, not the amount of data that
  // could have been returned.
  //
  result = NULL;
  done = false;
  do
  {
    assert(result == NULL);

    // Call sysctl with a NULL buffer.
    length = 0;
    err = sysctl((int *) name, (sizeof(name) / sizeof(*name)) - 1, NULL,
        &length, NULL, 0);
    if (err == -1)
      err = errno;

    // Allocate an appropriately sized buffer based on the results from the previous call.
    if (err == 0)
    {
      result = (kinfo_proc*) malloc(length);
      if (result == NULL)
        err = ENOMEM;
    }

    // Call sysctl again with the new buffer.  If we get an ENOMEM
    // error, toss away our buffer and start again.
    //
    if (err == 0)
    {
      err = sysctl((int *) name, (sizeof(name) / sizeof(*name)) - 1, result,
          &length, NULL, 0);

      if (err == -1)
        err = errno;
      else if (err == 0)
        done = true;
      else if (err == ENOMEM)
      {
        assert(result != NULL);
        free(result);
        result = NULL;
        err = 0;
      }
    }
  } while (err == 0 && !done);

  // Clean up and establish post conditions.
  if (err != 0 && result != NULL)
  {
    free(result);
    result = NULL;
  }

  *procList = result;
  if (err == 0)
    *procCount = length / sizeof(kinfo_proc);

  assert( (err == 0) == (*procList != NULL) );
  return err;
}
