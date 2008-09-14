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

- (IBAction)checkForUpdatesWithUI:(id)sender
{
  // Override the default method to use our custom update driver.
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
    CGUIDialogUtils::StartProgressDialog(CGUIDialogUtils::Localize(40000), CGUIDialogUtils::Localize(40015), "", "", false);
    objc_msgSend(self, selector, [[[SUPlexUpdateDriver alloc] initWithUpdater:self] autorelease]);
  }
}

- (void)checkForUpdatesInBackground
{
  [self resetUpdateCycle];
  SEL selector = @selector(checkForUpdatesWithDriver:);
  if ([self respondsToSelector:selector])
    objc_msgSend(self, selector, [[[SUPlexBackgroundUpdateCheckDriver alloc] initWithUpdater:self] autorelease]);
}

//
// Disable Sparkle's built-in update checks for now
//
- (BOOL)automaticallyChecksForUpdates
{
  return NO;
}

@end
