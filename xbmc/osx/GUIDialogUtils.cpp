/*
 *  GUIDialogUtils.cpp
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
#include "LocalizeStrings.h"
#include "Application.h"

using namespace std;

bool CGUIDialogUtils::progressDialogIsVisible = false;
string CGUIDialogUtils::progressDialogHeading = "";
string CGUIDialogUtils::progressDialogLine0 = "";
string CGUIDialogUtils::progressDialogLine1 = "";
string CGUIDialogUtils::progressDialogLine2 = "";
bool CGUIDialogUtils::progressDialogBarVisible = false;
int CGUIDialogUtils::progressDialogPercentage = 0;
bool CGUIDialogUtils::progressDialogBarNeedsDisplay = false;


const string CGUIDialogUtils::Localize(int dwCode)
{
  return g_localizeStrings.Get(dwCode);
}

//
// This function is needed to redisplay progress dialogs after the skin state has changed (e.g. the user has toggled fullscreen).
// Without it, the dialog disappears & the user has no way of knowing whether the operation is still running.
//
void CGUIDialogUtils::UpdateProgressDialog()
{
  CGUIDialogProgress *dialog = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (!dialog) return;
  if (CGUIDialogUtils::progressDialogIsVisible)
  {
    CGUIDialogUtils::progressDialogIsVisible = true;
    dialog->SetHeading(CGUIDialogUtils::progressDialogHeading);
    dialog->SetLine(0, CGUIDialogUtils::progressDialogLine0);
    dialog->SetLine(1, CGUIDialogUtils::progressDialogLine1);
    dialog->SetLine(2, CGUIDialogUtils::progressDialogLine2);
    dialog->SetPercentage(CGUIDialogUtils::progressDialogPercentage);

    if (CGUIDialogUtils::progressDialogBarNeedsDisplay)
      dialog->ShowProgressBar(CGUIDialogUtils::progressDialogBarVisible);
    CGUIDialogUtils::progressDialogBarNeedsDisplay = false;
    
    if (!dialog->IsDialogRunning())
    {
      dialog->ShowProgressBar(CGUIDialogUtils::progressDialogBarVisible);
      dialog->SetCanCancel(false);
      dialog->StartModal();
    }
  }
  else
    dialog->Close(true);
}

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
  CGUIDialogUtils::progressDialogIsVisible = true;
  dialog->SetHeading(heading);
  dialog->SetLine(0, line0);
  dialog->SetLine(1, line1);
  dialog->SetLine(2, line2);
  dialog->SetPercentage(0);
  dialog->ShowProgressBar(showBar);
  dialog->SetCanCancel(false);
  
  // Store the details in case they need to be restored later
  CGUIDialogUtils::progressDialogHeading = heading;
  CGUIDialogUtils::progressDialogLine0 = line0;
  CGUIDialogUtils::progressDialogLine1 = line1;
  CGUIDialogUtils::progressDialogLine2 = line2;
  CGUIDialogUtils::progressDialogBarVisible = showBar;
  CGUIDialogUtils::progressDialogPercentage = 0;
  
  dialog->StartModal();
}

void CGUIDialogUtils::CloseProgressDialog()
{
  CGUIDialogProgress *dialog = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (dialog) 
  {
    dialog->Close(true);
    CGUIDialogUtils::progressDialogIsVisible = false;
  }
}

void CGUIDialogUtils::SetProgressDialogPercentage(int iPercentage)
{
  CGUIDialogUtils::progressDialogPercentage = iPercentage;
}

void CGUIDialogUtils::SetProgressDialogLine(int iLine, const string &iString)
{
  switch (iLine)
  {
    case 0: CGUIDialogUtils::progressDialogLine0 = iString; break;
    case 1: CGUIDialogUtils::progressDialogLine1 = iString; break;
    case 2: CGUIDialogUtils::progressDialogLine2 = iString;
  }
}

void CGUIDialogUtils::SetProgressDialogBarVisible(bool iVisible)
{
  CGUIDialogUtils::progressDialogBarVisible = iVisible;
  CGUIDialogUtils::progressDialogBarNeedsDisplay = true;
}

bool CGUIDialogUtils::ShowYesNoDialog(const string &heading, const string &line0, const string &line1, const string &line2)
{
  return CGUIDialogYesNo::ShowAndGetInput(heading, line0, line1, line2);
}

void CGUIDialogUtils::QueueToast(const string &caption, const string &description)
{
  g_application.m_guiDialogKaiToast.QueueNotification(caption, description);
}