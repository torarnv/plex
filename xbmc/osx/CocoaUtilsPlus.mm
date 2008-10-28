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
  vector<string> result;

  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  @try
  {
    NSArray *fonts = [[[NSFontManager sharedFontManager] availableFontFamilies] sortedArrayUsingSelector:@selector(caseInsensitiveCompare:)];
    for (int i = 0; i < [fonts count]; i++)
    {
      result.push_back([[fonts objectAtIndex:i] cStringUsingEncoding:NSUTF8StringEncoding]);
    }
  }
  @finally
  {
    [pool drain];
  }
  return result;
}

string Cocoa_GetSystemFontPathFromDisplayName(const string displayName)
{
  CFURLRef fontFileURL = NULL;
  CFStringRef fontName = CFStringCreateWithCString(kCFAllocatorDefault, displayName.c_str(), kCFStringEncodingUTF8);
  if (!fontName)
    return displayName;

  @try
  {
    ATSFontRef fontRef = ATSFontFindFromName(fontName, kATSOptionFlagsDefault);
    if (!fontRef)
      return displayName;

    FSRef fontFileRef;
    if (ATSFontGetFileReference(fontRef, &fontFileRef) != noErr)
      return displayName;

    fontFileURL = CFURLCreateFromFSRef(kCFAllocatorDefault, &fontFileRef);
    return [[(NSURL *)fontFileURL path] cStringUsingEncoding:NSUTF8StringEncoding];
  }
  @finally
  {
    if (fontName)
      CFRelease(fontName);
    if (fontFileURL)
      CFRelease(fontFileURL);
  }
  return displayName;
}
