//
//  PlexApplication.m
//  Plex
//
//  Created by Elan Feingold on 7/16/2008.
//  Copyright 2008 Blue Mandrill Design. All rights reserved.
//
#include <SDL/SDL.h>
#define BOOL CPP_BOOL
#include "stdafx.h"
#include "Application.h"
#include "Settings.h"
#include "FileItem.h"
#undef BOOL
#undef DEBUG
#import <unistd.h>
#import "PlexApplication.h"
#import "KeyboardLayouts.h"
#import "BackgroundMusicPlayer.h"
#import "CocoaUtils.h"
#import "AdvancedSettingsController.h"

extern CApplication g_application;
id g_plexApplication;

int           gArgc;
const char  **gArgv;

BOOL gFinderLaunch = FALSE;
BOOL gCalledAppMainline = FALSE;

@implementation PlexApplication

+ (PlexApplication*)sharedInstance
{
  return (PlexApplication*)g_plexApplication;
}

- (void)awakeFromNib
{
  g_plexApplication = (id)self;
}

- (void) finishLaunching
{
  [[[NSWorkspace sharedWorkspace] notificationCenter] addObserver:self
                                                         selector:@selector(workspaceDidWake:)
                                                             name:NSWorkspaceDidWakeNotification
                                                           object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(applicationWillResignActive:)
                                              name:NSApplicationWillResignActiveNotification
                                             object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(applicationWillBecomeActive:)
                                               name:NSApplicationWillBecomeActiveNotification
                                             object:nil];
  
  [super finishLaunching];
  gCalledAppMainline = TRUE;
  SDL_main(gArgc, (char** )gArgv);
}

/* Invoked by the dock menu */
- (void)terminate:(id)sender
{
  [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:self];
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  
  [self quit:sender];
}

/* Invoked from the Quit menu item */
- (IBAction)quit:(id)sender
{
  [[[NSWorkspace sharedWorkspace] notificationCenter] removeObserver:self];

  /* Post a SDL_QUIT event */
  SDL_Event event;
  event.type = SDL_QUIT;
  SDL_PushEvent(&event);
}

- (IBAction)fullScreenToggle:(id)sender
{
  [[PlexApplication sharedInstance] activateIgnoringOtherApps:YES];
  // Post an toggle full-screen event to the application thread.
  SDL_Event event;
  memset(&event, 0, sizeof(event));
  event.type = PLEX_MESSAGE;
  event.user.code = TMSG_TOGGLEFULLSCREEN;
  SDL_PushEvent(&event);
}

- (IBAction)floatOnTopToggle:(id)sender
{
  NSWindow* window = [[[NSOpenGLContext currentContext] view] window];
  if ([window level] == NSFloatingWindowLevel)
  {
    [window setLevel:NSNormalWindowLevel];
    [sender setState:NSOffState];
  }
  else
  {
    [window setLevel:NSFloatingWindowLevel];
    [sender setState:NSOnState];
  }
}

- (IBAction)moveToPreviousScreen:(id)sender
{
  // Post an toggle full-screen event to the application thread.
  SDL_Event event;
  memset(&event, 0, sizeof(event));
  event.type = PLEX_MESSAGE;
  event.user.code = TMSG_MOVE_TO_PREV_SCREEN;
  SDL_PushEvent(&event);
}

- (IBAction)moveToNextScreen:(id)sender
{
  // Post an toggle full-screen event to the application thread.
  SDL_Event event;
  memset(&event, 0, sizeof(event));
  event.type = PLEX_MESSAGE;
  event.user.code = TMSG_MOVE_TO_NEXT_SCREEN;
  SDL_PushEvent(&event);
}

- (void)sendEvent:(NSEvent *)anEvent 
{
  SVKey sv_key;
  NSUInteger modif;

  // Ignore Shift+Control+Alt commands because we're going to process them inside Plex and we 
  // don't want the "beep" of an unknown key combination.
  //
  if ([anEvent type] == NSKeyDown)
  {
    if ([anEvent modifierFlags] & NSShiftKeyMask &&
        [anEvent modifierFlags] & NSControlKeyMask &&
        [anEvent modifierFlags] & NSAlternateKeyMask)
    {
      return;
    }
  }
    
  // If the advanced settings window or about window is visible, detect the Cmd+W shortcut & close the window.
  if ((([anEvent modifierFlags] & NSCommandKeyMask) != 0) && ([anEvent type] == NSKeyDown) && ([anEvent keyCode] == 13) && (Cocoa_IsGUIShowing()))
  {
    if ([[AdvancedSettingsController sharedInstance] windowIsVisible])
    {
      [[AdvancedSettingsController sharedInstance] closeWindow];
    }
    if ([aboutWindow isVisible])
    {
      [aboutWindow close];
    }
    return;
  }  
  
  if(NSKeyDown == [anEvent type] || NSKeyUp == [anEvent type]) 
  {
    modif = [anEvent modifierFlags];

    // Convert event to sv_key
    if(NSKeyUp == [anEvent type])
    {
      sv_key.SVKey = (BYTE) [anEvent keyCode];
      sv_key.Apple = ( modif & NSCommandKeyMask ) != 0;
      sv_key.Shift = ( modif &  NSShiftKeyMask ) != 0;
      sv_key.Alt = ( modif & NSAlternateKeyMask ) != 0;
      sv_key.Ctrl = ( modif & NSControlKeyMask ) != 0;
    }
    
    if((!g_OSXKeyboardLayouts.Process(sv_key) && (modif & NSCommandKeyMask || modif & NSControlKeyMask)) || (Cocoa_IsGUIShowing()))
    {
			[super sendEvent: anEvent];
    }
  }
  else
  {
      [super sendEvent: anEvent];
  }
}

- (void)workspaceDidWake:(NSNotification *)aNotification
{
  g_application.ResetDisplaySleep();
}

- (void)applicationWillResignActive:(NSNotification *)aNotification
{
  if (g_advancedSettings.m_bBackgroundMusicOnlyWhenFocused)
    [[BackgroundMusicPlayer sharedInstance] performSelectorOnMainThread:@selector(lostFocus) withObject:nil waitUntilDone:YES];
}

- (void)applicationWillBecomeActive:(NSNotification *)aNotification
{
  if (g_advancedSettings.m_bBackgroundMusicOnlyWhenFocused)
    [[BackgroundMusicPlayer sharedInstance] performSelectorOnMainThread:@selector(foundFocus) withObject:nil waitUntilDone:YES];
}

- (BOOL)isAboutWindowVisible
{
  return [aboutWindow isVisible];
}

@end

#ifdef main
#  undef main
#endif

/* Main entry point to executable - should *not* be SDL_main! */
int main(int argc, const char **argv)
{
  /* Copy the arguments into a global variable */
  /* This is passed if we are launched by double-clicking */
  if ( argc >= 2 && strncmp (argv[1], "-psn", 4) == 0 ) {
    gArgv = (const char **)malloc(sizeof (char *) * 2);
    gArgv[0] = argv[0];
    gArgv[1] = NULL;
    gArgc = 1;
    gFinderLaunch = YES;
  } 
  else 
  {
    int i;
    gArgc = argc;
    gArgv = (const char **)malloc(sizeof (char *) * (argc+1));
    for (i = 0; i <= argc; i++)
        gArgv[i] = argv[i];
    gFinderLaunch = NO;
  }

  NSApplicationMain(argc, argv);  
  return 0;
}
