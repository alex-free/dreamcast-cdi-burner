/* @(#)fdup.c	1.16 10/11/06 Copyright 1986, 1995-2010 J. Schilling */
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

EXPORT FILE *
fdup(f)
	register FILE  *f;
{
	int  newfd;

	down(f);
	if ((newfd = dup(fileno(f))) < 0)
		return ((FILE *)NULL);

	lseek(newfd, ftell(f), SEEK_SET);
	return (_fcons((FILE *)0, newfd, (FI_READ | FI_WRITE | FI_CLOSE)));
}
