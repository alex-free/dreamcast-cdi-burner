/* @(#)sleep.c	1.6 14/03/03 Copyright 2007-2014 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)sleep.c	1.6 14/03/03 Copyright 2007-2014 J. Schilling";
#endif
/*
 *	Emulate sleep where it does not exist
 *
 *	Copyright (c) 2007-2014 J. Schilling
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

#if	!defined(HAVE_SLEEP) && (defined(_MSC_VER) || defined(__MINGW32__))
#include <schily/windows.h>
#include <schily/standard.h>

EXPORT unsigned int
sleep(secs)
	unsigned int	secs;
{
	DWORD		ms;

	if (secs == 0)
		return (0);

	ms = secs * 1000;
	Sleep(ms);
	return (0);
}

EXPORT int
usleep(usecs)
	unsigned int	usecs;
{
	DWORD		ms;

	if (usecs == 0)
		return (0);

	ms = usecs / 1000;
	Sleep(ms);
	return (0);
}
#endif
