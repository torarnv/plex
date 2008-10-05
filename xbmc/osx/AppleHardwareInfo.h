//
//  AppleHardwareInfo.h
//  Plex
//
//  Created by James Clarke on 21/09/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface AppleHardwareInfo : NSObject {
  NSString *modelName;
  BOOL hasBattery;
  
  NSTimer *batteryCheckTimer;
  BOOL runningOnBatteryAndWarningDisplayed;
}

+ (AppleHardwareInfo*)sharedInstance;

- (NSString*)modelName;
- (NSString*)longModelName;
- (BOOL)hasBattery;

- (BOOL)isOnACPower;
- (BOOL)isCharging;
- (int)currentBatteryCapacity;
- (int)timeToEmpty;
- (int)timeToFullCharge;

- (void)setLowBatteryWarningEnabled:(BOOL)enabled;
- (void)checkBatteryCapacity;

@end
