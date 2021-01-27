/* @(#)jmpdefs.h	1.9 09/07/13 Copyright 1998-2009 J. Schilling */
/*
 *	Definitions that help to handle a jmp_buf
 *
 *	Copyright (c) 1998-2009 J. Schilling
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

#ifndef	_SCHILY_JMPDEFS_H
#define	_SCHILY_JMPDEFS_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef	_SCHILY_SETJMP_H
#include <schily/setjmp.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct {
	jmp_buf	jb;
} jmps_t;

#if defined(HAVE_SIGSETJMP) && defined(HAVE_SIGLONGJMP)
#define	HAVE_SIGJMPS_T

typedef struct {
	sigjmp_buf jb;
} sigjmps_t;

#else

/*
 * Make sure to use the best available
 */
#define	sigjmps_t		jmps_t
#define	sigsetjmp(je, sm)	setjmp(je)
#define	siglongjmp		longjmp

#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_JMPDEFS_H */
