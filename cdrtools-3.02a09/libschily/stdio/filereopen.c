/* @(#)filereopen.c	1.18 10/11/06 Copyright 1986, 1995-2010 J. Schilling */
/*
 *	open new file on old stream
 *
 *	Copyright (c) 1986, 1995-2010 J. Schilling
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

#include "schilyio.h"

/*
 * Note that because of a definition in schilyio.h we are using
 * fseeko()/ftello() instead of fseek()/ftell() if available.
 */

LOCAL	char	*fmtab[] = {
			"",	/* 0	FI_NONE				*/
			"r",	/* 1	FI_READ				*/
			"r+",	/* 2	FI_WRITE		**1)	*/
			"r+",	/* 3	FI_READ  | FI_WRITE		*/
			"b",	/* 4	FI_NONE  | FI_BINARY		*/
			"rb",	/* 5	FI_READ  | FI_BINARY		*/
			"r+b",	/* 6	FI_WRITE | FI_BINARY	**1)	*/
			"r+b",	/* 7	FI_READ  | FI_WRITE | FI_BINARY	*/

/* + FI_APPEND	*/	"",	/* 0	FI_NONE				*/
/* ...		*/	"r",	/* 1	FI_READ				*/
			"a",	/* 2	FI_WRITE		**1)	*/
			"a+",	/* 3	FI_READ  | FI_WRITE		*/
			"b",	/* 4	FI_NONE  | FI_BINARY		*/
			"rb",	/* 5	FI_READ  | FI_BINARY		*/
			"ab",	/* 6	FI_WRITE | FI_BINARY	**1)	*/
			"a+b",	/* 7	FI_READ  | FI_WRITE | FI_BINARY	*/
		};
/*
 * NOTES:
 *	1)	there is no fopen() mode that opens for writing
 *		without creating/truncating at the same time.
 *
 *	"w"	will create/trunc files with fopen()
 *	"a"	will create files with fopen()
 */

EXPORT FILE *
filereopen(name, mode, fp)
	const char	*name;
	const char 	*mode;
	FILE		*fp;
{
	int	ret = -1;
	int	omode = 0;
	int	flag = 0;

	if (!_cvmod(mode, &omode, &flag))
		return ((FILE *)NULL);

	/*
	 * create/truncate file if necessary
	 */
	if ((flag & (FI_CREATE | FI_TRUNC)) != 0) {
		if ((ret = _openfd(name, omode)) < 0)
			return ((FILE *)NULL);
		close(ret);
	}

	fp = freopen(name,
		fmtab[flag & (FI_READ | FI_WRITE | FI_BINARY | FI_APPEND)], fp);

	if (fp != (FILE *)NULL) {
		set_my_flag(fp, 0); /* must clear it if fp is reused */

		if (flag & FI_APPEND) {
			(void) fseek(fp, (off_t)0, SEEK_END);
		}
		if (flag & FI_UNBUF) {
			setbuf(fp, NULL);
			add_my_flag(fp, _JS_IOUNBUF);
		}
	}
	return (fp);
}
