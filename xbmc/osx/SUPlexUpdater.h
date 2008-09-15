//
//  SUPlexUpdater.h
//  Plex
//
//  Created by James Clarke on 12/09/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <Sparkle/Sparkle.h>

id g_plexUpdater;

@interface SUPlexUpdater : SUUpdater {
  int updateAlertType;
}

+ (SUPlexUpdater*)sharedInstance;
- (IBAction)checkForUpdatesWithUI:(id)sender;
- (void)checkForUpdatesInBackgroundAndAsk;

@end
