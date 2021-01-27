/* @(#)gettnum.c	1.9 09/07/08 Copyright 1984-2002, 2004-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)gettnum.c	1.9 09/07/08 Copyright 1984-2002, 2004-2009 J. Schilling";
#endif
/*
 *	Generic time conversion routines rewritten from
 *	'dd' like number conversion in 'sdd'.
 *
 *	Copyright (c) 1984-2002, 2004-2009 J. Schilling
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
#include <schily/time.h>
#include <schily/schily.h>

#define	MINSECS		(60)
#define	HOURSECS	(60  * MINSECS)
#define	DAYSECS		(24  * HOURSECS)
#define	WEEKSECS	(7   * DAYSECS)
#define	YEARSECS	(365 * DAYSECS)		/* A non leap year */


EXPORT	int	gettnum		__PR((char *arg, time_t *valp));
EXPORT	int	getlltnum	__PR((char *arg, Llong *lvalp));

LOCAL gnmult_t	nums[] = {
	{ 'y', YEARSECS, },
	{ 'w', WEEKSECS, },
	{ 'd', DAYSECS, },
	{ 'h', HOURSECS, },
	{ 'm', MINSECS, },
	{ 's', 1, },
	{ '\0', 0, },
};

EXPORT int
gettnum(arg, valp)
	char	*arg;
	time_t	*valp;
{
	return (getxtnum(arg, valp, nums));
}

EXPORT int
getlltnum(arg, lvalp)
	char	*arg;
	Llong	*lvalp;
{
	return (getllxtnum(arg, lvalp, nums));
}
