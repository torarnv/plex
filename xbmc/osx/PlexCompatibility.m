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
  NSDictionary *appDefaults = [NSDictionary dictionaryWithObject:@"YES" forKey:@"CheckForCandelaIR"];
  [defaults registerDefaults:appDefaults];
}

- (void)checkCompatibility
{
  SInt32 osVersion;
  
  // Try to get the OS version
  if (Gestalt(gestaltSystemVersion, &osVersion) != noErr) 
  {
    NSLog(@"Unable to check Mac OS X version.");
    return;
  }

  NSString* strTitle;
  NSString* strFormat;
  BOOL show = NO;
  
  // CandelaIR is already installed
  if ([HIDRemote isCandelairInstalled])
  {
    //NSLog(@"CandelaIR is installed");
    return;
  }
  
  // Check that the message hasn't been shown before
  NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
  if (![defaults boolForKey:@"CheckForCandelaIR"])
  {
    //NSLog(@"User doesn't want CandelaIR!");
    return;
  }
  
  // Create strings relevant to the current OS version.
  if (osVersion >= 0x1050 && osVersion < 0x1060)
  {
    //NSLog(@"On Leopard");
    strTitle = NSLocalizedString(@"Would you like to install CandelaIR, a replacement driver for the Apple remote?", nil);
    strFormat = NSLocalizedString(@"CandelaIR offers improved Apple remote support on Mac OS X 10.5. It will also improve compatibility for other applications making use of the remote, and will not interfere with their behavior.", nil);
    show = YES;
  }
  else if (osVersion >= 0x1060 && osVersion < 0x1070)
  {
    //NSLog(@"On Snow Leopard");
    strTitle = NSLocalizedString(@"The Apple remote has well known and documented issues on Snow Leopard. You can fix these problems by installing CandelaIR, a replacement driver for the Apple remote. Would you like to go to the CandelaIR website now?", nil);
    strFormat = NSLocalizedString(@"CandelaIR is required for reliable Apple remote support on Mac OS X 10.6. It will also improve compatibility for other applications making use of the remote, and will not interfere with their behavior.", nil);
    show = YES;
  }
    
  // Current OS version is unknown - return
  if (!show) return;

  // Ask the user if they want to install CandelaIR
  self.userInterfaceVisible = YES;
  NSInteger returnCode = NSRunAlertPanel(strTitle, strFormat, @"Go to the CandelaIR website", @"Remind me later", @"Don't ask me again");
  self.userInterfaceVisible = NO;
  switch (returnCode) {
    case NSAlertDefaultReturn:
    {
      [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://www.candelair.com"]];
      exit(0);
      break;
    }
    case NSAlertOtherReturn:
    {
      [defaults setBool:NO forKey:@"CheckForCandelaIR"];
      break;
    }
    default:
    {
      break;
    }
  }


}


@end
