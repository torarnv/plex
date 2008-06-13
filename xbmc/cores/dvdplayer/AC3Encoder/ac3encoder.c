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
#include <stdio.h>

static const int acmod_to_ch[8] = { 2, 1, 2, 3, 3, 4, 4, 5 };

static const char *acmod_str[8] = {
    "dual mono (1+1)", "mono (1/0)", "stereo (2/0)",
    "3/0", "2/1", "3/1", "2/2", "3/2"
};

void ac3encoder_init(struct AC3Encoder *encoder, int iChannels, unsigned int uiSamplesPerSec)
{
	rb_init(&encoder->m_encodeBuffer, 3072 * 5); // store at most 5 AC3 packets
	
	aften_set_defaults(&encoder->m_aftenContext);
	
	encoder->m_aftenContext.channels = iChannels;
	encoder->m_aftenContext.samplerate = uiSamplesPerSec;
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
	 
	 channels->acmod
	 
	 free buffer*/
	
}

// returns number of available AC3 frames
int ac3encoder_write_samples(struct AC3Encoder *encoder, unsigned char *samples, int length)
{
	
	return 0;
}

int ac3encoder_get_encoded_frame(struct AC3Encoder *encoder, unsigned char *frame)
{
	// read first 1536 samples (size/numchannels)
	// encode with aften
	// dump into *frame - max 3072 char buffer
	// return len 
	return 0;
}
							 
void ac3encoder_flush(struct AC3Encoder *encoder)
{
							 
}
			
void ac3encoder_free(struct AC3Encoder *encoder)
{
	rb_free(encoder->m_encodeBuffer);
#warning need to flush?
	aften_encode_close(&encoder->m_aftenContext);
}
					
					

