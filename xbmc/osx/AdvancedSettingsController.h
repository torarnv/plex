//
//  AdvancedSettingsController.h
//  Plex
//
//  Created by James Clarke on 05/03/2009.
//  Copyright 2009 Plex. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface AdvancedSettingsController : NSObject {
  IBOutlet NSWindow* window;
  
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
  IBOutlet NSPopUpButton* showAllSeasons; //
  IBOutlet NSPopUpButton* flattenTVShows; //
  IBOutlet NSPopUpButton* scalingAlgorithm; //
  IBOutlet NSButton* vizOnPlay;
  IBOutlet NSTextField* timeToViz;
  IBOutlet NSTextField* httpProxyUsername;
  IBOutlet NSTextField* httpProxyPassword;
}

+(AdvancedSettingsController*)sharedInstance;

-(IBAction)showWindow:(id)sender;
-(BOOL)windowIsVisible;

-(IBAction)settingChanged:(id)sender;
-(IBAction)restoreDefaults:(id)sender;
-(void)loadSettings;
-(void)saveSettings;

-(void)relaunchPlex;

@end
