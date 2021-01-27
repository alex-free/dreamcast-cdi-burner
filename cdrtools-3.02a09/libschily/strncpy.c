/* @(#)strncpy.c	1.5 10/05/07 Copyright 2006-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)strncpy.c	1.5 10/05/07 Copyright 2006-2010 J. Schilling";
#endif
/*
 *	strncpy() to be used if missing in libc
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
#include <schily/libport.h>

#ifndef	HAVE_STRNCPY

EXPORT char *
strncpy(s1, s2, len)
	register char		*s1;
	register const char	*s2;
	register size_t		len;
{
	char	 *ret	= s1;

	if (len == 0)
		return (ret);

	do {
		if ((*s1++ = *s2++) == '\0')
			break;
	} while (--len > 0);
	if (len > 0) {
		while (--len > 0)
			*s1++ = '\0';
	}
	return (ret);
}
#endif	/* HAVE_STRNCPY */
