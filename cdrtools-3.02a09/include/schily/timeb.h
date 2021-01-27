/* @(#)timeb.h	1.1 13/04/30 Copyright 2013 J. Schilling */
/*
 *	ftime abstraction
 *
 *	Copyright (c) 2013 J. Schilling
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

#ifndef	_SCHILY_TIMEB_H
#define	_SCHILY_TIMEB_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_TYPES_H
#include <schily/types.h>
#endif
#ifndef _SCHILY_TIME_H
#include <schily/time.h>
#endif

#ifdef	HAVE_FTIME
#ifndef	_INCL_SYS_TIMEB_H
#include <sys/timeb.h>
#define	_INCL_SYS_TIMEB_H
#endif
#endif

#endif	/* _SCHILY_TIMEB_H */
