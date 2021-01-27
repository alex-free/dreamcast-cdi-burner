/* @(#)movesect.h	1.1 01/06/02 Copyright 2001 J. Schilling */
/*
 *	Copyright (c) 2001 J. Schilling
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

#ifndef	_MOVESECT_H
#define	_MOVESECT_H

#define	move2352(from, to)	movebytes(from, to, 2352)
#define	move2336(from, to)	movebytes(from, to, 2336)
#define	move2048(from, to)	movebytes(from, to, 2048)

#define	fill2352(p, val)	fillbytes(p, 2352, val)
#define	fill2048(p, val)	fillbytes(p, 2048, val)
#define	fill96(p, val)		fillbytes(p, 96, val)

extern	void	scatter_secs	__PR((track_t *trackp, char *bp, int nsecs));

#endif
