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

@end
