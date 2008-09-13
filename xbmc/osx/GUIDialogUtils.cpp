/*
 *  CocoaGUIDialogUtils.cpp
 *  Plex
 *
 *  Created by James Clarke on 12/09/2008.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include "stdafx.h"
#include "GUIDialogUtils.h"
#include "GUIWindowManager.h"
#include "GUIDialogOK.h"
#include "GUIDialogProgress.h"
#include "GUIDialogYesNo.h"

using namespace std;

void CGUIDialogUtils::ShowOKDialog(int heading, int line0, int line1, int line2)
{
  CGUIDialogOK::ShowAndGetInput(heading, line0, line1, line2);
}

void CGUIDialogUtils::ShowOKDialog(const string &heading, const string &line0, const string &line1, const string &line2)
{
  CGUIDialogOK *dialog = (CGUIDialogOK *)m_gWindowManager.GetWindow(WINDOW_DIALOG_OK);
  if (!dialog) return;
  dialog->SetHeading( heading );
  dialog->SetLine( 0, line0 );
  dialog->SetLine( 1, line1 );
  dialog->SetLine( 2, line2 );
  dialog->DoModal();
}

void CGUIDialogUtils::StartProgressDialog(const string &heading, const string &line0, const string &line1, const string &line2, bool showBar)
{
  CGUIDialogProgress *dialog = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (!dialog) return;
  dialog->SetHeading(heading);
  dialog->SetLine(0, line0);
  dialog->SetLine(1, line1);
  dialog->SetLine(2, line2);
  dialog->SetPercentage(0);
  dialog->ShowProgressBar(showBar);
  dialog->SetCanCancel(false);
  dialog->StartModal();
}

void CGUIDialogUtils::CloseProgressDialog()
{
  CGUIDialogProgress *dialog = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (dialog) dialog->Close(true);
}

void CGUIDialogUtils::SetProgressDialogPercentage(int iPercentage)
{
  CGUIDialogProgress *dialog = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (dialog)
  {
    dialog->SetPercentage(iPercentage);
    dialog->Progress();
  }
}

void CGUIDialogUtils::SetProgressDialogLine(int iLine, const string &iString)
{
  CGUIDialogProgress *dialog = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (dialog) dialog->SetLine(iLine, iString);
}

void CGUIDialogUtils::SetProgressDialogBarVisible(bool iVisible)
{
  CGUIDialogProgress *dialog = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (dialog) dialog->ShowProgressBar(iVisible);
}

bool CGUIDialogUtils::ShowYesNoDialog(const string &heading, const string &line0, const string &line1, const string &line2)
{
  return CGUIDialogYesNo::ShowAndGetInput(heading, line0, line1, line2);
}