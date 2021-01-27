/* @(#)ioctl.h	1.1 07/05/24 Copyright 2007 J. Schilling */
/*
 *	Abstraction from sys/ioctl.h
 *
 *	Copyright (c) 2007 J. Schilling
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

#ifndef _SCHILY_IOCTL_H
#define	_SCHILY_IOCTL_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_SYS_IOCTL_H
#ifndef	_INCL_SYS_IOCTL_H
#include <sys/ioctl.h>
#define	_INCL_SYS_IOCTL_H
#endif
#endif

#endif	/* _SCHILY_IOCTL_H */
