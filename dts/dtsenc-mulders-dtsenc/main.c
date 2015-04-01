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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#include "dcaenc.h"
#include "wavfile.h"
#include "unicode_support.h"
#include "xgetopt.h"
#include "compiler_info.h"

//extern const int32_t prototype_filter[512];

static char status[4] = {'|','/','-','\\'};
static const int AUTO_SELECT = -1;

#define BUFFSIZE_SPL 512
#define BUFFSIZE_CHN 6
#define BUFFSIZE_OUT 16384

static int dcaenc_main(int argc, char *argv[])
{
	dcaenc_context c;
	int32_t data[BUFFSIZE_SPL * BUFFSIZE_CHN];
	uint8_t output[BUFFSIZE_OUT];
	wavfile * f;
	FILE * outfile;
	const char *error_msg;
	unsigned int samples_total;
	unsigned int samples_read;
	unsigned int samples_read_total;
	unsigned int current_pos;
	double percent_done;
	int bitrate;
	int wrote;
	int counter;
	int status_idx;
	int show_ver;
	int show_help;
	int ignore_len;
	int enc_flags;
	int channel_config;
	int has_lfe;
	xgetopt_t opt;
	char t;
	char *file_input;
	char *file_output;

	static const int channel_map[6] = {DCAENC_CHANNELS_MONO, DCAENC_CHANNELS_STEREO, DCAENC_CHANNELS_3FRONT,
		DCAENC_CHANNELS_2FRONT_2REAR, DCAENC_CHANNELS_3FRONT_2REAR, DCAENC_CHANNELS_3FRONT_2REAR};
	
	fprintf(stderr, "%s-%s [%s]\n", PACKAGE_NAME, PACKAGE_VERSION, __DATE__);
	fprintf(stderr, "Copyright (c) 2008-2012 Alexander E. Patrakov <patrakov@gmail.com>\n\n");
	fprintf(stderr, "This program is free software: you can redistribute it and/or modify\n");
	fprintf(stderr, "it under the terms of the GNU General Public License <http://www.gnu.org/>.\n");
	fprintf(stderr, "Note that this program is distributed with ABSOLUTELY NO WARRANTY.\n\n");

	// ----------------------------

	file_input = NULL;
	file_output = NULL;
	bitrate = 0;
	enc_flags = DCAENC_FLAG_BIGENDIAN;
	channel_config = AUTO_SELECT;
	has_lfe = 0;
	show_ver = 0;
	ignore_len = 0;
	show_help = 0;

	memset(&opt, 0, sizeof(xgetopt_t));
	while((t = xgetopt(argc, argv, "i:o:b:c:fhlev", &opt)) != EOF)
	{
		switch(t)
		{
		case 'i':
			file_input = opt.optarg;
			break;
		case 'o':
			file_output = opt.optarg;
			break;
		case 'b':
			bitrate = (int)(atof(opt.optarg) * 1000.0f);
			if(bitrate > 6144000 || bitrate < 32000)
			{
				fprintf(stderr, "Bitrate must be between 32 and 6144 kbps!\n");
				return 1;
			}
			break;
		case 'c':
			channel_config = atoi(opt.optarg) - 1;
			if((channel_config < 0) || (channel_config > 15))
			{
				fprintf(stderr, "Bad channel configuration. Must be between 1 and 16!\n");
				return 1;
			}
			break;
		case 'f':
			has_lfe = 1;
			break;
		case 'h':
			show_help = 1;
			break;
		case 'l':
			ignore_len = 1;
			break;
		case 'e':
			enc_flags = enc_flags & (~DCAENC_FLAG_BIGENDIAN);
			break;
		case 'v':
			show_ver = 1;
			break;
		case '?':
			fprintf(stderr, "Unknown commandline option or missing argument: %s\n", argv[opt.optind-1]);
			return 1;
		}
	}
	
	// ----------------------------

	if(!file_input || !file_output || bitrate < 1 || show_ver || show_help)
	{
		if(show_ver)
		{
			fprintf(stderr, PACKAGE_NAME "-" PACKAGE_VERSION "\n");
			fprintf(stderr, "Compiled on " __DATE__ " at " __TIME__ " using " __COMPILER__ ".\n");
			fprintf(stderr, PACKAGE_URL "\n");
			return 0;
		}
		else if(show_help)
		{
			fprintf(stderr, "Usage:\n  dcaenc -i <input.wav> -o <output.dts> -b <bitrate_kbps>\n\n");
			fprintf(stderr, "Optional:\n");
			fprintf(stderr, "  -l  Ignore input length, can be useful when reading from stdin\n");
			fprintf(stderr, "  -e  Switch output endianess to Little Endian (default is: Big Endian)\n");
			fprintf(stderr, "  -h  Print the help screen that your are looking at right now\n");
			fprintf(stderr, "  -c  Overwrite the channel configuration (default is: Auto Selection)\n");
			fprintf(stderr, "  -f  Add an additional LFE channel (default: used for 6ch input)\n");
			fprintf(stderr, "  -v  Show version info\n\n");
			fprintf(stderr, "Remarks:\n");
			fprintf(stderr, "  * Input or output file name can be \"-\" for stdin/stdout.\n");
			fprintf(stderr, "  * The bitrate is specified in kilobits per second and may be rounded up.\n");
			fprintf(stderr, "      - Use Float value for bitrates that are not a multiple of 1 kbps.\n");
			fprintf(stderr, "  * Supported input sample rates:\n");
			fprintf(stderr, "      - 32000\n");
			fprintf(stderr, "      - 44100\n");
			fprintf(stderr, "      - 48000\n");
			fprintf(stderr, "      - or those divided by 2 or 4\n");
			fprintf(stderr, "  * Supported channel modes:\n");
			fprintf(stderr, "      -  1: MONO\n");
			fprintf(stderr, "      -  2: DUAL_MONO\n");
			fprintf(stderr, "      -  3: STEREO\n");
			fprintf(stderr, "      -  4: STEREO_SUMDIFF\n");
			fprintf(stderr, "      -  5: STEREO_TOTAL\n");
			fprintf(stderr, "      -  6: 3FRONT\n");
			fprintf(stderr, "      -  7: 2FRONT_1REAR\n");
			fprintf(stderr, "      -  8: 3FRONT_1REAR\n");
			fprintf(stderr, "      -  9: 2FRONT_2REAR\n");
			fprintf(stderr, "      - 10: 3FRONT_2REAR\n");
			fprintf(stderr, "      - 11: 4FRONT_2REAR\n");
			fprintf(stderr, "      - 12: 3FRONT_2REAR_1OV\n");
			fprintf(stderr, "      - 13: 3FRONT_3REAR\n");
			fprintf(stderr, "      - 14: 5FRONT_2REAR\n");
			fprintf(stderr, "      - 15: 4FRONT_4REAR\n");
			fprintf(stderr, "      - 16: 5FRONT_3REAR\n");
			fprintf(stderr, "  * Supported bitrates:\n");
			fprintf(stderr, "      - mono @ 8 kHz:        32-2048 kbps\n");
			fprintf(stderr, "      - mono @ 12 kHz:       48-3072 kbps\n");
			fprintf(stderr, "      - mono @ 16 kHz:       48-3842 kbps\n");
			fprintf(stderr, "      - mono @ 22.05 kHz:    65-3842 kbps\n");
			fprintf(stderr, "      - mono @ 24 kHz:       71-3842 kbps\n");
			fprintf(stderr, "      - mono @ 32 kHz:       95-3842 kbps\n");
			fprintf(stderr, "      - mono @ 44.1 kHz:    130-3842 kbps\n");
			fprintf(stderr, "      - mono @ 48 kHz:      142-3842 kbps\n");
			fprintf(stderr, "      - stereo @ 8 kHz:      96-2048 kbps\n");
			fprintf(stderr, "      - stereo @ 12 kHz:     96-3842 kbps\n");
			fprintf(stderr, "      - stereo @ 16 kHz:     96-3842 kbps\n");
			fprintf(stderr, "      - stereo @ 22.05 kHz: 128-3842 kbps\n");
			fprintf(stderr, "      - stereo @ 24 kHz:    192-3842 kbps\n");
			fprintf(stderr, "      - stereo @ 32 kHz:    192-3842 kbps\n");
			fprintf(stderr, "      - stereo @ 44.1 kHz:  256-3842 kbps\n");
			fprintf(stderr, "      - stereo @ 48 kHz:    271-3842 kbps\n");
			fprintf(stderr, "      - 5.1 @ 8 kHz:        112-2048 kbps\n");
			fprintf(stderr, "      - 5.1 @ 12 kHz:       168-3072 kbps\n");
			fprintf(stderr, "      - 5.1 @ 16 kHz:       224-3842 kbps\n");
			fprintf(stderr, "      - 5.1 @ 22.05 kHz:    308-3842 kbps\n");
			fprintf(stderr, "      - 5.1 @ 32 kHz:       447-3842 kbps\n");
			fprintf(stderr, "      - 5.1 @ 44.1 kHz:     615-3842 kbps\n");
			fprintf(stderr, "      - 5.1 @ 48 kHz:       670-3842 kbps\n");

			return 0;
		}
		else
		{
			fprintf(stderr, "Required arguments are missing. Use '-h' option for help!\n");
			return 1;
		}
	}

	fprintf(stderr, "Source: %s\n", file_input);
	fprintf(stderr, "Output: %s\n", file_output);
	fprintf(stderr, "KBit/s: %d.%03d\n\n", bitrate / 1000, bitrate % 1000);

	// ----------------------------

	f = wavfile_open(file_input, &error_msg, ignore_len);
	if (!f) {
		fprintf(stderr, "Could not open or parse \"%s\".\n", file_input);
		fprintf(stderr, "Error: %s!\n", error_msg);
		return 1;
	}
	
	samples_total = f->samples_left;

	if(channel_config == AUTO_SELECT)
	{
		channel_config = channel_map[f->channels - 1];
		if(f->channels == 6) has_lfe = 1;
	}

	if(has_lfe)
	{
		enc_flags = enc_flags | DCAENC_FLAG_LFE;
	}

	switch(f->channels - (has_lfe ? 1 : 0))
	{
	case 1:
		if(!(channel_config == DCAENC_CHANNELS_MONO))
		{
			fprintf(stderr, "Invalid channel configuration for input audio!\n");
			return 1;
		}
		break;
	case 2:
		if(!(channel_config == DCAENC_CHANNELS_DUAL_MONO || channel_config == DCAENC_CHANNELS_STEREO ||
			channel_config == DCAENC_CHANNELS_STEREO_SUMDIFF || channel_config == DCAENC_CHANNELS_STEREO_TOTAL))
		{
			fprintf(stderr, "Invalid channel configuration for input audio!\n");
			return 1;
		}
		break;
	case 4:
		if(!(channel_config == DCAENC_CHANNELS_3FRONT_1REAR || channel_config == DCAENC_CHANNELS_2FRONT_2REAR))
		{
			fprintf(stderr, "Invalid channel configuration for input audio!\n");
			return 1;
		}
		break;
	case 5:
		if(!(channel_config == DCAENC_CHANNELS_3FRONT_2REAR))
		{
			fprintf(stderr, "Invalid channel configuration for input audio!\n");
			return 1;
		}
		break;
	case 6:
		if(!(channel_config == DCAENC_CHANNELS_3FRONT_3REAR || channel_config == DCAENC_CHANNELS_4FRONT_2REAR ||
			 channel_config == DCAENC_CHANNELS_3FRONT_2REAR_1OV))
		{
			fprintf(stderr, "Invalid channel configuration for input audio!\n");
			return 1;
		}
		break;
	}

	// ----------------------------

	c = dcaenc_create(f->sample_rate, channel_config, bitrate, enc_flags);
	
	if (!c) {
		fprintf(stderr, "Insufficient bitrate or unsupported sample rate!\n");
		return 1;
	}
	
	if((((size_t)(dcaenc_output_size(c))) > BUFFSIZE_OUT) || (((size_t)(dcaenc_input_size(c))) > BUFFSIZE_SPL))
	{
		fprintf(stderr, "Internal error, buffers are too small!\n", file_output);
		return 1;
	}
	
	outfile = strcmp(file_output, "-") ? fopen_utf8(file_output, "wb") : stdout;
	if(!outfile) {
		fprintf(stderr, "Could not open \"%s\" for writing!\n", file_output);
		return 1;
	}
	
	fflush(stdout);
	fflush(stderr);
	
	// ----------------------------

	counter = 0;
	samples_read_total = 0;
	status_idx = 0;
	
	while(samples_read = wavfile_read_s32(f, data, BUFFSIZE_SPL))
	{
		samples_read_total += samples_read;
		wrote = dcaenc_convert_s32(c, data, output);
		fwrite(output, 1, wrote, outfile);
		
		if(counter == 0)
		{
			current_pos = samples_read_total / f->sample_rate;
			
			if((samples_total > 0) && (samples_total < UNKNOWN_SIZE))
			{
				percent_done = ((double)(samples_total - f->samples_left)) / ((double)(samples_total));
				fprintf(stderr, "Encoding... %d:%02d [%3.1f%%]\r", current_pos / 60, current_pos % 60, percent_done * 100.0);
				fflush(stderr);
			}
			else
			{
				fprintf(stderr, "Encoding... %d:%02d [%c]\r", current_pos / 60, current_pos % 60, status[(status_idx = (status_idx+1) % 4)]);
				fflush(stderr);
			}
		}
		
		counter = (counter+1) % 125;
	}
	
	fprintf(stderr, "Encoding... %d:%02d [%3.1f%%]\n", (samples_read_total / f->sample_rate) / 60, (samples_read_total / f->sample_rate) % 60, 100.0);
	fflush(stderr);

	wrote = dcaenc_destroy(c, output);
	fwrite(output, 1, wrote, outfile);
	if(outfile != stdout)
	{
		fclose(outfile);
	}
	wavfile_close(f);

	fprintf(stderr, "Done.\n");
	return 0;
}

#ifdef _WIN32

#include <Windows.h>
#include <io.h>
#include <fcntl.h>

int main( int argc, char **argv )
{
	int dcaenc_argc;
	char **dcaenc_argv;
	UINT old_cp;
	int exit_code;

	_setmode(_fileno(stdin),  _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);

	old_cp = GetConsoleOutputCP();
	SetConsoleOutputCP(CP_UTF8);

	init_commandline_arguments_utf8(&dcaenc_argc, &dcaenc_argv);
	exit_code = dcaenc_main(dcaenc_argc, dcaenc_argv);
	free_commandline_arguments_utf8(&dcaenc_argc, &dcaenc_argv);

	SetConsoleOutputCP(old_cp);
	return exit_code;
}

#else //_WIN32

int main( int argc, char **argv )
{
	return dcaenc_main(argc, argv);
}

#endif //_WIN32