/* @(#)getxtnum.c	1.9 09/07/08 Copyright 1984-2002, 2004-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)getxtnum.c	1.9 09/07/08 Copyright 1984-2002, 2004-2009 J. Schilling";
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
#include <schily/nlsdefs.h>

LOCAL	Llong	tnumber		__PR((char *arg, int *retp, gnmult_t *mult, int level));
EXPORT	int	getxtnum	__PR((char *arg, time_t *valp, gnmult_t *mult));
EXPORT	int	getllxtnum	__PR((char *arg, Llong *lvalp, gnmult_t *mult));

LOCAL Llong
tnumber(arg, retp, mult, level)
	register char	*arg;
		int	*retp;
		gnmult_t *mult;
		int	level;
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
		if (*arg >= '0' && *arg <= '9')
			val += tnumber(arg, retp, mult, level+1);
		else if (*arg != '\0') {
			errmsgno(EX_BAD,
			gettext("Illegal character '%c' in timespec.\n"),
				*arg);
			*retp = -1;
		} else if (*arg == '\0')
			return (val);
	}
	if (level > 0 && *arg == '\0')
		*retp = -1;
	return (val);
}

EXPORT int
getxtnum(arg, valp, mult)
	char	*arg;
	time_t	*valp;
	gnmult_t *mult;
{
	Llong	llval;
	int	ret = 1;

	llval = tnumber(arg, &ret, mult, 0);
	*valp = llval;
	if (*valp != llval) {
		errmsgno(EX_BAD,
		gettext("Value %lld is too large for data type 'time_t'.\n"),
									llval);
		ret = -1;
	}
	return (ret);
}

EXPORT int
getllxtnum(arg, lvalp, mult)
	char	*arg;
	Llong	*lvalp;
	gnmult_t *mult;
{
	int	ret = 1;

	*lvalp = tnumber(arg, &ret, mult, 0);
	return (ret);
}
