/* @(#)wcseql.c	1.10 09/06/07 Copyright 1985, 1995-2009 J. Schilling */
/*
 *	Check if two wide strings are equal
 *
 *	Copyright (c) 1985, 1995-2009 J. Schilling
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

EXPORT int
wcseql(a, b)
	const wchar_t	*a;
	const wchar_t	*b;
{
	register const wchar_t	*s1 = a;
	register const wchar_t	*s2 = b;

	if (s1 == s2)
		return (TRUE);

#ifdef	__never__
	if (s1 == NULL || s2 ==  NULL)
		return (FALSE);
#endif

	while (*s1 == *s2++)
		if (*s1++ == '\0')
			return (TRUE);

	return (FALSE);
}
