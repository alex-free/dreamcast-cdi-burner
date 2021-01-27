/* @(#)getperm.c	1.5 12/04/15 Copyright 2004-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)getperm.c	1.5 12/04/15 Copyright 2004-2009 J. Schilling";
#endif
/*
 *	Parser for chmod(1)/find(1)-perm, ....
 *
 *	Copyright (c) 2004-2009 J. Schilling
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
#include <schily/stat.h>
#include <schily/utypes.h>
#include <schily/standard.h>
#include <schily/schily.h>

#include <schily/nlsdefs.h>

EXPORT	int	getperm		__PR((FILE *f, char *perm, char *opname, mode_t *modep, int smode, int flag));
LOCAL	char	*getsperm	__PR((FILE *f, char *p, mode_t *mp, int smode, int flag));
LOCAL	mode_t	iswho		__PR((int c));
LOCAL	int	isop		__PR((int c));
LOCAL	mode_t	isperm		__PR((int c, int isX));

/*
 * This is the mode bit translation code stolen from star...
 */
#define	TSUID		04000	/* Set UID on execution */
#define	TSGID		02000	/* Set GID on execution */
#define	TSVTX		01000	/* On directories, restricted deletion flag */
#define	TUREAD		00400	/* Read by owner */
#define	TUWRITE		00200	/* Write by owner special */
#define	TUEXEC		00100	/* Execute/search by owner */
#define	TGREAD		00040	/* Read by group */
#define	TGWRITE		00020	/* Write by group */
#define	TGEXEC		00010	/* Execute/search by group */
#define	TOREAD		00004	/* Read by other */
#define	TOWRITE		00002	/* Write by other */
#define	TOEXEC		00001	/* Execute/search by other */

#define	TALLMODES	07777	/* The low 12 bits mentioned in the standard */

#define	S_ALLPERM	(S_IRWXU|S_IRWXG|S_IRWXO)
#define	S_ALLFLAGS	(S_ISUID|S_ISGID|S_ISVTX)
#define	S_ALLMODES	(S_ALLFLAGS | S_ALLPERM)

#if	S_ISUID == TSUID && S_ISGID == TSGID && S_ISVTX == TSVTX && \
	S_IRUSR == TUREAD && S_IWUSR == TUWRITE && S_IXUSR == TUEXEC && \
	S_IRGRP == TGREAD && S_IWGRP == TGWRITE && S_IXGRP == TGEXEC && \
	S_IROTH == TOREAD && S_IWOTH == TOWRITE && S_IXOTH == TOEXEC

#define	HAVE_POSIX_MODE_BITS	/* st_mode bits are equal to TAR mode bits */
#endif

#ifdef	HAVE_POSIX_MODE_BITS	/* st_mode bits are equal to TAR mode bits */
#define	OSMODE(xmode)	    (xmode)
#else
#define	OSMODE(xmode)	    ((xmode & TSUID   ? S_ISUID : 0)  \
			    | (xmode & TSGID   ? S_ISGID : 0) \
			    | (xmode & TSVTX   ? S_ISVTX : 0) \
			    | (xmode & TUREAD  ? S_IRUSR : 0) \
			    | (xmode & TUWRITE ? S_IWUSR : 0) \
			    | (xmode & TUEXEC  ? S_IXUSR : 0) \
			    | (xmode & TGREAD  ? S_IRGRP : 0) \
			    | (xmode & TGWRITE ? S_IWGRP : 0) \
			    | (xmode & TGEXEC  ? S_IXGRP : 0) \
			    | (xmode & TOREAD  ? S_IROTH : 0) \
			    | (xmode & TOWRITE ? S_IWOTH : 0) \
			    | (xmode & TOEXEC  ? S_IXOTH : 0))
#endif

EXPORT int
getperm(f, perm, opname, modep, smode, flag)
	FILE	*f;			/* FILE * to print error messages to */
	char	*perm;			/* Perm string to parse		    */
	char	*opname;		/* find(1) operator name / NULL	    */
	mode_t	*modep;			/* The mode result		    */
	int	smode;			/* The start mode for the computation */
	int	flag;			/* Flags, see getperm() flag defs */
{
	char	*p;
	Llong	ll;
	mode_t	mm;

	p = perm;
	if ((flag & GP_FPERM) && *p == '-')
		p++;

	if (*p >= '0' && *p <= '7') {
		p = astollb(p, &ll, 8);
		if (*p) {
			if (f) {
				if (opname) {
					ferrmsgno(f, EX_BAD,
					gettext("Non octal character '%c' in '-%s %s'.\n"),
					*p, opname, perm);
				} else {
					ferrmsgno(f, EX_BAD,
					gettext("Non octal character '%c' in '%s'.\n"),
					*p, perm);
				}
			}
			return (-1);
		}
		mm = ll & TALLMODES;
		*modep = OSMODE(mm);
		return (0);
	}
	p = getsperm(f, p, modep, smode, flag);
	if (p && *p != '\0') {
		if (f) {
			if (opname) {
				ferrmsgno(f, EX_BAD,
				gettext("Bad perm character '%c' found in '-%s %s'.\n"),
					*p, opname, perm);
			} else {
				ferrmsgno(f, EX_BAD,
				gettext("Bad perm character '%c' found in '%s'.\n"),
					*p, perm);
			}
		}
		return (-1);
	}
#ifdef	PERM_DEBUG
	error("GETPERM (%c) %4.4lo\n", perm, (long)*modep);
#endif
	return (0);
}

