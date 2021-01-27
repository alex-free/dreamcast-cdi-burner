/* @(#)pwd.h	1.1 09/07/10 Copyright 2009 J. Schilling */
/*
 *	Passwd abstraction
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

#ifndef	_SCHILY_PWD_H
#define	_SCHILY_PWD_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_TYPES_H
#include <schily/types.h>
#endif

#ifdef	HAVE_PWD_H
#ifndef	_INCL_PWD_H
#include <pwd.h>
#define	_INCL_PWD_H
#endif
#else

#ifdef	__cplusplus
extern "C" {
#endif

struct passwd {
	char	*pw_name;
	char	*pw_passwd;
	uid_t	pw_uid;
	gid_t	pw_gid;
	char	*pw_gecos;
	char	*pw_dir;
	char	*pw_shell;
};

#ifdef	__cplusplus
}
#endif

#endif	/* ! HAVE_PWD_H */

#endif	/* _SCHILY_PWD_H */
