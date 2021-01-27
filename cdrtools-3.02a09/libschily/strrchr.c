/* @(#)strrchr.c	1.2 09/06/07 Copyright 1985,2009 J. Schilling */
/*
 *	strrchr() to be used if missing in libc
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

#ifndef	HAVE_STRRCHR

EXPORT char *
strrchr(s, c)
	register const char	*s;
	register	int	c;
{
	register char		*p = NULL;

	do {
		if (*s == (char)c)
			p = (char *)s;
	} while (*s++ != '\0');
	return (p);
}

#endif
