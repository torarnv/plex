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
#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h>
#import <SystemConfiguration/SystemConfiguration.h>
#include <arpa/inet.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/network/IOEthernetInterface.h>
#include <IOKit/network/IONetworkInterface.h>
#include <IOKit/network/IOEthernetController.h>

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
vector<in_addr_t> Cocoa_AddressesForHost(const string& hostname)
{
  NSHost *host = [NSHost hostWithName:[NSString stringWithCString:hostname.c_str()]];
  vector<in_addr_t> ret;
  for (NSString *address in [host addresses])
    ret.push_back(inet_addr([address UTF8String]));
  return ret;
}

///////////////////////////////////////////////////////////////////////////////
bool Cocoa_AreHostsEqual(const string& host1, const string& host2)
{
  NSHost *h1 = [NSHost hostWithName:[NSString stringWithCString:host1.c_str()]];
  NSHost *h2 = [NSHost hostWithName:[NSString stringWithCString:host2.c_str()]];
  return ([h1 isEqualToHost:h2] || ([h1 isEqualToHost:[NSHost currentHost]] && [h2 isEqualToHost:[NSHost currentHost]]));
}

///////////////////////////////////////////////////////////////////////////////
VECSOURCES Cocoa_GetPlexMediaServersAsSourcesWithMediaType(const string& mediaType)
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
bool Cocoa_IsLocalPlexMediaServerRunning()
{
  bool isRunning = PlexMediaServerHelper::Get().IsRunning(); 
  return isRunning;
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
string Cocoa_GetTimeFormat(bool withMeridian)
{
  string ret = Cocoa_GetFormatString(kCFDateFormatterNoStyle, kCFDateFormatterShortStyle);
  
  // Optionally remove meridian.
  if (withMeridian == false)
    boost::replace_all(ret, "a", "");

  // Remove timezone.
  boost::replace_all(ret, "z", "");
  boost::replace_all(ret, "Z", "");
  
  // Remove extra spaces.
  boost::replace_all(ret, "  ", "");
  
  boost::trim(ret);
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
string Cocoa_GetTimeString(const string& format, time_t time)
{
  id pool = [[NSAutoreleasePool alloc] init];
  NSDateFormatter *dateFormatter = [[[NSDateFormatter alloc] init] autorelease];
  [dateFormatter setLocale:[NSLocale currentLocale]];
  [dateFormatter setDateFormat:[NSString stringWithCString:format.c_str()]];

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

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetMyState()
{
  return Cocoa_GetMyAddressField(kABAddressStateKey);
}

///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetMachineSerialNumber()
{
  NSString* result = nil;
  CFStringRef serialNumber = NULL;

  io_service_t platformExpert = IOServiceGetMatchingService(
     kIOMasterPortDefault,
     IOServiceMatching("IOPlatformExpertDevice")
  );

  if (platformExpert)
  {
     CFTypeRef serialNumberAsCFString = IORegistryEntryCreateCFProperty(
        platformExpert,
        CFSTR(kIOPlatformSerialNumberKey),
        kCFAllocatorDefault,
        0
     );
     serialNumber = (CFStringRef)serialNumberAsCFString;
     IOObjectRelease(platformExpert);
   }

  if (serialNumber)
    result = [(NSString*)serialNumber autorelease];
  else
    result = @"";
  
  return [result UTF8String];
}

// Returns an iterator containing the primary (built-in) Ethernet interface. The caller is responsible for
// releasing the iterator after the caller is done with it.
static kern_return_t FindEthernetInterfaces(io_iterator_t *matchingServices)
{
    kern_return_t    kernResult; 
    mach_port_t      masterPort;
    CFMutableDictionaryRef  matchingDict;
    CFMutableDictionaryRef  propertyMatchDict;
    
    // Retrieve the Mach port used to initiate communication with I/O Kit
    kernResult = IOMasterPort(MACH_PORT_NULL, &masterPort);
    if (KERN_SUCCESS != kernResult)
    {
        printf("IOMasterPort returned %d\n", kernResult);
        return kernResult;
    }
    
    // Ethernet interfaces are instances of class kIOEthernetInterfaceClass. 
    // IOServiceMatching is a convenience function to create a dictionary with the key kIOProviderClassKey and 
    // the specified value.
    matchingDict = IOServiceMatching(kIOEthernetInterfaceClass);

    // Note that another option here would be:
    // matchingDict = IOBSDMatching("en0");
        
    if (NULL == matchingDict)
    {
        printf("IOServiceMatching returned a NULL dictionary.\n");
    }
    else {
        // Each IONetworkInterface object has a Boolean property with the key kIOPrimaryInterface. Only the
        // primary (built-in) interface has this property set to TRUE.
        
        // IOServiceGetMatchingServices uses the default matching criteria defined by IOService. This considers
        // only the following properties plus any family-specific matching in this order of precedence 
        // (see IOService::passiveMatch):
        //
        // kIOProviderClassKey (IOServiceMatching)
        // kIONameMatchKey (IOServiceNameMatching)
        // kIOPropertyMatchKey
        // kIOPathMatchKey
        // kIOMatchedServiceCountKey
        // family-specific matching
        // kIOBSDNameKey (IOBSDNameMatching)
        // kIOLocationMatchKey
        
        // The IONetworkingFamily does not define any family-specific matching. This means that in            
        // order to have IOServiceGetMatchingServices consider the kIOPrimaryInterface property, we must
        // add that property to a separate dictionary and then add that to our matching dictionary
        // specifying kIOPropertyMatchKey.
            
        propertyMatchDict = CFDictionaryCreateMutable( kCFAllocatorDefault, 0,
                                                       &kCFTypeDictionaryKeyCallBacks,
                                                       &kCFTypeDictionaryValueCallBacks);
    
        if (NULL == propertyMatchDict)
        {
            printf("CFDictionaryCreateMutable returned a NULL dictionary.\n");
        }
        else {
            // Set the value in the dictionary of the property with the given key, or add the key 
            // to the dictionary if it doesn't exist. This call retains the value object passed in.
            CFDictionarySetValue(propertyMatchDict, CFSTR(kIOPrimaryInterface), kCFBooleanTrue); 
            
            // Now add the dictionary containing the matching value for kIOPrimaryInterface to our main
            // matching dictionary. This call will retain propertyMatchDict, so we can release our reference 
            // on propertyMatchDict after adding it to matchingDict.
            CFDictionarySetValue(matchingDict, CFSTR(kIOPropertyMatchKey), propertyMatchDict);
            CFRelease(propertyMatchDict);
        }
    }
    
    // IOServiceGetMatchingServices retains the returned iterator, so release the iterator when we're done with it.
    // IOServiceGetMatchingServices also consumes a reference on the matching dictionary so we don't need to release
    // the dictionary explicitly.
    kernResult = IOServiceGetMatchingServices(masterPort, matchingDict, matchingServices);    
    if (KERN_SUCCESS != kernResult)
    {
        printf("IOServiceGetMatchingServices returned %d\n", kernResult);
    }
        
    return kernResult;
}
    
// Given an iterator across a set of Ethernet interfaces, return the MAC address of the last one.
// If no interfaces are found the MAC address is set to an empty string.
// In this sample the iterator should contain just the primary interface.
static kern_return_t GetMACAddress(io_iterator_t intfIterator, UInt8 *MACAddress)
{
    io_object_t    intfService;
    io_object_t    controllerService;
    kern_return_t  kernResult = KERN_FAILURE;
    
    // Initialize the returned address
    bzero(MACAddress, kIOEthernetAddressSize);
    
    // IOIteratorNext retains the returned object, so release it when we're done with it.
    while (intfService = IOIteratorNext(intfIterator))
    {
        CFTypeRef  MACAddressAsCFData;        

        // IONetworkControllers can't be found directly by the IOServiceGetMatchingServices call, 
        // since they are hardware nubs and do not participate in driver matching. In other words,
        // registerService() is never called on them. So we've found the IONetworkInterface and will 
        // get its parent controller by asking for it specifically.
        
        // IORegistryEntryGetParentEntry retains the returned object, so release it when we're done with it.
        kernResult = IORegistryEntryGetParentEntry( intfService,
                                                    kIOServicePlane,
                                                    &controllerService );

        if (KERN_SUCCESS != kernResult)
        {
            printf("IORegistryEntryGetParentEntry returned 0x%08x\n", kernResult);
        }
        else 
        {
            // Retrieve the MAC address property from the I/O Registry in the form of a CFData
            MACAddressAsCFData = IORegistryEntryCreateCFProperty( controllerService,
                                                                  CFSTR(kIOMACAddress),
                                                                  kCFAllocatorDefault,
                                                                  0);
            if (MACAddressAsCFData)
            {
                // Get the raw bytes of the MAC address from the CFData
                CFDataGetBytes((CFDataRef)MACAddressAsCFData, CFRangeMake(0, kIOEthernetAddressSize), MACAddress);
                CFRelease(MACAddressAsCFData);
            }
            else
            {
              printf("Error obtaining MAC Address.\n");
            }
                
            // Done with the parent Ethernet controller object so we release it.
            IOObjectRelease(controllerService);
        }
        
        // Done with the Ethernet interface object so we release it.
        IOObjectRelease(intfService);
    }
        
    return kernResult;
}


///////////////////////////////////////////////////////////////////////////////
string Cocoa_GetPrimaryMacAddress()
{
  io_iterator_t  intfIterator;
  UInt8          MACAddress[kIOEthernetAddressSize];
  kern_return_t  kernResult = KERN_SUCCESS;
  string         ret;
  
  memset(MACAddress, 0, sizeof(MACAddress));
  kernResult = FindEthernetInterfaces(&intfIterator);
      
  if (KERN_SUCCESS != kernResult)
  {
    printf("FindEthernetInterfaces returned 0x%08x\n", kernResult);
  }
  else 
  {
    kernResult = GetMACAddress(intfIterator, MACAddress);
    if (KERN_SUCCESS == kernResult)
    {
      for (int i=0; i<kIOEthernetAddressSize; i++)
      {
        char digit[3];
        sprintf(digit, "%02x", MACAddress[i]);
        ret += digit;
      }
    }
  }
      
  IOObjectRelease(intfIterator);
  return ret;
}
