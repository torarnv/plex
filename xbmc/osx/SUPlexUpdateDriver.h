//
//  SUPlexUpdateDriver.h
//  Plex
//
//  Created by James Clarke on 12/09/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <Sparkle/Sparkle.h>
#import "SUBasicUpdateDriver.h"

@interface SUPlexUpdateDriver : SUBasicUpdateDriver {
  int processedData, totalData;
}

- (NSString *)_temporaryCopyNameForPath:(NSString *)path;
- (BOOL)_copyPathWithForcedAuthentication:(NSString *)src toPath:(NSString *)dst error:(NSError **)error;
- (BOOL)copyPathWithAuthentication:(NSString *)src overPath:(NSString *)dst error:(NSError **)error;
- (int)removeXAttr:(const char*)name fromFile:(NSString*)file options:(int)options;
- (void)releaseFromQuarantine:(NSString*)root;


@end
