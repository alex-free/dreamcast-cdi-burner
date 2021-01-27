/* @(#)wcsstr.c	1.3 13/11/11 Copyright 1985-2013 J. Schilling */
/*
 *	find string s2 in string s1
 *	return NULL if s2 is not found
 *	otherwise return pointer of first occurrence of s2 in s1
 *
 *	Copyright (c) 1985-2013 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */
#include <schily/standard.h>
#include <schily/wchar.h>
#include <schily/schily.h>

#ifndef	HAVE_WCSSTR

EXPORT wchar_t *
wcsstr(s1, s2)
	register const wchar_t	*s1;
		const wchar_t	*s2;
{
	register const wchar_t	*a;
	register const wchar_t	*b;

	if (*s2 == '\0')
		return ((wchar_t *)s1);

	for (; *s1 != '\0'; s1++) {
		for (a = s2, b = s1; *a == *b++; ) {
			if (*a++ == '\0')
				return ((wchar_t *)s1);
		}
		if (*a == '\0')
			return ((wchar_t *)s1);
	}
	return ((wchar_t *)NULL);
}

#endif
