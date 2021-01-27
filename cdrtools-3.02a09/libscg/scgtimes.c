/* @(#)scgtimes.c	1.4 09/07/11 Copyright 1995-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)scgtimes.c	1.4 09/07/11 Copyright 1995-2009 J. Schilling";
#endif
/*
 *	SCSI user level command timing
 *
 *	Copyright (c) 1995-2009 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * The following exceptions apply:
 * CDDL §3.6 needs to be replaced by: "You may create a Larger Work by
 * combining Covered Software with other code if all other code is governed by
 * the terms of a license that is OSI approved (see www.opensource.org) and
 * you may distribute the Larger Work as a single product. In such a case,
 * You must make sure the requirements of this License are fulfilled for
 * the Covered Software."
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/standard.h>
#include <schily/time.h>
#include <schily/schily.h>

#include <scg/scsitransp.h>
#include "scgtimes.h"

EXPORT	void	__scg_times	__PR((SCSI *scgp));

/*
 * We don't like to make this a public interface to prevent bad users
 * from making our timing incorrect.
 */
EXPORT void
__scg_times(scgp)
	SCSI	*scgp;
{
	struct timeval	*stp = scgp->cmdstop;

	gettimeofday(stp, (struct timezone *)0);
	stp->tv_sec -= scgp->cmdstart->tv_sec;
	stp->tv_usec -= scgp->cmdstart->tv_usec;
	while (stp->tv_usec < 0) {
		stp->tv_sec -= 1;
		stp->tv_usec += 1000000;
	}
}
