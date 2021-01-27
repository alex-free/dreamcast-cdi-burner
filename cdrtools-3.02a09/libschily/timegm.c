/* @(#)timegm.c	1.1 11/06/04 Copyright 2011 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)timegm.c	1.1 11/06/04 Copyright 2011 J. Schilling";
#endif
/*
 *	timegm() is a complement to timelocal()
 *
 *	Copyright (c) 2011 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/time.h>
#include <schily/schily.h>

EXPORT	time_t	timegm	__PR((struct tm *tp));

#ifndef	HAVE_TIMEGM
EXPORT time_t
timegm(tp)
	struct tm	*tp;
{
	return (mkgmtime(tp));
}
#endif
