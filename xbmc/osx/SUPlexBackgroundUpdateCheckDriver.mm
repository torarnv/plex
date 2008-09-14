//
//  SUPlexBackgroundUpdateCheckDriver.m
//  Plex
//
//  Created by James Clarke on 14/09/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SUPlexBackgroundUpdateCheckDriver.h"
#import "GUIDialogUtils.h"

@implementation SUPlexBackgroundUpdateCheckDriver

- (void)didFindValidUpdate
{
  CGUIDialogUtils::QueueToast(CGUIDialogUtils::Localize(40001), CGUIDialogUtils::Localize(40002));
}

@end
