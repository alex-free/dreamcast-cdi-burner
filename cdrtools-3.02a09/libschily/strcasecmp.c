/* @(#)strcasecmp.c	1.1 11/11/24 Copyright 2011 J. Schilling */
/*
 *	strcasecmp() to be used if missing in libc
 *
 *	Copyright (c) 2011 J. Schilling
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
#include <schily/string.h>
#include <schily/schily.h>

#ifndef	HAVE_STRCASECMP

EXPORT	int	strcasecmp	__PR((const char *s1, const char *s2));

EXPORT int
strcasecmp(s1, s2)
	register const char	*s1;
	register const char	*s2;
{
extern	const char	js_strcase_charmap[];
	register const char	*chm = js_strcase_charmap;

	if (s1 == s2)
		return (0);

	while (chm[* (const unsigned char *)s1] ==
		chm[* (const unsigned char *)s2++]) {
		if (*s1++ == '\0')
			return (0);
	}
	return (chm[*(unsigned char *)s1] - chm[*(unsigned char *)(--s2)]);
}

#endif
