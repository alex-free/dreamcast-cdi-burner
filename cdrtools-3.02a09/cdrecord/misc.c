/* @(#)misc.c	1.9 14/02/17 Copyright 1998, 2001-2014 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)misc.c	1.9 14/02/17 Copyright 1998, 2001-2014 J. Schilling";
#endif
/*
 *	Misc support functions
 *
 *	Copyright (c) 1998, 2001-2014 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/time.h>
#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/schily.h>

EXPORT	void	timevaldiff	__PR((struct timeval *start, struct timeval *stop));
EXPORT	void	prtimediff	__PR((const char *fmt,
					struct timeval *start,
					struct timeval *stop));

EXPORT void
timevaldiff(start, stop)
	struct timeval	*start;
	struct timeval	*stop;
{
	struct timeval tv;

	tv.tv_sec = stop->tv_sec - start->tv_sec;
	tv.tv_usec = stop->tv_usec - start->tv_usec;
	while (tv.tv_usec > 1000000) {
		tv.tv_usec -= 1000000;
		tv.tv_sec += 1;
	}
	while (tv.tv_usec < 0) {
		tv.tv_usec += 1000000;
		tv.tv_sec -= 1;
	}
	*stop = tv;
}

EXPORT void
prtimediff(fmt, start, stop)
	const	char	*fmt;
	struct timeval	*start;
	struct timeval	*stop;
{
	struct timeval tv;

	tv.tv_sec = stop->tv_sec - start->tv_sec;
	tv.tv_usec = stop->tv_usec - start->tv_usec;
	while (tv.tv_usec > 1000000) {
		tv.tv_usec -= 1000000;
		tv.tv_sec += 1;
	}
	while (tv.tv_usec < 0) {
		tv.tv_usec += 1000000;
		tv.tv_sec -= 1;
	}
	/*
	 * We need to cast timeval->* to long because
	 * of the broken sys/time.h in Linux.
	 */
	printf("%s%4ld.%03lds (%2.2ld:%2.2ld:%2.2ld.%3.3ld)\n",
		fmt,
		(long)tv.tv_sec, (long)tv.tv_usec/1000,
		(long)tv.tv_sec/3600,
		(long)(tv.tv_sec/60)%60,
		(long)tv.tv_sec%60, (long)tv.tv_usec/1000);
	flush();
}
