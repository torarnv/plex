//
//  SUPlexUpdater.m
//  Plex
//
//  Created by James Clarke on 12/09/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//
#include "CocoaUtilsPlus.h"
#include "md5.h"
#import "SUPlexUpdater.h"
#import "SUBasicUpdateDriver.h"
#import "SUPlexUpdateDriver.h"
#import "SUPlexBackgroundUpdateToastDriver.h"
#import "SUPlexBackgroundUpdateDialogDriver.h"
#import "GUIDialogUtils.h"
#import <objc/objc-runtime.h>

@implementation SUPlexUpdater

id g_plexUpdater;

+ (SUPlexUpdater*)sharedInstance
{
  return (SUPlexUpdater*)g_plexUpdater;
}

- (void)dealloc
{
  [self setLastCheckTime:nil];
  [super dealloc];
}

- (void)awakeFromNib
{
  g_plexUpdater = (id)self;
  [g_plexUpdater setDelegate:self];
  isSuspended = NO;
  userHasBeenAlerted = NO;
  timer = nil;
  [self setLastCheckTime:[NSDate distantPast]];
}

- (BOOL)checkForUpdatesWithPlexDriver:(SUUpdateDriver*)plexDriver
{
  // Override the default method to use our custom update drivers.
  // checkForUpdatesWithDriver: is a private method, so check the object
  // will respond before sending the message to prevent compiler warnings
  [self resetUpdateCycle];
  SEL selector = @selector(checkForUpdatesWithDriver:);
  if ([self respondsToSelector:selector])
  {
    if (driver != nil)
    {
      [driver release];
      driver = nil;
    }
    objc_msgSend(self, selector, plexDriver);
    return true;
  }
  return false;
}

- (IBAction)checkForUpdatesWithUI:(id)sender
{
  [self setLastCheckTime:[NSDate date]];
  if ([self checkForUpdatesWithPlexDriver:[[[SUPlexUpdateDriver alloc] initWithUpdater:self] autorelease]])
    CGUIDialogUtils::StartProgressDialog(CGUIDialogUtils::Localize(40000), CGUIDialogUtils::Localize(40015), "", "", false);
}

- (void)checkForUpdatesInBackground
{
  [self setLastCheckTime:[NSDate date]];
  if (updateAlertType == UPDATE_ALERT_ASK)
    [self checkForUpdatesWithPlexDriver:[[[SUPlexBackgroundUpdateDialogDriver alloc] initWithUpdater:self] autorelease]];
  else
    [self checkForUpdatesWithPlexDriver:[[[SUPlexBackgroundUpdateToastDriver alloc] initWithUpdater:self] autorelease]];
}

- (void)setAlertType:(int)alertType
{
  updateAlertType = alertType;
}

- (void)checkScheduler
{
  // If the check interval has elapsed, the updater is not suspended & the user hasn't been alerted about an update
  // previously, check for updates
  //
  if (([[NSDate date] timeIntervalSinceDate:lastCheckTime] > checkInterval) && (!isSuspended) && (!userHasBeenAlerted))
  {
    [self checkForUpdatesInBackground];
  }
}

- (void)setCheckInterval:(double)seconds
{
  checkInterval = seconds;
  
  // Release the timer if it already exists
  if (timer != nil) 
  {
    [timer invalidate];
    timer = nil;
  }
  
  // Setting the interval to 0 clears the timer
  if (checkInterval > 0)
    timer = [NSTimer scheduledTimerWithTimeInterval:checkInterval target:self selector:@selector(checkScheduler) userInfo:nil repeats:NO];
}

- (void)setSuspended:(BOOL)willSuspend
{
  isSuspended = willSuspend;
}

- (void)userAlerted 
{
  userHasBeenAlerted = YES;
}

- (void)setLastCheckTime:(NSDate *)value
{
  if (lastCheckTime != value)
  {
    [lastCheckTime release];
    lastCheckTime = [value retain];
  }
}

//
// Disable Sparkle's built-in update checks for now
//
- (BOOL)automaticallyChecksForUpdates
{
  return NO;
}

#pragma mark Delegate methods

- (NSArray *)feedParametersForUpdater:(SUUpdater *)updater sendingSystemProfile:(BOOL)sendingProfile
{
  string macAddr = Cocoa_GetPrimaryMacAddress();
  
  XBMC::MD5 md5;
  unsigned char digest[16];
  md5.append((unsigned char* )macAddr.c_str(), macAddr.size());
  md5.getDigest(digest);
  
  string hashedMacAddr;
  for (int i=0; i<16; i++)
  {
    char hex[3];
    sprintf(hex, "%02x", digest[i]);
    hashedMacAddr += hex;
  }

  NSString* strHashed = [NSString stringWithUTF8String:hashedMacAddr.c_str()];
  NSDictionary *appDict = [NSDictionary dictionaryWithObjectsAndKeys:
          @"hashedMacAddress",   @"key",        strHashed, @"value",
          @"Hashed MAC Address", @"displayKey", strHashed, @"displayValue",
          nil];
  
  return [NSArray arrayWithObjects:appDict, nil];
}

- (BOOL)updaterShouldPromptForPermissionToCheckForUpdates:(id)bundle
{
  return NO;
}
@end
