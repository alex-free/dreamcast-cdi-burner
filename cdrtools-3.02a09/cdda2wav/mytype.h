/* @(#)mytype.h	1.5 16/02/14 Copyright 1998,1999 Heiko Eissfeldt */

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

#if	4 == SIZEOF_LONG_INT
#define	UINT4 long unsigned int
#define	UINT4_C ULONG_C
#else
#if	4 == SIZEOF_INT
#define	UINT4 unsigned int
#define	UINT4_C UINT_C
#else
#if	4 == SIZEOF_SHORT_INT
#define	UINT4 short unsigned int
#define	UINT4_C USHORT_C
#else
error need an integer type with 32 bits, but do not know one!
#endif
#endif
#endif
#ifndef	TRUE
#define	TRUE	1
#endif
#ifndef	FALSE
#define	FALSE	0
#endif

#ifndef	offsetof
#define	offsetof(TYPE, MEMBER)	((size_t) &((TYPE *)0)->MEMBER)
#endif
