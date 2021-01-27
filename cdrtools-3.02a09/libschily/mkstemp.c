/* @(#)mkstemp.c	1.2 14/05/15 Copyright 2011-2014 J. Schilling */
/*
 *	Copyright (c) 2011-2014 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/types.h>
#include <schily/fcntl.h>
#include <schily/stat.h>
#include <schily/errno.h>

#ifndef	HAVE_MKSTEMP
EXPORT int
mkstemp(path)
	char	*path;
{
#ifdef	HAVE_MKTEMP
	mktemp(path);
	return (open(path, O_CREAT|O_EXCL|O_RDWR, S_IRUSR|S_IWUSR));
#else
#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return (-1);
#endif
}
#endif
