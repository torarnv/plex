//
//  AppleHardwareInfo.m
//  Plex
//
//  Created by James Clarke on 21/09/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "AppleHardwareInfo.h"
#import <sys/sysctl.h>
#import <IOKit/ps/IOPowerSources.h>
#import <IOKit/ps/IOPSKeys.h>
#import "GUIDialogUtils.h"

#define BATTERY_CHECK_INTERVAL     60     // Seconds between checking battery
#define BATTERY_TIME_WARNING       10     // Minutes of power left before warning
#define BATTERY_CAPACITY_WARNING   5      // Percent of battery left before warning

static AppleHardwareInfo *_o_sharedMainInstance = nil;

@implementation AppleHardwareInfo

@synthesize batteryTimeWarning;
@synthesize batteryCapacityWarning;

//
// Retrieve power source information from IOKit for the first power source (battery)
//
NSDictionary* GetPowerInfoDictionary()
{
  NSDictionary *info = nil;
  CFTypeRef powerInfo = IOPSCopyPowerSourcesInfo();
  CFArrayRef powerList = IOPSCopyPowerSourcesList(powerInfo);
  if (CFArrayGetCount(powerList) > 0)
  {
    CFDictionaryRef powerDict = IOPSGetPowerSourceDescription(powerInfo, CFArrayGetValueAtIndex(powerList, 0));
    info = [NSDictionary dictionaryWithDictionary:(NSDictionary*)powerDict];
  }
  CFRelease(powerList);
  CFRelease(powerInfo);
  return info;
}

//
// Get values from the power source dictionary
//

BOOL GetPowerInfoBoolForKey(const char* key)
{
  NSDictionary *info = GetPowerInfoDictionary();
  return (BOOL)[[info valueForKey:[NSString stringWithCString:key]] boolValue];
}

int GetPowerInfoIntForKey(const char* key)
{
  NSDictionary *info = GetPowerInfoDictionary();
  return (int)[[info valueForKey:[NSString stringWithCString:key]] intValue];
}

NSString* GetPowerInfoStringForKey(const char* key)
{
  NSDictionary *info = GetPowerInfoDictionary();
  return (NSString*)[info valueForKey:[NSString stringWithCString:key]];
}
// End of convenience methods



+ (AppleHardwareInfo*)sharedInstance
{
  return _o_sharedMainInstance ? _o_sharedMainInstance : [[self alloc] init];
}

- (id)init
{
  if( _o_sharedMainInstance)
    [self dealloc];
  else
    _o_sharedMainInstance = [super init];
  
  // Initialise variables
  batteryCheckTimer = nil;
  runningOnBatteryAndWarningDisplayed = NO;
  batteryTimeWarning = BATTERY_TIME_WARNING;
  batteryCapacityWarning = BATTERY_CAPACITY_WARNING;

  id pool = [[NSAutoreleasePool alloc] init];
  
  // Get the model name for the machine
  char modelBuffer[256];
  size_t sz = sizeof(modelBuffer);
  if (0 == sysctlbyname("hw.model", modelBuffer, &sz, NULL, 0)) 
  {
    modelBuffer[sizeof(modelBuffer) - 1] = 0;
    modelName = [[NSString stringWithCString:modelBuffer] retain];
  } 
  else
  {
    modelName = @"Unknown";
  }
  
  // Check the machine model against known models supporting battery functions
  NSArray *modelsWithBattery = [NSArray arrayWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"ModelsWithBattery" ofType:@"plist"]];
  hasBattery = NO;
  int i=0;
  for (; i < [modelsWithBattery count]; i++) 
    if ([(NSString*)[modelsWithBattery objectAtIndex:i] isEqualToString:modelName]) 
       hasBattery = YES;

  [pool release];
  
  return _o_sharedMainInstance;
}

- (NSString*)modelName { return modelName; }

- (NSString*)longModelName
{
  NSDictionary* modelNames = [NSDictionary dictionaryWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"ModelNames" ofType:@"plist"]];
  return [modelNames objectForKey:modelName];
}

- (BOOL)hasBattery { return hasBattery; }

- (BOOL)isOnACPower
{
  if (hasBattery)
    return [GetPowerInfoStringForKey(kIOPSPowerSourceStateKey) isEqualToString:[NSString stringWithCString:kIOPSACPowerValue]];
  
  // If we don't know anything about the machine's battery, assume it's on AC power
  else
    return YES;
}

- (BOOL)isCharging
{
  if (hasBattery)
    return GetPowerInfoBoolForKey(kIOPSIsChargingKey);
  
  // Assume the battery isn't charging
  else
    return NO;
}

- (int)currentBatteryCapacity
{
  if (hasBattery)
    return GetPowerInfoIntForKey(kIOPSCurrentCapacityKey);
  else
    return -1;
}

- (int)timeToEmpty
{
  if (hasBattery)
    return GetPowerInfoIntForKey(kIOPSTimeToEmptyKey);
  else
    return -1;
}

- (int)timeToFullCharge
{
  if (hasBattery)
    return GetPowerInfoIntForKey(kIOPSTimeToFullChargeKey);
  else
    return -1;
}

- (void)setLowBatteryWarningEnabled:(BOOL)enabled
{
  if (hasBattery)
  {
    // If the timer already exists, invalidate & release it
    if (batteryCheckTimer != nil)
    {
      [batteryCheckTimer invalidate];
      [batteryCheckTimer release];
      batteryCheckTimer = nil;
    }
    
    // Enable the timer if required
    if (enabled)
      batteryCheckTimer = [NSTimer scheduledTimerWithTimeInterval:BATTERY_CHECK_INTERVAL target:self selector:@selector(checkBatteryCapacity) userInfo:nil repeats:YES];
  }
}
    
- (void)checkBatteryCapacity
{
  // If running on battery with less than the required power or capacity remaining, display a warning (if not already shown)
  if ((![self isOnACPower]) && (!runningOnBatteryAndWarningDisplayed) &&
      ((([self timeToEmpty] < batteryTimeWarning) && ([self timeToEmpty] >= 0)) ||
       ([self currentBatteryCapacity] < batteryCapacityWarning)))
  {
    CGUIDialogUtils::QueueToast(CGUIDialogUtils::Localize(17701), CGUIDialogUtils::Localize(17702));
    // Flag that the warning was displayed so the user isn't constantly bugged
    runningOnBatteryAndWarningDisplayed = YES;
  }
  
  // If the computer is connected to AC power, reset the warning
  else if ([self isOnACPower])
  {
    runningOnBatteryAndWarningDisplayed = NO;
  }
}

@end
