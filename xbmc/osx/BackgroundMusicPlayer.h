//
//  BackgroundMusicPlayer.h
//  Plex
//
//  Created by James Clarke on 08/09/2008.
//

#import <Cocoa/Cocoa.h>
#import <QTKit/QTKit.h>


@interface BackgroundMusicPlayer : NSObject {
  BOOL isEnabled;
  BOOL isThemeMusicEnabled;
  BOOL isThemeDownloadingEnabled;
  BOOL isPlaying;
  BOOL isPaused;
  
  float volume;
  int volumeFadeLevel;
  int volumeCrossFadeLevel;
  int targetVolumeFade;
  int targetVolumeCrossFade;
  int fadeIncrement;
  int globalVolumeAsPercent;
  
  NSString *mainMusicPath;
  NSString *themeMusicPath;
  NSMutableArray *mainMusicNames;
  NSMutableDictionary *themeMusicRequests;
  
  NSString *currentId;
  
  QTMovie *mainMusic, *themeMusic;
  NSTimer* fadeTimer;
  NSTimer* crossFadeTimer;
}

+ (BackgroundMusicPlayer *)sharedInstance;

- (BOOL)enabled;
- (void)setEnabled:(BOOL)enabled;
- (BOOL)themeMusicEnabled;
- (void)setThemeMusicEnabled:(BOOL)enabled;
- (void)setThemeDownloadsEnabled:(BOOL)enabled;

- (void)checkForThemeWithId:(NSString*)tvShowId;

- (void)pause;
- (void)play;
- (BOOL)isPlaying;

- (void)startMusic;
- (void)stopMusic;
- (void)loadNextTrack;
- (void)setThemeMusicId:(NSString*)newId;

- (void)updateMusicVolume;
- (float)volume;
- (void)setVolume:(float)newVolume;
- (void)setGlobalVolumeAsPercent:(int)newGlobalVolumeAsPercent;
- (void)crossFadeToTheme:(BOOL)toTheme;
- (void)fadeAudioTo:(NSNumber *)theTargetVolume;

- (void)foundFocus;
- (void)lostFocus;
@end
