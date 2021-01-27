/* @(#)scsimmc.h	1.19 06/12/02 Copyright 1997-2006 J. Schilling */
/*
 *	Definitions for SCSI/mmc compliant drives
 *
 *	Copyright (c) 1997-2006 J. Schilling
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

#ifndef	_SCSIMMC_H
#define	_SCSIMMC_H

#include <schily/utypes.h>
#include <schily/btorder.h>

typedef struct opc {
	Uchar	opc_speed[2];
	Uchar	opc_val[6];
} opc_t;

#if defined(_BIT_FIELDS_LTOH)	/* Intel bitorder */

struct disk_info {
	Uchar	data_len[2];		/* Data len without this info	*/
	Ucbit	disk_status	: 2;	/* Status of the disk		*/
	Ucbit	sess_status	: 2;	/* Status of last session	*/
	Ucbit	erasable	: 1;	/* Disk is erasable		*/
	Ucbit	dtype		: 3;	/* Disk information data type	*/
	Uchar	first_track;		/* # of first track on disk	*/
	Uchar	numsess;		/* # of sessions		*/
	Uchar	first_track_ls;		/* First track in last session	*/
	Uchar	last_track_ls;		/* Last track in last session	*/
	Ucbit	bg_format_stat	: 2;	/* Background format status	*/
	Ucbit	dbit		: 1;	/* Dirty Bit of defect table	*/
	Ucbit	res7_3		: 1;	/* Reserved			*/
	Ucbit	dac_v		: 1;	/* Disk application code valid	*/
	Ucbit	uru		: 1;	/* This is an unrestricted disk	*/
	Ucbit	dbc_v		: 1;	/* Disk bar code valid		*/
	Ucbit	did_v		: 1;	/* Disk id valid		*/
	Uchar	disk_type;		/* Disk type			*/
	Uchar	numsess_msb;		/* # of sessions (MSB)		*/
	Uchar	first_track_ls_msb;	/* First tr. in last ses. (MSB)	*/
	Uchar	last_track_ls_msb;	/* Last tr. in last ses. (MSB)	*/
	Uchar	disk_id[4];		/* Disk identification		*/
	Uchar	last_lead_in[4];	/* Last session lead in time	*/
	Uchar	last_lead_out[4];	/* Last session lead out time	*/
	Uchar	disk_barcode[8];	/* Disk bar code		*/
	Uchar	disk_appl_code;		/* Disk application code	*/
	Uchar	num_opc_entries;	/* # of OPC table entries	*/
	opc_t	opc_table[1];		/* OPC table 			*/
};

#else				/* Motorola bitorder */

struct disk_info {
	Uchar	data_len[2];		/* Data len without this info	*/
	Ucbit	dtype		: 3;	/* Disk information data type	*/
	Ucbit	erasable	: 1;	/* Disk is erasable		*/
	Ucbit	sess_status	: 2;	/* Status of last session	*/
	Ucbit	disk_status	: 2;	/* Status of the disk		*/
	Uchar	first_track;		/* # of first track on disk	*/
	Uchar	numsess;		/* # of sessions		*/
	Uchar	first_track_ls;		/* First track in last session	*/
	Uchar	last_track_ls;		/* Last track in last session	*/
	Ucbit	did_v		: 1;	/* Disk id valid		*/
	Ucbit	dbc_v		: 1;	/* Disk bar code valid		*/
	Ucbit	uru		: 1;	/* This is an unrestricted disk	*/
	Ucbit	dac_v		: 1;	/* Disk application code valid	*/
	Ucbit	res7_3		: 1;	/* Reserved			*/
	Ucbit	dbit		: 1;	/* Dirty Bit of defect table	*/
	Ucbit	bg_format_stat	: 2;	/* Background format status	*/
	Uchar	disk_type;		/* Disk type			*/
	Uchar	numsess_msb;		/* # of sessions (MSB)		*/
	Uchar	first_track_ls_msb;	/* First tr. in last ses. (MSB)	*/
	Uchar	last_track_ls_msb;	/* Last tr. in last ses. (MSB)	*/
	Uchar	disk_id[4];		/* Disk identification		*/
	Uchar	last_lead_in[4];	/* Last session lead in time	*/
	Uchar	last_lead_out[4];	/* Last session lead out time	*/
	Uchar	disk_barcode[8];	/* Disk bar code		*/
	Uchar	disk_appl_code;		/* Disk application code	*/
	Uchar	num_opc_entries;	/* # of OPC table entries	*/
	opc_t	opc_table[1];		/* OPC table 			*/
};

