#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

// REMOVE ME WHEN WE SWITCH TO SKIN VERSION 2.1
#define PRE_SKIN_VERSION_2_1_COMPATIBILITY 1
// REMOVE ME WHEN WE SWITCH TO SKIN VERSION 2.1

#include "settings/VideoSettings.h"
#include "StringUtils.h"
#include "GUISettings.h"
#include "Profile.h"
#include "MediaSource.h"
#include "XBVideoConfig.h"
#include "ViewState.h"

#include <vector>
#include <map>
#include <string>

using namespace std;

#define CACHE_AUDIO 0
#define CACHE_VIDEO 1
#define CACHE_VOB   2

#define VOLUME_MINIMUM -6000  // -60dB
#define VOLUME_MAXIMUM 0      // 0dB
#define VOLUME_DRC_MINIMUM 0    // 0dB
#define VOLUME_DRC_MAXIMUM 3000 // 30dB

#define VIEW_MODE_NORMAL        0
#define VIEW_MODE_ZOOM          1
#define VIEW_MODE_STRETCH_4x3   2
#define VIEW_MODE_STRETCH_14x9  3
#define VIEW_MODE_STRETCH_16x9  4
#define VIEW_MODE_ORIGINAL      5
#define VIEW_MODE_CUSTOM        6

#define STACK_NONE          0
#define STACK_SIMPLE        1

#define VIDEO_SHOW_ALL 0
#define VIDEO_SHOW_UNWATCHED 1
#define VIDEO_SHOW_WATCHED 2

#ifdef _XBOX
#define PROFILES_FILE "q:\\system\\profiles.xml"
#else
/* FIXME: eventaully the profile should dictate where t:\ is but for now it
   makes sense to leave all the profile settings in a user writeable location
   like t:\ */
#define PROFILES_FILE "t:\\profiles.xml"
#endif

class CSkinString
{
public:
  CStdString name;
  CStdString value;
};

class CSkinBool
{
public:
  CSkinBool() {}
  CSkinBool(const char* name, bool value)
    : name(name)
    , value(value) {}
  
  CStdString name;
  bool value;
};

struct VOICE_MASK {
  float energy;
  float pitch;
  float robotic;
  float whisper;
};

class CSettings
{
public:
  CSettings(void);
  virtual ~CSettings(void);

  bool Load(bool& bXboxMediacenter, bool& bSettings);
  void Save() const;
  bool Reset();

  void Clear();

  bool LoadProfile(int index);
  bool SaveSettingsToProfile(int index);
  bool DeleteProfile(int index);
  void CreateProfileFolders();

  VECSOURCES *GetSourcesFromType(const CStdString &type);
  CStdString GetDefaultSourceFromType(const CStdString &type);

  bool UpdateSource(const CStdString &strType, const CStdString strOldName, const CStdString &strUpdateChild, const CStdString &strUpdateValue);
  bool DeleteSource(const CStdString &strType, const CStdString strName, const CStdString strPath);
  bool UpdateShare(const CStdString &type, const CStdString oldName, const CMediaSource &share);
  bool AddShare(const CStdString &type, const CMediaSource &share);

  int TranslateSkinString(const CStdString &setting);
  const CStdString &GetSkinString(int setting) const;
  void SetSkinString(int setting, const CStdString &label);

  int TranslateSkinBool(const CStdString &setting);
  bool GetSkinBool(int setting) const;
  void SetSkinBool(int setting, bool set);

  void ResetSkinSetting(const CStdString &setting);
  void ResetSkinSettings();

  struct AdvancedSettings
  {
public:
    // multipath testing
    bool m_useMultipaths;
    bool m_DisableModChipDetection;

    int m_audioHeadRoom;
    float m_karaokeSyncDelay;

    float m_videoSubsDelayRange;
    float m_videoAudioDelayRange;
    int m_videoSmallStepBackSeconds;
    int m_videoSmallStepBackTries;
    int m_videoSmallStepBackDelay;
    bool m_videoUseTimeSeeking;
    int m_videoTimeSeekForward;
    int m_videoTimeSeekBackward;
    int m_videoTimeSeekForwardBig;
    int m_videoTimeSeekBackwardBig;
    int m_videoPercentSeekForward;
    int m_videoPercentSeekBackward;
    int m_videoPercentSeekForwardBig;
    int m_videoPercentSeekBackwardBig;
    bool m_musicUseTimeSeeking;
    int m_musicTimeSeekForward;
    int m_musicTimeSeekBackward;
    int m_musicTimeSeekForwardBig;
    int m_musicTimeSeekBackwardBig;
    int m_musicPercentSeekForward;
    int m_musicPercentSeekBackward;
    int m_musicPercentSeekForwardBig;
    int m_musicPercentSeekBackwardBig;
    int m_videoBlackBarColour;

