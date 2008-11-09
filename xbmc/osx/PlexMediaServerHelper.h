#pragma once

/*
 *  Created on: Nov 8, 2008
 *      Author: Elan Feingold
 */
#include "PlexHelperApp.h"

class PlexMediaServerHelper : public PlexHelperApp
{
 public:
   
  /// Singleton method.
  static PlexMediaServerHelper& Get()
  {
    if (_theInstance == 0)
    {
      _theInstance = new PlexMediaServerHelper();
      _theInstance->Initialize();
    }
    
    return *_theInstance;
  }
  
 protected:
   
  PlexMediaServerHelper() {}
  
  virtual bool   DoConfigure(int& mode, bool& alwaysRunning, bool& errorStarting);
  virtual string GetConfigString();
  
  virtual string GetHelperBinaryName() { return "PlexMediaServer"; }
  virtual string GetPlistName() { return "com.plexapp.mediaserver.plist"; }
  
 private:
   
  static PlexMediaServerHelper* _theInstance;
};
