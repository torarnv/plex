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

#include "GUIDialog.h"

class CMediaSource;

enum CONTEXT_BUTTON { CONTEXT_BUTTON_CANCELLED = 0,
                      CONTEXT_BUTTON_LAUNCH,
                      CONTEXT_BUTTON_LAUNCH_IN,
                      CONTEXT_BUTTON_GAMESAVES,
                      CONTEXT_BUTTON_RENAME,
                      CONTEXT_BUTTON_DELETE,
                      CONTEXT_BUTTON_COPY,
                      CONTEXT_BUTTON_MOVE,
                      CONTEXT_BUTTON_TRAINER_OPTIONS,
                      CONTEXT_BUTTON_SCAN_TRAINERS,
                      CONTEXT_BUTTON_ADD_FAVOURITE,
                      CONTEXT_BUTTON_SETTINGS,
                      CONTEXT_BUTTON_GOTO_ROOT,
                      CONTEXT_BUTTON_PLAY_DISC,
                      CONTEXT_BUTTON_RIP_CD,
                      CONTEXT_BUTTON_RIP_TRACK,
                      CONTEXT_BUTTON_EJECT_DISC,
                      CONTEXT_BUTTON_ADD_SOURCE,
                      CONTEXT_BUTTON_EDIT_SOURCE,
                      CONTEXT_BUTTON_REMOVE_SOURCE,
                      CONTEXT_BUTTON_SET_DEFAULT,
                      CONTEXT_BUTTON_CLEAR_DEFAULT,
                      CONTEXT_BUTTON_SET_THUMB,
                      CONTEXT_BUTTON_REMOVE_THUMB,
                      CONTEXT_BUTTON_ADD_LOCK,
                      CONTEXT_BUTTON_REMOVE_LOCK,
                      CONTEXT_BUTTON_CHANGE_LOCK,
                      CONTEXT_BUTTON_RESET_LOCK,
                      CONTEXT_BUTTON_REACTIVATE_LOCK,
                      CONTEXT_BUTTON_VIEW_SLIDESHOW,
                      CONTEXT_BUTTON_RECURSIVE_SLIDESHOW,
                      CONTEXT_BUTTON_REFRESH_THUMBS,
                      CONTEXT_BUTTON_SWITCH_MEDIA,
                      CONTEXT_BUTTON_MOVE_ITEM,
                      CONTEXT_BUTTON_MOVE_HERE,
                      CONTEXT_BUTTON_CANCEL_MOVE,
                      CONTEXT_BUTTON_MOVE_ITEM_UP,
                      CONTEXT_BUTTON_MOVE_ITEM_DOWN,
                      CONTEXT_BUTTON_SAVE,
                      CONTEXT_BUTTON_LOAD,
                      CONTEXT_BUTTON_CLEAR,
                      CONTEXT_BUTTON_QUEUE_ITEM,
                      CONTEXT_BUTTON_PLAY_ITEM,
                      CONTEXT_BUTTON_PLAY_WITH,
                      CONTEXT_BUTTON_PLAY_PARTYMODE,
                      CONTEXT_BUTTON_PLAY_PART,
                      CONTEXT_BUTTON_RESUME_ITEM,
                      CONTEXT_BUTTON_RESTART_ITEM,
                      CONTEXT_BUTTON_EDIT,
                      CONTEXT_BUTTON_EDIT_SMART_PLAYLIST,
                      CONTEXT_BUTTON_INFO,
                      CONTEXT_BUTTON_INFO_ALL,
                      CONTEXT_BUTTON_CDDB,
                      CONTEXT_BUTTON_UPDATE_LIBRARY,
                      CONTEXT_BUTTON_UPDATE_TVSHOW,
                      CONTEXT_BUTTON_SCAN,
                      CONTEXT_BUTTON_STOP_SCANNING,
                      CONTEXT_BUTTON_SET_ARTIST_THUMB,
                      CONTEXT_BUTTON_SET_SEASON_THUMB,
                      CONTEXT_BUTTON_NOW_PLAYING,
                      CONTEXT_BUTTON_PLAYLIST,
                      CONTEXT_BUTTON_SHUFFLE,
                      CONTEXT_BUTTON_CANCEL_PARTYMODE,
                      CONTEXT_BUTTON_MARK_WATCHED,
                      CONTEXT_BUTTON_MARK_UNWATCHED,
                      CONTEXT_BUTTON_SET_CONTENT,
                      CONTEXT_BUTTON_ADD_TO_LIBRARY,
                      CONTEXT_BUTTON_SONG_INFO,
                      CONTEXT_BUTTON_EDIT_PARTYMODE,
                      CONTEXT_BUTTON_LINK_MOVIE,
                      CONTEXT_BUTTON_UNLINK_MOVIE,
                      CONTEXT_BUTTON_GO_TO_ARTIST,
                      CONTEXT_BUTTON_GO_TO_ALBUM,
                      CONTEXT_BUTTON_PLAY_OTHER,
                      CONTEXT_BUTTON_SET_ACTOR_THUMB,
                      CONTEXT_BUTTON_SET_PLUGIN_THUMB,
                      CONTEXT_BUTTON_UNLINK_BOOKMARK,
                      CONTEXT_BUTTON_PLUGIN_SETTINGS,
                      CONTEXT_BUTTON_RATING,
                      CONTEXT_BUTTON_LASTFM_UNLOVE_ITEM,
                      CONTEXT_BUTTON_LASTFM_UNBAN_ITEM,
                      CONTEXT_BUTTON_USER1,
                      CONTEXT_BUTTON_USER2,
                      CONTEXT_BUTTON_USER3,
                      CONTEXT_BUTTON_USER4,
                      CONTEXT_BUTTON_USER5,
                      CONTEXT_BUTTON_USER6,
                      CONTEXT_BUTTON_USER7,
                      CONTEXT_BUTTON_USER8,
                      CONTEXT_BUTTON_USER9,
                      CONTEXT_BUTTON_USER10,
  
