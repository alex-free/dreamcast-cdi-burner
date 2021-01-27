/* @(#)deflts.h	1.9 11/11/24 Copyright 1997-2011 J. Schilling */
/*
 *	Definitions for reading program defaults.
 *
 *	Copyright (c) 1997-2011 J. Schilling
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

#ifndef	_SCHILY_DEFLTS_H
#define	_SCHILY_DEFLTS_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#define	DEFLT	"/etc/default"

/*
 * cmd's to defltcntl()
 */
#define	DC_GETFLAGS	0	/* Get actual flags	*/
#define	DC_SETFLAGS	1	/* Set new flags	*/

/*
 * flags to defltcntl()
 *
 * Make sure that when adding features, the default behaviour
 * is the same as old behaviour.
 */
#define	DC_CASE		0x0001	/* Don't ignore case	*/

#define	DC_STD		DC_CASE	/* Default flags	*/

/*
 * Macros to handle flags
 */
#ifndef	TURNON
#define	TURNON(flags, mask)	flags |= mask
#define	TURNOFF(flags, mask)	flags &= ~(mask)
#define	ISON(flags, mask)	(((flags) & (mask)) == (mask))
#define	ISOFF(flags, mask)	(((flags) & (mask)) != (mask))
#endif

extern	int	defltopen	__PR((const char *name));
extern	int	defltclose	__PR((void));
extern	int	defltsect	__PR((const char *name));
extern	int	defltfirst	__PR((void));
extern	char	*defltread	__PR((const char *name));
extern	char	*defltnext	__PR((const char *name));
extern	int	defltcntl	__PR((int cmd, int flags));

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_DEFLTS_H */
