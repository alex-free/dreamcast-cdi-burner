/* @(#)snprintf.c	1.14 16/08/10 Copyright 1985, 1996-2016 J. Schilling */
/*
 *	Copyright (c) 1985, 1996-2016 J. Schilling
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

#define	snprintf __nothing__	/* prototype may be wrong (e.g. IRIX) */
#include <schily/mconfig.h>
#include <schily/unistd.h>	/* include <sys/types.h> try to get size_t */
#include <schily/stdio.h>	/* Try again for size_t	*/
#include <schily/stdlib.h>	/* Try again for size_t	*/
#include <schily/varargs.h>
#include <schily/standard.h>
#include <schily/schily.h>
#undef	snprintf

/*
 * If PORT_ONLY is defined, snprintf() will only be compiled in if it is
 * missing on the local platform. This is used by e.g. libschily.
 */
#if	!defined(HAVE_SNPRINTF) || !defined(PORT_ONLY)

EXPORT	int snprintf __PR((char *, size_t maxcnt, const char *, ...));

typedef struct {
	char	*ptr;
	int	count;
} *BUF, _BUF;

#ifdef	PROTOTYPES
static void
_cput(char c, long l)
#else
static void
_cput(c, l)
	char	c;
	long	l;
#endif
{
	register BUF	bp = (BUF)l;

	if (--bp->count > 0) {
		*bp->ptr++ = c;
	} else {
		/*
		 * Make sure that there will never be a negative overflow.
		 */
		bp->count++;
	}
}

/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT int
snprintf(char *buf, size_t maxcnt, const char *form, ...)
#else
EXPORT int
snprintf(buf, maxcnt, form, va_alist)
	char	*buf;
	unsigned maxcnt;
	char	*form;
	va_dcl
#endif
{
	va_list	args;
	int	cnt;
	_BUF	bb;

	bb.ptr = buf;
	bb.count = maxcnt;

#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	cnt = format(_cput, (long)&bb, form,  args);
	va_end(args);
	if (maxcnt > 0)
		*(bb.ptr) = '\0';
	if (bb.count < 0)
		return (-1);

	return (cnt);
}

#endif	/* !defined(HAVE_SNPRINTF) || !defined(PORT_ONLY) */
