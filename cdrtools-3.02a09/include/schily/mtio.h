/* @(#)mtio.h	1.6 10/08/23 Copyright 1995,2000-2010 J. Schilling */
/*
 *	Generic header for users of magnetic tape ioctl interface.
 *
 *	If there is no local mtio.h or equivalent, define
 *	simplified mtio definitions here in order
 *	to be able to do at least remote mtio on systems
 *	that have no local mtio
 *
 *	Copyright (c) 1995,2000-2010 J. Schilling
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

#ifndef	_SCHILY_MTIO_H
#define	_SCHILY_MTIO_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	HAVE_SYS_MTIO_H

#include <sys/mtio.h>

#else	/* ! HAVE_SYS_MTIO_H */


#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Definitions for magnetic tape io control commands
 */

/*
 * structure for MTIOCTOP - magnetic tape operation command
 */
struct	mtop {
	short	mt_op;		/* op code (see below)			*/
	daddr_t	mt_count;	/* repeat count or param		*/
};

/*
 * op code values for mt_op
 */
#define	MTWEOF		0	/* write EOF record(s)			*/
#define	MTFSF		1	/* fwd space over file mark(s)		*/
#define	MTBSF		2	/* back space over file mark(s) (1/2" only ) */
#define	MTFSR		3	/* fwd space record(s) (to inter-record gap) */
#define	MTBSR		4	/* back space record(s) (to inter-record gap) */
#define	MTREW		5	/* rewind tape				*/
#define	MTOFFL		6	/* rewind and put the drive offline	*/
#define	MTNOP		7	/* no operation (sets status ?)		*/

/*
 * structure for MTIOCGET - magnetic tape get status command
 */
struct	mtget {
	short	mt_type;	/* type of magnetic tape device		*/
				/* the next two regs are device dependent */
	short	mt_dsreg;	/* drive status 'register'		*/
	short	mt_erreg;	/* error 'register'			*/
	daddr_t	mt_resid;	/* transfer residual count		*/
	daddr_t	mt_fileno;	/* file # for current position		*/
	daddr_t	mt_blkno;	/* block # for current position		*/
};

#define	HAVE_MTGET_TYPE
#define	HAVE_MTGET_DSREG
#define	HAVE_MTGET_ERREG
#define	HAVE_MTGET_RESID
#define	HAVE_MTGET_FILENO
#define	HAVE_MTGET_BLKNO

/*
 * Define some junk here as software may assume that these two definitions
 * are always present.
 */
#define	MTIOCGET	0x12340001
#define	MTIOCTOP	0x12340002

#ifdef	__cplusplus
}
#endif

#endif	/* HAVE_SYS_MTIO_H */

#endif /* _SCHILY_MTIO_H */
