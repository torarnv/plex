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

#include "stdafx.h"
#include "Settings.h"
#include "Application.h"
#include "KeyboardLayoutConfiguration.h"
#include "Util.h"
#include "URL.h"
#include "GUIWindowFileManager.h"
//#include "GUIDialogButtonMenu.h"
#include "GUIFontManager.h"
#include "LangCodeExpander.h"
#include "ButtonTranslator.h"
#include "XMLUtils.h"
#include "GUIPassword.h"
#include "GUIAudioManager.h"
#include "AudioContext.h"
#include "utils/GUIInfoManager.h"
#include "utils/Network.h"
#include "FileSystem/MultiPathDirectory.h"
#include "GUIBaseContainer.h" // for VIEW_TYPE enum
#include "utils/FanController.h"
#include "MediaManager.h"
#include "XBVideoConfig.h"
#include "DNSNameCache.h"
#include "GUIWindowManager.h"
#include "GUIDialogYesNo.h"
#include "FileSystem/Directory.h"
#include "FileItem.h"
#ifdef HAS_XBOX_HARDWARE
#include "utils/MemoryUnitManager.h"
#endif
#ifdef _WIN32PC
#include "win32/WIN32Util.h"
#endif
#include "FileSystem/SMBDirectory.h"
#ifdef __APPLE__
#include "CocoaUtils.h"
#include "CocoaUtilsPlus.h"
#endif

#include <map>

using namespace std;
using namespace XFILE;
using namespace DIRECTORY;
using namespace MEDIA_DETECT;

struct CSettings::stSettings g_stSettings;
struct CSettings::AdvancedSettings g_advancedSettings;
class CSettings g_settings;

extern CStdString g_LoadErrorStr;

CSettings::CSettings(void)
{
  // Map from the ISO languages to the deprecated XBMC ones.
  m_languageMap["ca"] = "Catalan";
  m_languageMap["zh-Hans"] = "Chinese (Simple)";
  m_languageMap["zh-Hant"] = "Chinese (Traditional)";
  m_languageMap["hr"] = "Croatian";
  m_languageMap["cs"] = "Czech";
  m_languageMap["da"] = "Danish";
  m_languageMap["nl"] = "Dutch";
  m_languageMap["en"] = "English (US)";
  m_languageMap["en-GB"] = "English";
  m_languageMap["en-AU"] = "English";
  m_languageMap["en-CA"] = "English";
  m_languageMap["eo"] = "Esperanto";
  m_languageMap["fi"] = "Finnish";
  m_languageMap["fr"] = "French";
  m_languageMap["de"] = "German";
  m_languageMap["de-AT"] = "German (Austria)";
  m_languageMap["el"] = "Greek";
  m_languageMap["he"] = "Hebrew";
  m_languageMap["hu"] = "Hungarian";
  m_languageMap["is"] = "Icelandic";
  m_languageMap["id"] = "Indonesian";
  m_languageMap["it"] = "Italian";
  m_languageMap["ja"] = "Japanese";
  m_languageMap["ko"] = "Korean";
  m_languageMap["mt"] = "Maltese";
  m_languageMap["no"] = "Norwegian";
  m_languageMap["pl"] = "Polish";
  m_languageMap["pt"] = "Portuguese";
  m_languageMap["pt-BR"] = "Portuguese (Brazil)";
  m_languageMap["ro"] = "Romanian";
  m_languageMap["ru"] = "Russian";
  m_languageMap["sr"] = "Serbian";
  m_languageMap["sk"] = "Slovak";
  m_languageMap["sl"] = "Slovenian";
  m_languageMap["es"] = "Spanish";
  m_languageMap["sv"] = "Swedish";
  m_languageMap["tr"] = "Turkish";
  m_languageMap["uk"] = "Ukrainian";
  
  for (int i = HDTV_1080i; i <= PAL60_16x9; i++)
  {
    ZeroMemory(&m_ResInfo[i], sizeof(RESOLUTION));
    g_graphicsContext.ResetScreenParameters((RESOLUTION)i);
    g_graphicsContext.ResetOverscan((RESOLUTION)i, m_ResInfo[i].Overscan);
  }

  for (int i = DESKTOP ; i<=CUSTOM ; i++)
  {
    ZeroMemory(&m_ResInfo[i], sizeof(RESOLUTION));
    g_graphicsContext.ResetScreenParameters((RESOLUTION)i);
    g_graphicsContext.ResetOverscan(m_ResInfo[i]);
  }

  g_stSettings.m_iMyVideoStack = STACK_NONE;

  strcpy(g_stSettings.m_szMyVideoCleanTokens, "divx|xvid|3ivx|ac3|ac351|dts|mp3|wma|m4a|m4b|mp4|aac|ogg|scr|ts|sharereactor|dvd|dvdrip|hd-dvd|remux|dtshd|ddplus|blue-ray");
  strcpy(g_stSettings.m_szMyVideoCleanSeparators, "- _.[({+");

  StringUtils::SplitString(g_stSettings.m_szMyVideoCleanTokens, "|", g_settings.m_szMyVideoCleanTokensArray);
  g_settings.m_szMyVideoCleanSeparatorsString = g_stSettings.m_szMyVideoCleanSeparators;

  for (int i = 0; i < (int)g_settings.m_szMyVideoCleanTokensArray.size(); i++)
    g_settings.m_szMyVideoCleanTokensArray[i].MakeLower();

  strcpy(g_stSettings.szOnlineArenaPassword, "");
  strcpy(g_stSettings.szOnlineArenaDescription, "It's Good To Play Together!");

  g_stSettings.m_bMyMusicSongInfoInVis = true;    // UNUSED - depreciated.
  g_stSettings.m_bMyMusicSongThumbInVis = false;  // used for music info in vis screen

  g_stSettings.m_bMyMusicPlaylistRepeat = false;
  g_stSettings.m_bMyMusicPlaylistShuffle = false;

  g_stSettings.m_bMyVideoPlaylistRepeat = false;
  g_stSettings.m_bMyVideoPlaylistShuffle = false;
  g_stSettings.m_bMyVideoNavFlatten = false;

  g_stSettings.m_nVolumeLevel = 0;
  g_stSettings.m_dynamicRangeCompressionLevel = 0;
  g_stSettings.m_iPreMuteVolumeLevel = 0;
  g_stSettings.m_bMute = false;
  g_stSettings.m_fZoomAmount = 1.0f;
  g_stSettings.m_fPixelRatio = 1.0f;

  g_stSettings.m_pictureExtensions = ".png|.jpg|.jpeg|.bmp|.gif|.ico|.tif|.tiff|.tga|.pcx|.cbz|.zip|.cbr|.rar|.m3u|.dng|.nef|.cr2|.crw|.orf|.arw|.erf|.3fr|.dcr|.x3f|.mef|.raf|.mrw|.pef|.sr2";
  g_stSettings.m_musicExtensions = ".m4p|.nsv|.m4a|.m4b|.flac|.aac|.strm|.pls|.rm|.rma|.mpa|.wav|.wma|.ogg|.mp3|.mp2|.m3u|.mod|.amf|.669|.dmf|.dsm|.far|.gdm|.imf|.it|.m15|.med|.okt|.s3m|.stm|.sfx|.ult|.uni|.xm|.sid|.ac3|.dts|.cue|.aif|.aiff|.wpl|.ape|.mac|.mpc|.mp+|.mpp|.shn|.zip|.rar|.wv|.nsf|.spc|.gym|.adplug|.adx|.dsp|.adp|.ymf|.ast|.afc|.hps|.xsp|.xwav|.waa|.wvs|.wam|.gcm|.idsp|.mpdsp|.mss|.spt|.rsd|.mid|.kar|.sap|.cmc|.cmr|.dmc|.mpt|.mpd|.rmt|.tmc|.tm8|.tm2|.oga|.url";
  g_stSettings.m_videoExtensions = ".m4v|.3gp|.nsv|.ts|.ty|.strm|.pls|.rm|.rmvb|.m3u|.ifo|.mov|.qt|.divx|.xvid|.bivx|.vob|.nrg|.img|.iso|.pva|.wmv|.asf|.asx|.ogm|.m2v|.avi|.bin|.dat|.mpg|.mpeg|.mp4|.mkv|.avc|.vp3|.svq3|.nuv|.viv|.dv|.fli|.flv|.rar|.001|.wpl|.zip|.vdr|.dvr-ms|.xsp|.mts|.m2t|.m2ts|.evo|.ogv|.sdp|.avs|.rec|.url";

  // internal music extensions
  g_stSettings.m_musicExtensions += "|.sidstream|.oggstream|.nsfstream|.asapstream|.cdda";

#ifdef __APPLE__
  CStdString logDir = getenv("HOME");
  logDir += "/Library/Logs/";
  g_stSettings.m_logFolder = logDir;
#else
  g_stSettings.m_logFolder = "Q:\\";              // log file location
#endif

  g_stSettings.m_defaultMusicScraper = "allmusic.xml";

  m_iLastLoadedProfileIndex = 0;

  // defaults for scanning
  g_stSettings.m_bMyMusicIsScanning = false;

  g_stSettings.iAdditionalSubtitleDirectoryChecked = 0;

  g_settings.bUseLoginScreen = false;

  // Advanced settings
  g_advancedSettings.m_useMultipaths = true;
  g_advancedSettings.m_DisableModChipDetection = true;

  g_advancedSettings.m_audioHeadRoom = 0;
  g_advancedSettings.m_karaokeSyncDelay = 0.0f;

  g_advancedSettings.m_videoSubsDelayRange = 10;
  g_advancedSettings.m_videoAudioDelayRange = 10;
  g_advancedSettings.m_videoSmallStepBackSeconds = 7;
  g_advancedSettings.m_videoSmallStepBackTries = 3;
  g_advancedSettings.m_videoSmallStepBackDelay = 300;
  g_advancedSettings.m_videoUseTimeSeeking = true;
  g_advancedSettings.m_videoTimeSeekForward = 30;
  g_advancedSettings.m_videoTimeSeekBackward = -15;
  g_advancedSettings.m_videoTimeSeekForwardBig = 600;
  g_advancedSettings.m_videoTimeSeekBackwardBig = -600;
  g_advancedSettings.m_videoPercentSeekForward = 2;
  g_advancedSettings.m_videoPercentSeekBackward = -2;
  g_advancedSettings.m_videoPercentSeekForwardBig = 10;
  g_advancedSettings.m_videoPercentSeekBackwardBig = -10;
  g_advancedSettings.m_videoBlackBarColour = 1;
  
  g_advancedSettings.m_musicUseTimeSeeking = true;
  g_advancedSettings.m_musicTimeSeekForward = 10;
  g_advancedSettings.m_musicTimeSeekBackward = -10;
  g_advancedSettings.m_musicTimeSeekForwardBig = 60;
  g_advancedSettings.m_musicTimeSeekBackwardBig = -60;
  g_advancedSettings.m_musicPercentSeekForward = 1;
  g_advancedSettings.m_musicPercentSeekBackward = -1;
  g_advancedSettings.m_musicPercentSeekForwardBig = 10;
  g_advancedSettings.m_musicPercentSeekBackwardBig = -10;

  g_advancedSettings.m_slideshowPanAmount = 2.5f;
  g_advancedSettings.m_slideshowZoomAmount = 5.0f;
  g_advancedSettings.m_slideshowBlackBarCompensation = 20.0f;

  g_advancedSettings.m_lcdRows = 4;
  g_advancedSettings.m_lcdColumns = 20;
  g_advancedSettings.m_lcdAddress1 = 0;
  g_advancedSettings.m_lcdAddress2 = 0x40;
  g_advancedSettings.m_lcdAddress3 = 0x14;
  g_advancedSettings.m_lcdAddress4 = 0x54;

  g_advancedSettings.m_autoDetectPingTime = 30;
  g_advancedSettings.m_playCountMinimumPercent = 90.0f;

  g_advancedSettings.m_songInfoDuration = 10;
  g_advancedSettings.m_busyDialogDelay = 2000;
  g_advancedSettings.m_logLevel = LOG_LEVEL_NORMAL;
  g_advancedSettings.m_cddbAddress = "freedb.freedb.org";
  g_advancedSettings.m_usePCDVDROM = false;
  g_advancedSettings.m_noDVDROM = false;
  g_advancedSettings.m_enableOpticalMedia = false;
  g_advancedSettings.m_cachePath = "Z:\\";
  g_advancedSettings.m_displayRemoteCodes = false;

  g_advancedSettings.m_videoStackRegExps.push_back("[ _\\.-]+cd[ _\\.-]*([0-9a-d]+)");
  g_advancedSettings.m_videoStackRegExps.push_back("[ _\\.-]+dvd[ _\\.-]*([0-9a-d]+)");
  g_advancedSettings.m_videoStackRegExps.push_back("[ _\\.-]+part[ _\\.-]*([0-9a-d]+)");
  g_advancedSettings.m_videoStackRegExps.push_back("[ _\\.-]+dis[ck][ _\\.-]*([0-9a-d]+)");
  //g_advancedSettings.m_videoStackRegExps.push_back("()[ _\\.-]+([0-9]*[abcd]+)(\\.....?)$"); // can anyone explain this one?  should this be ([0-9a-d]+) ?
  g_advancedSettings.m_videoStackRegExps.push_back("()cd([0-9a-d]+)(\\.....?)$");
  g_advancedSettings.m_videoStackRegExps.push_back("([a-z])([0-9]+)(\\.....?)$");
  g_advancedSettings.m_videoStackRegExps.push_back("()([ab])(\\.....?)$");

  // foo_[s01]_[e01]
  g_advancedSettings.m_tvshowStackRegExps.push_back("\\[[Ss]([0-9]+)\\]_\\[[Ee]([0-9]+)\\]?([^\\\\/]*)$");
  // foo.1x09*
  g_advancedSettings.m_tvshowStackRegExps.push_back("[\\._ -\\[]([0-9]+)x([0-9]+)([^\\\\/]*)$");
  // foo.s01.e01, foo.s01_e01, S01E02 foo
  g_advancedSettings.m_tvshowStackRegExps.push_back("[Ss]([0-9]+)[\\.-]?[Ee]([0-9]+)([^\\\\/]*)$");
  // foo.103*, 103 foo
  g_advancedSettings.m_tvshowStackRegExps.push_back("[\\\\/\\._ -]([0-9]+)([0-9][0-9])([\\._ -][^\\\\/]*)$");

  g_advancedSettings.m_tvshowMultiPartStackRegExp = "^[-EeXx]+([0-9]+)";

  g_advancedSettings.m_remoteRepeat = 480;
  g_advancedSettings.m_controllerDeadzone = 0.2f;
  g_advancedSettings.m_FTPShowCache = false;

  g_advancedSettings.m_playlistAsFolders = true;
  g_advancedSettings.m_detectAsUdf = false;

  g_advancedSettings.m_thumbSize = 512;

  g_advancedSettings.m_sambaclienttimeout = 10;
  g_advancedSettings.m_sambadoscodepage = "";
  g_advancedSettings.m_sambastatfiles = true;

  g_advancedSettings.m_musicThumbs = "folder.jpg|Folder.jpg|folder.JPG|Folder.JPG|cover.jpg|Cover.jpg|cover.jpeg";
  g_advancedSettings.m_dvdThumbs = "preview.jpg|Preview.jpg|preview.JPG|Preview.JPG|folder.jpg|Folder.jpg|folder.JPG|Folder.JPG";

  g_advancedSettings.m_bMusicLibraryHideAllItems = false;
  g_advancedSettings.m_bMusicLibraryAllItemsOnBottom = false;
  g_advancedSettings.m_bMusicLibraryAlbumsSortByArtistThenYear = false;
  g_advancedSettings.m_strMusicLibraryAlbumFormat = "";
  g_advancedSettings.m_strMusicLibraryAlbumFormatRight = "";
  g_advancedSettings.m_prioritiseAPEv2tags = false;
  g_advancedSettings.m_musicItemSeparator = " / ";
  g_advancedSettings.m_videoItemSeparator = " / ";

  g_advancedSettings.m_bVideoLibraryHideAllItems = false;
  g_advancedSettings.m_bVideoLibraryAllItemsOnBottom = false;
  g_advancedSettings.m_bVideoLibraryHideRecentlyAddedItems = false;
  g_advancedSettings.m_bVideoLibraryHideEmptySeries = false;
  g_advancedSettings.m_bVideoLibraryCleanOnUpdate = false;

  g_advancedSettings.m_bUseEvilB = true;

  g_advancedSettings.m_bTuxBoxAudioChannelSelection = false;
  g_advancedSettings.m_bTuxBoxSubMenuSelection = false;
  g_advancedSettings.m_bTuxBoxPictureIcon= true;
  g_advancedSettings.m_bTuxBoxSendAllAPids= false;
  g_advancedSettings.m_iTuxBoxEpgRequestTime = 10; //seconds
  g_advancedSettings.m_iTuxBoxDefaultSubMenu = 4;
  g_advancedSettings.m_iTuxBoxDefaultRootMenu = 0; //default TV Mode
  g_advancedSettings.m_iTuxBoxZapWaitTime = 0; // Time in sec. Default 0:OFF

  g_advancedSettings.m_curlclienttimeout = 40;
  g_advancedSettings.m_curlGlobalIdleTimeout = 30000; // Milliseconds

#ifdef HAS_SDL
  g_advancedSettings.m_fullScreen = false;
  g_advancedSettings.m_fakeFullScreen = true;
#endif

  g_advancedSettings.m_playlistRetries = 100;
  g_advancedSettings.m_playlistTimeout = 40;
  g_advancedSettings.m_GLRectangleHack = false;
  
  g_advancedSettings.m_secondsToVisualizer = 10;
  g_advancedSettings.m_bVisualizerOnPlay = true;
  g_advancedSettings.m_nowPlayingFlipTime = 120;
  g_advancedSettings.m_bBackgroundMusicOnlyWhenFocused = true;
  
  g_advancedSettings.m_bAutoShuffle = true;
  g_advancedSettings.m_bUseAnamorphicZoom = false;
  
  g_advancedSettings.m_bEnableViewRestrictions = true;
  g_advancedSettings.m_bEnableKeyboardBacklightControl = false;
}

