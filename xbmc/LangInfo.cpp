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
#include "LangInfo.h"
#include "Settings.h"
#include "Util.h"
#include "GUISettings.h"
#ifdef __APPLE__
#include "CocoaUtilsPlus.h"
#endif

CLangInfo g_langInfo;

#define TEMP_UNIT_STRINGS 20027

#define SPEED_UNIT_STRINGS 20200

CLangInfo::CRegion::CRegion(const CRegion& region)
{
  m_strName=region.m_strName;
  m_forceUnicodeFont=region.m_forceUnicodeFont;
  m_strGuiCharSet=region.m_strGuiCharSet;
  m_strSubtitleCharSet=region.m_strSubtitleCharSet;
  m_strDVDMenuLanguage=region.m_strDVDMenuLanguage;
  m_strDVDAudioLanguage=region.m_strDVDAudioLanguage;
  m_strDVDSubtitleLanguage=region.m_strDVDSubtitleLanguage;
  m_strDateFormatShort=region.m_strDateFormatShort;
  m_strDateFormatLong=region.m_strDateFormatLong;
  m_strTimeFormat=region.m_strTimeFormat;
  m_strMeridiemSymbols[MERIDIEM_SYMBOL_PM]=region.m_strMeridiemSymbols[MERIDIEM_SYMBOL_PM];
  m_strMeridiemSymbols[MERIDIEM_SYMBOL_AM]=region.m_strMeridiemSymbols[MERIDIEM_SYMBOL_AM];
  
  m_strTimeZone = region.m_strTimeZone;
}

CLangInfo::CRegion::CRegion()
{
  SetDefaults();
}

CLangInfo::CRegion::~CRegion()
{

}

void CLangInfo::CRegion::SetDefaults()
{
  m_strName="N/A";
  m_forceUnicodeFont=false;
  m_strGuiCharSet="CP1252";
  m_strSubtitleCharSet="CP1252";
  m_strDVDMenuLanguage="en";
  m_strDVDAudioLanguage="en";
  m_strDVDSubtitleLanguage="en";

#ifdef __APPLE__
  m_strDateFormatShort = Cocoa_GetShortDateFormat();
  m_strDateFormatLong = Cocoa_GetLongDateFormat();
  m_strTimeFormat = Cocoa_GetTimeFormat();
  m_strMeridiemSymbols[MERIDIEM_SYMBOL_PM] = Cocoa_GetMeridianSymbol(MERIDIEM_SYMBOL_PM);
  m_strMeridiemSymbols[MERIDIEM_SYMBOL_AM] = Cocoa_GetMeridianSymbol(MERIDIEM_SYMBOL_AM);
#else
  m_strDateFormatShort="DD/MM/YYYY";
  m_strDateFormatLong="DDDD, D MMMM YYYY";
  m_strTimeFormat="HH:mm:ss";
#endif
  
  m_strTimeZone.clear();
}

void CLangInfo::CRegion::SetSpeedUnit(const CStdString& strUnit)
{
}

void CLangInfo::CRegion::SetTimeZone(const CStdString& strTimeZone)
{
  m_strTimeZone = strTimeZone;
}

CLangInfo::CLangInfo()
{
  SetDefaults();
}

CLangInfo::~CLangInfo()
{
}

