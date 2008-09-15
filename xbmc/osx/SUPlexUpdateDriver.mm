//
//  SUPlexUpdateDriver.m
//  Plex
//
//  Created by James Clarke on 12/09/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "SUPlexUpdateDriver.h"
#import "SUHost.h"
#import "SUPlainInstaller.h"
#import "SUConstants.h"
#import "GUIDialogUtils.h"
#import <sys/stat.h>
#import <sys/wait.h>
#import <sys/xattr.h>
#import <dirent.h>
#import <unistd.h>

// From SUPlainInstaller
// Authorization code based on generous contribution from Allan Odgaard. Thanks, Allan!
static BOOL AuthorizationExecuteWithPrivilegesAndWait(AuthorizationRef authorization, const char* executablePath, AuthorizationFlags options, const char* const* arguments)
{
	sig_t oldSigChildHandler = signal(SIGCHLD, SIG_DFL);
	BOOL returnValue = YES;
  
	if (AuthorizationExecuteWithPrivileges(authorization, executablePath, options, (char* const*)arguments, NULL) == errAuthorizationSuccess)
	{
		int status;
		pid_t pid = wait(&status);
		if (pid == -1 || !WIFEXITED(status) || WEXITSTATUS(status) != 0)
			returnValue = NO;
	}
	else
		returnValue = NO;
  
	signal(SIGCHLD, oldSigChildHandler);
	return returnValue;
}

@implementation SUPlexUpdateDriver

- (void)didFindValidUpdate
{
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
  // Use the default Sparkle functions to extract the update
  totalData = processedData = 0;
  CGUIDialogUtils::StartProgressDialog(CGUIDialogUtils::Localize(40000), CGUIDialogUtils::Localize(40010), "", "", false);
  [super extractUpdate];
}

- (void)unarchiver:(SUUnarchiver *)ua extractedLength:(long)length
{
	// We do this here instead of in extractUpdate so that we only have a progress bar for archives with progress.
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
	if (ua) { CFRelease(ua); }
	[self installUpdate];
}

//
// Mimics the functionality of SUPlainInstaller
//
- (void)installUpdate
{
	if ([[updater delegate] respondsToSelector:@selector(updater:willInstallUpdate:)])
		[[updater delegate] updater:updater willInstallUpdate:updateItem];
	// Copy the relauncher (binary from the Sparkle framework) into a temporary directory so we can get to it after the new version's installed.
	NSString *relaunchPathToCopy = [[NSBundle mainBundle]  pathForResource:@"relaunch" ofType:@""];
	NSString *targetPath = [NSTemporaryDirectory() stringByAppendingPathComponent:[relaunchPathToCopy lastPathComponent]];

	// Only the paranoid survive: if there's already a stray copy of relaunch there, we would have problems
	[[NSFileManager defaultManager] removeFileAtPath:targetPath handler:nil];
  
	if ([[NSFileManager defaultManager] copyPath:relaunchPathToCopy toPath:targetPath handler:nil])
		relaunchPath = [targetPath retain];
	
  NSString *bundleFileName = [[[NSBundle mainBundle] bundlePath] lastPathComponent];
  NSString *currentFile, *newAppDownloadPath, *updateFolder = [downloadPath stringByDeletingLastPathComponent];
  NSDirectoryEnumerator *dirEnum = [[NSFileManager defaultManager] enumeratorAtPath:[downloadPath stringByDeletingLastPathComponent]];
  while (currentFile = [dirEnum nextObject])
  {
    NSString *currentPath = [updateFolder stringByAppendingPathComponent:currentFile];
    if ([[currentFile lastPathComponent] isEqual:bundleFileName])
    {
      newAppDownloadPath = currentPath;
      break;
    }
  }
  
  if (newAppDownloadPath == nil)
	{
		[[SUPlainInstaller class] _finishInstallationWithResult:NO host:host error:[NSError errorWithDomain:SUSparkleErrorDomain code:SUMissingUpdateError userInfo:[NSDictionary dictionaryWithObject:@"Couldn't find an appropriate update in the downloaded package." forKey:NSLocalizedDescriptionKey]] delegate:self];
	}
	else
	{
    // Install function from SUPlainInstaller
    NSError *error;
    BOOL result = [self copyPathWithAuthentication:newAppDownloadPath overPath:[[NSBundle mainBundle] bundlePath] error:&error];
    [SUInstaller _finishInstallationWithResult:result host:host error:error delegate:self];
	}
}

