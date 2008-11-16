//
//  BackgroundMusicPlayer.m
//  Plex
//
//  Created by James Clarke on 08/09/2008.
//

#import "BackgroundMusicPlayer.h"

#import "CocoaToCppThunk.h"

#define BACKGROUND_MUSIC_APP_SUPPORT_SUBDIR       @"/Plex/Background Music"
#define BACKGROUND_MUSIC_THEME_DOWNLOADS_ENABLED
#define BACKGROUND_MUSIC_THEME_DOWNLOAD_URL       @"http://tvthemes.plexapp.com"
#define BACKGROUND_MUSIC_THEME_REQ_LIMIT          3600
@implementation BackgroundMusicPlayer

static BackgroundMusicPlayer *_o_sharedMainInstance = nil;

+ (BackgroundMusicPlayer *)sharedInstance
{
  return _o_sharedMainInstance ? _o_sharedMainInstance : [[self alloc] init];
}

- (id)init
{
  if( _o_sharedMainInstance)
    [self dealloc];
  else
    _o_sharedMainInstance = [super init];
  
  // Default values
  isThemeMusicEnabled = NO;
  isThemeDownloadingEnabled = NO;
  isPlaying = NO;
  volumeFadeLevel = 100;
  targetVolumeFade = 100;
  fadeIncrement = 5;
  
  // Set the default volume to 50% (overridden in settings)
  volume = 0.5f;
  globalVolumeAsPercent = 100;
  
  // Get Application Support directory
  NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
  if ([paths count] == 1) {
    // Get paths for background music
    NSString* backgroundMusicPath = [[paths objectAtIndex:0] stringByAppendingPathComponent:BACKGROUND_MUSIC_APP_SUPPORT_SUBDIR];
    mainMusicPath = [backgroundMusicPath stringByAppendingPathComponent:@"Main"];
    themeMusicPath = [backgroundMusicPath stringByAppendingPathComponent:@"Themes"];
  }
  
  // Load available music into the array
  NSArray* validExtensions = [NSArray arrayWithObjects:@".mp3", @".m4a", @".m4p", nil];  // File types supported by QuickTime
  NSArray* directoryContents = [NSArray arrayWithArray:[[[NSFileManager defaultManager] enumeratorAtPath:mainMusicPath] allObjects]];
  NSMutableArray* validFiles = [[NSMutableArray alloc] init];
  
  // Check each file's extension to ensure it's a supported format
  for (NSString* fileName in directoryContents)
  {
    NSString* ext = [[fileName substringFromIndex:[[fileName stringByDeletingPathExtension] length]] lowercaseString];
    if ([validExtensions containsObject:ext])
      [validFiles addObject:fileName];
  }
  
  // Create the main music names array containing valid files
  mainMusicNames = [NSArray arrayWithArray:validFiles];
  [validFiles release];
  
  // Create a dictionary to store theme music requests
  themeMusicRequests = [[NSMutableDictionary alloc] init];
  
  // Register for notification when tracks finish playing
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(trackDidEnd:) name:QTMovieDidEndNotification object:mainMusic];
  
  // Load a random file
  [self loadNextTrack];
  
  return _o_sharedMainInstance;
}

- (void)dealloc
{
  [themeMusicRequests release];
  [super dealloc];
}

- (BOOL)themeMusicEnabled { return isThemeMusicEnabled; }
- (void)setThemeMusicEnabled:(BOOL)enabled { isThemeMusicEnabled = enabled; }

- (BOOL)themeDownloadsEnabled { return isThemeDownloadingEnabled; }
- (void)setThemeDownloadsEnabled:(BOOL)enabled { isThemeDownloadingEnabled = enabled; }

