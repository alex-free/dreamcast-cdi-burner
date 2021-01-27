/* @(#)find_misc.c	1.16 15/09/12 Copyright 2004-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)find_misc.c	1.16 15/09/12 Copyright 2004-2009 J. Schilling";
#endif
/*
 *	Copyright (c) 2004-2009 J. Schilling
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
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/stat.h>
#include <schily/errno.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

#include "find_misc.h"

#ifdef	USE_ACL
#ifdef	HAVE_SYS_ACL_H
#	include <sys/acl.h>
#endif
#ifdef	HAVE_ACL_LIBACL_H
#	include <acl/libacl.h>	/* Needed for acl_extended_file() */
#endif
#endif

/*
 * Define some things that either are missing or defined in a different way
 * on SCO UnixWare
 */
#if	defined(UNIXWARE) && defined(HAVE_ACL)
typedef struct acl	aclent_t;
#endif
#ifndef	GETACL
#define	GETACL	ACL_GET
#endif
#ifndef	SETACL
#define	SETACL	ACL_SET
#endif
#ifndef	GETACLCNT
#define	GETACLCNT	ACL_CNT
#endif
#ifndef	MIN_ACL_ENTRIES
#define	MIN_ACL_ENTRIES	NACLBASE
#endif

EXPORT	BOOL	has_acl		__PR((FILE *f, char *name, char *sname, struct stat *sp));
EXPORT	BOOL	has_xattr	__PR((FILE *f, char *sname));

EXPORT BOOL
has_acl(f, name, sname, sp)
	FILE	*f;
	char	*name;
	char	*sname;
	struct stat *sp;
{
#ifdef	USE_ACL
#ifdef	HAVE_SUN_ACL	/* Solaris & UnixWare */
	int	aclcount;

	/*
	 * Symlinks don't have ACLs
	 */
	if (S_ISLNK(sp->st_mode))
		return (FALSE);
#ifdef	HAVE_ST_ACLCNT
	return (sp->st_aclcnt > MIN_ACL_ENTRIES);
#else
	if ((aclcount = acl(sname, GETACLCNT, 0, NULL)) < 0) {
#ifdef	ENOSYS
		if (geterrno() == ENOSYS)
			return (FALSE);
#endif
		ferrmsg(f, _("Cannot get ACL count for '%s'.\n"), name);
		return (FALSE);
	}
	return (aclcount > MIN_ACL_ENTRIES);
#endif	/* HAVE_ST_ACLCNT */
#else	/* HAVE_SUN_ACL */
	/*
	 * Non Sun ACL implementations.
	 */
#ifdef	HAVE_ACL_EXTENDED_FILE
	/*
	 * Linux extension
	 */
	return (acl_extended_file(sname) == 1);
#else
#ifdef	HAVE_POSIX_ACL
	acl_t	acl;

	if ((acl = acl_get_file(sname, ACL_TYPE_ACCESS)) != NULL) {
#ifdef	HAVEACL_GET_ENTRY
		int	id = ACL_FIRST_ENTRY;
		int	num;
		acl_entry_t dummy;

		for (num = 0; acl_get_entry(acl, id, &dummy); num++)
			id = ACL_NEXT_ENTRY;
		acl_free(acl);
		if (num > 3)
			return (TRUE);
#else
#ifdef	NACLBASE
		if (acl->acl_cnt > NACLBASE)
			return (TRUE);
#endif
#endif
	}
	/*
	 * Only directories have DEFAULT ACLs
	 */
	if (!S_ISDIR(sp->st_mode))
		return (FALSE);
	if ((acl = acl_get_file(sname, ACL_TYPE_DEFAULT)) != NULL) {
#ifdef	HAVEACL_GET_ENTRY
		int	id = ACL_FIRST_ENTRY;
		int	num;
		acl_entry_t dummy;

		for (num = 0; acl_get_entry(acl, id, &dummy); num++)
			id = ACL_NEXT_ENTRY;
		acl_free(acl);
		if (num > 0)
			return (TRUE);
#else
#ifdef	NACLBASE
		if (acl->acl_cnt > NACLBASE)
			return (TRUE);
#endif
#endif
	}
#endif	/* HAVE_POSIX_ACL */

	return (FALSE);

#endif
#endif	/* HAVE_SUN_ACL */
#else	/* USE_ACL */
	return (FALSE);
#endif	/* USE_ACL */
}

EXPORT BOOL
has_xattr(f, sname)
	FILE	*f;
	char	*sname;
{
#ifdef	_PC_XATTR_EXISTS
		return (pathconf(sname, _PC_XATTR_EXISTS) > 0);
#else
		return (FALSE);
#endif
}
