#pragma once
/*
 *  GUIDialogUtils.h
 *  Plex
 *
 *  Created by James Clarke on 12/09/2008.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */

#include <string>

using namespace std;

class CGUIDialogUtils
  {
  private:
    static bool progressDialogIsVisible;
    static string progressDialogHeading;
    static string progressDialogLine0;
    static string progressDialogLine1;
    static string progressDialogLine2;
    static bool progressDialogBarVisible;
    static int progressDialogPercentage;
  public:
    // Simple wrappers around GUIDialog functions for accessing from Objective C
    static const string Localize(int dwCode);
    static void SkinStateChanged();
    static void ShowOKDialog(int heading, int line0, int line1, int line2);
    static void ShowOKDialog(const string &heading, const string &line0, const string &line1, const string &line2);
    static void StartProgressDialog(const string &heading, const string &line0, const string &line1, const string &line2, bool showBar);
    static void CloseProgressDialog();
    static void SetProgressDialogPercentage(int iPercentage);
    static void SetProgressDialogLine(int iLine, const string &iString);
    static void SetProgressDialogBarVisible(bool iVisible);
    static bool ShowYesNoDialog(const string &heading, const string &line0, const string &line1, const string &line2);
    static void QueueToast(const string &caption, const string &description);
  };