- (void)checkForThemeWithId:(NSString*)tvShowId
{
  if (isThemeMusicEnabled && isThemeDownloadingEnabled)
  {
    // If there's already a theme file, return
    NSString *localFile = [themeMusicPath stringByAppendingPathComponent:[NSString stringWithFormat:@"%@.mp3", tvShowId]];
    if ([[NSFileManager defaultManager] fileExistsAtPath:localFile]) return;

    // If the theme has been requested recently, return
    NSDate* lastRequestDate = [themeMusicRequests objectForKey:tvShowId];
    if (lastRequestDate != nil)
      if ([lastRequestDate timeIntervalSinceNow] > -BACKGROUND_MUSIC_THEME_REQ_LIMIT)
      {
        NSLog(@"Skipping download of theme %@ - recently requested", tvShowId);
        return;
      }
    
    // Construct the theme URL and attempt to download the file
    NSString *remoteFile = [[NSString stringWithFormat:@"%@/%@.mp3", BACKGROUND_MUSIC_THEME_DOWNLOAD_URL, tvShowId]
                              stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
    [themeMusicRequests setObject:[NSDate date] forKey:tvShowId];
    Cocoa_DownloadFile([remoteFile UTF8String], [localFile UTF8String]);
  }
}

- (BOOL)isPlaying { return isPlaying; }

- (void)startMusic
{
  if (!isPlaying) {
    if (currentId == nil)
      [mainMusic play];
    else
    {
      [themeMusic gotoBeginning]; // Make sure theme music always starts at the beginning when returning from video playback
      [themeMusic play];
    }
    isPlaying = YES;
  }
}

- (void)stopMusic
{
  if (isPlaying) {
    if (currentId == nil)
      [mainMusic stop];
    else
      [themeMusic stop];
    isPlaying = NO;
  }
}

- (void)loadNextTrack
{
  if ([mainMusicNames count] > 0) {
    // Load a random track from the mainMusicNames array
    if (mainMusic != nil) [mainMusic release];
    int index = (random() % [mainMusicNames count]);
    
    NSError *qtError = nil;
    mainMusic = [[QTMovie alloc] initWithFile:[mainMusicPath stringByAppendingPathComponent:[mainMusicNames objectAtIndex:index]] error:&qtError];
    
    // If an error occurs (unsupported file) start from the beginning again
    if (qtError != nil) {
      [self loadNextTrack];
      return;
    }
    
    // Set the volume level
    [self updateMusicVolume];
    
    //Start playing if required
    if (isPlaying && (currentId == nil)) [mainMusic play];
  }
}

- (void)trackDidEnd:(NSNotification *)aNotification
{
  // When a track ends, load the next one
  if ([aNotification object] == mainMusic)
    [self loadNextTrack];
}

- (void)setThemeMusicId:(NSString*)newId;
{
  // If we should play music & the theme given is different to the current one...
  if (isThemeMusicEnabled)
  {
    // If the theme is nil, restart the background music
    if (newId == nil)
    {
      if ((currentId != nil) && (isPlaying))
      {
        [self fadeToTheme:NO];
        [mainMusic play];
      }
      currentId = nil;
    }

    else
    {
      // If given a new theme, see if there's a file available
      if (![newId isEqualToString:currentId])
      {
        NSString *localFile = [themeMusicPath stringByAppendingPathComponent:[NSString stringWithFormat:@"%@.mp3", newId]];
        if ([[NSFileManager defaultManager] fileExistsAtPath:localFile])
        {
          currentId = newId;
          themeMusic = [[QTMovie alloc] initWithFile:localFile error:nil];  
          [self updateMusicVolume];
          if (isPlaying) {
            [themeMusic play];
            [self fadeToTheme:YES];
          }
          return;
        }
      }
    }
  }
}

- (void)updateMusicVolume
{
  // Update the volume of mainMusic & themeMusic using the background music volume setting in relation to the global system volume
  if (mainMusic != nil) [mainMusic setVolume:(float)(volume * (globalVolumeAsPercent / 100.0f) * (volumeFadeLevel / 100.0f))];
  if (themeMusic != nil) [themeMusic setVolume:(float)(volume * (globalVolumeAsPercent / 100.0f) * ((100 - volumeFadeLevel) / 100.0f))];
}

- (float)volume
{
  return volume;
}

- (void)setVolume:(float)newVolume
{
  volume = newVolume;
  [self updateMusicVolume];
}

- (void)setGlobalVolumeAsPercent:(int)newGlobalVolumeAsPercent
{
  globalVolumeAsPercent = newGlobalVolumeAsPercent;
  [self updateMusicVolume]; 
}

- (void)fadeToTheme:(BOOL)toTheme
{
  if (toTheme) targetVolumeFade = 0;
  else targetVolumeFade = 100;
  [self adjustVolumeFadeLevel];
}

- (void)adjustVolumeFadeLevel
{
  // Fade between main & theme music
  if (volumeFadeLevel < targetVolumeFade) volumeFadeLevel += fadeIncrement;
  else if (volumeFadeLevel > targetVolumeFade) volumeFadeLevel -= fadeIncrement;
  
  // If the target's not reached yet, reschedule the timer
  if (volumeFadeLevel != targetVolumeFade)
    [NSTimer scheduledTimerWithTimeInterval:0.01 target:self selector:@selector(adjustVolumeFadeLevel) userInfo:nil repeats:NO];
  
  // Stop the silenced track (release if theme music)
  if ((volumeFadeLevel == 0) && isPlaying)
    if (mainMusic != nil) [mainMusic stop];
  if ((volumeFadeLevel == 100) && isPlaying)
    if (themeMusic != nil)
    {
      [themeMusic stop];
      [themeMusic release];
      themeMusic = nil;
    }
  
  // Update the volume
  [self updateMusicVolume];
}

@end
