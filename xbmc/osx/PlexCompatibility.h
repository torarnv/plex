//
//  PlexCompatibility.h
//  Plex
//
//  Created by James Clarke on 21/10/2009.
//  Copyright 2009 Plex. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface PlexCompatibility : NSObject {
  BOOL userInterfaceVisible;
}

@property (readwrite, assign) BOOL userInterfaceVisible;

+ (PlexCompatibility*)sharedInstance;
- (void)checkCompatibility;

@end
