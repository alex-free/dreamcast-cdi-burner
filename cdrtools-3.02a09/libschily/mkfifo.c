/* @(#)mkfifo.c	1.1 13/10/27 Copyright 2013 J. Schilling */
/*
 *	Emulate the behavior of mkfifo(const char *name, mode_t mode)
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
#include <schily/time.h>
#include <schily/fcntl.h>
#include <schily/stat.h>
#include <schily/errno.h>
#include <schily/standard.h>
#include <schily/schily.h>

#ifndef	HAVE_MKFIFO

#define	S_ALLPERM	(S_IRWXU|S_IRWXG|S_IRWXO)
#define	S_ALLFLAGS	(S_ISUID|S_ISGID|S_ISVTX)
#define	S_ALLMODES	(S_ALLFLAGS | S_ALLPERM)

#ifdef	PROTOTYPES
EXPORT int
mkfifo(const char *name, mode_t mode)
#else
EXPORT int
mkfifo(name, mode)
	const char		*name;
	mode_t			mode;
#endif
{
#ifdef	HAVE_MKNOD
	return (mknod(name, S_IFIFO | (mode & S_ALLMODES), 0));
#else
#ifdef	ENOSYS
	seterrno(ENOSYS);
#else
	seterrno(EINVAL);
#endif
	return (-1);
#endif
}

#endif	/* HAVE_MKFIFO */