CSettings::~CSettings(void)
{
}


void CSettings::Save() const
{
  if (g_application.m_bStop)
  {
    //don't save settings when we're busy stopping the application
    //a lot of screens try to save settings on deinit and deinit is called
    //for every screen when the application is stopping.
    return ;
  }
  if (!SaveSettings(GetSettingsFile()))
  {
    CLog::Log(LOGERROR, "Unable to save settings to %s", GetSettingsFile().c_str());
  }
}

bool CSettings::Reset()
{
  CLog::Log(LOGINFO, "Resetting settings");
  CFile::Delete(GetSettingsFile());
  Save();
  return LoadSettings(GetSettingsFile());
}

bool CSettings::Load(bool& bXboxMediacenter, bool& bSettings)
{
  // load settings file...
  bXboxMediacenter = bSettings = false;

  CStdString strMnt = GetProfileUserDataFolder();
#ifdef _XBOX
  char szDevicePath[1024];
  if (GetProfileUserDataFolder().Left(2).Equals("Q:"))
  {
    CUtil::GetHomePath(strMnt);
    strMnt += GetProfileUserDataFolder().substr(2);
  }
  CIoSupport::GetPartition(strMnt.c_str()[0], szDevicePath);
  strcat(szDevicePath,strMnt.c_str()+2);
  CIoSupport::RemapDriveLetter('P', szDevicePath);
#else
  CIoSupport::RemapDriveLetter('P', (char*) strMnt.c_str());
#endif
  CLog::Log(LOGNOTICE, "loading %s", GetSettingsFile().c_str());
  CStdString strFile=GetSettingsFile();
  if (!LoadSettings(strFile))
  {
    CLog::Log(LOGERROR, "Unable to load %s, creating new %s with default values", GetSettingsFile().c_str(), GetSettingsFile().c_str());
    Save();
    if (!(bSettings = Reset()))
      return false;
  }

  // clear sources, then load xml file...
  m_fileSources.clear();
  m_musicSources.clear();
  m_pictureSources.clear();
  m_programSources.clear();
  m_videoSources.clear();
  CStdString strXMLFile = GetSourcesFile();
  CLog::Log(LOGNOTICE, "%s",strXMLFile.c_str());
  TiXmlDocument xmlDoc;
  TiXmlElement *pRootElement = NULL;
  if ( xmlDoc.LoadFile( strXMLFile.c_str() ) )
  {
    pRootElement = xmlDoc.RootElement();
    CStdString strValue;
    if (pRootElement)
      strValue = pRootElement->Value();
    if ( strValue != "sources")
      CLog::Log(LOGERROR, "%s sources.xml file does not contain <sources>", __FUNCTION__);
  }
  else
    CLog::Log(LOGERROR, "%s Error loading %s: Line %d, %s", __FUNCTION__, strXMLFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());

  // look for external sources file
  CStdString strCached = "Z:\\remotesources.xml";
  bool bRemoteSourceFile = false;
  TiXmlNode *pInclude = pRootElement ? pRootElement->FirstChild("remote") : NULL;
  if (pInclude)
  {
    CStdString strRemoteFile = pInclude->FirstChild()->Value();
    if (!strRemoteFile.IsEmpty())
    {
      // local file is not allowed as a remote source
      if (!CUtil::IsHD(strRemoteFile))
      {
        CLog::Log(LOGDEBUG, "Found <remote> tag");
        CLog::Log(LOGDEBUG, "Attempting to retrieve remote file: %s", strRemoteFile.c_str());
        // sometimes we have to wait for the network
        if (g_application.getNetwork().IsAvailable())
        {
          // cache the external source file
          if (CFile::Cache(strRemoteFile, strCached))
          {
            bRemoteSourceFile = true;
            CLog::Log(LOGDEBUG, "Success! Remote sources will be used");
          }
        }
        else
          CLog::Log(LOGERROR, "Could not retrieve remote file, defaulting to local sources");
      }
      else
        CLog::Log(LOGERROR, "Local harddrive path is not allowed as remote");
    }
  }

  // open cached external source file
  if (bRemoteSourceFile)
  {
    strXMLFile = strCached;
    if ( xmlDoc.LoadFile( strXMLFile.c_str() ) )
    {
      pRootElement = xmlDoc.RootElement();
      CStdString strValue;
      if (pRootElement)
        strValue = pRootElement->Value();
      if ( strValue != "sources")
        CLog::Log(LOGERROR, "%s remote_sources.xml file does not contain <sources>", __FUNCTION__);
    }
    else
      CLog::Log(LOGERROR, "%s unable to load file: %s, Line %d, %s", __FUNCTION__, strXMLFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
  }

  if (pRootElement)
  { // parse sources...
    GetSources(pRootElement, "programs", m_programSources, m_defaultProgramSource);
    if (!m_programSources.size()) // backward compatibility with "my" notation
      GetSources(pRootElement, "myprograms", m_programSources, m_defaultProgramSource);

    GetSources(pRootElement, "pictures", m_pictureSources, m_defaultPictureSource);
    GetSources(pRootElement, "files", m_fileSources, m_defaultFileSource);
    GetSources(pRootElement, "music", m_musicSources, m_defaultMusicSource);
    GetSources(pRootElement, "video", m_videoSources, m_defaultVideoSource);
  }
  
  bXboxMediacenter = true;

  LoadRSSFeeds();
  LoadUserFolderLayout();

  return true;
}

void CSettings::ConvertHomeVar(CStdString& strText)
{
  // Replaces first occurence of $HOME with the home directory.
  // "$HOME\foo" becomes for instance "e:\apps\xbmc\foo"

  char szText[1024];
  char szTemp[1024];
  char *pReplace, *pReplace2;

  CStdString strHomePath = "Q:";
  strcpy(szText, strText.c_str());

  pReplace = strstr(szText, "$HOME");

  if (pReplace != NULL)
  {
    pReplace2 = pReplace + sizeof("$HOME") - 1;
    strcpy(szTemp, pReplace2);
    strcpy(pReplace, strHomePath.c_str() );
    strcat(szText, szTemp);
  }
  strText = szText;
  // unroll any relative paths used
  std::vector<CStdString> token;
  CUtil::Tokenize(_P(strText),token,"\\");
  if (token.size() > 1)
  {
    strText = "";
    for (unsigned int i=0;i<token.size();++i)
      if (token[i] == "..")
      {
        CStdString strParent;
        CUtil::GetParentPath(strText,strParent);
        strText = strParent;
      }
      else
        strText += token[i]+"\\";
  }
}

VECSOURCES *CSettings::GetSourcesFromType(const CStdString &type)
{
  if (type == "programs" || type == "myprograms")
    return &g_settings.m_programSources;
  else if (type == "files")
  {
    // this nasty block of code is needed as we have to
    // call getlocaldrives after localize strings has been loaded
    bool bAdded=false;
    for (unsigned int i=0;i<g_settings.m_fileSources.size();++i)
    {
      if (g_settings.m_fileSources[i].m_ignore)
      {
        bAdded = true;
        break;
      }
    }
    if (!bAdded)
    {
      VECSOURCES shares;
      g_mediaManager.GetLocalDrives(shares, true);  // true to include Q
      m_fileSources.insert(m_fileSources.end(),shares.begin(),shares.end());
    }

    return &g_settings.m_fileSources;
  }
  else if (type == "music")
    return &g_settings.m_musicSources;
  else if (type == "video")
    return &g_settings.m_videoSources;
  else if (type == "pictures")
    return &g_settings.m_pictureSources;
  else if (type == "upnpmusic")
    return &g_settings.m_UPnPMusicSources;
  else if (type == "upnpvideo")
    return &g_settings.m_UPnPVideoSources;
  else if (type == "upnppictures")
    return &g_settings.m_UPnPPictureSources;

  return NULL;
}

CStdString CSettings::GetDefaultSourceFromType(const CStdString &type)
{
  CStdString defaultShare;
  if (type == "programs" || type == "myprograms")
    defaultShare = g_settings.m_defaultProgramSource;
  else if (type == "files")
    defaultShare = g_settings.m_defaultFileSource;
  else if (type == "music")
    defaultShare = g_settings.m_defaultMusicSource;
  else if (type == "video")
    defaultShare = g_settings.m_defaultVideoSource;
  else if (type == "pictures")
    defaultShare = g_settings.m_defaultPictureSource;
  return defaultShare;
}

void CSettings::GetSources(const TiXmlElement* pRootElement, const CStdString& strTagName, VECSOURCES& items, CStdString& strDefault)
{
  //CLog::Log(LOGDEBUG, "  Parsing <%s> tag", strTagName.c_str());
  strDefault = "";

  items.clear();
  const TiXmlNode *pChild = pRootElement->FirstChild(strTagName.c_str());
  if (pChild)
  {
    pChild = pChild->FirstChild();
    while (pChild > 0)
    {
      CStdString strValue = pChild->Value();
      if (strValue == "source" || strValue == "bookmark") // "bookmark" left in for backwards compatibility
      {
        CMediaSource share;
        if (GetSource(strTagName, pChild, share))
        {
          items.push_back(share);
        }
        else
        {
          CLog::Log(LOGERROR, "    Missing or invalid <name> and/or <path> in source");
        }
      }

      if (strValue == "default")
      {
        const TiXmlNode *pValueNode = pChild->FirstChild();
        if (pValueNode)
        {
          const char* pszText = pChild->FirstChild()->Value();
          if (strlen(pszText) > 0)
            strDefault = pszText;
          CLog::Log(LOGDEBUG, "    Setting <default> source to : %s", strDefault.c_str());
        }
      }
      pChild = pChild->NextSibling();
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "  <%s> tag is missing or sources.xml is malformed", strTagName.c_str());
  }
}

bool CSettings::GetSource(const CStdString &category, const TiXmlNode *source, CMediaSource &share)
{
  //CLog::Log(LOGDEBUG,"    ---- SOURCE START ----");
  const TiXmlNode *pNodeName = source->FirstChild("name");
  CStdString strName;
  if (pNodeName && pNodeName->FirstChild())
  {
    strName = pNodeName->FirstChild()->Value();
    //CLog::Log(LOGDEBUG,"    Found name: %s", strName.c_str());
  }
  // get multiple paths
  vector<CStdString> vecPaths;
  const TiXmlNode *pPathName = source->FirstChild("path");
  while (pPathName)
  {
    if (pPathName->FirstChild())
    {
      CStdString strPath = pPathName->FirstChild()->Value();
      // make sure there are no virtualpaths or stack paths defined in xboxmediacenter.xml
      //CLog::Log(LOGDEBUG,"    Found path: %s", strPath.c_str());
      if (!CUtil::IsVirtualPath(strPath) && !CUtil::IsStack(strPath))
      {
        // translate special tags
        if (strPath.at(0) == '$')
        {
          CStdString strPathOld(strPath);
          strPath = CUtil::TranslateSpecialSource(strPath);
          if (!strPath.IsEmpty())
          {
            //CLog::Log(LOGDEBUG,"    -> Translated to path: %s", strPath.c_str());
          }
          else
          {
            //CLog::Log(LOGERROR,"    -> Skipping invalid token: %s", strPathOld.c_str());
            pPathName = pPathName->NextSibling("path");
            continue;
          }
        }
        CUtil::AddSlashAtEnd(strPath);
        vecPaths.push_back(strPath);
      }
      else
        CLog::Log(LOGERROR,"    Invalid path type (%s) in source", strPath.c_str());
    }
    pPathName = pPathName->NextSibling("path");
  }

  const TiXmlNode *pLockMode = source->FirstChild("lockmode");
  const TiXmlNode *pLockCode = source->FirstChild("lockcode");
  const TiXmlNode *pBadPwdCount = source->FirstChild("badpwdcount");
  const TiXmlNode *pThumbnailNode = source->FirstChild("thumbnail");

  if (!strName.IsEmpty() && vecPaths.size() > 0)
  {
    vector<CStdString> verifiedPaths;
    // disallowed for files, or theres only a single path in the vector
    if ((category.Equals("files")) || (vecPaths.size() == 1))
      verifiedPaths.push_back(vecPaths[0]);

    // multiple paths?
    else
    {
      // validate the paths
      for (int j = 0; j < (int)vecPaths.size(); ++j)
      {
        CURL url(vecPaths[j]);
        CStdString protocol = url.GetProtocol();
        bool bIsInvalid = false;

        // for my programs
        if (category.Equals("programs") || category.Equals("myprograms"))
        {
          // only allow HD and plugins
          if (url.IsLocal() || protocol.Equals("plugin"))
            verifiedPaths.push_back(vecPaths[j]);
          else
            bIsInvalid = true;
        }

        // for others allow everything (if the user does something silly, we can't stop them)
        else
          verifiedPaths.push_back(vecPaths[j]);

        // error message
        if (bIsInvalid)
          CLog::Log(LOGERROR,"    Invalid path type (%s) for multipath source", vecPaths[j].c_str());
      }

      // no valid paths? skip to next source
      if (verifiedPaths.size() == 0)
      {
        CLog::Log(LOGERROR,"    Missing or invalid <name> and/or <path> in source");
        return false;
      }
    }

    share.FromNameAndPaths(category, strName, verifiedPaths);

/*    CLog::Log(LOGDEBUG,"      Adding source:");
    CLog::Log(LOGDEBUG,"        Name: %s", share.strName.c_str());
    if (CUtil::IsVirtualPath(share.strPath) || CUtil::IsMultiPath(share.strPath))
    {
      for (int i = 0; i < (int)share.vecPaths.size(); ++i)
        CLog::Log(LOGDEBUG,"        Path (%02i): %s", i+1, share.vecPaths.at(i).c_str());
    }
    else
      CLog::Log(LOGDEBUG,"        Path: %s", share.strPath.c_str());
*/
    share.m_iBadPwdCount = 0;
    if (pLockMode)
    {
      share.m_iLockMode = LockType(atoi(pLockMode->FirstChild()->Value()));
      share.m_iHasLock = 2;
    }

    if (pLockCode)
    {
      if (pLockCode->FirstChild())
        share.m_strLockCode = pLockCode->FirstChild()->Value();
    }

    if (pBadPwdCount)
    {
      if (pBadPwdCount->FirstChild())
        share.m_iBadPwdCount = atoi( pBadPwdCount->FirstChild()->Value() );
    }

    if (pThumbnailNode)
    {
      if (pThumbnailNode->FirstChild())
        share.m_strThumbnailImage = pThumbnailNode->FirstChild()->Value();
    }

    return true;
  }
  return false;
}

bool CSettings::GetString(const TiXmlElement* pRootElement, const char *tagName, CStdString &strValue)
{
  CStdString defaultValue = strValue;
  return GetString(pRootElement, tagName, strValue, defaultValue);
}

bool CSettings::GetString(const TiXmlElement* pRootElement, const char *tagName, CStdString &strValue, const CStdString& strDefaultValue)
{
  if (XMLUtils::GetString(pRootElement, tagName, strValue))
  { // tag exists
    // check for "-" for backward compatibility
    if (!strValue.Equals("-"))
      return true;
  }
  // tag doesn't exist - set default
  strValue = strDefaultValue;
  return false;
}

bool CSettings::GetString(const TiXmlElement* pRootElement, const char *tagName, char *szValue, const CStdString& strDefaultValue)
{
  CStdString strValue;
  bool ret = GetString(pRootElement, tagName, strValue, strDefaultValue);
  if (szValue)
    strcpy(szValue, strValue.c_str());
  return ret;
}

bool CSettings::GetInteger(const TiXmlElement* pRootElement, const char *tagName, int& iValue, const int iDefault, const int iMin, const int iMax)
{
  if (GetInteger(pRootElement, tagName, iValue, iMin, iMax))
    return true;
  // default
  iValue = iDefault;
  return false;
}

bool CSettings::GetInteger(const TiXmlElement* pRootElement, const char *tagName, int& iValue, const int iMin, const int iMax)
{
  if (XMLUtils::GetInt(pRootElement, tagName, iValue))
  { // check range
    if (iValue < iMin) iValue = iMin;
    if (iValue > iMax) iValue = iMax;
    return true;
  }
  return false;
}

bool CSettings::GetFloat(const TiXmlElement* pRootElement, const char *tagName, float& fValue, const float fDefault, const float fMin, const float fMax)
{
  if (GetFloat(pRootElement, tagName, fValue, fMin, fMax))
    return true;
  // default
  fValue = fDefault;
  return false;
}

bool CSettings::GetFloat(const TiXmlElement* pRootElement, const char *tagName, float& fValue, const float fMin, const float fMax)
{
  if (XMLUtils::GetFloat(pRootElement, tagName, fValue))
  { // check range
    if (fValue < fMin) fValue = fMin;
    if (fValue > fMax) fValue = fMax;
    return true;
  }
  return false;
}

void CSettings::GetViewState(const TiXmlElement *pRootElement, const CStdString &strTagName, CViewState &viewState, SORT_METHOD defaultSort)
{
  const TiXmlElement* pNode = pRootElement->FirstChildElement(strTagName);
  if (!pNode)
  {
    viewState.m_sortMethod = defaultSort;
    return;
  }
  GetInteger(pNode, "viewmode", viewState.m_viewMode, DEFAULT_VIEW_LIST, DEFAULT_VIEW_LIST, DEFAULT_VIEW_MAX);
  GetInteger(pNode, "sortmethod", (int&)viewState.m_sortMethod, defaultSort, SORT_METHOD_NONE, SORT_METHOD_MAX);
  GetInteger(pNode, "sortorder", (int&)viewState.m_sortOrder, SORT_ORDER_ASC, SORT_ORDER_NONE, SORT_ORDER_DESC);
}

void CSettings::SetString(TiXmlNode* pRootNode, const CStdString& strTagName, const CStdString& strValue) const
{
  CStdString strPersistedValue = strValue;
  if (strPersistedValue.length() == 0)
  {
    strPersistedValue = '-';
  }

  TiXmlElement newElement(strTagName);
  TiXmlNode *pNewNode = pRootNode->InsertEndChild(newElement);
  if (pNewNode)
  {
    TiXmlText value(strValue);
    pNewNode->InsertEndChild(value);
  }
}

void CSettings::SetInteger(TiXmlNode* pRootNode, const CStdString& strTagName, int iValue) const
{
  CStdString strValue;
  strValue.Format("%d", iValue);
  SetString(pRootNode, strTagName, strValue);
}

void CSettings::SetViewState(TiXmlNode *pRootNode, const CStdString &strTagName, const CViewState &viewState) const
{
  TiXmlElement newElement(strTagName);
  TiXmlNode *pNewNode = pRootNode->InsertEndChild(newElement);
  if (pNewNode)
  {
    SetInteger(pNewNode, "viewmode", viewState.m_viewMode);
    SetInteger(pNewNode, "sortmethod", (int)viewState.m_sortMethod);
    SetInteger(pNewNode, "sortorder", (int)viewState.m_sortOrder);
  }
}

void CSettings::SetFloat(TiXmlNode* pRootNode, const CStdString& strTagName, float fValue) const
{
  CStdString strValue;
  strValue.Format("%f", fValue);
  SetString(pRootNode, strTagName, strValue);
}

void CSettings::SetBoolean(TiXmlNode* pRootNode, const CStdString& strTagName, bool bValue) const
{
  if (bValue)
    SetString(pRootNode, strTagName, "true");
  else
    SetString(pRootNode, strTagName, "false");
}

void CSettings::SetHex(TiXmlNode* pRootNode, const CStdString& strTagName, DWORD dwHexValue) const
{
  CStdString strValue;
  strValue.Format("%x", dwHexValue);
  SetString(pRootNode, strTagName, strValue);
}

bool CSettings::LoadCalibration(const TiXmlElement* pElement, const CStdString& strSettingsFile)
{
  // reset the calibration to the defaults
  //g_graphicsContext.SetD3DParameters(NULL, m_ResInfo);
  //for (int i=0; i<10; i++)
  //  g_graphicsContext.ResetScreenParameters((RESOLUTION)i);

  const TiXmlElement *pRootElement;
  CStdString strTagName = pElement->Value();
  if (!strcmp(strTagName.c_str(), "calibration"))
  {
    pRootElement = pElement;
  }
  else
  {
    pRootElement = pElement->FirstChildElement("calibration");
  }
  if (!pRootElement)
  {
    g_LoadErrorStr.Format("%s Doesn't contain <calibration>", strSettingsFile.c_str());
    return false;
  }
  const TiXmlElement *pResolution = pRootElement->FirstChildElement("resolution");
  while (pResolution)
  {
    // get the data for this resolution
    int iRes;
    CStdString mode;
    GetInteger(pResolution, "id", iRes, (int)PAL_4x3, HDTV_1080i, MAX_RESOLUTIONS); //PAL4x3 as default data
    GetString(pResolution, "description", mode, m_ResInfo[iRes].strMode);
#ifdef HAS_SDL
    if(iRes == DESKTOP && !mode.Equals(m_ResInfo[iRes].strMode))
    {
      CLog::Log(LOGDEBUG, "%s - Ignoring desktop resolution \"%s\" that differs from current \"%s\"", __FUNCTION__, mode.c_str(), m_ResInfo[iRes].strMode);

      pResolution = pResolution->NextSiblingElement("resolution");
      continue;
    }
#endif

    // get the appropriate "safe graphics area" = 10% for 4x3, 3.5% for 16x9
    float fSafe;
    if (iRes == PAL_4x3 || iRes == NTSC_4x3 || iRes == PAL60_4x3 || iRes == HDTV_480p_4x3)
      fSafe = 0.1f;
    else
      fSafe = 0.035f;
    GetInteger(pResolution, "subtitles", m_ResInfo[iRes].iSubtitles, (int)((1 - fSafe)*m_ResInfo[iRes].iHeight), m_ResInfo[iRes].iHeight / 2, m_ResInfo[iRes].iHeight*5 / 4);
    GetFloat(pResolution, "pixelratio", m_ResInfo[iRes].fPixelRatio, 128.0f / 117.0f, 0.5f, 2.0f);

#ifdef HAS_XRANDR
    const CStdString def("");
    CStdString val;
    GetString(pResolution, "xrandrid", val, def);
    strncpy(m_ResInfo[iRes].strId, val.c_str(), sizeof(m_ResInfo[iRes].strId));
    GetString(pResolution, "output", val, def);
    strncpy(m_ResInfo[iRes].strOutput, val.c_str(), sizeof(m_ResInfo[iRes].strOutput));
    GetFloat(pResolution, "refreshrate", m_ResInfo[iRes].fRefreshRate, 0, 0, 200);
#endif

    // get the overscan info
    const TiXmlElement *pOverscan = pResolution->FirstChildElement("overscan");
    if (pOverscan)
    {
      GetInteger(pOverscan, "left", m_ResInfo[iRes].Overscan.left, 0, -m_ResInfo[iRes].iWidth / 4, m_ResInfo[iRes].iWidth / 4);
      GetInteger(pOverscan, "top", m_ResInfo[iRes].Overscan.top, 0, -m_ResInfo[iRes].iHeight / 4, m_ResInfo[iRes].iHeight / 4);
      GetInteger(pOverscan, "right", m_ResInfo[iRes].Overscan.right, m_ResInfo[iRes].iWidth, m_ResInfo[iRes].iWidth / 2, m_ResInfo[iRes].iWidth*3 / 2);
      GetInteger(pOverscan, "bottom", m_ResInfo[iRes].Overscan.bottom, m_ResInfo[iRes].iHeight, m_ResInfo[iRes].iHeight / 2, m_ResInfo[iRes].iHeight*3 / 2);
    }

    CLog::Log(LOGDEBUG, "  calibration for %s %ix%i", m_ResInfo[iRes].strMode, m_ResInfo[iRes].iWidth, m_ResInfo[iRes].iHeight);
    CLog::Log(LOGDEBUG, "    subtitle yposition:%i pixelratio:%03.3f offsets:(%i,%i)->(%i,%i)",
              m_ResInfo[iRes].iSubtitles, m_ResInfo[iRes].fPixelRatio,
              m_ResInfo[iRes].Overscan.left, m_ResInfo[iRes].Overscan.top,
              m_ResInfo[iRes].Overscan.right, m_ResInfo[iRes].Overscan.bottom);

    // iterate around
    pResolution = pResolution->NextSiblingElement("resolution");
  }
  return true;
}

bool CSettings::SaveCalibration(TiXmlNode* pRootNode) const
{
  TiXmlElement xmlRootElement("calibration");
  TiXmlNode *pRoot = pRootNode->InsertEndChild(xmlRootElement);
  for (int i = 0; i < 10; i++)
  {
    // Write the resolution tag
    TiXmlElement resElement("resolution");
    TiXmlNode *pNode = pRoot->InsertEndChild(resElement);
    // Now write each of the pieces of information we need...
    SetString(pNode, "description", m_ResInfo[i].strMode);
    SetInteger(pNode, "id", i);
    SetInteger(pNode, "subtitles", m_ResInfo[i].iSubtitles);
    SetFloat(pNode, "pixelratio", m_ResInfo[i].fPixelRatio);
    // create the overscan child
    TiXmlElement overscanElement("overscan");
    TiXmlNode *pOverscanNode = pNode->InsertEndChild(overscanElement);
    SetInteger(pOverscanNode, "left", m_ResInfo[i].Overscan.left);
    SetInteger(pOverscanNode, "top", m_ResInfo[i].Overscan.top);
    SetInteger(pOverscanNode, "right", m_ResInfo[i].Overscan.right);
    SetInteger(pOverscanNode, "bottom", m_ResInfo[i].Overscan.bottom);
  }

  // save WINDOW, DESKTOP and CUSTOM resolution
  for (int i = (int)WINDOW ; i<(CUSTOM+g_videoConfig.GetNumberOfResolutions()) ; i++)
  {
    // Write the resolution tag
    TiXmlElement resElement("resolution");
    TiXmlNode *pNode = pRoot->InsertEndChild(resElement);
    // Now write each of the pieces of information we need...
    SetString(pNode, "description", m_ResInfo[i].strMode);
    SetInteger(pNode, "id", i);
    SetInteger(pNode, "subtitles", m_ResInfo[i].iSubtitles);
    SetFloat(pNode, "pixelratio", m_ResInfo[i].fPixelRatio);
#ifdef HAS_XRANDR
    SetFloat(pNode, "refreshrate", m_ResInfo[i].fRefreshRate);
    SetString(pNode, "output", m_ResInfo[i].strOutput);
    SetString(pNode, "xrandrid", m_ResInfo[i].strId);
#endif
    // create the overscan child
    TiXmlElement overscanElement("overscan");
    TiXmlNode *pOverscanNode = pNode->InsertEndChild(overscanElement);
    SetInteger(pOverscanNode, "left", m_ResInfo[i].Overscan.left);
    SetInteger(pOverscanNode, "top", m_ResInfo[i].Overscan.top);
    SetInteger(pOverscanNode, "right", m_ResInfo[i].Overscan.right);
    SetInteger(pOverscanNode, "bottom", m_ResInfo[i].Overscan.bottom);
  }
  return true;
}

bool CSettings::LoadSettings(const CStdString& strSettingsFile)
{
  // Load the xml file, and avoid condensing white space. We should probably avoid
  // this elsewhere, but I'll just do it here for now as it fixes a specific problem
  // with audio devices with trailing spaces.
  //
  TiXmlDocument xmlDoc;
  TiXmlBase::SetCondenseWhiteSpace(false);
  
  if (!xmlDoc.LoadFile(strSettingsFile))
  {
    g_LoadErrorStr.Format("%s, Line %d\n%s", strSettingsFile.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
    TiXmlBase::SetCondenseWhiteSpace(true);
    return false;
  }
  
  TiXmlBase::SetCondenseWhiteSpace(true);

  TiXmlElement *pRootElement = xmlDoc.RootElement();
  if (strcmpi(pRootElement->Value(), "settings") != 0)
  {
    g_LoadErrorStr.Format("%s\nDoesn't contain <settings>", strSettingsFile.c_str());
    return false;
  }

  // mymusic settings
  TiXmlElement *pElement = pRootElement->FirstChildElement("mymusic");
  if (pElement)
  {
    TiXmlElement *pChild = pElement->FirstChildElement("playlist");
    if (pChild)
    {
      XMLUtils::GetBoolean(pChild, "repeat", g_stSettings.m_bMyMusicPlaylistRepeat);
      XMLUtils::GetBoolean(pChild, "shuffle", g_stSettings.m_bMyMusicPlaylistShuffle);
    }
    // if the user happened to reboot in the middle of the scan we save this state
    pChild = pElement->FirstChildElement("scanning");
    if (pChild)
    {
      XMLUtils::GetBoolean(pChild, "isscanning", g_stSettings.m_bMyMusicIsScanning);
    }
    GetInteger(pElement, "startwindow", g_stSettings.m_iMyMusicStartWindow, WINDOW_MUSIC_FILES, WINDOW_MUSIC_FILES, WINDOW_MUSIC_NAV); //501; view songs
    XMLUtils::GetBoolean(pElement, "songinfoinvis", g_stSettings.m_bMyMusicSongInfoInVis);
    XMLUtils::GetBoolean(pElement, "songthumbinvis", g_stSettings.m_bMyMusicSongThumbInVis);
    GetString(pElement, "defaultlibview", g_settings.m_defaultMusicLibSource, g_settings.m_defaultMusicLibSource);
    GetString(pElement, "defaultscraper", g_stSettings.m_defaultMusicScraper, g_stSettings.m_defaultMusicScraper);
  }
  // myvideos settings
  pElement = pRootElement->FirstChildElement("myvideos");
  if (pElement)
  {
    GetInteger(pElement, "startwindow", g_stSettings.m_iVideoStartWindow, WINDOW_VIDEO_FILES, WINDOW_VIDEO_FILES, WINDOW_VIDEO_NAV);
    GetInteger(pElement, "stackvideomode", g_stSettings.m_iMyVideoStack, STACK_NONE, STACK_NONE, STACK_SIMPLE);

    GetString(pElement, "cleantokens", g_stSettings.m_szMyVideoCleanTokens, g_stSettings.m_szMyVideoCleanTokens);
    GetString(pElement, "cleanseparators", g_stSettings.m_szMyVideoCleanSeparators, g_stSettings.m_szMyVideoCleanSeparators);

    StringUtils::SplitString(g_stSettings.m_szMyVideoCleanTokens, "|", g_settings.m_szMyVideoCleanTokensArray);
    g_settings.m_szMyVideoCleanSeparatorsString = g_stSettings.m_szMyVideoCleanSeparators;

    for (int i = 0; i < (int)g_settings.m_szMyVideoCleanTokensArray.size(); i++)
      g_settings.m_szMyVideoCleanTokensArray[i].MakeLower();

    GetString(pElement, "defaultlibview", g_settings.m_defaultVideoLibSource, g_settings.m_defaultVideoLibSource);
    GetInteger(pElement, "watchmode", g_stSettings.m_iMyVideoWatchMode, VIDEO_SHOW_ALL, VIDEO_SHOW_ALL, VIDEO_SHOW_WATCHED);
    XMLUtils::GetBoolean(pElement, "flatten", g_stSettings.m_bMyVideoNavFlatten);

    TiXmlElement *pChild = pElement->FirstChildElement("playlist");
    if (pChild)
    { // playlist
      XMLUtils::GetBoolean(pChild, "repeat", g_stSettings.m_bMyVideoPlaylistRepeat);
      XMLUtils::GetBoolean(pChild, "shuffle", g_stSettings.m_bMyVideoPlaylistShuffle);
    }
  }

  pElement = pRootElement->FirstChildElement("viewstates");
  if (pElement)
  {
    GetViewState(pElement, "musicnavartists", g_stSettings.m_viewStateMusicNavArtists);
    GetViewState(pElement, "musicnavalbums", g_stSettings.m_viewStateMusicNavAlbums);
    GetViewState(pElement, "musicnavsongs", g_stSettings.m_viewStateMusicNavSongs);
    GetViewState(pElement, "musicshoutcast", g_stSettings.m_viewStateMusicShoutcast);
    GetViewState(pElement, "musiclastfm", g_stSettings.m_viewStateMusicLastFM);
    GetViewState(pElement, "videonavactors", g_stSettings.m_viewStateVideoNavActors);
    GetViewState(pElement, "videonavyears", g_stSettings.m_viewStateVideoNavYears);
    GetViewState(pElement, "videonavgenres", g_stSettings.m_viewStateVideoNavGenres);
    GetViewState(pElement, "videonavtitles", g_stSettings.m_viewStateVideoNavTitles);
    GetViewState(pElement, "videonavepisodes", g_stSettings.m_viewStateVideoNavEpisodes, SORT_METHOD_EPISODE);
    GetViewState(pElement, "videonavtvshows", g_stSettings.m_viewStateVideoNavTvShows);
    GetViewState(pElement, "videonavseasons", g_stSettings.m_viewStateVideoNavSeasons);
    GetViewState(pElement, "videonavmusicvideos", g_stSettings.m_viewStateVideoNavMusicVideos);
  }

  // general settings
  pElement = pRootElement->FirstChildElement("general");
  if (pElement)
  {
    GetInteger(pElement, "systemtotaluptime", g_stSettings.m_iSystemTimeTotalUp, 0, 0, INT_MAX);
#ifdef HAS_KAI
    GetString(pElement, "kaiarenapass", g_stSettings.szOnlineArenaPassword, "");
    GetString(pElement, "kaiarenadesc", g_stSettings.szOnlineArenaDescription, "");
#endif
    GetInteger(pElement, "httpapibroadcastlevel", g_stSettings.m_HttpApiBroadcastLevel, 0, 0,5);
    GetInteger(pElement, "httpapibroadcastport", g_stSettings.m_HttpApiBroadcastPort, 8278, 1, 65535);
  }

  pElement = pRootElement->FirstChildElement("defaultvideosettings");
  if (pElement)
  {
    GetInteger(pElement, "interlacemethod", (int &)g_stSettings.m_defaultVideoSettings.m_InterlaceMethod, VS_INTERLACEMETHOD_NONE, VS_INTERLACEMETHOD_NONE, VS_INTERLACEMETHOD_RENDER_BLEND);
    GetInteger(pElement, "filmgrain", g_stSettings.m_defaultVideoSettings.m_FilmGrain, 0, 0, 10);
    GetInteger(pElement, "viewmode", g_stSettings.m_defaultVideoSettings.m_ViewMode, VIEW_MODE_NORMAL, VIEW_MODE_NORMAL, VIEW_MODE_CUSTOM);
    GetFloat(pElement, "zoomamount", g_stSettings.m_defaultVideoSettings.m_CustomZoomAmount, 1.0f, 0.5f, 2.0f);
    GetFloat(pElement, "pixelratio", g_stSettings.m_defaultVideoSettings.m_CustomPixelRatio, 1.0f, 0.5f, 2.0f);
    GetFloat(pElement, "volumeamplification", g_stSettings.m_defaultVideoSettings.m_VolumeAmplification, VOLUME_DRC_MINIMUM * 0.01f, VOLUME_DRC_MINIMUM * 0.01f, VOLUME_DRC_MAXIMUM * 0.01f);
    XMLUtils::GetBoolean(pElement, "outputtoallspeakers", g_stSettings.m_defaultVideoSettings.m_OutputToAllSpeakers);
    XMLUtils::GetBoolean(pElement, "showsubtitles", g_stSettings.m_defaultVideoSettings.m_SubtitleOn);
    GetInteger(pElement, "brightness", g_stSettings.m_defaultVideoSettings.m_Brightness, 50, 0, 100);
    GetInteger(pElement, "contrast", g_stSettings.m_defaultVideoSettings.m_Contrast, 50, 0, 100);
    GetInteger(pElement, "gamma", g_stSettings.m_defaultVideoSettings.m_Gamma, 20, 0, 100);
    GetFloat(pElement, "audiodelay", g_stSettings.m_defaultVideoSettings.m_AudioDelay, 0.0f, -10.0f, 10.0f);
    GetFloat(pElement, "subtitledelay", g_stSettings.m_defaultVideoSettings.m_SubtitleDelay, 0.0f, -10.0f, 10.0f);

    g_stSettings.m_defaultVideoSettings.m_SubtitleCached = false;
  }
  // audio settings
  pElement = pRootElement->FirstChildElement("audio");
  if (pElement)
  {
    GetInteger(pElement, "volumelevel", g_stSettings.m_nVolumeLevel, VOLUME_MAXIMUM, VOLUME_MINIMUM, VOLUME_MAXIMUM);
    GetInteger(pElement, "dynamicrangecompression", g_stSettings.m_dynamicRangeCompressionLevel, VOLUME_DRC_MINIMUM, VOLUME_DRC_MINIMUM, VOLUME_DRC_MAXIMUM);
    for (int i = 0; i < 4; i++)
    {
      CStdString setting;
      setting.Format("karaoke%i", i);
#ifndef HAS_XBOX_AUDIO
#define XVOICE_MASK_PARAM_DISABLED (-1.0f)
#endif
      GetFloat(pElement, setting + "energy", g_stSettings.m_karaokeVoiceMask[i].energy, XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED, 1.0f);
      GetFloat(pElement, setting + "pitch", g_stSettings.m_karaokeVoiceMask[i].pitch, XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED, 1.0f);
      GetFloat(pElement, setting + "whisper", g_stSettings.m_karaokeVoiceMask[i].whisper, XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED, 1.0f);
      GetFloat(pElement, setting + "robotic", g_stSettings.m_karaokeVoiceMask[i].robotic, XVOICE_MASK_PARAM_DISABLED, XVOICE_MASK_PARAM_DISABLED, 1.0f);
    }
  }

  LoadCalibration(pRootElement, strSettingsFile);
  g_guiSettings.LoadXML(pRootElement);
  LoadSkinSettings(pRootElement);

  // Advanced settings
  LoadAdvancedSettings();

  return true;
}

void CSettings::LoadAdvancedSettings()
{
  // NOTE: This routine should NOT set the default of any of these parameters
  //       it should instead use the versions of GetString/Integer/Float that
  //       don't take defaults in.  Defaults are set in the constructor above
  CStdString advancedSettingsXML;
  advancedSettingsXML  = g_settings.GetUserDataItem("advancedsettings.xml");
  TiXmlDocument advancedXML;
  if (!CFile::Exists(advancedSettingsXML))
    return;

  if (!advancedXML.LoadFile(advancedSettingsXML.c_str()))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d\n%s", advancedSettingsXML.c_str(), advancedXML.ErrorRow(), advancedXML.ErrorDesc());
    return;
  }

  TiXmlElement *pRootElement = advancedXML.RootElement();
  if (!pRootElement || strcmpi(pRootElement->Value(),"advancedsettings") != 0)
  {
    CLog::Log(LOGERROR, "Error loading %s, no <advancedsettings> node", advancedSettingsXML.c_str());
    return;
  }

  TiXmlElement *pElement = pRootElement->FirstChildElement("audio");
  if (pElement)
  {
    GetInteger(pElement, "headroom", g_advancedSettings.m_audioHeadRoom, 0, 12);
    GetFloat(pElement, "karaokesyncdelay", g_advancedSettings.m_karaokeSyncDelay, -3.0f, 3.0f);

    XMLUtils::GetBoolean(pElement, "usetimeseeking", g_advancedSettings.m_musicUseTimeSeeking);
    GetInteger(pElement, "timeseekforward", g_advancedSettings.m_musicTimeSeekForward, 0, 6000);
    GetInteger(pElement, "timeseekbackward", g_advancedSettings.m_musicTimeSeekBackward, -6000, 0);
    GetInteger(pElement, "timeseekforwardbig", g_advancedSettings.m_musicTimeSeekForwardBig, 0, 6000);
    GetInteger(pElement, "timeseekbackwardbig", g_advancedSettings.m_musicTimeSeekBackwardBig, -6000, 0);

    GetInteger(pElement, "percentseekforward", g_advancedSettings.m_musicPercentSeekForward, 0, 100);
    GetInteger(pElement, "percentseekbackward", g_advancedSettings.m_musicPercentSeekBackward, -100, 0);
    GetInteger(pElement, "percentseekforwardbig", g_advancedSettings.m_musicPercentSeekForwardBig, 0, 100);
    GetInteger(pElement, "percentseekbackwardbig", g_advancedSettings.m_musicPercentSeekBackwardBig, -100, 0);
  }

  pElement = pRootElement->FirstChildElement("video");
  if (pElement)
  {
    GetFloat(pElement, "subsdelayrange", g_advancedSettings.m_videoSubsDelayRange, 10, 600);
    GetFloat(pElement, "audiodelayrange", g_advancedSettings.m_videoAudioDelayRange, 10, 600);
    GetInteger(pElement, "smallstepbackseconds", g_advancedSettings.m_videoSmallStepBackSeconds, 1, INT_MAX);
    GetInteger(pElement, "smallstepbacktries", g_advancedSettings.m_videoSmallStepBackTries, 1, 10);
    GetInteger(pElement, "smallstepbackdelay", g_advancedSettings.m_videoSmallStepBackDelay, 100, 5000); //MS

    XMLUtils::GetBoolean(pElement, "usetimeseeking", g_advancedSettings.m_videoUseTimeSeeking);
    GetInteger(pElement, "timeseekforward", g_advancedSettings.m_videoTimeSeekForward, 0, 6000);
    GetInteger(pElement, "timeseekbackward", g_advancedSettings.m_videoTimeSeekBackward, -6000, 0);
    GetInteger(pElement, "timeseekforwardbig", g_advancedSettings.m_videoTimeSeekForwardBig, 0, 6000);
    GetInteger(pElement, "timeseekbackwardbig", g_advancedSettings.m_videoTimeSeekBackwardBig, -6000, 0);

    GetInteger(pElement, "percentseekforward", g_advancedSettings.m_videoPercentSeekForward, 0, 100);
    GetInteger(pElement, "percentseekbackward", g_advancedSettings.m_videoPercentSeekBackward, -100, 0);
    GetInteger(pElement, "percentseekforwardbig", g_advancedSettings.m_videoPercentSeekForwardBig, 0, 100);
    GetInteger(pElement, "percentseekbackwardbig", g_advancedSettings.m_videoPercentSeekBackwardBig, -100, 0);
    GetInteger(pElement, "blackbarcolour", g_advancedSettings.m_videoBlackBarColour, 0, 255);
  }

  pElement = pRootElement->FirstChildElement("musiclibrary");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "hideallitems", g_advancedSettings.m_bMusicLibraryHideAllItems);
    XMLUtils::GetBoolean(pElement, "prioritiseapetags", g_advancedSettings.m_prioritiseAPEv2tags);
    XMLUtils::GetBoolean(pElement, "allitemsonbottom", g_advancedSettings.m_bMusicLibraryAllItemsOnBottom);
    XMLUtils::GetBoolean(pElement, "albumssortbyartistthenyear", g_advancedSettings.m_bMusicLibraryAlbumsSortByArtistThenYear);
    GetString(pElement, "albumformat", g_advancedSettings.m_strMusicLibraryAlbumFormat);
    GetString(pElement, "albumformatright", g_advancedSettings.m_strMusicLibraryAlbumFormatRight);
    GetString(pElement, "itemseparator", g_advancedSettings.m_musicItemSeparator);
  }

  pElement = pRootElement->FirstChildElement("videolibrary");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "hideallitems", g_advancedSettings.m_bVideoLibraryHideAllItems);
    XMLUtils::GetBoolean(pElement, "allitemsonbottom", g_advancedSettings.m_bVideoLibraryAllItemsOnBottom);
    XMLUtils::GetBoolean(pElement, "hiderecentlyaddeditems", g_advancedSettings.m_bVideoLibraryHideRecentlyAddedItems);
    XMLUtils::GetBoolean(pElement, "hideemptyseries", g_advancedSettings.m_bVideoLibraryHideEmptySeries);
    XMLUtils::GetBoolean(pElement, "cleanonupdate", g_advancedSettings.m_bVideoLibraryCleanOnUpdate);
    GetString(pElement, "itemseparator", g_advancedSettings.m_videoItemSeparator);
  }

  pElement = pRootElement->FirstChildElement("slideshow");
  if (pElement)
  {
    GetFloat(pElement, "panamount", g_advancedSettings.m_slideshowPanAmount, 0.0f, 20.0f);
    GetFloat(pElement, "zoomamount", g_advancedSettings.m_slideshowZoomAmount, 0.0f, 20.0f);
    GetFloat(pElement, "blackbarcompensation", g_advancedSettings.m_slideshowBlackBarCompensation, 0.0f, 50.0f);
  }

  pElement = pRootElement->FirstChildElement("lcd");
  if (pElement)
  {
    GetInteger(pElement, "rows", g_advancedSettings.m_lcdRows, 1, 4);
    GetInteger(pElement, "columns", g_advancedSettings.m_lcdColumns, 1, 40);
    GetInteger(pElement, "address1", g_advancedSettings.m_lcdAddress1, 0, 0x100);
    GetInteger(pElement, "address2", g_advancedSettings.m_lcdAddress2, 0, 0x100);
    GetInteger(pElement, "address3", g_advancedSettings.m_lcdAddress3, 0, 0x100);
    GetInteger(pElement, "address4", g_advancedSettings.m_lcdAddress4, 0, 0x100);
  }
  pElement = pRootElement->FirstChildElement("network");
  if (pElement)
  {
    GetInteger(pElement, "autodetectpingtime", g_advancedSettings.m_autoDetectPingTime, 1, 240);
    GetInteger(pElement, "curlclienttimeout", g_advancedSettings.m_curlclienttimeout, 1, 1000);
    GetInteger(pElement, "curlglobalidletimeout", g_advancedSettings.m_curlGlobalIdleTimeout, 1, 200000);
  }

  GetFloat(pRootElement, "playcountminimumpercent", g_advancedSettings.m_playCountMinimumPercent, 1.0f, 100.0f);

  pElement = pRootElement->FirstChildElement("samba");
  if (pElement)
  {
    GetString(pElement,  "doscodepage",   g_advancedSettings.m_sambadoscodepage);
    GetInteger(pElement, "clienttimeout", g_advancedSettings.m_sambaclienttimeout, 5, 100);
    XMLUtils::GetBoolean(pElement, "statfiles", g_advancedSettings.m_sambastatfiles);
  }

  if (GetInteger(pRootElement, "loglevel", g_advancedSettings.m_logLevel, LOG_LEVEL_NONE, LOG_LEVEL_MAX))
  { // read the loglevel setting, so set the setting advanced to hide it in GUI
    // as altering it will do nothing - we don't write to advancedsettings.xml
    CSettingBool *setting = (CSettingBool *)g_guiSettings.GetSetting("system.debuglogging");
    if (setting)
    {
      setting->SetData(g_advancedSettings.m_logLevel >= LOG_LEVEL_DEBUG_FREEMEM);
      setting->SetAdvanced();
    }
  }
  GetString(pRootElement, "cddbaddress", g_advancedSettings.m_cddbAddress);
