/* @(#)wcscmp.c	1.6 09/06/07 Copyright 1985,2009 J. Schilling */
/*
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

#ifndef	HAVE_WCSCMP

EXPORT int
wcscmp(s1, s2)
	register const wchar_t	*s1;
	register const wchar_t	*s2;
{
	if (s1 == s2)
		return (0);

	while (*s1 == *s2++) {
		if (*s1++ == '\0')
			return (0);
	}
	return (*s1 - *(--s2));
}

#endif
