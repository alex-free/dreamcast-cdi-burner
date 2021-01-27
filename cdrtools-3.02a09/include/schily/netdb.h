/* @(#)netdb.h	1.2 15/11/30 Copyright 2009-2015 J. Schilling */
/*
 *	Netdb abstraction
 *
 *	Copyright (c) 2009-2015 J. Schilling
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

#ifndef	_SCHILY_NETDB_H
#define	_SCHILY_NETDB_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_NETDB_H
#ifndef	_INCL_NETDB_H
#include <netdb.h>
#define	_INCL_NETDB_H
#endif
#endif

#ifdef	HAVE_ARPA_AIXRCMDS_H
#ifndef	_INCL_ARPA_AIXRCMDS_H
#include <arpa/aixrcmds.h>
#define	_INCL_ARPA_AIXRCMDS_H
#endif
#endif

#endif	/* _SCHILY_NETDB_H */