#endif

struct cd_mode_data {
	struct scsi_mode_header	header;
	union cd_pagex	{
		struct cd_mode_page_05	page05;
		struct cd_mode_page_2A	page2A;
	} pagex;
};

struct tocheader {
	Uchar	len[2];
	Uchar	first;
	Uchar	last;
};

/*
 * Full TOC entry
 */
struct ftrackdesc {
	Uchar	sess_number;

#if defined(_BIT_FIELDS_LTOH)		/* Intel byteorder */
	Ucbit	control		: 4;
	Ucbit	adr		: 4;
#else					/* Motorola byteorder */
	Ucbit	adr		: 4;
	Ucbit	control		: 4;
#endif

	Uchar	track;
	Uchar	point;
	Uchar	amin;
	Uchar	asec;
	Uchar	aframe;
	Uchar	res7;
	Uchar	pmin;
	Uchar	psec;
	Uchar	pframe;
};

struct fdiskinfo {
	struct tocheader	hd;
	struct ftrackdesc	desc[1];
};



#if defined(_BIT_FIELDS_LTOH)	/* Intel bitorder */

struct atipdesc {
	Ucbit	ref_speed	: 3;	/* Reference speed		*/
	Ucbit	res4_3		: 1;	/* Reserved			*/
	Ucbit	ind_wr_power	: 3;	/* Indicative tgt writing power	*/
	Ucbit	res4_7		: 1;	/* Reserved (must be "1")	*/
	Ucbit	res5_05		: 6;	/* Reserved			*/
	Ucbit	uru		: 1;	/* Disk is for unrestricted use	*/
	Ucbit	res5_7		: 1;	/* Reserved (must be "0")	*/
	Ucbit	a3_v		: 1;	/* A 3 Values valid		*/
	Ucbit	a2_v		: 1;	/* A 2 Values valid		*/
	Ucbit	a1_v		: 1;	/* A 1 Values valid		*/
	Ucbit	sub_type	: 3;	/* Disc sub type		*/
	Ucbit	erasable	: 1;	/* Disk is erasable		*/
	Ucbit	res6_7		: 1;	/* Reserved (must be "1")	*/
	Uchar	lead_in[4];		/* Lead in time			*/
	Uchar	lead_out[4];		/* Lead out time		*/
	Uchar	res15;			/* Reserved			*/
	Ucbit	clv_high	: 4;	/* Highes usable CLV recording speed */
	Ucbit	clv_low		: 3;	/* Lowest usable CLV recording speed */
	Ucbit	res16_7		: 1;	/* Reserved (must be "0")	*/
	Ucbit	res17_0		: 1;	/* Reserved			*/
	Ucbit	tgt_y_pow	: 3;	/* Tgt y val of the power mod fun */
	Ucbit	power_mult	: 3;	/* Power multiplication factor	*/
	Ucbit	res17_7		: 1;	/* Reserved (must be "0")	*/
	Ucbit	res_18_30	: 4;	/* Reserved			*/
	Ucbit	rerase_pwr_ratio: 3;	/* Recommended erase/write power*/
	Ucbit	res18_7		: 1;	/* Reserved (must be "1")	*/
	Uchar	res19;			/* Reserved			*/
	Uchar	a2[3];			/* A 2 Values			*/
	Uchar	res23;			/* Reserved			*/
	Uchar	a3[3];			/* A 3 Vaules			*/
	Uchar	res27;			/* Reserved			*/
};

