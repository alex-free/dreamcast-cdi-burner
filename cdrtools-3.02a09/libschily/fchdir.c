/* @(#)fchdir.c	1.2 11/10/21 Copyright 2011 J. Schilling */
/*
 *	Emulate the behavior of fchdir(fd)
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

#include <schily/unistd.h>
#include <schily/types.h>
#include <schily/fcntl.h>
#include <schily/maxpath.h>
#include <schily/errno.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include "at-defs.h"

#ifndef	HAVE_FCHDIR

#define	DN_SIZE	256
LOCAL	char	*dnames[DN_SIZE];

EXPORT int
fchdir(fd)
	int	fd;
{
	if (fd < 0) {
		seterrno(EBADF);
		return (-1);
	}
	if (fd >= DN_SIZE) {
		seterrno(ENOTDIR);
		return (-1);
	}
	if (dnames[fd] == NULL) {
		seterrno(ENOTDIR);
		return (-1);
	}
	return (lxchdir(dnames[fd]));
}

EXPORT int
fdsetname(fd, name)
	int		fd;
	const char	*name;
{
	if (fd < 0) {
		seterrno(EBADF);
		return (-1);
	}
	if (fd >= DN_SIZE)
		return (-1);

	{
		char	buf[max(8192, PATH_MAX+1)];

		if (abspath(name, buf, sizeof (buf)) == NULL) {
			return (-1);
		}
		if (dnames[fd] != NULL)
			free(dnames[fd]);
		dnames[fd] = strdup(buf);
		if (dnames[fd] == NULL)
			return (-1);
	}

	return (0);
}

EXPORT int
fdclosename(fd)
	int	fd;
{
	if (fd < 0) {
		seterrno(EBADF);
		return (-1);
	}
	if (fd >= DN_SIZE)
		return (-1);

	if (dnames[fd] != NULL)
		free(dnames[fd]);
	dnames[fd] = NULL;
	return (0);
}

#else	/* HAVE_FCHDIR */

#undef	fdsetname
#undef	fdclosename

extern	int	fdsetname __PR((int fd, const char *name));
extern	int	fdclosename __PR((int fd));

/* ARGSUSED */
EXPORT int
fdsetname(fd, name)
		int	fd;
	const char	*name;
{
	return (0);
}

/* ARGSUSED */
EXPORT int
fdclosename(fd)
	int	fd;
{
	return (0);
}
#endif	/* HAVE_FCHDIR */
