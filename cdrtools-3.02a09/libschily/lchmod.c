/* @(#)lchmod.c	1.1 14/06/03 Copyright 2014 J. Schilling */
/*
 *	Emulate the behavior of lchmod(const char *name, mode_t mode)
 *
 *	Copyright (c) 2014 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/unistd.h>
#include <schily/types.h>
#include <schily/fcntl.h>
#include <schily/stat.h>
#include <schily/maxpath.h>
#include <schily/errno.h>
#include <schily/standard.h>
#include <schily/schily.h>

#ifndef	HAVE_LCHMOD
#ifdef	PROTOTYPES
EXPORT int
lchmod(const char *name, mode_t mode)
#else
EXPORT int
lchmod(name, mode)
	const char	*name;
	mode_t		mode;
#endif
{
	struct stat	statbuf;

	statbuf.st_mode = 0;
	fstatat(AT_FDCWD, name, &statbuf, AT_SYMLINK_NOFOLLOW);

#ifdef	HAVE_CHMOD
	if (!S_ISLNK(statbuf.st_mode))
		return (chmod(name, mode));

#else	/* !HAVE_CHMOD */
	if (!S_ISLNK(statbuf.st_mode)) {
#ifdef	ENOSYS
		seterrno(ENOSYS);
#else
		seterrno(EINVAL);
#endif
		return (-1);
	}
#endif	/* !HAVE_CHMOD */

#ifdef	EOPNOTSUPP
	seterrno(EOPNOTSUPP);
#else
	seterrno(EINVAL);
#endif
	return (-1);
}
#endif	/* HAVE_LCHMOD */
