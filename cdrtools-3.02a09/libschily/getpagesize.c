/* @(#)getpagesize.c	1.5 09/07/10 Copyright 2001-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)getpagesize.c	1.5 09/07/10 Copyright 2001-2009 J. Schilling";
#endif
/*
 *	Copyright (c) 2001-2009 J. Schilling
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

#ifndef	HAVE_GETPAGESIZE
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/param.h>
#include <schily/libport.h>

#ifdef	HAVE_OS_H
#include <OS.h>		/* BeOS for B_PAGE_SIZE */
#endif

EXPORT	int	getpagesize	__PR((void));

EXPORT int
getpagesize()
{
	register int	pagesize;

#ifdef	_SC_PAGESIZE
	pagesize = sysconf(_SC_PAGESIZE);
#else	/* ! _SC_PAGESIZE */


#ifdef	PAGESIZE		/* Traditional UNIX page size from param.h */
	pagesize = PAGESIZE;

#else	/* ! PAGESIZE */

#ifdef	B_PAGE_SIZE		/* BeOS page size from OS.h */
	pagesize = B_PAGE_SIZE;

#else	/* ! B_PAGE_SIZE */

	pagesize = 512;
#endif	/* ! B_PAGE_SIZE */
#endif	/* ! PAGESIZE */
#endif	/* ! _SC_PAGESIZE */

	return (pagesize);
}

#endif	/* HAVE_GETPAGESIZE */
