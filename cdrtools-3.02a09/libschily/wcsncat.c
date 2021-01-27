/* @(#)wcsncat.c	1.3 10/05/06 Copyright 2006-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)wcsncat.c	1.3 10/05/06 Copyright 2006-2010 J. Schilling";
#endif
/*
 *	wcsncat() to be used if missing in libc
 *
 *	Copyright (c) 2006-2010 J. Schilling
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

#ifndef	HAVE_WCSNCAT

EXPORT wchar_t *
wcsncat(s1, s2, len)
	register wchar_t	*s1;
	register const wchar_t	*s2;
	register size_t		len;
{
	wchar_t	 *ret	= s1;


	if (len == 0)
		return (ret);

	while (*s1++ != '\0')
		;
	s1--;
	do {
		if ((*s1++ = *s2++) == '\0')
			return (ret);
	} while (--len > 0);
	*s1 = '\0';
	return (ret);
}
#endif	/* HAVE_WCSNCAT */
