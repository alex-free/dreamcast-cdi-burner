/* @(#)wcsncmp.c	1.4 10/05/06 Copyright 2006-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)wcsncmp.c	1.4 10/05/06 Copyright 2006-2010 J. Schilling";
#endif
/*
 *	wcsncmp() to be used if missing in libc
 *
 *	Copyright (c) 2006-2010 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/standard.h>
#include <schily/unistd.h>
#include <schily/wchar.h>
#include <schily/schily.h>

#ifndef	HAVE_WCSNCMP

EXPORT int
wcsncmp(s1, s2, len)
	register const wchar_t	*s1;
	register const wchar_t	*s2;
	register size_t		len;
{
	if (s1 == s2)
		return (0);

	if (len == 0)
		return (0);

	do {
		if (*s1 == *s2++) {
			if (*s1++ == '\0')
				return (0);
		} else {
			break;
		}
	} while (--len > 0);

	return (len == 0 ? 0 : *s1 - *(--s2));
}
#endif	/* HAVE_WCSNCMP */
