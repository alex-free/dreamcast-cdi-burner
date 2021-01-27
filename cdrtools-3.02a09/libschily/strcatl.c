/* @(#)strcatl.c	1.13 06/09/13 Copyright 1985, 1989, 1995-2003 J. Schilling */
/*
 *	list version of strcat()
 *
 *	concatenates all past first parameter until a NULL pointer is reached
 *
 *	WARNING: a NULL constant is not a NULL pointer, so a caller must
 *		cast a NULL constant to a pointer: (char *)NULL
 *
 *	returns pointer past last character (to '\0' byte)
 *
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
#include <schily/varargs.h>
#include <schily/standard.h>
#include <schily/schily.h>

/* VARARGS3 */
#ifdef	PROTOTYPES
EXPORT char *
strcatl(char *to, ...)
#else
EXPORT char *
strcatl(to, va_alist)
	char *to;
	va_dcl
#endif
{
		va_list	args;
	register char	*p;
	register char	*tor = to;

#ifdef	PROTOTYPES
	va_start(args, to);
#else
	va_start(args);
#endif
	while ((p = va_arg(args, char *)) != NULL) {
		while ((*tor = *p++) != '\0') {
			tor++;
		}
	}
	*tor = '\0';
	va_end(args);
	return (tor);
}