- (void)abortUpdate
{
  // Alert the user if the update was aborted
  CGUIDialogUtils::CloseProgressDialog();
  CGUIDialogUtils::ShowOKDialog(CGUIDialogUtils::Localize(40012), CGUIDialogUtils::Localize(40013),  CGUIDialogUtils::Localize(40014), "");
  [super abortUpdate];
}



/* functions below from Sparkle's SUPlainInstallerInterals */

- (NSString *)_temporaryCopyNameForPath:(NSString *)path
{
  // This code currently causes Plex to crash...
	/*
  // Let's try to read the version number so the filename will be more meaningful.
	NSString *postFix;
	NSBundle *bundle;
  
	if ((bundle = [NSBundle bundleWithPath:path]))
	{
		// We'll clean it up a little for safety.
		// The cast is necessary because of a bug in the headers in pre-10.5 SDKs
		NSMutableCharacterSet *validCharacters = (id)[NSMutableCharacterSet alphanumericCharacterSet];
		[validCharacters formUnionWithCharacterSet:[NSCharacterSet characterSetWithCharactersInString:@".-()"]];
		postFix = [[bundle objectForInfoDictionaryKey:@"CFBundleVersion"] stringByTrimmingCharactersInSet:[validCharacters invertedSet]];
	}
	else
		postFix = @"old"; */
  
  // Instead, just use a standard postFix
  NSString *postFix = @"old";
  
	NSString *prefix = [[path stringByDeletingPathExtension] stringByAppendingFormat:@" (%@)", postFix];
	NSString *tempDir = [prefix stringByAppendingPathExtension:[path pathExtension]];
	// Now let's make sure we get a unique path.
	int cnt=2;
	while ([[NSFileManager defaultManager] fileExistsAtPath:tempDir] && cnt <= 999999)
  {
		tempDir = [NSString stringWithFormat:@"%@ %d.%@", prefix, cnt++, [path pathExtension]];
  }
	return tempDir;
}

