/* @(#)fileseek.c	1.15 12/02/26 Copyright 1986, 1996-2012 J. Schilling */
/*
 *	Copyright (c) 1986, 1996-2012 J. Schilling
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

static	char	_seekerr[]	= "file_seek_err";

EXPORT int
fileseek(f, pos)
	register FILE	*f;
	off_t	pos;
{
	int	ret;

	down(f);
	ret = fseek(f, pos, SEEK_SET);
	if (ret < 0 && !(my_flag(f) & _JS_IONORAISE) &&
	    !(_io_glflag & _JS_IONORAISE))
		raisecond(_seekerr, 0L);
	return (ret);
}
