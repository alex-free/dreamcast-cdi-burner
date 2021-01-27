/* @(#)scsicdb.h	2.20 06/09/13 Copyright 1986 J. Schilling */
/*
 *	Definitions for the SCSI Command Descriptor Block
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

#ifndef	_SCG_SCSICDB_H
#define	_SCG_SCSICDB_H

#ifndef  _SCHILY_UTYPES_H
#include <schily/utypes.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif
/*
 * SCSI Operation codes.
 */
#define	SC_TEST_UNIT_READY	0x00
#define	SC_REZERO_UNIT		0x01
#define	SC_REQUEST_SENSE	0x03
#define	SC_FORMAT		0x04
#define	SC_FORMAT_TRACK		0x06
#define	SC_REASSIGN_BLOCK	0x07		/* CCS only */
#define	SC_SEEK			0x0b
#define	SC_TRANSLATE		0x0f		/* ACB4000 only */
#define	SC_INQUIRY		0x12		/* CCS only */
#define	SC_MODE_SELECT		0x15
#define	SC_RESERVE		0x16
#define	SC_RELEASE		0x17
#define	SC_MODE_SENSE		0x1a
#define	SC_START		0x1b
#define	SC_READ_DEFECT_LIST	0x37		/* CCS only, group 1 */
#define	SC_READ_BUFFER		0x3c		/* CCS only, group 1 */
	/*
	 * Note, these two commands use identical command blocks for all
	 * controllers except the Adaptec ACB 4000 which sets bit 1 of byte 1.
	 */
#define	SC_READ			0x08
#define	SC_WRITE		0x0a
#define	SC_EREAD		0x28		/* 10 byte read */
#define	SC_EWRITE		0x2a		/* 10 byte write */
#define	SC_WRITE_VERIFY		0x2e		/* 10 byte write+verify */
#define	SC_WRITE_FILE_MARK	0x10
#define	SC_UNKNOWN		0xff		/* cmd list terminator */


/*
 * Standard SCSI control blocks.
 * These go in or out over the SCSI bus.
 */

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct	scsi_g0cdb {		/* scsi group 0 command description block */
	Uchar	cmd;		/* command code */
	Ucbit	high_addr : 5;	/* high part of block address */
	Ucbit	lun	  : 3;	/* logical unit number */
	Uchar	mid_addr;	/* middle part of block address */
	Uchar	low_addr;	/* low part of block address */
	Uchar	count;		/* transfer length */
	Ucbit	link	  : 1;	/* link (another command follows) */
	Ucbit	fr	  : 1;	/* flag request (interrupt at completion) */
	Ucbit	naca	  : 1;	/* Normal ACA (Auto Contingent Allegiance) */
	Ucbit	rsvd	  : 3;	/* reserved */
	Ucbit	vu_56	  : 1;	/* vendor unique (byte 5 bit 6) */
	Ucbit	vu_57	  : 1;	/* vendor unique (byte 5 bit 7) */
};

#else	/* Motorola byteorder */

struct	scsi_g0cdb {		/* scsi group 0 command description block */
	Uchar	cmd;		/* command code */
	Ucbit	lun	  : 3;	/* logical unit number */
	Ucbit	high_addr : 5;	/* high part of block address */
	Uchar	mid_addr;	/* middle part of block address */
	Uchar	low_addr;	/* low part of block address */
	Uchar	count;		/* transfer length */
	Ucbit	vu_57	  : 1;	/* vendor unique (byte 5 bit 7) */
	Ucbit	vu_56	  : 1;	/* vendor unique (byte 5 bit 6) */
	Ucbit	rsvd	  : 3;	/* reserved */
	Ucbit	naca	  : 1;	/* Normal ACA (Auto Contingent Allegiance) */
	Ucbit	fr	  : 1;	/* flag request (interrupt at completion) */
	Ucbit	link	  : 1;	/* link (another command follows) */
};
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct	scsi_g1cdb {		/* scsi group 1 command description block */
	Uchar	cmd;		/* command code */
	Ucbit	reladr	  : 1;	/* address is relative */
	Ucbit	res	  : 4;	/* reserved bits 1-4 of byte 1 */
	Ucbit	lun	  : 3;	/* logical unit number */
	Uchar	addr[4];	/* logical block address */
	Uchar	res6;		/* reserved byte 6 */
	Uchar	count[2];	/* transfer length */
	Ucbit	link	  : 1;	/* link (another command follows) */
	Ucbit	fr	  : 1;	/* flag request (interrupt at completion) */
	Ucbit	naca	  : 1;	/* Normal ACA (Auto Contingent Allegiance) */
	Ucbit	rsvd	  : 3;	/* reserved */
	Ucbit	vu_96	  : 1;	/* vendor unique (byte 5 bit 6) */
	Ucbit	vu_97	  : 1;	/* vendor unique (byte 5 bit 7) */
};

