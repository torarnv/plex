/*
 OSXBMC
 ac3encoder.c  - AC3Encoder

 Copyright (c) 2008, Ryan Walklin (ryanwalklin@gmail.com)

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public Licensse as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 */


#include "ac3encoder.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "aften.h"


// private interface
void ac3encoderReset(struct AC3Encoder *encoder); // primes decoder with blank frame


static const int acmod_to_ch[8] = { 2, 1, 2, 3, 3, 4, 4, 5 };

static const char *acmod_str[8] = {
"dual mono (1+1)", "mono (1/0)", "stereo (2/0)",
"3/0", "2/1", "3/1", "2/2", "3/2"
};

static inline int swabdata(char* dst, char* src, int size)
{
	if( size & 0x1 )
	{
		swab(src, dst, size-1);
		dst+=size-1;
		src+=size-1;

		dst[0] = 0x0;
		dst[1] = src[0];
		return size+1;
	}
	else
	{
		swab(src, dst, size);
		return size;
	}

}

// call before using the encoder
// initialises the aften context
int ac3encoderInit(struct AC3Encoder *encoder, uint32_t iChannels, uint32_t uiSamplesPerSec, uint32_t uiBitsPerSample, uint32_t remap)
{
	aften_set_defaults(&encoder->m_aftenContext);

	encoder->m_aftenContext.channels = iChannels;
	encoder->m_aftenContext.samplerate = uiSamplesPerSec;
	encoder->m_iSampleSize = uiBitsPerSample;
	encoder->remap = remap;

	encoder->m_aftenContext.params.bitrate = AC3_BITRATE; // set AC3 output bitrate to maximum
	//encoder->m_aftenContext.params.expstr_search = 32;
	encoder->m_aftenContext.params.quality = 1023;
	encoder->m_aftenContext.params.bwcode = -2;
	encoder->m_aftenContext.system.n_threads= 1;

	encoder->m_aftenContext.acmod = -1;
	encoder->m_aftenContext.lfe = 0;
	switch (iChannels)
	{
		case 1:
			encoder->m_aftenContext.acmod = 1; // mono
			break;
		case 2:
			encoder->m_aftenContext.acmod = 2; // stereo
			break;
		case 3:
			encoder->m_aftenContext.acmod = 2; // stereo with LFE, ie Stereo/Prologic 2.1
			encoder->m_aftenContext.lfe = 1;
			break;
		case 4:
			encoder->m_aftenContext.acmod = 6; // Quadraphonic, ie 2/2
			break;
		case 5:
			encoder->m_aftenContext.acmod = 7; // 3F2R without LFE
			encoder->m_aftenContext.lfe = 0;
			break;
		case 6:
			encoder->m_aftenContext.acmod = 7;
			encoder->m_aftenContext.lfe = 1;
			break;
		default:
			fprintf(stderr, "Invalid number of channels for AC3 (%i)\n", iChannels);
			break;
	}

	encoder->last_frame = 0;
    encoder->got_fs_once = 0;
    encoder->iAC3FrameSize = 0;
    encoder->irawSampleBytesRead = 0;

	// print ac3 info to console

	if (aften_encode_init(&encoder->m_aftenContext))
	{
		fprintf(stderr, "Error initialising AC3 encoder\n");
		aften_encode_close(&encoder->m_aftenContext);
		return -1;
	}
	else fprintf(stdout, "AC3 encoder initialised with configuration: %dHz %s %s",
				 encoder->m_aftenContext.samplerate,
				 acmod_str[encoder->m_aftenContext.acmod],
				 (encoder->m_aftenContext.lfe == 1) ? "+ LFE\n" : "\n");

	return 0;
}

