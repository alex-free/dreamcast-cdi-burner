/* @(#)strcmp.c	1.6 09/06/07 Copyright 1985,2009 J. Schilling */
/*
 *	strcmp() to be used if missing in libc
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
#include <schily/schily.h>

#ifndef	HAVE_STRCMP

EXPORT int
strcmp(s1, s2)
	register const char	*s1;
	register const char	*s2;
{
	if (s1 == s2)
		return (0);

	while (*s1 == *s2++) {
		if (*s1++ == '\0')
			return (0);
	}
	return (*(unsigned char *)s1 - *(unsigned char *)(--s2));
}

#endif
