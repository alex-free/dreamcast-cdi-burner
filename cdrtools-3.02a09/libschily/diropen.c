/* @(#)diropen.c	1.2 12/12/02 Copyright 2011-2012 J. Schilling */
/*
 *	open a directory and call fdsetname() if needed
 *
 *	Copyright (c) 2011-2012 J. Schilling
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
#include <schily/errno.h>
#include <schily/standard.h>
#include <schily/schily.h>

LOCAL int	__diropen	__PR((const char *name, int oflag));

LOCAL int
__diropen(name, oflag)
	const char	*name;
		int	oflag;
{
	int	f = open(name, oflag);
#if O_DIRECTORY == 0
	struct stat sb;
#endif
	if (f < 0)
		return (f);

#if O_DIRECTORY == 0
	fstat(f, &sb);
	if (!S_ISDIR(sb.st_mode)) {
		close(f);
		seterrno(ENOTDIR);
		return (-1);
	}
#endif
#ifndef	HAVE_FCHDIR
	if (fdsetname(f, name) < 0) {
		close(f);
		f = -1;
	}
#endif
	return (f);
}

/*
 * Open for search only, which is sufficient for fchdir() and fstat().
 * This avoids permission errors when trying to open a non-readable dir.
 */
EXPORT int
diropen(name)
	const char	*name;
{
	return (__diropen(name, O_SEARCH|O_DIRECTORY|O_NDELAY));
}

/*
 * Open for reading (e.g. needed for readdir())
 */
EXPORT int
dirrdopen(name)
	const char	*name;
{
	return (__diropen(name, O_RDONLY|O_DIRECTORY|O_NDELAY));
}

EXPORT int
dirclose(f)
	int	f;
{
#ifndef	HAVE_FCHDIR
	fdclosename(f);
#endif
	return (close(f));
}
