/* @(#)find_misc.h	1.7 09/07/10 Copyright 2004-2009 J. Schilling */
/*
 *	Copyright (c) 2004-2009 J. Schilling
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

#ifndef	_FIND_MISC_H
#define	_FIND_MISC_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_UNIXSTD_H
#include <schily/unistd.h>
#endif
#ifndef _SCHILY_STDIO_H
#include <schily/stdio.h>
#endif

#if	defined(UNIXWARE) && defined(HAVE_ACL)
#	define	HAVE_SUN_ACL
#	define	HAVE_ANY_ACL
#endif

#ifdef	USE_ACL
/*
 * HAVE_ANY_ACL currently includes HAVE_POSIX_ACL and HAVE_SUN_ACL.
 * HAVE_HP_ACL is currently not included in HAVE_ANY_ACL.
 */
#	ifndef	HAVE_ANY_ACL
#	undef	USE_ACL		/* Do not try to get or set ACLs */
#	endif
#endif

#ifdef	USE_XATTR
#ifndef	_PC_XATTR_EXISTS
#undef	USE_XATTR
#endif
#endif

extern	BOOL	has_acl		__PR((FILE *f, char *name, char *sname, struct stat *sp));
extern	BOOL	has_xattr	__PR((FILE *f, char *sname));

#endif	/* _FIND_MISC_H */
