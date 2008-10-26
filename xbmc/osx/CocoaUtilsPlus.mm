//
//  CocoaUtilsPlus.MM
//  Plex
//
//  Created by Enrique Osuna on 10/26/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#include "CocoaUtilsPlus.h"
#include <Cocoa/Cocoa.h>

vector<string> Cocoa_GetSystemFonts()
{
  NSArray *fonts = [[[NSFontManager sharedFontManager] availableFonts] sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)];
  vector<string> result;

  for (int i = 0; i < [fonts count]; i++)
  {
    NSFont *font = [NSFont fontWithName:[fonts objectAtIndex:i] size:12.0];
    result.push_back([[font displayName] cStringUsingEncoding:NSUTF8StringEncoding]);
  }
  return result;
}
