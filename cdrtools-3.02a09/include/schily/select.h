/* @(#)select.h	1.1 09/08/04 Copyright 2009 J. Schilling */
/*
 *	Select abstraction
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

#ifndef	_SCHILY_SELECT_H
#define	_SCHILY_SELECT_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_TYPES_H
#include <schily/types.h>
#endif
#ifndef _SCHILY_TIME_H
#include <schily/time.h>
#endif

#ifdef	HAVE_SYS_SELECT_H
#ifndef	_INCL_SYS_SELECT_H
#include <sys/select.h>
#define	_INCL_SYS_SELECT_H
#endif
#endif

/*
 * Some systems need sys/socket.h instead or in addition to sys/select.h
 */
#ifdef	NEED_SYS_SOCKET_H
#ifndef _SCHILY_SOCKET_H
#include <schily/socket.h>
#endif
#endif

#endif	/* _SCHILY_SELECT_H */
