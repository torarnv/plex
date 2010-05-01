#pragma once

/*
 *      Copyright (C) 2010 Plex Incorporated
 *		http://www.plexapp.com
 *
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

#include "cores/ffmpeg/DllAvFormat.h"

#include "DVDVideoCodec.h"
#include <VideoDecodeAcceleration/VDADecoder.h>

#define DEBUG 0
#include <VideoDecodeAcceleration/VDADecoder.h>

// tracks a frame in and output queue in display order
typedef struct displayFrame {
    Float64					frameDisplayTime;
    CVPixelBufferRef        frame;
    struct displayFrame		*nextFrame;
} displayFrame, *displayFrameRef;

class CDVDVideoCodecVDA : public CDVDVideoCodec
{
public:
	CDVDVideoCodecVDA();
	virtual ~CDVDVideoCodecVDA();
	virtual bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);  
	virtual void Dispose();
	virtual int Decode(BYTE* pData, int iSize, double pts);
	virtual void Reset();
	virtual bool GetPicture(DVDVideoPicture* pDvdVideoPicture);
	virtual const char* GetName() { return "AppleHardwareVDA"; };
	virtual void SetDropState(bool bDrop);
	
protected:
	void GetVideoAspect(AVCodecContext* pCodecContext, unsigned int& iWidth, unsigned int& iHeight);
	static void DecoderOutputCallback(void        *decompressionOutputRefCon,
							   CFDictionaryRef    frameInfo,
							   OSStatus           status, 
							   uint32_t           infoFlags,
							   CVImageBufferRef   imageBuffer);
	void AdvanceQueueHead();


	AVPicture* m_pConvertFrame;
	
	SInt32 m_iPictureWidth;
	SInt32 m_iPictureHeight;
	bool m_DropPictures;
	
  bool m_dropPictures;
	int m_iScreenWidth;
	int m_iScreenHeight;
	//	DllSwScale m_dllSwScale;
	void *hardwareDecoder;
	void *yuvBuffer[3];
	UInt32 yuvPlaneSize[3];
	int m_iQueueDepth;
	displayFrameRef _displayQueue;    
	pthread_mutex_t   _queueMutex; // mutex protecting queue manipulation

	
};

