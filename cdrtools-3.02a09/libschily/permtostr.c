/* @(#)permtostr.c	1.2 11/08/16 Copyright 2011 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)permtostr.c	1.2 11/08/16 Copyright 2011 J. Schilling";
#endif
/*
 *	mkgmtime() is a complement to getperm()
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

#include <schily/mconfig.h>
#include <schily/types.h>
#include <schily/stat.h>
#include <schily/schily.h>

EXPORT	void	permtostr	__PR((mode_t mode, char *s));

LOCAL mode_t modebits[9] = {
	S_IXOTH, S_IWOTH, S_IROTH,
	S_IXGRP, S_IWGRP, S_IRGRP,
	S_IXUSR, S_IWUSR, S_IRUSR
};

/*
 * The maximum length of the perm string (including the final null byte) is 24.
 * If all permission related mode bits are set, the generated string will
 * be: "u=srwx,g=srwx,o=rwx,a+t".
 */
#ifdef	PROTOTYPES
EXPORT void
permtostr(register mode_t mode, char *s)
#else
EXPORT void
permtostr(mode, s)
	register mode_t	mode;
	char		*s;
#endif
{
	register char	*mstr = "xwrxwrxwr";
	register char	*str = s;
	register int	i;

	for (i = 9; --i >= 0; ) {
		if (i % 3 == 2) {
			if (str > s)
				*str++ = ',';
			*str++ = "ogu"[i/3];
			*str++ = '=';
			if (i == 8 && mode & S_ISUID)
				*str++ = 's';
			if (i == 5 && mode & S_ISGID)
				*str++ = 's';
		}
		if (mode & modebits[i])
			*str++ = mstr[i];
	}
	if (mode & S_ISVTX) {
		*str++ = ',';
		*str++ = 'a';
		*str++ = '+';
		*str++ = 't';
	}
	*str = '\0';
}
