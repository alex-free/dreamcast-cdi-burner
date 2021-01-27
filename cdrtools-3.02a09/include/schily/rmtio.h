/* @(#)rmtio.h	1.7 10/08/24 Copyright 1995,2000-2010 J. Schilling */
/*
 *	Definition for enhanced remote tape IO
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

#ifndef	_SCHILY_RMTIO_H
#define	_SCHILY_RMTIO_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef	_SCHILY_UTYPES_H
#include <schily/utypes.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * values for mt_op
 */
#define	RMTWEOF		0	/* write an end-of-file record */
#define	RMTFSF		1	/* forward space over file mark */
#define	RMTBSF		2	/* backward space over file mark (1/2" only ) */
#define	RMTFSR		3	/* forward space to inter-record gap */
#define	RMTBSR		4	/* backward space to inter-record gap */
#define	RMTREW		5	/* rewind */
#define	RMTOFFL		6	/* rewind and put the drive offline */
#define	RMTNOP		7	/* no operation, sets status only */

#ifdef	__needed__
#define	MTRETEN		8	/* retension the tape (cartridge tape only) */
#define	MTERASE		9	/* erase the entire tape */
#define	MTEOM		10	/* position to end of media */
#define	MTNBSF		11	/* backward space file to BOF */

#define	MTSRSZ		12	/* set record size */
#define	MTGRSZ		13	/* get record size */
#define	MTLOAD		14	/* for loading a tape (use o_delay to open */
				/* the tape device) */
#endif

/*
 * Definitions for the new RMT Protocol version 1
 *
 * The new Protocol version tries to make the use
 * of rmtioctl() more portable between different platforms.
 */
#define	RMTIVERSION	-1	/* Opcode to request version */
#define	RMT_NOVERSION	-1	/* Old version code */
#define	RMT_VERSION	1	/* New (current) version code */

/*
 * Support for commands bejond MTWEOF..MTNOP (0..7)
 */
#define	RMTICACHE	0	/* enable controller cache */
#define	RMTINOCACHE	1	/* disable controller cache */
#define	RMTIRETEN	2	/* retension the tape (cartridge tape only) */
#define	RMTIERASE	3	/* erase the entire tape */
#define	RMTIEOM		4	/* position to end of media */
#define	RMTINBSF	5	/* backward space file to BOF */

/*
 * Old MTIOCGET copies a binary version of struct mtget back
 * over the wire. This is highly non portable.
 * MTS_* retrieves ascii versions (%d format) of a single
 * field in the struct mtget.
 * NOTE: MTS_ERREG may only be valid on the first call and
 *	 must be retrived first.
 */
#define	MTS_TYPE	'T'		/* mtget.mt_type */
#define	MTS_DSREG	'D'		/* mtget.mt_dsreg */
#define	MTS_ERREG	'E'		/* mtget.mt_erreg */
#define	MTS_RESID	'R'		/* mtget.mt_resid */
#define	MTS_FILENO	'F'		/* mtget.mt_fileno */
#define	MTS_BLKNO	'B'		/* mtget.mt_blkno */
#define	MTS_FLAGS	'f'		/* mtget.mt_flags */
#define	MTS_BF		'b'		/* mtget.mt_bf */

/*
 * structure for remote MTIOCGET - mag tape get status command
 */
struct rmtget	{
	Llong		mt_type;	/* type of magtape device	    */
	/* the following two registers are grossly device dependent	    */
	Llong		mt_dsreg;	/* ``drive status'' register	    */
	Int32_t		mt_dsreg1;	/* ``drive status'' register	    */
	Int32_t		mt_dsreg2;	/* ``drive status'' register	    */
	Llong		mt_gstat;	/* ``generic status'' register	    */
	Llong		mt_erreg;	/* ``error'' register		    */
	/* optional error info.						    */
	Llong		mt_resid;	/* residual count		    */
	Llong		mt_fileno;	/* file number of current position  */
	Llong		mt_blkno;	/* block number of current position */
	Llong		mt_flags;
	Llong		mt_gflags;	/* generic flags		    */
	long		mt_bf;		/* optimum blocking factor	    */
	int		mt_xflags;	/* eXistence flags for struct members */
};

/*
 * Values for mt_xflags
 */
#define	RMT_TYPE		0x0001	/* mt_type/mt_model present	*/
#define	RMT_DSREG		0x0002	/* mt_dsreg present		*/
#define	RMT_DSREG1		0x0004	/* mt_dsreg1 present		*/
#define	RMT_DSREG2		0x0008	/* mt_dsreg2 present		*/
#define	RMT_GSTAT		0x0010	/* mt_gstat present		*/
#define	RMT_ERREG		0x0020	/* mt_erreg present		*/
#define	RMT_RESID		0x0040	/* mt_resid present		*/
#define	RMT_FILENO		0x0080	/* mt_fileno present		*/
#define	RMT_BLKNO		0x0100	/* mt_blkno present		*/
#define	RMT_FLAGS		0x0200	/* mt_flags present		*/
#define	RMT_BF			0x0400	/* mt_bf present		*/
#define	RMT_COMPAT		0x0800	/* Created from old compat data	*/

/*
 * values for mt_flags
 */
#define	RMTF_SCSI		0x01
#define	RMTF_REEL		0x02
#define	RMTF_ASF		0x04
#define	RMTF_TAPE_HEAD_DIRTY	0x08
#define	RMTF_TAPE_CLN_SUPPORTED	0x10

/*
 * these are recommended
 */
#ifdef	__needed__
#define	MT_ISQIC	0x32		/* generic QIC tape drive */
#define	MT_ISREEL	0x33		/* generic reel tape drive */
#define	MT_ISDAT	0x34		/* generic DAT tape drive */
#define	MT_IS8MM	0x35		/* generic 8mm tape drive */
#define	MT_ISOTHER	0x36		/* generic other type of tape drive */

/* more Sun devices */
#define	MT_ISTAND25G	0x37		/* sun: SCSI Tandberg 2.5 Gig QIC */
#define	MT_ISDLT	0x38		/* sun: SCSI DLT tape drive */
#define	MT_ISSTK9840	0x39		/* sun: STK 9840 (Ironsides) */
#endif

#ifdef	__cplusplus
}
#endif

#endif /* _SCHILY_RMTIO_H */
