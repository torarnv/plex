/*
   OSXBMC
   ac3eoncoder.c  - AC3Encoder
   
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

static const int acmod_to_ch[8] = { 2, 1, 2, 3, 3, 4, 4, 5 };

static const char *acmod_str[8] = {
    "dual mono (1+1)", "mono (1/0)", "stereo (2/0)",
    "3/0", "2/1", "3/1", "2/2", "3/2"
};


// call before using the encoder
// initialises the aften context and the I/O buffers
void ac3encoder_init(struct AC3Encoder *encoder, int iChannels, unsigned int uiSamplesPerSec, int uiBitsPerSample)
{
	rb_init(&encoder->m_inputBuffer, 2048 * 6 * 10); // store up to 10240 six-channel samples
	rb_init(&encoder->m_outputBuffer, AC3_SAMPLES_PER_FRAME * SPDIF_SAMPLESIZE / 8 * 5); // store up to 5 AC3 frames
	
	aften_set_defaults(&encoder->m_aftenContext);
	
	encoder->m_aftenContext.channels = iChannels;
	encoder->m_aftenContext.samplerate = uiSamplesPerSec;
	encoder->m_iSampleSize = uiBitsPerSample;
	//m_aftenContext.sample_format = 
	//#ifdef CONFIG_DOUBLE
	//		s.sample_format = A52_SAMPLE_FMT_DBL;
	//#else
	//		s.sample_format = A52_SAMPLE_FMT_FLT;
	
	// we have no way of knowing which LPCM channel is which, so just use best guess
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
			encoder->m_aftenContext.acmod = 6; // Quadraphonic with LFE - I don't know if this even exists
			encoder->m_aftenContext.lfe = 1;
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
	}
	else fprintf(stdout, "AC3 encoder initialised with configuration: %d Hz %s %s", 
				   encoder->m_aftenContext.samplerate, 
				   acmod_str[encoder->m_aftenContext.acmod], 
				   (encoder->m_aftenContext.lfe == 1) ? "+ LFE\n" : "\n");
	/*
	 
	 
	 struct ac3encoder
	 (ringbuffer, acmod, channels, will need some way to access portaudio) - could just encode and return struct = win
	 
	 channels->acmod*/
	
}

// returns number of available AC3 frames
int ac3encoder_write_samples(struct AC3Encoder *encoder, unsigned char *samples, int length)
{
	rb_write(encoder->m_inputBuffer, samples, length);
	
	while (ac3coder_get_PCM_samplecount(encoder) > AC3_SAMPLES_PER_FRAME)
	{
		// encode to output buffer
		encoder->irawSampleBytesRead = AC3_SAMPLES_PER_FRAME * SPDIF_CHANNELS * SPDIF_SAMPLESIZE / 8;
		unsigned char *sample_buffer;
		sample_buffer = calloc(1, encoder->irawSampleBytesRead);
		if (rb_read(encoder->m_inputBuffer, sample_buffer, encoder->irawSampleBytesRead) < encoder->irawSampleBytesRead)
		{
			free(sample_buffer);
			return 0;
		}
		
		//encode
		unsigned char AC3_frame_buffer[AC3_SPDIF_FRAME_SIZE];
		memset(&AC3_frame_buffer, 0, AC3_SPDIF_FRAME_SIZE);
		do 
		{
			memset(&AC3_frame_buffer, 0, AC3_SPDIF_FRAME_SIZE);
			encoder->iAC3FrameSize = aften_encode_frame(&encoder->m_aftenContext, AC3_frame_buffer, sample_buffer, AC3_SAMPLES_PER_FRAME);
			
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
		rb_write(m_outputBuffer, &AC3_frame_buffer, AC3_SPDIF_FRAME_SIZE);
		free(sample_buffer);
		
		// return len 
		//return encoder->iAC3FrameSize;
		
	}
	return ac3encoder_get_AC3_framecount(encoder);
}

// returns number of samples in input buffer
int ac3coder_get_PCM_samplecount(struct AC3Encoder *encoder)
{
	int samplecount = rb_data_size(encoder->m_inputBuffer) / encoder->m_aftenContext.channels / encoder->m_iSampleSize / 8;
	return samplecount;
}

// returns number of available AC3 frames
int ac3encoder_get_AC3_framecount(struct AC3Encoder *encoder)
{
	int available_frames = rb_data_size(encoder->m_outputBuffer) / (SPDIF_SAMPLESIZE / 8) / SPDIF_CHANNELS / AC3_SAMPLES_PER_FRAME;
	return available_frames;
}

int ac3encoder_get_encoded_frame(struct AC3Encoder *encoder, unsigned char *frame)
{
	// retunr number of frames
}
							 
int ac3encoder_channelcount(struct AC3Encoder *encoder)
{
	return encoder->m_aftenContext.channels;
}

void ac3encoder_flush(struct AC3Encoder *encoder)
{
    //while (encode_frame(..., NULL));
}
			
void ac3encoder_free(struct AC3Encoder *encoder)
{
	rb_free(encoder->m_encodeBuffer);
#warning need to flush?
	aften_encode_close(&encoder->m_aftenContext);
}
					
					