    float m_slideshowBlackBarCompensation;
    float m_slideshowZoomAmount;
    float m_slideshowPanAmount;

    int m_lcdRows;
    int m_lcdColumns;
    int m_lcdAddress1;
    int m_lcdAddress2;
    int m_lcdAddress3;
    int m_lcdAddress4;

    int m_autoDetectPingTime;
    float m_playCountMinimumPercent;

    int m_songInfoDuration;
    int m_busyDialogDelay;
    int m_logLevel;
    CStdString m_cddbAddress;
    bool m_usePCDVDROM;
    bool m_noDVDROM;
    bool m_enableOpticalMedia;
    CStdString m_cachePath;
    bool m_displayRemoteCodes;
    CStdStringArray m_videoStackRegExps;
    CStdStringArray m_tvshowStackRegExps;
    CStdString m_tvshowMultiPartStackRegExp;
    CStdStringArray m_pathSubstitutions;
    int m_remoteRepeat;
    float m_controllerDeadzone;
    bool m_FTPShowCache;

    bool m_playlistAsFolders;
    bool m_detectAsUdf;

    int m_thumbSize;

    int m_sambaclienttimeout;
    CStdString m_sambadoscodepage;
    CStdString m_musicThumbs;
    CStdString m_dvdThumbs;

    bool m_bMusicLibraryHideAllItems;
    bool m_bMusicLibraryAllItemsOnBottom;
    bool m_bMusicLibraryAlbumsSortByArtistThenYear;
    CStdString m_strMusicLibraryAlbumFormat;
    CStdString m_strMusicLibraryAlbumFormatRight;
    bool m_prioritiseAPEv2tags;
    CStdString m_musicItemSeparator;
    CStdString m_videoItemSeparator;
    std::vector<CStdString> m_musicTagsFromFileFilters;

    bool m_bVideoLibraryHideAllItems;
    bool m_bVideoLibraryAllItemsOnBottom;
    bool m_bVideoLibraryHideRecentlyAddedItems;
    bool m_bVideoLibraryHideEmptySeries;
    bool m_bVideoLibraryCleanOnUpdate;
    bool m_sambastatfiles;
    bool m_bUseEvilB;
    std::vector<CStdString> m_vecTokens; // cleaning strings tied to language
    //TuxBox
    bool m_bTuxBoxSubMenuSelection;
    int m_iTuxBoxDefaultSubMenu;
    int m_iTuxBoxDefaultRootMenu;
    bool m_bTuxBoxAudioChannelSelection;
    bool m_bTuxBoxPictureIcon;
    int m_iTuxBoxEpgRequestTime;
    int m_iTuxBoxZapWaitTime;
    bool m_bTuxBoxSendAllAPids;

    int m_curlclienttimeout;
    int m_curlGlobalIdleTimeout;

#ifdef HAS_SDL
    bool m_fullScreen;
    bool m_fakeFullScreen;
#endif
    int m_playlistRetries;
    int m_playlistTimeout;
    bool m_GLRectangleHack;
    
    int m_secondsToVisualizer;
    bool m_bVisualizerOnPlay;
    int m_nowPlayingFlipTime;
    bool m_bBackgroundMusicOnlyWhenFocused;

    bool m_bAutoShuffle;
    bool m_bUseAnamorphicZoom;
    
    bool m_bEnableViewRestrictions;
    bool m_bEnableKeyboardBacklightControl;
    
    
    CStdString m_language;
    CStdString m_units;
  };

  struct stSettings
  {
public:
    stSettings()
      : m_viewStateVideoNavEpisodes(65590, SORT_METHOD_EPISODE, SORT_ORDER_ASC)
      , m_viewStateVideoNavSeasons(65536, SORT_METHOD_LABEL, SORT_ORDER_ASC)
      {}
  
    CStdString m_pictureExtensions;
    CStdString m_musicExtensions;
    CStdString m_videoExtensions;

    CStdString m_defaultMusicScraper;

    CStdString m_logFolder;

    bool m_bMyMusicSongInfoInVis;
    bool m_bMyMusicSongThumbInVis;

