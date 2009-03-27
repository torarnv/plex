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
#include "DVDSubtitleParserSami.h"
#include "DVDClock.h"
#include "DVDStreamInfo.h"

using namespace std;

CDVDSubtitleParserSami::CDVDSubtitleParserSami(CDVDSubtitleStream* stream, const string& filename)
    : CDVDSubtitleParser(stream, filename),m_reg(true)
{

}

CDVDSubtitleParserSami::~CDVDSubtitleParserSami()
{
  Dispose();
}

bool CDVDSubtitleParserSami::Open(CDVDStreamInfo &hints)
{
  if (!m_pStream->Open(m_strFileName))
    return false;

  if (!initializeParsingInfo())
    return false;

  char line[1024];

  while ( m_pStream->ReadLine(line, sizeof(line)))
  {
    CDVDOverlayText* pOverlay = parseLineAndReturnOverlayText( line);
    if ( pOverlay)
    {
      m_collection.Add( pOverlay);
    }
  }
  m_pStream->Close();

  return true;
}

void CDVDSubtitleParserSami::Dispose()
{
  m_collection.Clear();
}

void CDVDSubtitleParserSami::Reset()
{
  m_collection.Reset();
}

// parse exactly one subtitle
CDVDOverlay* CDVDSubtitleParserSami::Parse(double iPts)
{
  return  m_collection.Get(iPts);;
}

bool CDVDSubtitleParserSami::initializeParsingInfo()
{
  m_startTime = 0;
  m_endTime = 0;
  m_pOverlay = NULL;
  m_textCount = 0;

  if ( !m_reg.RegComp("<SYNC (Start|START)=([0-9]+)><[^>]+>"))
  {
    CLog::Log(LOGINFO, "reg compile faile");
    return false;
  }
  return true;
}

// There can be text within the same line like "<SYNC Start=1234><P Class=XX>some text<br>some text"
char* CDVDSubtitleParserSami::getTextFromSyncLine( char* line)
{
  char* first = strchr( line, '>');
  if ( first == NULL) return NULL;
  char* second = strchr( first + 1, '>');
  if ( second == NULL) return NULL;
  char* text = second + 1;
  if ( *text != '\0' && strstr( text, "&nbsp;") == NULL) // ignore line containing "&nbsp;"
  {
    return text;
  }
  return NULL;
}
CDVDOverlayText* CDVDSubtitleParserSami::getOverlayTextObjectCompleted()
{
  // if all informations for overlay text are collected, return OverlayText object and initialize it.
  if ( m_pOverlay && m_textCount != 0 && m_endTime != 0)
  {
    CDVDOverlayText* ret = m_pOverlay;
    ret->iPTSStartTime = m_startTime * DVD_TIME_BASE/1000;
    ret->iPTSStopTime = m_endTime * DVD_TIME_BASE/1000;
    //CLog::Log(LOGINFO, "Sami stime:%f, etime:%f", ret->iPTSStartTime, ret->iPTSStopTime);

    m_pOverlay = NULL;
    m_startTime = m_endTime;
    m_endTime = 0;
    m_textCount = 0;

    return ret;
  }
  return NULL;
}

void CDVDSubtitleParserSami::convertAndAddElement( char* line)
{
  if ( m_startTime == 0)
  {
    return;
  }
  CStdStringW strUTF16;
  CStdStringA strUTF8;

  g_charsetConverter.subtitleCharsetToW(line, strUTF16);
  g_charsetConverter.wToUTF8(strUTF16, strUTF8);

  if (!strUTF8.IsEmpty())
  {
    // if overlay text object is initial state, create object
    if ( m_pOverlay == NULL && m_textCount == 0)
    {
      m_pOverlay = new CDVDOverlayText();
      m_pOverlay->Acquire(); // increase ref count with one so that we can hold a handle to this overlay
    }
    // remove "<br>" at the end of line. it's for better display overlay text.
    char* str = strdup( strUTF8.c_str());
    char* str_br = strstr(str + strlen(str)-6,"<br>");
    if ( str_br) *str_br='\0';
    // add a new text element to our container
    m_pOverlay->AddElement(new CDVDOverlayText::CElementText(str));
    free(str);
    m_textCount++;
  }
}

void CDVDSubtitleParserSami::addTime( char* line)
{
  if ( m_textCount == 0)
  {
    m_startTime = atoi( line);
  }
  else
  {
    m_endTime = atoi( line);
  }
}
CDVDOverlayText* CDVDSubtitleParserSami::parseLineAndReturnOverlayText( char* line)
{
  if ( m_reg.RegFind( line) > -1)
  {
    //CLog::Log(LOGINFO, "Sami sync: %s", line);
    char* timeFrame = m_reg.GetReplaceString("\\2");
    addTime( timeFrame);
    char* text = getTextFromSyncLine( line);
    if ( text)
    {
      convertAndAddElement( text);
    }

    return getOverlayTextObjectCompleted();
  }
  else
  {
    //CLog::Log(LOGINFO, "Sami text: %s", line);
    convertAndAddElement( line);
    return NULL;
  }
}
