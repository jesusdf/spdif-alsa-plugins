/* 
 * This file is part of dcaenc.
 *
 * Copyright (c) 2008-2012 Alexander E. Patrakov <patrakov@gmail.com>
 *
 * dcaenc is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * dcaenc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with dcaenc; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "wavfile.h"
#include "unicode_support.h"

#ifdef _M_X64
#define SIZE2UINT32(X) (uint32_t)(((X) > UINT32_MAX) ? UINT32_MAX : (X))
#define SIZE2INT(X) (uint32_t)(((X) > INT_MAX) ? INT_MAX : (X))
#else
#define SIZE2UINT32(X) (X)
#define SIZE2INT(X) (X)
#endif

#define BUFFSIZE_SAMPLES 512
#define BUFFSIZE_BYTES (BUFFSIZE_SAMPLES * 6 * 4)

static const char *g_error_msg[13] =
{
	/* 0*/ "Success",
	/* 1*/ "Failed to open file",
	/* 2*/ "RIFF header not found",
	/* 3*/ "WAVE chunk not found",
	/* 4*/ "Format chunk not found",
	/* 5*/ "Failed to read Format chunk",
	/* 6*/ "Invalid or unsupported format tag",
	/* 7*/ "Unsupported number of channels (only 1, 2, 3, 4, 5 and 6)",
	/* 8*/ "Unsupported bits per sample (only 16 and 32 for now)",
	/* 9*/ "Inconsistent block alignment",
	/*10*/ "Inconsistent average bitrate",
	/*11*/ "Data chunk not found",
	/*12*/ "Data chunk size is invalid"
};

static int find_chunk(FILE * file, const uint8_t chunk_id[4], size_t *chunk_size)
{
	uint8_t buffer[8];
	*chunk_size = 0;
	
	while (1) {
		size_t chunksize;
		size_t s = fread(buffer, 1, 8, file);
		if (s < 8)
			return 0;
		chunksize = (uint32_t)buffer[4] | ((uint32_t)buffer[5] << 8) |
			((uint32_t)buffer[6] << 16) | ((uint32_t)buffer[7] << 24);
		
		if(!memcmp(buffer, chunk_id, 4))
		{
			*chunk_size = chunksize;
			return 1;
		}
		
		if((chunksize % 2) == 1) chunksize++; //Skip extra "unused" byte at the end of odd-size chunks

		if(fseek(file, SIZE2UINT32(chunksize), SEEK_CUR))
		{
			while(chunksize > 8)
			{
				s = fread(buffer, 1, 8, file);
				if (s < 8)
					return 0;
				chunksize -= 8;
			}
			s = fread(buffer, 1, chunksize, file);
			if (s < chunksize)
				return 0;
		}
	}
}

wavfile * wavfile_open(const char * filename, const char ** error_msg, const int ignore_len)
{
	wavfile *result;
	size_t s;
	uint8_t buffer[8];
	uint8_t *fmt;
	size_t v;
	uint32_t avg_bps;
	uint32_t block_align;
	static const uint8_t riff[4] = {'R', 'I', 'F', 'F'};
	static const uint8_t wave[4] = { 'W', 'A', 'V', 'E'};
	static const uint8_t fmt_[4] = {'f', 'm', 't', ' '};
	static const uint8_t data[4] = {'d', 'a', 't', 'a'};

	result = (wavfile *)calloc(1, sizeof(wavfile));
	if (!result)
		goto err0;
	
	result->file = strcmp(filename, "-") ? fopen_utf8(filename, "rb") : stdin;
	if (!result->file)
	{
		*error_msg = g_error_msg[1];
		goto err1;
	}

	s = fread(buffer, 1, 8, result->file);
	if (s < 8)
	{
		*error_msg = g_error_msg[2];
		goto err2;
	}

	if (memcmp(buffer, riff, 4))
	{
		*error_msg = g_error_msg[2];
		goto err2;
	}

	/* TODO: check size (in buffer[4..8]) */
	s = fread(buffer, 1, 4, result->file);
	if (s < 4)
	{
		*error_msg = g_error_msg[3];
		goto err2;
	}

	if (memcmp(buffer, wave, 4))
	{
		*error_msg = g_error_msg[3];
		goto err2;
	}

	if(!find_chunk(result->file, fmt_, &s))
	{
		*error_msg = g_error_msg[4];
		goto err2;
	}
	if((s < 16) || (s > 40))
	{
		*error_msg = g_error_msg[4];
		goto err2;
	}

	fmt = (uint8_t*)malloc(s);
	if(!fmt)
	{
		*error_msg = g_error_msg[5];
		goto err2;
	}

	if(fread(fmt, 1, s, result->file) != s)
	{
		*error_msg = g_error_msg[5];
		goto err3;
	}

	/* skip unused byte (for odd-size chunks) */
	if((s % 2) == 1)
	{
		char dummy[1];
		if(fread(&dummy, 1, 1,  result->file) != 1)
		{
			*error_msg = g_error_msg[5];
			goto err3;
		}
	}

	/* wFormatTag */
	v = (uint32_t)fmt[0] | ((uint32_t)fmt[1] << 8);
	if(v != 1 && v != 0xfffe)
	{
		*error_msg = g_error_msg[6];
		goto err3;
	}

	/* wChannels */
	v = (uint32_t)fmt[2] | ((uint32_t)fmt[3] << 8);
	if((v < 1) || (v > 6))
	{
		*error_msg = g_error_msg[7];
		goto err3;
	}
	result->channels = SIZE2UINT32(v);

	/* dwSamplesPerSec */
	result->sample_rate = (uint32_t)fmt[4] | ((uint32_t)fmt[5] << 8) |
		((uint32_t)fmt[6] << 16) | ((uint32_t)fmt[7] << 24);

	/* dwAvgBytesPerSec */
	avg_bps = (uint32_t)fmt[8] | ((uint32_t)fmt[9] << 8) |
		((uint32_t)fmt[10] << 16) | ((uint32_t)fmt[11] << 24);

	/* wBlockAlign */
	block_align = (uint32_t)fmt[12] | ((uint32_t)fmt[13] << 8);

	/* wBitsPerSample */
	result->bits_per_sample = (uint32_t)fmt[14] | ((uint32_t)fmt[15] << 8);
	if(result->bits_per_sample != 16 && result->bits_per_sample != 32)
	{
		*error_msg = g_error_msg[8];
		goto err3;
	}

	if(block_align != result->channels * (result->bits_per_sample / 8))
	{
		*error_msg = g_error_msg[9];
		goto err3;
	}

	if(avg_bps != block_align * result->sample_rate)
	{
		*error_msg = g_error_msg[10];
		goto err3;
	}

	if(!find_chunk(result->file, data, &v))
	{
		*error_msg = g_error_msg[11];
		goto err3;
	}
	if(((v == 0) || (v % block_align != 0)) && (!ignore_len))
	{
		*error_msg = g_error_msg[12];
		goto err3;
	}

	result->samples_left = SIZE2UINT32(ignore_len ? UNKNOWN_SIZE : (v / block_align));
	free(fmt);
	*error_msg = g_error_msg[0];
	return result;

	err3:
	free(fmt);
	err2:
	if(result->file != stdin) fclose(result->file);
	err1:
	free(result);
	err0:
	return NULL;
}

