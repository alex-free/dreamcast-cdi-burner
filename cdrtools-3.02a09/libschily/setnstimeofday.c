/* @(#)setnstimeofday.c	1.1 13/09/30 Copyright 2007-2013 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)setnstimeofday.c	1.1 13/09/30 Copyright 2007-2013 J. Schilling";
#endif
/*
 *	setnstimeofday() a nanosecond enabled version of settimeofday()
 *
 *	Copyright (c) 2007-2013 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#if	(defined(_MSC_VER) || defined(__MINGW32__))
#include <schily/windows.h>
#include <schily/time.h>
#include <schily/utypes.h>
#include <schily/standard.h>

#ifdef	_MSC_VER
const	__int64 MS_FTIME_ADD	= 0x2b6109100i64;
const	__int64 MS_FTIME_SECS	= 10000000i64;
#else
const	Int64_t MS_FTIME_ADD	= 0x2b6109100LL;
const	Int64_t MS_FTIME_SECS	= 10000000LL;
#endif

EXPORT int
setnstimeofday(tp)
	struct timespec	*tp;
{
	FILETIME	ft;
	SYSTEMTIME	st;
	Int64_t		T;

	if (tp == 0)
		return (0);

	/* 100ns time since 1601 */

	T = (tp->tv_sec + MS_FTIME_ADD) * MS_FTIME_SECS;
	T += tp->tv_nsec / 100;		/* T Granularity is 100ns */
	ft.dwLowDateTime = T & 0xFFFFFFFFUL;
	T >>= 32;
	ft.dwHighDateTime = T;

	if (FileTimeToSystemTime(&ft, &st) == 0)
		return (-1);

	if (SetSystemTime(&st) == 0)
		return (-1);

	return (0);
}
#else	/* (defined(_MSC_VER) || defined(__MINGW32__)) */

#include <schily/time.h>
#include <schily/standard.h>
#include <schily/schily.h>

EXPORT int
setnstimeofday(tp)
	struct timespec	*tp;
{
	struct timeval	tv;

#if	defined(HAVE_CLOCK_SETTIME) && defined(HAVE_CLOCK_GETTIME_IN_LIBC)
	if (clock_settime(CLOCK_REALTIME, tp) == 0)
		return (0);
#endif

	tv.tv_sec = tp->tv_sec;
	tv.tv_usec = tp->tv_nsec / 1000;
	return (settimeofday(&tv, (void *)0));
}

#endif	/* (defined(_MSC_VER) || defined(__MINGW32__)) */
