/* @(#)xio.c	1.20 09/07/18 Copyright 2003-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)xio.c	1.20 09/07/18 Copyright 2003-2009 J. Schilling";
#endif
/*
 *	EXtended I/O functions for cdrecord
 *
 *	Copyright (c) 2003-2009 J. Schilling
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
#include <schily/unistd.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/fcntl.h>
#include <schily/io.h>				/* for setmode() prototype */

#ifdef	VMS
#include <vms_init.h>
#define	open(n, p, m)	(open)((n), (p), (m), \
				"acc", acc_cb, &open_id)
#endif

#include "xio.h"

LOCAL xio_t	x_stdin = {
	NULL,		/* x_next	*/
	NULL,		/* x_name	*/
	0,		/* x_off	*/
	0,		/* x_startoff	*/
	STDIN_FILENO,	/* x_file	*/
	999,		/* x_refcnt	*/
	O_RDONLY,	/* x_oflag	*/
	0,		/* x_omode	*/
	0		/* x_xflags	*/
};

LOCAL xio_t	*x_root = &x_stdin;
LOCAL xio_t	**x_tail = NULL;


LOCAL	xio_t	*xnewnode	__PR((char *name));
EXPORT	void	*xopen		__PR((char *name, int oflag, int mode,
								int xflags));
EXPORT	off_t	xmarkpos	__PR((void *vp));
EXPORT	int	xclose		__PR((void *vp));

LOCAL xio_t *
xnewnode(name)
	char	*name;
{
	xio_t	*xp;

	if ((xp = malloc(sizeof (xio_t))) == NULL)
		return ((xio_t *) NULL);

	xp->x_next = (xio_t *) NULL;
	xp->x_name = strdup(name);
	if (xp->x_name == NULL) {
		free(xp);
		return ((xio_t *) NULL);
	}
	xp->x_off = 0;
	xp->x_startoff = (off_t)0;
	xp->x_file = -1;
	xp->x_refcnt = 1;
	xp->x_oflag = 0;
	xp->x_omode = 0;
	xp->x_xflags = 0;
	return (xp);
}

EXPORT void *
xopen(name, oflag, mode, xflags)
	char	*name;
	int	oflag;
	int	mode;
	int	xflags;
{
	int	f;
	xio_t	*xp;
	xio_t	*pp = x_root;

	if (x_tail == NULL)
		x_tail = &x_stdin.x_next;
	if (name == NULL) {
		xp = &x_stdin;
		xp->x_refcnt++;
		if ((oflag & O_BINARY) != 0) {
			setmode(STDIN_FILENO, O_BINARY);
		}
		return (xp);
	}
	for (; pp; pp = pp->x_next) {
		if (pp->x_name == NULL)	/* stdin avoid core dump in strcmp() */
			continue;
		if ((strcmp(pp->x_name, name) == 0) &&
		    (pp->x_oflag == oflag) && (pp->x_omode == mode)) {
			break;
		}
	}
	if (pp) {
		pp->x_refcnt++;
		return ((void *)pp);
	}
	if ((f = open(name, oflag, mode)) < 0)
		return (NULL);

	if ((xp = xnewnode(name)) == NULL) {
		close(f);
		return (NULL);
	}
	xp->x_file = f;
	xp->x_oflag = oflag;
	xp->x_omode = mode;
	xp->x_xflags = xflags & X_UFLAGS;
	*x_tail = xp;
	x_tail = &xp->x_next;
	return ((void *)xp);
}

EXPORT off_t
xmarkpos(vp)
	void	*vp;
{
	xio_t	*xp = vp;
	off_t	off = (off_t)0;

	if (xp == (xio_t *)NULL)
		return ((off_t)-1);

	xp->x_startoff = off = lseek(xp->x_file, (off_t)0, SEEK_CUR);
	if (xp->x_startoff == (off_t)-1) {
		xp->x_startoff = (off_t)0;
		xp->x_xflags |= X_NOSEEK;
	}
	if (isatty(xp->x_file)) {
		off = (off_t)-1;
		xp->x_xflags |= X_NOSEEK;
	}
	return (off);
}

EXPORT int
xclose(vp)
	void	*vp;
{
	xio_t	*xp = vp;
	xio_t	*pp = x_root;
	int	ret = 0;

	if (xp == &x_stdin)
		return (ret);
	if (x_tail == NULL)
		x_tail = &x_stdin.x_next;

/*error("xclose(%p) refcnt = %d\n", vp, xp->x_refcnt);*/
	if (--xp->x_refcnt <= 0) {
		ret = close(xp->x_file);
		while (pp) {
			if (pp->x_next == xp)
				break;
			if (pp->x_next == NULL)
				break;
			pp = pp->x_next;
		}
		if (pp->x_next == xp) {
			if (x_tail == &xp->x_next)
				x_tail = &pp->x_next;
			pp->x_next = xp->x_next;
		}

		free(xp->x_name);
		free(xp);
	} else if ((xp->x_xflags & (X_NOREWIND|X_NOSEEK)) == 0) {
		lseek(xp->x_file, xp->x_startoff, SEEK_SET);
	}
	return (ret);
}
