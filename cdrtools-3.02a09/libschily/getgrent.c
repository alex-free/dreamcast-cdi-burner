/* @(#)getgrent.c	1.1 11/07/12 Copyright 2011 J. Schilling */
/*
 *	Group functions for platforms (like MINGW) that do not have them.
 *
 *	Copyright (c) 2011 J. Schilling
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

#include <schily/stdio.h>
#include <schily/grp.h>
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/schily.h>

#if !defined(HAVE_GETGRNAM) && !defined(HAVE_GETGRENT) && \
	!defined(HAVE_GETGRGID) && !defined(HAVE_SETGRENT) && \
	!defined(HAVE_ENDGRENT)

LOCAL	FILE		*grpf;
LOCAL	char		*list[16+1];
LOCAL	struct group	grp;
LOCAL	struct group	rootgrp = { "root", "*", 0, list };
LOCAL	char		grpbuf[128];

LOCAL	struct group *mkgrp	__PR((char *arr[]));
LOCAL	struct group *findgrent	__PR((const char *string, int field));


EXPORT struct group *
getgrent()
{
	char	*arr[7];

	if (grpf == (FILE *)NULL) {
		grpf = fileopen("/etc/group", "r");
		if (grpf == (FILE *)NULL)
			return ((struct group *)0);
	}
	if (fgetline(grpf, grpbuf, sizeof (grpbuf)) == EOF)
		return ((struct group *)0);
	breakline(grpbuf, ':', arr, 7);
	return (mkgrp(arr));
}

EXPORT void
setgrent()
{
	if (grpf == (FILE *)NULL) {
		grpf = fileopen("/etc/group", "r");
		if (grpf == (FILE *)NULL)
			return;
	}
	fileseek(grpf, (off_t)0);
}

EXPORT void
endgrent()
{
	if (grpf != (FILE *)NULL) {
		fclose(grpf);
		grpf = (FILE *)NULL;
	}
}

EXPORT struct group *
getgrnam(name)
	const char	*name;
{
	setgrent();
	if (grpf == (FILE *)NULL) {
		list[0] = NULL;
		if (streql(rootgrp.gr_name, name))
			return (&rootgrp);
		return ((struct group *)0);
	}
	return (findgrent(name, 0));
}

EXPORT struct group *
getgrgid(gid)
	gid_t	gid;
{
	char	sgid[32];

	setgrent();
	if (grpf == (FILE *)NULL) {
		list[0] = NULL;
		if (gid == 0)
			return (&rootgrp);
		return ((struct group *)0);
	}
	js_snprintf(sgid, sizeof (sgid), "%lld", (Llong)gid);
	return (findgrent(sgid, 2));
}

LOCAL struct group *
mkgrp(arr)
	char	*arr[];
{
	long	l;

	grp.gr_name = arr[0];
	grp.gr_passwd = arr[1];
	(void) astolb(arr[2], &l, 10);
	grp.gr_gid = l;
	l = breakline(arr[3], ',', list, 16);
	list[l] = NULL;
	if (list[0][0] == '\0')
		list[0] = NULL;
	grp.gr_mem = list;
	return (&grp);
}

LOCAL struct group *
findgrent(string, field)
	const char	*string;
	int		field;
{
	char	*arr[7];

	for (;;) {
		if (fgetline(grpf, grpbuf, sizeof (grpbuf)) == EOF)
			return ((struct group *)0);
		breakline(grpbuf, ':', arr, 7);
		if (streql(string, arr[field]))
			return (mkgrp(arr));
	}
}

#endif
