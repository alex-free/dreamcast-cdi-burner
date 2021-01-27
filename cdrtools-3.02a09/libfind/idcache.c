/* @(#)idcache.c	1.30 15/05/01 Copyright 1993, 1995-2015 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)idcache.c	1.30 15/05/01 Copyright 1993, 1995-2015 J. Schilling";
#endif
/*
 *	UID/GID caching functions
 *
 *	Copyright (c) 1993, 1995-2015 J. Schilling
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
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/pwd.h>
#include <schily/grp.h>
#include <schily/string.h>
#include <schily/schily.h>

#define	TUNMLEN	32			/* Max. UID name len (POSIX)	*/
#define	TGNMLEN	32			/* Max. GID name len (POSIX)	*/

#define	C_SIZE	16			/* Cache size (cache is static) */

typedef struct u_id {
	uid_t	uid;
	char	name[TUNMLEN+1];
	char	valid;
} uidc_t;

typedef struct g_id {
	gid_t	gid;
	char	name[TGNMLEN+1];
	char	valid;
} gidc_t;

LOCAL	uidc_t	uidcache[C_SIZE];
LOCAL	int	lastuidx;		/* Last index for new entry */

LOCAL	gidc_t	gidcache[C_SIZE];
LOCAL	int	lastgidx;		/* Last index for new entry */

LOCAL	uid_t	_uid_nobody;		/* Uid for user "nobody"    */
LOCAL	gid_t	_gid_nobody;		/* Gid for user "nobody"    */
LOCAL	BOOL	name_init = FALSE;

EXPORT	BOOL	ic_nameuid	__PR((char *name, int namelen, uid_t uid));
EXPORT	BOOL	ic_uidname	__PR((char *name, int namelen, uid_t *uidp));
EXPORT	BOOL	ic_namegid	__PR((char *name, int namelen, gid_t gid));
EXPORT	BOOL 	ic_gidname	__PR((char *name, int namelen, gid_t *gidp));
LOCAL	void	nameinit	__PR((void));
EXPORT	uid_t	ic_uid_nobody	__PR((void));
EXPORT	gid_t	ic_gid_nobody	__PR((void));

/*
 * Get name from uid
 */
#ifdef	PROTOTYPES
EXPORT BOOL
ic_nameuid(char *name, int namelen, uid_t uid)
#else
EXPORT BOOL
ic_nameuid(name, namelen, uid)
	char	*name;
	int	namelen;
	uid_t	uid;
#endif
{
	struct passwd	*pw;
	register int	i;
	register uidc_t	*idp;

	for (i = 0, idp = uidcache; i < C_SIZE; i++, idp++) {
		if (idp->valid == 0)		/* Entry not yet filled */
			break;
		if (idp->uid == uid)
			goto out;
	}
	idp = &uidcache[lastuidx++];		/* Round robin fill next ent */
	if (lastuidx >= C_SIZE)
		lastuidx = 0;

	idp->uid = uid;
	idp->name[0] = '\0';
	idp->valid = 1;
	if ((pw = getpwuid(uid)) != NULL)
		strlcpy(idp->name, pw->pw_name, sizeof (idp->name));

out:
	strlcpy(name, idp->name, namelen);
	return (name[0] != '\0');
}

/*
 * Get uid from name
 */
EXPORT BOOL
ic_uidname(name, namelen, uidp)
	char	*name;
	int	namelen;
	uid_t	*uidp;
{
	struct passwd	*pw;
	register int	len = namelen > TUNMLEN?TUNMLEN:namelen;
	register int	i;
	register uidc_t	*idp;

	if (name[0] == '\0') {
		*uidp = ic_uid_nobody();	/* Return UID_NOBODY */
		return (FALSE);
	}

	for (i = 0, idp = uidcache; i < C_SIZE; i++, idp++) {
		if (idp->valid == 0)		/* Entry not yet filled */
			break;
		if (name[0] == idp->name[0] &&
					strncmp(name, idp->name, len) == 0) {
			*uidp = idp->uid;
			if (idp->valid == 2) {	/* Name not found */
				*uidp = ic_uid_nobody(); /* Return UID_NOBODY */
				return (FALSE);
			}
			return (TRUE);
		}
	}
	idp = &uidcache[lastuidx++];		/* Round robin fill next ent */
	if (lastuidx >= C_SIZE)
		lastuidx = 0;

	idp->uid = 0;
	idp->name[0] = '\0';
	strlcpy(idp->name, name, len+1);	/* uidc_t.name is TUNMLEN+1 */
	idp->valid = 1;
	if ((pw = getpwnam(idp->name)) != NULL) {
		idp->uid = pw->pw_uid;
		*uidp = idp->uid;
		return (TRUE);
	} else {
		idp->valid = 2;			/* Mark name as not found */
		*uidp = ic_uid_nobody();	/* Return UID_NOBODY */
		return (FALSE);
	}
}

