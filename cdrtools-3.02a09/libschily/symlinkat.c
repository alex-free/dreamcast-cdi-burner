/* @(#)symlinkat.c	1.3 13/10/30 Copyright 2013 J. Schilling */
/*
 *	Emulate the behavior of symlinkat(const char *content, int fd, const char *name)
 *
 *	Note that emulation methods that do not use the /proc filesystem are
 *	not MT safe. In the non-MT-safe case, we do:
 *
 *		savewd()/fchdir()/open(name)/restorewd()
 *
 *	Errors may force us to abort the program as our caller is not expected
 *	to know that we do more than a simple open() here and that the
 *	working directory may be changed by us.
 *
 *	Copyright (c) 2013 J. Schilling
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
#include <schily/maxpath.h>
#include <schily/errno.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include "at-defs.h"

#ifndef	HAVE_SYMLINKAT
#ifndef	HAVE_SYMLINK
/* ARGSUSED */
EXPORT int
symlinkat(content, fd, name)
	const char	*content;
	int		fd;
	const char	*name;
{
#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return (-1);
}
#else

EXPORT int	rev_symlink __PR((const char *name, const char *content));
EXPORT int	rev_symlinkat __PR((int fd, const char *name, const char *content));

EXPORT int
rev_symlink(name, content)
	const char	*name;
	const char	*content;
{
	return (symlink(content, name));
}

/* CSTYLED */
#define	PROTO_DECL	, const char *content
#define	KR_DECL		const char *content;
/* CSTYLED */
#define	KR_ARGS		, content
#define	FUNC_CALL(n)	rev_symlink(n, content)
#define	FLAG_CHECK()
#define	FUNC_NAME	rev_symlinkat
#define	FUNC_RESULT	int

#include "at-base.c"

EXPORT int
symlinkat(content, fd, name)
	const char	*content;
	int		fd;
	const char	*name;
{
	return (rev_symlinkat(fd, name, content));
}

#endif	/* HAVE_SYMLINK */
#endif	/* HAVE_SYMLINKAT */
