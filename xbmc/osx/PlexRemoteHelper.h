#pragma once

/*
 *  Created on: Nov 8, 2008
 *      Author: Elan Feingold
 */
#include "PlexHelperApp.h"
 
class PlexRemoteHelper : public PlexHelperApp
{
 public:

  /// Singleton method.
  static PlexRemoteHelper& Get()
  {
    if (_theInstance == 0)
    {
      _theInstance = new PlexRemoteHelper();
      _theInstance->Initialize();
    }
    
    return *_theInstance;
  }

  bool IsSofaControlRunning();

 protected:

  PlexRemoteHelper()
    : m_sequenceDelay(-1)
    , m_secureInput(-1)
  {}
   
  bool IsRemoteBuddyInstalled();
   
  virtual bool   DoConfigure(int& mode, bool& alwaysRunning, bool& errorStarting);
  virtual string GetConfigString();
  
  virtual string GetHelperBinaryName() { return "PlexHelper"; }
  virtual string GetPlistName() { return "com.plexapp.helper.plist"; }
  
 private:

  int  m_sequenceDelay;
  bool m_secureInput;
  
  static PlexRemoteHelper* _theInstance;
};
