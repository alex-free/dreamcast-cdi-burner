/* @(#)at-defs.h	1.1 11/10/21 Copyright 2011 J. Schilling */
/*
 *	Libschily internal definitions for openat() emulation
 *	and related functions.
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
#ifndef	_AT_DEFS_H

/*
 * This is a list of errors that are expected to be not a result of
 * a /proc fs problem. If such an error is encountered, then we return
 * after the open() or other call without trying to emulate the *at()
 * interface via savewd()/fchdir()/doit()/restorewd().
 */
#ifdef	ENOSYS
#define	__ENOSYS	ENOSYS
#else
#define	__ENOSYS	ENOENT
#endif
#ifdef	EOPNOTSUPP
#define	__EOPNOTSUPP	EOPNOTSUPP
#else
#define	__EOPNOTSUPP	ENOENT
#endif

#define	NON_PROCFS_ERRNO(e)					\
			((e) == ENOENT || (e) == ENOTDIR ||	\
			(e) == EACCES || (e) == EPERM ||	\
			(e) == __ENOSYS /* Solaris */ ||	\
			(e) == __EOPNOTSUPP /* FreeBSD */)

/*
 * n refers to an absolute path name.
 */
#define	ABS_NAME(n)	((n)[0] == '/')

#ifdef	min
#undef	min
#endif
#define	min(a, b)	((a) < (b) ? (a):(b))

#ifdef	max
#undef	max
#endif
#define	max(a, b)	((a) < (b) ? (b):(a))

/*
 * From procnameat.c
 */
extern char	*proc_fd2name	__PR((char *buf, int fd, const char *name));

/*
 * From wdabort.c
 */
extern	void	savewd_abort	__PR((int err));
extern	void	fchdir_abort	__PR((int err));
extern	void	restorewd_abort	__PR((int err));

#endif	/* _AT_DEFS_H */
