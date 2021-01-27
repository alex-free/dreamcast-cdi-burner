/* @(#)systeminfo.h	1.1 09/07/14 Copyright 2009 J. Schilling */
/*
 *	Systeminfo abstraction
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

#ifndef	_SCHILY_SYSTEMINFO_H
#define	_SCHILY_SYSTEMINFO_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_SYS_SYSTEMINFO_H
#ifndef	_INCL_SYS_SYSTEMINFO_H
#define	_INCL_SYS_SYSTEMINFO_H
#include <sys/systeminfo.h>
#endif
#endif

#endif	/* _SCHILY_SYSTEMINFO_H */
