//
//  SUPlexUpdateDriver.m
//  Plex
//
//  Created by James Clarke on 12/09/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SUPlexUpdateDriver.h"
#import "SUHost.h"
//#import "SUInstaller.h"
#import "SUPle xInstaller.h"
#import "SUConstants.h"
#import "GUIDialogUtils.h"

@implementation SUPlexUpdateDriver

- (void)didFindValidUpdate
{
  NSLog([host bundlePath]);

  // Update found - ask the user if they want to download it
  CGUIDialogUtils::CloseProgressDialog();
  if (CGUIDialogUtils::ShowYesNoDialog(CGUIDialogUtils::Localize(40000),
                                       CGUIDialogUtils::Localize(40003),
                                       [[NSString stringWithFormat:[NSString stringWithCString:(CGUIDialogUtils::Localize(40004).c_str())],
                                                                    [updateItem displayVersionString], [host displayVersion]] UTF8String],
                                       CGUIDialogUtils::Localize(40005)))
  {
    [self downloadUpdate];
  }

  if ([[updater delegate] respondsToSelector:@selector(updater:didFindValidUpdate:)])
		[[updater delegate] updater:updater didFindValidUpdate:updateItem];
}

- (void)didNotFindUpdate
{
  // No update found, let the user know they're up to date
  CGUIDialogUtils::CloseProgressDialog();
  CGUIDialogUtils::ShowOKDialog(CGUIDialogUtils::Localize(40000),
                                [[NSString stringWithFormat:[NSString stringWithCString:(CGUIDialogUtils::Localize(40006).c_str())], [host displayVersion]] UTF8String], "", "");
  [super didNotFindUpdate];
}

- (void)downloadUpdate
{
  // Starting a download - set up a progress dialog with default values & start the download
  CGUIDialogUtils::StartProgressDialog(CGUIDialogUtils::Localize(40000), CGUIDialogUtils::Localize(40007), "", "", false);
  totalData = processedData = 0;
  [super downloadUpdate];
}

- (void)download:(NSURLDownload *)download didReceiveResponse:(NSURLResponse *)response
{
  // Get the expected length of the download (if provided)
  totalData = [response expectedContentLength];
  if (totalData > 0) {
    CGUIDialogUtils::SetProgressDialogBarVisible(true);
  }
}

- (NSString *)_humanReadableSizeFromDouble:(double)value
{
  // Convert number of bytes into human readable sizes
	if (value < 1024)
		return [NSString stringWithFormat:@"%.0lf %@", value, @"B"];
	
	if (value < 1024 * 1024)
		return [NSString stringWithFormat:@"%.0lf %@", value / 1024.0, @"KB"];
	
	if (value < 1024 * 1024 * 1024)
		return [NSString stringWithFormat:@"%.1lf %@", value / 1024.0 / 1024.0, @"MB"];
	
	return [NSString stringWithFormat:@"%.2lf %@", value / 1024.0 / 1024.0 / 1024.0, @"GB", @"the unit for gigabytes"];	
}

- (void)download:(NSURLDownload *)download didReceiveDataOfLength:(NSUInteger)length
{
  // When data is received, update the progress dialog
  processedData += length;
  if (totalData > 0) {
    int iPercentage = (int)(((float)processedData/(float)totalData)*100.0f);
    CGUIDialogUtils::SetProgressDialogPercentage(iPercentage);    
    CGUIDialogUtils::SetProgressDialogLine(2, [[NSString stringWithFormat:[NSString stringWithCString:(CGUIDialogUtils::Localize(40008).c_str())], [self _humanReadableSizeFromDouble:(double)processedData], [self _humanReadableSizeFromDouble:(double)totalData], iPercentage] UTF8String]);
  } else {
    CGUIDialogUtils::SetProgressDialogLine(2, [[NSString stringWithFormat:[NSString stringWithCString:(CGUIDialogUtils::Localize(40009).c_str())], [self _humanReadableSizeFromDouble:(double)processedData]] UTF8String]);
  }
}

- (void)extractUpdate
{
  totalData = processedData = 0;
  CGUIDialogUtils::StartProgressDialog(CGUIDialogUtils::Localize(40000), CGUIDialogUtils::Localize(40010), "", "", false);
  [super extractUpdate];
}

