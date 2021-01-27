/* @(#)grp.h	1.1 09/07/10 Copyright 2009 J. Schilling */
/*
 *	Group abstraction
 *
 *	Copyright (c) 2009 J. Schilling
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

#ifndef	_SCHILY_GRP_H
#define	_SCHILY_GRP_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_TYPES_H
#include <schily/types.h>
#endif

#ifdef	HAVE_GRP_H
#ifndef	_INCL_GRP_H
#include <grp.h>
#define	_INCL_GRP_H
#endif
#else

#ifdef	__cplusplus
extern "C" {
#endif

struct	group {
	char	*gr_name;
	char	*gr_passwd;
	gid_t	gr_gid;
	char	**gr_mem;
};

#ifdef	__cplusplus
}
#endif

#endif	/* ! HAVE_GRP_H */

#endif	/* _SCHILY_GRP_H */
