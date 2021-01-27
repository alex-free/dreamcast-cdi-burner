/* @(#)getnum.c	1.6 09/07/08 Copyright 1984-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)getnum.c	1.6 09/07/08 Copyright 1984-2009 J. Schilling";
#endif
/*
 *	Number conversion routines to implement 'dd' like number options.
 *
 *	Copyright (c) 1984-2009 J. Schilling
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
#include <schily/utypes.h>
#include <schily/schily.h>

EXPORT	int	getnum		__PR((char *arg, long *valp));
EXPORT	int	getllnum	__PR((char *arg, Llong *lvalp));

LOCAL gnmult_t	nums[] = {
#ifdef	HAVE_LONGLONG
	{ 'e', 1024*1024*1024*1024LL*1024*1024, },
	{ 'E', 1024*1024*1024*1024LL*1024*1024, },
	{ 'p', 1024*1024*1024*1024LL*1024, },
	{ 'P', 1024*1024*1024*1024LL*1024, },
	{ 't', 1024*1024*1024*1024LL, },
	{ 'T', 1024*1024*1024*1024LL, },
#endif
	{ 'g', 1024*1024*1024, },
	{ 'G', 1024*1024*1024, },
	{ 'm', 1024*1024, },
	{ 'M', 1024*1024, },

	{ 'f', 2352, },
	{ 'F', 2352, },
	{ 's', 2048, },
	{ 'S', 2048, },

	{ 'k', 1024, },
	{ 'K', 1024, },
	{ 'b', 512, },
	{ 'B', 512, },
	{ 'w', 2, },
	{ 'W', 2, },
	{ '\0', 0, },
};

EXPORT int
getnum(arg, valp)
	char	*arg;
	long	*valp;
{
	return (getxnum(arg, valp, nums));
}

EXPORT int
getllnum(arg, lvalp)
	char	*arg;
	Llong	*lvalp;
{
	return (getllxnum(arg, lvalp, nums));
}
