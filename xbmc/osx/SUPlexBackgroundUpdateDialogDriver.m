//
//  SUPlexBackgroundUpdateDialogDriver.m
//  Plex
//
//  Created by James Clarke on 19/10/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SUPlexBackgroundUpdateDialogDriver.h"


@implementation SUPlexBackgroundUpdateDialogDriver

- (void)didNotFindUpdate {}

- (void)abortUpdateWithError:(NSError *)error
{
  [self abortUpdate];
}

@end
