//
//  frontrowlauncher.m
//  Plex
//
//  Created by James Clarke on 23/10/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <unistd.h>
#import <AppKit/AppKit.h>

#define STATE_PLEX_STOPPING        0
#define STATE_FRONTROW_STARTING    1
#define STATE_FRONTROW_RUNNING     2
#define STATE_FRONTROW_EXIT        3

@interface FrontRowLauncher : NSObject
{
  const char* executablePath;
  int state;
}

@end


@implementation FrontRowLauncher

- (id)initWithExecutablePath:(const char *)execPath
{
  self = [super init];
  executablePath = execPath;
  state = STATE_PLEX_STOPPING;
  [NSTimer scheduledTimerWithTimeInterval:1.0 target:self selector:@selector(watchdog:) userInfo:nil repeats:YES];
  
  return self;
}

- (void)watchdog:(NSTimer *)timer
{
  
  switch (state)
  {
    case STATE_PLEX_STOPPING:
    {
      BOOL plexIsRunning = NO;
      int i = 0;
      NSArray* launchedApps = [[NSWorkspace sharedWorkspace] launchedApplications];
      for (; i < [launchedApps count]; i++)
        if ([[[launchedApps objectAtIndex:i] objectForKey:@"NSApplicationBundleIdentifier"] isEqualToString:@"com.plexsquared.Plex"])
          plexIsRunning = YES;
      
      if (!plexIsRunning)
      {
        [[NSWorkspace sharedWorkspace] launchApplication:@"/Applications/Front Row.app"];
        state = STATE_FRONTROW_STARTING;
      }
      break;
    }
      
    case STATE_FRONTROW_STARTING:
    {
      if ([[[[NSWorkspace sharedWorkspace] activeApplication] objectForKey:@"NSApplicationBundleIdentifier"] isEqualToString:@"com.apple.frontrow"])
        state = STATE_FRONTROW_RUNNING;
      else
        NSLog(@"FrontRow not running yet");
      break;
    }
      
    case STATE_FRONTROW_RUNNING:
    {
      if (![[[[NSWorkspace sharedWorkspace] activeApplication] objectForKey:@"NSApplicationBundleIdentifier"] isEqualToString:@"com.apple.frontrow"])
      {
        [[NSWorkspace sharedWorkspace] openFile:[[NSFileManager defaultManager] stringWithFileSystemRepresentation:executablePath length:strlen(executablePath)]];
        exit(0);
      }
      break;
    }
  }
}

@end



int main (int argc, const char * argv[])
{
  if (argc != 2) return EXIT_FAILURE;
  
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	[NSApplication sharedApplication];
	[[[FrontRowLauncher alloc] initWithExecutablePath:argv[1]] autorelease];
	[[NSApplication sharedApplication] run];
	
	[pool drain];
	
	return EXIT_SUCCESS;
}