/* @(#)printf.c	1.17 09/07/10 Copyright 1985, 1989, 1995-2003 J. Schilling */
/*
 *	Copyright (c) 1985, 1989, 1995-2003 J. Schilling
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

#ifdef	printf
#	define	__no_undef__
#else
#	define	printf		__nothing__
#endif
#ifdef	fprintf
#	define	__no_undef2__
#else
#	define	fprintf		__nothing2__
#endif

#include <schily/stdio.h>
#include <schily/varargs.h>
#include <schily/standard.h>
#include <schily/schily.h>

#ifndef	__no_undef__
#	undef	printf
#endif
#ifndef	__no_undef2__
#	undef	fprintf
#endif

#define	BFSIZ	256

typedef struct {
	short	cnt;
	char	*ptr;
	char	buf[BFSIZ];
	int	count;
	FILE	*f;
} *BUF, _BUF;

LOCAL	void	_bflush	__PR((BUF));
LOCAL	void	_bput	__PR((char, long));
EXPORT	int	fprintf	__PR((FILE *, const char *, ...))	__printflike__(2, 3);
EXPORT	int	printf	__PR((const char *, ...))		__printflike__(1, 2);

LOCAL void
_bflush(bp)
	register BUF	bp;
{
	bp->count += bp->ptr - bp->buf;
	if (filewrite(bp->f, bp->buf, bp->ptr - bp->buf) < 0)
		bp->count = EOF;
	bp->ptr = bp->buf;
	bp->cnt = BFSIZ;
}

#ifdef	PROTOTYPES
LOCAL void
_bput(char c, long l)
#else
LOCAL void
_bput(c, l)
		char	c;
		long	l;
#endif
{
	register BUF	bp = (BUF)l;

	*bp->ptr++ = c;
	if (--bp->cnt <= 0)
		_bflush(bp);
}

/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT int
printf(const char *form, ...)
#else
EXPORT int
printf(form, va_alist)
	char	*form;
	va_dcl
#endif
{
	va_list	args;
	_BUF	bb;

	bb.ptr = bb.buf;
	bb.cnt = BFSIZ;
	bb.count = 0;
	bb.f = stdout;
#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	format(_bput, (long)&bb, form, args);
	va_end(args);
	if (bb.cnt < BFSIZ)
		_bflush(&bb);
	return (bb.count);
}

/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT int
fprintf(FILE *file, const char *form, ...)
#else
EXPORT int
fprintf(file, form, va_alist)
	FILE	*file;
	char	*form;
	va_dcl
#endif
{
	va_list	args;
	_BUF	bb;

	bb.ptr = bb.buf;
	bb.cnt = BFSIZ;
	bb.count = 0;
	bb.f = file;
#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	format(_bput, (long)&bb, form, args);
	va_end(args);
	if (bb.cnt < BFSIZ)
		_bflush(&bb);
	return (bb.count);
}
