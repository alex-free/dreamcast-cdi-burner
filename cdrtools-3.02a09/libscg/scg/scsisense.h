/* @(#)scsisense.h	2.18 04/09/04 Copyright 1986 J. Schilling */
/*
 *	Definitions for the SCSI status code and sense structure
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

#ifndef	_SCG_SCSISENSE_H
#define	_SCG_SCSISENSE_H

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * SCSI status completion block.
 */
#define	SCSI_EXTENDED_STATUS

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct	scsi_status {
	Ucbit	vu_00	: 1;	/* vendor unique */
	Ucbit	chk	: 1;	/* check condition: sense data available */
	Ucbit	cm	: 1;	/* condition met */
	Ucbit	busy	: 1;	/* device busy or reserved */
	Ucbit	is	: 1;	/* intermediate status sent */
	Ucbit	vu_05	: 1;	/* vendor unique */
#define	st_scsi2	vu_05	/* SCSI-2 modifier bit */
	Ucbit	vu_06	: 1;	/* vendor unique */
	Ucbit	st_rsvd	: 1;	/* reserved */

#ifdef	SCSI_EXTENDED_STATUS
#define	ext_st1	st_rsvd		/* extended status (next byte valid) */
	/* byte 1 */
	Ucbit	ha_er	: 1;	/* host adapter detected error */
	Ucbit	reserved: 6;	/* reserved */
	Ucbit	ext_st2	: 1;	/* extended status (next byte valid) */
	/* byte 2 */
	Uchar	byte2;		/* third byte */
#endif	/* SCSI_EXTENDED_STATUS */
};

#else	/* Motorola byteorder */

struct	scsi_status {
	Ucbit	st_rsvd	: 1;	/* reserved */
	Ucbit	vu_06	: 1;	/* vendor unique */
	Ucbit	vu_05	: 1;	/* vendor unique */
#define	st_scsi2	vu_05	/* SCSI-2 modifier bit */
	Ucbit	is	: 1;	/* intermediate status sent */
	Ucbit	busy	: 1;	/* device busy or reserved */
	Ucbit	cm	: 1;	/* condition met */
	Ucbit	chk	: 1;	/* check condition: sense data available */
	Ucbit	vu_00	: 1;	/* vendor unique */
#ifdef	SCSI_EXTENDED_STATUS
#define	ext_st1	st_rsvd		/* extended status (next byte valid) */
	/* byte 1 */
	Ucbit	ext_st2	: 1;	/* extended status (next byte valid) */
	Ucbit	reserved: 6;	/* reserved */
	Ucbit	ha_er	: 1;	/* host adapter detected error */
	/* byte 2 */
	Uchar	byte2;		/* third byte */
#endif	/* SCSI_EXTENDED_STATUS */
};
#endif

/*
 * OLD Standard (Non Extended) SCSI Sense. Used mainly by the
 * Adaptec ACB 4000 which is the only controller that
 * does not support the Extended sense format.
 */
#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct	scsi_sense {		/* scsi sense for error classes 0-6 */
	Ucbit	code	: 7;	/* error class/code */
	Ucbit	adr_val	: 1;	/* sense data is valid */
#ifdef	comment
	Ucbit	high_addr:5;	/* high byte of block addr */
	Ucbit	rsvd	: 3;
#else
	Uchar	high_addr;	/* high byte of block addr */
#endif
	Uchar	mid_addr;	/* middle byte of block addr */
	Uchar	low_addr;	/* low byte of block addr */
};

#else	/* Motorola byteorder */

struct	scsi_sense {		/* scsi sense for error classes 0-6 */
	Ucbit	adr_val	: 1;	/* sense data is valid */
	Ucbit	code	: 7;	/* error class/code */
#ifdef	comment
	Ucbit	rsvd	: 3;
	Ucbit	high_addr:5;	/* high byte of block addr */
#else
	Uchar	high_addr;	/* high byte of block addr */
#endif
	Uchar	mid_addr;	/* middle byte of block addr */
	Uchar	low_addr;	/* low byte of block addr */
};
#endif