#ifdef HAS_HAL
  XMLUtils::GetBoolean(pRootElement, "usehalmount", g_advancedSettings.m_useHalMount);
#endif
  XMLUtils::GetBoolean(pRootElement, "nodvdrom", g_advancedSettings.m_noDVDROM);
  XMLUtils::GetBoolean(pRootElement, "enableopticalmedia", g_advancedSettings.m_enableOpticalMedia);
  XMLUtils::GetBoolean(pRootElement, "usemultipaths", g_advancedSettings.m_useMultipaths);
  XMLUtils::GetBoolean(pRootElement, "disablemodchipdetection", g_advancedSettings.m_DisableModChipDetection);

#ifdef HAS_SDL
  XMLUtils::GetBoolean(pRootElement, "fakefullscreen", g_advancedSettings.m_fakeFullScreen);
#endif
  GetInteger(pRootElement, "songinfoduration", g_advancedSettings.m_songInfoDuration, 15, 1, 15);
  GetInteger(pRootElement, "busydialogdelay", g_advancedSettings.m_busyDialogDelay, 2000, 0, 5000);
  GetInteger(pRootElement, "playlistretries", g_advancedSettings.m_playlistRetries, 100, -1, 5000);
  GetInteger(pRootElement, "playlisttimeout", g_advancedSettings.m_playlistTimeout, 20, 0, 5000);

  XMLUtils::GetBoolean(pRootElement,"rootovershoot",g_advancedSettings.m_bUseEvilB);
  
  GetInteger(pRootElement, "secondstovisualizer", g_advancedSettings.m_secondsToVisualizer, 10, 0, 6000);
  GetInteger(pRootElement, "nowplayingfliptime", g_advancedSettings.m_nowPlayingFlipTime, 120, 10, 6000);
  XMLUtils::GetBoolean(pRootElement, "visualizeronplay", g_advancedSettings.m_bVisualizerOnPlay);
  XMLUtils::GetBoolean(pRootElement, "backgroundmusiconlywhenfocused", g_advancedSettings.m_bBackgroundMusicOnlyWhenFocused);
  
  XMLUtils::GetBoolean(pRootElement, "autoshuffle", g_advancedSettings.m_bAutoShuffle);
  XMLUtils::GetBoolean(pRootElement, "anamorphiczoom", g_advancedSettings.m_bUseAnamorphicZoom);
  
  XMLUtils::GetBoolean(pRootElement, "enableviewrestrictions", g_advancedSettings.m_bEnableViewRestrictions);
  XMLUtils::GetBoolean(pRootElement, "enablekeyboardbacklightcontrol", g_advancedSettings.m_bEnableKeyboardBacklightControl);
  

  GetString(pRootElement, "language", g_advancedSettings.m_language);
  GetString(pRootElement, "units", g_advancedSettings.m_units);
  
  //Tuxbox
  pElement = pRootElement->FirstChildElement("tuxbox");
  if (pElement)
  {
    XMLUtils::GetBoolean(pElement, "audiochannelselection", g_advancedSettings.m_bTuxBoxAudioChannelSelection);
    XMLUtils::GetBoolean(pElement, "submenuselection", g_advancedSettings.m_bTuxBoxSubMenuSelection);
    XMLUtils::GetBoolean(pElement, "pictureicon", g_advancedSettings.m_bTuxBoxPictureIcon);
    XMLUtils::GetBoolean(pElement, "sendallaudiopids", g_advancedSettings.m_bTuxBoxSendAllAPids);
    GetInteger(pElement, "epgrequesttime", g_advancedSettings.m_iTuxBoxEpgRequestTime, 0, 3600);
    GetInteger(pElement, "defaultsubmenu", g_advancedSettings.m_iTuxBoxDefaultSubMenu, 1, 4);
    GetInteger(pElement, "defaultrootmenu", g_advancedSettings.m_iTuxBoxDefaultRootMenu, 0, 4);
    GetInteger(pElement, "zapwaittime", g_advancedSettings.m_iTuxBoxZapWaitTime, 0, 120);

  }

  CStdString extraExtensions;
  TiXmlElement* pExts = pRootElement->FirstChildElement("pictureextensions");
  if (pExts)
  {
    GetString(pExts,"add",extraExtensions,"");
    if (extraExtensions != "")
      g_stSettings.m_pictureExtensions += "|" + extraExtensions;
    GetString(pExts,"remove",extraExtensions,"");
    if (extraExtensions != "")
    {
      CStdStringArray exts;
      StringUtils::SplitString(extraExtensions,"|",exts);
      for (unsigned int i=0;i<exts.size();++i)
      {
        int iPos = g_stSettings.m_pictureExtensions.Find(exts[i]);
        if (iPos == -1)
          continue;
        g_stSettings.m_pictureExtensions.erase(iPos,exts[i].size()+1);
      }
    }
  }
  pExts = pRootElement->FirstChildElement("musicextensions");
  if (pExts)
  {
    GetString(pExts,"add",extraExtensions,"");
    if (extraExtensions != "")
      g_stSettings.m_musicExtensions += "|" + extraExtensions;
    GetString(pExts,"remove",extraExtensions,"");
    if (extraExtensions != "")
    {
      CStdStringArray exts;
      StringUtils::SplitString(extraExtensions,"|",exts);
      for (unsigned int i=0;i<exts.size();++i)
      {
        int iPos = g_stSettings.m_musicExtensions.Find(exts[i]);
        if (iPos == -1)
          continue;
        g_stSettings.m_musicExtensions.erase(iPos,exts[i].size()+1);
      }
    }
  }
  pExts = pRootElement->FirstChildElement("videoextensions");
  if (pExts)
  {
    GetString(pExts,"add",extraExtensions,"");
    if (extraExtensions != "")
      g_stSettings.m_videoExtensions += "|" + extraExtensions;
    GetString(pExts,"remove",extraExtensions,"");
    if (extraExtensions != "")
    {
      CStdStringArray exts;
      StringUtils::SplitString(extraExtensions,"|",exts);
      for (unsigned int i=0;i<exts.size();++i)
      {
        int iPos = g_stSettings.m_videoExtensions.Find(exts[i]);
        if (iPos == -1)
          continue;
        g_stSettings.m_videoExtensions.erase(iPos,exts[i].size()+1);
      }
    }
  }

  const TiXmlNode *pTokens = pRootElement->FirstChild("sorttokens");
  g_advancedSettings.m_vecTokens.clear();
  if (pTokens && !pTokens->NoChildren())
  {
    const TiXmlNode *pToken = pTokens->FirstChild("token");
    while (pToken)
    {
      if (pToken->FirstChild() && pToken->FirstChild()->Value())
        g_advancedSettings.m_vecTokens.push_back(CStdString(pToken->FirstChild()->Value()) + " ");
      pToken = pToken->NextSibling();
    }
  }

  XMLUtils::GetBoolean(pRootElement, "displayremotecodes", g_advancedSettings.m_displayRemoteCodes);

  // TODO: Should cache path be given in terms of our predefined paths??
  //       Are we even going to have predefined paths??
  GetString(pRootElement, "cachepath", g_advancedSettings.m_cachePath);
  g_advancedSettings.m_cachePath = CUtil::TranslateSpecialSource(g_advancedSettings.m_cachePath);
  CUtil::AddSlashAtEnd(g_advancedSettings.m_cachePath);

  XMLUtils::GetBoolean(pRootElement, "ftpshowcache", g_advancedSettings.m_FTPShowCache);

  g_LangCodeExpander.LoadUserCodes(pRootElement->FirstChildElement("languagecodes"));
  // stacking regexps
  TiXmlElement* pVideoStacking = pRootElement->FirstChildElement("moviestacking");
  if (pVideoStacking)
  {
    const char* szAppend = pVideoStacking->Attribute("append");
    if ((szAppend && stricmp(szAppend,"yes") != 0) || !szAppend)
      g_advancedSettings.m_videoStackRegExps.clear();
    TiXmlNode* pStackRegExp = pVideoStacking->FirstChild("regexp");
    while (pStackRegExp)
    {
      if (pStackRegExp->FirstChild())
      {
        CStdString regExp = pStackRegExp->FirstChild()->Value();
        regExp.MakeLower();
        g_advancedSettings.m_videoStackRegExps.push_back(regExp);
      }
      pStackRegExp = pStackRegExp->NextSibling("regexp");
    }
  }
  //tv stacking regexps
  TiXmlElement* pTVStacking = pRootElement->FirstChildElement("tvshowmatching");
  if (pTVStacking)
  {
    const char* szAppend = pTVStacking->Attribute("append");
    if ((szAppend && stricmp(szAppend,"yes") != 0) || !szAppend)
        g_advancedSettings.m_tvshowStackRegExps.clear();
    TiXmlNode* pStackRegExp = pTVStacking->FirstChild("regexp");
    while (pStackRegExp)
    {
      if (pStackRegExp->FirstChild())
      {
        CStdString regExp = pStackRegExp->FirstChild()->Value();
        regExp.MakeLower();
        g_advancedSettings.m_tvshowStackRegExps.push_back(regExp);
      }
      pStackRegExp = pStackRegExp->NextSibling("regexp");
    }
  }
  // path substitutions
  TiXmlElement* pPathSubstitution = pRootElement->FirstChildElement("pathsubstitution");
  if (pPathSubstitution)
  {
    g_advancedSettings.m_pathSubstitutions.clear();
    CLog::Log(LOGDEBUG,"Configuring path substitutions");
    TiXmlNode* pSubstitute = pPathSubstitution->FirstChildElement("substitute");
    while (pSubstitute)
    {
      CStdString strFrom, strTo;
      TiXmlNode* pFrom = pSubstitute->FirstChild("from");
      if (pFrom)
        strFrom = pFrom->FirstChild()->Value();
      TiXmlNode* pTo = pSubstitute->FirstChild("to");
      if (pTo)
        strTo = pTo->FirstChild()->Value();

      if (!strFrom.IsEmpty() && !strTo.IsEmpty())
      {
        CLog::Log(LOGDEBUG,"  Registering substition pair:");
        CLog::Log(LOGDEBUG,"    From: [%s]", strFrom.c_str());
        CLog::Log(LOGDEBUG,"    To:   [%s]", strTo.c_str());
        // keep literal commas since we use comma as a seperator
        strFrom.Replace(",",",,");
        strTo.Replace(",",",,");
        g_advancedSettings.m_pathSubstitutions.push_back(strFrom + " , " + strTo);
      }
      else
      {
        // error message about missing tag
        if (strFrom.IsEmpty())
          CLog::Log(LOGERROR,"  Missing <from> tag");
        else
          CLog::Log(LOGERROR,"  Missing <to> tag");
      }

      // get next one
      pSubstitute = pSubstitute->NextSiblingElement("substitute");
    }
  }

  GetInteger(pRootElement, "remoterepeat", g_advancedSettings.m_remoteRepeat, 1, INT_MAX);
  GetFloat(pRootElement, "controllerdeadzone", g_advancedSettings.m_controllerDeadzone, 0.0f, 1.0f);
  GetInteger(pRootElement, "thumbsize", g_advancedSettings.m_thumbSize, g_advancedSettings.m_thumbSize, 64, 1024);

  XMLUtils::GetBoolean(pRootElement, "playlistasfolders", g_advancedSettings.m_playlistAsFolders);
  XMLUtils::GetBoolean(pRootElement, "detectasudf", g_advancedSettings.m_detectAsUdf);

  // music thumbs
  CStdString extraThumbs;
  TiXmlElement* pThumbs = pRootElement->FirstChildElement("musicthumbs");
  if (pThumbs)
  {
    // remove before add so that the defaults can be restored after user defined ones
    // (ie, the list can be:cover.jpg|cover.png|folder.jpg)
    GetString(pThumbs, "remove", extraThumbs, "");
    if (extraThumbs != "")
    {
      CStdStringArray thumbs;
      StringUtils::SplitString(extraThumbs, "|", thumbs);
      for (unsigned int i = 0; i < thumbs.size(); ++i)
      {
        int iPos = g_advancedSettings.m_musicThumbs.Find(thumbs[i]);
        if (iPos == -1)
          continue;
        g_advancedSettings.m_musicThumbs.erase(iPos, thumbs[i].size() + 1);
      }
    }
    GetString(pThumbs, "add", extraThumbs,"");
    if (extraThumbs != "")
    {
      if (!g_advancedSettings.m_musicThumbs.IsEmpty())
        g_advancedSettings.m_musicThumbs += "|";
      g_advancedSettings.m_musicThumbs += extraThumbs;
    }
  }

  // dvd thumbs
  pThumbs = pRootElement->FirstChildElement("dvdthumbs");
  if (pThumbs)
  {
    // remove before add so that the defaults can be restored after user defined ones
    // (ie, the list can be:cover.jpg|cover.png|folder.jpg)
    GetString(pThumbs, "remove", extraThumbs, "");
    if (extraThumbs != "")
    {
      CStdStringArray thumbs;
      StringUtils::SplitString(extraThumbs, "|", thumbs);
      for (unsigned int i = 0; i < thumbs.size(); ++i)
      {
        int iPos = g_advancedSettings.m_musicThumbs.Find(thumbs[i]);
        if (iPos == -1)
          continue;
        g_advancedSettings.m_musicThumbs.erase(iPos, thumbs[i].size() + 1);
      }
    }
    GetString(pThumbs, "add", extraThumbs,"");
    if (extraThumbs != "")
    {
      if (!g_advancedSettings.m_musicThumbs.IsEmpty())
        g_advancedSettings.m_musicThumbs += "|";
      g_advancedSettings.m_musicThumbs += extraThumbs;
    }
  }

  // music filename->tag filters
  TiXmlElement* filters = pRootElement->FirstChildElement("musicfilenamefilters");
  if (filters)
  {
    TiXmlNode* filter = filters->FirstChild("filter");
    while (filter)
    {
      if (filter->FirstChild())
        g_advancedSettings.m_musicTagsFromFileFilters.push_back(filter->FirstChild()->ValueStr());
      filter = filter->NextSibling("filter");
    }
  }

  TiXmlElement* pHostEntries = pRootElement->FirstChildElement("hosts");
  if (pHostEntries)
  {
    TiXmlElement* element = pHostEntries->FirstChildElement("entry");
    while(element)
    {
      CStdString name  = element->Attribute("name");
      CStdString value;
      if(element->GetText())
        value = element->GetText();

      if(name.length() > 0 && value.length() > 0)
        CDNSNameCache::Add(name, value);
      element = element->NextSiblingElement("entry");
    }
  }
  
  // Override the default battery warning settings in AppleHardwareInfo
  TiXmlElement* pBatteryWarnings = pRootElement->FirstChildElement("batterywarnings");
  if (pBatteryWarnings)
  {
    int iValue;
    if (XMLUtils::GetInt(pBatteryWarnings, "timeremaining", iValue))
      Cocoa_HW_SetBatteryTimeWarning(iValue);
    if (XMLUtils::GetInt(pBatteryWarnings, "percentremaining", iValue))
      Cocoa_HW_SetBatteryCapacityWarning(iValue);
  }

  // load in the GUISettings overrides:
  g_guiSettings.LoadXML(pRootElement, true);  // true to hide the settings we read in
}

