/* @(#)times.h	1.1 09/07/13 Copyright 2009 J. Schilling */
/*
 *	Times abstraction
 *
 *	Copyright (c) 2009 J. Schilling
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

#ifndef	_SCHILY_TIMES_H
#define	_SCHILY_TIMES_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef	_SCHILY_TIME_H
#include <schily/time.h>
#endif
#ifndef	_SCHILY_LIMITS_H
#include <schily/limits.h>
#endif
#ifndef	_SCHILY_UNISTD_H
#include <schily/unistd.h>
#endif

/*
 * Make sure to include schily/time.h before, because of a Next Step bug.
 */
#ifdef	HAVE_SYS_TIMES_H
#ifndef	_INCL_SYS_TIMES_H
#define	_INCL_SYS_TIMES_H
#include <sys/times.h>
#endif
#endif

#ifndef	CLK_TCK
#if	defined(_SC_CLK_TCK)
#define	CLK_TCK	((clock_t)sysconf(_SC_CLK_TCK))
#else
#define	CLK_TCK	60
#endif
#endif

#endif	/* _SCHILY_TIMES_H */