/*
 * SCSI extended sense parameter block.
 */
#ifdef	comment
#define	SC_CLASS_EXTENDED_SENSE 0x7	/* indicates extended sense */
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct	scsi_ext_sense {	/* scsi extended sense for error class 7 */
	/* byte 0 */
	Ucbit	type	: 7;	/* fixed at 0x70 */
	Ucbit	adr_val	: 1;	/* sense data is valid */
	/* byte 1 */
	Uchar	seg_num;	/* segment number, applies to copy cmd only */
	/* byte 2 */
	Ucbit	key	: 4;	/* sense key, see below */
	Ucbit		: 1;	/* reserved */
	Ucbit	ili	: 1;	/* incorrect length indicator */
	Ucbit	eom	: 1;	/* end of media */
	Ucbit	fil_mk	: 1;	/* file mark on device */
	/* bytes 3 through 7 */
	Uchar	info_1;		/* information byte 1 */
	Uchar	info_2;		/* information byte 2 */
	Uchar	info_3;		/* information byte 3 */
	Uchar	info_4;		/* information byte 4 */
	Uchar	add_len;	/* number of additional bytes */
	/* bytes 8 through 13, CCS additions */
	Uchar	optional_8;	/* CCS search and copy only */
	Uchar	optional_9;	/* CCS search and copy only */
	Uchar	optional_10;	/* CCS search and copy only */
	Uchar	optional_11;	/* CCS search and copy only */
	Uchar 	sense_code;	/* sense code */
	Uchar	qual_code;	/* sense code qualifier */
	Uchar	fru_code;	/* Field replacable unit code */
	Ucbit	bptr	: 3;	/* bit pointer for failure (if bpv) */
	Ucbit	bpv	: 1;	/* bit pointer is valid */
	Ucbit		: 2;
	Ucbit	cd	: 1;	/* pointers refer to command not data */
	Ucbit	sksv	: 1;	/* sense key specific valid */
	Uchar	field_ptr[2];	/* field pointer for failure */
	Uchar	add_info[2];	/* round up to 20 bytes */
};

#else	/* Motorola byteorder */

struct	scsi_ext_sense {	/* scsi extended sense for error class 7 */
	/* byte 0 */
	Ucbit	adr_val	: 1;	/* sense data is valid */
	Ucbit	type	: 7;	/* fixed at 0x70 */
	/* byte 1 */
	Uchar	seg_num;	/* segment number, applies to copy cmd only */
	/* byte 2 */
	Ucbit	fil_mk	: 1;	/* file mark on device */
	Ucbit	eom	: 1;	/* end of media */
	Ucbit	ili	: 1;	/* incorrect length indicator */
	Ucbit		: 1;	/* reserved */
	Ucbit	key	: 4;	/* sense key, see below */
	/* bytes 3 through 7 */
	Uchar	info_1;		/* information byte 1 */
	Uchar	info_2;		/* information byte 2 */
	Uchar	info_3;		/* information byte 3 */
	Uchar	info_4;		/* information byte 4 */
	Uchar	add_len;	/* number of additional bytes */
	/* bytes 8 through 13, CCS additions */
	Uchar	optional_8;	/* CCS search and copy only */
	Uchar	optional_9;	/* CCS search and copy only */
	Uchar	optional_10;	/* CCS search and copy only */
	Uchar	optional_11;	/* CCS search and copy only */
	Uchar 	sense_code;	/* sense code */
	Uchar	qual_code;	/* sense code qualifier */
	Uchar	fru_code;	/* Field replacable unit code */
	Ucbit	sksv	: 1;	/* sense key specific valid */
	Ucbit	cd	: 1;	/* pointers refer to command not data */
	Ucbit		: 2;
	Ucbit	bpv	: 1;	/* bit pointer is valid */
	Ucbit	bptr	: 3;	/* bit pointer for failure (if bpv) */
	Uchar	field_ptr[2];	/* field pointer for failure */
	Uchar	add_info[2];	/* round up to 20 bytes */
};
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SCG_SCSISENSE_H */
