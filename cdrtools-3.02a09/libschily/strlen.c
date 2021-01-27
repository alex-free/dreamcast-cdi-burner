/* @(#)strlen.c	1.3 09/06/06 Copyright 1985,2009 J. Schilling */
/*
 *	strlen() to be used if missing in libc
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
#include <schily/unistd.h>
#include <schily/schily.h>

#ifndef	HAVE_STRLEN

EXPORT size_t
strlen(s)
	register const char	*s;
{
	register const char	*rs = s;

	while (*rs++ != '\0')
		;
	return (--rs - s);
}

#endif
