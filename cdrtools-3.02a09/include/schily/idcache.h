/* @(#)idcache.h	1.3 10/08/24 Copyright 1993, 1995-2010 J. Schilling */
/*
 *	Copyright (c) 1993, 1995-2010 J. Schilling
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

#ifndef	_SCHILY_IDCACHE_H
#define	_SCHILY_IDCACHE_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

extern	BOOL	ic_nameuid	__PR((char *name, int namelen, uid_t uid));
extern	BOOL	ic_uidname	__PR((char *name, int namelen, uid_t *uidp));
extern	BOOL	ic_namegid	__PR((char *name, int namelen, gid_t gid));
extern	BOOL 	ic_gidname	__PR((char *name, int namelen, gid_t *gidp));
extern	uid_t	ic_uid_nobody	__PR((void));
extern	gid_t	ic_gid_nobody	__PR((void));

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_IDCACHE_H */
