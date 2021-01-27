/* @(#)cmpbytes.c	1.23 10/08/21 Copyright 1988, 1995-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)cmpbytes.c	1.23 10/08/21 Copyright 1988, 1995-2010 J. Schilling";
#endif  /* lint */
/*
 *	compare data
 *	Return the index of the first differing character
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
#include <schily/align.h>
#include <schily/types.h>
#include <schily/schily.h>

#define	DO8(a)	a; a; a; a; a; a; a; a;

/*
 * Return the index of the first differing character
 * This interface is not compatible to memcmp()
 */
EXPORT ssize_t
cmpbytes(fromp, top, cnt)
	const void	*fromp;
	const void	*top;
	ssize_t		cnt;
{
	register const char	*from	= (char *)fromp;
	register const char	*to	= (char *)top;
	register ssize_t	n;
	register ssize_t	i;

	/*
	 * If we change cnt to be unsigned, check for == instead of <=
	 */
	if ((n = cnt) <= 0)
		return (cnt);

	/*
	 * Compare byte-wise until properly aligned for a long pointer.
	 */
	i = sizeof (long) - 1;
	while (--n >= 0 && --i >= 0 && !l2aligned(from, to)) {
		if (*to++ != *from++)
			goto cdiff;
	}
	n++;

	if (n >= (ssize_t)(8 * sizeof (long))) {
		if (l2aligned(from, to)) {
			register const long *froml = (const long *)from;
			register const long *tol   = (const long *)to;
			register ssize_t rem = n % (8 * sizeof (long));

			n /= (8 * sizeof (long));
			do {
				/* BEGIN CSTYLED */
				DO8(
					if (*tol++ != *froml++)
						break;
				);
				/* END CSTYLED */
			} while (--n > 0);

			if (n > 0) {
				--froml;
				--tol;
				to = (const char *)tol;
				from = (const char *)froml;
				goto ldiff;
			}
			to = (const char *)tol;
			from = (const char *)froml;
			n = rem;
		}

		if (n >= 8) {
			n -= 8;
			do {
				/* BEGIN CSTYLED */
				DO8(
					if (*to++ != *from++)
						goto cdiff;
				);
				/* END CSTYLED */
			} while ((n -= 8) >= 0);
			n += 8;
		}
		if (n > 0) do {
			if (*to++ != *from++)
				goto cdiff;
		} while (--n > 0);
		return (cnt);
	}
	if (n > 0) do {
		if (*to++ != *from++)
			goto cdiff;
	} while (--n > 0);
	return (cnt);
ldiff:
	n = sizeof (long);
	do {
		if (*to++ != *from++)
			goto cdiff;
	} while (--n > 0);
cdiff:
	return ((ssize_t)(--from - (char *)fromp));
}
