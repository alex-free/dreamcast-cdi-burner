/* @(#)fetchdir.c	1.29 16/03/10 Copyright 2002-2016 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)fetchdir.c	1.29 16/03/10 Copyright 2002-2016 J. Schilling";
#endif
/*
 *	Blocked directory handling.
 *
 *	Copyright (c) 2002-2016 J. Schilling
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
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/dirent.h>
#include <schily/stat.h>	/* needed in case we have no dirent->d_ino */
#include <schily/string.h>
#include <schily/libport.h>
#include <schily/schily.h>
#include <schily/fetchdir.h>

#ifndef	HAVE_LSTAT
#define	lstat	stat
#endif

EXPORT	char	*fetchdir	__PR((char *dir, int *entp, int *lenp,
						ino_t **inop));
#ifdef	HAVE_DIRENT_D_TYPE
LOCAL	int	fdt		__PR((int type));
#endif
EXPORT	char	*dfetchdir	__PR((DIR *dir, char *dir_name, int *entp,
						int *lenp, ino_t **inop));

EXPORT char *
fetchdir(dir, entp, lenp, inop)
	char	*dir;			/* The name of the directory	  */
	int	*entp;			/* Pointer to # of entries found  */
	int	*lenp;			/* Pointer to len of returned str */
	ino_t	**inop;
{
	char	*ret;
	DIR	*d = opendir(dir);

	if (d == NULL)
		return (NULL);
	ret = dfetchdir(d, dir, entp, lenp, inop);
	closedir(d);
	return (ret);
}

#ifdef	HAVE_DIRENT_D_TYPE
LOCAL int
fdt(type)
	int	type;
{
	switch (type) {

#ifdef	DT_FIFO
	case DT_FIFO: return (FDT_FIFO);
#endif
#ifdef	DT_CHR
	case DT_CHR: return (FDT_CHR);
#endif
#ifdef	DT_DIR
	case DT_DIR: return (FDT_DIR);
#endif
#ifdef	DT_BLK
	case DT_BLK: return (FDT_BLK);
#endif
#ifdef	DT_REG
	case DT_REG: return (FDT_REG);
#endif
#ifdef	DT_LNK
	case DT_LNK: return (FDT_LNK);
#endif
#ifdef	DT_SOCK
	case DT_SOCK: return (FDT_SOCK);
#endif
#ifdef	DT_WHT
	case DT_WHT: return (FDT_WHT);
#endif
	default: return (FDT_UNKN);
	}
}
#endif	/* HAVE_DIRENT_D_TYPE */

/*
 * Fetch content of a directory and return all entries (except '.' & '..')
 * concatenated in one memory chunk.
 *
 * Each name is prepended by a binary 1 ('^A') that is used by star to flag
 * additional information for this entry.
 * The end of the returned string contains two additional null character.
 */
EXPORT char *
dfetchdir(d, dir_name, entp, lenp, inop)
	DIR	*d;
	char	*dir_name;		/* The name of the directory	  */
	int	*entp;			/* Pointer to # of entries found  */
	int	*lenp;			/* Pointer to len of returned str */
	ino_t	**inop;
{
		char	*erg = NULL;
		int	esize = 2;
		int	msize = getpagesize();
		int	off = 0;
		ino_t	*ino = NULL;
		int	mino = 0;
	struct dirent	*dp;
	register char	*name;
	register int	nlen;
	register int	nents = 0;
#ifndef	HAVE_DIRENT_D_INO
	struct stat	sbuf;
		char	sname[PATH_MAX+1];
#endif

	if ((erg = ___malloc(esize, "fetchdir")) == NULL)
		return (NULL);
	erg[0] = '\0';
	erg[1] = '\0';

	while ((dp = readdir(d)) != NULL) {
		name = dp->d_name;
		/*
		 * Skip the following names: "", ".", "..".
		 */
		if (name[name[0] != '.' ? 0 : name[1] != '.' ? 1 : 2] == '\0')
			continue;
		if (inop) {
			if (mino <= nents) {
				if (mino == 0)
					mino = 32;
				else if (mino < (msize / sizeof (ino_t)))
					mino *= 2;
				else
					mino += msize / sizeof (ino_t);
				if ((ino = ___realloc(ino, mino * sizeof (ino_t), "fetchdir")) == NULL)
					return (NULL);
			}
#ifdef	HAVE_DIRENT_D_INO
			ino[nents] = dp->d_ino;
#else
			/*
			 * d_ino is currently missing on __DJGPP__ & __CYGWIN__
			 * We need to call lstat(2) for every file
			 * in order to get the needed information.
			 * Do not care about speed, this should be a rare
			 * exception.
			 */
			if (dir_name != NULL) {
				snprintf(sname, sizeof (sname), "%s/%s",
								dir_name, name);
				sbuf.st_ino = (ino_t)0;
				lstat(sname, &sbuf);
				ino[nents] = sbuf.st_ino;
			} else {
				ino[nents] = (ino_t)-1;
			}
#endif
		}
		nents++;
		nlen = strlen(name);
		nlen += 4;		/* ^A name ^@ + ^@^@ Platz fuer Ende */
		while (esize < (off + nlen)) {
			if (esize < 64)
				esize = 32;
			if (esize < msize)
				esize *= 2;
			else
				esize += msize;
			if (esize < (off + nlen))
				continue;

			if ((erg = ___realloc(erg, esize, "fetchdir")) == NULL)
				return (NULL);
		}
#ifdef	DEBUG
		if (off > 0)
			erg[off-1] = 2;	/* Hack: ^B statt ^@ zwischen Namen */
#endif
#ifdef	HAVE_DIRENT_D_TYPE
		erg[off++] = fdt(dp->d_type); /* File type		    */
#else
		erg[off++] = FDT_UNKN;	/* Platzhalter: ^A vor jeden Namen  */
#endif

		strcpy(&erg[off], name);
		off += nlen -3;		/* ^A  + ^@^@ Platz fuer Ende	    */
	}
#ifdef	DEBUG
	erg[off-1] = 2;			/* Hack: ^B st. ^@ am letzten Namen */
#endif
	erg[off] = 0;
	erg[off+1] = 0;

#ifdef	DEBUG
	erg[off] = 1;			/* Platzhalter: ^A n. letztem Namen */
	erg[off+1] = 0;			/* Letztes Null Byte		    */
#endif
	off++;				/* List terminator null Byte zaehlt */
	if (lenp)
		*lenp = &erg[off] - erg; /* Alloziert ist 1 Byte mehr	    */

	if (entp)
		*entp = nents;
	if (inop)
		*inop = ino;

	return (erg);
}
