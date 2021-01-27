/* @(#)findinpath.c	1.6 10/10/07 Copyright 2004-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)findinpath.c	1.6 10/10/07 Copyright 2004-2010 J. Schilling";
#endif
/*
 * Search a file name in the PATH and return the path name in allocated space.
 *
 * Copyright 2004-2010 J. Schilling
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

#include <schily/stdlib.h>
#include <schily/stat.h>
#include <schily/errno.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/schily.h>

EXPORT	char	*findinpath	__PR((char *name, int mode, BOOL plain_file,
								char *path));

#ifdef	JOS
#define	enofile(t)			((t) == EMISSDIR || \
					(t)  == ENOFILE || \
					(t)  == EISADIR || \
					(t)  == EIOERR)
#else
#define	enofile(t)			((t) == ENOENT || \
					(t)  == ENOTDIR || \
					(t)  == EISDIR || \
					(t)  == EIO)
#endif

EXPORT char *
findinpath(name, mode, plain_file, path)
	char	*name;			/* The name to execute			*/
	int	mode;			/* Mode for access() e.g. X_OK		*/
	BOOL	plain_file;		/* Whether to check only plain files	*/
	char	*path;			/* PATH to use if not NULL		*/
{
	char	*pathlist;
	char	*p1;
	char	*p2;
	char	*tmp;
	int	err = 0;
	int	exerr = 0;
	struct stat sb;

	if (name == NULL)
		return (NULL);
	if (strchr(name, '/'))
		return (strdup(name));

	if (path != NULL)
		pathlist = path;
	else if ((pathlist = getenv("PATH")) == NULL)
		pathlist = "/bin";
	p2 = pathlist = strdup(pathlist);
	if (pathlist == NULL)
		return (NULL);

	for (;;) {
		p1 = p2;
		if ((p2 = strchr(p2, PATH_ENV_DELIM)) != NULL)
			*p2++ = '\0';
		if (*p1 == '\0') {
			tmp = strdup(name);
			if (tmp == NULL) {
				free(pathlist);
				return (NULL);
			}
		} else {
			size_t	len = strlen(p1) + strlen(name) + 2;

			tmp = malloc(len);
			if (tmp == NULL) {
				free(pathlist);
				return (strdup(name));
			}
			strcatl(tmp, p1, PATH_DELIM_STR, name, (char *)NULL);
		}

		seterrno(0);
		if (stat(tmp, &sb) >= 0) {
			if ((!plain_file || S_ISREG(sb.st_mode)) &&
				(eaccess(tmp, mode) >= 0)) {
				free(pathlist);
				return (tmp);
			}
			if ((err = geterrno()) == 0)
				err = ENOEXEC;
		} else {
			err = geterrno();
		}
		free(tmp);
		if (exerr == 0 && !enofile(err))
			exerr = err;
		if ((!enofile(err) && !(err == EACCES)) || p2 == NULL)
			break;
	}
	free(pathlist);
	seterrno(exerr);
	return (NULL);
}