#else				/* Motorola bitorder */

struct atipdesc {
	Ucbit	res4_7		: 1;	/* Reserved (must be "1")	*/
	Ucbit	ind_wr_power	: 3;	/* Indicative tgt writing power	*/
	Ucbit	res4_3		: 1;	/* Reserved			*/
	Ucbit	ref_speed	: 3;	/* Reference speed		*/
	Ucbit	res5_7		: 1;	/* Reserved (must be "0")	*/
	Ucbit	uru		: 1;	/* Disk is for unrestricted use	*/
	Ucbit	res5_05		: 6;	/* Reserved			*/
	Ucbit	res6_7		: 1;	/* Reserved (must be "1")	*/
	Ucbit	erasable	: 1;	/* Disk is erasable		*/
	Ucbit	sub_type	: 3;	/* Disc sub type		*/
	Ucbit	a1_v		: 1;	/* A 1 Values valid		*/
	Ucbit	a2_v		: 1;	/* A 2 Values valid		*/
	Ucbit	a3_v		: 1;	/* A 3 Values valid		*/
	Uchar	lead_in[4];		/* Lead in time			*/
	Uchar	lead_out[4];		/* Lead out time		*/
	Uchar	res15;			/* Reserved			*/
	Ucbit	res16_7		: 1;	/* Reserved (must be "0")	*/
	Ucbit	clv_low		: 3;	/* Lowest usable CLV recording speed */
	Ucbit	clv_high	: 4;	/* Highes usable CLV recording speed */
	Ucbit	res17_7		: 1;	/* Reserved (must be "0")	*/
	Ucbit	power_mult	: 3;	/* Power multiplication factor	*/
	Ucbit	tgt_y_pow	: 3;	/* Tgt y val of the power mod fun */
	Ucbit	res17_0		: 1;	/* Reserved			*/
	Ucbit	res18_7		: 1;	/* Reserved (must be "1")	*/
	Ucbit	rerase_pwr_ratio: 3;	/* Recommended erase/write power*/
	Ucbit	res_18_30	: 4;	/* Reserved			*/
	Uchar	res19;			/* Reserved			*/
	Uchar	a2[3];			/* A 2 Values			*/
	Uchar	res23;			/* Reserved			*/
	Uchar	a3[3];			/* A 3 Vaules			*/
	Uchar	res27;			/* Reserved			*/
};

#endif

struct atipinfo {
	struct tocheader	hd;
	struct atipdesc		desc;
};

/*
 * XXX Check how we may merge Track_info & Rzone_info
 */
#if defined(_BIT_FIELDS_LTOH)	/* Intel bitorder */

struct track_info {
	Uchar	data_len[2];		/* Data len without this info	*/
	Uchar	track_number;		/* Track number for this info	*/
	Uchar	session_number;		/* Session number for this info	*/
	Uchar	res4;			/* Reserved			*/
	Ucbit	track_mode	: 4;	/* Track mode (Q-sub control)	*/
	Ucbit	copy		: 1;	/* This track is a higher copy	*/
	Ucbit	damage		: 1;	/* if 1 & nwa_valid 0: inc track*/
	Ucbit	res5_67		: 2;	/* Reserved			*/
	Ucbit	data_mode	: 4;	/* Data mode of this track	*/
	Ucbit	fp		: 1;	/* This is a fixed packet track	*/
	Ucbit	packet		: 1;	/* This track is in packet mode	*/
	Ucbit	blank		: 1;	/* This is an invisible track	*/
	Ucbit	rt		: 1;	/* This is a reserved track	*/
	Ucbit	nwa_valid	: 1;	/* Next writable addr valid	*/
	Ucbit	res7_17		: 7;	/* Reserved			*/
	Uchar	track_start[4];		/* Track start address		*/
	Uchar	next_writable_addr[4];	/* Next writable address	*/
	Uchar	free_blocks[4];		/* Free usr blocks in this track*/
	Uchar	packet_size[4];		/* Packet size if in fixed mode	*/
	Uchar	track_size[4];		/* # of user data blocks in trk	*/
};