// returns number of available AC3 frames
int ac3encoderEncodePCM(struct AC3Encoder *encoder, uint8_t *pcmSamples, uint8_t *frameOutput, uint32_t sampleCount)
{
	// sanity
	if (sampleCount > AC3_SAMPLES_PER_FRAME)
	{
		fprintf(stderr, "Tried to encode too many samples (%i)\n", sampleCount);
		return ENCODER_ERROR;
	}
	
	// allocate and copy to mutable input buffer
	uint32_t pcmBufferSize = sampleCount * encoder->m_aftenContext.channels * encoder->m_iSampleSize / 8;
	uint8_t *pcmBuffer = calloc(1, pcmBufferSize);

	
	memcpy(pcmBuffer, pcmSamples, pcmBufferSize); // any difference between sampleCount and AC3_SAMPLES_PER_FRAME encoded as silence
	
	// set up MPEG channel remapping
	void (*aften_remap)(void *samples, int n, int ch,
                        A52SampleFormat fmt, int acmod) = NULL;

	if(encoder->remap == 1)
        aften_remap = aften_remap_mpeg_to_a52;
    else
        aften_remap = aften_remap_wav_to_a52;
	
	
	// remap sample buffer
	if(aften_remap)
		aften_remap(pcmBuffer, sampleCount, encoder->m_aftenContext.channels, encoder->m_aftenContext.sample_format, encoder->m_aftenContext.acmod);
	
	// pre-set encoder parameters
	encoder->irawSampleBytesRead = AC3_SAMPLES_PER_FRAME * encoder->m_aftenContext.channels * encoder->m_iSampleSize / 8;

	//encode frame
	uint8_t AC3framebuffer[AC3_SPDIF_FRAME_SIZE], oldendian[AC3_SPDIF_FRAME_SIZE];
	do
	{
		memset(AC3framebuffer, 0, AC3_SPDIF_FRAME_SIZE);
		encoder->iAC3FrameSize = aften_encode_frame(&encoder->m_aftenContext, (uint8_t *)&oldendian, pcmBuffer, AC3_SAMPLES_PER_FRAME);
		
		
		// note this assumes a little-endian system and transforms to big-endian for SPDIF transport
		swabdata((char*)AC3framebuffer+8, (char*)oldendian, encoder->iAC3FrameSize);
		
		AC3framebuffer[0] = 0x72; /* sync words */
		AC3framebuffer[1] = 0xF8;
		AC3framebuffer[2] = 0x1F;
		AC3framebuffer[3] = 0x4E;
		AC3framebuffer[4] = 0x01;
		AC3framebuffer[5] = oldendian[5] & 0x7; /* bsmod */
		AC3framebuffer[6] = (encoder->iAC3FrameSize << 3) & 0xFF;
		AC3framebuffer[7] = (encoder->iAC3FrameSize >> 5) & 0xFF;
		
		if(encoder->iAC3FrameSize < 0)
		{
			fprintf(stderr, "Error encoding frame\n");
			break;
		}
		else
		{
			if (encoder->iAC3FrameSize > 0)
			{
				encoder->got_fs_once = 1;// means encoder started outputting samples.
			}
			encoder->last_frame = encoder->irawSampleBytesRead;
		}
	} while (!encoder->got_fs_once);
	
	// return to caller
	memcpy(frameOutput, &AC3framebuffer, encoder->iAC3FrameSize+8);

	// tidy up
	free(pcmBuffer);
	return encoder->iAC3FrameSize+8;
}

void ac3encoderReset(struct AC3Encoder *encoder)
{
	// write one silent frame
	uint8_t *silent_PCM_init = (uint8_t *)calloc(1, encoder->m_aftenContext.channels * encoder->m_iSampleSize / 8 * AC3_SAMPLES_PER_FRAME); // 1536 silent samples
	uint8_t encoder_output[AC3_SPDIF_FRAME_SIZE];
	if (ac3encoderEncodePCM(encoder, silent_PCM_init, (uint8_t *)&encoder_output, AC3_SAMPLES_PER_FRAME) < AC3_SAMPLES_PER_FRAME)
	{
		fprintf(stderr, "Error pre-buffering AC3 encoder\n");
		aften_encode_close(&encoder->m_aftenContext);
	}
	free(silent_PCM_init);
    //while (encode_frame(..., NULL));
}

void ac3encoderFinalise(struct AC3Encoder *encoder)
{
	#warning need to flush?
	aften_encode_close(&encoder->m_aftenContext);
}



