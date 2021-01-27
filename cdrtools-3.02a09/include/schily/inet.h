/* @(#)inet.h	1.3 13/07/08 Copyright 2009-2013 J. Schilling */
/*
 *	Inet abstraction
 *
 *	Copyright (c) 2009-2013 J. Schilling
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

#ifndef	_SCHILY_INET_H
#define	_SCHILY_INET_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_ARPA_INET_H
#ifndef	_INCL_ARPA_INET_H
#include <arpa/inet.h>
#define	_INCL_ARPA_INET_H
#endif
#else	/* !HAVE_ARPA_INET_H */

/*
 * BeOS does not have <arpa/inet.h>
 * but inet_ntaoa() is in <netdb.h>
 */
#ifdef	HAVE_NETDB_H
#ifndef	_SCHILY_NETDB_H
#include <schily/netdb.h>
#endif
#endif	/* HAVE_NETDB_H */

#endif	/* !HAVE_ARPA_INET_H */

#endif	/* _SCHILY_INET_H */
