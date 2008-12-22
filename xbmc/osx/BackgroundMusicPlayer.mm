//
//  BackgroundMusicPlayer.m
//  Plex
//
//  Created by James Clarke on 08/09/2008.
//

#import "BackgroundMusicPlayer.h"

#import "CocoaToCppThunk.h"

#define BACKGROUND_MUSIC_APP_SUPPORT_SUBDIR       @"/Plex/Background Music"
#define BACKGROUND_MUSIC_THEME_DOWNLOAD_URL       @"http://tvthemes.plexapp.com"
#define BACKGROUND_MUSIC_THEME_REQ_LIMIT          3600
#define BACKGROUND_MUSIC_FADE_DELAY               0.5

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
  isFocused = NO;
  volumeFadeLevel = 100;
  volumeCrossFadeLevel = 100;
  targetVolumeFade = 100;
  targetVolumeCrossFade = 100;
  fadeIncrement = 5;
  
  // Set the default volume to 50% (overridden in settings)
  volume = 0.5f;
  globalVolumeAsPercent = 100;
  
  // Get Application Support directory
  NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
  if ([paths count] == 1) 
  {
    // Get paths for background music
    NSString* backgroundMusicPath = [[paths objectAtIndex:0] stringByAppendingPathComponent:BACKGROUND_MUSIC_APP_SUPPORT_SUBDIR];
    mainMusicPath = [[backgroundMusicPath stringByAppendingPathComponent:@"Main"] retain];
    themeMusicPath = [[backgroundMusicPath stringByAppendingPathComponent:@"Themes"] retain];
  }
  
  // Load available music into the array
  NSArray* validExtensions = [NSArray arrayWithObjects:@".mp3", @".m4a", @".m4p", nil];  // File types supported by QuickTime
  NSArray* directoryContents = [NSArray arrayWithArray:[[[NSFileManager defaultManager] enumeratorAtPath:mainMusicPath] allObjects]];
  
  // Check each file's extension to ensure it's a supported format
  // Create the main music names array containing valid files
  mainMusicNames = [[NSMutableArray alloc] initWithCapacity:[directoryContents count]];
  for (NSString* fileName in directoryContents)
  {
    NSString* ext = [[fileName substringFromIndex:[[fileName stringByDeletingPathExtension] length]] lowercaseString];
    if ([validExtensions containsObject:ext])
      [mainMusicNames addObject:fileName];
  }
  
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
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [mainMusic release], mainMusic = nil;
  [themeMusic release], themeMusic = nil;
  [fadeTimer invalidate];
  [fadeTimer release], fadeTimer = nil;
  [crossFadeTimer invalidate];
  [crossFadeTimer release], crossFadeTimer = nil;
  [themeMusicRequests release], themeMusicRequests = nil;
  [mainMusicNames release], mainMusicNames = nil;
  [mainMusicPath release], mainMusicPath = nil;
  [themeMusicPath release], themeMusicPath = nil;
  [super dealloc];
}

- (BOOL)enabled 
{
  return isEnabled; 
}

- (void)setEnabled:(BOOL)enabled
{
  if (isEnabled != enabled)
  {
    isEnabled = enabled;
    if (enabled)
    {
      [self loadNextTrack];
    }
    else
    {
      [mainMusic release], mainMusic = nil;
      [themeMusic release], themeMusic = nil;
    }
  }
}

- (BOOL)themeMusicEnabled { return isThemeMusicEnabled; }
- (void)setThemeMusicEnabled:(BOOL)enabled { isThemeMusicEnabled = enabled; }

- (BOOL)themeDownloadsEnabled { return isThemeDownloadingEnabled; }
- (void)setThemeDownloadsEnabled:(BOOL)enabled { isThemeDownloadingEnabled = enabled; }

