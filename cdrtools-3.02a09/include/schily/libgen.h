/* @(#)libgen.h	1.2 10/05/07 Copyright 2010 J. Schilling */
/*
 *	Abstraction from libgen.h
 *
 *	Copyright (c) 2010 J. Schilling
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

#ifndef	_SCHILY_LIBGEN_H
#define	_SCHILY_LIBGEN_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_LIBGEN_H
#ifndef	_INCL_LIBGEN_H
#define	_INCL_LIBGEN_H
#include <libgen.h>
#endif
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * See also libport.h
 */
#ifndef	HAVE_BASENAME
extern	char		*basename	__PR((char *name));
#endif
#ifndef	HAVE_DIRNAME
extern	char		*dirname	__PR((char *name));
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_LIBGEN_H */
