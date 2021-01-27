/* @(#)nixwrite.c	1.7 09/06/30 Copyright 1986, 2001-2009 J. Schilling */
/*
 *	Non interruptable extended write
 *
 *	Copyright (c) 1986, 2001-2009 J. Schilling
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

EXPORT ssize_t
_nixwrite(f, buf, count)
	int	f;
	void	*buf;
	size_t	count;
{
	register char	*p = (char *)buf;
	register ssize_t ret;
	register int	total = 0;
		int	oerrno = geterrno();

	if ((ret = (ssize_t)count) < 0) {
		seterrno(EINVAL);
		return ((ssize_t)-1);
	}
	while (count > 0) {
		while ((ret = write(f, p, count)) < 0) {
			if (geterrno() == EINTR) {
				/*
				 * Set back old 'errno' so we don't change the
				 * errno visible to the outside if we did
				 * not fail.
				 */
				seterrno(oerrno);
				continue;
			}
			return (ret);	/* Any other error */
		}
		if (ret == 0)		/* EOF */
			break;

		total += ret;
		count -= ret;
		p += ret;
	}
	return (total);
}
