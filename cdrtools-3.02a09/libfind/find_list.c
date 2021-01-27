/* @(#)find_list.c	1.28 15/09/12 Copyright 1985, 1995, 2000-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)find_list.c	1.28 15/09/12 Copyright 1985, 1995, 2000-2010 J. Schilling";
#endif
/*
 *	List a file
 *
 *	Copyright (c) 1985, 1995, 2000-2010 J. Schilling
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
#include <schily/utypes.h>
#include <schily/dirent.h>
#include <schily/time.h>
#include <schily/standard.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/device.h>
#include <schily/nlsdefs.h>
#include <schily/param.h>
#include <schily/walk.h>
#include <schily/find.h>
#include <schily/idcache.h>
#include "find_list.h"
#include "find_misc.h"

#define	K_DIV	(1024/DEV_BSIZE)

#if	defined(IS_MACOS_X)
/*
 * The MAC OS X linker does not grok "common" varaibles.
 * Make find_sixmonth/find_now a "data" variable.
 */
EXPORT	time_t	find_sixmonth = 0;	/* 6 months before limit (ls)	*/
EXPORT	time_t	find_now = 0;		/* now limit (ls)		*/
#else
EXPORT	time_t	find_sixmonth;		/* 6 months before limit (ls)	*/
EXPORT	time_t	find_now;		/* now limit (ls)		*/
#endif

#define	paxls	TRUE
extern	BOOL	numeric;
extern	int	verbose;

LOCAL	void	modstr		__PR((FILE *f, struct stat *fs, char *s,
					char *name, char *sname,
					struct WALK *state));
EXPORT	void	find_list	__PR((FILE *std[3], struct stat *fs,
					char *name, char *sname,
					struct WALK *state));

/*
 * Convert POSIX.1 mode/permission flags into string.
 */
LOCAL void
modstr(f, fs, s, name, sname, state)
	FILE		*f;		/* FILE * for error messages	 */
	struct stat	*fs;		/* Struct stat for this file	 */
		char	*s;		/* String to fill with mode	 */
		char	*name;		/* Name to use for stat/readlink */
		char	*sname;		/* Name to use for stat/readlink */
	struct WALK	*state;		/* state from treewalk()	 */
{
	register char	*mstr = "xwrxwrxwr";
	register char	*str = s;
	register int	i;
	register mode_t	mode = fs->st_mode;

	for (i = 9; --i >= 0; ) {
		if (mode & (1 << i))
			*str++ = mstr[i];
		else
			*str++ = '-';
	}
#ifdef	USE_ACL
	*str++ = ' ';
#endif
#ifdef	USE_XATTR
	*str++ = '\0';				/* Don't claim space for '@' */
#endif
	*str = '\0';
	str = s;
	if (mode & S_ISVTX) {
		if (mode &  S_IXOTH) {
			str[8] = 't';		/* Sticky & exec. by others  */
		} else {
			str[8] = 'T';		/* Sticky but !exec. by oth  */
		}
	}
	if (mode & S_ISGID) {
		if (mode & S_IXGRP) {
			str[5] = 's';		/* Sgid & executable by grp  */
		} else {
			if (S_ISDIR(mode))
				str[5] = 'S';	/* Sgid directory	    */
			else
				str[5] = 'l';	/* Mandatory lock file	    */
		}
	}
	if (mode & S_ISUID) {
		if (mode & S_IXUSR)
			str[2] = 's';		/* Suid & executable by own. */
		else
			str[2] = 'S';		/* Suid but not executable   */
	}
	i = 9;
#ifdef	USE_ACL
	if (state->pflags & PF_ACL) {
		if (state->pflags & PF_HAS_ACL)
			str[i++] = '+';
	} else
	if (has_acl(f, name, sname, fs))
		str[i++] = '+';
#endif
#ifdef	USE_XATTR
	if (state->pflags & PF_XATTR) {
		if (state->pflags & PF_HAS_XATTR)
			str[i++] = '@';
	} else
	if (has_xattr(f, sname))
		str[i++] = '@';
#endif
	i++;	/* Make lint believe that we always use i. */
}