    CViewState m_viewStateMusicNavArtists;
    CViewState m_viewStateMusicNavAlbums;
    CViewState m_viewStateMusicNavSongs;
    CViewState m_viewStateMusicShoutcast;
    CViewState m_viewStateMusicLastFM;
    CViewState m_viewStateVideoNavActors;
    CViewState m_viewStateVideoNavYears;
    CViewState m_viewStateVideoNavGenres;
    CViewState m_viewStateVideoNavTitles;
    CViewState m_viewStateVideoNavEpisodes;
    CViewState m_viewStateVideoNavSeasons;
    CViewState m_viewStateVideoNavTvShows;
    CViewState m_viewStateVideoNavMusicVideos;

    bool m_bMyMusicPlaylistRepeat;
    bool m_bMyMusicPlaylistShuffle;
    int m_iMyMusicStartWindow;

    // for scanning
    bool m_bMyMusicIsScanning;

    CVideoSettings m_defaultVideoSettings;
    CVideoSettings m_currentVideoSettings;

    float m_fZoomAmount;      // current zoom amount
    float m_fPixelRatio;      // current pixel ratio

    int m_iMyVideoWatchMode;

    bool m_bMyVideoPlaylistRepeat;
    bool m_bMyVideoPlaylistShuffle;
    bool m_bMyVideoNavFlatten;

    int m_iVideoStartWindow;

    int m_iMyVideoStack;
    char m_szMyVideoCleanTokens[256];
    char m_szMyVideoCleanSeparators[32];

    int iAdditionalSubtitleDirectoryChecked;

    char szOnlineArenaPassword[32]; // private arena password
    char szOnlineArenaDescription[64]; // private arena description

    int m_HttpApiBroadcastPort;
    int m_HttpApiBroadcastLevel;
    int m_nVolumeLevel;                     // measured in milliBels -60dB -> 0dB range.
    int m_dynamicRangeCompressionLevel;     // measured in milliBels  0dB -> 30dB range.
    int m_iPreMuteVolumeLevel;    // save the m_nVolumeLevel for proper restore
    bool m_bMute;
    int m_iSystemTimeTotalUp;    // Uptime in minutes!

    VOICE_MASK m_karaokeVoiceMask[4];
  };

  std::map<int,std::pair<std::vector<int>,std::vector<std::string> > > m_mapRssUrls;
  std::map<int, CSkinString> m_skinStrings;
  std::map<int, CSkinBool> m_skinBools;

  // cache copies of these parsed values, to avoid re-parsing over and over
  CStdString m_szMyVideoStackSeparatorsString;
  CStdStringArray m_szMyVideoStackTokensArray;
  CStdString m_szMyVideoCleanSeparatorsString;
  CStdStringArray m_szMyVideoCleanTokensArray;

  VECSOURCES m_programSources;
  VECSOURCES m_pictureSources;
  VECSOURCES m_fileSources;
  VECSOURCES m_musicSources;
  VECSOURCES m_videoSources;

  CStdString m_defaultProgramSource;
  CStdString m_defaultMusicSource;
  CStdString m_defaultPictureSource;
  CStdString m_defaultFileSource;
  CStdString m_defaultVideoSource;
  CStdString m_defaultMusicLibSource;
  CStdString m_defaultVideoLibSource;

  VECSOURCES m_UPnPMusicSources;
  VECSOURCES m_UPnPVideoSources;
  VECSOURCES m_UPnPPictureSources;

  CStdString m_UPnPUUIDServer;
  int        m_UPnPPortServer;
  int        m_UPnPMaxReturnedItems;
  CStdString m_UPnPUUIDRenderer;
  int        m_UPnPPortRenderer;

  //VECFILETYPEICONS m_vecIcons;
  VECPROFILES m_vecProfiles;
  int m_iLastLoadedProfileIndex;
  int m_iLastUsedProfileIndex;
  bool bUseLoginScreen;
  RESOLUTION_INFO m_ResInfo[CUSTOM+MAX_RESOLUTIONS];