/*
 * Get name from gid
 */
#ifdef	PROTOTYPES
EXPORT BOOL
ic_namegid(char *name, int namelen, gid_t gid)
#else
EXPORT BOOL
ic_namegid(name, namelen, gid)
	char	*name;
	int	namelen;
	gid_t	gid;
#endif
{
	struct group	*gr;
	register int	i;
	register gidc_t	*idp;

	for (i = 0, idp = gidcache; i < C_SIZE; i++, idp++) {
		if (idp->valid == 0)		/* Entry not yet filled */
			break;
		if (idp->gid == gid)
			goto out;
	}
	idp = &gidcache[lastgidx++];		/* Round robin fill next ent */
	if (lastgidx >= C_SIZE)
		lastgidx = 0;

	idp->gid = gid;
	idp->name[0] = '\0';
	idp->valid = 1;
	if ((gr = getgrgid(gid)) != NULL)
		strlcpy(idp->name, gr->gr_name, sizeof (idp->name));

out:
	strlcpy(name, idp->name, namelen);
	return (name[0] != '\0');
}

/*
 * Get gid from name
 */
EXPORT BOOL
ic_gidname(name, namelen, gidp)
	char	*name;
	int	namelen;
	gid_t	*gidp;
{
	struct group	*gr;
	register int	len = namelen > TGNMLEN?TGNMLEN:namelen;
	register int	i;
	register gidc_t	*idp;

	if (name[0] == '\0') {
		*gidp = ic_gid_nobody();	/* Return GID_NOBODY */
		return (FALSE);
	}

	for (i = 0, idp = gidcache; i < C_SIZE; i++, idp++) {
		if (idp->valid == 0)		/* Entry not yet filled */
			break;
		if (name[0] == idp->name[0] &&
					strncmp(name, idp->name, len) == 0) {
			*gidp = idp->gid;
			if (idp->valid == 2) {	/* Name not found */
				*gidp = ic_gid_nobody(); /* Return GID_NOBODY */
				return (FALSE);
			}
			return (TRUE);
		}
	}
	idp = &gidcache[lastgidx++];		/* Round robin fill next ent */
	if (lastgidx >= C_SIZE)
		lastgidx = 0;

	idp->gid = 0;
	idp->name[0] = '\0';
	strlcpy(idp->name, name, len+1);	/* gidc_t.name is TGNMLEN+1 */
	idp->valid = 1;
	if ((gr = getgrnam(idp->name)) != NULL) {
		idp->gid = gr->gr_gid;
		*gidp = idp->gid;
		return (TRUE);
	} else {
		idp->valid = 2;			/* Mark name as not found */
		*gidp = ic_gid_nobody();	/* Return GID_NOBODY */
		return (FALSE);
	}
}

#include <schily/param.h>

#ifndef	UID_NOBODY
#define	UID_NOBODY	65534		/* The old SunOS-4.x and *BSD value */
#endif
#ifndef	GID_NOBODY
#define	GID_NOBODY	65534		/* The old SunOS-4.x and *BSD value */
#endif

LOCAL void
nameinit()
{
	char	*name;
	int	namelen;
	uid_t	uid;
	gid_t	gid;

	/*
	 * Make sure that uidname()/gidname() do not call nameinit().
	 */
	name_init = TRUE;

	name = "nobody";
	namelen = strlen(name);
	if (!ic_uidname(name, namelen, &uid))
		uid = UID_NOBODY;
	_uid_nobody = uid;

	if (!ic_gidname(name, namelen, &gid))
		gid = GID_NOBODY;
	_gid_nobody = gid;
}

EXPORT uid_t
ic_uid_nobody()
{
	if (!name_init)
		nameinit();
	return (_uid_nobody);
}

EXPORT gid_t
ic_gid_nobody()
{
	if (!name_init)
		nameinit();
	return (_gid_nobody);
}