bool CSettings::SaveSettings(const CStdString& strSettingsFile, CGUISettings *localSettings /* = NULL */) const
{
  TiXmlDocument xmlDoc;
  TiXmlElement xmlRootElement("settings");
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;
  // write our tags one by one - just a big list for now (can be flashed up later)

  // mymusic settings
  TiXmlElement musicNode("mymusic");
  TiXmlNode *pNode = pRoot->InsertEndChild(musicNode);
  if (!pNode) return false;
  {
    TiXmlElement childNode("playlist");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    SetBoolean(pChild, "repeat", g_stSettings.m_bMyMusicPlaylistRepeat);
    SetBoolean(pChild, "shuffle", g_stSettings.m_bMyMusicPlaylistShuffle);
  }
  {
    TiXmlElement childNode("scanning");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    SetBoolean(pChild, "isscanning", g_stSettings.m_bMyMusicIsScanning);
  }

  SetInteger(pNode, "startwindow", g_stSettings.m_iMyMusicStartWindow);
  SetBoolean(pNode, "songinfoinvis", g_stSettings.m_bMyMusicSongInfoInVis);
  SetBoolean(pNode, "songthumbinvis", g_stSettings.m_bMyMusicSongThumbInVis);
  SetString(pNode, "defaultlibview", g_settings.m_defaultMusicLibSource);
  SetString(pNode, "defaultscraper", g_stSettings.m_defaultMusicScraper);

  // myvideos settings
  TiXmlElement videosNode("myvideos");
  pNode = pRoot->InsertEndChild(videosNode);
  if (!pNode) return false;

  SetInteger(pNode, "startwindow", g_stSettings.m_iVideoStartWindow);
  SetInteger(pNode, "stackvideomode", g_stSettings.m_iMyVideoStack);

  SetString(pNode, "cleantokens", g_stSettings.m_szMyVideoCleanTokens);
  SetString(pNode, "cleanseparators", g_stSettings.m_szMyVideoCleanSeparators);
  SetString(pNode, "defaultlibview", g_settings.m_defaultVideoLibSource);

  SetInteger(pNode, "watchmode", g_stSettings.m_iMyVideoWatchMode);
  SetBoolean(pNode, "flatten", g_stSettings.m_bMyVideoNavFlatten);

  { // playlist window
    TiXmlElement childNode("playlist");
    TiXmlNode *pChild = pNode->InsertEndChild(childNode);
    if (!pChild) return false;
    SetBoolean(pChild, "repeat", g_stSettings.m_bMyVideoPlaylistRepeat);
    SetBoolean(pChild, "shuffle", g_stSettings.m_bMyVideoPlaylistShuffle);
  }

  // view states
  TiXmlElement viewStateNode("viewstates");
  pNode = pRoot->InsertEndChild(viewStateNode);
  if (pNode)
  {
    SetViewState(pNode, "musicnavartists", g_stSettings.m_viewStateMusicNavArtists);
    SetViewState(pNode, "musicnavalbums", g_stSettings.m_viewStateMusicNavAlbums);
    SetViewState(pNode, "musicnavsongs", g_stSettings.m_viewStateMusicNavSongs);
    SetViewState(pNode, "musicshoutcast", g_stSettings.m_viewStateMusicShoutcast);
    SetViewState(pNode, "musiclastfm", g_stSettings.m_viewStateMusicLastFM);
    SetViewState(pNode, "videonavactors", g_stSettings.m_viewStateVideoNavActors);
    SetViewState(pNode, "videonavyears", g_stSettings.m_viewStateVideoNavYears);
    SetViewState(pNode, "videonavgenres", g_stSettings.m_viewStateVideoNavGenres);
    SetViewState(pNode, "videonavtitles", g_stSettings.m_viewStateVideoNavTitles);
    SetViewState(pNode, "videonavepisodes", g_stSettings.m_viewStateVideoNavEpisodes);
    SetViewState(pNode, "videonavseasons", g_stSettings.m_viewStateVideoNavSeasons);
    SetViewState(pNode, "videonavtvshows", g_stSettings.m_viewStateVideoNavTvShows);
    SetViewState(pNode, "videonavmusicvideos", g_stSettings.m_viewStateVideoNavMusicVideos);
  }

  // general settings
  TiXmlElement generalNode("general");
  pNode = pRoot->InsertEndChild(generalNode);
  if (!pNode) return false;
#ifdef HAS_KAI
  SetString(pNode, "kaiarenapass", g_stSettings.szOnlineArenaPassword);
  SetString(pNode, "kaiarenadesc", g_stSettings.szOnlineArenaDescription);
#endif
  SetInteger(pNode, "systemtotaluptime", g_stSettings.m_iSystemTimeTotalUp);
  SetInteger(pNode, "httpapibroadcastport", g_stSettings.m_HttpApiBroadcastPort);
  SetInteger(pNode, "httpapibroadcastlevel", g_stSettings.m_HttpApiBroadcastLevel);

  // default video settings
  TiXmlElement videoSettingsNode("defaultvideosettings");
  pNode = pRoot->InsertEndChild(videoSettingsNode);
  if (!pNode) return false;
  SetInteger(pNode, "interlacemethod", g_stSettings.m_defaultVideoSettings.m_InterlaceMethod);
  SetInteger(pNode, "filmgrain", g_stSettings.m_defaultVideoSettings.m_FilmGrain);
  SetInteger(pNode, "viewmode", g_stSettings.m_defaultVideoSettings.m_ViewMode);
  SetFloat(pNode, "zoomamount", g_stSettings.m_defaultVideoSettings.m_CustomZoomAmount);
  SetFloat(pNode, "pixelratio", g_stSettings.m_defaultVideoSettings.m_CustomPixelRatio);
  SetFloat(pNode, "volumeamplification", g_stSettings.m_defaultVideoSettings.m_VolumeAmplification);
  SetBoolean(pNode, "outputtoallspeakers", g_stSettings.m_defaultVideoSettings.m_OutputToAllSpeakers);
  SetBoolean(pNode, "showsubtitles", g_stSettings.m_defaultVideoSettings.m_SubtitleOn);
  SetInteger(pNode, "brightness", g_stSettings.m_defaultVideoSettings.m_Brightness);
  SetInteger(pNode, "contrast", g_stSettings.m_defaultVideoSettings.m_Contrast);
  SetInteger(pNode, "gamma", g_stSettings.m_defaultVideoSettings.m_Gamma);
  SetFloat(pNode, "audiodelay", g_stSettings.m_defaultVideoSettings.m_AudioDelay);
  SetFloat(pNode, "subtitledelay", g_stSettings.m_defaultVideoSettings.m_SubtitleDelay);


  // audio settings
  TiXmlElement volumeNode("audio");
  pNode = pRoot->InsertEndChild(volumeNode);
  if (!pNode) return false;
  SetInteger(pNode, "volumelevel", g_stSettings.m_nVolumeLevel);
  SetInteger(pNode, "dynamicrangecompression", g_stSettings.m_dynamicRangeCompressionLevel);
  for (int i = 0; i < 4; i++)
  {
    CStdString setting;
    setting.Format("karaoke%i", i);
    SetFloat(pNode, setting + "energy", g_stSettings.m_karaokeVoiceMask[i].energy);
    SetFloat(pNode, setting + "pitch", g_stSettings.m_karaokeVoiceMask[i].pitch);
    SetFloat(pNode, setting + "whisper", g_stSettings.m_karaokeVoiceMask[i].whisper);
    SetFloat(pNode, setting + "robotic", g_stSettings.m_karaokeVoiceMask[i].robotic);
  }

  SaveCalibration(pRoot);

  if (localSettings) // local settings to save
    localSettings->SaveXML(pRoot);
  else // save the global settings
    g_guiSettings.SaveXML(pRoot);

  SaveSkinSettings(pRoot);

  // For mastercode
  SaveProfiles( PROFILES_FILE );

  // save the file
  return xmlDoc.SaveFile(_P(strSettingsFile));
}

