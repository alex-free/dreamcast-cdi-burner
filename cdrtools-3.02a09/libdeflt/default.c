/* @(#)default.c	1.11 17/07/15 Copyright 1997-2017 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)default.c	1.11 17/07/15 Copyright 1997-2017 J. Schilling";
#endif
/*
 *	Copyright (c) 1997-2017 J. Schilling
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

#include <schily/standard.h>
#include <schily/stdio.h>
#include <schily/string.h>
#include <schily/types.h>	/* For strncasecmp  size_t */
#include <schily/unistd.h>
#include <schily/deflts.h>
#include <schily/libport.h>	/* Define missing prototypes */

#define	MAXLINE	512

static	FILE	*dfltfile	= (FILE *)NULL;
static	const char *dfltsect	= NULL;
static	int	dfltflags	= DC_STD;

EXPORT	int	defltopen	__PR((const char *name));
EXPORT	int	defltclose	__PR((void));
EXPORT	int	defltsect	__PR((const char *name));
EXPORT	int	defltfirst	__PR((void));
EXPORT	char	*defltread	__PR((const char *name));
EXPORT	char	*defltnext	__PR((const char *name));
EXPORT	int	defltcntl	__PR((int cmd, int flags));

EXPORT int
defltopen(name)
	const char	*name;
{
	if (dfltfile != (FILE *)NULL) {
		fclose(dfltfile);
		dfltfile = NULL;
	}

	if (name == (char *)NULL) {
		return (0);
	}

	if ((dfltfile = fopen(name, "r")) == (FILE *)NULL) {
		return (-1);
	}
	dfltflags = DC_STD;
	dfltsect  = NULL;

	return (0);
}

EXPORT int
defltclose()
{
	int	ret;

	if (dfltfile != (FILE *)NULL) {
		ret = fclose(dfltfile);
		dfltfile = NULL;
		return (ret);
	}
	return (0);
}

EXPORT int
defltsect(name)
	const char	*name;
{
	size_t	len;

	if (name == NULL) {
		dfltsect = NULL;
		return (0);
	}
	if (*name != '[')
		return (-1);

	len = strlen(name);
	if (len >= MAXLINE)
		return (-1);
	if (len < 3 || name[len-1] != ']')
		return (-1);

	dfltsect = name;
	return (0);
}

EXPORT int
defltfirst()
{
	if (dfltfile == (FILE *)NULL) {
		return (-1);
	}
	rewind(dfltfile);
	if (dfltsect) {
		register int	len;
		register int	sectlen;
			char	buf[MAXLINE];

		sectlen = strlen(dfltsect);
		while (fgets(buf, sizeof (buf), dfltfile)) {
			len = strlen(buf);
			if (buf[len-1] == '\n') {
				buf[len-1] = 0;
			} else {
				continue;
			}
			if (buf[0] != '[')
				continue;
			if (dfltflags & DC_CASE) {
				if (strncmp(dfltsect, buf, sectlen) == 0) {
					return (0);
				}
			} else {
				if (strncasecmp(dfltsect, buf, sectlen) == 0) {
					return (0);
				}
			}
		}
		return (-1);
	}
	return (0);
}

EXPORT char *
defltread(name)
	const char	*name;
{
	if (dfltfile == (FILE *)NULL) {
		return ((char *)NULL);
	}
	defltfirst();
	return (defltnext(name));
}

EXPORT char *
defltnext(name)
	const char	*name;
{
	register int	len;
	register int	namelen;
	static	 char	buf[MAXLINE];

	if (dfltfile == (FILE *)NULL) {
		return ((char *)NULL);
	}
	namelen = strlen(name);

	while (fgets(buf, sizeof (buf), dfltfile)) {
		len = strlen(buf);
		if (buf[len-1] == '\n') {
			buf[len-1] = 0;
		} else {
			return ((char *)NULL);
		}
		/*
		 * Check for end of current section. Seek to the end of file to
		 * prevent other calls to defltnext() return unexpected data.
		 */
		if (dfltsect != NULL && buf[0] == '[') {
			fseek(dfltfile, (off_t)0, SEEK_END);
			return ((char *)NULL);
		}
		if (dfltflags & DC_CASE) {
			if (strncmp(name, buf, namelen) == 0) {
				return (&buf[namelen]);
			}
		} else {
			if (strncasecmp(name, buf, namelen) == 0) {
				return (&buf[namelen]);
			}
		}
	}
	return ((char *)NULL);
}

EXPORT int
defltcntl(cmd, flags)
	int	cmd;
	int	flags;
{
	int  oldflags = dfltflags;

	dfltflags = flags;

	return (oldflags);
}
