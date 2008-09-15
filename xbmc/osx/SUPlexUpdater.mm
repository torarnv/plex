//
//  SUPlexUpdater.m
//  Plex
//
//  Created by James Clarke on 12/09/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//
#import "SUPlexUpdater.h"
#import "SUPlexUpdateDriver.h"
#import "SUPlexBackgroundUpdateCheckDriver.h"
#import "GUIDialogUtils.h"
#import <objc/objc-runtime.h>


@implementation SUPlexUpdater

+ (SUPlexUpdater*)sharedInstance
{
  return (SUPlexUpdater*)g_plexUpdater;
}

- (void)awakeFromNib
{
  g_plexUpdater = (id)self;
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
  if ([self checkForUpdatesWithPlexDriver:[[[SUPlexUpdateDriver alloc] initWithUpdater:self] autorelease]])
    CGUIDialogUtils::StartProgressDialog(CGUIDialogUtils::Localize(40000), CGUIDialogUtils::Localize(40015), "", "", false);
}

- (void)checkForUpdatesInBackground
{
  /*
  [self resetUpdateCycle];
  SEL selector = @selector(checkForUpdatesWithDriver:);
  if ([self respondsToSelector:selector])
    objc_msgSend(self, selector, [[[SUPlexBackgroundUpdateCheckDriver alloc] initWithUpdater:self] autorelease]);
   */
  [self checkForUpdatesWithPlexDriver:[[[SUPlexBackgroundUpdateCheckDriver alloc] initWithUpdater:self] autorelease]];
}

- (void)checkForUpdatesInBackgroundAndAsk
{
  [self checkForUpdatesWithPlexDriver:[[[SUPlexUpdateDriver alloc] initWithUpdater:self] autorelease]];
}

//
// Disable Sparkle's built-in update checks for now
//
- (BOOL)automaticallyChecksForUpdates
{
  return NO;
}

@end
