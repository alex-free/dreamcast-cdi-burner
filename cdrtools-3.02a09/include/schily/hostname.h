/* @(#)hostname.h	1.22 14/01/01 Copyright 1995-2014 J. Schilling */
/*
 *	This file has been separated from libport.h in order to avoid
 *	to include netdb.h in case gethostname() is not needed.
 *
 *	Copyright (c) 1995-2014 J. Schilling
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


#ifndef _SCHILY_HOSTNAME_H
#define	_SCHILY_HOSTNAME_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_TYPES_H
#include <schily/types.h>
#endif

/*
 * Try to get HOST_NAME_MAX for gethostname() from unistd.h or limits.h
 */
#ifndef _SCHILY_UNISTD_H
#include <schily/unistd.h>
#endif
#ifndef _SCHILY_LIMITS_H
#include <schily/limits.h>
#endif

#ifndef HOST_NAME_MAX
#if	defined(HAVE_NETDB_H) && !defined(HOST_NOT_FOUND) && \
				!defined(_INCL_NETDB_H)
#ifndef	_SCHILY_NETDB_H
#include <schily/netdb.h>	/* #defines MAXHOSTNAMELEN */
#endif
#endif
#ifdef	MAXHOSTNAMELEN
#define	HOST_NAME_MAX	MAXHOSTNAMELEN
#endif
#endif

#ifndef HOST_NAME_MAX
#ifndef	_SCHILY_PARAM_H
#include <schily/param.h>	/* Include various defs needed with some OS */
#endif				/* Linux MAXHOSTNAMELEN */
#ifdef	MAXHOSTNAMELEN
#define	HOST_NAME_MAX	MAXHOSTNAMELEN
#endif
#endif

#ifndef HOST_NAME_MAX
#define	HOST_NAME_MAX	255
#endif
#ifndef	MAXHOSTNAMELEN
#define	MAXHOSTNAMELEN	HOST_NAME_MAX
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * The gethostname() prototype from Win-DOS are in the wrong include files
 * (winsock*.h instead of unistd.h) and they are in conflict with the standard.
 * Because the prototype is in the wrong file and the function is not in libc,
 * configure will believe gethostname() is missing.
 * We cannot define our own correct prototype, but this is not a problem as the
 * return type is int.
 */
#if !defined(__MINGW32__)
#ifndef	HAVE_GETHOSTNAME
extern	int		gethostname	__PR((char *name, int namelen));
#endif
#endif
#ifndef	HAVE_GETDOMAINNAME
extern	int		getdomainname	__PR((char *name, int namelen));
#endif

#ifdef	__SUNOS4
/*
 * Define prototypes for POSIX standard functions that are missing on SunOS-4.x
 * to make compilation smooth.
 */
extern	int		gethostname	__PR((char *name, int namelen));
extern	int		getdomainname	__PR((char *name, int namelen));
#endif	/* __SUNOS4 */

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_HOSTNAME_H */
