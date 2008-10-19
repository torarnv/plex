//
//  BackgroundMusicPlayer.h
//  Plex
//
//  Created by James Clarke on 08/09/2008.
//

#import <Cocoa/Cocoa.h>
#import <QTKit/QTKit.h>


@interface BackgroundMusicPlayer : NSObject {
  BOOL isAvailable;
  BOOL isEnabled;
  BOOL isThemeMusicEnabled;
  BOOL isThemeDownloadingEnabled;
  BOOL isPlaying;
  
  float volume;
  int volumeFadeLevel;
  int targetVolumeFade;
  int fadeIncrement;
  int globalVolumeAsPercent;
  
  
  NSString *mainMusicPath;
  NSString *themeMusicPath;
  NSArray *mainMusicNames;
  NSArray *themeMusicNames;
  NSArray *remoteThemeMusicNames;
  NSDate *remoteThemeListCacheDate;
  
  NSString *currentTheme;
  
  QTMovie *mainMusic, *themeMusic;
}

+ (BackgroundMusicPlayer *)sharedInstance;

- (BOOL)enabled;
- (void)setEnabled:(BOOL)enabled;
- (BOOL)themeMusicEnabled;
- (void)setThemeMusicEnabled:(BOOL)enabled;
- (void)setThemeDownloadsEnabled:(BOOL)enabled;
- (NSString*)safeThemeName:(NSString*)themeMusicName;

- (void)checkForThemeNamed:(NSString*)themeMusicName;

- (BOOL)isPlaying;

- (void)startMusic;
- (void)stopMusic;
- (void)loadNextTrack;
- (void)setThemeMusicName:(NSString*)themeMusicName;

- (void)updateMusicVolume;
- (float)volume;
- (void)setVolume:(float)newVolume;
- (void)setGlobalVolumeAsPercent:(int)newGlobalVolumeAsPercent;
- (void)fadeToTheme:(BOOL)toTheme;
- (void)adjustVolumeFadeLevel;

@end
