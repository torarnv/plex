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
#define BACKGROUND_MUSIC_THEME_LIST_CACHE_TIME    3600
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
  isEnabled = NO;
  isThemeMusicEnabled = NO;
  isThemeDownloadingEnabled = NO;
  isPlaying = NO;
  isAvailable = NO;
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
    
    // Check whether the main music directory is available
    BOOL isDir = NO;
    if ([[NSFileManager defaultManager] fileExistsAtPath:mainMusicPath isDirectory:&isDir] && isDir)
    {
      // Seed random number generator & set availability
      srandom(time(NULL));
      isAvailable = YES;
    }
  }
  
  return _o_sharedMainInstance;
}

- (BOOL)enabled { return isEnabled; }
- (void)setEnabled:(BOOL)enabled
{
  if (isAvailable)
  {
    if (enabled != isEnabled)
    {
      isEnabled = enabled;
      
      // If enabling, load a track ready for playback. Otherwise, release the unused QTMovie.
      if (enabled)
      {
        // Register for notification when tracks finish playing
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(trackDidEnd:) name:QTMovieDidEndNotification object:mainMusic];
        
        // Load available music into the array
        mainMusicNames = [[[NSFileManager defaultManager] enumeratorAtPath:mainMusicPath] allObjects];
        
        // Load a random file
        [self loadNextTrack];
      }
      else if (mainMusic != nil)
      {
        // Remove from the list of observers
        [[NSNotificationCenter defaultCenter] removeObserver:self];
        
        // Release objects so memory isn't used when music is disabled
        [mainMusic release];
        mainMusic = nil; // Must be set to nil or Plex crashes when disabling then enabling again
        [mainMusicNames release];
        [themeMusicNames release];
      }
    }
  }
}

- (BOOL)themeMusicEnabled { return isThemeMusicEnabled; }
- (void)setThemeMusicEnabled:(BOOL)enabled { isThemeMusicEnabled = enabled; }

- (BOOL)themeDownloadsEnabled { return isThemeDownloadingEnabled; }
- (void)setThemeDownloadsEnabled:(BOOL)enabled { isThemeDownloadingEnabled = enabled; }

- (void)checkForThemeWithId:(NSString*)tvShowId
{
#ifdef BACKGROUND_MUSIC_THEME_DOWNLOADS_ENABLED
  if (isEnabled && isThemeMusicEnabled && isThemeDownloadingEnabled)
  {
    // If there's already a theme file, return
    NSString *localFile = [themeMusicPath stringByAppendingPathComponent:[NSString stringWithFormat:@"%@.mp3", tvShowId]];
    if ([[NSFileManager defaultManager] fileExistsAtPath:localFile]) return;

    // Construct the theme URL and attempt to download the file
    NSString *remoteFile = [[NSString stringWithFormat:@"%@/%@.mp3", BACKGROUND_MUSIC_THEME_DOWNLOAD_URL, tvShowId]
                              stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding];
    Cocoa_DownloadFile([remoteFile UTF8String], [localFile UTF8String]);
  }
#endif
}

- (BOOL)isPlaying { return isPlaying; }

- (void)startMusic
{
  if ((!isPlaying) && isAvailable) {
    if (isEnabled)
    {
      if (currentId == nil)
        [mainMusic play];
      else
      {
        [themeMusic gotoBeginning]; // Make sure theme music always starts at the beginning when returning from video playback
        [themeMusic play];
      }
    }
    isPlaying = YES;
  }
}

- (void)stopMusic
{
  if ((isPlaying) && isAvailable) {
    if (isEnabled) 
    {
      if (currentId == nil)
        [mainMusic stop];
      else
        [themeMusic stop];
    }
    isPlaying = NO;
  }
}

- (void)loadNextTrack
{
  if (isEnabled && isAvailable && ([mainMusicNames count] > 0)) {
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
  if (isEnabled && isAvailable && isThemeMusicEnabled)
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
  if (isAvailable)
  {
    volume = newVolume;
    [self updateMusicVolume];
  }
}

- (void)setGlobalVolumeAsPercent:(int)newGlobalVolumeAsPercent
{
  if (isAvailable)
  {
    globalVolumeAsPercent = newGlobalVolumeAsPercent;
    [self updateMusicVolume]; 
  }
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
  if ((volumeFadeLevel == 0) && isPlaying && isEnabled && isAvailable)
    if (mainMusic != nil) [mainMusic stop];
  if ((volumeFadeLevel == 100) && isPlaying && isEnabled && isAvailable)
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
