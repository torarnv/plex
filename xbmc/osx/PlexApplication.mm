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
#include "FileItem.h"
#undef BOOL
#undef DEBUG
#import <unistd.h>
#import "PlexApplication.h"
#import "KeyboardLayouts.h"

extern CApplication g_application;

int           gArgc;
const char  **gArgv;

BOOL gFinderLaunch = FALSE;
BOOL gCalledAppMainline = FALSE;

@implementation PlexApplication

- (void) finishLaunching
{
  [super finishLaunching];
  gCalledAppMainline = TRUE;
  SDL_main(gArgc, (char** )gArgv);
}

/* Invoked by the dock menu */
- (void)terminate:(id)sender
{
  [self quit:sender];
}

/* Invoked from the Quit menu item */
- (IBAction)quit:(id)sender
{
  /* Post a SDL_QUIT event */
  SDL_Event event;
  event.type = SDL_QUIT;
  SDL_PushEvent(&event);
}

- (IBAction)fullScreenToggle:(id)sender
{
 // Post an toggle full-screen event to the application thread.
 SDL_Event event;
 memset(&event, 0, sizeof(event));
 event.type = PLEX_MESSAGE;
 event.user.code = TMSG_TOGGLEFULLSCREEN;
 SDL_PushEvent(&event);
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
//
    if(!g_OSXKeyboardLayouts.Process(sv_key) && modif & NSCommandKeyMask)
			[super sendEvent: anEvent];
  }
  else
  {
      [super sendEvent: anEvent];
  }
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