#else	/* Motorola byteorder */

struct	scsi_g1cdb {		/* scsi group 1 command description block */
	Uchar	cmd;		/* command code */
	Ucbit	lun	  : 3;	/* logical unit number */
	Ucbit	res	  : 4;	/* reserved bits 1-4 of byte 1 */
	Ucbit	reladr	  : 1;	/* address is relative */
	Uchar	addr[4];	/* logical block address */
	Uchar	res6;		/* reserved byte 6 */
	Uchar	count[2];	/* transfer length */
	Ucbit	vu_97	  : 1;	/* vendor unique (byte 5 bit 7) */
	Ucbit	vu_96	  : 1;	/* vendor unique (byte 5 bit 6) */
	Ucbit	rsvd	  : 3;	/* reserved */
	Ucbit	naca	  : 1;	/* Normal ACA (Auto Contingent Allegiance) */
	Ucbit	fr	  : 1;	/* flag request (interrupt at completion) */
	Ucbit	link	  : 1;	/* link (another command follows) */
};
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct	scsi_g5cdb {		/* scsi group 5 command description block */
	Uchar	cmd;		/* command code */
	Ucbit	reladr	  : 1;	/* address is relative */
	Ucbit	res	  : 4;	/* reserved bits 1-4 of byte 1 */
	Ucbit	lun	  : 3;	/* logical unit number */
	Uchar	addr[4];	/* logical block address */
	Uchar	count[4];	/* transfer length */
	Uchar	res10;		/* reserved byte 10 */
	Ucbit	link	  : 1;	/* link (another command follows) */
	Ucbit	fr	  : 1;	/* flag request (interrupt at completion) */
	Ucbit	naca	  : 1;	/* Normal ACA (Auto Contingent Allegiance) */
	Ucbit	rsvd	  : 3;	/* reserved */
	Ucbit	vu_B6	  : 1;	/* vendor unique (byte B bit 6) */
	Ucbit	vu_B7	  : 1;	/* vendor unique (byte B bit 7) */
};

#else	/* Motorola byteorder */

struct	scsi_g5cdb {		/* scsi group 5 command description block */
	Uchar	cmd;		/* command code */
	Ucbit	lun	  : 3;	/* logical unit number */
	Ucbit	res	  : 4;	/* reserved bits 1-4 of byte 1 */
	Ucbit	reladr	  : 1;	/* address is relative */
	Uchar	addr[4];	/* logical block address */
	Uchar	count[4];	/* transfer length */
	Uchar	res10;		/* reserved byte 10 */
	Ucbit	vu_B7	  : 1;	/* vendor unique (byte B bit 7) */
	Ucbit	vu_B6	  : 1;	/* vendor unique (byte B bit 6) */
	Ucbit	rsvd	  : 3;	/* reserved */
	Ucbit	naca	  : 1;	/* Normal ACA (Auto Contingent Allegiance) */
	Ucbit	fr	  : 1;	/* flag request (interrupt at completion) */
	Ucbit	link	  : 1;	/* link (another command follows) */
};
#endif