#else				/* Motorola bitorder */

struct track_info {
	Uchar	data_len[2];		/* Data len without this info	*/
	Uchar	track_number;		/* Track number for this info	*/
	Uchar	session_number;		/* Session number for this info	*/
	Uchar	res4;			/* Reserved			*/
	Ucbit	res5_67		: 2;	/* Reserved			*/
	Ucbit	damage		: 1;	/* if 1 & nwa_valid 0: inc track*/
	Ucbit	copy		: 1;	/* This track is a higher copy	*/
	Ucbit	track_mode	: 4;	/* Track mode (Q-sub control)	*/
	Ucbit	rt		: 1;	/* This is a reserved track	*/
	Ucbit	blank		: 1;	/* This is an invisible track	*/
	Ucbit	packet		: 1;	/* This track is in packet mode	*/
	Ucbit	fp		: 1;	/* This is a fixed packet track	*/
	Ucbit	data_mode	: 4;	/* Data mode of this track	*/
	Ucbit	res7_17		: 7;	/* Reserved			*/
	Ucbit	nwa_valid	: 1;	/* Next writable addr valid	*/
	Uchar	track_start[4];		/* Track start address		*/
	Uchar	next_writable_addr[4];	/* Next writable address	*/
	Uchar	free_blocks[4];		/* Free usr blocks in this track*/
	Uchar	packet_size[4];		/* Packet size if in fixed mode	*/
	Uchar	track_size[4];		/* # of user data blocks in trk	*/
};

#endif

/*
 * XXX Check how we may merge Track_info & Rzone_info
 */
#if defined(_BIT_FIELDS_LTOH)	/* Intel bitorder */

struct rzone_info {
	Uchar	data_len[2];		/* Data len without this info	*/
	Uchar	rzone_num_lsb;		/* RZone number LSB		*/
	Uchar	border_num_lsb;		/* Border number LSB		*/
	Uchar	res_4;			/* Reserved			*/
	Ucbit	trackmode	: 4;	/* Track mode			*/
	Ucbit	copy		: 1;	/* Higher generation CD copy	*/
	Ucbit	damage		: 1;	/* Damaged RZone		*/
	Ucbit	ljrs		: 2;	/* Layer jump recording status	*/
	Ucbit	datamode	: 4;	/* Data mode			*/
	Ucbit	fp		: 1;	/* Fixed packet			*/
	Ucbit	incremental	: 1;	/* RZone is to be written incremental */
	Ucbit	blank		: 1;	/* RZone is blank		*/
	Ucbit	rt		: 1;	/* RZone is reserved		*/
	Ucbit	nwa_v		: 1;	/* Next WR address is valid	*/
	Ucbit	lra_v		: 1;	/* Last rec address is valid	*/
	Ucbit	res7_27		: 6;	/* Reserved			*/
	Uchar	rzone_start[4];		/* RZone start address		*/
	Uchar	next_recordable_addr[4]; /* Next recordable address	*/
	Uchar	free_blocks[4];		/* Free blocks in RZone		*/
	Uchar	block_factor[4];	/* # of sectors of disc acc unit */
	Uchar	rzone_size[4];		/* RZone size			*/
	Uchar	last_recorded_addr[4];	/* Last Recorded addr in RZone	*/
	Uchar	rzone_num_msb;		/* RZone number MSB		*/
	Uchar	border_num_msb;		/* Border number MSB		*/
	Uchar	res_34_35[2];		/* Reserved			*/
	Uchar	read_compat_lba[4];	/* Read Compatibilty LBA	*/
	Uchar	next_layer_jump[4];	/* Next layer jump address	*/
	Uchar	last_layer_jump[4];	/* Last layer jump address	*/
};

#else				/* Motorola bitorder */

