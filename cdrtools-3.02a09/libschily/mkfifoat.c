/* @(#)mkfifoat.c	1.3 13/12/26 Copyright 2013 J. Schilling */
/*
 *	Emulate the behavior of mkfifoat(int fd, const char *name, mode_t mode)
 *
 *	Note that emulation methods that do not use the /proc filesystem are
 *	not MT safe. In the non-MT-safe case, we do:
 *
 *		savewd()/fchdir()/open(name)/restorewd()
 *
 *	Errors may force us to abort the program as our caller is not expected
 *	to know that we do more than a simple open() here and that the
 *	working directory may be changed by us.
 *
 *	Copyright (c) 2013 J. Schilling
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

#include <schily/unistd.h>
#include <schily/types.h>
#include <schily/fcntl.h>
#include <schily/maxpath.h>
#include <schily/errno.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include "at-defs.h"

#ifndef	HAVE_MKFIFOAT

/* CSTYLED */
#define	PROTO_DECL	, mode_t mode
#define	KR_DECL		mode_t mode;
/* CSTYLED */
#define	KR_ARGS		, mode
#define	FUNC_CALL(n)	mkfifo(n, mode)
#define	FLAG_CHECK()
#define	FUNC_NAME	mkfifoat
#define	FUNC_RESULT	int

#include "at-base.c"

#endif	/* HAVE_FIFOAT */
