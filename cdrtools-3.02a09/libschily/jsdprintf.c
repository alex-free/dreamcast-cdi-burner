/* @(#)jsdprintf.c	1.2 17/08/03 Copyright 1985, 1989, 1995-2017 J. Schilling */
/*
 *	Copyright (c) 1985, 1989, 1995-2017 J. Schilling
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
#include <schily/stdio.h>
#include <schily/varargs.h>
#include <schily/standard.h>
#include <schily/schily.h>

#define	BFSIZ	256

typedef struct {
	short	cnt;
	char	*ptr;
	char	buf[BFSIZ];
	int	count;
	int	fd;
} *BUF, _BUF;

LOCAL	void	_bflush	__PR((BUF));
LOCAL	void	_bput	__PR((char, void *));
EXPORT	int	js_dprintf __PR((int, const char *, ...))	__printflike__(2, 3);

LOCAL void
_bflush(bp)
	register BUF	bp;
{
	bp->count += bp->ptr - bp->buf;
	if (write(bp->fd, bp->buf, bp->ptr - bp->buf) < 0)
		bp->count = EOF;
	bp->ptr = bp->buf;
	bp->cnt = BFSIZ;
}

#ifdef	PROTOTYPES
LOCAL void
_bput(char c, void *l)
#else
LOCAL void
_bput(c, l)
		char	c;
		void	*l;
#endif
{
	register BUF	bp = (BUF)l;

	*bp->ptr++ = c;
	if (--bp->cnt <= 0)
		_bflush(bp);
}

/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT int
js_dprintf(int fd, const char *form, ...)
#else
EXPORT int
js_dprintf(fd, form, va_alist)
	int	fd;
	char	*form;
	va_dcl
#endif
{
	va_list	args;
	_BUF	bb;

	bb.ptr = bb.buf;
	bb.cnt = BFSIZ;
	bb.count = 0;
	bb.fd = fd;
#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	format(_bput, &bb, form, args);
	va_end(args);
	if (bb.cnt < BFSIZ)
		_bflush(&bb);
	return (bb.count);
}
