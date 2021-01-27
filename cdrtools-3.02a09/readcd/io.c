/* @(#)io.c	1.8 10/12/19 Copyright 1988-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)io.c	1.8 10/12/19 Copyright 1988-2010 J. Schilling";
#endif
/*
 *	Copyright (c) 1988-2010 J. Schilling
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
#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/varargs.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/utypes.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>
#include <schily/ctype.h>

#include "iodefs.h"

struct disk {
	int	dummy;
};

LOCAL	char	*skipwhite	__PR((const char *));
LOCAL	void	prt_std		__PR((char *, long, long, long, struct disk *));
EXPORT	BOOL	cvt_std		__PR((char *, long *, long, long, struct disk *));
extern	BOOL	getvalue	__PR((char *, long *, long, long,
				void (*)(char *, long, long, long, struct disk *),
				BOOL (*)(char *, long *, long, long, struct disk *),
				struct disk *));
extern	BOOL	getlong		__PR((char *, long *, long, long));
extern	BOOL	getint		__PR((char *, int *, int, int));
extern	BOOL	yes		__PR((char *, ...));

LOCAL char *
skipwhite(s)
		const char	*s;
{
	register const Uchar	*p = (const Uchar *)s;

	while (*p) {
		if (!isspace(*p))
			break;
		p++;
	}
	return ((char *)p);
}

/* ARGSUSED */
EXPORT BOOL
cvt_std(linep, lp, mini, maxi, dp)
	char	*linep;
	long	*lp;
	long	mini;
	long	maxi;
	struct disk	*dp;
{
	long	l	= -1L;

/*	printf("cvt_std(\"%s\", %d, %d, %d);\n", linep, *lp, mini, maxi);*/

	if (linep[0] == '?') {
		printf(_("Enter a number in the range from %ld to %ld\n"),
								mini, maxi);
		printf(_("The default radix is 10\n"));
		printf(_("Precede number with '0x' for hexadecimal or with '0' for octal\n"));
		printf(_("Shorthands are:\n"));
		printf(_("\t'^' for minimum value (%ld)\n"), mini);
		printf(_("\t'$' for maximum value (%ld)\n"), maxi);
		printf(_("\t'+' for incrementing value to %ld\n"), *lp + 1);
		printf(_("\t'-' for decrementing value to %ld\n"), *lp - 1);
		return (FALSE);
	}
	if (linep[0] == '^' && *skipwhite(&linep[1]) == '\0') {
		l = mini;
	} else if (linep[0] == '$' && *skipwhite(&linep[1]) == '\0') {
		l = maxi;
	} else if (linep[0] == '+' && *skipwhite(&linep[1]) == '\0') {
		if (*lp < maxi)
			l = *lp + 1;
	} else if (linep[0] == '-' && *skipwhite(&linep[1]) == '\0') {
		if (*lp > mini)
			l = *lp - 1;
	} else if (*astol(linep, &l)) {
		printf(_("Not a number: '%s'.\n"), linep);
		return (FALSE);
	}
	if (l < mini || l > maxi) {
		printf(_("'%s' is out of range.\n"), linep);
		return (FALSE);
	}
	*lp = l;
	return (TRUE);
}

/* ARGSUSED */
LOCAL void
prt_std(s, l, mini, maxi, dp)
	char	*s;
	long	l;
	long	mini;
	long	maxi;
	struct disk *dp;
{
	printf("%s %ld (%ld - %ld)/<cr>:", s, l, mini, maxi);
}

EXPORT BOOL
getvalue(s, lp, mini, maxi, prt, cvt, dp)
	char	*s;
	long	*lp;
	long	mini;
	long	maxi;
	void	(*prt) __PR((char *, long, long, long, struct disk *));
	BOOL	(*cvt) __PR((char *, long *, long, long, struct disk *));
	struct disk	*dp;
{
	char	line[128];
	char	*linep;

	for (;;) {
		(*prt)(s, *lp, mini, maxi, dp);
		flush();
		line[0] = '\0';
		if (getline(line, 80) == EOF)
			exit(EX_BAD);

		linep = skipwhite(line);
		/*
		 * Nicht initialisierte Variablen
		 * duerfen nicht uebernommen werden
		 */
		if (linep[0] == '\0' && *lp != -1L)
			return (FALSE);

		if (strlen(linep) == 0) {
			/* Leere Eingabe */
			/* EMPTY */
		} else if ((*cvt)(linep, lp, mini, maxi, dp))
			return (TRUE);
	}
	/* NOTREACHED */
}

EXPORT BOOL
getlong(s, lp, mini, maxi)
	char	*s;
	long	*lp;
	long	mini;
	long	maxi;
{
	return (getvalue(s, lp, mini, maxi, prt_std, cvt_std, (void *)0));
}

EXPORT BOOL
getint(s, ip, mini, maxi)
	char	*s;
	int	*ip;
	int	mini;
	int	maxi;
{
	long	l = *ip;
	BOOL	ret;

	ret = getlong(s, &l, (long)mini, (long)maxi);
	*ip = l;
	return (ret);
}

/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT BOOL
yes(char *form, ...)
#else
EXPORT BOOL
yes(form, va_alist)
	char	*form;
	va_dcl
#endif
{
	va_list	args;
	char okbuf[10];

again:
#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	printf("%r", form, args);
	va_end(args);
	flush();
	if (getline(okbuf, sizeof (okbuf)) == EOF)
		exit(EX_BAD);
	if (okbuf[0] == '?') {
		printf(_("Enter 'y', 'Y', 'yes' or 'YES' if you agree with the previous asked question.\n"));
		printf(_("All other input will be handled as if the question has beed answered with 'no'.\n"));
		goto again;
	}
	if (streql(okbuf, "y") || streql(okbuf, "yes") ||
	    streql(okbuf, "Y") || streql(okbuf, "YES"))
		return (TRUE);
#if	defined(USE_NLS) && defined(HAVE_NL_LANGINFO) && defined(YESSTR)
	if (streql(okbuf, nl_langinfo(YESSTR)))
		return (TRUE);
#endif
	return (FALSE);
}
