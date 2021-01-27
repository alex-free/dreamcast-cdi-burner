/* @(#)savewd.c	1.1 11/10/21 Copyright 2004-2011 J. Schilling */
/*
 *	Save and restore working directory.
 *
 *	The code has been adopted from libfind.
 *
 *	Copyright (c) 2004-2011 J. Schilling
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

#include <schily/unistd.h>
#include <schily/types.h>
#include <schily/fcntl.h>
#include <schily/maxpath.h>
#include <schily/string.h>
#include <schily/errno.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include "at-defs.h"

EXPORT	int	savewd	__PR((struct save_wd *sp));
EXPORT	void	closewd	__PR((struct save_wd *sp));
EXPORT	int	restorewd __PR((struct save_wd *sp));


EXPORT int
savewd(sp)
	struct save_wd	*sp;
{
	sp->fd = -1;
	sp->name = NULL;
#ifdef	HAVE_FCHDIR
	if ((sp->fd = open(".", O_SEARCH|O_NDELAY)) >= 0) {
#ifdef	F_SETFD
		(void) fcntl(sp->fd, F_SETFD, FD_CLOEXEC);
#endif
		return (0);
	}
#endif
	{
		char	buf[max(8192, PATH_MAX+1)];

		if (getcwd(buf, sizeof (buf)) == NULL) {
			return (-1);
		}
		sp->name = strdup(buf);
		if (sp->name == NULL)
			return (-1);
	}
	return (0);
}

EXPORT void
closewd(sp)
	struct save_wd	*sp;
{
	if (sp->fd >= 0)
		close(sp->fd);
	if (sp->name != NULL)
		free(sp->name);
	sp->fd = -1;
	sp->name = NULL;
}

EXPORT int
restorewd(sp)
	struct save_wd	*sp;
{
#ifdef	HAVE_FCHDIR
	if (sp->fd >= 0)
		return (fchdir(sp->fd));
#endif
	if (sp->name != NULL)
		return (lxchdir(sp->name));

	seterrno(EINVAL);
	return (-1);
}
