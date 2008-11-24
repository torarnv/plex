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
#include "PCMAmplifier.h"

CPCMAmplifier::CPCMAmplifier() : m_nVolume(VOLUME_MAXIMUM), m_dFactor(0)
{
}

CPCMAmplifier::~CPCMAmplifier()
{
}

void CPCMAmplifier::SetVolume(int nVolume)
{
  m_nVolume = nVolume;
  if (nVolume > VOLUME_MAXIMUM)
    nVolume = VOLUME_MAXIMUM;

  if (nVolume < VOLUME_MINIMUM)
    nVolume = VOLUME_MINIMUM;

  m_dFactor = 1.0 - fabs((float)nVolume / (float)(VOLUME_MAXIMUM - VOLUME_MINIMUM));
}

int  CPCMAmplifier::GetVolume()
{
  return m_nVolume;
}

// 32 bit float de-amplifier
void CPCMAmplifier::DeAmplifyInt16(short *pcm, int nSamples)
{
	if (m_dFactor >= 1.0)
	{
		// no process required. using >= to make sure no amp is ever done (only de-amp)
		return;
	}
	
	for (int nSample=0; nSample<nSamples; nSample++)
	{
		int nSampleValue = pcm[nSample];
		nSampleValue = (int)((double)nSampleValue * m_dFactor);		
		
		pcm[nSample] = (short)nSampleValue;
	}
}

// 32 bit float de-amplifier
void CPCMAmplifier::DeAmplifyFloat32(float *pcm, int nSamples)
{
  if (m_dFactor >= 1.0)
  {
    // no process required. using >= to make sure no amp is ever done (only de-amp)
    return;
  }

  for (int nSample=0; nSample<nSamples; nSample++)
  {
    float nSampleValue = pcm[nSample];
    nSampleValue *= m_dFactor;

    pcm[nSample] = nSampleValue;
  }
}
