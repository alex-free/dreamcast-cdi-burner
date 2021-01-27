/* @(#)linkat.c	1.4 16/08/10 Copyright 2011-2016 J. Schilling */
/*
 *	Emulate the behavior of linkat(int fd1, const char *name1, int fd2,
 *						const char *name2, int flag)
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
 *	Copyright (c) 2011-2016 J. Schilling
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

#ifndef	HAVE_LINKAT

#ifndef	ENAMETOOLONG
#define	ENAMETOOLONG	EINVAL
#endif

EXPORT int	linkfollow	__PR((const char *old, const char *new));
EXPORT int
linkfollow(old, new)
	const char	*old;
	const char	*new;
{
	char	buf[max(8192, PATH_MAX+1)];
	int	blen;

	buf[0] = '\0';
	if ((blen = resolvepath(old, buf, sizeof (buf))) < 0) {
		return (-1);
	} else if (blen >= sizeof (buf)) {
		seterrno(ENAMETOOLONG);
		return (-1);		/* Path too long */
	} else {
		buf[blen] = '\0'; /* Solaris syscall does not null-terminate */
	}
	return (link(buf, new));
}

#define	KR_DECL		int flag;
/* CSTYLED */
#define	KR_ARGS		, flag
#define	FUNC_CALL(n1, n2)	(flag & AT_SYMLINK_FOLLOW ? \
					linkfollow(n1, n2) : link(n1, n2))
#define	FLAG_CHECK()	if (flag & ~(AT_SYMLINK_FOLLOW)) { \
				seterrno(EINVAL);			 \
				return (-1);				 \
			}
#define	FUNC_NAME	linkat
#define	FUNC_RESULT	int

#include "at-base2.c"

#endif	/* HAVE_LINKAT */
