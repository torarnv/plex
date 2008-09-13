//
//  SUPlexUpdater.m
//  Plex
//
//  Created by James Clarke on 12/09/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//
#import "SUPlexUpdater.h"
#import "SUPlexUpdateDriver.h"
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

- (IBAction)checkForUpdates:sender
{
  // Override the default method to use our custom update driver.
  // checkForUpdatesWithDriver: is a private method, so check the object
  // will respond before sending the message to prevent compiler warnings
  SEL selector = @selector(checkForUpdatesWithDriver:);
  if ([self respondsToSelector:selector])
  {  
    CGUIDialogUtils::StartProgressDialog("Software Update", "Checking for updates", "", "", false);
    objc_msgSend(self, selector, [[[SUPlexUpdateDriver alloc] initWithUpdater:self] autorelease]);
  }
}

@end
