/* @(#)swabbytes.c	1.10 10/08/21 Copyright 1988, 1995-2010 J. Schilling */
/*
 *	swab bytes in memory
 *
 *	Copyright (c) 1988, 1995-2010 J. Schilling
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
#include <schily/types.h>
#include <schily/schily.h>

#define	DO8(a)	a; a; a; a; a; a; a; a;

EXPORT void
swabbytes(vp, cnt)
		void	*vp;
	register ssize_t cnt;
{
	register char	*bp = (char *)vp;
	register char	c;

	cnt /= 2;	/* even count only */
	while ((cnt -= 8) >= 0) {
		/* CSTYLED */
		DO8(c = *bp++; bp[-1] = *bp; *bp++ = c;);
	}
	cnt += 8;
	while (--cnt >= 0) {
		c = *bp++; bp[-1] = *bp; *bp++ = c;
	}
}
