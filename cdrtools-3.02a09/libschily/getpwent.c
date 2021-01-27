/* @(#)getpwent.c	1.1 11/07/12 Copyright 2011 J. Schilling */
/*
 *	Passwd functions for platforms (like MINGW) that do not have them.
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
#include <schily/pwd.h>
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/schily.h>

#if !defined(HAVE_GETPWNAM) && !defined(HAVE_GETPWENT) && \
	!defined(HAVE_GETPWUID) && !defined(HAVE_SETPWENT) && \
	!defined(HAVE_ENDPWENT)


LOCAL	FILE		*pwdf;
LOCAL	struct passwd	pwd;
LOCAL	struct passwd	rootpwd = { "root", "*", 0, 0, "", "/", "" };
LOCAL	char		pwdbuf[128];

LOCAL	struct passwd *mkpwd	__PR((char *arr[]));
LOCAL	struct passwd *findpwent __PR((const char *string, int field));

EXPORT struct passwd *
getpwent()
{
	char	*arr[7];

	if (pwdf == (FILE *)NULL) {
		pwdf = fileopen("/etc/passwd", "r");
		if (pwdf == (FILE *)NULL)
			return ((struct passwd *)0);
	}
	if (fgetline(pwdf, pwdbuf, sizeof (pwdbuf)) == EOF)
		return ((struct passwd *)0);
	breakline(pwdbuf, ':', arr, 7);
	return (mkpwd(arr));
}

EXPORT void
setpwent()
{
	if (pwdf == (FILE *)NULL) {
		pwdf = fileopen("/etc/passwd", "r");
		if (pwdf == (FILE *)NULL)
			return;
	}
	fileseek(pwdf, (off_t)0);
}

EXPORT void
endpwent()
{
	if (pwdf != (FILE *)NULL) {
		fclose(pwdf);
		pwdf = (FILE *)NULL;
	}
}

EXPORT struct passwd *
getpwnam(name)
	const char	*name;
{
	setpwent();
	if (pwdf == (FILE *)NULL) {
		if (streql(rootpwd.pw_name, name))
			return (&rootpwd);
		return ((struct passwd *)0);
	}
	return (findpwent(name, 0));
}

EXPORT struct passwd *
getpwuid(uid)
	uid_t	uid;
{
	char	suid[32];

	setpwent();
	if (pwdf == (FILE *)NULL) {
		if (uid == 0)
			return (&rootpwd);
		return ((struct passwd *)0);
	}
	js_snprintf(suid, sizeof (suid), "%lld", (Llong)uid);
	return (findpwent(suid, 2));
}

LOCAL struct passwd *
mkpwd(arr)
	char	*arr[];
{
	long	l;

	pwd.pw_name = arr[0];
	pwd.pw_passwd = arr[1];
	(void) astolb(arr[2], &l, 10);
	pwd.pw_uid = l;
	(void) astolb(arr[3], &l, 10);
	pwd.pw_gid = l;
	pwd.pw_gecos = arr[4];
	pwd.pw_dir = arr[5];
	pwd.pw_shell = arr[6];

	return (&pwd);
}

LOCAL struct passwd *
findpwent(string, field)
	const char	*string;
	int		field;
{
	char	*arr[7];

	for (;;) {
		if (fgetline(pwdf, pwdbuf, sizeof (pwdbuf)) == EOF)
			return ((struct passwd *)0);
		breakline(pwdbuf, ':', arr, 7);
		if (streql(string, arr[field]))
			return (mkpwd(arr));
	}
}

#endif