                      // Dynamic context buttons for plug-ins, etc.
                      CONTEXT_BUTTON_DYNAMIC1,
                      CONTEXT_BUTTON_DYNAMIC2,
                      CONTEXT_BUTTON_DYNAMIC3,
                      CONTEXT_BUTTON_DYNAMIC4,
                      CONTEXT_BUTTON_DYNAMIC5,
                      CONTEXT_BUTTON_DYNAMIC6,
                      CONTEXT_BUTTON_DYNAMIC7,
                      CONTEXT_BUTTON_DYNAMIC8,
                      CONTEXT_BUTTON_DYNAMIC9,
                      CONTEXT_BUTTON_DYNAMIC10,
                      CONTEXT_BUTTON_DYNAMIC11,
                      CONTEXT_BUTTON_DYNAMIC12,
                      CONTEXT_BUTTON_DYNAMIC13,
                      CONTEXT_BUTTON_DYNAMIC14,
                      CONTEXT_BUTTON_DYNAMIC15,
                      CONTEXT_BUTTON_DYNAMIC16,
                      CONTEXT_BUTTON_DYNAMIC17,
                      CONTEXT_BUTTON_DYNAMIC18,
                      CONTEXT_BUTTON_DYNAMIC19,
                      CONTEXT_BUTTON_DYNAMIC20,
                      CONTEXT_BUTTON_DYNAMIC21,
                      CONTEXT_BUTTON_DYNAMIC22,
                      CONTEXT_BUTTON_DYNAMIC23,
                      CONTEXT_BUTTON_DYNAMIC24,
                      CONTEXT_BUTTON_DYNAMIC25,
                      CONTEXT_BUTTON_DYNAMIC26,
                      CONTEXT_BUTTON_DYNAMIC27,
                      CONTEXT_BUTTON_DYNAMIC28,
                      CONTEXT_BUTTON_DYNAMIC29,
                      CONTEXT_BUTTON_DYNAMIC30,
                      CONTEXT_BUTTON_DYNAMIC31,
                      CONTEXT_BUTTON_DYNAMIC32,
                      CONTEXT_BUTTON_DYNAMIC33,
                      CONTEXT_BUTTON_DYNAMIC34,
                      CONTEXT_BUTTON_DYNAMIC35,
                      CONTEXT_BUTTON_DYNAMIC36,
                      CONTEXT_BUTTON_DYNAMIC37,
                      CONTEXT_BUTTON_DYNAMIC38,
                      CONTEXT_BUTTON_DYNAMIC39,
                      CONTEXT_BUTTON_DYNAMIC40,
                      CONTEXT_BUTTON_DYNAMIC41,
                      CONTEXT_BUTTON_DYNAMIC42,
                      CONTEXT_BUTTON_DYNAMIC43,
                      CONTEXT_BUTTON_DYNAMIC44,
                      CONTEXT_BUTTON_DYNAMIC45,
                      CONTEXT_BUTTON_DYNAMIC46,
                      CONTEXT_BUTTON_DYNAMIC47,
                      CONTEXT_BUTTON_DYNAMIC48,
                      CONTEXT_BUTTON_DYNAMIC49,
                      CONTEXT_BUTTON_DYNAMIC50,
                      CONTEXT_BUTTON_DYNAMIC51,
                      CONTEXT_BUTTON_DYNAMIC52,
                      CONTEXT_BUTTON_DYNAMIC53,
                      CONTEXT_BUTTON_DYNAMIC54,
                      CONTEXT_BUTTON_DYNAMIC55,
                      CONTEXT_BUTTON_DYNAMIC56,
                      CONTEXT_BUTTON_DYNAMIC57,
                      CONTEXT_BUTTON_DYNAMIC58,
                      CONTEXT_BUTTON_DYNAMIC59,
                      CONTEXT_BUTTON_DYNAMIC60,
                      CONTEXT_BUTTON_DYNAMIC61,
                      CONTEXT_BUTTON_DYNAMIC62,
                      CONTEXT_BUTTON_DYNAMIC63,
                      CONTEXT_BUTTON_DYNAMIC64,
                      CONTEXT_BUTTON_DYNAMIC65,
                      CONTEXT_BUTTON_DYNAMIC66,
                      CONTEXT_BUTTON_DYNAMIC67,
                      CONTEXT_BUTTON_DYNAMIC68,
                      CONTEXT_BUTTON_DYNAMIC69,
                      CONTEXT_BUTTON_DYNAMIC70,
                      CONTEXT_BUTTON_DYNAMIC71,
                      CONTEXT_BUTTON_DYNAMIC72,
                      CONTEXT_BUTTON_DYNAMIC73,
                      CONTEXT_BUTTON_DYNAMIC74,
                      CONTEXT_BUTTON_DYNAMIC75,
                      CONTEXT_BUTTON_DYNAMIC76,
                      CONTEXT_BUTTON_DYNAMIC77,
                      CONTEXT_BUTTON_DYNAMIC78,
                      CONTEXT_BUTTON_DYNAMIC79,
                      CONTEXT_BUTTON_DYNAMIC80,
                      CONTEXT_BUTTON_DYNAMIC81,
                      CONTEXT_BUTTON_DYNAMIC82,
                      CONTEXT_BUTTON_DYNAMIC83,
                      CONTEXT_BUTTON_DYNAMIC84,
                      CONTEXT_BUTTON_DYNAMIC85,
                      CONTEXT_BUTTON_DYNAMIC86,
                      CONTEXT_BUTTON_DYNAMIC87,
                      CONTEXT_BUTTON_DYNAMIC88,
                      CONTEXT_BUTTON_DYNAMIC89,
                      CONTEXT_BUTTON_DYNAMIC90,
                      CONTEXT_BUTTON_DYNAMIC91,
                      CONTEXT_BUTTON_DYNAMIC92,
                      CONTEXT_BUTTON_DYNAMIC93,
                      CONTEXT_BUTTON_DYNAMIC94,
                      CONTEXT_BUTTON_DYNAMIC95,
                      CONTEXT_BUTTON_DYNAMIC96,
                      CONTEXT_BUTTON_DYNAMIC97,
                      CONTEXT_BUTTON_DYNAMIC98,
                      CONTEXT_BUTTON_DYNAMIC99
                    };

