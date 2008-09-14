//
//  SUPlexInstaller.m
//  Sparkle
//
//  Created by James Clarke on 14/09/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SUPlexInstaller.h"
#import "SUHost.h"

@implementation SUPlexInstaller

+ (void)performInstallationWithPath:(NSString *)path host:(SUHost *)host delegate:delegate synchronously:(BOOL)synchronously;
{
	NSDictionary *info = [NSDictionary dictionaryWithObjectsAndKeys:path, SUInstallerPathKey, host, SUInstallerHostKey, delegate, SUInstallerDelegateKey, nil];
	if (synchronously)
		[self _performInstallationWithInfo:info];
	else
		[NSThread detachNewThreadSelector:@selector(_performInstallationWithInfo:) toTarget:self withObject:info];
}


@end
