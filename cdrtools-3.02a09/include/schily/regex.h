/* @(#)regex.h	1.1 10/05/07 Copyright 2010 J. Schilling */
/*
 *	Abstraction from regex.h
 *
 *	Copyright (c) 2010 J. Schilling
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

#ifndef	_SCHILY_REGEX_H
#define	_SCHILY_REGEX_H

#ifndef	_SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_REGEX_Ha
#ifndef	_INCL_REGEX_H
#define	_INCL_REGEX_H
#include <regex.h>
#endif
#else
#include <schily/_regex.h>
#endif


#endif	/* _SCHILY_REGEX_H */