- (BOOL)_copyPathWithForcedAuthentication:(NSString *)src toPath:(NSString *)dst error:(NSError **)error
{
  
	NSString *tmp = [self _temporaryCopyNameForPath:dst];
	const char* srcPath = [src fileSystemRepresentation];
	const char* tmpPath = [tmp fileSystemRepresentation];
	const char* dstPath = [dst fileSystemRepresentation];
	
	struct stat dstSB;
	stat(dstPath, &dstSB);
	
	AuthorizationRef auth = NULL;
	OSStatus authStat = errAuthorizationDenied;
	while (authStat == errAuthorizationDenied) {
		authStat = AuthorizationCreate(NULL,
                                   kAuthorizationEmptyEnvironment,
                                   kAuthorizationFlagDefaults,
                                   &auth);
	}
	
	BOOL res = NO;
	if (authStat == errAuthorizationSuccess) {
		res = YES;
		
		char uidgid[42];
		snprintf(uidgid, sizeof(uidgid), "%d:%d",
             dstSB.st_uid, dstSB.st_gid);
		
		const char* executables[] = {
			"/bin/rm",
			"/bin/mv",
			"/bin/mv",
			"/bin/rm",
			NULL,  // pause here and do some housekeeping before
			// continuing
			"/usr/sbin/chown",
			NULL   // stop here for real
		};
		
		// 4 is the maximum number of arguments to any command,
		// including the NULL that signals the end of an argument
		// list.
		const char* const argumentLists[][4] = {
			{ "-rf", tmpPath, NULL }, // make room for the temporary file... this is kinda unsafe; should probably do something better.
			{ "-f", dstPath, tmpPath, NULL },  // mv
			{ "-f", srcPath, dstPath, NULL },  // mv
			{ "-rf", tmpPath, NULL },  // rm
			{ NULL },  // pause
			{ "-R", uidgid, dstPath, NULL },  // chown
			{ NULL }  // stop
		};
		
		// Process the commands up until the first NULL
		int commandIndex = 0;
		for (; executables[commandIndex] != NULL; ++commandIndex) {
			if (res)
				res = AuthorizationExecuteWithPrivilegesAndWait(auth, executables[commandIndex], kAuthorizationFlagDefaults, argumentLists[commandIndex]);
		}
		
		// If the currently-running application is trusted, the new
		// version should be trusted as well.  Remove it from the
		// quarantine to avoid a delay at launch, and to avoid
		// presenting the user with a confusing trust dialog.
		//
		// This needs to be done after the application is moved to its
		// new home with "mv" in case it's moved across filesystems: if
		// that happens, "mv" actually performs a copy and may result
		// in the application being quarantined.  It also needs to be
		// done before "chown" changes ownership, because the ownership
		// change will almost certainly make it impossible to change
		// attributes to release the files from the quarantine.
		if (res) {
			[self releaseFromQuarantine:dst];
		}
		
		// Now move past the NULL we found and continue executing
		// commands from the list.
		++commandIndex;
		
		for (; executables[commandIndex] != NULL; ++commandIndex) {
			if (res)
				res = AuthorizationExecuteWithPrivilegesAndWait(auth, executables[commandIndex], kAuthorizationFlagDefaults, argumentLists[commandIndex]);
		}
		
		AuthorizationFree(auth, 0);
		
		if (!res)
		{
			// Something went wrong somewhere along the way, but we're not sure exactly where.
			NSString *errorMessage = [NSString stringWithFormat:@"Authenticated file copy from %@ to %@ failed.", src, dst];
			if (error != NULL)
				*error = [NSError errorWithDomain:SUSparkleErrorDomain code:SUAuthenticationFailure userInfo:[NSDictionary dictionaryWithObject:errorMessage forKey:NSLocalizedDescriptionKey]];
		}
	}
	else
	{
		if (error != NULL)
			*error = [NSError errorWithDomain:SUSparkleErrorDomain code:SUAuthenticationFailure userInfo:[NSDictionary dictionaryWithObject:@"Couldn't get permission to authenticate." forKey:NSLocalizedDescriptionKey]];
	}
	return res;
}

