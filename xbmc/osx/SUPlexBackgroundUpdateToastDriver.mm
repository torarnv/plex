//
//  SUPlexBackgroundUpdateToastDriver.m
//  Plex
//
//  Created by James Clarke on 14/09/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SUPlexBackgroundUpdateToastDriver.h"
#import "GUIDialogUtils.h"

@implementation SUPlexBackgroundUpdateToastDriver

- (void)didFindValidUpdate
{
  CGUIDialogUtils::QueueToast(CGUIDialogUtils::Localize(40001), CGUIDialogUtils::Localize(40002));
}

@end
