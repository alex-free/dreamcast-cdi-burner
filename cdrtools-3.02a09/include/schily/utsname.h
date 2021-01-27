/* @(#)utsname.h	1.3 11/08/05 Copyright 2009-2011 J. Schilling */
/*
 *	Utsname abstraction
 *
 *	Copyright (c) 2009-2011 J. Schilling
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

#ifndef	_SCHILY_UTSNAME_H
#define	_SCHILY_UTSNAME_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

/*
 * NeXT Step has sys/utsname but not uname()
 */
#ifdef	HAVE_SYS_UTSNAME_H
#ifndef	_INCL_SYS_UTSNAME_H
#define	_INCL_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#else	/* !HAVE_SYS_UTSNAME_H */

#ifdef	__cplusplus
extern "C" {
#endif

#ifndef	SYS_NMLN
#define	SYS_NMLN	257
#endif

struct utsname {
	char	sysname[SYS_NMLN];	/* Name of this OS		*/
	char	nodename[SYS_NMLN];	/* Name of this network node	*/
	char	release[SYS_NMLN];	/* Release level		*/
	char	version[SYS_NMLN];	/* Version level		*/
	char	machine[SYS_NMLN];	/* hardware type		*/
};

#ifndef	HAVE_UNAME
extern	int	uname	__PR((struct utsname *));
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* !HAVE_SYS_UTSNAME_H */

#endif	/* _SCHILY_UTSNAME_H */
