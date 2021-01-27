/* @(#)file_raise.c	1.9 07/04/03 Copyright 1986, 1995-2007 J. Schilling */
/*
 *	Copyright (c) 1986, 1995-2007 J. Schilling
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

EXPORT void
file_raise(f, flg)
	register FILE *f;
	int flg;
{
	int	oflag;
extern	int	_io_glflag;

	if (f == (FILE *)NULL) {
		if (flg)
			_io_glflag &= ~_JS_IONORAISE;
		else
			_io_glflag |= _JS_IONORAISE;
		return;
	}
	down(f);

	oflag = my_flag(f);

	if (flg)
		oflag &= ~_JS_IONORAISE;
	else
		oflag |= _JS_IONORAISE;

	set_my_flag(f, oflag);
}