- (void)unarchiver:(SUUnarchiver *)ua extractedLength:(long)length
{
	// We do this here instead of in extractUpdate so that we only have a determinate progress bar for archives with progress.
  processedData += length;
	if (totalData == 0)
	{	
    totalData == [[[[NSFileManager defaultManager] fileAttributesAtPath:downloadPath traverseLink:NO] objectForKey:NSFileSize] intValue];
    CGUIDialogUtils::SetProgressDialogBarVisible(true);
  }
  CGUIDialogUtils::SetProgressDialogPercentage((int)(((float)processedData/(float)totalData)*100.0f));
}

- (void)unarchiverDidFinish:(SUUnarchiver *)ua
{
  // If the archive extracted successfully, install it
  CGUIDialogUtils::StartProgressDialog(CGUIDialogUtils::Localize(40000), CGUIDialogUtils::Localize(40011), "", "", false);
  [super unarchiverDidFinish:ua];
}

- (void)installUpdate
{
	if ([[updater delegate] respondsToSelector:@selector(updater:willInstallUpdate:)])
		[[updater delegate] updater:updater willInstallUpdate:updateItem];
	// Copy the relauncher into a temporary directory so we can get to it after the new version's installed.
	//NSString *relaunchPathToCopy = [[NSBundle bundleForClass:[self class]]  pathForResource:@"relaunch" ofType:@""];
  NSString *relaunchPathToCopy = [[NSBundle mainBundle]  pathForResource:@"relaunch" ofType:@""];
	NSString *targetPath = [NSTemporaryDirectory() stringByAppendingPathComponent:[relaunchPathToCopy lastPathComponent]];
  NSLog(@"Source:   %@\nTarget:   %@\nDLPath:   %@", relaunchPathToCopy, targetPath, downloadPath);
	// Only the paranoid survive: if there's already a stray copy of relaunch there, we would have problems
	[[NSFileManager defaultManager] removeFileAtPath:targetPath handler:nil];
	if ([[NSFileManager defaultManager] copyPath:relaunchPathToCopy toPath:targetPath handler:nil])
		relaunchPath = [targetPath retain];
	
  NSString *bundleFileName = [[[NSBundle mainBundle] bundlePath] lastPathComponent];
  NSString *currentFile, *newAppDownloadPath, *updateFolder = [downloadPath stringByDeletingLastPathComponent];
  NSDirectoryEnumerator *dirEnum = [[NSFileManager defaultManager] enumeratorAtPath:[downloadPath stringByDeletingLastPathComponent]];
  while (currentFile = [dirEnum nextObject])
  {
    NSLog(@"%@", currentFile);
    NSString *currentPath = [updateFolder stringByAppendingPathComponent:currentFile];
    if ([[currentFile lastPathComponent] isEqual:bundleFileName])
    {
      newAppDownloadPath = currentPath;
      NSLog(@"FOUND!!!!!!");
      break;
    }
  }
  
  if (newAppDownloadPath == nil)
	{
		[[SUPlainInstaller class] _finishInstallationWithResult:NO host:host error:[NSError errorWithDomain:SUSparkleErrorDomain code:SUMissingUpdateError userInfo:[NSDictionary dictionaryWithObject:@"Couldn't find an appropriate update in the downloaded package." forKey:NSLocalizedDescriptionKey]] delegate:self];
	}
	else
	{
    NSLog(@"Found something");
		[[SUPlexInstaller class] performInstallationWithPath:newAppDownloadPath host:host delegate:self synchronously:YES];
	}
  
	//[SUPlexInstaller installFromUpdateFolder:[downloadPath stringByDeletingLastPathComponent] overHost:host delegate:self synchronously:YES];
}

- (void)abortUpdate
{
  // Alert the user if the update was aborted
  CGUIDialogUtils::CloseProgressDialog();
  CGUIDialogUtils::ShowOKDialog(CGUIDialogUtils::Localize(40012), CGUIDialogUtils::Localize(40013),  CGUIDialogUtils::Localize(40014), "");
  [super abortUpdate];
}

@end