LOCAL char *
getsperm(f, p, mp, smode, flag)
	FILE	*f;
	char	*p;		/* The perm input string		*/
	mode_t	*mp;		/* To set the mode			*/
	int	smode;		/* The start mode for the computation	*/
	int	flag;		/* Flags, see getperm() flag defs	*/
{
#ifdef	OLD
	mode_t	permval = 0;	/* POSIX start value for "find" */
#else
	mode_t	permval = smode;	/* POSIX start value for "find" */
#endif
	mode_t	who;
	mode_t	m;
	int	op;
	mode_t	perms;
	mode_t	pm;

nextclause:
#ifdef	PERM_DEBUG
	error("getsperm(%s)\n", p);
#endif
	who = 0;
	while ((m = iswho(*p)) != 0) {
		p++;
		who |= m;
	}
	if (who == 0) {
		mode_t	mask;

		if (flag & GP_UMASK) {
			who = (S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO);
		} else {
			umask(mask = umask(0));
			who = ~mask;
			who &= (S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO);
		}
	}
#ifdef	PERM_DEBUG
	error("WHO %4.4lo\n", (long)who);
	error("getsperm(--->%s)\n", p);
#endif

nextop:
	if ((op = isop(*p)) != '\0')
		p++;
#ifdef	PERM_DEBUG
	error("op '%c'\n", op);
	error("getsperm(--->%s)\n", p);
#endif
	if (op == 0) {
		if (f) {
			ferrmsgno(f, EX_BAD, gettext("Missing -perm op.\n"));
		}
		return (p);
	}

	flag &= ~(GP_FPERM|GP_UMASK);
	if (flag & GP_XERR)
		flag = -1;
	perms = 0;
	while ((pm = isperm(*p, flag)) != (mode_t)-1) {
		p++;
		perms |= pm;
	}
#ifdef	PERM_DEBUG
	error("PERM %4.4lo\n", (long)perms);
	error("START PERMVAL %4.4lo\n", (long)permval);
#endif
	if ((perms == 0) && (*p == 'u' || *p == 'g' || *p == 'o')) {
		mode_t	what = 0;

		/*
		 * First select the bit triple we like to copy.
		 */
		switch (*p) {

		case 'u':
			what = permval & S_IRWXU;
			break;
		case 'g':
			what = permval & S_IRWXG;
			break;
		case 'o':
			what = permval & S_IRWXO;
			break;
		}
		/*
		 * Now copy over bit by bit without relying on shifts
		 * that would make implicit assumptions on values.
		 */
		if (what & (S_IRUSR|S_IRGRP|S_IROTH))
			perms |= (who & (S_IRUSR|S_IRGRP|S_IROTH));
		if (what & (S_IWUSR|S_IWGRP|S_IWOTH))
			perms |= (who & (S_IWUSR|S_IWGRP|S_IWOTH));
		if (what & (S_IXUSR|S_IXGRP|S_IXOTH))
			perms |= (who & (S_IXUSR|S_IXGRP|S_IXOTH));
		p++;
	}
#ifdef	PERM_DEBUG
	error("getsperm(--->%s)\n", p);
#endif
	switch (op) {

	case '=':
		permval &= ~who;
		/* FALLTHRU */
	case '+':
		permval |= (who & perms);
		break;

	case '-':
		permval &= ~(who & perms);
		break;
	}
#ifdef	PERM_DEBUG
	error("END PERMVAL %4.4lo\n", (long)permval);
#endif
	if (isop(*p))
		goto nextop;
	if (*p == ',') {
		p++;
		goto nextclause;
	}
	*mp = permval;
	return (p);
}

LOCAL mode_t
iswho(c)
	int	c;
{
	switch (c) {

	case 'u':					/* user */
		return (S_ISUID|S_ISVTX|S_IRWXU);
	case 'g':					/* group */
		return (S_ISGID|S_ISVTX|S_IRWXG);
	case 'o':					/* other */
		return (S_ISVTX|S_IRWXO);
	case 'a':					/* all */
		return (S_ISUID|S_ISGID|S_ISVTX|S_IRWXU|S_IRWXG|S_IRWXO);
	default:
		return (0);
	}
}

LOCAL int
isop(c)
	int	c;		/* The current perm character to parse	*/
{
	switch (c) {

	case '+':
	case '-':
	case '=':
		return (c);
	default:
		return (0);
	}
}

LOCAL mode_t
isperm(c, isX)
	int	c;		/* The current perm character to parse	*/
	int	isX;		/* -1: Ignore X, 0: no dir/X 1: X OK	*/
{
	switch (c) {

	case 'r':
		return (S_IRUSR|S_IRGRP|S_IROTH);
	case 'w':
		return (S_IWUSR|S_IWGRP|S_IWOTH);
	case 'X':
		if (isX < 0)
			return ((mode_t)-1);	/* Singnal parse error	*/
		if (isX == 0)
			return ((mode_t)0);	/* No 'X' handling here	*/
		/* FALLTHRU */
	case 'x':
		return (S_IXUSR|S_IXGRP|S_IXOTH);
	case 's':
		return (S_ISUID|S_ISGID);
	case 'l':
		return (S_ISGID);
	case 't':
		return (S_ISVTX);
	default:
		return ((mode_t)-1);
	}
}