bool CSettings::LoadProfile(int index)
{
  int iOldIndex = m_iLastLoadedProfileIndex;
  m_iLastLoadedProfileIndex = index;
  bool bSourcesXML=true;
  CStdString strOldSkin = g_guiSettings.GetString("lookandfeel.skin");
  CStdString strOldFont = g_guiSettings.GetString("lookandfeel.font");
  CStdString strOldTheme = g_guiSettings.GetString("lookandfeel.skintheme");
  CStdString strOldColors = g_guiSettings.GetString("lookandfeel.skincolors");
  //int iOldRes = g_guiSettings.GetInt("videoscreen.resolution");
  if (Load(bSourcesXML,bSourcesXML))
  {
    g_settings.CreateProfileFolders();
    CreateDirectory(_P("P:\\visualisations"),NULL);

    // initialize our charset converter
    g_charsetConverter.reset();

    // Load the langinfo to have user charset <-> utf-8 conversion
    CStdString strLanguage = g_settings.GetLanguage();
    strLanguage[0] = toupper(strLanguage[0]);

    CStdString strLangInfoPath;
    strLangInfoPath.Format("Q:\\language\\%s\\langinfo.xml", strLanguage.c_str());
    strLangInfoPath = _P(strLangInfoPath);
    CLog::Log(LOGINFO, "load language info file:%s", strLangInfoPath.c_str());
    g_langInfo.Load(strLangInfoPath);

    CStdString strKeyboardLayoutConfigurationPath;
    strKeyboardLayoutConfigurationPath.Format("Q:\\language\\%s\\keyboardmap.xml", strLanguage.c_str());
    strKeyboardLayoutConfigurationPath = _P(strKeyboardLayoutConfigurationPath);
    CLog::Log(LOGINFO, "load keyboard layout configuration info file: %s", strKeyboardLayoutConfigurationPath.c_str());
    g_keyboardLayoutConfiguration.Load(strKeyboardLayoutConfigurationPath);

    CStdString strLanguagePath;
    strLanguagePath.Format("Q:\\language\\%s\\strings.xml", strLanguage.c_str());

    g_buttonTranslator.Load();
    g_localizeStrings.Load(_P(strLanguagePath));

    g_infoManager.ResetCache();

    if (m_iLastLoadedProfileIndex != 0)
    {
      TiXmlDocument doc;
      if (doc.LoadFile(GetUserDataFolder()+"\\guisettings.xml"))
        g_guiSettings.LoadMasterLock(doc.RootElement());
    }
    
    // New display setting.
    g_graphicsContext.SetVideoResolution(g_guiSettings.m_LookAndFeelResolution, TRUE);
    
    // always reload the skin - we need it for the new language strings
    g_application.LoadSkin(g_guiSettings.GetString("lookandfeel.skin"));

#ifdef HAS_XBOX_HARDWARE
    if (g_guiSettings.GetBool("system.autotemperature"))
    {
      CLog::Log(LOGNOTICE, "start fancontroller");
      CFanController::Instance()->Start(g_guiSettings.GetInt("system.targettemperature"), g_guiSettings.GetInt("system.minfanspeed"));
    }
    else if (g_guiSettings.GetBool("system.fanspeedcontrol"))
    {
      CLog::Log(LOGNOTICE, "setting fanspeed");
      CFanController::Instance()->SetFanSpeed(g_guiSettings.GetInt("system.fanspeed"));
    }
    g_application.StartLEDControl(false);
#endif

    // to set labels - shares are reloaded
    CDetectDVDMedia::UpdateState();
    // init windows
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_WINDOW_RESET);
    m_gWindowManager.SendMessage(msg);

    CUtil::DeleteMusicDatabaseDirectoryCache();
    CUtil::DeleteVideoDatabaseDirectoryCache();

    return true;
  }

  m_iLastLoadedProfileIndex = iOldIndex;

  return false;
}