void wavfile_close(wavfile * f)
{
	if(f->file != stdin)
	{
		fclose(f->file);
	}
	free(f);
}

static int32_t get_s32_sample(const wavfile * f, const uint8_t *buffer, int sample, int channel)
{
	int offset = (f->bits_per_sample / 8) * (f->channels * sample + channel);
	uint32_t v;
	switch (f->bits_per_sample)
	{
	case 16:
		v = (uint32_t)buffer[offset + 0] | ((uint32_t)buffer[offset + 1] << 8);
		return v << 16;
		break;
	case 32:
		v = (uint32_t)buffer[offset + 0] | ((uint32_t)buffer[offset + 1] << 8) |
		((uint32_t)buffer[offset + 2] << 16) | ((uint32_t)buffer[offset + 3] << 24);
		return v;
		break;
	default:
		return 0;
	}
}

int wavfile_read_s32(wavfile * f, int32_t *samples, size_t sample_count)
{
	uint8_t buffer[BUFFSIZE_BYTES];
	int32_t smpte_sample[6];
	int samples_to_read;
	int bytes_to_read;
	int bytes_read;
	unsigned int i, ch;
	
	if(sample_count != BUFFSIZE_SAMPLES)
	{
		fprintf(stderr, "Only 512 samples currently supported!\n");
		return 0;
	}

	if(f->samples_left < 1)
	{
		return 0;
	}
	
	memset(buffer, 0, BUFFSIZE_BYTES);
	samples_to_read = (f->samples_left < BUFFSIZE_SAMPLES) ? f->samples_left : BUFFSIZE_SAMPLES;
	bytes_to_read = samples_to_read * f->channels * (f->bits_per_sample / 8);
	if(f->samples_left != UNKNOWN_SIZE) {
		f->samples_left -= samples_to_read;
	}
	
	bytes_read = SIZE2INT(fread(buffer, 1, bytes_to_read, f->file));
	if(bytes_read != bytes_to_read) {
		f->samples_left = 0;
	}
	
	for (i = 0; i < BUFFSIZE_SAMPLES; i++)
	{
		for (ch = 0; ch < f->channels; ch++)
			smpte_sample[ch] = get_s32_sample(f, buffer, i, ch);
		switch(f->channels)
		{
		case 1:
		case 2:
		case 4:
			for (ch = 0; ch < f->channels; ch++)
				*(samples++) = smpte_sample[ch];
			break;
		case 3:
			*(samples++) = smpte_sample[2];
			*(samples++) = smpte_sample[0];
			*(samples++) = smpte_sample[1];
			break;
		case 5:
			*(samples++) = smpte_sample[2];
			*(samples++) = smpte_sample[0];
			*(samples++) = smpte_sample[1];
			*(samples++) = smpte_sample[3];
			*(samples++) = smpte_sample[4];
			break;
		case 6:
			*(samples++) = smpte_sample[2];
			*(samples++) = smpte_sample[0];
			*(samples++) = smpte_sample[1];
			*(samples++) = smpte_sample[4];
			*(samples++) = smpte_sample[5];
			*(samples++) = smpte_sample[3];
			break;
		default:
			fprintf(stderr, "FIXME: Unexpected channel number!\n");
			exit(1);
		}
	}
	
	return bytes_read / (f->channels * (f->bits_per_sample / 8));
}
