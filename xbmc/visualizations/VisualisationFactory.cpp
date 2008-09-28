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
#include "VisualisationFactory.h"
#include "Util.h"


CVisualisationFactory::CVisualisationFactory()
{

}

CVisualisationFactory::~CVisualisationFactory()
{

}

CVisualisation* CVisualisationFactory::LoadVisualisation(const CStdString& strVisz) const
{
#ifdef HAS_VISUALISATION
  printf("VIZ: %s\n", strVisz.c_str());
  
  // Strip of the path & extension to get the name of the visualisation like goom or spectrum
  CStdString strName = CUtil::GetFileName(strVisz);
  strName = strName.Left(strName.size() - 4);

  CStdString strLibrary = strVisz;
  CStdString strVisualization = strName;
  CStdString strSubmodule = strName;
  
  // See if the name implies a sub-visualizer.
  int start = strName.Find("(");
  int end   = strName.Find(")");
  
  if (start > 0 && end > 0)
  {
    // The name is in the format "Module (Visualizer)"
    strVisualization = strName.substr(start+1, end-start-1);
    strSubmodule = strName.substr(0, start-1);
    
    // Compute the name of the library to load.
    CUtil::GetDirectory(strVisz, strLibrary);
    strLibrary = CUtil::AddFileToFolder(strLibrary, strVisualization + ".vis");
  }

  printf("Got library: [%s] viz: [%s], subvisualizer: [%s]\n", strLibrary.c_str(), strVisualization.c_str(), strSubmodule.c_str());
  
  // load visualisation.
  DllVisualisation* pDll = new DllVisualisation();
  pDll->SetFile(strLibrary);
  
  // FIXME: Some Visualisations do not work when their dll is not unloaded immediately.
  pDll->EnableDelayedUnload(false);
  if (!pDll->Load())
  {
    delete pDll;
    return 0;
  }

  Visualisation* pVisz = (Visualisation* )malloc(sizeof(struct Visualisation));
  ZeroMemory(pVisz, sizeof(struct Visualisation));
  pDll->GetModule(pVisz);

  // and pass it to a new instance of CVisualisation() which will hanle the visualisation
  return new CVisualisation(pVisz, pDll, strName, strSubmodule);
  
#else
  return 0;
#endif
}