bool CSettings::DeleteProfile(int index)
{
  if (index < 0 && index >= (int)g_settings.m_vecProfiles.size())
    return false;

  CGUIDialogYesNo* dlgYesNo = (CGUIDialogYesNo*)m_gWindowManager.GetWindow(WINDOW_DIALOG_YES_NO);
  if (dlgYesNo)
  {
    CStdString message;
    CStdString str = g_localizeStrings.Get(13201);
    message.Format(str.c_str(), g_settings.m_vecProfiles.at(index).getName());
    dlgYesNo->SetHeading(13200);
    dlgYesNo->SetLine(0, message);
    dlgYesNo->SetLine(1, "");
    dlgYesNo->SetLine(2, "");
    dlgYesNo->DoModal();

    if (dlgYesNo->IsConfirmed())
    {
      //delete profile
      CStdString strDirectory = g_settings.m_vecProfiles[index].getDirectory();
      m_vecProfiles.erase(g_settings.m_vecProfiles.begin()+index);
      if (index == g_settings.m_iLastLoadedProfileIndex)
      {
        g_settings.LoadProfile(0);
        g_settings.Save();
      }

      CFileItem item(g_settings.GetUserDataFolder()+"\\"+strDirectory);
      item.m_strPath = g_settings.GetUserDataFolder()+"\\"+strDirectory+"\\";
      item.m_bIsFolder = true;
      item.Select(true);
      CGUIWindowFileManager::DeleteItem(&item);
    }
    else
      return false;
  }

  SaveProfiles( PROFILES_FILE );

  return true;
}

bool CSettings::SaveSettingsToProfile(int index)
{
  /*CProfile& profile = m_vecProfiles.at(index);
  return SaveSettings(profile.getFileName(), false);*/
  return true;
}


bool CSettings::LoadProfiles(const CStdString& strSettingsFile)
{
  TiXmlDocument profilesDoc;
  if (!CFile::Exists(strSettingsFile))
  { // set defaults, or assume no rss feeds??
    return false;
  }
  if (!profilesDoc.LoadFile(strSettingsFile.c_str()))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d\n%s", strSettingsFile.c_str(), profilesDoc.ErrorRow(), profilesDoc.ErrorDesc());
    return false;
  }

  TiXmlElement *pRootElement = profilesDoc.RootElement();
  if (!pRootElement || strcmpi(pRootElement->Value(),"profiles") != 0)
  {
    CLog::Log(LOGERROR, "Error loading %s, no <profiles> node", strSettingsFile.c_str());
    return false;
  }
  GetInteger(pRootElement,"lastloaded",m_iLastLoadedProfileIndex,0,0,1000);
  if (m_iLastLoadedProfileIndex < 0)
    m_iLastLoadedProfileIndex = 0;

  XMLUtils::GetBoolean(pRootElement,"useloginscreen",bUseLoginScreen);

  TiXmlElement* pProfile = pRootElement->FirstChildElement("profile");
  CProfile profile;

  while (pProfile)
  {
    profile.setName("Master user");
    if (CDirectory::Exists(_P("u:\\userdata")))
      profile.setDirectory("u:\\userdata");
    else
      profile.setDirectory("q:\\userdata");

    CStdString strName;
    XMLUtils::GetString(pProfile,"name",strName);
    profile.setName(strName);

    CStdString strDirectory;
    XMLUtils::GetString(pProfile,"directory",strDirectory);
    profile.setDirectory(strDirectory);

    CStdString strThumb;
    XMLUtils::GetString(pProfile,"thumbnail",strThumb);
    profile.setThumb(strThumb);

    bool bHas=true;
    XMLUtils::GetBoolean(pProfile, "hasdatabases", bHas);
    profile.setDatabases(bHas);

    bHas = true;
    XMLUtils::GetBoolean(pProfile, "canwritedatabases", bHas);
    profile.setWriteDatabases(bHas);

    bHas = true;
    XMLUtils::GetBoolean(pProfile, "hassources", bHas);
    profile.setSources(bHas);

    bHas = true;
    XMLUtils::GetBoolean(pProfile, "canwritesources", bHas);
    profile.setWriteSources(bHas);

    bHas = false;
    XMLUtils::GetBoolean(pProfile, "locksettings", bHas);
    profile.setSettingsLocked(bHas);

    bHas = false;
    XMLUtils::GetBoolean(pProfile, "lockfiles", bHas);
    profile.setFilesLocked(bHas);

    bHas = false;
    XMLUtils::GetBoolean(pProfile, "lockmusic", bHas);
    profile.setMusicLocked(bHas);

    bHas = false;
    XMLUtils::GetBoolean(pProfile, "lockvideo", bHas);
    profile.setVideoLocked(bHas);

    bHas = false;
    XMLUtils::GetBoolean(pProfile, "lockpictures", bHas);
    profile.setPicturesLocked(bHas);

    bHas = false;
    XMLUtils::GetBoolean(pProfile, "lockprograms", bHas);
    profile.setProgramsLocked(bHas);

    bHas = false;
    LockType iLockMode=LOCK_MODE_EVERYONE;
    XMLUtils::GetInt(pProfile,"lockmode",(int&)iLockMode);
    if (iLockMode > LOCK_MODE_QWERTY || iLockMode < LOCK_MODE_EVERYONE)
      iLockMode = LOCK_MODE_EVERYONE;
    profile.setLockMode(iLockMode);

    CStdString strLockCode;
    XMLUtils::GetString(pProfile,"lockcode",strLockCode);
    profile.setLockCode(strLockCode);

    CStdString strDate;
    XMLUtils::GetString(pProfile,"lastdate",strDate);
    profile.setDate(strDate);

    m_vecProfiles.push_back(profile);
    pProfile = pProfile->NextSiblingElement("profile");
  }

  if (m_iLastLoadedProfileIndex >= (int)m_vecProfiles.size() || m_iLastLoadedProfileIndex < 0)
    m_iLastLoadedProfileIndex = 0;

  return true;
}

