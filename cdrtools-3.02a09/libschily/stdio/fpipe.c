/* @(#)fpipe.c	1.12 04/08/08 Copyright 1986, 1995-2003 J. Schilling */
/*
 *	Copyright (c) 1986, 1995-2003 J. Schilling
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

EXPORT int
fpipe(pipef)
	FILE *pipef[];
{
	int filedes[2];

	if (pipe(filedes) < 0)
		return (0);

	if ((pipef[0] = _fcons((FILE *)0,
				filedes[0], FI_READ|FI_CLOSE)) != (FILE *)0) {
		if ((pipef[1] = _fcons((FILE *)0,
				filedes[1], FI_WRITE|FI_CLOSE)) != (FILE *)0) {
			return (1);
		}
		fclose(pipef[0]);
	}
	close(filedes[1]);
	return (0);
}
