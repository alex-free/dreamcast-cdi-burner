/* @(#)dirname.c	1.1 10/05/08 Copyright 2010 J. Schilling */
/*
 *	dirname() to be used if missing in libc
 *
 *	Copyright (c) 2010 J. Schilling
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

#include <schily/libgen.h>
#include <schily/string.h>
#include <schily/schily.h>

#ifndef	HAVE_DIRNAME

EXPORT char *
dirname(name)
	char	*name;
{
	char	*n;

	if (name == 0 || *name == '\0')
		return (".");

	n = name + strlen(name);
	while (n != name && *--n == '/')
		;

	if (n == name && *n == '/')
		return ("/");

	while (n != name) {
		if (*--n == '/') {
			while (*n == '/' && n != name)
				n--;
			*++n = '\0';
			return (name);
		}
	}
	return (".");
}

#endif
