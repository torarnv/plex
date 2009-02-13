/*
 PLEX
 ac3encoder.h  - AC3Encoder (header)

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
#pragma once
#include "aften-types.h"
#include "stdint.h"

#define ENCODER_OK 0
#define ENCODER_ERROR -1

//These values are forced to allow spdif out
#define SPDIF_SAMPLESIZE 16
#define SPDIF_CHANNELS 2
#define SPDIF_SAMPLERATE 48000
#define SPDIF_SAMPLE_BYTES SPDIF_CHANNELS * SPDIF_SAMPLESIZE / 8
#define AC3_SAMPLES_PER_FRAME 1536
#define AC3_SPDIF_FRAME_SIZE AC3_SAMPLES_PER_FRAME * SPDIF_SAMPLE_BYTES
#define AC3_BITRATE 640

struct AC3Encoder
{
	AftenContext m_aftenContext;
	int m_iSampleSize;

	// encoder flags
	int last_frame;
    int got_fs_once;
    int iAC3FrameSize;
    int irawSampleBytesRead;
	int remap;
};

int ac3encoderInit(struct AC3Encoder *encoder, uint32_t iChannels, uint32_t uiSamplesPerSec, uint32_t uiBitsPerSample, uint32_t remap);
int ac3encoderEncodePCM(struct AC3Encoder *encoder, uint8_t *pcmSamples, uint8_t *frameOutput, uint32_t sampleCount);
void ac3encoderReset(struct AC3Encoder *encoder);
void ac3encoderFinalise(struct AC3Encoder *encoder);
