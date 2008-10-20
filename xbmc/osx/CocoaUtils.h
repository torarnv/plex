//
//  Utils.h
//  XBMC
//
//  Created by Elan Feingold on 1/5/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#ifndef _OSX_UTILS_H_
#define _OSX_UTILS_H_

#include "AppleRemoteKeys.h"

#ifdef __cplusplus
extern "C"
{
#endif
  //
  // Initialization.
  //
  void Cocoa_Initialize(void* pApplication);
  void InstallCrashReporter();
  void Cocoa_DisplayError(const char* error);

  //
  // Pools.
  //
  void* InitializeAutoReleasePool();
  void DestroyAutoReleasePool(void* pool);

  //
  // Graphics.
  //
  void Cocoa_GetScreenResolution(int* w, int* h);
  double Cocoa_GetScreenRefreshRate(int screen);
  void Cocoa_GetScreenResolutionOfAnotherScreen(int display, int* w, int* h);
  int  Cocoa_GetNumDisplays();
  int  Cocoa_GetDisplay(int screen);
  int Cocoa_GetCurrentDisplay();
  void  Cocoa_MoveWindowToDisplay(int display);
  
  //
  // Open GL.
  //
  void  Cocoa_GL_MakeCurrentContext(void* theContext);
  void  Cocoa_GL_ReleaseContext(void* context);
  void  Cocoa_GL_SwapBuffers(void* theContext);
  void* Cocoa_GL_GetWindowPixelFormat();
  void* Cocoa_GL_GetFullScreenPixelFormat(int screen);
  void* Cocoa_GL_GetCurrentContext();
  void* Cocoa_GL_CreateContext(void* pixFmt, void* shareCtx);
  void* Cocoa_GL_ResizeWindow(void *theContext, int w, int h);
  void  Cocoa_GL_SetFullScreen(int screen, int width, int height, bool fs, bool blankOtherDisplay, bool fakeFullScreen);
  void  Cocoa_GL_EnableVSync(bool enable);


  //
  // Blanking.
  //
  void Cocoa_GL_UnblankOtherDisplays(int screen);
  void Cocoa_GL_BlankOtherDisplays(int screen);

  //
  // SDL Hack
  //
  void* Cocoa_GL_ReplaceSDLWindowContext();

  //
  // Power and Screen
  //
  int Cocoa_DimDisplayNow();
  void Cocoa_UpdateSystemActivity();
  int Cocoa_SleepSystem();
  void Cocoa_TurnOffScreenSaver();

  //
  // Mouse.
  //
  void Cocoa_HideMouse();

  //
  // Smart folders.
  //
  void Cocoa_GetSmartFolderResults(const char* strFile, void (*)(void* userData, void* userData2, const char* path), void* userData, void* userData2);

  //
  // Version.
  //
  const char* Cocoa_GetAppVersion();
  
  //
  // Get display port.
  //
  void* Cocoa_GetDisplayPort();
  
  void  Cocoa_MakeChildWindow();
  void  Cocoa_DestroyChildWindow();
  
  //
  // Get/set LCD panel brightness
  //
  void Cocoa_GetPanelBrightness(float* brightness);
  void Cocoa_SetPanelBrightness(float brightness);
  
  //
  // Apple hardware info
  //
  const char* Cocoa_HW_ModelName();
  const char* Cocoa_HW_LongModelName();
  bool Cocoa_HW_HasBattery();
  bool Cocoa_HW_IsOnACPower();
  bool Cocoa_HW_IsCharging();
  int  Cocoa_HW_CurrentBatteryCapacity();
  int  Cocoa_HW_TimeToBatteryEmpty();
  int  Cocoa_HW_TimeToFullCharge();
  void Cocoa_HW_SetBatteryWarningEnabled(bool enabled);
  void Cocoa_HW_SetBatteryTimeWarning(int timeWarning);
  void Cocoa_HW_SetBatteryCapacityWarning(int capacityWarning);

  //
  // Background music control functions
  //
  void Cocoa_StartBackgroundMusic();
  void Cocoa_StopBackgroundMusic();
  void Cocoa_SetBackgroundMusicEnabled(bool enabled);
  void Cocoa_SetBackgroundMusicThemesEnabled(bool enabled);
  void Cocoa_SetBackgroundMusicThemeDownloadsEnabled(bool enabled);
  void Cocoa_CheckForThemeNamed(const char* themeName);
  void Cocoa_SetBackgroundMusicThemeName(const char* themeName);
  void Cocoa_SetBackgroundMusicVolume(float volume);
  void Cocoa_UpdateGlobalVolume(int globalVolume);

  //
  // Sparkle software update
  //
  void Cocoa_CheckForUpdates();
  void Cocoa_CheckForUpdatesInBackground();
  void Cocoa_SetUpdateAlertType(int alertType);
  void Cocoa_SetUpdateSuspended(bool willSuspend);
  void Cocoa_SetUpdateCheckInterval(double seconds);
  void Cocoa_UpdateProgressDialog();

  // Display dimming.
  //
  void Cocoa_SetGammaRamp(unsigned short* pRed, unsigned short* pGreen, unsigned short* pBlue);

#ifdef __cplusplus
}
#endif

#endif