struct rzone_info {
	Uchar	data_len[2];		/* Data len without this info	*/
	Uchar	rzone_num_lsb;		/* RZone number LSB		*/
	Uchar	border_num_lsb;		/* Border number LSB		*/
	Uchar	res_4;			/* Reserved			*/
	Ucbit	ljrs		: 2;	/* Layer jump recording status	*/
	Ucbit	damage		: 1;	/* Damaged RZone		*/
	Ucbit	copy		: 1;	/* Higher generation CD copy	*/
	Ucbit	trackmode	: 4;	/* Track mode			*/
	Ucbit	rt		: 1;	/* RZone is reserved		*/
	Ucbit	blank		: 1;	/* RZone is blank		*/
	Ucbit	incremental	: 1;	/* RZone is to be written incremental */
	Ucbit	fp		: 1;	/* Fixed packet			*/
	Ucbit	datamode	: 4;	/* Data mode			*/
	Ucbit	res7_27		: 6;	/* Reserved			*/
	Ucbit	lra_v		: 1;	/* Last rec address is valid	*/
	Ucbit	nwa_v		: 1;	/* Next WR address is valid	*/
	Uchar	rzone_start[4];		/* RZone start address		*/
	Uchar	next_recordable_addr[4]; /* Next recordable address	*/
	Uchar	free_blocks[4];		/* Free blocks in RZone		*/
	Uchar	block_factor[4];	/* # of sectors of disc acc unit */
	Uchar	rzone_size[4];		/* RZone size			*/
	Uchar	last_recorded_addr[4];	/* Last Recorded addr in RZone	*/
	Uchar	rzone_num_msb;		/* RZone number MSB		*/
	Uchar	border_num_msb;		/* Border number MSB		*/
	Uchar	res_34_35[2];		/* Reserved			*/
	Uchar	read_compat_lba[4];	/* Read Compatibilty LBA	*/
	Uchar	next_layer_jump[4];	/* Next layer jump address	*/
	Uchar	last_layer_jump[4];	/* Last layer jump address	*/
};

#endif

/*
 * The lrjs values:
 */
#define	LRJS_NONE	0		/* DAO/Incremental/Blank	*/
#define	LRJS_UNSPEC	1		/* WT == LJ but layerjump not set */
#define	LRJS_MANUAL	2		/* Manual layer jump set	*/
#define	LRJS_INTERVAL	3		/* Jump interval size set	*/

#if defined(_BIT_FIELDS_LTOH)	/* Intel bitorder */

struct dvd_structure_00 {
	Uchar	data_len[2];		/* Data len without this info	*/
	Uchar	res23[2];		/* Reserved			*/
	Ucbit	book_version	: 4;	/* DVD Book version		*/
	Ucbit	book_type	: 4;	/* DVD Book type		*/
	Ucbit	maximum_rate	: 4;	/* Maximum data rate (coded)	*/
	Ucbit	disc_size	: 4;	/* Disc size (coded)		*/
	Ucbit	layer_type	: 4;	/* Layer type			*/
	Ucbit	track_path	: 1;	/* 0 = parallel, 1 = opposit dir*/
	Ucbit	numlayers	: 2;	/* Number of Layers (0 == 1)	*/
	Ucbit	res2_7		: 1;	/* Reserved			*/
	Ucbit	track_density	: 4;	/* Track density (coded)	*/
	Ucbit	linear_density	: 4;	/* Linear data density (coded)	*/
	Uchar	res8;			/* Reserved			*/
	Uchar	phys_start[3];		/* Starting Physical sector #	*/
	Uchar	res12;			/* Reserved			*/
	Uchar	phys_end[3];		/* End physical data sector #	*/
	Uchar	res16;			/* Reserved			*/
	Uchar	end_layer0[3];		/* End sector # in layer	*/
	Ucbit	res20		: 7;	/* Reserved			*/
	Ucbit	bca		: 1;	/* BCA flag bit			*/
};

#else				/* Motorola bitorder */

