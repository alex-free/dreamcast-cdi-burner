/* @(#)getav0.c	1.23 09/07/08 Copyright 1985, 1995-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)getav0.c	1.23 09/07/08 Copyright 1985, 1995-2009 J. Schilling";
#endif
/*
 *	Get arg vector by scanning the stack
 *
 *	Copyright (c) 1985, 1995-2009 J. Schilling
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

#include <schily/sigblk.h>
#include <schily/avoffset.h>
#include <schily/standard.h>
#include <schily/schily.h>

#if	!defined(AV_OFFSET) || !defined(FP_INDIR)
#	ifdef	HAVE_SCANSTACK
#	undef	HAVE_SCANSTACK
#	endif
#endif

#ifdef	HAVE_SCANSTACK

#include <schily/stkframe.h>

#define	is_even(p)	((((long)(p)) & 1) == 0)
#define	even(p)		(((long)(p)) & ~1L)
#ifdef	__future__
#define	even(p)		(((long)(p)) - 1) /* will this work with 64 bit ?? */
#endif

EXPORT	char	**getmainfp	__PR((void));
EXPORT	char	**getavp	__PR((void));
EXPORT	char	*getav0		__PR((void));


EXPORT char **
getmainfp()
{
	register struct frame *fp;
		char	**av;
#if	FP_INDIR > 0
	register int	i = 0;
#endif

	/*
	 * As the SCO OpenServer C-Compiler has a bug that may cause
	 * the first function call to getfp() been done before the
	 * new stack frame is created, we call getfp() twice.
	 */
	(void) getfp();
	fp = (struct frame *)getfp();
	if (fp == NULL)
		return (NULL);

	while (fp->fr_savfp) {
		if (fp->fr_savpc == 0)
			break;

		if (!is_even(fp->fr_savfp)) {
			fp = (struct frame *)even(fp->fr_savfp);
			if (fp == NULL)
				break;
			fp = (struct frame *)((SIGBLK *)fp)->sb_savfp;
			continue;
		}
		fp = (struct frame *)fp->fr_savfp;

#if	FP_INDIR > 0
		i++;
#endif
	}

#if	FP_INDIR > 0
	i -= FP_INDIR;
	fp = (struct frame *)getfp();
	if (fp == NULL)
		return (NULL);

	while (fp->fr_savfp) {
		if (fp->fr_savpc == 0)
			break;

		if (!is_even(fp->fr_savfp)) {
			fp = (struct frame *)even(fp->fr_savfp);
			if (fp == NULL)
				break;
			fp = (struct frame *)((SIGBLK *)fp)->sb_savfp;
			continue;
		}
		fp = (struct frame *)fp->fr_savfp;

		if (--i <= 0)
			break;
	}
#endif

	av = (char **)fp;
	return (av);
}

EXPORT char **
getavp()
{
	register struct frame *fp;
		char	**av;

	fp = (struct frame *)getmainfp();
	if (fp == NULL)
		return (NULL);

	av = (char **)(((char *)fp) + AV_OFFSET);	/* aus avoffset.h */
							/* -> avoffset.c  */
	return (av);
}

EXPORT char *
getav0()
{
	return (getavp()[0]);
}

#else

EXPORT char **
getmainfp()
{
	raisecond("getmainfp", 0);
	return ((char **)0);
}

EXPORT char **
getavp()
{
	raisecond("getavp", 0);
	return ((char **)0);
}

EXPORT char *
getav0()
{
	return ("???");
}

#endif	/* HAVE_SCANSTACK */