  // utility functions for user data folders
  CStdString GetUserDataItem(const CStdString& strFile) const;
  CStdString GetProfileUserDataFolder() const;
  CStdString GetUserDataFolder() const;
  CStdString GetDatabaseFolder() const;
  CStdString GetCDDBFolder() const;
  CStdString GetThumbnailsFolder() const;
  CStdString GetMusicThumbFolder() const;
  CStdString GetLastFMThumbFolder() const;
  CStdString GetMusicArtistThumbFolder() const;
  CStdString GetVideoThumbFolder() const;
  CStdString GetBookmarksThumbFolder() const;
  CStdString GetPicturesThumbFolder() const;
  CStdString GetProgramsThumbFolder() const;
  CStdString GetGameSaveThumbFolder() const;
  CStdString GetXLinkKaiThumbFolder() const;
  CStdString GetProfilesThumbFolder() const;
  CStdString GetSourcesFile() const;
  CStdString GetSkinFolder() const;
  CStdString GetSkinFolder(const CStdString& skinName) const;
  CStdString GetScriptsFolder() const;
  CStdString GetVideoFanartFolder() const;
  CStdString GetProgramFanartFolder() const;
  
  CStdString GetPlexMediaServerThumbFolder() const;
  CStdString GetPlexMediaServerFanartFolder() const;

  CStdString GetSettingsFile() const;

  bool LoadUPnPXml(const CStdString& strSettingsFile);
  bool SaveUPnPXml(const CStdString& strSettingsFile) const;

  bool LoadProfiles(const CStdString& strSettingsFile);
  bool SaveProfiles(const CStdString& strSettingsFile) const;

  bool SaveSettings(const CStdString& strSettingsFile, CGUISettings *localSettings = NULL) const;

  bool SaveSources();

protected:
  // these 3 don't have a default - used for advancedsettings.xml
  bool GetInteger(const TiXmlElement* pRootElement, const char *strTagName, int& iValue, const int iMin, const int iMax);
  bool GetFloat(const TiXmlElement* pRootElement, const char *strTagName, float& fValue, const float fMin, const float fMax);
  bool GetString(const TiXmlElement* pRootElement, const char *strTagName, CStdString& strValue);

  bool GetInteger(const TiXmlElement* pRootElement, const char *strTagName, int& iValue, const int iDefault, const int iMin, const int iMax);
  bool GetFloat(const TiXmlElement* pRootElement, const char *strTagName, float& fValue, const float fDefault, const float fMin, const float fMax);
  bool GetString(const TiXmlElement* pRootElement, const char *strTagName, CStdString& strValue, const CStdString& strDefaultValue);
  bool GetString(const TiXmlElement* pRootElement, const char *strTagName, char *szValue, const CStdString& strDefaultValue);
  bool GetSource(const CStdString &category, const TiXmlNode *source, CMediaSource &share);
  void GetSources(const TiXmlElement* pRootElement, const CStdString& strTagName, VECSOURCES& items, CStdString& strDefault);
  bool SetSources(TiXmlNode *root, const char *section, const VECSOURCES &shares, const char *defaultPath);
  void GetViewState(const TiXmlElement* pRootElement, const CStdString& strTagName, CViewState &viewState, SORT_METHOD defaultSort = SORT_METHOD_LABEL);

  void ConvertHomeVar(CStdString& strText);
  // functions for writing xml files
  void SetString(TiXmlNode* pRootNode, const CStdString& strTagName, const CStdString& strValue) const;
  void SetInteger(TiXmlNode* pRootNode, const CStdString& strTagName, int iValue) const;
  void SetFloat(TiXmlNode* pRootNode, const CStdString& strTagName, float fValue) const;
  void SetBoolean(TiXmlNode* pRootNode, const CStdString& strTagName, bool bValue) const;
  void SetHex(TiXmlNode* pRootNode, const CStdString& strTagName, DWORD dwHexValue) const;
  void SetViewState(TiXmlNode* pRootNode, const CStdString& strTagName, const CViewState &viewState) const;

  bool LoadCalibration(const TiXmlElement* pElement, const CStdString& strSettingsFile);
  bool SaveCalibration(TiXmlNode* pRootNode) const;

  bool LoadSettings(const CStdString& strSettingsFile);
//  bool SaveSettings(const CStdString& strSettingsFile) const;

  // skin activated settings
  void LoadSkinSettings(const TiXmlElement* pElement);
  void SaveSkinSettings(TiXmlNode *pElement) const;

  // Advanced settings
  void LoadAdvancedSettings();

  void LoadUserFolderLayout();

  void LoadRSSFeeds();
  
 public:
  
   string GetLanguage();
  
 private:
   
   map<string, string> m_languageMap;
};

extern class CSettings g_settings;
extern struct CSettings::stSettings g_stSettings;
extern struct CSettings::AdvancedSettings g_advancedSettings;
