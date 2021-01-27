/* @(#)xio.h	1.4 06/11/11 Copyright 2003-2004 J. Schilling */
/*
 *	EXtended I/O functions for cdrecord
 *
 *	Copyright (c) 2003-2004 J. Schilling
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

#ifndef	_XIO_H
#define	_XIO_H

#include <schily/utypes.h>
#include <schily/unistd.h>

typedef struct xio {
	struct xio	*x_next;
	char		*x_name;
	Ullong		x_off;
	off_t		x_startoff;
	int		x_file;
	int		x_refcnt;
	int		x_oflag;
	int		x_omode;
	int		x_xflags;
} xio_t;

/*
 * Defines for x_xflags
 */
#define	X_NOREWIND	0x01	/* Do not rewind() the file on xclose() */
#define	X_UFLAGS	0xFF	/* Mask for flags allowed with xopen() */

#define	X_NOSEEK	0x1000	/* Cannot seek on this fd */


#define	xfileno(p)	(((xio_t *)(p))->x_file)

extern	void	*xopen		__PR((char *name, int oflag, int mode,
								int xflags));
extern	off_t	xmarkpos	__PR((void *vp));
extern	int	xclose		__PR((void *vp));

#endif
