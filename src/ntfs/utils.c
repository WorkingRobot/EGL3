/**
 * utils.c - Part of the Linux-NTFS project.
 *
 * Copyright (c) 2002-2005 Richard Russon
 * Copyright (c) 2003-2006 Anton Altaparmakov
 * Copyright (c) 2003 Lode Leroy
 * Copyright (c) 2005-2007 Yura Pakhuchiy
 * Copyright (c) 2014      Jean-Pierre Andre
 *
 * A set of shared functions for ntfs utilities
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the Linux-NTFS
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <locale.h>
#include <limits.h>
#include <ctype.h>

#include "utils.h"
#include "types.h"
#include "volume.h"
#include "debug.h"
#include "dir.h"
/* #include "version.h" */
#include "logging.h"
#include "misc.h"


/**
 * utils_set_locale
 */
int utils_set_locale(void)
{
	const char *locale;

	locale = setlocale(LC_ALL, "");
	if (!locale) {
		locale = setlocale(LC_ALL, NULL);
		ntfs_log_error("Failed to set locale, using default '%s'.\n",
				locale);
		return 1;
	} else {
		return 0;
	}
}

/**
 * linux-ntfs's ntfs_mbstoucs has different semantics, so we emulate it with
 * ntfs-3g's.
 */
int ntfs_mbstoucs_libntfscompat(const char *ins,
		ntfschar **outs, int outs_len)
{
	if(!outs) {
		errno = EINVAL;
		return -1;
	}
	else if(*outs != NULL) {
		/* Note: libntfs's mbstoucs implementation allows the caller to
		 * specify a preallocated buffer while libntfs-3g's always
		 * allocates the output buffer.
		 */
		ntfschar *tmpstr = NULL;
		int tmpstr_len;

		tmpstr_len = ntfs_mbstoucs(ins, &tmpstr);
		if(tmpstr_len >= 0) {
			if((tmpstr_len + 1) > outs_len) {
				/* Doing a realloc instead of reusing tmpstr
				 * because it emulates libntfs's mbstoucs more
				 * closely. */
				ntfschar *re_outs = realloc(*outs,
					sizeof(ntfschar)*(tmpstr_len + 1));
				if(!re_outs)
					tmpstr_len = -1;
				else
					*outs = re_outs;
			}

			if(tmpstr_len >= 0) {
				/* The extra character is the \0 terminator. */
				memcpy(*outs, tmpstr,
					sizeof(ntfschar)*(tmpstr_len + 1));
			}

			free(tmpstr);
		}

		return tmpstr_len;
	}
	else
		return ntfs_mbstoucs(ins, outs);
}

#ifdef HAVE_WINDOWS_H

/*
 *		Translate formats for older Windows
 *
 *	Up to Windows XP, msvcrt.dll does not support long long format
 *	specifications (%lld, %llx, etc). We have to translate them
 *	to %I64.
 */

char *ntfs_utils_reformat(char *out, int sz, const char *fmt)
{
	const char *f;
	char *p;
	int i;
	enum { F_INIT, F_PERCENT, F_FIRST } state;

	i = 0;
	f = fmt;
	p = out;
	state = F_INIT;
	while (*f && ((i + 3) < sz)) {
		switch (state) {
		case F_INIT :
			if (*f == '%')
				state = F_PERCENT;
			*p++ = *f++;
			i++;
			break;
		case F_PERCENT :
			if (*f == 'l') {
				state = F_FIRST;
				f++;
			} else {
				if (((*f < '0') || (*f > '9'))
				    && (*f != '*') && (*f != '-'))
					state = F_INIT;
				*p++ = *f++;
				i++;
			}
			break;
		case F_FIRST :
			if (*f == 'l') {
				*p++ = 'I';
				*p++ = '6';
				*p++ = '4';
				f++;
				i += 3;
			} else {
				*p++ = 'l';
				*p++ = *f++;
				i += 2;
			}
			state = F_INIT;
			break;
		}
	}
	*p++ = 0;
	return (out);
}

#endif