- (void)checkForThemeWithId:(NSString*)tvShowId
{
  if (isEnabled && isThemeMusicEnabled && isThemeDownloadingEnabled)
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

- (void)pause
{
  [self fadeAudioTo:[NSNumber numberWithInt:0]];
}

- (void)play
{
  [self fadeAudioTo:[NSNumber numberWithInt:100]];
}

- (BOOL)isPlaying 
{ 
  return isPlaying; 
}

- (void)startMusic
{
  if (!isPlaying) {
    if (isEnabled && isFocused)
    {
      if (currentId == nil)
      {
        [mainMusic play];
      }
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
  if (isPlaying) {
    if (isEnabled)
    {
      [mainMusic stop];
      [themeMusic stop];
    }
    isPlaying = NO;
  }
}

- (void)loadNextTrack
{
  if ([mainMusicNames count] > 0) {
    // Load a random track from the mainMusicNames array
    [mainMusic release];
    int index = (random() % [mainMusicNames count]);
    
    NSError *qtError = nil;
    mainMusic = [[QTMovie alloc] initWithFile:[mainMusicPath stringByAppendingPathComponent:[mainMusicNames objectAtIndex:index]] error:&qtError];
    
    // If an error occurs (unsupported file) start from the beginning again
    if (qtError != nil) {
      // Maybe we should remove the unsupported file from the mainMusicNames array?
      // TODO: Potential endless loop
      [self loadNextTrack];
      return;
    }
    
    // Set the volume level
    [self updateMusicVolume];
    
    //Start playing if required
    if (isEnabled && isPlaying && (currentId == nil) && isFocused)
      [mainMusic play];
  }
}

- (void)trackDidEnd:(NSNotification *)aNotification
{
  // When a track ends, load the next one
  if ([aNotification object] && [aNotification object] == mainMusic)
    [self loadNextTrack];
}

- (void)checkThemeCrossFade:(NSTimer *)timer
{
  if (!themeMusic)
  {
    [timer invalidate];
    return;
  }
  double duration = [themeMusic duration].timeValue - (0.6 * [themeMusic duration].timeScale);
  if ([themeMusic currentTime].timeValue > duration)
  {
    [timer invalidate];
    [self setThemeMusicId:nil];
  }
}

- (void)setThemeMusicId:(NSString*)newId;
{
  // If we should play music & the theme given is different to the current one...
  if (isEnabled && isThemeMusicEnabled)
  {
    // If the theme is nil, restart the background music
    if (newId == nil)
    {
      if ((currentId != nil) && (isPlaying) && isFocused)
      {
        [self crossFadeToTheme:NO];
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
          if (isPlaying && isFocused)
          {
            [NSTimer scheduledTimerWithTimeInterval:1.0 target:self selector:@selector(checkThemeCrossFade:) userInfo:nil repeats:YES];
            [themeMusic play];
            [self crossFadeToTheme:YES];
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
  [mainMusic setVolume:(float)(volume * (globalVolumeAsPercent / 100.0f) * (volumeCrossFadeLevel / 100.0f) * (volumeFadeLevel / 100.0f))];
  [themeMusic setVolume:(float)(volume * (globalVolumeAsPercent / 100.0f) * ((100 - volumeCrossFadeLevel) / 100.0f) * (volumeFadeLevel / 100.0f))];
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

- (void)fadeAudioTo:(NSNumber *)theTargetVolume
{
  if (!isPlaying || !isEnabled)
    return;

  float targetVolume = [theTargetVolume floatValue];
  if (targetVolume < 0)
    targetVolume = 0;
  else if (targetVolume > 100)
    targetVolume = 100;

  if ((targetVolume != targetVolumeFade) && isFocused || (!isFocused && targetVolume < volumeFadeLevel))
  {
    targetVolumeFade = targetVolume;
    NSTimeInterval duration = 0.6;
    NSTimeInterval interval = duration / (fabsf(targetVolumeFade - volumeFadeLevel) / (float)(fadeIncrement <= 0 ? 5 : fadeIncrement));
    if (interval == 0)
    {
      volumeFadeLevel = targetVolumeFade;
    }
    else
    {
      [fadeTimer invalidate];
      [fadeTimer release], fadeTimer = nil;
      if (volumeFadeLevel == targetVolumeFade)
        return;
      fadeTimer = [[NSTimer scheduledTimerWithTimeInterval:interval target:self selector:@selector(adjustVolumeFadeLevel) userInfo:nil repeats:YES] retain];
    }
  }
  [self updateMusicVolume];
  if (volumeFadeLevel > 0 || targetVolumeFade > 0)
    [mainMusic play];
}

- (void)crossFadeToTheme:(BOOL)toTheme
{
  if (toTheme) 
    targetVolumeCrossFade = 0;
  else 
    targetVolumeCrossFade = 100;

  NSTimeInterval duration = 0.6; //(toTheme ? 0.6 : ([themeMusic duration].timeValue - [themeMusic currentTime].timeValue) / [themeMusic currentTime].timeScale);
  NSTimeInterval interval = duration / (fabsf(targetVolumeCrossFade - volumeCrossFadeLevel) / (float)(fadeIncrement <= 0 ? 1 : fadeIncrement));
  if (interval == 0)
  {
    volumeCrossFadeLevel = targetVolumeCrossFade;
    [self updateMusicVolume];
  }
  else
  {
    [crossFadeTimer invalidate];
    [crossFadeTimer release], crossFadeTimer = nil;
    if (volumeCrossFadeLevel == targetVolumeCrossFade)
      return;
    crossFadeTimer = [[NSTimer scheduledTimerWithTimeInterval:interval target:self selector:@selector(adjustVolumeCrossFadeLevel) userInfo:nil repeats:YES] retain];
  }
}

- (void)adjustVolumeFadeLevel
{
  if (volumeFadeLevel < targetVolumeFade)
    volumeFadeLevel += fadeIncrement;
  else if (volumeFadeLevel > targetVolumeFade) 
    volumeFadeLevel -= fadeIncrement;
  
  if (volumeFadeLevel < 0)
    volumeFadeLevel = 0;
  else if (volumeFadeLevel > 100)
    volumeFadeLevel = 100;
  
  // If the target's reached then kill the timer
  if (volumeFadeLevel == targetVolumeFade)
  {
    [fadeTimer invalidate];
    [fadeTimer release], fadeTimer = nil;
  }
  
  // Stop the silenced track (release if theme music)
  if (volumeFadeLevel <= 0)
  {
    [mainMusic stop];
    [themeMusic stop];
  }
  
  // Update the volume
  [self updateMusicVolume];
}

- (void)adjustVolumeCrossFadeLevel
{
  if (volumeCrossFadeLevel < targetVolumeCrossFade)
    volumeCrossFadeLevel += fadeIncrement;
  else if (volumeCrossFadeLevel > targetVolumeCrossFade) 
    volumeCrossFadeLevel -= fadeIncrement;
  
  if (volumeCrossFadeLevel < 0)
    volumeCrossFadeLevel = 0;
  else if (volumeCrossFadeLevel > 100)
    volumeCrossFadeLevel = 100;
  
  // If the target's reached then kill the timer
  if (volumeCrossFadeLevel == targetVolumeCrossFade)
  {
    [crossFadeTimer invalidate];
    [crossFadeTimer release], crossFadeTimer = nil;
  }
  
  // Stop the silenced track (release if theme music)
  if (volumeCrossFadeLevel <= 0)
  {
    [mainMusic stop];
  }
  else if (volumeCrossFadeLevel >= 100)
  {
    [themeMusic stop];
    [themeMusic release], themeMusic = nil;
  }
  
  // Update the volume
  [self updateMusicVolume];
}

- (void)foundFocus
{
  isFocused = YES;
  
  if (isPlaying && isEnabled)
  {
    // Cancel previous pause or play calls
    [NSObject cancelPreviousPerformRequestsWithTarget:self];

    // Fade the audio in after we wait .5 seconds
    [self performSelector:@selector(play) withObject:nil afterDelay:0.5];  // wait 2 seconds before we try to pause
  }
}

- (void)lostFocus
{
  isFocused = NO;
  
  if (isPlaying && isEnabled)
  {
    // Cancel previous pause or play calls
    [NSObject cancelPreviousPerformRequestsWithTarget:self];

    // Play the audio at 50% while we don't have the focus
    [self performSelector:@selector(fadeAudioTo:) withObject:[NSNumber numberWithInt:15] afterDelay:0.1];

    // Play the audio at 50% for 2 seconds before we completely fade it out
    [self performSelector:@selector(pause) withObject:nil afterDelay:5.0];
  }
}
@end