struct dvd_structure_00 {
	Uchar	data_len[2];		/* Data len without this info	*/
	Uchar	res23[2];		/* Reserved			*/
	Ucbit	book_type	: 4;	/* DVD Book type		*/
	Ucbit	book_version	: 4;	/* DVD Book version		*/
	Ucbit	disc_size	: 4;	/* Disc size (coded)		*/
	Ucbit	maximum_rate	: 4;	/* Maximum data rate (coded)	*/
	Ucbit	res2_7		: 1;	/* Reserved			*/
	Ucbit	numlayers	: 2;	/* Number of Layers (0 == 1)	*/
	Ucbit	track_path	: 1;	/* 0 = parallel, 1 = opposit dir*/
	Ucbit	layer_type	: 4;	/* Layer type			*/
	Ucbit	linear_density	: 4;	/* Linear data density (coded)	*/
	Ucbit	track_density	: 4;	/* Track density (coded)	*/
	Uchar	res8;			/* Reserved			*/
	Uchar	phys_start[3];		/* Starting Physical sector #	*/
	Uchar	res12;			/* Reserved			*/
	Uchar	phys_end[3];		/* End physical data sector #	*/
	Uchar	res16;			/* Reserved			*/
	Uchar	end_layer0[3];		/* End sector # in layer	*/
	Ucbit	bca		: 1;	/* BCA flag bit			*/
	Ucbit	res20		: 7;	/* Reserved			*/
};

#endif

struct dvd_structure_01 {
	Uchar	data_len[2];		/* Data len without this info	*/
	Uchar	res23[2];		/* Reserved			*/
	Uchar	copyr_prot_type;	/* Copyright prot system type	*/
	Uchar	region_mgt_info;	/* Region management info	*/
	Uchar	res67[2];		/* Reserved			*/
};

struct dvd_structure_02 {
	Uchar	data_len[2];		/* Data len without this info	*/
	Uchar	res23[2];		/* Reserved			*/
	Uchar	key_data[2048];		/* Disc Key data		*/
};

struct dvd_structure_03 {
	Uchar	data_len[2];		/* Data len without this info	*/
	Uchar	res23[2];		/* Reserved			*/
	Uchar	bca_info[1];		/* BCA information (12-188 bytes)*/
};

struct dvd_structure_04 {
	Uchar	data_len[2];		/* Data len without this info	*/
	Uchar	res23[2];		/* Reserved			*/
	Uchar	man_info[2048];		/* Disc manufacturing info	*/
};

#if defined(_BIT_FIELDS_LTOH)	/* Intel bitorder */

struct dvd_structure_05 {
	Uchar	data_len[2];		/* Data len without this info	*/
	Uchar	res23[2];		/* Reserved			*/
	Ucbit	res4_03		: 4;	/* Reserved			*/
	Ucbit	cgms		: 2;	/* CGMS (see below)		*/
	Ucbit	res4_6		: 1;	/* Reserved			*/
	Ucbit	cpm		: 1;	/* This is copyrighted material	*/
	Uchar	res57[3];		/* Reserved			*/
};

#else				/* Motorola bitorder */

struct dvd_structure_05 {
	Uchar	data_len[2];		/* Data len without this info	*/
	Uchar	res23[2];		/* Reserved			*/
	Ucbit	cpm		: 1;	/* This is copyrighted material	*/
	Ucbit	res4_6		: 1;	/* Reserved			*/
	Ucbit	cgms		: 2;	/* CGMS (see below)		*/
	Ucbit	res4_03		: 4;	/* Reserved			*/
	Uchar	res57[3];		/* Reserved			*/
};

#endif

#define	CGMS_PERMITTED		0	/* Unlimited copy permitted	*/
#define	CGMS_RES		1	/* Reserved			*/
#define	CGMS_ONE_COPY		2	/* One copy permitted		*/
#define	CGMS_NO_COPY		3	/* No copy permitted		*/

struct dvd_structure_0D {
	Uchar	data_len[2];		/* Data len without this info	*/
	Uchar	res23[2];		/* Reserved			*/
	Uchar	last_rma_sector[2];	/* Last recorded RMA sector #	*/
	Uchar	rmd_bytes[1];		/* Content of Record man area	*/
};

