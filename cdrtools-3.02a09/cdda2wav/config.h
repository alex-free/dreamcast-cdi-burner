/* @(#)config.h	1.14 09/04/19 Copyright 1998-2003 Heiko Eissfeldt, Copyright 2006-2009 J. Schilling */
/*
 *	a central configuration file
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

#ifndef	_CONFIG_H
#define	_CONFIG_H
#include <schily/mconfig.h>

#if	__STDC__-0 != 0 || (defined PROTOTYPES && defined STDC_HEADERS)
#define	UINT_C(a)	(a##u)
#define	ULONG_C(a)	(a##ul)
#define	USHORT_C(a)	(a##uh)
#define	CONCAT(a, b)	a##b
#else
#define	UINT_C(a)	((unsigned) a)
#define	ULONG_C(a)	((unsigned long) a)
#define	USHORT_C(a)	((unsigned short) a)
			/* CSTYLED */
#define	CONCAT(a, b)	a/**/b
#endif

#include "lconfig.h"

/*
 * First the old static check.
 */
#ifdef	__BEOS__
#define	HAVE_AREAS	1
#endif

#if	defined(HAVE_OS_H) && \
	defined(HAVE_CLONE_AREA) && defined(HAVE_CREATE_AREA) && \
	defined(HAVE_DELETE_AREA)
#	define	HAVE_BEOS_AREAS	/* BeOS/Zeta/Haiku */
#	ifndef	HAVE_AREAS
#		define	HAVE_AREAS	1
#	endif
#endif


#if defined HAVE_FORK && (defined(HAVE_SMMAP) || defined(HAVE_USGSHM) || \
			defined(HAVE_DOSALLOCSHAREDMEM) || defined(HAVE_AREAS))
#define	HAVE_FORK_AND_SHAREDMEM
#undef	FIFO
#define	FIFO
#else
#undef	FIFO
#endif
#if	!defined	HAVE_MEMMOVE
#define	memmove(dst, src, size)	movebytes((src), (dst), (size))
#endif

#endif	/* _CONFIG_H */
