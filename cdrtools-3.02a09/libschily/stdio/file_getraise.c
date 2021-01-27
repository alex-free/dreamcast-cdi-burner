/* @(#)file_getraise.c	1.1 07/04/03 Copyright 2007 J. Schilling */
/*
 *	Copyright (c) 2007 J. Schilling
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
file_getraise(f)
	register FILE *f;
{
	int	oflag;
extern	int	_io_glflag;

	if (f == (FILE *)NULL)
		return ((_io_glflag & _JS_IONORAISE) != 0);

	down(f);

	oflag = my_flag(f);
	return ((oflag & _JS_IONORAISE) != 0);
}
