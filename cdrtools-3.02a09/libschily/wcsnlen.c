/* @(#)wcsnlen.c	1.3 10/05/07 Copyright 2010 J. Schilling */
/*
 *	wcsnlen() to be used if missing in libc
 *
 *	Copyright (c) 2010 J. Schilling
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

#ifndef	HAVE_WCSNLEN

EXPORT size_t
wcsnlen(s, len)
	register const wchar_t	*s;
	register size_t		len;
{
	register const wchar_t	*rs = s;

	if (len == 0)
		return (0);

	while (*rs++ != '\0' && --len > 0)
			;
	if (len == 0)
		return (rs - s);
	return (--rs - s);
}

#endif