#define	g0_cdbaddr(cdb, a)	((cdb)->high_addr   = (a) >> 16,\
				    (cdb)->mid_addr = ((a) >> 8) & 0xFF,\
				    (cdb)->low_addr = (a) & 0xFF)

#define	g1_cdbaddr(cdb, a)	((cdb)->addr[0]    = (a) >> 24,\
				    (cdb)->addr[1] = ((a) >> 16)& 0xFF,\
				    (cdb)->addr[2] = ((a) >> 8) & 0xFF,\
				    (cdb)->addr[3] = (a) & 0xFF)

#define	g5_cdbaddr(cdb, a)	g1_cdbaddr(cdb, a)


#define	g0_cdblen(cdb, len)	((cdb)->count = (len))

#define	g1_cdblen(cdb, len)	((cdb)->count[0]    = ((len) >> 8) & 0xFF,\
				    (cdb)->count[1] = (len) & 0xFF)

#define	g5_cdblen(cdb, len)	((cdb)->count[0]    = (len) >> 24L,\
				    (cdb)->count[1] = ((len) >> 16L)& 0xFF,\
				    (cdb)->count[2] = ((len) >> 8L) & 0xFF,\
				    (cdb)->count[3] = (len) & 0xFF)

/*#define	XXXXX*/
#ifdef	XXXXX
#define	i_to_long(a, i)		(((Uchar    *)(a))[0] = ((i) >> 24)& 0xFF,\
				    ((Uchar *)(a))[1] = ((i) >> 16)& 0xFF,\
				    ((Uchar *)(a))[2] = ((i) >> 8) & 0xFF,\
				    ((Uchar *)(a))[3] = (i) & 0xFF)

#define	i_to_3_byte(a, i)	(((Uchar    *)(a))[0] = ((i) >> 16)& 0xFF,\
				    ((Uchar *)(a))[1] = ((i) >> 8) & 0xFF,\
				    ((Uchar *)(a))[2] = (i) & 0xFF)

#define	i_to_4_byte(a, i)	(((Uchar    *)(a))[0] = ((i) >> 24)& 0xFF,\
				    ((Uchar *)(a))[1] = ((i) >> 16)& 0xFF,\
				    ((Uchar *)(a))[2] = ((i) >> 8) & 0xFF,\
				    ((Uchar *)(a))[3] = (i) & 0xFF)

#define	i_to_short(a, i)	(((Uchar *)(a))[0] = ((i) >> 8) & 0xFF,\
				    ((Uchar *)(a))[1] = (i) & 0xFF)

#define	a_to_u_short(a)	((unsigned short) \
			((((Uchar*)    a)[1]	   & 0xFF) | \
			    (((Uchar*) a)[0] << 8  & 0xFF00)))

#define	a_to_3_byte(a)	((Ulong) \
			((((Uchar*)    a)[2]	   & 0xFF) | \
			    (((Uchar*) a)[1] << 8  & 0xFF00) | \
			    (((Uchar*) a)[0] << 16 & 0xFF0000)))

#ifdef	__STDC__
#define	a_to_u_long(a)	((Ulong) \
			((((Uchar*)    a)[3]	   & 0xFF) | \
			    (((Uchar*) a)[2] << 8  & 0xFF00) | \
			    (((Uchar*) a)[1] << 16 & 0xFF0000) | \
			    (((Uchar*) a)[0] << 24 & 0xFF000000UL)))
#else
#define	a_to_u_long(a)	((Ulong) \
			((((Uchar*)    a)[3]	   & 0xFF) | \
			    (((Uchar*) a)[2] << 8  & 0xFF00) | \
			    (((Uchar*) a)[1] << 16 & 0xFF0000) | \
			    (((Uchar*) a)[0] << 24 & 0xFF000000)))
#endif
#endif	/* XXXX */


#ifdef	__cplusplus
}
#endif

#endif	/* _SCG_SCSICDB_H */
