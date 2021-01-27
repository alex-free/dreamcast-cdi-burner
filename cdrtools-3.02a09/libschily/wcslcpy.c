/* @(#)wcslcpy.c	1.5 09/07/08 Copyright 2006-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)wcslcpy.c	1.5 09/07/08 Copyright 2006-2009 J. Schilling";
#endif
/*
 *	wcslcpy() to be used if missing in libc
 *
 *	Copyright (c) 2006-2009 J. Schilling
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

#ifndef	HAVE_WCSLCPY

EXPORT size_t
wcslcpy(s1, s2, len)
	register wchar_t	*s1;
	register const wchar_t	*s2;
	register size_t		len;
{
	const wchar_t		 *os2	= s2;

	if (len > 0) {
		while (--len > 0 && (*s1++ = *s2++) != '\0')
			;
		if (len == 0) {
			*s1 = '\0';
			while (*s2++ != '\0')
				;
		}
	} else {
		while (*s2++ != '\0')
			;
	}
	return (--s2 - os2);
}
#endif	/* HAVE_WCSLCPY */
