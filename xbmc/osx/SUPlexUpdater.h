//
//  SUPlexUpdater.h
//  Plex
//
//  Created by James Clarke on 12/09/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <Sparkle/Sparkle.h>

#define UPDATE_ALERT_NOTIFY 0
#define UPDATE_ALERT_ASK 1

@interface SUPlexUpdater : SUUpdater {
@private
  int updateAlertType;
  NSDate *lastCheckTime;
  NSTimeInterval checkInterval;
  NSTimer *timer;
  BOOL isSuspended;
  BOOL userHasBeenAlerted;
}

+ (SUPlexUpdater*)sharedInstance;
- (IBAction)checkForUpdatesWithUI:(id)sender;
- (void)checkForUpdatesInBackground;
- (void)setAlertType:(int)alertType;
- (void)setCheckInterval:(double)seconds;
- (void)setSuspended:(BOOL)willSuspend;
- (void)userAlerted;

- (void)setLastCheckTime:(NSDate *)value;
@end
