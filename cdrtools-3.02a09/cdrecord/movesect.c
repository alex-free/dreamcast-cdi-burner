/* @(#)movesect.c	1.6 09/07/05 Copyright 2001, 2004-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)movesect.c	1.6 09/07/05 Copyright 2001, 2004-2009 J. Schilling";
#endif
/*
 *	Copyright (c) 2001, 2004-2009 J. Schilling
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
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/schily.h>

#include "cdrecord.h"
#include "movesect.h"

EXPORT	void	scatter_secs	__PR((track_t *trackp, char *bp, int nsecs));

/*
 * Scatter input sector size records over buffer to make them
 * output sector size.
 *
 * If input sector size is less than output sector size,
 *
 *	| sector_0 || sector_1 || ... || sector_n ||
 *
 * will be convterted into:
 *
 *	| sector_0 |grass|| sector_1 |grass|| ... || sector_n |grass||
 *
 *	Sector_n must me moved n * grass_size forward,
 *	Sector_1 must me moved 1 * grass_size forward
 *
 *
 * If output sector size is less than input sector size,
 *
 *	| sector_0 |grass|| sector_1 |grass|| ... || sector_n |grass||
 *
 * will be convterted into:
 *
 *	| sector_0 || sector_1 || ... || sector_n ||
 *
 *	Sector_1 must me moved 1 * grass_size backward,
 *	Sector_n must me moved n * grass_size backward,
 *
 *	Sector_0 must never be moved.
 */
EXPORT void
scatter_secs(trackp, bp, nsecs)
	track_t	*trackp;
	char	*bp;
	int	nsecs;
{
	char	*from;
	char	*to;
	int	isecsize = trackp->isecsize;
	int	secsize = trackp->secsize;
	int	i;

	if (secsize == isecsize)
		return;

	nsecs -= 1;	/* we do not move sector # 0 */

	if (secsize < isecsize) {
		from = bp + isecsize;
		to   = bp + secsize;

		for (i = nsecs; i > 0; i--) {
			movebytes(from, to, secsize);
			from += isecsize;
			to   += secsize;
		}
	} else {
		from = bp + (nsecs * isecsize);
		to   = bp + (nsecs * secsize);

		for (i = nsecs; i > 0; i--) {
			movebytes(from, to, isecsize);
			from -= isecsize;
			to   -= secsize;
		}
	}
}
