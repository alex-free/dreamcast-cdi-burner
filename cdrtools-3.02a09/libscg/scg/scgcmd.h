/* @(#)scgcmd.h	2.26 07/11/22 Copyright 1986 J. Schilling */
/*
 *	Definitions for the SCSI 'scg_cmd' structure that has been created
 *	for the SCSI general driver 'scg' for SunOS and Solaris but
 *	now is used for wrapping general libscg SCSI transport requests.
 *
 *	Copyright (c) 1986 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * The following exceptions apply:
 * CDDL §3.6 needs to be replaced by: "You may create a Larger Work by
 * combining Covered Software with other code if all other code is governed by
 * the terms of a license that is OSI approved (see www.opensource.org) and
 * you may distribute the Larger Work as a single product. In such a case,
 * You must make sure the requirements of this License are fulfilled for
 * the Covered Software."
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifndef	_SCG_SCGCMD_H
#define	_SCG_SCGCMD_H

#ifdef	_KERNEL
#define	scsi_status	scg_status
#define	scsi_sense	scg_sense
#define	scsi_ext_sense	scg_ext_sense
#endif

#include <schily/utypes.h>
#include <schily/btorder.h>

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */
#else
# if	defined(_BIT_FIELDS_HTOL)	/* Motorola byteorder */
# else
/*
 * #error will not work for all compilers (e.g. sunos4)
 * The following line will abort compilation on all compilers
 * if none of the above is defines. And that's  what we want.
 */
error  One of _BIT_FIELDS_LTOH or _BIT_FIELDS_HTOL must be defined
# endif
#endif

#include <scg/scsisense.h>
#include <scg/scsicdb.h>
#include <schily/intcvt.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Leave these definitions here if possible to avoid the need to
 * include scsireg.h which makes problems on some OS because these
 * OS define the same types as in scsireg.h
 */

/*
 * SCSI status bits.
 */
#define	ST_VU_00	0x01	/* Vendor unique		*/
#define	ST_CHK_COND	0x02	/* Check condition		*/
#define	ST_COND_MET	0x04	/* Condition met		*/
#define	ST_BUSY		0x08	/* Busy				*/
#define	ST_IS_SEND	0x10	/* Intermediate status send	*/
#define	ST_VU_05	0x20	/* Vendor unique		*/
#define	ST_VU_06	0x40	/* Vendor unique		*/
#define	ST_RSVD_07	0x80	/* Reserved			*/

/*
 * Sense key values for extended sense.
 */
#define	SC_NO_SENSE		0x00
#define	SC_RECOVERABLE_ERROR	0x01
#define	SC_NOT_READY		0x02
#define	SC_MEDIUM_ERROR		0x03
#define	SC_HARDWARE_ERROR	0x04
#define	SC_ILLEGAL_REQUEST	0x05
#define	SC_UNIT_ATTENTION	0x06
#define	SC_WRITE_PROTECT	0x07
#define	SC_BLANK_CHECK		0x08
#define	SC_VENDOR_UNIQUE	0x09
#define	SC_COPY_ABORTED		0x0A
#define	SC_ABORTED_COMMAND	0x0B
#define	SC_EQUAL		0x0C
#define	SC_VOLUME_OVERFLOW	0x0D
#define	SC_MISCOMPARE		0x0E
#define	SC_RESERVED		0x0F

/*
 * Messages that SCSI can send.
 */
#define	SC_COMMAND_COMPLETE	0x00
#define	SC_SYNCHRONOUS		0x01
#define	SC_SAVE_DATA_PTR	0x02
#define	SC_RESTORE_PTRS		0x03
#define	SC_DISCONNECT		0x04
#define	SC_ABORT		0x06
#define	SC_MSG_REJECT		0x07
#define	SC_NO_OP		0x08
#define	SC_PARITY		0x09
#define	SC_IDENTIFY		0x80
#define	SC_DR_IDENTIFY		0xc0
#define	SC_DEVICE_RESET		0x0c

#define	SC_G0_CDBLEN	6	/* Len of Group 0 commands */
#define	SC_G1_CDBLEN	10	/* Len of Group 1 commands */
#define	SC_G5_CDBLEN	12	/* Len of Group 5 commands */

#define	SCG_MAX_CMD	24	/* 24 bytes max. size is supported */
#define	SCG_MAX_STATUS	3	/* XXX (sollte 4 allign.) Mamimum Status Len */
#define	SCG_MAX_SENSE	32	/* Mamimum Sense Len for auto Req. Sense */

#define	DEF_SENSE_LEN	16	/* Default Sense Len */
#define	CCS_SENSE_LEN	18	/* Sense Len for CCS compatible devices */

