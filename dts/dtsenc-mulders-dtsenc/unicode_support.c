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
#include "unicode_support.h"

#ifdef _WIN32

#include <windows.h>
#include <sys/stat.h>
#include <stdio.h>
#include <wchar.h>

char *utf16_to_utf8(const wchar_t *input)
{
	char *Buffer;
	int BuffSize = 0, Result = 0;

	BuffSize = WideCharToMultiByte(CP_UTF8, 0, input, -1, NULL, 0, NULL, NULL);
	Buffer = (char*) malloc(sizeof(char) * BuffSize);
	if(Buffer)
	{
		Result = WideCharToMultiByte(CP_UTF8, 0, input, -1, Buffer, BuffSize, NULL, NULL);
	}

	return ((Result > 0) && (Result <= BuffSize)) ? Buffer : NULL;
}

char *utf16_to_ansi(const wchar_t *input)
{
	char *Buffer;
	int BuffSize = 0, Result = 0;

	BuffSize = WideCharToMultiByte(CP_ACP, 0, input, -1, NULL, 0, NULL, NULL);
	Buffer = (char*) malloc(sizeof(char) * BuffSize);
	if(Buffer)
	{
		Result = WideCharToMultiByte(CP_ACP, 0, input, -1, Buffer, BuffSize, NULL, NULL);
	}

	return ((Result > 0) && (Result <= BuffSize)) ? Buffer : NULL;
}

wchar_t *utf8_to_utf16(const char *input)
{
	wchar_t *Buffer;
	int BuffSize = 0, Result = 0;

	BuffSize = MultiByteToWideChar(CP_UTF8, 0, input, -1, NULL, 0);
	Buffer = (wchar_t*) malloc(sizeof(wchar_t) * BuffSize);
	if(Buffer)
	{
		Result = MultiByteToWideChar(CP_UTF8, 0, input, -1, Buffer, BuffSize);
	}

	return ((Result > 0) && (Result <= BuffSize)) ? Buffer : NULL;
}

void init_commandline_arguments_utf8(int *argc, char ***argv)
{
	int i, nArgs;
	LPWSTR *szArglist;

	szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);

	if(NULL == szArglist)
	{
		fprintf(stderr, "\nFATAL: CommandLineToArgvW failed\n\n");
		exit(-1);
	}

	*argv = (char**) malloc(sizeof(char*) * nArgs);
	*argc = nArgs;

	if(NULL == *argv)
	{
		fprintf(stderr, "\nFATAL: Malloc failed\n\n");
		exit(-1);
	}
	
	for(i = 0; i < nArgs; i++)
	{
		(*argv)[i] = utf16_to_utf8(szArglist[i]);
		if(NULL == (*argv)[i])
		{
			fprintf(stderr, "\nFATAL: utf16_to_utf8 failed\n\n");
			exit(-1);
		}
	}

	LocalFree(szArglist);
}

void free_commandline_arguments_utf8(int *argc, char ***argv)
{
	int i = 0;
	
	if(*argv != NULL)
	{
		for(i = 0; i < *argc; i++)
		{
			if((*argv)[i] != NULL)
			{
				free((*argv)[i]);
				(*argv)[i] = NULL;
			}
		}
		free(*argv);
		*argv = NULL;
	}
}

FILE *fopen_utf8(const char *filename_utf8, const char *mode_utf8)
{
	FILE *ret = NULL;
	wchar_t *filename_utf16 = utf8_to_utf16(filename_utf8);
	wchar_t *mode_utf16 = utf8_to_utf16(mode_utf8);
	
	if(filename_utf16 && mode_utf16)
	{
		ret = _wfopen(filename_utf16, mode_utf16);
	}

	if(filename_utf16) free(filename_utf16);
	if(mode_utf16) free(mode_utf16);

	return ret;
}

/* int stat_utf8(const char *path_utf8, struct stat *buf)
{
	int ret = -1;
	
	wchar_t *path_utf16 = utf8_to_utf16(path_utf8);
	if(path_utf16)
	{
		ret = wstat(path_utf16, buf);
		free(path_utf16);
	}
	
	return ret;
} */

int rename_utf8(const char *oldname_utf8, const char *newname_utf8)
{
	int ret = -1;
	
	wchar_t *oldname_utf16 = utf8_to_utf16(oldname_utf8);
	wchar_t *newname_utf16 = utf8_to_utf16(newname_utf8);
	
	if(oldname_utf16 && newname_utf16)
	{
		_wunlink(newname_utf16);
		ret = _wrename(oldname_utf16, newname_utf16);
	}

	if(newname_utf16) free(newname_utf16);
	if(oldname_utf16) free(oldname_utf16);

	return ret;
}

char *path_utf8_to_ansi(const char *psz_filename_utf8, int b_create)
{
	char *short_path = NULL;
	wchar_t *psz_filename_utf16 = NULL;

	if(!psz_filename_utf8)
	{
		return NULL;
	}
	
	psz_filename_utf16 = utf8_to_utf16(psz_filename_utf8);
	if(psz_filename_utf16)
	{
		char *psz_filename_ansi = NULL;
		if(b_create)
		{
			FILE *fh = _wfopen(psz_filename_utf16, L"a+");
			if(fh) fclose(fh);
		}
		psz_filename_ansi = utf16_to_ansi(psz_filename_utf16);
		if(psz_filename_ansi)
		{
			if(strcmp(psz_filename_ansi, psz_filename_utf8))
			{
				wchar_t short_name_utf16[MAX_PATH];
				DWORD result = GetShortPathNameW(psz_filename_utf16, short_name_utf16, MAX_PATH);
				if((result > 0) && (result < MAX_PATH))
				{
					short_path = utf16_to_ansi(short_name_utf16);
					free(psz_filename_ansi);
				}
				else
				{
					short_path = psz_filename_ansi; /*if GetShortPathName() failed, use the ANSI version*/
				}
			}
			else
			{
				short_path = psz_filename_ansi; /*don't convert plain ASCII filenames to "short" path*/
			}
		}
		free(psz_filename_utf16);
	}

	return short_path;
}

#endif //_WIN32
