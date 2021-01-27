/* @(#)abspath.c	1.5 13/10/30 Copyright 2011-2013 J. Schilling */
/*
 *	Compute the absolute path for a relative path name
 *
 *	Copyright (c) 2011-2013 J. Schilling
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

#include <schily/unistd.h>
#include <schily/types.h>
#include <schily/errno.h>
#include <schily/maxpath.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/schily.h>

EXPORT	char	*abspath	__PR((const char *relp, char *absp, size_t asize));
EXPORT	char	*absnpath	__PR((const char *relp, char *absp, size_t asize));
LOCAL	char	*pathabs	__PR((const char *relp, char *absp, size_t asize, int flags));
LOCAL	void	ashorten	__PR((char *name));

/*
 * Expands a relative pathname to a full (absolute) pathname.
 */
EXPORT char *
abspath(relp, absp, asize)
		const	char	*relp;
			char	*absp;
			size_t	asize;
{
	return (pathabs(relp, absp, asize, RSPF_EXIST));
}

/*
 * Expands a relative pathname to a full (absolute) pathname.
 * Not all path elements need to exist.
 */
EXPORT char *
absnpath(relp, absp, asize)
		const	char	*relp;
			char	*absp;
			size_t	asize;
{
	return (pathabs(relp, absp, asize, 0));
}

/*
 * Expands a relative pathname to a full (absolute) pathname.
 * The behavior may be controlled via flags.
 */
EXPORT char *
absfpath(relp, absp, asize, flags)
		const	char	*relp;
			char	*absp;
			size_t	asize;
			int	flags;
{
	return (pathabs(relp, absp, asize, flags));
}

/*
 * Expands a relative pathname to a full (absolute) pathname.
 */
LOCAL char *
pathabs(relp, absp, asize, flags)
		const	char	*relp;
			char	*absp;
			size_t	asize;
			int	flags;
{
	register	char	*rel;
	register	char	*full;
			int	ret;

	ret = resolvefpath(relp, absp, asize, flags);
	if (ret < 0)
		return (NULL);			/* errno set by resolvepath() */
	if (ret >= asize) {
		seterrno(ERANGE);
		return (NULL);
	}
	if (absp[0] == '/')
		return (absp);

	if (absp[0] == '.' && absp[1] == '\0')
		return (getcwd(absp, asize));	/* Working directory. */

	{	int	len = strlen(absp)+1;
#ifdef	HAVE_DYN_ARRAYS
		char	tbuf[len];
#else
		char	*tbuf = malloc(len);
		if (tbuf == NULL)
			return (NULL);
#endif
		strcpy(tbuf, absp);
		absp[0] = '\0';
		full = getcwd(absp, asize);	/* Working directory. */

		if (full && strlcat(full, "/", asize) >= asize) {
			seterrno(ERANGE);
			full = NULL;
		}
		if (full && strlcat(full, tbuf, asize) >= asize) {
			seterrno(ERANGE);
			full = NULL;
		}

#ifndef	HAVE_DYN_ARRAYS
		free(tbuf);
#endif
		if (full == NULL)
			return (full);
	}
	rel = full;
	for (;;) {
		for (;;) {
			rel = strchr(++rel, '/');
			if (rel == NULL)
				break;
			if (rel[1] == '/' || rel[1] == '\0') {
				*rel++ = '\0';
							/* CSTYLED */
							/* /foo//bar = /foo/bar */
				while (rel[0] == '/')
					rel++;
				break;
			}
			if (rel[1] == '.') {
							/* /foo/./bar = /foo/bar */
				if (rel[2] == '/') {
					*rel = '\0';
					rel += 3;
					break;
				}
							/* /foo/. = /foo    */
				if (rel[2] == '\0') {
					*rel = '\0';
					rel += 2;
					break;
				}
				if (rel[2] == '.') {
							/* /foo/../bar = /bar */
					if (rel[3] == '/') {
						*rel = '\0';
						rel += 4;
						ashorten(full);
						break;
					}
							/* /foo/bar/.. = /foo */
					if (rel[3] == '\0') {
						*rel = '\0';
						ashorten(full);
						break;
					}
				}
			}
		}
		if (rel == NULL || rel[0] == '\0')
			break;

#ifdef	DEBUG
		printf("%s / %s\n", full, rel);
#endif
		if (strlcat(full, "/", asize) >= asize) {
			seterrno(ERANGE);
			return (NULL);
		}
		if (strlcat(full, rel, asize) >= asize) {
			seterrno(ERANGE);
			return (NULL);
		}
		rel = full;
	}
	if (full[1] == '.' && full[2] == '\0') 			/* /. = /   */
		full[1] = '\0';
	return (full);
}

/*
 * Removes last path name component in absolute path name.
 */
LOCAL void
ashorten(name)
	register	char	*name;
{
	register	char	*p;

	for (p = name++; *p++ != '\0'; );
	while (p > name)
		if (*--p == '/')
			break;
	*p = '\0';
}
