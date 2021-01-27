/* @(#)findline.c	1.18 09/07/10 Copyright 1985, 1995-2003 J. Schilling */
/*
 *	findline
 *	get a line from a file with matching string in a given field
 *
 *	ofindline
 *	get a line from open file with matching string in a given field
 *
 *	getbroken
 *	separate a line into fields
 *
 *	all fill pointers into the given array, allocated to new storage
 *
 *	Copyright (c) 1985, 1995-2003 J. Schilling
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

#define	USE_LARGEFILES	/* We must make this module large file aware */

#include <schily/mconfig.h>
#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>	/* Include sys/types.h to make off_t available */
#include <schily/string.h>
#include <schily/schily.h>

#define	MAXLBUF		4096

/*
 * Return codes:
 */
#define	SUCCESS		 1	/* Found match */
#define	NO_MATCH	-1	/* Found no line with matching string	*/
#define	ARG_ERROR	-2	/* Bad parameter combination:		*/
				/* calls raisecond("findline_arg_err") before */
#define	OPEN_ERROR	-3	/* Cannot open file */
#define	NO_MEM		-4	/* No memory:				*/
				/* calls raisescond("findline_storage") before */

EXPORT	int	ofindline	__PR((FILE *, char, const char *, int,
								char **, int));
EXPORT	int	findline	__PR((const char *, char, const char *, int,
								char **, int));
EXPORT	int	getbroken	__PR((FILE *, char *, char, char **, int));

LOCAL	char	*savestr	__PR((const char *));

#ifdef	PROTOTYPES
EXPORT int
ofindline(FILE	*f,
		char	delim,
		const char *string,
		int	field,
		char	*array[],
		int	arraysize)
#else
EXPORT int
ofindline(f, delim, string, field, array, arraysize)
	FILE	*f;
	char	delim;
	char	*string;
	int	field;
	char	*array[];
	int	arraysize;
#endif
{
	int	i;
	char	lbuf[MAXLBUF];

	if (field >= arraysize) {
		raisecond("findline_arg_err", 0L);
		return (ARG_ERROR);
	}

	fileseek(f, (off_t)0);	/* XXX ??? Interface ändern!!! */
	for (;;) {
		if (getbroken(f, lbuf, delim, array, arraysize) < 0) {
			return (NO_MATCH);
		}
		if (streql(string, array[field])) {
			for (i = 0; i < arraysize; i++) {
				if ((array[i] = savestr(array[i])) == NULL) {
					raisecond("findline_storage", 0L);
					while (--i >= 0)
						free(array[i]);
					return (NO_MEM);
				}
			}
			return (SUCCESS);
		}
	}
}

#ifdef	PROTOTYPES
EXPORT int
findline(const char *fname,
		char	delim,
		const char *string,
		int	field,
		char	*array[],
		int	arraysize)
#else
EXPORT int
findline(fname, delim, string, field, array, arraysize)
	char	*fname;
	char	delim;
	char	*string;
	int	field;
	char	*array[];
	int	arraysize;
#endif
{
	FILE	*f;
	int	ret;

	if ((f = fileopen(fname, "r")) == 0)
		return (OPEN_ERROR);

	ret = ofindline(f, delim, string, field, array, arraysize);
	fclose(f);
	return (ret);
}

#ifdef	PROTOTYPES
EXPORT int
getbroken(FILE	*f,
		char	*linebuf,
		char	delim,
		char	*array[],
		int 	len)
#else
EXPORT int
getbroken(f, linebuf, delim, array, len)
	FILE	*f;
	char	*linebuf;
	char	delim;
	char	*array[];
	int	len;
#endif
{
	if (fgetline(f, linebuf, MAXLBUF) < 0)
		return (EOF);

	breakline(linebuf, delim, array, len);
	return (SUCCESS);
}

LOCAL char *
savestr(s)
	register const char	*s;
{
	register char	*p;
		char	*ret;

	if ((p = malloc(strlen(s)+1)) == NULL)
		return (p);
	ret = p;
	while ((*p++ = *s++) != '\0')
		/* LINTED */
		;
	return (ret);
}
