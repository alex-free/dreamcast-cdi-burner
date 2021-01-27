/* @(#)iodefs.h	1.1 07/06/10 Copyright 1988-2007 J. Schilling */
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

#ifndef	_IODEFS_H
#define	_IODEFS_H

#include <schily/ccomdefs.h>

/*
 * io.c:
 */
#ifdef	D_OK_MASK
extern	BOOL	cvt_std		__PR((char *, long *, long, long,
							struct disk *));
extern	BOOL	getvalue	__PR((char *, long *, long, long,
				void (*)(char *, long, long, long, struct disk *),
				BOOL (*)(char *, long *, long, long, struct disk *),
				struct disk *));
#endif
extern	BOOL	getlong		__PR((char *, long *, long, long));
extern	BOOL	getint		__PR((char *, int *, int, int));
extern	BOOL	yes		__PR((char *, ...)) __printflike__(1, 2);

#endif	/* _IODEFS_H */
