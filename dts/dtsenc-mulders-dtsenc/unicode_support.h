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
#ifndef UNICODE_SUPPORT_H_INCLUDED
#define UNICODE_SUPPORT_H_INCLUDED

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#ifndef _INC_STAT
struct stat;
#endif
char *utf16_to_utf8(const wchar_t *input);
char *utf16_to_ansi(const wchar_t *input);
wchar_t *utf8_to_utf16(const char *input);
void init_commandline_arguments_utf8(int *argc, char ***argv);
void free_commandline_arguments_utf8(int *argc, char ***argv);
FILE *fopen_utf8(const char *filename_utf8, const char *mode_utf8);
int rename_utf8(const char *oldname_utf8, const char *newname_utf8);
char *path_utf8_to_ansi(const char *psz_filename_utf8, int b_create);
#else
#define fopen_utf8(NAME, MODE) fopen(NAME, MODE)
#define rename_utf8(OLD, NEW) rename(OLD, NEW)
#endif

#ifdef __cplusplus
}
#endif
#endif