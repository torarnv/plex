//
//  CocoaUtilsPlus.h
//  Plex
//
//  Created by Enrique Osuna on 10/26/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#ifndef _COCOA_UTILS_PLUS_H_
#define _COCOA_UTILS_PLUS_H_

#include <vector>
#include <string>
#include <sys/types.h>
#include "MediaSource.h"

using namespace std;

#ifdef __cplusplus
extern "C"
{
#endif
  //
  // Initialization
  //
  void CocoaPlus_Initialize();

  //
  // Fonts
  //
  vector<string> Cocoa_GetSystemFonts();
  string Cocoa_GetSystemFontPathFromDisplayName(const string displayName);

  //
  // Plex Media Server Services
  //
  vector<in_addr_t> Cocoa_AddressesForHost(const string hostname);
  bool Cocoa_AreHostsEqual(const string host1, const string host2);
  VECSOURCES Cocoa_GetPlexMediaServersAsSourcesWithMediaType(const string mediaType);

  //
  // Proxy Settings (continued)
  //
  vector<CStdString> Cocoa_Proxy_ExceptionList();
#ifdef __cplusplus
}
#endif

#endif
