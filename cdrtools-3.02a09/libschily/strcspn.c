/* @(#)strcspn.c	1.1 14/01/06 Copyright 2014 J. Schilling */
/*
 *	find maximum number of chars in s1 that consists solely of chars
 *	NOT in s2
 *
 *	Copyright (c) 2014 J. Schilling
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
#include <schily/schily.h>
#include <schily/string.h>

#ifndef	HAVE_STRCSPN

EXPORT size_t
strcspn(s1, s2)
	register const char	*s1;	/* The string to search */
		const char	*s2;	/* The charset used to search */
{
	register const char	*a;
	register const char	*b;

	if (*s2 == '\0')
		return (strlen(s1));

	for (a = s1; *a != '\0'; a++) {
		for (b = s2; *b != '\0' && *a != *b; b++)
			;
		if (*b != '\0')
			break;
	}
	return (a - s1);
}

#endif
