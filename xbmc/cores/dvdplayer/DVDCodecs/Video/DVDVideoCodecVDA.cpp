/*
 *      Copyright (C) 2010 Plex, Inc.
 *      http://www.plexapp.com 
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

#include "stdafx.h"
#include "DVDVideoCodecVDA.h"
#include "DVDDemuxers/DVDDemux.h"
#include "DVDStreamInfo.h"
#include "DVDClock.h"
#include "DVDCodecs/DVDCodecs.h"
#include "../../../../utils/Win32Exception.h"
#ifdef _LINUX
#include "utils/CPUInfo.h"
#endif
#include "GUISettings.h"

#define kVDADecodeInfo_Asynchronous 1UL << 0
#define kVDADecodeInfo_FrameDropped 1UL << 1

// example helper function that wraps a time into a dictionary
static CFDictionaryRef MakeDictionaryWithDisplayTime(double inFrameDisplayTime)
{
    CFStringRef key = CFSTR("MyFrameDisplayTimeKey");
    CFNumberRef value = CFNumberCreate(kCFAllocatorDefault, kCFNumberFloat64Type, &inFrameDisplayTime);
	
    return CFDictionaryCreate(kCFAllocatorDefault,
                              (const void **)&key,
                              (const void **)&value,
                              1,
                              &kCFTypeDictionaryKeyCallBacks,
                              &kCFTypeDictionaryValueCallBacks);
}

// example helper function to extract a time from our dictionary
static Float64 GetFrameDisplayTimeFromDictionary(CFDictionaryRef inFrameInfoDictionary)
{
    CFNumberRef timeNumber = NULL;
    Float64 outValue = 0.0;
	
    if (NULL == inFrameInfoDictionary) return 0.0;
	
    timeNumber = (CFNumberRef)CFDictionaryGetValue(inFrameInfoDictionary, CFSTR("MyFrameDisplayTimeKey"));
    if (timeNumber) CFNumberGetValue(timeNumber, kCFNumberFloat64Type, &outValue);
	
    return outValue;
}



CDVDVideoCodecVDA::CDVDVideoCodecVDA() : CDVDVideoCodec()
{
	m_pConvertFrame = NULL;
	
	m_iPictureWidth = 0;
	m_iPictureHeight = 0;
	
  m_dropPictures = FALSE;
	m_iScreenWidth = 0;
	m_iScreenHeight = 0;
	
	m_iQueueDepth = 0;
	_displayQueue = NULL;
	for (int i = 0; i<3; i++)
	{
		yuvPlaneSize[i] = 0;
		yuvBuffer[i] = NULL;
	}
	pthread_mutex_init(&_queueMutex, NULL);
}

CDVDVideoCodecVDA::~CDVDVideoCodecVDA()
{
	Dispose();
}

CV_EXPORT const CFStringRef kCVPixelBufferIOSurfacePropertiesKey;

bool CDVDVideoCodecVDA::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
	hardwareDecoder = NULL;
	
	OSStatus status;
	
	CFMutableDictionaryRef decoderConfiguration = NULL;
	CFMutableDictionaryRef destinationImageBufferAttributes = NULL;
	CFDictionaryRef emptyDictionary; 
	
	CFNumberRef height = NULL;
	CFNumberRef width= NULL;
	CFNumberRef avcFormat = NULL;
	CFNumberRef pixelFormat = NULL; 
	CFDataRef avcCData = NULL;
	
	// source must be H.264
	if (hints.codec != CODEC_ID_H264)
	{
		CLog::Log(LOGERROR, "Source format is not H.264!");
		return FALSE;
	}
	// the avcC data chunk from the bitstream must be present
	if (hints.extrasize < 7 || hints.extradata == NULL) 
	{
		CLog::Log(LOGERROR, "avcC atom cannot be NULL!");
		return FALSE;
	}
	// the avcC data chunk from the bitstream must be valid
	if ( *(char*)hints.extradata != 1 )
	{
		CLog::Log(LOGERROR, "invalid avcC atom data");
		return FALSE;
	}
	
	
	// create a CFDictionary describing the source material for decoder configuration
	decoderConfiguration = CFDictionaryCreateMutable(kCFAllocatorDefault,
													 4,
													 &kCFTypeDictionaryKeyCallBacks,
													 &kCFTypeDictionaryValueCallBacks);
	
	SInt32 sourceFormat = 'avc1';
	m_iPictureHeight = (SInt32)hints.height;
	m_iPictureWidth = (SInt32)hints.width;
	
	height = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &m_iPictureHeight);
	width = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &m_iPictureWidth);
	avcFormat = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &sourceFormat);
	avcCData = CFDataCreate(kCFAllocatorDefault, (const UInt8 *)hints.extradata, hints.extrasize);
	
	CFDictionarySetValue(decoderConfiguration, kVDADecoderConfiguration_Height, height);
	CFDictionarySetValue(decoderConfiguration, kVDADecoderConfiguration_Width, width);
	CFDictionarySetValue(decoderConfiguration, kVDADecoderConfiguration_SourceFormat, avcFormat);
	CFDictionarySetValue(decoderConfiguration, kVDADecoderConfiguration_avcCData, avcCData);
	
	// create a CFDictionary describing the wanted destination image buffer
	destinationImageBufferAttributes = CFDictionaryCreateMutable(kCFAllocatorDefault,
																 2,
																 &kCFTypeDictionaryKeyCallBacks,
																 &kCFTypeDictionaryValueCallBacks);
	
	OSType cvPixelFormatType = kCVPixelFormatType_420YpCbCr8Planar;
	pixelFormat = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &cvPixelFormatType);
	emptyDictionary = CFDictionaryCreate(kCFAllocatorDefault, // our empty IOSurface properties dictionary
										 NULL,
										 NULL,
										 0,
										 &kCFTypeDictionaryKeyCallBacks,
										 &kCFTypeDictionaryValueCallBacks);
	
	CFDictionarySetValue(destinationImageBufferAttributes, kCVPixelBufferPixelFormatTypeKey, pixelFormat);
	CFDictionarySetValue(destinationImageBufferAttributes,
						 kCVPixelBufferIOSurfacePropertiesKey,
						 emptyDictionary);
	
	// create the hardware decoder object
	status = VDADecoderCreate(decoderConfiguration,
							  destinationImageBufferAttributes, 
							  (VDADecoderOutputCallback *)&DecoderOutputCallback,
							  this,
							  (VDADecoder *)&hardwareDecoder);
	
	if (kVDADecoderNoErr != status) 
	{
		if (status == kVDADecoderFormatNotSupportedErr)
		{
			CLog::Log(LOGERROR, "Video hardware does not suport VDA acceleration");
		}
		else if (status == kVDADecoderFormatNotSupportedErr)
		{
			CLog::Log(LOGERROR,  "Video hardware does not suport VDA acceleration of this video format");
		}
		else if (status == kVDADecoderConfigurationError)
		{
			CLog::Log(LOGERROR,  "Incorrect configuration passed to VDA decoder");
		}
		else if (status == kVDADecoderDecoderFailedErr)
		{
			CLog::Log(LOGERROR,  "Internal VDA decoder error");
		}
		else 
		{
			CLog::Log(LOGERROR, "VDA decoder failed, error %i", status);
		}					  
		return FALSE;
	}
	
	if (decoderConfiguration) CFRelease(decoderConfiguration);
	if (destinationImageBufferAttributes) CFRelease(destinationImageBufferAttributes);
	if (emptyDictionary) CFRelease(emptyDictionary);
	
	return TRUE;
}

void CDVDVideoCodecVDA::Dispose()
{
	if (hardwareDecoder)
	{
		VDADecoderDestroy((VDADecoder)hardwareDecoder);
		hardwareDecoder = NULL;
		for (int i=0; i<3; i++)
		{
			free(yuvBuffer[i]);
			yuvBuffer[i] = NULL;
		}
	}
}

void CDVDVideoCodecVDA::DecoderOutputCallback(void               *decompressionOutputRefCon,
											  CFDictionaryRef    frameInfo,
											  OSStatus           status, 
											  uint32_t           infoFlags,
											  CVImageBufferRef   imageBuffer)
{	
	CDVDVideoCodecVDA *self = (CDVDVideoCodecVDA *)decompressionOutputRefCon;
	
	if (NULL == imageBuffer) 
	{
        printf("myDecoderOutputCallback - NULL image buffer!\n");
        if (kVDADecodeInfo_FrameDropped & infoFlags) {
            printf("myDecoderOutputCallback - frame dropped!\n");
        }
        return;
    }
	if (kCVPixelFormatType_420YpCbCr8Planar != CVPixelBufferGetPixelFormatType(imageBuffer)) {
        printf("myDecoderOutputCallback - image buffer format not '2vuy'!\n");
        return;
    }
	
	displayFrameRef newFrame = NULL;
	
    // allocate a new frame and populate it with some information
    // this pointer to a myDisplayFrame type keeps track of the newest decompressed frame
    // and is then inserted into a linked list of  frame pointers depending on the display time
    // parsed out of the bitstream and stored in the frameInfo dictionary by the client
    newFrame = (displayFrameRef)malloc(sizeof(displayFrame));
	bzero(newFrame, sizeof(displayFrame));
	newFrame->nextFrame = NULL;
    newFrame->frame = CVPixelBufferRetain(imageBuffer);
    newFrame->frameDisplayTime = GetFrameDisplayTimeFromDictionary(frameInfo);
	
    // since the frames we get may be in decode order rather than presentation order
    // our hypothetical callback places them in a queue of frames which will
    // hold them in display order for display on another thread
    pthread_mutex_lock(&self->_queueMutex);
	
	displayFrameRef queueWalker = self->_displayQueue;
    if (!queueWalker || (newFrame->frameDisplayTime < queueWalker->frameDisplayTime)) 
	{
        // we have an empty queue, or this frame earlier than the current queue head
        newFrame->nextFrame = queueWalker;
        self->_displayQueue = newFrame;
    } 
	else 
	{
        // walk the queue and insert this frame where it belongs in display order
        BOOL frameInserted = FALSE;
        displayFrameRef nextFrame = NULL;
		
        while (!frameInserted) 
		{
            nextFrame = queueWalker->nextFrame;
            if (!nextFrame || (newFrame->frameDisplayTime < nextFrame->frameDisplayTime)) 
			{
                // if the next frame is the tail of the queue, or our new frame is ealier
                newFrame->nextFrame = nextFrame;
                queueWalker->nextFrame = newFrame;
                frameInserted = TRUE;
            }
            queueWalker = nextFrame;
        }
    }
	
    self->m_iQueueDepth++;
	pthread_mutex_unlock(&self->_queueMutex);
	
}

void CDVDVideoCodecVDA::SetDropState(bool bDrop)
{
	m_dropPictures = bDrop;
}

int CDVDVideoCodecVDA::Decode(BYTE* pData, int iSize, double pts)
{
	//CLog::Log(LOGDEBUG, "Queuing packet pts %.2f", pts);
	
	CFDictionaryRef frameInfo = NULL;
    OSStatus status = kVDADecoderNoErr;
	CFDataRef frameData = CFDataCreate(kCFAllocatorDefault, pData, iSize);
    // create a dictionary containg some information about the frame being decoded
    // in this case, we pass in the display time aquired from the stream
    frameInfo = MakeDictionaryWithDisplayTime(pts);
	
    // ask the hardware to decode our frame, frameInfo will be retained and pased back to us
    // in the output callback for this frame
	uint32_t avc_flags = 0;
	if (m_dropPictures)
		avc_flags = kVDADecoderDecodeFlags_DontEmitFrame;
	
    status = VDADecoderDecode((VDADecoder)hardwareDecoder, avc_flags, frameData, frameInfo);
	
	// the dictionary passed into decode is retained by the framework so
    // make sure to release it here
    CFRelease(frameInfo);
	CFRelease(frameData);
	
    if (kVDADecoderNoErr != status) 
	{
		CLog::Log(LOGERROR, "VDADecoderDecode failed. err: %d\n", (int)status);
		return VC_ERROR;
    }
	
	if (m_iQueueDepth < 16)
	{
		return VC_BUFFER;
		
	}
	return VC_PICTURE | VC_BUFFER;
}

void CDVDVideoCodecVDA::Reset()
{
	VDADecoderFlush((VDADecoder)hardwareDecoder, 0);
	
	while (_displayQueue)
	{
		AdvanceQueueHead();
	}
}

void CDVDVideoCodecVDA::AdvanceQueueHead()
{
	if (!_displayQueue || m_iQueueDepth == 0) return;
	
	// advance head
	displayFrameRef dropFrame = _displayQueue;
	_displayQueue = _displayQueue->nextFrame;
	
	// deallocate old frame
	CVPixelBufferRelease(dropFrame->frame);
	free(dropFrame);
	
	m_iQueueDepth--;
}

bool CDVDVideoCodecVDA::GetPicture(DVDVideoPicture* pDvdVideoPicture)
{
	pDvdVideoPicture->iWidth = pDvdVideoPicture->iDisplayWidth = m_iPictureWidth;
	pDvdVideoPicture->iHeight = pDvdVideoPicture->iDisplayHeight = m_iPictureHeight;
	pDvdVideoPicture->pts = DVD_NOPTS_VALUE;
	
	pthread_mutex_lock(&_queueMutex);
	CVPixelBufferLockBaseAddress(_displayQueue->frame, 0);
	
	// YUV420p frame
	for (int i = 0; i < 3; i++)
	{
		UInt32 planeHeight = CVPixelBufferGetHeightOfPlane(_displayQueue->frame, i);
		UInt32 planeWidth = CVPixelBufferGetBytesPerRowOfPlane(_displayQueue->frame, i);
		UInt32 planeSize = planeWidth * planeHeight;
		pDvdVideoPicture->iLineSize[i] = planeWidth;
		void *planePointer = CVPixelBufferGetBaseAddressOfPlane(_displayQueue->frame, i);
		
		if (!yuvBuffer[i] || yuvPlaneSize[i] != planeSize)
		{
			if (yuvBuffer[i])
			{
				free(yuvBuffer[i]);
			}
			// allocate YUV plane buffer
			yuvPlaneSize[i] = planeSize;
			yuvBuffer[i] = malloc(yuvPlaneSize[i]);
		}
		
		memcpy(yuvBuffer[i], planePointer, yuvPlaneSize[i]);
		pDvdVideoPicture->data[i] = (BYTE *)yuvBuffer[i];
	}
	CVPixelBufferUnlockBaseAddress(_displayQueue->frame, 0);
	
	pDvdVideoPicture->iRepeatPicture = 0;
	pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED;    
	
	pDvdVideoPicture->pts = _displayQueue->frameDisplayTime;
	
	AdvanceQueueHead();
	
	pthread_mutex_unlock(&_queueMutex);
	
	return VC_PICTURE | VC_BUFFER;
}

/*
 * Calculate the height and width this video should be displayed in
 */
void CDVDVideoCodecVDA::GetVideoAspect(AVCodecContext* pCodecContext, unsigned int& iWidth, unsigned int& iHeight)
{
	double aspect_ratio;
	
	/* XXX: use variable in the frame */
	if (pCodecContext->sample_aspect_ratio.num == 0) aspect_ratio = 0;
	else aspect_ratio = av_q2d(pCodecContext->sample_aspect_ratio) * pCodecContext->width / pCodecContext->height;
	
	if (aspect_ratio <= 0.0) aspect_ratio = (float)pCodecContext->width / (float)pCodecContext->height;
	
	/* XXX: we suppose the screen has a 1.0 pixel ratio */ // CDVDVideo will compensate it.
	iHeight = pCodecContext->height;
	if (iWidth > (unsigned int)pCodecContext->width)
	{
		iWidth = pCodecContext->width;
	}
}
