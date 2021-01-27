/* @(#)mman.h	1.4 07/01/16 Copyright 2001-2007 J. Schilling */
/*
 *	Definitions to be used for mmap()
 *
 *	Copyright (c) 2001-2007 J. Schilling
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

#ifndef	_SCHILY_MMAN_H
#define	_SCHILY_MMAN_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>
#endif

#if defined(HAVE_SMMAP)

#ifndef	_INCL_SYS_MMAN_H
#include <sys/mman.h>
#define	_INCL_SYS_MMAN_H
#endif

#ifndef	MAP_ANONYMOUS
#	ifdef	MAP_ANON
#	define	MAP_ANONYMOUS	MAP_ANON
#	endif
#endif

#ifndef MAP_FILE
#	define MAP_FILE		0	/* Needed on Apollo Domain/OS */
#endif

/*
 * Needed for Apollo Domain/OS and may be for others?
 */
#ifdef	_MMAP_WITH_SIZEP
#	define	mmap_sizeparm(s)	(&(s))
#else
#	define	mmap_sizeparm(s)	(s)
#endif

#endif	/* defined(HAVE_SMMAP) */

#endif	/* _SCHILY_MMAN_H */
