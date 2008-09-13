//
//  SUPlexUpdateDriver.m
//  Plex
//
//  Created by James Clarke on 12/09/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SUPlexUpdateDriver.h"
#import "SUHost.h"
#import "GUIDialogUtils.h"


@implementation SUPlexUpdateDriver

- (void)didFindValidUpdate
{
  // Update found - ask the user if they want to download it
  CGUIDialogUtils::CloseProgressDialog();
  if (CGUIDialogUtils::ShowYesNoDialog("Software Update",
                                       "An update for Plex is now available",
                                       [[NSString stringWithFormat:@"Version: %@  (you have %@).", [updateItem displayVersionString], [host displayVersion]] UTF8String],
                                       "Would you like to download and install it now?"))
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
  CGUIDialogUtils::ShowOKDialog("Software Update", [[NSString stringWithFormat:@"You currently have the latest version of Plex  (%@)", [host displayVersion]] UTF8String], "", "");
  [super didNotFindUpdate];
}

- (void)downloadUpdate
{
  // Starting a download - set up a progress dialog with default values & start the download
  CGUIDialogUtils::StartProgressDialog("Software Update", "Downloading Plex update", "", "", false);
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
    CGUIDialogUtils::SetProgressDialogLine(2, [[NSString stringWithFormat:@"%@ of %@", [self _humanReadableSizeFromDouble:(double)processedData], [self _humanReadableSizeFromDouble:(double)totalData], iPercentage] UTF8String]);
  } else {
    CGUIDialogUtils::SetProgressDialogLine(2, [[NSString stringWithFormat:@"%@ downloaded", [self _humanReadableSizeFromDouble:(double)processedData]] UTF8String]);
  }
}

- (void)extractUpdate
{
  totalData = processedData = 0;
  CGUIDialogUtils::StartProgressDialog("Software Update", "Extracting Plex update", "", "", false);
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
  CGUIDialogUtils::StartProgressDialog("Software Update", "Installing Plex update", "", "", false);
  [self installUpdate];
}

- (void)abortUpdate
{
  // Alert the user if the update was aborted
  CGUIDialogUtils::CloseProgressDialog();
  CGUIDialogUtils::ShowOKDialog("Update Failed", "An error has occurred.",  "Please check the log for more details.", "");
  [super abortUpdate];
}

@end