bool CLangInfo::Load(const CStdString& strFileName)
{
  printf("Loading file: %s\n", strFileName.c_str());
  SetDefaults();

  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(strFileName.c_str()))
  {
    CLog::Log(LOGERROR, "unable to load %s: %s at line %d", strFileName.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  CStdString strValue = pRootElement->Value();
  if (strValue != CStdString("language"))
  {
    CLog::Log(LOGERROR, "%s Doesn't contain <language>", strFileName.c_str());
    return false;
  }

  const TiXmlNode *pCharSets = pRootElement->FirstChild("charsets");
  if (pCharSets && !pCharSets->NoChildren())
  {
    const TiXmlNode *pGui = pCharSets->FirstChild("gui");
    if (pGui && !pGui->NoChildren())
    {
      CStdString strForceUnicodeFont = ((TiXmlElement*) pGui)->Attribute("unicodefont");

      if (strForceUnicodeFont.Equals("true"))
        m_defaultRegion.m_forceUnicodeFont=true;

      m_defaultRegion.m_strGuiCharSet=pGui->FirstChild()->Value();
    }

    const TiXmlNode *pSubtitle = pCharSets->FirstChild("subtitle");
    if (pSubtitle && !pSubtitle->NoChildren())
      m_defaultRegion.m_strSubtitleCharSet=pSubtitle->FirstChild()->Value();
  }

  const TiXmlNode *pDVD = pRootElement->FirstChild("dvd");
  if (pDVD && !pDVD->NoChildren())
  {
    const TiXmlNode *pMenu = pDVD->FirstChild("menu");
    if (pMenu && !pMenu->NoChildren())
      m_defaultRegion.m_strDVDMenuLanguage=pMenu->FirstChild()->Value();

    const TiXmlNode *pAudio = pDVD->FirstChild("audio");
    if (pAudio && !pAudio->NoChildren())
      m_defaultRegion.m_strDVDAudioLanguage=pAudio->FirstChild()->Value();

    const TiXmlNode *pSubtitle = pDVD->FirstChild("subtitle");
    if (pSubtitle && !pSubtitle->NoChildren())
      m_defaultRegion.m_strDVDSubtitleLanguage=pSubtitle->FirstChild()->Value();
  }

  const TiXmlNode *pRegions = pRootElement->FirstChild("regions");
  if (pRegions && !pRegions->NoChildren())
  {
    const TiXmlElement *pRegion=pRegions->FirstChildElement("region");
    while (pRegion)
    {
      CRegion region(m_defaultRegion);
      region.m_strName=pRegion->Attribute("name");
      if (region.m_strName.IsEmpty())
        region.m_strName="N/A";

      const TiXmlNode *pTimeZone=pRegion->FirstChild("timezone");
      if (pTimeZone && !pTimeZone->NoChildren())
        region.SetTimeZone(pTimeZone->FirstChild()->Value());

      m_regions.insert(PAIR_REGIONS(region.m_strName, region));

      pRegion=pRegion->NextSiblingElement("region");
    }
  }

  const TiXmlNode *pTokens = pRootElement->FirstChild("sorttokens");
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
  return true;
}

void CLangInfo::SetDefaults()
{
  m_regions.clear();

  //Reset default region
  m_defaultRegion.SetDefaults();

  // Set the default region, we may be unable to load langinfo.xml
  m_currentRegion=&m_defaultRegion;
}

CStdString CLangInfo::GetGuiCharSet() const
{
  CStdString strCharSet;
  strCharSet=g_guiSettings.GetString("region.charset");
  if (strCharSet=="DEFAULT")
    strCharSet=m_currentRegion->m_strGuiCharSet;

  return strCharSet;
}

CStdString CLangInfo::GetSubtitleCharSet() const
{
  CStdString strCharSet=g_guiSettings.GetString("subtitles.charset");
  if (strCharSet=="DEFAULT")
    strCharSet=m_currentRegion->m_strSubtitleCharSet;

  return strCharSet;
}

// two character codes as defined in ISO639
const CStdString& CLangInfo::GetDVDMenuLanguage() const
{
  return m_currentRegion->m_strDVDMenuLanguage;
}

// two character codes as defined in ISO639
const CStdString& CLangInfo::GetDVDAudioLanguage() const
{
  return m_currentRegion->m_strDVDAudioLanguage;
}

// two character codes as defined in ISO639
const CStdString& CLangInfo::GetDVDSubtitleLanguage() const
{
  return m_currentRegion->m_strDVDSubtitleLanguage;
}

// Returns the format string for the date of the current language
const CStdString& CLangInfo::GetDateFormat(bool bLongDate/*=false*/) const
{
  if (bLongDate)
    return m_currentRegion->m_strDateFormatLong;
  else
    return m_currentRegion->m_strDateFormatShort;
}

// Returns the format string for the time of the current language
const CStdString& CLangInfo::GetTimeFormat() const
{
  return m_currentRegion->m_strTimeFormat;
}

const CStdString& CLangInfo::GetTimeZone() const
{
  return m_currentRegion->m_strTimeZone;
}

// Returns the AM/PM symbol of the current language
const CStdString& CLangInfo::GetMeridiemSymbol(MERIDIEM_SYMBOL symbol) const
{
  return m_currentRegion->m_strMeridiemSymbols[symbol];
}

// Fills the array with the region names available for this language
void CLangInfo::GetRegionNames(CStdStringArray& array)
{
  for (ITMAPREGIONS it=m_regions.begin(); it!=m_regions.end(); ++it)
  {
    CStdString strName=it->first;
    if (strName=="N/A")
      strName=g_localizeStrings.Get(416);
    array.push_back(strName);
  }
}

// Set the current region by its name, names from GetRegionNames() are valid.
// If the region is not found the first available region is set.
void CLangInfo::SetCurrentRegion(const CStdString& strName)
{
  ITMAPREGIONS it=m_regions.find(strName);
  if (it!=m_regions.end())
    m_currentRegion=&it->second;
  else if (!m_regions.empty())
    m_currentRegion=&m_regions.begin()->second;
  else
    m_currentRegion=&m_defaultRegion;
}

// Returns the current region set for this language
const CStdString& CLangInfo::GetCurrentRegion()
{
  return m_currentRegion->m_strName;
}

CLangInfo::TEMP_UNIT CLangInfo::GetTempUnit()
{
#ifdef __APPLE__
  return Cocoa_IsMetricSystem() ? TEMP_UNIT_CELSIUS : TEMP_UNIT_FAHRENHEIT;
#else
  return (CLangInfo::TEMP_UNIT)g_guiSettings.GetInt("region.temperatureunits");
#endif
}

// Returns the temperature unit string for the current language
const CStdString& CLangInfo::GetTempUnitString()
{
  TEMP_UNIT tempUnit = GetTempUnit();
  
  if (tempUnit == 0)
    return g_localizeStrings.Get(TEMP_UNIT_STRINGS+0);
  else
    return g_localizeStrings.Get(TEMP_UNIT_STRINGS+1+tempUnit);
}

CLangInfo::SPEED_UNIT CLangInfo::GetSpeedUnit()
{
#ifdef __APPLE__
  return Cocoa_IsMetricSystem() ? SPEED_UNIT_KMH : SPEED_UNIT_MPH;
#else
  return (CLangInfo::SPEED_UNIT)g_guiSettings.GetInt("region.speedunits");
#endif
}

// Returns the speed unit string for the current language
const CStdString& CLangInfo::GetSpeedUnitString()
{
  // Translate offset (we whacked a bunch of useless ones, more is not always better).
  SPEED_UNIT unit = GetSpeedUnit();
  int offset = 0;
  
  switch (unit)
  {
  case SPEED_UNIT_KMH: offset=0; break;
  case SPEED_UNIT_MPS: offset=2; break;
  case SPEED_UNIT_MPH: offset=6; break;
  case SPEED_UNIT_BEAUFORT: offset=8; break;
  }
  
  return g_localizeStrings.Get(SPEED_UNIT_STRINGS+offset);
}
