//
//  AppleHardwareInfo.m
//  Plex
//
//  Created by James Clarke on 21/09/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "AppleHardwareInfo.h"
#import <sys/sysctl.h>
#import <IOKit/IOKitLib.h>
#import <IOKit/ps/IOPowerSources.h>
#import <IOKit/ps/IOPSKeys.h>
#import "GUIDialogUtils.h"

#define BATTERY_CHECK_INTERVAL     60     // Seconds between checking battery
#define BATTERY_TIME_WARNING       10     // Minutes of power left before warning
#define BATTERY_CAPACITY_WARNING   5      // Percent of battery left before warning

enum {
  kGetSensorReadingID   = 0,  // getSensorReading(int *, int *)
  kGetLEDBrightnessID   = 1,  // getLEDBrightness(int, int *)
  kSetLEDBrightnessID   = 2,  // setLEDBrightness(int, int, int *)
  kSetLEDFadeID         = 3,  // setLEDFade(int, int, int, int *)
};

BOOL OpenLMUDataPort(io_connect_t &dataPort)
{
  io_service_t    serviceObject;
  kern_return_t   kr;
  
  // Get the LMU controller
  serviceObject = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("AppleLMUController"));
  if (!serviceObject)
  {
    return NO;
  }
  
  // Create a connection to the IOService object
  kr = IOServiceOpen(serviceObject, mach_task_self(), 0, &dataPort);
  IOObjectRelease(serviceObject);
  if (kr != KERN_SUCCESS) {
    return NO;
  }
  
  return YES;
}

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
  keyboardBacklightEnabled = YES;
  keyboardBacklightTimer = nil;
  keyboardBacklightControlEnabled = NO;

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

- (void)checkKeyboardBacklightLevel
{
  if (keyboardBacklightEnabled) return;
  
  kern_return_t   kr;
  io_connect_t    dataPort = 0;
  SInt32          in_unknown = 0, in_brightness = 0, in_time_ms = 10, out_brightness;
  IOItemCount     getInputCount = 1;
  IOItemCount     setInputCount = 3;
  IOItemCount     outputCount = 1;
  
  // Open the data port
  if (!OpenLMUDataPort(dataPort)) return;
  
  // Get the current brightness level
  kr = IOConnectMethodScalarIScalarO(dataPort, kGetLEDBrightnessID, getInputCount, outputCount, in_unknown, &out_brightness);

  // If the backlight is lit, set the level back to 0
  if (out_brightness > 0)
  {
    kr = IOConnectMethodScalarIScalarO(dataPort, kSetLEDFadeID, setInputCount, outputCount, in_unknown, in_brightness, in_time_ms, &out_brightness);
  }
}

- (void)setKeyboardBacklightEnabled:(BOOL)enabled
{ 
  if (!keyboardBacklightControlEnabled) return;
  
  if (enabled == keyboardBacklightEnabled) return;
  
  kern_return_t   kr;
  io_connect_t    dataPort = 0;
  SInt32          in_unknown = 0, in_brightness, in_time_ms = 1000, out_brightness;
  IOItemCount     getInputCount = 1;
  IOItemCount     setInputCount = 3;
  IOItemCount     outputCount = 1;
  
  // Get the data port
  if (!OpenLMUDataPort(dataPort)) return;
  
  // If enabling, cancel the timer & restore the old brightness value
  if (enabled)
  {
    in_brightness = oldKeyboardBacklightBrightness;
    if (keyboardBacklightTimer != nil)
    {
      [keyboardBacklightTimer invalidate];
      [keyboardBacklightTimer release];
    }
  }
  
  // If disabling, store the old brightness value, set the brightness level to 0 & start a timer to maintain the state.
  else
  {
    // Start a timer to check the backlight level while playing - it'll come back on again if the ambient light sensor is enabled & the light level changes.
    kr = IOConnectMethodScalarIScalarO(dataPort, kGetLEDBrightnessID, getInputCount, outputCount, in_unknown, &out_brightness);
    oldKeyboardBacklightBrightness = out_brightness;
    in_brightness = 0;
    
    keyboardBacklightTimer = [NSTimer scheduledTimerWithTimeInterval:0.2 target:self selector:@selector(checkKeyboardBacklightLevel) userInfo:nil repeats:YES];
  }

  kr = IOConnectMethodScalarIScalarO(dataPort, kSetLEDFadeID, setInputCount, outputCount, in_unknown, in_brightness, in_time_ms, &out_brightness);
  keyboardBacklightEnabled = enabled;

}

- (void)setKeyboardBacklightControlEnabled:(BOOL)enabled;
{
  keyboardBacklightControlEnabled = enabled;
}

@end
