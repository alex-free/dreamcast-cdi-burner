/* @(#)usleep.c	1.24 14/03/03 Copyright 1995-2014 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)usleep.c	1.24 14/03/03 Copyright 1995-20014 J. Schilling";
#endif
/*
 *	Copyright (c) 1995-2014 J. Schilling
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

#define	usleep	__nothing_    /* prototype in unistd.h may be different */
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/time.h>
#ifdef	HAVE_POLL_H
#	include <poll.h>
#else
#	ifdef	HAVE_SYS_POLL_H
#	include <sys/poll.h>
#	endif
#endif
#include <schily/systeminfo.h>
#include <schily/libport.h>
#undef	usleep

#ifndef	HAVE_USLEEP
EXPORT	int	usleep		__PR((int usec));
#endif

#ifdef	OPENSERVER
/*
 * Don't use the usleep() from libc on SCO's OPENSERVER.
 * It will kill our processes with SIGALRM.
 * SCO has a usleep() prototype in unistd.h, for this reason we
 * #define usleep to __nothing__ before including unistd.h
 */
#undef	HAVE_USLEEP
#endif

#ifdef apollo
/*
 * Apollo sys5.3 usleep is broken.  Define a version based on time_$wait.
 */
#include <apollo/base.h>
#include <apollo/time.h>
#undef HAVE_USLEEP
#endif

#if	!defined(HAVE_USLEEP)

EXPORT int
usleep(usec)
	int	usec;
{
#if defined(apollo)
	/*
	 * Need to check apollo before HAVE_SELECT, because Apollo has select,
	 * but it's time wait feature is also broken :-(
	 */
#define	HAVE_USLEEP
	/*
	 * XXX Do these vars need to be static on Domain/OS ???
	 */
	static time_$clock_t	DomainDelay;
	static status_$t	DomainStatus;

	/*
	 * DomainDelay is a 48 bit value that defines how many 4uS periods to
	 * delay.  Since the input value range is 32 bits, the upper 16 bits of
	 * DomainDelay must be zero.  So we just divide the input value by 4 to
	 * determine how many "ticks" to wait
	 */
	DomainDelay.c2.high16 = 0;
	DomainDelay.c2.low32 = usec / 4;
	time_$wait(time_$relative, DomainDelay, &DomainStatus);
#endif	/* Apollo */

#if	defined(HAVE_SELECT) && !defined(HAVE_USLEEP)
#define	HAVE_USLEEP

	struct timeval tv;
	tv.tv_sec = usec / 1000000;
	tv.tv_usec = usec % 1000000;
	select(0, 0, 0, 0, &tv);
#endif

#if	defined(HAVE_POLL) && !defined(HAVE_USLEEP)
#define	HAVE_USLEEP

	if (poll(0, 0, usec/1000) < 0)
		comerr("poll delay failed.\n");

#endif

	/*
	 * We cannot use nanosleep() until we found a way to check
	 * separately for nanosleep() in libc and for nanosleep in librt.
	 */
#if	defined(xxxHAVE_NANOSLEEP) && !defined(HAVE_USLEEP)
#define	HAVE_USLEEP

	struct timespec ts;

	ts.tv_sec = usec / 1000000;
	ts.tv_nsec = (usec % 1000000) * 1000;

	nanosleep(&ts, 0);
#endif

#if	(defined(_MSC_VER) || defined(__MINGW32__)) && !defined(HAVE_USLEEP)
#define	HAVE_USLEEP
	Sleep(usec/1000);
#endif

#if	!defined(HAVE_USLEEP)
#define	HAVE_USLEEP

	sleep((usec+500000)/1000000);
#endif

	return (0);
}
#endif
