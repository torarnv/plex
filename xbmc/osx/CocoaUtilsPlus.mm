//
//  CocoaUtilsPlus.MM
//  Plex
//
//  Created by Enrique Osuna on 10/26/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//
#include "CocoaUtilsPlus.h"
#include "XBMCMain.h"
#include "MediaSource.h"
#include <Cocoa/Cocoa.h>
#include <CoreServices/CoreServices.h>
#import <SystemConfiguration/SystemConfiguration.h>
#include <arpa/inet.h>

#define COCOA_KEY_PLAYPAUSE  1051136
#define COCOA_KEY_PREV_TRACK 1313280
#define COCOA_KEY_NEXT_TRACK 1248000

///////////////////////////////////////////////////////////////////////////////
CGEventRef tapEventCallback(CGEventTapProxy proxy, CGEventType type, CGEventRef event, void *refcon)
{
  // 1051136
  NSEvent* nsEvent = [NSEvent eventWithCGEvent:event];
  NSInteger data = [nsEvent data1];
  
  printf( "Got event of type %d - data1 %i \n", type, data);
  
  if(data!=1051136)
    return event;
  else
    return NULL;
}

///////////////////////////////////////////////////////////////////////////////
void CocoaPlus_Initialize()
{
#if 0
  // kCGHeadInsertEventTap - can't use this because stopping in debugger/hang means nobody gets it!
  CFMachPortRef eventPort = CGEventTapCreate(kCGSessionEventTap, kCGTailAppendEventTap, kCGEventTapOptionDefault, CGEventMaskBit(NX_SYSDEFINED), tapEventCallback, NULL);
  if (eventPort == 0)
    NSLog(@"No event port available.");
      
  CFRunLoopSourceRef eventSrc = CFMachPortCreateRunLoopSource(NULL, eventPort, 0);
  if (eventSrc == 0)
    NSLog(@"No event run loop ");
      
  CFRunLoopRef runLoop = CFRunLoopGetCurrent();
  if ( runLoop == NULL)
    NSLog(@"No run loop for tap callback.");
      
  CFRunLoopAddSource(runLoop,  eventSrc, kCFRunLoopDefaultMode);
#endif
}

///////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////
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

///////////////////////////////////////////////////////////////////////////////
vector<in_addr_t> Cocoa_AddressesForHost(const string hostname)
{
  NSHost *host = [NSHost hostWithName:[NSString stringWithCString:hostname.c_str()]];
  vector<in_addr_t> ret;
  for (NSString *address in [host addresses])
    ret.push_back(inet_addr([address UTF8String]));
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
bool Cocoa_AreHostsEqual(const string host1, const string host2)
{
  NSHost *h1 = [NSHost hostWithName:[NSString stringWithCString:host1.c_str()]];
  NSHost *h2 = [NSHost hostWithName:[NSString stringWithCString:host2.c_str()]];
  return ([h1 isEqualToHost:h2] || ([h1 isEqualToHost:[NSHost currentHost]] && [h2 isEqualToHost:[NSHost currentHost]]));
}

///////////////////////////////////////////////////////////////////////////////
VECSOURCES Cocoa_GetPlexMediaServersAsSourcesWithMediaType(const string mediaType)
{
  VECSOURCES ret;
  @synchronized([[XBMCMain sharedInstance] plexMediaServers])
  {
    for (NSNetService *server in [[XBMCMain sharedInstance] plexMediaServers])
    {
      // Server has not been resolved
      if ([[server addresses] count] == 0)
        continue;

      CMediaSource source;
      source.strName = [[server name] UTF8String];
      source.strPath.Format("plex://%s:%d/%s", [[server hostName] UTF8String], [server port], mediaType);
      ret.push_back(source);
    }
  }
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
vector<CStdString> Cocoa_Proxy_ExceptionList()
{
  vector<CStdString> ret;
  NSDictionary* proxyDict = (NSDictionary*)SCDynamicStoreCopyProxies(NULL);
  @try
  {
    for (id exceptionItem in [proxyDict objectForKey:(id)kSCPropNetProxiesExceptionsList])
      ret.push_back(CStdString([exceptionItem UTF8String]));
  }
  @finally
  {
    [proxyDict release];
  }
  return ret;
}
