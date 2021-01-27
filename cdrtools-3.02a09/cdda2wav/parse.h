/* @(#)parse.h	1.1 08/06/24 Copyright 2008 J. Schilling */
/*
 *	Interactive command parser for cdda2wav
 *
 *	Copyright (c) 2008 J. Schilling
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

#ifndef	_PARSE_H
#define	_PARSE_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

EXPORT	int	parse		__PR((long *lp));

#endif	/* _PARSE_H */
