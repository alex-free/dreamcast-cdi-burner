/* @(#)matchmbl.c	1.1 17/07/02 Copyright 2017 J. Schilling */
/*
 *	Pattern matching function for multi byte line match (grep like).
 *
 *	Copyright (c) 2017 J. Schilling
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

#define	USE_WCHAR
#include <schily/wchar.h>

#define	__MB_CHAR
#define	__LINE_MATCH
#include "match.c"
