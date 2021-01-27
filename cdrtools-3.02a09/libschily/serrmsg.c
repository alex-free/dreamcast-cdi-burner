/* @(#)serrmsg.c	1.5 09/07/10 Copyright 1985, 2000-2003 J. Schilling */
/*
 *	Routines for printing command errors
 *
 *	Copyright (c) 1985, 2000-2003 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/unistd.h>	/* include <sys/types.h> try to get size_t */
#include <schily/stdio.h>	/* Try again for size_t	*/
#include <schily/stdlib.h>	/* Try again for size_t	*/
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/varargs.h>
#include <schily/string.h>
#include <schily/schily.h>
#ifndef	HAVE_STRERROR
extern	char	*sys_errlist[];
extern	int	sys_nerr;
#endif

EXPORT	int	serrmsg		__PR((char *buf, size_t maxcnt, const char *, ...));
EXPORT	int	serrmsgno	__PR((int, char *buf, size_t maxcnt, const char *, ...));
LOCAL	int	_serrmsg	__PR((int, char *buf, size_t maxcnt, const char *, va_list));

/* VARARGS1 */
EXPORT int
#ifdef	PROTOTYPES
serrmsg(char *buf, size_t maxcnt, const char *msg, ...)
#else
serrmsg(buf, maxcnt, msg, va_alist)
	char	*buf;
	size_t	maxcnt;
	char	*msg;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	ret = _serrmsg(geterrno(), buf, maxcnt, msg, args);
	va_end(args);
	return (ret);
}

/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT int
serrmsgno(int err, char *buf, size_t maxcnt, const char *msg, ...)
#else
serrmsgno(err, buf, maxcnt, msg, va_alist)
	int	err;
	char	*buf;
	size_t	maxcnt;
	char	*msg;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, msg);
#else
	va_start(args);
#endif
	ret = _serrmsg(err, buf, maxcnt, msg, args);
	va_end(args);
	return (ret);
}

LOCAL int
_serrmsg(err, buf, maxcnt, msg, args)
	int		err;
	char		*buf;
	size_t		maxcnt;
	const char	*msg;
	va_list		args;
{
	int	ret;
	char	errbuf[20];
	char	*errnam;
	char	*prognam = get_progname();

	if (err < 0) {
		ret = js_snprintf(buf, maxcnt, "%s: %r", prognam, msg, args);
	} else {
		errnam = errmsgstr(err);
		if (errnam == NULL) {
			(void) js_snprintf(errbuf, sizeof (errbuf),
						"Error %d", err);
			errnam = errbuf;
		}
		ret = js_snprintf(buf, maxcnt,
				"%s: %s. %r", prognam, errnam, msg, args);
	}
	return (ret);
}
