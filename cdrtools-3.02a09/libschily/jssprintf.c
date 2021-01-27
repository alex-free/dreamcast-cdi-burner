/* @(#)jssprintf.c	1.19 17/08/03 Copyright 1985, 1995-2017 J. Schilling */
/*
 *	Copyright (c) 1985, 1995-2017 J. Schilling
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
#include <schily/standard.h>
#include <schily/schily.h>

/*
 * Do not include stdio.h, BSD systems define sprintf the wrong way!
 */
EXPORT	int js_sprintf __PR((char *, const char *, ...));

#ifdef	PROTOTYPES
static void
_cput(char c, void *ba)
#else
static void
_cput(c, ba)
	char	c;
	void	*ba;
#endif
{
	*(*(char **)ba)++ = c;
}

/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT int
js_sprintf(char *buf, const char *form, ...)
#else
EXPORT int
js_sprintf(buf, form, va_alist)
	char	*buf;
	char	*form;
	va_dcl
#endif
{
	va_list	args;
	int	cnt;
	char	*bp = buf;

#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	cnt = format(_cput, &bp, form,  args);
	va_end(args);
	*bp = '\0';

	return (cnt);
}
