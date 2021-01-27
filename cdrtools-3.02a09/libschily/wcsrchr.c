/* @(#)wcsrchr.c	1.2 09/06/05 Copyright 1985,2009 J. Schilling */
/*
 *	wcsrchr() to be used if missing in libc
 *
 *	Copyright (c) 1985,2009 J. Schilling
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
#include <schily/wchar.h>
#include <schily/schily.h>

#ifndef	HAVE_WCSRCHR

EXPORT wchar_t *
wcsrchr(s, c)
	register const wchar_t	*s;
	register	wchar_t	c;
{
	register wchar_t	*p = NULL;

	do {
		if (*s == c)
			p = (wchar_t *)s;
	} while (*s++ != '\0');
	return (p);
}

#endif
