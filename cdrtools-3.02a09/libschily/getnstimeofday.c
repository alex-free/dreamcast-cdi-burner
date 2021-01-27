/* @(#)getnstimeofday.c	1.2 13/09/30 Copyright 2007-2013 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)getnstimeofday.c	1.2 13/09/30 Copyright 2007-2013 J. Schilling";
#endif
/*
 *	getnstimeofday() a nanosecond enabled version of gettimeofday()
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
getnstimeofday(tp)
	struct timespec	*tp;
{
	FILETIME	ft;
	Int64_t		T;

	if (tp == 0)
		return (0);

	GetSystemTimeAsFileTime(&ft);	/* 100ns time since 1601 */
	T   = ft.dwHighDateTime;
	T <<= 32;
	T  += ft.dwLowDateTime;

	/*
	 * Cast to avoid a loss of data warning
	 * MSVC uses long instead of time_t for tv_sec
	 */
	tp->tv_sec  = (long) (T / MS_FTIME_SECS - MS_FTIME_ADD);
	tp->tv_nsec = (long) (T % MS_FTIME_SECS) * 100;

	return (0);
}
#else	/* (defined(_MSC_VER) || defined(__MINGW32__)) */

#include <schily/time.h>
#include <schily/standard.h>
#include <schily/schily.h>

EXPORT int
getnstimeofday(tp)
	struct timespec	*tp;
{
	struct timeval	tv;
	int		ret;

#if	defined(HAVE_CLOCK_GETTIME) && defined(HAVE_CLOCK_GETTIME_IN_LIBC)
#ifdef	__CLOCK_REALTIME0
	/*
	 * Use fasttrap on Solaris
	 */
	if (clock_gettime(__CLOCK_REALTIME0, tp) == 0)
		return (0);
#else
	if (clock_gettime(CLOCK_REALTIME, tp) == 0)
		return (0);
#endif
#endif

	ret = gettimeofday(&tv, (void *)0);
	tp->tv_sec = tv.tv_sec;
	tp->tv_nsec = tv.tv_usec * 1000;
	return (ret);
}

#endif	/* (defined(_MSC_VER) || defined(__MINGW32__)) */
