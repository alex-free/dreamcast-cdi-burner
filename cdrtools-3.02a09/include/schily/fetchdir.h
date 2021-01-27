/* @(#)fetchdir.h	1.8 16/03/10 Copyright 2002-2016 J. Schilling */
/*
 *	Copyright (c) 2002-2016 J. Schilling
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

#ifndef _SCHILY_FETCHDIR_H
#define	_SCHILY_FETCHDIR_H

#ifndef	_SCHILY_DIRENT_H
#include <schily/dirent.h>		/* Includes mconfig.h if needed */
#endif

/*
 * The original value used to fill the info byte before each name was ^A (1).
 * We thus need to make this value the "unknown" type value and this  finally
 * results in the FDT_* values be 1 + the UNIX file type value.
 */
#define	FDT_UNKN	 1
#define	FDT_FIFO	 2
#define	FDT_CHR		 3
#define	FDT_DIR		 5
#define	FDT_BLK		 7
#define	FDT_REG		 9
#define	FDT_LNK		11
#define	FDT_SOCK	13
#define	FDT_WHT		15

#ifdef	__cplusplus
extern "C" {
#endif

extern	char	*fetchdir	__PR((char *dir, int *entp, int *lenp,
					ino_t **inop));
extern	char	*dfetchdir	__PR((DIR *dir, char *__dirname, int *entp,
					int *lenp, ino_t **inop));
extern	int	fdircomp	__PR((const void *p1, const void *p2));
extern	char	**sortdir	__PR((char *dir, int *entp));
extern	int	cmpdir		__PR((int ents1, int ents2,
					char **ep1, char **ep2,
					char **oa, char **od,
					int *alenp, int *dlenp));

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_FETCHDIR_H */