EXPORT void
find_list(std, fs, name, sname, state)
	FILE		*std[3];	/* To redirect stdin/stdout/err	 */
	struct stat	*fs;		/* Struct stat for this file	 */
	char		*name;		/* Complete path name		 */
	char		*sname;		/* Name to use for stat/readlink */
	struct WALK	*state;		/* state from treewalk()	 */
{
		time_t	*tp;
		char	*tstr;
		char	mstr[12]; /* 9 UNIX chars + ACL '+' XATTR '@' + nul */
		char	lstr[11]; /* contains link count as string */
	static	char	nuid[32+1];
	static	char	ngid[32+1];
		char	*add = "";
char	lname[8192];
char	*lnamep = lname;
		int	lsize;

#define	verbose	1
	if (verbose) {
		char	*uname;
		char	*gname;
		int	umaxlen;
		int	gmaxlen;
		char	ft;

		tp = state->walkflags & WALK_LS_ATIME ? &fs->st_atime :
				(state->walkflags & WALK_LS_CTIME ?
					&fs->st_ctime : &fs->st_mtime);
		tstr = ctime(tp);

		if (ic_nameuid(nuid, sizeof (nuid), fs->st_uid)) {
			uname = nuid;
			umaxlen = sizeof (nuid)-1;
		} else {
			sprintf(nuid, "%llu", (Llong)fs->st_uid);
			uname = nuid;
			umaxlen = sizeof (nuid)-1;
		}

		if (ic_namegid(ngid, sizeof (ngid), fs->st_gid)) {
			gname = ngid;
			gmaxlen = sizeof (ngid)-1;
		} else {
			sprintf(ngid, "%llu", (Llong)fs->st_gid);
			gname = ngid;
			gmaxlen = sizeof (ngid)-1;
		}

		{
			fprintf(std[1], "%7llu ", (Llong)fs->st_ino);
#ifdef	HAVE_ST_BLOCKS
			fprintf(std[1], "%4llu ", (Llong)fs->st_blocks/K_DIV);
#else
			fprintf(std[1], "%4llu ",
				(Llong)(fs->st_size+1023)/1024);
#endif
		}
		if (!paxls) {
			if (S_ISBLK(fs->st_mode) || S_ISCHR(fs->st_mode))
				fprintf(std[1], "%3lu %3lu",
					(long)major(fs->st_rdev),
					(long)minor(fs->st_rdev));
			else
				fprintf(std[1], "%7llu", (Llong)fs->st_size);
		}
		modstr(std[2], fs, mstr, name, sname, state);

		if (paxls || fs->st_nlink > 0) {
			/*
			 * UNIX ls uses %3d for the link count
			 * and does not claim space for ACL '+'
			 */
			js_sprintf(lstr, " %2ld", (long)fs->st_nlink);
		} else {
			lstr[0] = 0;
		}

		switch (fs->st_mode & S_IFMT) {

		case S_IFREG:	ft = '-'; break;
#ifdef	S_IFLNK
		case S_IFLNK:	ft = 'l';
				if (state->lname != NULL) {
					lnamep = state->lname;
					break;
				}
				lname[0] = '\0';
				lsize = readlink(sname, lname, sizeof (lname));
				if (lsize < 0)
					ferrmsg(std[2],
					_("Cannot read link '%s'.\n"),
						name);
				lname[sizeof (lname)-1] = '\0';
				if (lsize >= 0)
					lname[lsize] = '\0';
				break;
#endif
		case S_IFDIR:	ft = 'd'; break;
#ifdef	S_IFBLK
		case S_IFBLK:	ft = 'b'; break;
#endif
#ifdef	S_IFCHR
		case S_IFCHR:	ft = 'c'; break;
#endif
#ifdef	S_IFIFO
		case S_IFIFO:	ft = 'p'; break;
#endif
#ifdef	S_IFDOOR
		case S_IFDOOR:	ft = 'D'; break;
#endif
#ifdef	S_IFSOCK
		case S_IFSOCK:	ft = 's'; break;
#endif
#ifdef	S_IFPORT
		case S_IFPORT:	ft = 'P'; break;
#endif
#ifdef	S_IFNAM
		case S_IFNAM:	switch (fs->st_rdev) {
				case S_INSEM:
					ft = 's';
					break;
				case S_INSHD:
					ft = 'm';
					break;
				default:
					ft = '-';
					break;
				}
#endif

		default:	ft = '?'; break;
		}

		if (!paxls) {
			fprintf(std[1], " %c%s%s %3.*s/%-3.*s %.12s %4.4s ",
				ft,
				mstr,
				lstr,
				umaxlen, uname,
				gmaxlen, gname,
				&tstr[4], &tstr[20]);
		} else {
			fprintf(std[1], "%c%s%s %-8.*s %-8.*s ",
				ft,
				mstr,
				lstr,
				umaxlen, uname,
				gmaxlen, gname);
			if (S_ISBLK(fs->st_mode) || S_ISCHR(fs->st_mode))
/* XXXXXXXXXXXXXX */		fprintf(std[1], "%3lu, %3lu",
					(long)major(fs->st_rdev),
					(long)minor(fs->st_rdev));
			else
				fprintf(std[1], "%7llu", (Llong)fs->st_size);
			if ((*tp < find_sixmonth) || (*tp > find_now)) {
				fprintf(std[1], " %.6s  %4.4s ",
					&tstr[4], &tstr[20]);
			} else {
				fprintf(std[1], " %.12s ",
					&tstr[4]);
			}
		}
	}
	fprintf(std[1], "%s%s", name, add);

	if (S_ISLNK(fs->st_mode))
		fprintf(std[1], " -> %s", lnamep);

	fprintf(std[1], "\n");
}