bool CSettings::SaveProfiles(const CStdString& strSettingsFile) const
{
  TiXmlDocument xmlDoc;
  TiXmlElement xmlRootElement("profiles");
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;
  SetInteger(pRoot,"lastloaded",m_iLastLoadedProfileIndex);
  SetBoolean(pRoot,"useloginscreen",bUseLoginScreen);
  for (unsigned int iProfile=0;iProfile<g_settings.m_vecProfiles.size();++iProfile)
  {
    TiXmlElement profileNode("profile");
    TiXmlNode *pNode = pRoot->InsertEndChild(profileNode);
    SetString(pNode,"name",g_settings.m_vecProfiles[iProfile].getName());
    SetString(pNode,"directory",g_settings.m_vecProfiles[iProfile].getDirectory());
    SetString(pNode,"thumbnail",g_settings.m_vecProfiles[iProfile].getThumb());
    SetString(pNode,"lastdate",g_settings.m_vecProfiles[iProfile].getDate());

    if (g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE)
    {
      SetInteger(pNode,"lockmode",g_settings.m_vecProfiles[iProfile].getLockMode());
      SetString(pNode,"lockcode",g_settings.m_vecProfiles[iProfile].getLockCode());
      SetBoolean(pNode,"lockmusic",g_settings.m_vecProfiles[iProfile].musicLocked());
      SetBoolean(pNode,"lockvideo",g_settings.m_vecProfiles[iProfile].videoLocked());
      SetBoolean(pNode,"lockpictures",g_settings.m_vecProfiles[iProfile].picturesLocked());
      SetBoolean(pNode,"lockprograms",g_settings.m_vecProfiles[iProfile].programsLocked());
      SetBoolean(pNode,"locksettings",g_settings.m_vecProfiles[iProfile].settingsLocked());
      SetBoolean(pNode,"lockfiles",g_settings.m_vecProfiles[iProfile].filesLocked());
    }

    if (iProfile > 0)
    {
      SetBoolean(pNode,"hasdatabases",g_settings.m_vecProfiles[iProfile].hasDatabases());
      SetBoolean(pNode,"canwritedatabases",g_settings.m_vecProfiles[iProfile].canWriteDatabases());
      SetBoolean(pNode,"hassources",g_settings.m_vecProfiles[iProfile].hasSources());
      SetBoolean(pNode,"canwritesources",g_settings.m_vecProfiles[iProfile].canWriteSources());
    }
  }
  // save the file
  return xmlDoc.SaveFile(_P(strSettingsFile));
}

bool CSettings::LoadUPnPXml(const CStdString& strSettingsFile)
{
  TiXmlDocument UPnPDoc;

  if (!CFile::Exists(strSettingsFile))
  { // set defaults, or assume no rss feeds??
    return false;
  }
  if (!UPnPDoc.LoadFile(strSettingsFile.c_str()))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d\n%s", strSettingsFile.c_str(), UPnPDoc.ErrorRow(), UPnPDoc.ErrorDesc());
    return false;
  }

  TiXmlElement *pRootElement = UPnPDoc.RootElement();
  if (!pRootElement || strcmpi(pRootElement->Value(),"upnpserver") != 0)
  {
    CLog::Log(LOGERROR, "Error loading %s, no <upnpserver> node", strSettingsFile.c_str());
    return false;
  }
  // load settings

  // default values for ports
  g_settings.m_UPnPPortServer = 0;
  g_settings.m_UPnPPortRenderer = 0;
  g_settings.m_UPnPMaxReturnedItems = 0;
  
  XMLUtils::GetString(pRootElement, "UUID", g_settings.m_UPnPUUIDServer);
  XMLUtils::GetInt(pRootElement, "Port", g_settings.m_UPnPPortServer);
  XMLUtils::GetInt(pRootElement, "MaxReturnedItems", g_settings.m_UPnPMaxReturnedItems);
  XMLUtils::GetString(pRootElement, "UUIDRenderer", g_settings.m_UPnPUUIDRenderer);
  XMLUtils::GetInt(pRootElement, "PortRenderer", g_settings.m_UPnPPortRenderer);

  CStdString strDefault;
  GetSources(pRootElement,"music",g_settings.m_UPnPMusicSources,strDefault);
  GetSources(pRootElement,"video",g_settings.m_UPnPVideoSources,strDefault);
  GetSources(pRootElement,"pictures",g_settings.m_UPnPPictureSources,strDefault);

  return true;
}

bool CSettings::SaveUPnPXml(const CStdString& strSettingsFile) const
{
  TiXmlDocument xmlDoc;
  TiXmlElement xmlRootElement("upnpserver");
  TiXmlNode *pRoot = xmlDoc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;

  // create a new Element for UUID
  XMLUtils::SetString(pRoot, "UUID", g_settings.m_UPnPUUIDServer);
  XMLUtils::SetInt(pRoot, "Port", g_settings.m_UPnPPortServer);
  XMLUtils::SetInt(pRoot, "MaxReturnedItems", g_settings.m_UPnPMaxReturnedItems);
  XMLUtils::SetString(pRoot, "UUIDRenderer", g_settings.m_UPnPUUIDRenderer);
  XMLUtils::SetInt(pRoot, "PortRenderer", g_settings.m_UPnPPortRenderer);

  VECSOURCES* pShares[3];
  pShares[0] = &g_settings.m_UPnPMusicSources;
  pShares[1] = &g_settings.m_UPnPVideoSources;
  pShares[2] = &g_settings.m_UPnPPictureSources;
  for (int k=0;k<3;++k)
  {
    if ((*pShares)[k].size()==0)
      continue;

    TiXmlElement xmlType("");
    if (k==0)
      xmlType = TiXmlElement("music");
    if (k==1)
      xmlType = TiXmlElement("video");
    if (k==2)
      xmlType = TiXmlElement("pictures");

    TiXmlNode* pNode = pRoot->InsertEndChild(xmlType);

    for (unsigned int j=0;j<(*pShares)[k].size();++j)
    {
      // create a new Element
      TiXmlText xmlName((*pShares)[k][j].strName);
      TiXmlElement eName("name");
      eName.InsertEndChild(xmlName);

      TiXmlElement source("source");
      source.InsertEndChild(eName);

      for (unsigned int i = 0; i < (*pShares)[k][j].vecPaths.size(); i++)
      {
        TiXmlText xmlPath((*pShares)[k][j].vecPaths[i]);
        TiXmlElement ePath("path");
        ePath.InsertEndChild(xmlPath);
        source.InsertEndChild(ePath);
      }

      if (pNode)
        pNode->ToElement()->InsertEndChild(source);
    }
  }
  // save the file
  return xmlDoc.SaveFile(_P(strSettingsFile));
}

bool CSettings::UpdateShare(const CStdString &type, const CStdString oldName, const CMediaSource &share)
{
  VECSOURCES *pShares = GetSourcesFromType(type);

  if (!pShares) return false;

  // update our current share list
  CMediaSource* pShare=NULL;
  for (IVECSOURCES it = pShares->begin(); it != pShares->end(); it++)
  {
    if ((*it).strName == oldName)
    {
      (*it).strName = share.strName;
      (*it).strPath = share.strPath;
      (*it).vecPaths = share.vecPaths;
      pShare = &(*it);
      break;
    }
  }

  if (!pShare)
    return false;

  // Update our XML file as well
  return SaveSources();
}

// NOTE: This function does NOT save the sources.xml file - you need to call SaveSources() separately.
bool CSettings::UpdateSource(const CStdString &strType, const CStdString strOldName, const CStdString &strUpdateElement, const CStdString &strUpdateText)
{
  VECSOURCES *pShares = GetSourcesFromType(strType);

  if (!pShares) return false;

  // disallow virtual paths
  if (strUpdateElement.Equals("path") && CUtil::IsVirtualPath(strUpdateText))
    return false;

  for (IVECSOURCES it = pShares->begin(); it != pShares->end(); it++)
  {
    if ((*it).strName == strOldName)
    {
      if ("name" == strUpdateElement)
        (*it).strName = strUpdateText;
      else if ("lockmode" == strUpdateElement)
        (*it).m_iLockMode = LockType(atoi(strUpdateText));
      else if ("lockcode" == strUpdateElement)
        (*it).m_strLockCode = strUpdateText;
      else if ("badpwdcount" == strUpdateElement)
        (*it).m_iBadPwdCount = atoi(strUpdateText);
      else if ("thumbnail" == strUpdateElement)
        (*it).m_strThumbnailImage = strUpdateText;
      else if ("path" == strUpdateElement)
      {
        (*it).vecPaths.clear();
        (*it).strPath = strUpdateText;
        (*it).vecPaths.push_back(strUpdateText);
      }
      else
        return false;
      return true;
    }
  }
  return false;
}

bool CSettings::DeleteSource(const CStdString &strType, const CStdString strName, const CStdString strPath)
{
  VECSOURCES *pShares = GetSourcesFromType(strType);
  if (!pShares) return false;

  bool found(false);

  for (IVECSOURCES it = pShares->begin(); it != pShares->end(); it++)
  {
    if ((*it).strName == strName && (*it).strPath == strPath)
    {
      CLog::Log(LOGDEBUG,"found share, removing!");
      pShares->erase(it);
      found = true;
      break;
    }
  }

  if (strType.Find("upnp") > -1)
    return found;

  return SaveSources();
}

bool CSettings::AddShare(const CStdString &type, const CMediaSource &share)
{
  VECSOURCES *pShares = GetSourcesFromType(type);
  if (!pShares) return false;

  // translate dir and add to our current shares
  CStdString strPath1 = share.strPath;
  strPath1.ToUpper();
  if(strPath1.IsEmpty())
  {
    CLog::Log(LOGERROR, "unable to add empty path");
    return false;
  }

  CMediaSource shareToAdd = share;
  if (strPath1.at(0) == '$')
  {
    shareToAdd.strPath = CUtil::TranslateSpecialSource(strPath1);
    if (!share.strPath.IsEmpty())
      CLog::Log(LOGDEBUG, "%s Translated (%s) to Path (%s)",__FUNCTION__ ,strPath1.c_str(),shareToAdd.strPath.c_str());
    else
    {
      CLog::Log(LOGDEBUG, "%s Skipping invalid special directory token: %s",__FUNCTION__,strPath1.c_str());
      return false;
    }
  }
  pShares->push_back(shareToAdd);

  if (type.Find("upnp") < 0)
  {
    return SaveSources();
  }
  return true;
}

bool CSettings::SaveSources()
{
  // TODO: Should we be specifying utf8 here??
  TiXmlDocument doc;
  TiXmlElement xmlRootElement("sources");
  TiXmlNode *pRoot = doc.InsertEndChild(xmlRootElement);
  if (!pRoot) return false;

  // ok, now run through and save each sources section
  SetSources(pRoot, "programs", g_settings.m_programSources, g_settings.m_defaultProgramSource);
  SetSources(pRoot, "video", g_settings.m_videoSources, g_settings.m_defaultVideoSource);
  SetSources(pRoot, "music", g_settings.m_musicSources, g_settings.m_defaultMusicSource);
  SetSources(pRoot, "pictures", g_settings.m_pictureSources, g_settings.m_defaultPictureSource);
  SetSources(pRoot, "files", g_settings.m_fileSources, g_settings.m_defaultFileSource);

  return doc.SaveFile(g_settings.GetSourcesFile());
}

bool CSettings::SetSources(TiXmlNode *root, const char *section, const VECSOURCES &shares, const char *defaultPath)
{
  TiXmlElement sectionElement(section);
  TiXmlNode *sectionNode = root->InsertEndChild(sectionElement);
  if (sectionNode)
  {
    SetString(sectionNode, "default", defaultPath);
    for (unsigned int i = 0; i < shares.size(); i++)
    {
      const CMediaSource &share = shares[i];
      if (share.m_ignore)
        continue;
      TiXmlElement source("source");

      SetString(&source, "name", share.strName);

      for (unsigned int i = 0; i < share.vecPaths.size(); i++)
        SetString(&source, "path", share.vecPaths[i]);

      if (share.m_iHasLock)
      {
        SetInteger(&source, "lockmode", share.m_iLockMode);
        SetString(&source, "lockcode", share.m_strLockCode);
        SetInteger(&source, "badpwdcount", share.m_iBadPwdCount);
      }
      if (!share.m_strThumbnailImage.IsEmpty())
        SetString(&source, "thumbnail", share.m_strThumbnailImage);

      sectionNode->InsertEndChild(source);
    }
  }
  return true;
}

void CSettings::LoadSkinSettings(const TiXmlElement* pRootElement)
{
  int number = 0;
  const TiXmlElement *pElement = pRootElement->FirstChildElement("skinsettings");
  if (pElement)
  {
    m_skinStrings.clear();
    m_skinBools.clear();
    
    const TiXmlElement *pChild = pElement->FirstChildElement("setting");
    while (pChild)
    {
      CStdString settingName = pChild->Attribute("name");
      if (pChild->Attribute("type") && strcmpi(pChild->Attribute("type"),"string") == 0)
      { // string setting
        CSkinString string;
        string.name = settingName;
        string.value = pChild->FirstChild() ? pChild->FirstChild()->Value() : "";
        m_skinStrings.insert(pair<int, CSkinString>(number++, string));
      }
      else
      { // bool setting
        CSkinBool setting;
        setting.name = settingName;
        setting.value = pChild->FirstChild() ? strcmpi(pChild->FirstChild()->Value(), "true") == 0 : false;
        m_skinBools.insert(pair<int, CSkinBool>(number++, setting));
      }
      pChild = pChild->NextSiblingElement("setting");
    }
    
    if (m_skinBools.size() == 0)
    {
      // Hardwired defaults for MediaStream. Bogus, but works for now and better than editing XML every week.
      m_skinBools.insert(pair<int, CSkinBool>(number++, CSkinBool("MediaStream.enablecustombghome", true)));
      //m_skinBools.insert(pair<int, CSkinBool>(number++, CSkinBool("MediaStream.hideprograms", true)));
      m_skinBools.insert(pair<int, CSkinBool>(number++, CSkinBool("MediaStream.hideweather", true)));
      m_skinBools.insert(pair<int, CSkinBool>(number++, CSkinBool("MediaStream.hidedvd", true)));
      m_skinBools.insert(pair<int, CSkinBool>(number++, CSkinBool("MediaStream.showweatheronhome", true)));
      m_skinBools.insert(pair<int, CSkinBool>(number++, CSkinBool("MediaStream.showmediacount", true)));
    }
  }
}

void CSettings::SaveSkinSettings(TiXmlNode *pRootElement) const
{
  // add the <skinsettings> tag
  TiXmlElement xmlSettingsElement("skinsettings");
  TiXmlNode *pSettingsNode = pRootElement->InsertEndChild(xmlSettingsElement);
  if (!pSettingsNode) return;
  for (std::map<int, CSkinBool>::const_iterator it = m_skinBools.begin(); it != m_skinBools.end(); ++it)
  {
    // Add a <setting type="bool" name="name">true/false</setting>
    TiXmlElement xmlSetting("setting");
    xmlSetting.SetAttribute("type", "bool");
    xmlSetting.SetAttribute("name", (*it).second.name.c_str());
    TiXmlText xmlBool((*it).second.value ? "true" : "false");
    xmlSetting.InsertEndChild(xmlBool);
    pSettingsNode->InsertEndChild(xmlSetting);
  }
  for (std::map<int, CSkinString>::const_iterator it = m_skinStrings.begin(); it != m_skinStrings.end(); ++it)
  {
    // Add a <setting type="string" name="name">string</setting>
    TiXmlElement xmlSetting("setting");
    xmlSetting.SetAttribute("type", "string");
    xmlSetting.SetAttribute("name", (*it).second.name.c_str());
    TiXmlText xmlLabel((*it).second.value);
    xmlSetting.InsertEndChild(xmlLabel);
    pSettingsNode->InsertEndChild(xmlSetting);
  }
}

