/* @(#)wcsdup.c	1.7 10/08/21 Copyright 2003-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)wcsdup.c	1.7 10/08/21 Copyright 2003-2010 J. Schilling";
#endif
/*
 *	wcsdup() to be used if missing in libc
 *
 *	Copyright (c) 2003-2010 J. Schilling
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
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/wchar.h>
#include <schily/schily.h>

#ifndef	HAVE_WCSDUP

EXPORT wchar_t *
wcsdup(s)
	const wchar_t	*s;
{
	unsigned i	= wcslen(s) + 1;
	wchar_t	 *res	= malloc(i * sizeof (wchar_t));

	if (res == NULL)
		return (NULL);
	if (i > 16) {
		movebytes(s, res, (int)(i * sizeof (wchar_t)));
	} else {
		wchar_t	*s2 = res;

		while ((*s2++ = *s++) != '\0')
			;
	}
	return (res);
}
#endif	/* HAVE_WCSDUP */
