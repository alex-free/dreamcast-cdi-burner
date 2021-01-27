/* @(#)getdomainname.c	1.23 10/08/23 Copyright 1995-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)getdomainname.c	1.23 10/08/23 Copyright 1995-2010 J. Schilling";
#endif
/*
 *	Copyright (c) 1995-2010 J. Schilling
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

#ifndef	HAVE_GETDOMAINNAME
EXPORT	int	getdomainname	__PR((char *name, int namelen));
#endif


#if	!defined(HAVE_GETDOMAINNAME) && defined(SI_SRPC_DOMAIN)
#define	FUNC_GETDOMAINNAME

EXPORT int
getdomainname(name, namelen)
	char	*name;
	int	namelen;
{
	if (sysinfo(SI_SRPC_DOMAIN, name, namelen) < 0)
		return (-1);
	return (0);
}
#endif

#if	!defined(HAVE_GETDOMAINNAME) && !defined(FUNC_GETDOMAINNAME)
#define	FUNC_GETDOMAINNAME

#include <schily/stdio.h>
#include <schily/string.h>
#include <schily/schily.h>

EXPORT int
getdomainname(name, namelen)
	char	*name;
	int	namelen;
{
	FILE	*f;
	char	name1[1024];
	char	*p;
	char	*p2;

	name[0] = '\0';

	f = fileopen("/etc/resolv.conf", "r");

	if (f == NULL)
		return (-1);

	while (fgetline(f, name1, sizeof (name1)) >= 0) {
		if ((p = strchr(name1, '#')) != NULL)
			*p = '\0';

		/*
		 * Skip leading whitespace.
		 */
		p = name1;
		while (*p != '\0' && (*p == ' ' || *p == '\t'))
			p++;

		if (strncmp(p, "domain", 6) == 0) {
			p += 6;
			while (*p != '\0' && (*p == ' ' || *p == '\t'))
				p++;
			if ((p2 = strchr(p, ' ')) != NULL)
				*p2 = '\0';
			if ((p2 = strchr(p, '\t')) != NULL)
				*p2 = '\0';

			strncpy(name, p, namelen);

			fclose(f);
			return (0);
		}
	}
	fclose(f);
	return (0);
}
#endif
