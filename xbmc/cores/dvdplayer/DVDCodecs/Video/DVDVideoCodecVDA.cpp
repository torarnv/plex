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

CV_EXPORT const CFStringRef kCVPixelBufferIOSurfacePropertiesKey;

/*
 * if extradata size is greater than 7, then have a valid quicktime 
 * avcC atom header.
 *
 *      -: avcC atom header :-
 *  -----------------------------------
 *  1 byte  - version
 *  1 byte  - h.264 stream profile
 *  1 byte  - h.264 compatible profiles
 *  1 byte  - h.264 stream level
 *  6 bits  - reserved set to 63
 *  2 bits  - NAL length 
 *            ( 0 - 1 byte; 1 - 2 bytes; 3 - 4 bytes)
 *  3 bit   - reserved
 *  5 bits  - number of SPS 
 *  for (i=0; i < number of SPS; i++) {
 *      2 bytes - SPS length
 *      SPS length bytes - SPS NAL unit
 *  }
 *  1 byte  - number of PPS
 *  for (i=0; i < number of PPS; i++) {
 *      2 bytes - PPS length 
 *      PPS length bytes - PPS NAL unit 
 *  }
 */

#define kVDADecodeInfo_Asynchronous 1UL << 0
#define kVDADecodeInfo_FrameDropped 1UL << 1

#pragma mark -
#pragma mark FFmpeg h264 bytestream functions

// AVC helper functions for muxers, 
//  * Copyright (c) 2006 Baptiste Coudurier <baptiste.coudurier@smartjog.com>
// This is part of FFmpeg
//  * License as published by the Free Software Foundation; either
//  * version 2.1 of the License, or (at your option) any later version.
#define VDA_RB24(x)                          \
((((const uint8_t*)(x))[0] << 16) |        \
(((const uint8_t*)(x))[1] <<  8) |        \
((const uint8_t*)(x))[2])

#define VDA_RB32(x)                          \
((((const uint8_t*)(x))[0] << 24) |        \
(((const uint8_t*)(x))[1] << 16) |        \
(((const uint8_t*)(x))[2] <<  8) |        \
((const uint8_t*)(x))[3])

static const uint8_t *avc_find_startcode_internal(const uint8_t *p, const uint8_t *end)
{
	const uint8_t *a = p + 4 - ((intptr_t)p & 3);
	
	for (end -= 3; p < a && p < end; p++)
	{
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
			return p;
	}
	
	for (end -= 3; p < end; p += 4)
	{
		uint32_t x = *(const uint32_t*)p;
		if ((x - 0x01010101) & (~x) & 0x80808080) // generic
		{
			if (p[1] == 0)
			{
				if (p[0] == 0 && p[2] == 1)
					return p;
				if (p[2] == 0 && p[3] == 1)
					return p+1;
			}
			if (p[3] == 0)
			{
				if (p[2] == 0 && p[4] == 1)
					return p+2;
				if (p[4] == 0 && p[5] == 1)
					return p+3;
			}
		}
	}
	
	for (end += 3; p < end; p++)
	{
		if (p[0] == 0 && p[1] == 0 && p[2] == 1)
			return p;
	}
	
	return end + 3;
}

const uint8_t *avc_find_startcode(const uint8_t *p, const uint8_t *end)
{
	const uint8_t *out= avc_find_startcode_internal(p, end);
	if (p<out && out<end && !out[-1])
		out--;
	return out;
}

const int avc_parse_nal_units(ByteIOContext *pb, const uint8_t *buf_in, int size)
{
	const uint8_t *p = buf_in;
	const uint8_t *end = p + size;
	const uint8_t *nal_start, *nal_end;
	
	size = 0;
	nal_start = avc_find_startcode(p, end);
	while (nal_start < end)
	{
		while (!*(nal_start++));
		nal_end = avc_find_startcode(nal_start, end);
		put_be32(pb, nal_end - nal_start);
		put_buffer(pb, nal_start, nal_end - nal_start);
		size += 4 + nal_end - nal_start;
		nal_start = nal_end;
	}
	return size;
}

const int avc_parse_nal_units_buf(const uint8_t *buf_in, uint8_t **buf, int *size)
{
	ByteIOContext *pb;
	int ret = url_open_dyn_buf(&pb);
	if (ret < 0)
		return ret;
	
	avc_parse_nal_units(pb, buf_in, *size);
	
	av_freep(buf);
	*size = url_close_dyn_buf(pb, buf);
	return 0;
}

