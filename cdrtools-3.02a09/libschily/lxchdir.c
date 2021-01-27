/* @(#)lxchdir.c	1.1 11/10/21 Copyright 2004-2011 J. Schilling */
/*
 *	Long path name aware chdir().
 *
 *	The code has been adopted from libfind.
 *
 *	Copyright (c) 2004-2011 J. Schilling
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

#include <schily/unistd.h>
#include <schily/types.h>
#include <schily/fcntl.h>
#include <schily/maxpath.h>
#include <schily/string.h>
#include <schily/errno.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include "at-defs.h"

#ifndef	ENAMETOOLONG
#define	ENAMETOOLONG	EINVAL
#endif

EXPORT	int	lxchdir	__PR((char *p));
LOCAL	int	xchdir	__PR((char *p));

EXPORT int
lxchdir(p)
	char	*p;
{
	if (chdir(p) < 0) {
		if (geterrno() != ENAMETOOLONG)
			return (-1);

		return (xchdir(p));
	}
	return (0);
}

LOCAL int
xchdir(p)
	char	*p;
{
	char	*p2;
	BOOL	first = TRUE;

	while (*p) {
		if ((p2 = strchr(p, '/')) != NULL)
			*p2 = '\0';

		if (first && p2 && p[0] == '\0') {
			if (chdir("/") < 0)
				return (-1);
		} else {
			/*
			 * If this is not the first chdir() and we are doing a
			 * multi hop chdir(), we may be on an undefined
			 * intermediate directory. Mark this case by returning
			 * -2 instead of -1.
			 */
			if (chdir(p) < 0)
				return (first?-1:-2);
		}
		if (p2 == NULL)
			break;
		*p2 = '/';
		p = &p2[1];
		first = FALSE;
	}
	return (0);
}
