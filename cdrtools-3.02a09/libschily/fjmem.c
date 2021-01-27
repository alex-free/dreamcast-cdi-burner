/* @(#)fjmem.c	1.14 15/05/10 Copyright 1998-2015 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)fjmem.c	1.14 15/05/10 Copyright 1998-2015 J. Schilling";
#endif
/*
 *	Memory handling with error checking
 *
 *	Copyright (c) 1998-2015 J. Schilling
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

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/jmpdefs.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

EXPORT	int	__fjmexval	__PR((int exval));
EXPORT	void	*__fjmalloc	__PR((FILE *f, size_t size, char *msg, sigjmps_t *jmp));
EXPORT	void	*__fjrealloc	__PR((FILE *f, void *ptr, size_t size, char *msg, sigjmps_t *jmp));
EXPORT	char	*__fjsavestr	__PR((FILE *f, const char *s, sigjmps_t *jmp));

LOCAL	int	fjmexval;

EXPORT	int
__fjmexval(exval)
	int	exval;
{
	int	ret = fjmexval;

	fjmexval = exval;

	return (ret);
}

EXPORT void *
__fjmalloc(f, size, msg, jmp)
	FILE		*f;
	size_t		size;
	char		*msg;
	sigjmps_t	*jmp;
{
	void	*ret;

	ret = malloc(size);
	if (ret == NULL) {
		int	err = geterrno();

		if (f != NULL) {
			ferrmsg(f,
				gettext("Cannot allocate memory for %s.\n"),
									msg);
		}
		if (fjmexval)
			err = fjmexval;
		if (jmp == JM_EXIT)
			comexit(err);
		if (jmp != NULL)
			siglongjmp(jmp->jb, 1);
	}
	return (ret);
}

EXPORT void *
__fjrealloc(f, ptr, size, msg, jmp)
	FILE		*f;
	void		*ptr;
	size_t		size;
	char		*msg;
	sigjmps_t	*jmp;
{
	void	*ret;

	if (ptr == NULL)
		ret = malloc(size);
	else
		ret = realloc(ptr, size);
	if (ret == NULL) {
		int	err = geterrno();

		if (f != NULL) {
			ferrmsg(f,
				gettext("Cannot realloc memory for %s.\n"),
									msg);
		}
		if (fjmexval)
			err = fjmexval;
		if (jmp == JM_EXIT)
			comexit(err);
		if (jmp != NULL)
			siglongjmp(jmp->jb, 1);
	}
	return (ret);
}

EXPORT char *
__fjsavestr(f, s, jmp)
	FILE		*f;
	const char	*s;
	sigjmps_t		*jmp;
{
	char	*ret = __fjmalloc(f, strlen(s)+1, "saved string", jmp);

	if (ret != NULL)
		strcpy(ret, s);
	return (ret);
}