const int isom_write_avcc(ByteIOContext *pb, const uint8_t *data, int len)
{
	// extradata from bytestream h264, convert to avcC atom data for bitstream
	if (len > 6)
	{
		/* check for h264 start code */
		if (VDA_RB32(data) == 0x00000001 || VDA_RB24(data) == 0x000001)
		{
			uint8_t *buf=NULL, *end, *start;
			uint32_t sps_size=0, pps_size=0;
			uint8_t *sps=0, *pps=0;
			
			int ret = avc_parse_nal_units_buf(data, &buf, &len);
			if (ret < 0)
				return ret;
			start = buf;
			end = buf + len;
			
			/* look for sps and pps */
			while (buf < end)
			{
				unsigned int size;
				uint8_t nal_type;
				size = VDA_RB32(buf);
				nal_type = buf[4] & 0x1f;
				if (nal_type == 7) /* SPS */
				{
					sps = buf + 4;
					sps_size = size;
				}
				else if (nal_type == 8) /* PPS */
				{
					pps = buf + 4;
					pps_size = size;
				}
				buf += size + 4;
			}
			assert(sps);
			assert(pps);
			
			put_byte(pb, 1); /* version */
			put_byte(pb, sps[1]); /* profile */
			put_byte(pb, sps[2]); /* profile compat */
			put_byte(pb, sps[3]); /* level */
			put_byte(pb, 0xff); /* 6 bits reserved (111111) + 2 bits nal size length - 1 (11) */
			put_byte(pb, 0xe1); /* 3 bits reserved (111) + 5 bits number of sps (00001) */
			
			put_be16(pb, sps_size);
			put_buffer(pb, sps, sps_size);
			put_byte(pb, 1); /* number of pps */
			put_be16(pb, pps_size);
			put_buffer(pb, pps, pps_size);
			av_free(start);
		}
		else
		{
			put_buffer(pb, data, len);
		}
	}
	return 0;
}

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
	
	m_iScreenWidth = 0;
	m_iScreenHeight = 0;
	
	m_iQueueDepth = 0;
	_displayQueue = NULL;
	
	m_convert_bytestream = false;
	m_DropPictures = false;
	
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
	
	UInt8 *extradata = (UInt8 *)hints.extradata;
	UInt32 extrasize = hints.extrasize;
	// the avcC data chunk from the bitstream must be present
	if (extrasize < 7 || extradata == NULL) 
	{
		CLog::Log(LOGERROR, "avcC atom cannot be NULL!");
		return FALSE;
	}
		// valid avcC atom data always starts with the value 1 (version)
	if ( extradata[0] != 1 )
	{
		if (extradata[0] == 0 && extradata[1] == 0 && extradata[2] == 0 && extradata[3] == 1)
		{
            // video content is from x264 or from bytestream h264 (AnnexB format)
            // NAL reformating to bitstream format needed
           ByteIOContext *pb;
            if (url_open_dyn_buf(&pb) < 0)
				return false;
			
            m_convert_bytestream = true;
            // create a valid avcC atom data from ffmpeg's extradata
            isom_write_avcc(pb, extradata, extrasize);
            // unhook from ffmpeg's extradata
            extradata = NULL;
            // extract the avcC atom data into extradata then write it into avcCData for VDADecoder
            extrasize = url_close_dyn_buf(pb, &extradata);
            // CFDataCreate makes a copy of extradata contents
            avcCData = CFDataCreate(kCFAllocatorDefault, (const uint8_t*)extradata, extrasize);
            // done with the converted extradata, we MUST free using av_free
            av_free(extradata);
		}
		else
		{
            CLog::Log(LOGNOTICE, "%s - invalid avcC atom data", __FUNCTION__);
            return false;
		}
	}
	else
	{
		// CFDataCreate makes a copy of extradata contents
		avcCData = CFDataCreate(kCFAllocatorDefault, (const uint8_t*)extradata, extrasize);
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
		CLog::Log(LOGERROR, "VDADecoderCreate failed. err: %d\n", (int)status);
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
	m_DropPictures = bDrop;
}

int CDVDVideoCodecVDA::Decode(BYTE* pData, int iSize, double pts)
{
	//CLog::Log(LOGDEBUG, "Queuing packet pts %.2f", pts);
	
	CFDictionaryRef frameInfo = NULL;
    OSStatus status = kVDADecoderNoErr;
	CFDataRef frameData = NULL;

	if (m_convert_bytestream)
	{
		// convert demuxer packet from bytestream (AnnexB) to bitstream
		ByteIOContext *pb;
		int demuxer_bytes;
		uint8_t *demuxer_content;
		
		if(url_open_dyn_buf(&pb) < 0)
			return VC_ERROR;
		demuxer_bytes = avc_parse_nal_units(pb, pData, iSize);
		demuxer_bytes = url_close_dyn_buf(pb, &demuxer_content);
		frameData = CFDataCreate(kCFAllocatorDefault, demuxer_content, demuxer_bytes);
		av_free(demuxer_content);
	}
	else
	{
		frameData = CFDataCreate(kCFAllocatorDefault, pData, iSize);
	}
	
    // create a dictionary containg some information about the frame being decoded
    // in this case, we pass in the display time aquired from the stream
    frameInfo = MakeDictionaryWithDisplayTime(pts);
	
    // ask the hardware to decode our frame, frameInfo will be retained and pased back to us
    // in the output callback for this frame
	uint32_t avc_flags = 0;
	if (m_DropPictures)
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
	if (!_displayQueue->frame) return VC_BUFFER;

	pDvdVideoPicture->iWidth = pDvdVideoPicture->iDisplayWidth = m_iPictureWidth;
	pDvdVideoPicture->iHeight = pDvdVideoPicture->iDisplayHeight = m_iPictureHeight;
	pDvdVideoPicture->pts = DVD_NOPTS_VALUE;
	
	// get reference to queue head
		
	pthread_mutex_lock(&_queueMutex);

	pDvdVideoPicture->pts = _displayQueue->frameDisplayTime;
	CVPixelBufferRef nextFrame = CVPixelBufferRetain(_displayQueue->frame);

	AdvanceQueueHead();

	pthread_mutex_unlock(&_queueMutex);

	CVPixelBufferLockBaseAddress(nextFrame, 0);

	// YUV420p frame
	for (int i = 0; i < 3; i++)
	{
		UInt32 planeHeight = CVPixelBufferGetHeightOfPlane(nextFrame, i);
		UInt32 planeWidth = CVPixelBufferGetBytesPerRowOfPlane(nextFrame, i);
		UInt32 planeSize = planeWidth * planeHeight;
		pDvdVideoPicture->iLineSize[i] = planeWidth;
		void *planePointer = CVPixelBufferGetBaseAddressOfPlane(nextFrame, i);
		
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
	CVPixelBufferUnlockBaseAddress(nextFrame, 0);
	CVPixelBufferRelease(nextFrame);

	pDvdVideoPicture->iRepeatPicture = 0;
	pDvdVideoPicture->iFlags = DVP_FLAG_ALLOCATED;    
	
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
