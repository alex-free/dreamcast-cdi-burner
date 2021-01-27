/* @(#)openat.c	1.2 11/10/22 Copyright 2011 J. Schilling */
/*
 *	Emulate the behavior of openat(fd, name, flag, mode)
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
#include <schily/varargs.h>
#include <schily/maxpath.h>
#include <schily/errno.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include "at-defs.h"

#ifndef	HAVE_OPENAT

#ifndef	ENAMETOOLONG
#define	ENAMETOOLONG	EINVAL
#endif

/* VARARGS3 */
#ifdef  PROTOTYPES
EXPORT int
openat(int fd, const char *name, int oflag, ...)
#else
EXPORT int
openat(fd, name, oflag, va_alist)
	int		fd;
	const char	*name;
	int		oflag;
	va_dcl
#endif
{
	va_list	args;
	mode_t	omode = 0;
	int	n;
	int	ret;
	char	buf[PATH_MAX];
	char	*proc_name;
	struct save_wd save_wd;

	if (oflag & O_CREAT) {
#ifdef	PROTOTYPES
		va_start(args, oflag);
#else
		va_start(args);
#endif
		/*
		 * If sizeof (mode_t) is < sizeof (int) and used with va_arg(),
		 * GCC4 will abort the code. So we need to use the promoted
		 * size.
		 */
		omode = va_arg(args, PROMOTED_MODE_T);
		va_end(args);
	}
	if (fd == AT_FDCWD || ABS_NAME(name))
		return (open(name, oflag, omode));

	if ((proc_name = proc_fd2name(buf, fd, name)) != NULL) {
		ret = open(proc_name, oflag, omode);
		if (ret >= 0 || NON_PROCFS_ERRNO(errno))
			return (ret);
	} else if (geterrno() == ENAMETOOLONG) {
		return (-1);
	}

	if (savewd(&save_wd) < 0) {
		/*
		 * We abort here as the caller may not know that we are forced
		 * to savewd/fchdir/restorewd here and misinterpret errno.
		 */
		savewd_abort(geterrno());
		/* NOTREACHED */
		return (-1);
	}
	if (fd >= 0 && save_wd.fd == fd) {
		/*
		 * If we just opened "fd" with the same number in savewd(), fd
		 * must have been illegal when calling openat();
		 */
		closewd(&save_wd);
		seterrno(EBADF);
		return (-1);
	}
	if ((n = fchdir(fd)) < 0) {
		int	err = geterrno();
		/*
		 * In case that fchdir() is emulated via chdir() and we use a
		 * multi hop chdir(), we may be on an undefined intermediate
		 * directory. Try to return to last working directory and if
		 * this fails, abort for security.
		 */
		if (n == -2 && restorewd(&save_wd) < 0) {
			restorewd_abort(geterrno());
		}
		closewd(&save_wd);
		seterrno(err);
		return (-1);
	}

	ret = open(name, oflag, omode);		/* The actual open() */

	if (restorewd(&save_wd) < 0) {
		int	err = geterrno();

		close(ret);
		closewd(&save_wd);
		restorewd_abort(err);
		seterrno(err);
		return (-1);
	}
	closewd(&save_wd);

	return (ret);
}

#endif	/* HAVE_OPENAT */
