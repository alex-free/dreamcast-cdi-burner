/* @(#)wcscpy.c	1.2 09/07/08 Copyright 2006-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)wcscpy.c	1.2 09/07/08 Copyright 2006-2009 J. Schilling";
#endif
/*
 *	wcscpy() to be used if missing in libc
 *
 *	Copyright (c) 2006-2009 J. Schilling
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

#ifndef	HAVE_WCSCPY

EXPORT wchar_t *
wcscpy(s1, s2)
	register wchar_t	*s1;
	register const wchar_t	*s2;
{
	wchar_t	 *ret	= s1;

	while ((*s1++ = *s2++) != '\0')
		;
	return (ret);
}
#endif	/* HAVE_WCSCPY */
