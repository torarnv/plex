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

#include "BackgroundInfoLoader.h"
#include "GUIWindow.h"
#include "FileSystem/VirtualDirectory.h"
#include "FileSystem/DirectoryHistory.h"
#include "GUIViewControl.h"
#include "GUIDialogContextMenu.h"

class CFileItemList;
class MediaRefresher;

// base class for all media windows
class CGUIMediaWindow : public CGUIWindow
{
public:
  CGUIMediaWindow(DWORD id, const char *xmlFile);
  virtual ~CGUIMediaWindow(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void OnWindowLoaded();
  virtual void OnWindowUnload();
  virtual void OnInitWindow();
  virtual bool IsMediaWindow() const { return true; };
  const CFileItemList &CurrentDirectory() const;
  const CStdString& StartDirectory() const { return m_startDirectory; }
  void SetStartDirectory(const CStdString& startDir) { m_startDirectory = startDir; }
  CDirectoryHistory& DirectoryHistory()  { return m_history; }
  void SetDirectoryHistory(const CDirectoryHistory& history) { m_history = history; }
  
  int GetViewContainerID() const { return m_viewControl.GetCurrentControl(); };
  virtual bool HasListItems() const { return true; };
  const CGUIViewState *GetViewState() const;
  virtual CFileItemPtr GetCurrentListItem(int offset = 0);
  virtual void Render();
  void SetUpdatedItem(const CFileItemPtr& updatedItem) 
  { 
    CFileItem* pItem = new CFileItem();
    (*pItem) = *(updatedItem.get());
    
    m_updatedItem = CFileItemPtr(pItem); 
  }
  
protected:
  virtual void LoadAdditionalTags(TiXmlElement *root);
  CGUIControl *GetFirstFocusableControl(int id);
  void SetupShares();
  virtual void GoParentFolder();
  virtual bool OnClick(int iItem);
  bool OnClick(const CFileItemPtr& pItem, int iItem);
  virtual bool OnPopupMenu(int iItem);
  virtual void GetContextButtons(int itemNumber, CContextButtons &buttons);
  virtual bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
  virtual void FormatItemLabels(CFileItemList &items, const LABEL_MASKS &labelMasks);
  virtual void UpdateButtons();
  virtual bool GetDirectory(const CStdString &strDirectory, CFileItemList &items);
  virtual bool Update(const CStdString &strDirectory);
  virtual void FormatAndSort(CFileItemList &items);
  virtual void OnPrepareFileItems(CFileItemList &items);
  virtual void OnFinalizeFileItems(CFileItemList &items);

  virtual void ClearFileItems();
  virtual void SortItems(CFileItemList &items);

  // check for a disc or connection
  virtual bool HaveDiscOrConnection(CStdString& strPath, int iDriveType);
  void ShowShareErrorMessage(CFileItem* pItem);

  void GetDirectoryHistoryString(const CFileItem* pItem, CStdString& strHistoryString);
  void SetHistoryForPath(const CStdString& strDirectory);
  virtual void LoadPlayList(const CStdString& strFileName) {}
  virtual bool OnPlayMedia(int iItem);
  virtual bool OnPlayMedia(CFileItem* pItem);
  void UpdateFileList();
  virtual void OnDeleteItem(int iItem);
  void OnRenameItem(int iItem);

protected:
  virtual CBackgroundInfoLoader* GetBackgroundLoader() { return 0; }
  
  bool WaitForNetwork() const;
  CPoint GetContextPosition() const;
  void RefreshShares(bool update=false);

  DIRECTORY::CVirtualDirectory m_rootDir;
  CGUIViewControl m_viewControl;

  // current path and history
  CFileItemList* m_vecItems;
  CDirectoryHistory m_history;
  std::auto_ptr<CGUIViewState> m_guiState;
  
  CFileItemPtr m_updatedItem;

  // save control state on window exit
  int m_iLastControl;
  int m_iSelectedItem;
  
  MediaRefresher* m_mediaRefresher;
  bool m_wasDirectoryListingCancelled;
  CStopWatch m_refreshTimer;
  bool m_isRefreshing;
  
  CStdString m_startDirectory;
};
