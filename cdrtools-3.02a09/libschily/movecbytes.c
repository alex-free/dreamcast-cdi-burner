/* @(#)movecbytes.c	1.2 16/11/05 Copyright 2016 J. Schilling */
/*
 *	move data, stop if character c is copied
 *
 *	Copyright (c) 2016 J. Schilling
 */
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

#include <schily/standard.h>
#include <schily/types.h>
#include <schily/schily.h>

#define	DO8(a)	a; a; a; a; a; a; a; a;

/*
 * movecbytes(from, to, c, cnt) is the same as memccpy(to, from, c, cnt)
 */
EXPORT char *
movecbytes(fromv, tov, c, cnt)
	const void	*fromv;
	void		*tov;
	register int	c;
	size_t		cnt;
{
	register const char	*from	= fromv;
	register char		*to	= tov;
	register size_t		n;

	if ((n = cnt) == 0)
		return (NULL);

#define	separate_code
#ifdef	separate_code
	while (n >= 8) {
		/* BEGIN CSTYLED */
		DO8(
			if ((*to++ = *from++) == (char)c)
				return (to);
		);
		/* END CSTYLED */
		n -= 8;
	}

	switch (n) {

	case 7:	if ((*to++ = *from++) == (char)c)
			return (to);
	case 6:	if ((*to++ = *from++) == (char)c)
			return (to);
	case 5:	if ((*to++ = *from++) == (char)c)
			return (to);
	case 4:	if ((*to++ = *from++) == (char)c)
			return (to);
	case 3:	if ((*to++ = *from++) == (char)c)
			return (to);
	case 2:	if ((*to++ = *from++) == (char)c)
			return (to);
	case 1:	if ((*to++ = *from++) == (char)c)
			return (to);
	}
#else
	/*
	 * This variant should be as fast as the code above but
	 * half the size. Unfortunately, most compilers do not optmize
	 * it correctly.
	 */
	int	rest = n % 8;

	n -= rest;

	switch (rest) {

	case 0: while (n != 0) {
			n -= 8;
			if ((*to++ = *from++) == (char)c)
				return (to);
	case 7:		if ((*to++ = *from++) == (char)c)
				return (to);
	case 6:		if ((*to++ = *from++) == (char)c)
				return (to);
	case 5:		if ((*to++ = *from++) == (char)c)
				return (to);
	case 4:		if ((*to++ = *from++) == (char)c)
				return (to);
	case 3:		if ((*to++ = *from++) == (char)c)
				return (to);
	case 2:		if ((*to++ = *from++) == (char)c)
				return (to);
	case 1:		if ((*to++ = *from++) == (char)c)
				return (to);
		}
	}
#endif
	return (NULL);
}
