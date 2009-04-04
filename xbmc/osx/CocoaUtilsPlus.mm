//
//  CocoaUtilsPlus.MM
//  Plex
//
//  Created by Enrique Osuna on 10/26/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//
#include "stdafx.h"
#include "CocoaUtilsPlus.h"
#include "XBMCMain.h"
#include "Settings.h"
#include "MediaSource.h"
#include <Cocoa/Cocoa.h>
#import <AddressBook/AddressBook.h>
#include <CoreFoundation/CFString.h>
#include <CoreServices/CoreServices.h>
#import <SystemConfiguration/SystemConfiguration.h>
#include <arpa/inet.h>
#include "PlexMediaServerHelper.h"

#include <boost/algorithm/string.hpp>

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

bool Cocoa_IsLocalPlexMediaServerRunning()
{
#if 1
  
  bool isRunning = PlexMediaServerHelper::Get().IsRunning(); 
  printf("Cocoa_IsLocalPlexMediaServerRunning() => %d\n", isRunning);
  
  return isRunning;
  
#else
  NSArray* localAddresses = [[NSHost currentHost] addresses];
  
  NSLog(@"Local hostname: %@", [[NSHost currentHost] name]);
  NSLog(@"Local addresses: \n%@", localAddresses);
  NSData* address = nil;
  for (NSNetService* service in [[XBMCMain sharedInstance] plexMediaServers])
  {
    struct sockaddr_in *socketAddress = nil;
    int i = 0;
    for( ; i < [[service addresses] count]; i++ )
    {
      address = [[service addresses] objectAtIndex: i];
      socketAddress = (struct sockaddr_in *) [address bytes];
      NSString* pmsAddr = [NSString stringWithFormat: @"%s", inet_ntoa(socketAddress->sin_addr)];
      NSLog(@"Detected PMS address: %@", pmsAddr);
      for (NSString* localAddr in localAddresses)
        if ([pmsAddr isEqualToString:localAddr])
        {
          NSLog(@"Local PMS is running!");
          return true;
        }
    }
  }
  NSLog(@"Local PMS is NOT found!");
  return false;
#endif
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

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetLanguage()
{
  // See if we're overriden.
  if (g_advancedSettings.m_language.size() > 0)
    return g_advancedSettings.m_language;
  
  // Otherwise, use the OS X default.
  NSArray* languages = [NSLocale preferredLanguages];
  NSString* language = [languages objectAtIndex:0];
  
  return [language UTF8String];
}

///////////////////////////////////////////////////////////////////////////////
bool Cocoa_IsMetricSystem()
{
  // See if we're overriden.
  if (g_advancedSettings.m_units.size() > 0)
  {
    if (g_advancedSettings.m_units == "metric")
      return true;
    else
      return false;
  }
   
  // Otherwise, use the OS X default.
  NSLocale* locale = [NSLocale currentLocale];
  NSNumber* isMetric = [locale objectForKey:NSLocaleUsesMetricSystem];
  
  return [isMetric boolValue] == YES;
}

///////////////////////////////////////////////////////////////////////////////
static string Cocoa_GetFormatString(int dateFormat, int timeFormat)
{
  id pool = [[NSAutoreleasePool alloc] init];
  
  NSDateFormatter *dateFormatter = [[[NSDateFormatter alloc] init] autorelease];
  [dateFormatter setLocale:[NSLocale currentLocale]];
  [dateFormatter setDateStyle:dateFormat];
  [dateFormatter setTimeStyle:timeFormat];
  
  string ret = [[dateFormatter dateFormat] UTF8String];
  [pool release];

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetLongDateFormat()
{
  return Cocoa_GetFormatString(kCFDateFormatterLongStyle, kCFDateFormatterNoStyle);
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetShortDateFormat()
{
  return Cocoa_GetFormatString(kCFDateFormatterShortStyle, kCFDateFormatterNoStyle);
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetTimeFormat()
{
  string ret = Cocoa_GetFormatString(kCFDateFormatterNoStyle, kCFDateFormatterShortStyle);
  
  boost::replace_all(ret, "a", "xx");
  boost::replace_all(ret, " z", "");
  
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetMeridianSymbol(int i)
{
  string ret;
  
  id pool = [[NSAutoreleasePool alloc] init];
  NSDateFormatter *dateFormatter = [[[NSDateFormatter alloc] init] autorelease];
  [dateFormatter setLocale:[NSLocale currentLocale]];
  
  if (i == 0)
    ret = [[dateFormatter PMSymbol] UTF8String];
  else if (i == 1)
    ret = [[dateFormatter AMSymbol] UTF8String];
  
  [pool release];
  
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetCountryCode()
{
  id pool = [[NSAutoreleasePool alloc] init];
  NSLocale* myLocale = [NSLocale currentLocale];
  NSString* countryCode = [myLocale objectForKey:NSLocaleCountryCode];
  
  string ret = [countryCode UTF8String];
  [pool release];

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetDateString(time_t time, bool longDate)
{
  id pool = [[NSAutoreleasePool alloc] init];
  NSDateFormatter *dateFormatter = [[[NSDateFormatter alloc] init] autorelease];
  [dateFormatter setLocale:[NSLocale currentLocale]];
  
  if (longDate)
    [dateFormatter setDateStyle:kCFDateFormatterLongStyle];
  else
    [dateFormatter setDateStyle:kCFDateFormatterShortStyle];
  
  [dateFormatter setTimeStyle:kCFDateFormatterNoStyle];
  
  NSDate* date = [NSDate dateWithTimeIntervalSince1970:time];
  NSString* formattedDateString = [dateFormatter stringFromDate:date];
  
  string ret = [formattedDateString UTF8String];
  
  [pool release];
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetTimeString(time_t time)
{
  id pool = [[NSAutoreleasePool alloc] init];
  NSDateFormatter *dateFormatter = [[[NSDateFormatter alloc] init] autorelease];
  [dateFormatter setLocale:[NSLocale currentLocale]];
  [dateFormatter setDateStyle:kCFDateFormatterNoStyle];
  [dateFormatter setTimeStyle:kCFDateFormatterShortStyle];
   
  NSDate* date = [NSDate dateWithTimeIntervalSince1970:time];
  NSString* formattedDateString = [dateFormatter stringFromDate:date];
   
  string ret = [formattedDateString UTF8String];
   
  [pool release];
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetMyAddressField(NSString* field)
{
  string ret;

  id pool = [[NSAutoreleasePool alloc] init];
  ABPerson* aPerson = [[ABAddressBook sharedAddressBook] me];
  ABMutableMultiValue* anAddressList = [aPerson valueForProperty:kABAddressProperty];

  if (anAddressList)
  {
    int primaryIndex = [anAddressList indexForIdentifier:[anAddressList primaryIdentifier]];
    if (primaryIndex >= 0)
    {
      NSMutableDictionary* anAddress = [anAddressList valueAtIndex:primaryIndex];
      NSString* value = (NSString* )[anAddress objectForKey:field];

      if (value)
        ret = [value UTF8String];
    }
  }

  [pool release];

  return ret;
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetMyZip()
{
  return Cocoa_GetMyAddressField(kABAddressZIPKey);
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetMyCity()
{
  return Cocoa_GetMyAddressField(kABAddressCityKey);
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetMyCountry()
{
  return Cocoa_GetMyAddressField(kABAddressCountryKey);
}


