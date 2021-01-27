/* @(#)setfp.c	1.13 06/10/05 Copyright 1988, 1995-2003 J. Schilling */
/*
 *	Set frame pointer
 *
 *	Copyright (c) 1988, 1995-2003 J. Schilling
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
#include <schily/standard.h>
#include <schily/avoffset.h>
#include <schily/schily.h>

#if	!defined(AV_OFFSET) || !defined(FP_INDIR)
#	ifdef	HAVE_SCANSTACK
#	undef	HAVE_SCANSTACK
#	endif
#endif

#	ifdef	HAVE_SCANSTACK
#include <schily/stkframe.h>

#define	MAXWINDOWS	32
#define	NWINDOWS	7

extern	void	**___fpoff	__PR((char *cp));

EXPORT void
setfp(fp)
	void	* const *fp;
{
		long	**dummy[1];

#ifdef	sparc
	flush_reg_windows(MAXWINDOWS-2);
#endif
	*(long ***)(&((struct frame *)___fpoff((char *)&dummy[0]))->fr_savfp) =
								(long **)fp;
#ifdef	sparc
	flush_reg_windows(MAXWINDOWS-2);
#endif
}

#else

EXPORT void
setfp(fp)
	void	* const *fp;
{
	raisecond("setfp_not_implemented", 0L);
}

#endif
