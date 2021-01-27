/* @(#)fileluopen.c	1.19 10/11/06 Copyright 1986, 1995-2010 J. Schilling */
/*
 *	Copyright (c) 1986, 1995-2010 J. Schilling
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

#include "schilyio.h"

/*
 * Note that because of a definition in schilyio.h we are using
 * fseeko()/ftello() instead of fseek()/ftell() if available.
 */

#ifndef	O_NDELAY		/* This is undefined on BeOS :-( */
#define	O_NDELAY	0
#endif
#ifndef	O_CREAT
#define	O_CREAT		0
#endif
#ifndef	O_TRUNC
#define	O_TRUNC		0
#endif
#ifndef	O_EXCL
#define	O_EXCL		0
#endif

/*
 *	fileluopen - open a stream for lun
 */
EXPORT FILE *
fileluopen(f, mode)
	int		f;
	const char	*mode;
{
	int	omode = 0;
	int	flag = 0;

	if (!_cvmod(mode, &omode, &flag))
		return ((FILE *)NULL);

	if (omode & (O_NDELAY|O_CREAT|O_TRUNC|O_EXCL)) {
		raisecond(_badmode, 0L);
		return ((FILE *)NULL);
	}

#ifdef	F_GETFD
	if (fcntl(f, F_GETFD, 0) < 0) {
		raisecond(_badfile, 0L);
		return ((FILE *)NULL);
	}
#endif

#ifdef	O_APPEND
	if (omode & O_APPEND)
		lseek(f, (off_t)0, SEEK_END);
#endif

	return (_fcons((FILE *)0, f, flag));
}
