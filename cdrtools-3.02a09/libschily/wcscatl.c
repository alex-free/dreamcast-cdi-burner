/* @(#)wcscatl.c	1.14 09/06/05 Copyright 1985, 1989, 1995-2009 J. Schilling */
/*
 *	list version of wcscat()
 *
 *	concatenates all past first parameter until a NULL pointer is reached
 *
 *	WARNING: a NULL constant is not a NULL pointer, so a caller must
 *		cast a NULL constant to a pointer: (wchar_t *)NULL
 *
 *	returns pointer past last character (to '\0' "char")
 *
 *	Copyright (c) 1985, 1989, 1995-2009 J. Schilling
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

#include <schily/varargs.h>
#include <schily/standard.h>
#include <schily/wchar.h>
#include <schily/schily.h>

/* VARARGS3 */
#ifdef	PROTOTYPES
EXPORT wchar_t *
wcscatl(wchar_t *to, ...)
#else
EXPORT wchar_t *
wcscatl(to, va_alist)
	wchar_t *to;
	va_dcl
#endif
{
		va_list		args;
	register wchar_t	*p;
	register wchar_t	*tor = to;

#ifdef	PROTOTYPES
	va_start(args, to);
#else
	va_start(args);
#endif
	while ((p = va_arg(args, wchar_t *)) != NULL) {
		while ((*tor = *p++) != '\0') {
			tor++;
		}
	}
	*tor = '\0';
	va_end(args);
	return (tor);
}
