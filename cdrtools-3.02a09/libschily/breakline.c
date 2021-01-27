/* @(#)breakline.c	1.11 06/09/13 Copyright 1985, 1995-2003 J. Schilling */
/*
 *	break a line pointed to by *buf into fields
 *	returns the number of tokens, the line was broken into (>= 1)
 *
 *	delim is the delimiter to break at
 *	array[0 .. found-1] point to strings from broken line
 *	array[found ... len] point to '\0'
 *	len is the size of the array
 *
 *	Copyright (c) 1985, 1995-2003 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/standard.h>
#include <schily/schily.h>

#ifdef	PROTOTYPES
EXPORT int
breakline(char *buf,
		register char delim,
		register char *array[],
		register int len)
#else
EXPORT int
breakline(buf, delim, array, len)
		char	*buf;
	register char	delim;
	register char	*array[];
	register int	len;
#endif
{
	register char	*bp = buf;
	register char	*dp;
	register int	i;
	register int	found;

	for (i = 0, found = 1; i < len; i++) {
		for (dp = bp; *dp != '\0' && *dp != delim; dp++)
			/* LINTED */
			;

		array[i] = bp;
		if (*dp == delim) {
			*dp++ = '\0';
			found++;
		}
		bp = dp;
	}
	return (found);
}
