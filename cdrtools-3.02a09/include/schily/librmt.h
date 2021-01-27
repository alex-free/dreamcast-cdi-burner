/* @(#)librmt.h	1.22 10/08/27 Copyright 1995,1996,2000-2010 J. Schilling */
/*
 *	Prototypes for rmt client subroutines
 *
 *	Copyright (c) 1995,1996,2000-2010 J. Schilling
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

#ifndef	_SCHILY_LIBRMT_H
#define	_SCHILY_LIBRMT_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>
#endif

#include <schily/rmtio.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * remote.c
 */
extern	void		rmtinit		__PR((int (*errmsgn)
						(int, const char *, ...),
						void (*eexit)(int)));
extern	int		rmtdebug	__PR((int dlevel));
extern	char		*rmtfilename	__PR((char *name));
extern	char		*rmthostname	__PR((char *hostname, int hnsize,
						char *rmtspec));
extern	int		rmtgetconn	__PR((char *host, int trsize,
						int excode));
extern	int		rmtopen		__PR((int fd, char *fname, int fmode));
extern	int		rmtclose	__PR((int fd));
extern	int		rmtread		__PR((int fd, char *buf, int count));
extern	int		rmtwrite	__PR((int fd, char *buf, int count));
extern	off_t		rmtseek		__PR((int fd, off_t offset,
						int whence));
extern	int		rmtioctl	__PR((int fd, int cmd, int count));
#ifdef	MTWEOF
extern	int		rmtstatus	__PR((int fd, struct mtget *mtp));
#endif
extern	int		rmtxstatus	__PR((int fd, struct rmtget *mtp));
#ifdef	MTWEOF
extern	void		_rmtg2mtg	__PR((struct mtget *mtp,
						struct rmtget *rmtp));
extern	int		_mtg2rmtg	__PR((struct rmtget *rmtp,
						struct mtget *mtp));
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_LIBRMT_H */
