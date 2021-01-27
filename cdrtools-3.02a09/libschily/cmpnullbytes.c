/* @(#)cmpnullbytes.c	1.10 10/08/21 Copyright 1988,2002-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)cmpnullbytes.c	1.10 10/08/21 Copyright 1988,2002-2010 J. Schilling";
#endif  /* lint */
/*
 *	compare data against null
 *	Return the index of the first non-null character
 *
 *	Copyright (c) 1988,2002-2010 J. Schilling
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
 * Return the index of the first non-null character
 */
EXPORT ssize_t
cmpnullbytes(fromp, cnt)
	const void	*fromp;
	ssize_t		cnt;
{
	register const char	*from	= (char *)fromp;
	register ssize_t	n;

	/*
	 * If we change cnt to be unsigned, check for == instead of <=
	 */
	if ((n = cnt) <= 0)
		return (cnt);

	/*
	 * Compare byte-wise until properly aligned for a long pointer.
	 */
	while (--n >= 0 && !laligned(from)) {
		if (*from++ != 0)
			goto cdiff;
	}
	n++;

	if (n >= (ssize_t)(8 * sizeof (long))) {
		if (laligned(from)) {
			register const long *froml = (const long *)from;
			register ssize_t rem = n % (8 * sizeof (long));

			n /= (8 * sizeof (long));
			do {
				/* BEGIN CSTYLED */
				DO8(
					if (*froml++ != 0)
						break;
				);
				/* END CSTYLED */
			} while (--n > 0);

			if (n > 0) {
				--froml;
				from = (const char *)froml;
				goto ldiff;
			}
			from = (const char *)froml;
			n = rem;
		}

		if (n >= 8) {
			n -= 8;
			do {
				/* BEGIN CSTYLED */
				DO8(
					if (*from++ != 0)
						goto cdiff;
				);
				/* END CSTYLED */
			} while ((n -= 8) >= 0);
			n += 8;
		}
		if (n > 0) do {
			if (*from++ != 0)
				goto cdiff;
		} while (--n > 0);
		return (cnt);
	}
	if (n > 0) do {
		if (*from++ != 0)
			goto cdiff;
	} while (--n > 0);
	return (cnt);
ldiff:
	n = sizeof (long);
	do {
		if (*from++ != 0)
			goto cdiff;
	} while (--n > 0);
cdiff:
	return ((ssize_t)(--from - (char *)fromp));
}