class CContextButtons : public std::vector< std::pair<CONTEXT_BUTTON, CStdString> >
{
public:
  void Add(CONTEXT_BUTTON, const CStdString &label);
  void Add(CONTEXT_BUTTON, int label);
};

class CGUIDialogContextMenu :
      public CGUIDialog
{
public:
  CGUIDialogContextMenu(void);
  virtual ~CGUIDialogContextMenu(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void DoModal(int iWindowID = WINDOW_INVALID, const CStdString &param = "");
  virtual void OnWindowLoaded();
  virtual void OnWindowUnload();
  virtual void SetPosition(float posX, float posY, bool center = true);
  void ClearButtons();
  int AddButton(int iLabel);
  int AddButton(const CStdString &strButton);
  int GetNumButtons();
  void EnableButton(int iButton, bool bEnable);
  int GetButton();
  float GetWidth();
  float GetHeight();

  static bool SourcesMenu(const CStdString &strType, const CFileItem *item, float posX, float posY);
  static void SwitchMedia(const CStdString& strType, const CStdString& strPath);

  static void GetContextButtons(const CStdString &type, CMediaSource *share, CContextButtons &buttons);
  static bool OnContextButton(const CStdString &type, CMediaSource *share, CONTEXT_BUTTON button);

  static int ShowAndGetChoice(const std::vector<CStdString> &choices, const CPoint &point);

  static CMediaSource *GetShare(const CStdString &type, const CFileItem *item);
protected:
  virtual void OnInitWindow();
  static CStdString GetDefaultShareNameByType(const CStdString &strType);
  static void SetDefault(const CStdString &strType, const CStdString &strDefault);
  static void ClearDefault(const CStdString &strType);

private:
  int m_iNumButtons;
  int m_iClickedButton;
};