void CSettings::Clear()
{
  m_programSources.clear();
  m_pictureSources.clear();
  m_fileSources.clear();
  m_musicSources.clear();
  m_videoSources.clear();
//  m_vecIcons.clear();
  m_vecProfiles.clear();
  m_szMyVideoStackTokensArray.clear();
  m_szMyVideoCleanTokensArray.clear();
  g_advancedSettings.m_videoStackRegExps.clear();
  m_mapRssUrls.clear();
  m_skinBools.clear();
  m_skinStrings.clear();
}

int CSettings::TranslateSkinString(const CStdString &setting)
{
  CStdString settingName;
  settingName.Format("%s.%s", g_guiSettings.GetString("lookandfeel.skin").c_str(), setting);
  // run through and see if we have this setting
  for (std::map<int, CSkinString>::const_iterator it = m_skinStrings.begin(); it != m_skinStrings.end(); it++)
  {
    if (settingName.Equals((*it).second.name))
      return (*it).first;
  }
  // didn't find it - insert it
  CSkinString skinString;
  skinString.name = settingName;
  m_skinStrings.insert(pair<int, CSkinString>(m_skinStrings.size() + m_skinBools.size(), skinString));
  return m_skinStrings.size() + m_skinBools.size() - 1;
}

const CStdString &CSettings::GetSkinString(int setting) const
{
  std::map<int, CSkinString>::const_iterator it = m_skinStrings.find(setting);
  if (it != m_skinStrings.end())
  {
    return (*it).second.value;
  }
  return StringUtils::EmptyString;
}

void CSettings::SetSkinString(int setting, const CStdString &label)
{
  std::map<int, CSkinString>::iterator it = m_skinStrings.find(setting);
  if (it != m_skinStrings.end())
  {
    (*it).second.value = label;
    return;
  }
  assert(false);
  CLog::Log(LOGFATAL, "%s : Unknown setting requested", __FUNCTION__);
}

void CSettings::ResetSkinSetting(const CStdString &setting)
{
  CStdString settingName;
  settingName.Format("%s.%s", g_guiSettings.GetString("lookandfeel.skin").c_str(), setting);
  // run through and see if we have this setting as a string
  for (std::map<int, CSkinString>::iterator it = m_skinStrings.begin(); it != m_skinStrings.end(); it++)
  {
    if (settingName.Equals((*it).second.name))
    {
      (*it).second.value = "";
      return;
    }
  }
  // and now check for the skin bool
  for (std::map<int, CSkinBool>::iterator it = m_skinBools.begin(); it != m_skinBools.end(); it++)
  {
    if (settingName.Equals((*it).second.name))
    {
      (*it).second.value = false;
      return;
    }
  }
}

int CSettings::TranslateSkinBool(const CStdString &setting)
{
  CStdString settingName;
  settingName.Format("%s.%s", g_guiSettings.GetString("lookandfeel.skin").c_str(), setting);
  // run through and see if we have this setting
  for (std::map<int, CSkinBool>::const_iterator it = m_skinBools.begin(); it != m_skinBools.end(); it++)
  {
    if (settingName.Equals((*it).second.name))
      return (*it).first;
  }
  // didn't find it - insert it
  CSkinBool skinBool;
  skinBool.name = settingName;
  skinBool.value = false;
  m_skinBools.insert(pair<int, CSkinBool>(m_skinBools.size() + m_skinStrings.size(), skinBool));
  return m_skinBools.size() + m_skinStrings.size() - 1;
}

bool CSettings::GetSkinBool(int setting) const
{
  std::map<int, CSkinBool>::const_iterator it = m_skinBools.find(setting);
  if (it != m_skinBools.end())
  {
    return (*it).second.value;
  }
  // default is to return false
  return false;
}

void CSettings::SetSkinBool(int setting, bool set)
{
  std::map<int, CSkinBool>::iterator it = m_skinBools.find(setting);
  if (it != m_skinBools.end())
  {
    (*it).second.value = set;
    return;
  }
  assert(false);
  CLog::Log(LOGFATAL,"%s : Unknown setting requested", __FUNCTION__);
}

void CSettings::ResetSkinSettings()
{
  CStdString currentSkin = g_guiSettings.GetString("lookandfeel.skin") + ".";
  // clear all the settings and strings from this skin.
  std::map<int, CSkinBool>::iterator it = m_skinBools.begin();
  while (it != m_skinBools.end())
  {
    CStdString skinName = (*it).second.name;
    if (skinName.Left(currentSkin.size()) == currentSkin)
      (*it).second.value = false;

    it++;
  }
  std::map<int, CSkinString>::iterator it2 = m_skinStrings.begin();
  while (it2 != m_skinStrings.end())
  {
    CStdString skinName = (*it2).second.name;
    if (skinName.Left(currentSkin.size()) == currentSkin)
      (*it2).second.value = "";

    it2++;
  }
  g_infoManager.ResetCache();
}

void CSettings::LoadUserFolderLayout()
{
  // check them all
  CStdString strDir = g_guiSettings.GetString("system.playlistspath");
  if (strDir == "set default")
  {
    strDir = "P:\\playlists\\";
    g_guiSettings.SetString("system.playlistspath",strDir.c_str());
  }
  CDirectory::Create(strDir);
  CStdString strDir2;
  CUtil::AddFileToFolder(strDir,"music",strDir2);
  CDirectory::Create(strDir2);
  CUtil::AddFileToFolder(strDir,"video",strDir2);
  CDirectory::Create(strDir2);
  CUtil::AddFileToFolder(strDir,"mixed",strDir2);
  CDirectory::Create(strDir2);
}

CStdString CSettings::GetProfileUserDataFolder() const
{
  CStdString folder;
  if (m_iLastLoadedProfileIndex == 0)
    return GetUserDataFolder();

  CUtil::AddFileToFolder(GetUserDataFolder(),m_vecProfiles[m_iLastLoadedProfileIndex].getDirectory(),folder);

  return _P(folder);
}

CStdString CSettings::GetUserDataItem(const CStdString& strFile) const
{
  CStdString folder;
  folder = "P:\\"+strFile;
  if (!CFile::Exists(folder))
    folder = "T:\\"+strFile;
  return _P(folder);
}

CStdString CSettings::GetUserDataFolder() const
{
  return _P(m_vecProfiles[0].getDirectory());
}

CStdString CSettings::GetDatabaseFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "Database", folder);
  else
    CUtil::AddFileToFolder(GetUserDataFolder(), "Database", folder);

  return folder;
}

CStdString CSettings::GetCDDBFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), _P("Database\\CDDB"), folder);
  else
    CUtil::AddFileToFolder(GetUserDataFolder(), _P("Database\\CDDB"), folder);

  return folder;
}

CStdString CSettings::GetThumbnailsFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "Thumbnails", folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "Thumbnails", folder);

  return folder;
}

CStdString CSettings::GetMusicThumbFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), _P("Thumbnails\\Music"), folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), _P("Thumbnails\\Music"), folder);

  return folder;
}

CStdString CSettings::GetLastFMThumbFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), _P("Thumbnails\\Music\\LastFM"), folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), _P("Thumbnails\\Music\\LastFM"), folder);

  return folder;
}

CStdString CSettings::GetMusicArtistThumbFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), _P("Thumbnails\\Music\\Artists"), folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), _P("Thumbnails\\Music\\Artists"), folder);

  return folder;
}

CStdString CSettings::GetVideoThumbFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), _P("Thumbnails\\Video"), folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), _P("Thumbnails\\Video"), folder);

  return folder;
}

CStdString CSettings::GetProgramFanartFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "Thumbnails\\Programs\\Fanart", folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "Thumbnails\\Programs\\Fanart", folder);

  return folder;
}

CStdString CSettings::GetVideoFanartFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "Thumbnails\\Video\\Fanart", folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "Thumbnails\\Video\\Fanart", folder);

  return folder;
}

CStdString CSettings::GetBookmarksThumbFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), _P("Thumbnails\\Video\\Bookmarks"), folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), _P("Thumbnails\\Video\\Bookmarks"), folder);

  return folder;
}

CStdString CSettings::GetPicturesThumbFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), _P("Thumbnails\\Pictures"), folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), _P("Thumbnails\\Pictures"), folder);

  return folder;
}

CStdString CSettings::GetProgramsThumbFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), _P("Thumbnails\\Programs"), folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), _P("Thumbnails\\Programs"), folder);

  return folder;
}

CStdString CSettings::GetGameSaveThumbFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), _P("Thumbnails\\GameSaves"), folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), _P("Thumbnails\\GameSaves"), folder);

  return folder;
}

CStdString CSettings::GetProfilesThumbFolder() const
{
  CStdString folder;
  CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), _P("Thumbnails\\Profiles"), folder);

  return folder;
}

CStdString CSettings::GetPlexMediaServerThumbFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), _P("Thumbnails\\PlexMediaServer"), folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), _P("Thumbnails\\PlexMediaServer"), folder);

  return folder;

}

CStdString CSettings::GetPlexMediaServerFanartFolder() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), "Thumbnails\\PlexMediaServer\\Fanart", folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), "Thumbnails\\PlexMediaServer\\Fanart", folder);

  return folder;
}

CStdString CSettings::GetXLinkKaiThumbFolder() const
{
  CStdString folder;
#ifdef HAS_KAI
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasDatabases())
    CUtil::AddFileToFolder(g_settings.GetProfileUserDataFolder(), _P("Thumbnails\\Programs\\XLinkKai"), folder);
  else
    CUtil::AddFileToFolder(g_settings.GetUserDataFolder(), _P("Thumbnails\\Programs\\XlinkKai"), folder);
#endif
  return folder;
}

CStdString CSettings::GetSourcesFile() const
{
  CStdString folder;
  if (m_vecProfiles[m_iLastLoadedProfileIndex].hasSources())
    CUtil::AddFileToFolder(GetProfileUserDataFolder(),"sources.xml",folder);
  else
    CUtil::AddFileToFolder(GetUserDataFolder(),"sources.xml",folder);

  return folder;
}

CStdString CSettings::GetSkinFolder() const
{
  CStdString folder;

  // Get the Current Skin Path
  return GetSkinFolder(g_guiSettings.GetString("lookandfeel.skin"));
}

CStdString CSettings::GetScriptsFolder() const
{
  CStdString folder = _P("U:\\scripts");

  if ( CDirectory::Exists(folder) )
    return folder;

  folder = "Q:\\scripts";
  return _P(folder);
}

CStdString CSettings::GetSkinFolder(const CStdString &skinName) const
{
  CStdString folder;

  // Get the Current Skin Path
  CUtil::AddFileToFolder("U:\\skin\\", skinName, folder);
  if ( ! CDirectory::Exists(_P(folder)) )
    CUtil::AddFileToFolder("Q:\\skin\\", skinName, folder);

  return _P(folder);
}

void CSettings::LoadRSSFeeds()
{
  CStdString rssXML;
  //rssXML.Format("%s\\RSSFeeds.xml", GetUserDataFolder().c_str());
  rssXML = GetUserDataItem("RssFeeds.xml");
  TiXmlDocument rssDoc;
  if (!CFile::Exists(rssXML))
  { // set defaults, or assume no rss feeds??
    return;
  }
  if (!rssDoc.LoadFile(rssXML.c_str()))
  {
    CLog::Log(LOGERROR, "Error loading %s, Line %d\n%s", rssXML.c_str(), rssDoc.ErrorRow(), rssDoc.ErrorDesc());
    return;
  }

  TiXmlElement *pRootElement = rssDoc.RootElement();
  if (!pRootElement || strcmpi(pRootElement->Value(),"rssfeeds") != 0)
  {
    CLog::Log(LOGERROR, "Error loading %s, no <rssfeeds> node", rssXML.c_str());
    return;
  }

  g_settings.m_mapRssUrls.clear();
  TiXmlElement* pSet = pRootElement->FirstChildElement("set");
  while (pSet)
  {
    int iId;
    if (pSet->QueryIntAttribute("id", &iId) == TIXML_SUCCESS)
    {
      std::vector<string> vecSet;
      std::vector<int> vecIntervals;
      TiXmlElement* pFeed = pSet->FirstChildElement("feed");
      while (pFeed)
      {
        int iInterval;
        if ( pFeed->QueryIntAttribute("updateinterval",&iInterval) != TIXML_SUCCESS)
        {
          iInterval=30; // default to 30 min
          CLog::Log(LOGDEBUG,"no interval set, default to 30!");
        }
        if (pFeed->FirstChild())
        {
          // TODO: UTF-8: Do these URLs need to be converted to UTF-8?
          //              What about the xml encoding?
          CStdString strUrl = pFeed->FirstChild()->Value();
          vecSet.push_back(strUrl);
          vecIntervals.push_back(iInterval);
        }
        pFeed = pFeed->NextSiblingElement("feed");
      }
      g_settings.m_mapRssUrls.insert(std::make_pair<int,std::pair<std::vector<int>,std::vector<string> > >(iId,std::make_pair<std::vector<int>,std::vector<string> >(vecIntervals,vecSet)));
    }
    else
      CLog::Log(LOGERROR,"found rss url set with no id in RssFeeds.xml, ignored");

    pSet = pSet->NextSiblingElement("set");
  }
}

CStdString CSettings::GetSettingsFile() const
{
  CStdString settings;
  if (g_settings.m_iLastLoadedProfileIndex == 0)
    settings = "T:\\guisettings.xml";
  else
    settings = "P:\\guisettings.xml";
  return _P(settings);
}

void CSettings::CreateProfileFolders()
{
  CreateDirectory(GetDatabaseFolder(), NULL);
  CreateDirectory(GetCDDBFolder().c_str(), NULL);

  // Thumbnails/
  CreateDirectory(GetThumbnailsFolder().c_str(), NULL);
  CreateDirectory(GetMusicThumbFolder().c_str(), NULL);
  CreateDirectory(GetMusicArtistThumbFolder().c_str(), NULL);
  CreateDirectory(GetLastFMThumbFolder().c_str(), NULL);
  CreateDirectory(GetVideoThumbFolder().c_str(), NULL);
  CreateDirectory(GetVideoFanartFolder().c_str(), NULL);
  CreateDirectory(GetBookmarksThumbFolder().c_str(), NULL);
  CreateDirectory(GetProgramsThumbFolder().c_str(), NULL);
  CreateDirectory(GetProgramFanartFolder().c_str(), NULL);
  CreateDirectory(GetPicturesThumbFolder().c_str(), NULL);
  
  CreateDirectory(GetPlexMediaServerThumbFolder().c_str(), NULL);
  CreateDirectory(GetPlexMediaServerFanartFolder().c_str(), NULL);
  
  CLog::Log(LOGINFO, "  thumbnails folder:%s", GetThumbnailsFolder().c_str());
  for (unsigned int hex=0; hex < 16; hex++)
  {
    CStdString strHex;
    strHex.Format("%x",hex);
    CStdString strThumbLoc;
    CUtil::AddFileToFolder(GetPicturesThumbFolder(), strHex, strThumbLoc);
    CreateDirectory(strThumbLoc.c_str(),NULL);
    CUtil::AddFileToFolder(GetMusicThumbFolder(), strHex, strThumbLoc);
    CreateDirectory(strThumbLoc.c_str(),NULL);
    CUtil::AddFileToFolder(GetVideoThumbFolder(), strHex, strThumbLoc);
    CreateDirectory(strThumbLoc.c_str(),NULL);
    CUtil::AddFileToFolder(GetPlexMediaServerThumbFolder(), strHex, strThumbLoc);
    CreateDirectory(strThumbLoc.c_str(), NULL);
    CUtil::AddFileToFolder(GetProgramsThumbFolder(), strHex, strThumbLoc);
    CreateDirectory(strThumbLoc.c_str(), NULL);
  }
}

string CSettings::GetLanguage()
{
  string ret = "English";
  
#ifdef __APPLE__
  
  // Get the system language.
  string systemLang = Cocoa_GetLanguage();
  
  if (m_languageMap.find(systemLang) != m_languageMap.end())
  {
    // Great match.
    ret = m_languageMap[systemLang];
  }
  else
  {
    // See if we can just get the language.
    int dash = systemLang.find("-");
    if (dash != -1)
    {
      systemLang = systemLang.substr(0, dash);
      if (m_languageMap.find(systemLang) != m_languageMap.end())
        ret = m_languageMap[systemLang];
    }
  }
  
#endif
  
  return ret;
}
