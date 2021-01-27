/* @(#)getxnum.c	1.6 09/07/08 Copyright 1984-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)getxnum.c	1.6 09/07/08 Copyright 1984-2009 J. Schilling";
#endif
/*
 *	Generic number conversion routines.
 *	Originally taken from sdd.c to implement 'dd' like number options.
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

LOCAL	Llong	number		__PR((char *arg, int *retp, gnmult_t *mult));
EXPORT	int	getxnum		__PR((char *arg, long *valp, gnmult_t *mult));
EXPORT	int	getllxnum	__PR((char *arg, Llong *lvalp, gnmult_t *mult));

LOCAL Llong
number(arg, retp, mult)
	register char	*arg;
		int	*retp;
		gnmult_t *mult;
{
	Llong	val	= 0;

	if (*retp != 1)
		return (val);
	if (*arg == '\0') {
		*retp = -1;
	} else if (*(arg = astoll(arg, &val))) {
		gnmult_t *n;

		for (n = mult; n->key; n++) {
			if (n->key == *arg) {
				val *= n->mult;
				arg++;
				break;
			}
		}
		if (*arg == '*' || *arg == 'x')
			val *= number(++arg, retp, mult);
		else if (*arg != '\0')
			*retp = -1;
	}
	return (val);
}

EXPORT int
getxnum(arg, valp, mult)
	char	*arg;
	long	*valp;
	gnmult_t *mult;
{
	Llong	llval;
	int	ret = 1;

	llval = number(arg, &ret, mult);
	*valp = llval;
	if (*valp != llval) {
		errmsgno(EX_BAD,
			"Value %lld is too large for data type 'long'.\n",
									llval);
		ret = -1;
	}
	return (ret);
}

EXPORT int
getllxnum(arg, lvalp, mult)
	char	*arg;
	Llong	*lvalp;
	gnmult_t *mult;
{
	int	ret = 1;

	*lvalp = number(arg, &ret, mult);
	return (ret);
}
