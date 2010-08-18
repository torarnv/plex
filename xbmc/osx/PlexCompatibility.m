//
//  PlexCompatibility.m
//  Plex
//
//  Created by James Clarke on 21/10/2009.
//  Copyright 2009 Plex. All rights reserved.
//

#import "PlexCompatibility.h"
#import "HIDRemote.h"


@implementation PlexCompatibility

id g_plexCompatibility;

@synthesize userInterfaceVisible;

+ (PlexCompatibility*)sharedInstance
{
  if (!g_plexCompatibility)
    g_plexCompatibility = [[PlexCompatibility alloc] init];
  return (PlexCompatibility*)g_plexCompatibility;
}

+ (void)initialize
{
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  NSDictionary *appDefaults = [NSDictionary dictionaryWithObject:@"YES" forKey:@"CheckForCandelair"];
  [defaults registerDefaults:appDefaults];
}

- (void)checkCompatibility
{
  SInt32 osVersionMinor;
  SInt32 osVersionBugFix;
  
  // Try to get the OS version
  if ((Gestalt(gestaltSystemVersionMinor, &osVersionMinor) != noErr) ||
      (Gestalt(gestaltSystemVersionBugFix, &osVersionBugFix) != noErr))
  {
    NSLog(@"Unable to check Mac OS X version.");
    return;
  }
  
  NSString* strTitle;
  NSString* strFormat;
  BOOL show = NO;
  
  // Candelair is already installed
  if ([HIDRemote isCandelairInstalled])
  {
    //NSLog(@"Candelair is installed");
    return;
  }
  
  // Check that the message hasn't been shown before
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  if (![defaults boolForKey:@"CheckForCandelair"])
  {
    //NSLog(@"User doesn't want Candelair!");
    return;
  }
  
  // Create strings relevant to the current OS version.
  if (osVersionMinor == 6 && osVersionBugFix < 3)
  {
    //NSLog(@"On a broken Snow Leopard release");
    strTitle = NSLocalizedString(@"The Apple remote has well known and documented issues on Snow Leopard. You can fix these problems by installing Candelair, a replacement driver for the Apple remote. Would you like to go to the Candelair website now?", nil);
    strFormat = NSLocalizedString(@"Candelair is required for reliable Apple remote support on Mac OS X 10.6.0, 10.6.1 and 10.6.2. It will also improve compatibility for other applications making use of the remote, and will not interfere with their behavior.", nil);
    show = YES;
  }
  else if (osVersionMinor == 5 || (osVersionMinor == 6 && osVersionBugFix >= 3))
  {
    //NSLog(@"On Leopard or a fixed Snow Leopard release");
    strTitle = NSLocalizedString(@"Would you like to install Candelair, a replacement driver for the Apple remote?", nil);
    strFormat = NSLocalizedString(@"Candelair offers improved Apple remote support. It will also improve compatibility for other applications making use of the remote, and will not interfere with their behavior.", nil);
    show = YES;
  }
  
  // Current OS version is unknown - return
  if (!show) return;
  
  // Ask the user if they want to install Candelair
  self.userInterfaceVisible = YES;
  [NSApp activateIgnoringOtherApps:YES];
  NSInteger returnCode = NSRunAlertPanel(strTitle, strFormat, @"Go to the Candelair website", @"Remind me later", @"Don't ask me again");
  self.userInterfaceVisible = NO;
  switch (returnCode) {
    case NSAlertDefaultReturn:
    {
      [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://www.Candelair.com"]];
      exit(0);
      break;
    }
    case NSAlertOtherReturn:
    {
      [defaults setBool:NO forKey:@"CheckForCandelair"];
      break;
    }
    default:
    {
      break;
    }
  }
  
  
}


@end
