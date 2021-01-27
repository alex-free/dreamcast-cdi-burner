/* @(#)fprformat.c	1.4 16/01/18 Copyright 2013-2016 J. Schilling */
/*
 *	fprformat
 *	common code for printf fprintf & sprintf
 *	This is the variant that uses stdio und directly calls putc.
 *	If putc() is a FILE * derived macro, then a printf() based on this
 * 	variant is faster than a format() based printf that needs to call a
 *	function for every character in the output.
 *
 *	allows recursive printf with "%r", used in:
 *	error, comerr, comerrno, errmsg, errmsgno and the like
 *
 *	Copyright (c) 2013-2016 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/varargs.h>
#include <schily/string.h>
#include <schily/stdlib.h>
#ifdef	DEBUG
#include <schily/unistd.h>
#endif
#include <schily/standard.h>
#include <schily/utypes.h>
#define	__EXTENSIONS__		/* Enable putc_unlocked() macro on Solaris */
#define	FAST_GETC_PUTC		/* Enable stdio extensions in schily/stdio.h */
#include <schily/stdio.h>
#include <schily/schily.h>	/* Must be past stdio.h */

#define	FORMAT_FUNC_NAME	fprformat
#define	FORMAT_FUNC_PROTO_DECL
#define	FORMAT_FUNC_KR_DECL
#define	FORMAT_FUNC_KR_ARGS
#undef	FORMAT_FUNC_PARM
#ifdef	HAVE_PUTC_UNLOCKED
#define	ofun(c, fp)		putc_unlocked(c, (FILE *)fp)
#else
#define	ofun(c, fp)		putc(c, (FILE *)fp)
#endif

/*
 * If enabled, an internally buffered version of fprformat() is implemented
 * that avoids completely unbuffered printing on onubuffered FILE pointers.
 * This is 15% slower than the version that directly calls putc_unlocked()
 * but it works on any OS.
 */
#define	FORMAT_BUFFER

#include "format.c"
