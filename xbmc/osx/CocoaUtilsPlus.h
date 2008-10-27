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

using namespace std;

#ifdef __cplusplus
extern "C"
{
#endif
  //
  // Fonts
  //
  vector<string> Cocoa_GetSystemFonts();
  string Cocoa_GetSystemFontPathFromDisplayName(const string displayName);

#ifdef __cplusplus
}
#endif

#endif
