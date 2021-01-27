/* @(#)utime.h	1.9 10/08/24 Copyright 2001-2010 J. Schilling */
/*
 *	Defines for utimes() / utime()
 *
 *	Copyright (c) 2001-2010 J. Schilling
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

#ifndef	_SCHILY_UTIME_H
#define	_SCHILY_UTIME_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>
#endif

#ifdef	HAVE_UTIMES
#ifndef _SCHILY_TIME_H
#include <schily/time.h>
#endif
#endif

#ifdef	HAVE_UTIME_H
#ifndef	_INCL_UTIME_H
#include <utime.h>
#define	_INCL_UTIME_H
#endif
#else
#ifdef	HAVE_SYS_UTIME_H
#ifndef	_INCL_SYS_UTIME_H
#include <sys/utime.h>
#define	_INCL_SYS_UTIME_H
#endif
#else

#ifdef	__cplusplus
extern "C" {
#endif

struct utimbuf {
	time_t	actime;
	time_t	modtime;
};

#ifdef	__cplusplus
}
#endif

#endif
#endif

#ifdef	__comment__
/*
 * file.c contains this
 * I am not sure if it is really needed.
 * It may be a good idea to add a test for HAVE_STRUCT_UTIMBUF
 * as in gnutar.
 */
#if (__COHERENT__ >= 0x420)
# include <sys/utime.h>
#else
#  include <utime.h>
#endif

#endif	/* __comment__ */

#endif	/* _SCHILY_UTIME_H */
