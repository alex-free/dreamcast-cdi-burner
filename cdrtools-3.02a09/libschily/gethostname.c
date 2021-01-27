/* @(#)gethostname.c	1.21 11/08/04 Copyright 1995-2011 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)gethostname.c	1.21 11/08/04 Copyright 1995-2011 J. Schilling";
#endif
/*
 *	Copyright (c) 1995-2011 J. Schilling
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

#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/systeminfo.h>
#include <schily/hostname.h>

#ifndef	HAVE_GETHOSTNAME
EXPORT	int	gethostname	__PR((char *name, int namelen));


#ifdef	SI_HOSTNAME

EXPORT int
gethostname(name, namelen)
	char	*name;
	int	namelen;
{
	if (sysinfo(SI_HOSTNAME, name, namelen) < 0)
		return (-1);
	return (0);
}
#else	/* ! SI_HOSTNAME */

#ifdef	HAVE_UNAME
#include <schily/utsname.h>
#include <schily/string.h>

EXPORT int
gethostname(name, namelen)
	char	*name;
	int	namelen;
{
	struct utsname	uts;

	if (uname(&uts) < 0)
		return (-1);

	strncpy(name, uts.nodename, namelen);
	return (0);
}
#else	/* !HAVE_UNAME */

#if	defined(__MINGW32__) || defined(_MSC_VER)
#include <schily/utypes.h>
#include <schily/errno.h>
#define	gethostname	__winsock_gethostname
#include <schily/windows.h>
#undef	gethostname

EXPORT int
gethostname(name, namelen)
	char	*name;
	int	namelen;
{
	uint32_t	len = namelen;
	char		nbuf[MAX_COMPUTERNAME_LENGTH+1];

	if (namelen < 0) {
#ifdef	ENOSYS
		seterrno(ENOSYS);
#else
		seterrno(EINVAL);
#endif
		return (-1);
	}
	if (namelen == 0)
		return (0);

	name[0] = '\0';
	if (!GetComputerName(name, &len)) {
		if (len > namelen) {
			nbuf[0] = '\0';
			len = sizeof (nbuf);
			(void) GetComputerName(nbuf, &len);
			strncpy(name, nbuf, namelen);
			return (0);
		}
		seterrno(EIO);
		return (-1);
	}
	return (0);
}
#else
#include <schily/errno.h>

EXPORT int
gethostname(name, namelen)
	char	*name;
	int	namelen;
{
	if (namelen < 0) {
#ifdef	ENOSYS
		seterrno(ENOSYS);
#else
		seterrno(EINVAL);
#endif
		return (-1);
	}
	if (namelen > 0)
		name[0] = '\0';
	return (0);
}
#endif
#endif	/* !HAVE_UNAME */

#endif	/* !SI_HOSTNAME */

#endif	/* HAVE_GETHOSTNAME */