- (BOOL)copyPathWithAuthentication:(NSString *)src overPath:(NSString *)dst error:(NSError **)error
{
	if (![[NSFileManager defaultManager] fileExistsAtPath:dst])
	{
		NSString *errorMessage = [NSString stringWithFormat:@"Couldn't copy %@ over %@ because there is no file at %@.", src, dst, dst];
		if (error != NULL)
			*error = [NSError errorWithDomain:SUSparkleErrorDomain code:SUFileCopyFailure userInfo:[NSDictionary dictionaryWithObject:errorMessage forKey:NSLocalizedDescriptionKey]];
		return NO;
	}
  
	if (![[NSFileManager defaultManager] isWritableFileAtPath:dst] || ![[NSFileManager defaultManager] isWritableFileAtPath:[dst stringByDeletingLastPathComponent]])
		return [self _copyPathWithForcedAuthentication:src toPath:dst error:error];
  
	NSString *tmpPath = [self _temporaryCopyNameForPath:dst];
  
	if (![[NSFileManager defaultManager] movePath:dst toPath:tmpPath handler:self])
	{
		if (error != NULL)
			*error = [NSError errorWithDomain:SUSparkleErrorDomain code:SUFileCopyFailure userInfo:[NSDictionary dictionaryWithObject:[NSString stringWithFormat:@"Couldn't move %@ to %@.", dst, tmpPath] forKey:NSLocalizedDescriptionKey]];
		return NO;			
	}
  
	if (![[NSFileManager defaultManager] copyPath:src toPath:dst handler:self])
	{
		if (error != NULL)
			*error = [NSError errorWithDomain:SUSparkleErrorDomain code:SUFileCopyFailure userInfo:[NSDictionary dictionaryWithObject:[NSString stringWithFormat:@"Couldn't copy %@ to %@.", src, dst] forKey:NSLocalizedDescriptionKey]];
		return NO;			
	}
	
	// Trash the old copy of the app.
	NSInteger tag = 0;
	if (![[NSWorkspace sharedWorkspace] performFileOperation:NSWorkspaceRecycleOperation source:[tmpPath stringByDeletingLastPathComponent] destination:@"" files:[NSArray arrayWithObject:[tmpPath lastPathComponent]] tag:&tag])
		NSLog(@"Sparkle error: couldn't move %@ to the trash. This is often a sign of a permissions error.", tmpPath);
  
	// If the currently-running application is trusted, the new
	// version should be trusted as well.  Remove it from the
	// quarantine to avoid a delay at launch, and to avoid
	// presenting the user with a confusing trust dialog.
	//
	// This needs to be done after the application is moved to its
	// new home in case it's moved across filesystems: if that
	// happens, the move is actually a copy, and it may result
	// in the application being quarantined.
	[self releaseFromQuarantine:dst];
	return YES;
}

- (int)removeXAttr:(const char*)name
          fromFile:(NSString*)file
           options:(int)options
{
	typedef int (*removexattr_type)(const char*, const char*, int);
	// Reference removexattr directly, it's in the SDK.
	static removexattr_type removexattr_func = removexattr;
	
	// Make sure that the symbol is present.  This checks the deployment
	// target instead of the SDK so that it's able to catch dlsym failures
	// as well as the null symbol that would result from building with the
	// 10.4 SDK and a lower deployment target, and running on 10.3.
	if (!removexattr_func) {
		errno = ENOSYS;
		return -1;
	}
	
	const char* path = NULL;
	@try {
		path = [file fileSystemRepresentation];
	}
	@catch (id exception) {
		// -[NSString fileSystemRepresentation] throws an exception if it's
		// unable to convert the string to something suitable.  Map that to
		// EDOM, "argument out of domain", which sort of conveys that there
		// was a conversion failure.
		errno = EDOM;
		return -1;
	}
	
	return removexattr_func(path, name, options);
}

- (void)releaseFromQuarantine:(NSString*)root
{
	const char* quarantineAttribute = "com.apple.quarantine";
	const int removeXAttrOptions = XATTR_NOFOLLOW;
	
	[self removeXAttr:quarantineAttribute
           fromFile:root
            options:removeXAttrOptions];
	
	// Only recurse if it's actually a directory.  Don't recurse into a
	// root-level symbolic link.
	NSDictionary* rootAttributes =
	[[NSFileManager defaultManager] fileAttributesAtPath:root traverseLink:NO];
	NSString* rootType = [rootAttributes objectForKey:NSFileType];
	
	if (rootType == NSFileTypeDirectory) {
		// The NSDirectoryEnumerator will avoid recursing into any contained
		// symbolic links, so no further type checks are needed.
		NSDirectoryEnumerator* directoryEnumerator = [[NSFileManager defaultManager] enumeratorAtPath:root];
		NSString* file = nil;
		while ((file = [directoryEnumerator nextObject])) {
			[self removeXAttr:quarantineAttribute
               fromFile:[root stringByAppendingPathComponent:file]
                options:removeXAttrOptions];
		}
	}
}

@end