struct dvd_structure_0E {
	Uchar	data_len[2];		/* Data len without this info	*/
	Uchar	res23[2];		/* Reserved			*/
	Uchar	field_id;		/* Field ID (1)			*/
	Uchar	application_code;	/* Disc Application code	*/
	Uchar	phys_data;		/* Disc Phisical Data		*/
	Uchar	last_recordable_addr[3]; /* Last addr of recordable area */
	Uchar	res_a[2];		/* Reserved			*/
	Uchar	field_id_2;		/* Field ID (2)			*/
	Uchar	ind_wr_power;		/* Recommended writing power	*/
	Uchar	ind_wavelength;		/* Wavelength for ind_wr_power	*/
	Uchar	opt_wr_strategy[4];	/* Optimum write Strategy	*/
	Uchar	res_b[1];		/* Reserved			*/
	Uchar	field_id_3;		/* Field ID (3)			*/
	Uchar	man_id[6];		/* Manufacturer ID		*/
	Uchar	res_m1;			/* Reserved			*/
	Uchar	field_id_4;		/* Field ID (4)			*/
	Uchar	man_id2[6];		/* Manufacturer ID		*/
	Uchar	res_m2;			/* Reserved			*/
};

struct dvd_structure_0F {
	Uchar	data_len[2];		/* Data len without this info	*/
	Uchar	res23[2];		/* Reserved			*/
	Uchar	res45[2];		/* Reserved			*/
	Uchar	random[2];		/* Random number		*/
	Uchar	year[4];		/* Year (ascii)			*/
	Uchar	month[2];		/* Month (ascii)		*/
	Uchar	day[2];			/* Day (ascii)			*/
	Uchar	hour[2];		/* Hour (ascii)			*/
	Uchar	minute[2];		/* Minute (ascii)		*/
	Uchar	second[2];		/* Second (ascii)		*/
};

struct dvd_structure_0F_w {
	Uchar	data_len[2];		/* Data len without this info	*/
	Uchar	res23[2];		/* Reserved			*/
	Uchar	res45[2];		/* Reserved			*/
	Uchar	year[4];		/* Year (ascii)			*/
	Uchar	month[2];		/* Month (ascii)		*/
	Uchar	day[2];			/* Day (ascii)			*/
	Uchar	hour[2];		/* Hour (ascii)			*/
	Uchar	minute[2];		/* Minute (ascii)		*/
	Uchar	second[2];		/* Second (ascii)		*/
};

struct dvd_structure_20 {
	Uchar	data_len[2];		/* Data len without this info	*/
	Uchar	res23[2];		/* Reserved			*/
	Uchar	res47[4];		/* Reserved			*/
	Uchar	l0_area_cap[4];		/* Layer 0 area capacity	*/
};

struct dvd_structure_22 {
	Uchar	data_len[2];		/* Data len without this info	*/
	Uchar	res23[2];		/* Reserved			*/
	Uchar	res47[4];		/* Reserved			*/
	Uchar	jump_interval_size[4];	/* Jump interval size		*/
};

struct dvd_structure_23 {
	Uchar	data_len[2];		/* Data len without this info	*/
	Uchar	res23[2];		/* Reserved			*/
	Uchar	res47[4];		/* Reserved			*/
	Uchar	jump_lba[4];		/* Jump logical block address	*/
};

struct mmc_cue {
	Uchar	cs_ctladr;		/* CTL/ADR for this track	*/
	Uchar	cs_tno;			/* This track number		*/
	Uchar	cs_index;		/* Index within this track	*/
	Uchar	cs_dataform;		/* Data form 			*/
	Uchar	cs_scms;		/* Serial copy management	*/
	Uchar	cs_min;			/* Absolute time minutes	*/
	Uchar	cs_sec;			/* Absolute time seconds	*/
	Uchar	cs_frame;		/* Absolute time frames		*/
};

