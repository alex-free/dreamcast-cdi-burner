/* @(#)mkgmtime.c	1.4 11/08/28 Copyright 2011 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)mkgmtime.c	1.4 11/08/28 Copyright 2011 J. Schilling";
#endif
/*
 *	mkgmtime() is a complement to mktime()
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
#include <schily/utypes.h>
#include <schily/errno.h>
#include <schily/schily.h>

EXPORT	Llong	mklgmtime	__PR((struct tm *tp));
EXPORT	time_t	mkgmtime	__PR((struct tm *tp));

/*
 * The Gregorian leap year formula
 */
#define	dysize(A) (((A)%4)? 365 : (((A)%100) == 0 && ((A)%400)) ? 365 : 366)
#define	isleap(A) (((A)%4)? 0   : (((A)%100) == 0 && ((A)%400)) ? 0   : 1)
/*
 * Return the number of leap years since 0 AD assuming that the Gregorian
 * calendar applies to all years.
 */
#define	LEAPS(Y) 	((Y) / 4 - (Y) / 100 + (Y) / 400)
/*
 * Return the number of days since 0 AD
 */
#define	YRDAYS(Y)	(((Y) * 365L) + LEAPS(Y))
/*
 * Return the number of days between Januar 1 1970 and the end of the year
 * before the the year used as argument.
 */
#define	DAYS_SINCE_70(Y) (YRDAYS((Y)-1) - YRDAYS(1970-1))

LOCAL	int dmbeg[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

EXPORT Llong
mklgmtime(tp)
	struct tm	*tp;
{
	register Llong	tim;

	if (tp->tm_mon >= 12) {
		tp->tm_year += tp->tm_mon / 12;
		tp->tm_mon %= 12;
	} else if (tp->tm_mon < 0) {
		int	m = -tp->tm_mon;

		tp->tm_year -= m / 12;
		m %= 12;
		if (m) {
			tp->tm_year -= 1;
			tp->tm_mon = 12 - m;
		} else {
			tp->tm_mon = 0;
		}
	}

	{
		register int	y = tp->tm_year + 1900;
		register int	t = tp->tm_mon;

		tim = DAYS_SINCE_70(y);

		if (t >= 2 && isleap(y))	/* March or later */
			tim += 1;		/* Add 29.2.	  */
		tim += dmbeg[t];
	}

	tim += tp->tm_mday - 1;
	tim *= 24;
	tim += tp->tm_hour;
	tim *= 60;
	tim += tp->tm_min;
	tim *= 60;
	tim += tp->tm_sec;
	return (tim);
}

EXPORT time_t
mkgmtime(tp)
	struct tm	*tp;
{
	Llong	tim;
	time_t	t;

	t = tim = mklgmtime(tp);
	if (t != tim) {
#ifdef	EOVERFLOW
		seterrno(EOVERFLOW);
#else
		seterrno(EINVAL);
#endif
		return (-1);
	}
	return (t);
}
