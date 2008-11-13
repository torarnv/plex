#pragma once

/*
 *  Created on: Nov 8, 2008
 *      Author: Elan Feingold
 */
#include <string>

using namespace std;

class PlexHelperApp
{
 public:
   
  enum { MODE_DISABLED=0, MODE_ENABLED=1 };
   
  void Start();
  void Stop(bool hup = false);
  void Configure();
  void Install();
  void Uninstall();

  bool IsAlwaysOn() const { return m_alwaysOn; }
  bool ErrorStarting()    { return m_errorStarting; }
  int  GetMode() const    { return m_mode; }

  /// Fill-ins for subclasses.
  virtual string GetHelperBinaryName() = 0;
  virtual string GetPlistName() = 0;
  
  virtual bool   DoConfigure(int& mode, bool& alwaysRunning, bool& errorStarting) = 0;
  virtual string GetConfigString() = 0;
  
  static int GetProcessPid(const string& processName);
  static string ReadFile(const string& fileName);
  static void WriteFile(const string& fileName, const string& data);
  
 protected:

  /// Constructor/destructor.
  PlexHelperApp();
  virtual ~PlexHelperApp() {}
  
  /// Initialization.
  void Initialize();
    
 private:

  bool EnsureLatestHelperInstalled();
  std::string GetInstalledHelperVersion();
  
  int  m_mode;
  bool m_alwaysOn;
  bool m_errorStarting;

  std::string m_configFile;
  std::string m_launchAgentLocalFile;
  std::string m_launchAgentInstallFile;
  std::string m_helperFile;
  std::string m_helperInstalledFile;
  std::string m_helperInstalledVersionFile;
};
