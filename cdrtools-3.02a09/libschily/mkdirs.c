/* @(#)mkdirs.c	1.4 14/01/02 Copyright 2011-2014 J. Schilling */
/*
 *	mkdirs() is the equivalent to "mkdir -p path"
 *	makedirs() allows to create missing direcories before a final
 *	path element if called makedirs(path, mode, TRUE).
 *
 *	"name" must be a modifyable string.
 *
 *	Copyright (c) 2011-2014 J. Schilling
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

#include <schily/types.h>
#include <schily/stat.h>
#include <schily/errno.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/schily.h>

#if defined(ENOTEMPTY) && ENOTEMPTY != EEXIST
#define	is_eexist(err)	((err) == EEXIST || (err) == ENOTEMPTY)
#else
#define	is_eexist(err)	((err) == EEXIST)
#endif
#if defined(EMISSDIR)
#define	is_enoent(err)	((err) == ENOENT || (err) == EMISSDIR)
#else
#define	is_enoent(err)	((err) == ENOENT)
#endif

#ifdef	__MINGW32__
#define	mkdir(a, b)	mkdir(a)
#endif

EXPORT	BOOL	makedirs	__PR((char *name, mode_t mode, int striplast));

#ifdef	PROTOTYPES
EXPORT int
mkdirs(register char *name, mode_t mode)
#else
EXPORT int
mkdirs(name, mode)
	register char	*name;
		mode_t	mode;
#endif
{
	return (makedirs(name, mode, FALSE));
}

#ifdef	PROTOTYPES
EXPORT int
makedirs(register char *name, mode_t mode, int striplast)
#else
EXPORT int
makedirs(name, mode, striplast)
	register char	*name;
		mode_t	mode;
		int	striplast;
#endif
{
	register char	*p;
		char	*ls = NULL;
		int	ret = 0;
		int	err = 0;
		mode_t	mask;
		struct stat sb;

	if (name == NULL) {
		seterrno(EFAULT);
		return (-1);
	}
	if (*name == '\0') {
		seterrno(EINVAL);
		return (-1);
	}
	mask = umask(S_IRWXU|S_IRWXG|S_IRWXO);
	(void) umask(mask);

	for (p = &name[1]; *p; p++) {
		if (*p != '/')
			continue;

		*p = '\0';
		if (stat(name, &sb) < 0) {
			if (mkdir(name, mode | S_IRWXU) < 0) {
				err = geterrno();

				if (!is_eexist(err)) {
					ret = -1;
					goto errout;
				}
			} else {
				if (ls && (mode & S_IRWXU) != S_IRWXU) {
					*ls = '\0';
					chmod(name, mode & ~mask);
					*ls = '/';
				}
				ls = p;
			}
		}
		*p = '/';
	}
	err = 0;
	if (!striplast)
		ret = mkdir(name, mode);
errout:
	if (ls && (mode & S_IRWXU) != S_IRWXU) {
		*ls = '\0';
		chmod(name, mode & ~mask);
		*ls = '/';
		if (err)
			seterrno(err);
	}
	return (ret);
}
