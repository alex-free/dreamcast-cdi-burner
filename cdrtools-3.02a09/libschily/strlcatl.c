/* @(#)strlcatl.c	1.1 15/03/03 Copyright 2015 J. Schilling */
/*
 *	list version of strlcat()
 *
 *	concatenates all past first parameter until a NULL pointer is reached
 *
 *	WARNING: a NULL constant is not a NULL pointer, so a caller must
 *		cast a NULL constant to a pointer: (char *)NULL
 *
 *	returns number of bytes needed to fit all
 *
 *	Copyright (c) 2015 J. Schilling
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
#include <schily/string.h>
#include <schily/schily.h>

/* VARARGS3 */
#ifdef	PROTOTYPES
EXPORT size_t
strlcatl(char *to, size_t len, ...)
#else
EXPORT size_t
strlcatl(to, len, va_alist)
	char	*to;
	size_t	len;
	va_dcl
#endif
{
		va_list	args;
	register char	*p;
	register char	*tor = to;
		size_t	olen = len;

#ifdef	PROTOTYPES
	va_start(args, len);
#else
	va_start(args);
#endif
	if (len > 0) {
		while (--len > 0 && *tor++ != '\0')
			;

		if (len == 0)
			goto toolong;

		while ((p = va_arg(args, char *)) != NULL) {
			tor--;
			len++;
			while (--len > 0 && (*tor++ = *p++) != '\0')
				;
			if (len == 0) {
				*tor = '\0';
				olen = tor - to + strlen(p);
				goto toolong;
			}
		}
		olen = --tor - to;
	}
	va_end(args);
	return (olen);
toolong:
	while ((p = va_arg(args, char *)) != NULL) {
		olen += strlen(p);
	}
	va_end(args);
	return (olen);
}
