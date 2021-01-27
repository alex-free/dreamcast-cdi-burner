/* @(#)termcap.h	1.10 10/08/27 Copyright 1995-2010 J. Schilling */
/*
 *	Copyright (c) 1995-2010 J. Schilling
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

#ifndef	_SCHILY_TERMCAP_H
#define	_SCHILY_TERMCAP_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_STANDARD_H
#include <schily/standard.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Flags for tcsetflags()
 */
#define	TCF_NO_TC		0x0001	/* Don't follow tc= entries */
#define	TCF_NO_SIZE		0x0002	/* Don't get actual ttysize (li#/co#) */
#define	TCF_NO_STRIP		0x0004	/* Don't strip down termcap buffer */

extern	char	PC;		/* Pad character */
extern	char	*BC;		/* Backspace if not "\b" from "bc" capability */
extern	char	*UP;		/* Cursor up string from "up" capability */
extern	short	ospeed;		/* output speed coded as in ioctl */

extern	int	tgetent		__PR((char *bp, char *name));
extern	int	tcsetflags	__PR((int flags));
extern	char	*tcgetbuf	__PR((void));
extern	int	tgetnum		__PR((char *ent));
extern	BOOL	tgetflag	__PR((char *ent));
extern	char	*tgetstr	__PR((char *ent, char **array));
extern	char	*tdecode	__PR((char *ep, char **array));

extern	int	tputs		__PR((char *cp, int affcnt,
					int (*outc)(int c)));
extern	char	*tgoto		__PR((char *cm, int destcol, int destline));

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_TERMCAP_H */
