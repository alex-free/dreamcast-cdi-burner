/* @(#)wdabort.c	1.1 11/10/21 Copyright 2011 J. Schilling */
/*
 *	Functions to abort after problems in directory handling.
 *
 *	Copyright (c) 2011 J. Schilling
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

#include <schily/standard.h>
#include <schily/schily.h>
#include "at-defs.h"

void
savewd_abort(err)
	int	err;
{
	comerrno(err, "Cannot save current working directory.\n");
}

void
fchdir_abort(err)
	int	err;
{
	comerrno(err, "Cannot completely chdir() to new directory.\n");
}

void
restorewd_abort(err)
	int	err;
{
	comerrno(err, "Cannot restore current working directory.\n");
}