struct	scg_cmd {
	caddr_t	addr;			/* Address of data in user space */
	int	size;			/* DMA count for data transfer */
	int	flags;			/* see below for definition */
	int	cdb_len;		/* Size of SCSI command in bytes */
					/* NOTE: rel 4 uses this field only */
					/* with commands not in group 1 or 2*/
	int	sense_len;		/* for intr() if -1 don't get sense */
	int	timeout;		/* timeout in seconds */
					/* NOTE: actual resolution depends */
					/* on driver implementation */
	int	kdebug;			/* driver kernel debug level */
	int	resid;			/* Bytes not transfered */
	int	error;			/* Error code from scgintr() */
	int	ux_errno;		/* UNIX error code */
	union {
		struct	scsi_status Scb; /* Status returnd by command */
		Uchar	cmd_scb[SCG_MAX_STATUS];
	} u_scb;
#define	scb	u_scb.Scb

	union {
		struct	scsi_sense Sense; /* Sense bytes from command */
		Uchar	cmd_sense[SCG_MAX_SENSE];
	} u_sense;
#define	sense	u_sense.Sense
	int	sense_count;		/* Number of bytes valid in sense */
	int	target;			/* SCSI target id */
					/* NOTE: The SCSI target id field   */
					/* does not need to be filled unless */
					/* the low level transport is a real */
					/* scg driver. In this case the low */
					/* level transport routine of libscg */
					/* will fill in the needed value    */
	union {				/* SCSI command descriptor block */
		struct	scsi_g0cdb g0_cdb;
		struct	scsi_g1cdb g1_cdb;
		struct	scsi_g5cdb g5_cdb;
		Uchar	cmd_cdb[SCG_MAX_CMD];
	} cdb;				/* 24 bytes max. size is supported */
};

#if	defined(_KERNEL) && defined(_MULTI_DATAMODEL)
struct	scg_cmd32 {
	caddr32_t	addr;			/* Address of data in user space */
	int	size;			/* DMA count for data transfer */
	int	flags;			/* see below for definition */
	int	cdb_len;		/* Size of SCSI command in bytes */
					/* NOTE: rel 4 uses this field only */
					/* with commands not in group 1 or 2*/
	int	sense_len;		/* for intr() if -1 don't get sense */
	int	timeout;		/* timeout in seconds */
					/* NOTE: actual resolution depends */
					/* on driver implementation */
	int	kdebug;			/* driver kernel debug level */
	int	resid;			/* Bytes not transfered */
	int	error;			/* Error code from scgintr() */
	int	ux_errno;		/* UNIX error code */
	union {
		struct	scsi_status Scb; /* Status returnd by command */
		Uchar	cmd_scb[SCG_MAX_STATUS];
	} u_scb;
#define	scb	u_scb.Scb

	union {
		struct	scsi_sense Sense; /* Sense bytes from command */
		Uchar	cmd_sense[SCG_MAX_SENSE];
	} u_sense;
#define	sense	u_sense.Sense
	int	sense_count;		/* Number of bytes valid in sense */
	int	target;			/* SCSI target id */
	union {				/* SCSI command descriptor block */
		struct	scsi_g0cdb g0_cdb;
		struct	scsi_g1cdb g1_cdb;
		struct	scsi_g5cdb g5_cdb;
		Uchar	cmd_cdb[SCG_MAX_CMD];
	} cdb;				/* 24 bytes max. size is supported */
};
#endif	/* defined(_KERNEL) && defined(_MULTI_DATAMODEL) */

#define	dma_read	flags		/* 1 if DMA to Sun, 0 otherwise */

/*
 * definition for flags field in scg_cmd struct
 */
#define	SCG_RECV_DATA	0x0001		/* DMA direction to Sun */
#define	SCG_DISRE_ENA	0x0002		/* enable disconnect/reconnect */
#define	SCG_SILENT	0x0004		/* be silent on errors */
#define	SCG_CMD_RETRY	0x0008		/* enable retries */
#define	SCG_NOPARITY	0x0010		/* disable parity for this command */

/*
 * definition for error field in scg_cmd struct
 *
 * The codes refer to SCSI general errors, not to device
 * specific errors.  Device specific errors are discovered
 * by checking the sense data.
 * The distinction between retryable and fatal is somewhat ad hoc.
 */
#define	SCG_NO_ERROR	0		/* cdb transported without error    */
					/* SCG_NO_ERROR incudes all commands */
					/* where the SCSI status is valid   */

#define	SCG_RETRYABLE	1		/* any other case e.g. SCSI bus busy */
					/* SCSI cdb could not be send,	    */
					/* includes DMA errors other than   */
					/* DMA underrun			    */

#define	SCG_FATAL	2		/* could not select target	    */
#define	SCG_TIMEOUT	3		/* driver timed out		    */


#ifdef	__cplusplus
}
#endif

#ifdef	_KERNEL
#undef	scsi_status
#undef	scsi_sense
#undef	scsi_ext_sense
#endif

#endif	/* _SCG_SCGCMD_H */
