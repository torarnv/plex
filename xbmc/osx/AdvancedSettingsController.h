//
//  AdvancedSettingsController.h
//  Plex
//
//  Created by James Clarke on 05/03/2009.
//  Copyright 2009 Plex. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <BWToolkitFramework/BWSelectableToolbar.h>


@interface AdvancedSettingsController : NSObject {
  IBOutlet NSWindow* window;
  IBOutlet BWSelectableToolbar* toolbar;
  IBOutlet NSTabView* musicTabView;
  
  BOOL m_settingChanged;
  BOOL m_shouldClose;
  BOOL m_isVisible;

  IBOutlet NSButton* debugLogging;
  IBOutlet NSButton* opticalMedia;
  IBOutlet NSButton* fileDeletion;
  IBOutlet NSButton* trueFullscreen;
  IBOutlet NSButton* cleanOnUpdate;
  IBOutlet NSButton* showExtensions;
  IBOutlet NSButton* showAddSource;
  IBOutlet NSButton* ignoreSortTokens;
  IBOutlet NSPopUpButton* showAllSeasons;
  IBOutlet NSPopUpButton* flattenTVShows;
  IBOutlet NSPopUpButton* scalingAlgorithm;
  IBOutlet NSButton* vizOnPlay;
  IBOutlet NSTextField* timeToViz;
  IBOutlet NSTextField* httpProxyUsername;
  IBOutlet NSTextField* httpProxyPassword;
  IBOutlet NSTextField* nowPlayingFlipTime;
  IBOutlet NSButton* enableKeyboardBacklightControl;
}

+(AdvancedSettingsController*)sharedInstance;

-(IBAction)showWindow:(id)sender;
-(void)closeWindow;
-(BOOL)windowIsVisible;

-(IBAction)settingChanged:(id)sender;
-(IBAction)restoreDefaults:(id)sender;
-(void)loadSettings;
-(void)saveSettings;

-(void)relaunchPlex;

@end