struct mmc_performance_header {
	Uchar	p_datalen[4];		/* Performance Data length	*/
#if defined(_BIT_FIELDS_LTOH)	/* Intel bitorder */
	Ucbit	p_exept		:1;	/* Nominal vs. Exept. conditions*/
	Ucbit	p_write		:1;	/* Write vs. Read performance	*/
	Ucbit	p_res_4		:6;	/* Reserved bits...		*/
#else				/* Motorola bitorder */
	Ucbit	p_res_4		:6;	/* Reserved bits...		*/
	Ucbit	p_write		:1;	/* Write vs. Read performance	*/
	Ucbit	p_exept		:1;	/* Nominal vs. Exept. conditions*/
#endif
	Uchar	p_res[3];		/* Reserved bytes		*/
};


struct mmc_performance {		/* Type == 00 (nominal)		*/
	Uchar	start_lba[4];		/* Starting LBA			*/
	Uchar	start_perf[4];		/* Start Performance		*/
	Uchar	end_lba[4];		/* Ending LBA			*/
	Uchar	end_perf[4];		/* Ending Performance		*/
};

struct mmc_exceptions {			/* Type == 00 (exceptions)	*/
	Uchar	lba[4];			/* LBA				*/
	Uchar	time[2];		/* Time				*/
};

struct mmc_write_speed {		/* Type == 00 (write speed)	*/
#if defined(_BIT_FIELDS_LTOH)	/* Intel bitorder */
	Ucbit	p_mrw		:1;	/* Suitable for mixed read/write*/
	Ucbit	p_exact		:1;	/* Speed count for whole media	*/
	Ucbit	p_rdd		:1;	/* Media rotational control	*/
	Ucbit	p_wrc		:2;	/* Write rotational control	*/
	Ucbit	p_res		:3;	/* Reserved bits...		*/
#else				/* Motorola bitorder */
	Ucbit	p_res		:3;	/* Reserved bits...		*/
	Ucbit	p_wrc		:2;	/* Write rotational control	*/
	Ucbit	p_rdd		:1;	/* Media rotational control	*/
	Ucbit	p_exact		:1;	/* Speed count for whole media	*/
	Ucbit	p_mrw		:1;	/* Suitable for mixed read/write*/
#endif
	Uchar	res[3];			/* Reserved Bytes		*/
	Uchar	end_lba[4];		/* Ending LBA			*/
	Uchar	read_speed[4];		/* Read Speed			*/
	Uchar	write_speed[4];		/* Write Speed			*/
};

#define	WRC_DEF_RC	0		/* Media default rotational control */
#define	WRC_CAV		1		/* CAV				    */


struct mmc_streaming {			/* Performance for set streaming*/
#if defined(_BIT_FIELDS_LTOH)	/* Intel bitorder */
	Ucbit	p_ra		:1;	/* Random Acess			*/
	Ucbit	p_exact		:1;	/* Set values exactly		*/
	Ucbit	p_rdd		:1;	/* Restore unit defaults	*/
	Ucbit	p_wrc		:2;	/* Write rotational control	*/
	Ucbit	p_res		:3;	/* Reserved bits...		*/
#else				/* Motorola bitorder */
	Ucbit	p_res		:3;	/* Reserved bits...		*/
	Ucbit	p_wrc		:2;	/* Write rotational control	*/
	Ucbit	p_rdd		:1;	/* Restore unit defaults	*/
	Ucbit	p_exact		:1;	/* Set values exactly		*/
	Ucbit	p_ra		:1;	/* Random Acess			*/
#endif
	Uchar	res[3];			/* Reserved Bytes		*/
	Uchar	start_lba[4];		/* Starting LBA			*/
	Uchar	end_lba[4];		/* Ending LBA			*/
	Uchar	read_size[4];		/* Read Size			*/
	Uchar	read_time[4];		/* Read Time			*/
	Uchar	write_size[4];		/* Write Size			*/
	Uchar	write_time[4];		/* Write Time			*/
};

#endif	/* _SCSIMMC_H */
