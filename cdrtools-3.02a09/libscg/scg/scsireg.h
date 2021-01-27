/* @(#)scsireg.h	1.35 12/03/16 Copyright 1987-2011 J. Schilling */
/*
 *	usefull definitions for dealing with CCS SCSI - devices
 *
 *	Copyright (c) 1987-2012 J. Schilling
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

#ifndef	_SCG_SCSIREG_H
#define	_SCG_SCSIREG_H

#include <schily/utypes.h>
#include <schily/btorder.h>

#ifdef	__cplusplus
extern "C" {
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct	scsi_inquiry {
	Ucbit	type		: 5;	/*  0 */
	Ucbit	qualifier	: 3;	/*  0 */

	Ucbit	type_modifier	: 7;	/*  1 */
	Ucbit	removable	: 1;	/*  1 */

	Ucbit	ansi_version	: 3;	/*  2 */
	Ucbit	ecma_version	: 3;	/*  2 */
	Ucbit	iso_version	: 2;	/*  2 */

	Ucbit	data_format	: 4;	/*  3 */
	Ucbit	res3_54		: 2;	/*  3 */
	Ucbit	termiop		: 1;	/*  3 */
	Ucbit	aenc		: 1;	/*  3 */

	Ucbit	add_len		: 8;	/*  4 */
	Ucbit	sense_len	: 8;	/*  5 */ /* only Emulex ??? */
	Ucbit	res2		: 8;	/*  6 */

	Ucbit	softreset	: 1;	/*  7 */
	Ucbit	cmdque		: 1;
	Ucbit	res7_2		: 1;
	Ucbit	linked		: 1;
	Ucbit	sync		: 1;
	Ucbit	wbus16		: 1;
	Ucbit	wbus32		: 1;
	Ucbit	reladr		: 1;	/*  7 */

	union {

		struct {
		char	vendor_info[8];		/*  8 */
		char	prod_ident[16];		/* 16 */
		char	prod_revision[4];	/* 32 */
#ifdef	comment
		char	vendor_uniq[20];	/* 36 */
		char	reserved[40];		/* 56 */
#endif
		} vi;
		char	vi_space[8+16+4];
	} vu;
};					/* 96 */

#else					/* Motorola byteorder */

struct	scsi_inquiry {
	Ucbit	qualifier	: 3;	/*  0 */
	Ucbit	type		: 5;	/*  0 */

	Ucbit	removable	: 1;	/*  1 */
	Ucbit	type_modifier	: 7;	/*  1 */

	Ucbit	iso_version	: 2;	/*  2 */
	Ucbit	ecma_version	: 3;
	Ucbit	ansi_version	: 3;	/*  2 */

	Ucbit	aenc		: 1;	/*  3 */
	Ucbit	termiop		: 1;
	Ucbit	res3_54		: 2;
	Ucbit	data_format	: 4;	/*  3 */

	Ucbit	add_len		: 8;	/*  4 */
	Ucbit	sense_len	: 8;	/*  5 */ /* only Emulex ??? */
	Ucbit	res2		: 8;	/*  6 */
	Ucbit	reladr		: 1;	/*  7 */
	Ucbit	wbus32		: 1;
	Ucbit	wbus16		: 1;
	Ucbit	sync		: 1;
	Ucbit	linked		: 1;
	Ucbit	res7_2		: 1;
	Ucbit	cmdque		: 1;
	Ucbit	softreset	: 1;

	union {

		struct {
		char	vendor_info[8];		/*  8 */
		char	prod_ident[16];		/* 16 */
		char	prod_revision[4];	/* 32 */
#ifdef	comment
		char	vendor_uniq[20];	/* 36 */
		char	reserved[40];		/* 56 */
#endif
		} vi;
		char	vi_space[8+16+4];
	} vu;
};					/* 96 */
#endif

#ifdef	__SCG_COMPAT__
#define	info		inq_vendor_info
#define	ident		inq_prod_ident
#define	revision	inq_prod_revision
#endif

#define	inq_vendor_info		vu.vi.vendor_info
#define	inq_prod_ident		vu.vi.prod_ident
#define	inq_prod_revision	vu.vi.prod_revision

#define	inq_info_space		vu.vi_space

/* Peripheral Device Qualifier */

#define	INQ_DEV_PRESENT	0x00		/* Physical device present */
#define	INQ_DEV_NOTPR	0x01		/* Physical device not present */
#define	INQ_DEV_RES	0x02		/* Reserved */
#define	INQ_DEV_NOTSUP	0x03		/* Logical unit not supported */

/* Peripheral Device Type */

#define	INQ_DASD	0x00		/* Direct-access device (disk) */
#define	INQ_SEQD	0x01		/* Sequential-access device (tape) */
#define	INQ_PRTD	0x02 		/* Printer device */
#define	INQ_PROCD	0x03 		/* Processor device */
#define	INQ_OPTD	0x04		/* Write once device (optical disk) */
#define	INQ_WORM	0x04		/* Write once device (optical disk) */
#define	INQ_ROMD	0x05		/* CD-ROM device */
#define	INQ_SCAN	0x06		/* Scanner device */
#define	INQ_OMEM	0x07		/* Optical Memory device */
#define	INQ_JUKE	0x08		/* Medium Changer device (jukebox) */
#define	INQ_COMM	0x09		/* Communications device */
#define	INQ_IT8_1	0x0A		/* IT8 */
#define	INQ_IT8_2	0x0B		/* IT8 */
#define	INQ_STARR	0x0C		/* Storage array device */
#define	INQ_ENCL	0x0D		/* Enclosure services device */
#define	INQ_SDAD	0x0E		/* Simplyfied direct-access device */
#define	INQ_OCRW	0x0F		/* Optical card reader/writer device */
#define	INQ_BRIDGE	0x10		/* Bridging expander device */
#define	INQ_OSD		0x11		/* Object based storage device */
#define	INQ_ADC		0x12		/* Automation/Drive interface */
#define	INQ_WELLKNOWN	0x1E		/* Well known logical unit */
#define	INQ_NODEV	0x1F		/* Unknown or no device */
#define	INQ_NOTPR	0x1F		/* Logical unit not present (SCSI-1) */

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_header {
	Ucbit	sense_data_len	: 8;
	Uchar	medium_type;
	Ucbit	res2		: 4;
	Ucbit	cache		: 1;
	Ucbit	res		: 2;
	Ucbit	write_prot	: 1;
	Uchar	blockdesc_len;
};

#else					/* Motorola byteorder */

struct scsi_mode_header {
	Ucbit	sense_data_len	: 8;
	Uchar	medium_type;
	Ucbit	write_prot	: 1;
	Ucbit	res		: 2;
	Ucbit	cache		: 1;
	Ucbit	res2		: 4;
	Uchar	blockdesc_len;
};
#endif

struct scsi_modesel_header {
	Ucbit	sense_data_len	: 8;
	Uchar	medium_type;
	Ucbit	res2		: 8;
	Uchar	blockdesc_len;
};

struct scsi_mode_blockdesc {
	Uchar	density;
	Uchar	nlblock[3];
	Ucbit	res		: 8;
	Uchar	lblen[3];
};

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct acb_mode_data {
	Uchar	listformat;
	Uchar	ncyl[2];
	Uchar	nhead;
	Uchar	start_red_wcurrent[2];
	Uchar	start_precomp[2];
	Uchar	landing_zone;
	Uchar	step_rate;
	Ucbit			: 2;
	Ucbit	hard_sec	: 1;
	Ucbit	fixed_media	: 1;
	Ucbit			: 4;
	Uchar	sect_per_trk;
};

#else					/* Motorola byteorder */

struct acb_mode_data {
	Uchar	listformat;
	Uchar	ncyl[2];
	Uchar	nhead;
	Uchar	start_red_wcurrent[2];
	Uchar	start_precomp[2];
	Uchar	landing_zone;
	Uchar	step_rate;
	Ucbit			: 4;
	Ucbit	fixed_media	: 1;
	Ucbit	hard_sec	: 1;
	Ucbit			: 2;
	Uchar	sect_per_trk;
};
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_header {
	Ucbit	p_code		: 6;
	Ucbit	res		: 1;
	Ucbit	parsave		: 1;
	Uchar	p_len;
};

/*
 * This is a hack that allows mode pages without
 * any further bitfileds to be defined bitorder independent.
 */
#define	MP_P_CODE			\
	Ucbit	p_code		: 6;	\
	Ucbit	p_res		: 1;	\
	Ucbit	parsave		: 1

#else					/* Motorola byteorder */

struct scsi_mode_page_header {
	Ucbit	parsave		: 1;
	Ucbit	res		: 1;
	Ucbit	p_code		: 6;
	Uchar	p_len;
};

/*
 * This is a hack that allows mode pages without
 * any further bitfileds to be defined bitorder independent.
 */
#define	MP_P_CODE			\
	Ucbit	parsave		: 1;	\
	Ucbit	p_res		: 1;	\
	Ucbit	p_code		: 6

#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_01 {		/* Error recovery Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x0A = 12 Bytes */
	Ucbit	disa_correction	: 1;	/* Byte 2 */
	Ucbit	term_on_rec_err	: 1;
	Ucbit	report_rec_err	: 1;
	Ucbit	en_early_corr	: 1;
	Ucbit	read_continuous	: 1;
	Ucbit	tranfer_block	: 1;
	Ucbit	en_auto_reall_r	: 1;
	Ucbit	en_auto_reall_w	: 1;	/* Byte 2 */
	Uchar	rd_retry_count;		/* Byte 3 */
	Uchar	correction_span;
	char	head_offset_count;
	char	data_strobe_offset;
	Uchar	res;
	Uchar	wr_retry_count;
	Uchar	res_tape[2];
	Uchar	recov_timelim[2];
};

#else					/* Motorola byteorder */

struct scsi_mode_page_01 {		/* Error recovery Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x0A = 12 Bytes */
	Ucbit	en_auto_reall_w	: 1;	/* Byte 2 */
	Ucbit	en_auto_reall_r	: 1;
	Ucbit	tranfer_block	: 1;
	Ucbit	read_continuous	: 1;
	Ucbit	en_early_corr	: 1;
	Ucbit	report_rec_err	: 1;
	Ucbit	term_on_rec_err	: 1;
	Ucbit	disa_correction	: 1;	/* Byte 2 */
	Uchar	rd_retry_count;		/* Byte 3 */
	Uchar	correction_span;
	char	head_offset_count;
	char	data_strobe_offset;
	Uchar	res;
	Uchar	wr_retry_count;
	Uchar	res_tape[2];
	Uchar	recov_timelim[2];
};
#endif


#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_02 {		/* Device dis/re connect Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x0E = 16 Bytes */
	Uchar	buf_full_ratio;
	Uchar	buf_empt_ratio;
	Uchar	bus_inact_limit[2];
	Uchar	disc_time_limit[2];
	Uchar	conn_time_limit[2];
	Uchar	max_burst_size[2];	/* Start SCSI-2 */
	Ucbit	data_tr_dis_ctl	: 2;
	Ucbit			: 6;
	Uchar	res[3];
};

#else					/* Motorola byteorder */

struct scsi_mode_page_02 {		/* Device dis/re connect Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x0E = 16 Bytes */
	Uchar	buf_full_ratio;
	Uchar	buf_empt_ratio;
	Uchar	bus_inact_limit[2];
	Uchar	disc_time_limit[2];
	Uchar	conn_time_limit[2];
	Uchar	max_burst_size[2];	/* Start SCSI-2 */
	Ucbit			: 6;
	Ucbit	data_tr_dis_ctl	: 2;
	Uchar	res[3];
};
#endif

#define	DTDC_DATADONE	0x01		/*
					 * Target may not disconnect once
					 * data transfer is started until
					 * all data successfully transferred.
					 */

#define	DTDC_CMDDONE	0x03		/*
					 * Target may not disconnect once
					 * data transfer is started until
					 * command completed.
					 */


#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_03 {		/* Direct access format Paramters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x16 = 24 Bytes */
	Uchar	trk_per_zone[2];
	Uchar	alt_sec_per_zone[2];
	Uchar	alt_trk_per_zone[2];
	Uchar	alt_trk_per_vol[2];
	Uchar	sect_per_trk[2];
	Uchar	bytes_per_phys_sect[2];
	Uchar	interleave[2];
	Uchar	trk_skew[2];
	Uchar	cyl_skew[2];
	Ucbit			: 3;
	Ucbit	inhibit_save	: 1;
	Ucbit	fmt_by_surface	: 1;
	Ucbit	removable	: 1;
	Ucbit	hard_sec	: 1;
	Ucbit	soft_sec	: 1;
	Uchar	res[3];
};

#else					/* Motorola byteorder */

struct scsi_mode_page_03 {		/* Direct access format Paramters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x16 = 24 Bytes */
	Uchar	trk_per_zone[2];
	Uchar	alt_sec_per_zone[2];
	Uchar	alt_trk_per_zone[2];
	Uchar	alt_trk_per_vol[2];
	Uchar	sect_per_trk[2];
	Uchar	bytes_per_phys_sect[2];
	Uchar	interleave[2];
	Uchar	trk_skew[2];
	Uchar	cyl_skew[2];
	Ucbit	soft_sec	: 1;
	Ucbit	hard_sec	: 1;
	Ucbit	removable	: 1;
	Ucbit	fmt_by_surface	: 1;
	Ucbit	inhibit_save	: 1;
	Ucbit			: 3;
	Uchar	res[3];
};
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_04 {		/* Rigid disk Geometry Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x16 = 24 Bytes */
	Uchar	ncyl[3];
	Uchar	nhead;
	Uchar	start_precomp[3];
	Uchar	start_red_wcurrent[3];
	Uchar	step_rate[2];
	Uchar	landing_zone[3];
	Ucbit	rot_pos_locking	: 2;	/* Start SCSI-2 */
	Ucbit			: 6;	/* Start SCSI-2 */
	Uchar	rotational_off;
	Uchar	res1;
	Uchar	rotation_rate[2];
	Uchar	res2[2];
};

#else					/* Motorola byteorder */

struct scsi_mode_page_04 {		/* Rigid disk Geometry Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x16 = 24 Bytes */
	Uchar	ncyl[3];
	Uchar	nhead;
	Uchar	start_precomp[3];
	Uchar	start_red_wcurrent[3];
	Uchar	step_rate[2];
	Uchar	landing_zone[3];
	Ucbit			: 6;	/* Start SCSI-2 */
	Ucbit	rot_pos_locking	: 2;	/* Start SCSI-2 */
	Uchar	rotational_off;
	Uchar	res1;
	Uchar	rotation_rate[2];
	Uchar	res2[2];
};
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_05 {		/* Flexible disk Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x1E = 32 Bytes */
	Uchar	transfer_rate[2];
	Uchar	nhead;
	Uchar	sect_per_trk;
	Uchar	bytes_per_phys_sect[2];
	Uchar	ncyl[2];
	Uchar	start_precomp[2];
	Uchar	start_red_wcurrent[2];
	Uchar	step_rate[2];
	Uchar	step_pulse_width;
	Uchar	head_settle_delay[2];
	Uchar	motor_on_delay;
	Uchar	motor_off_delay;
	Ucbit	spc		: 4;
	Ucbit			: 4;
	Ucbit			: 5;
	Ucbit	mo		: 1;
	Ucbit	ssn		: 1;
	Ucbit	trdy		: 1;
	Uchar	write_compensation;
	Uchar	head_load_delay;
	Uchar	head_unload_delay;
	Ucbit	pin_2_use	: 4;
	Ucbit	pin_34_use	: 4;
	Ucbit	pin_1_use	: 4;
	Ucbit	pin_4_use	: 4;
	Uchar	rotation_rate[2];
	Uchar	res[2];
};

#else					/* Motorola byteorder */

struct scsi_mode_page_05 {		/* Flexible disk Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x1E = 32 Bytes */
	Uchar	transfer_rate[2];
	Uchar	nhead;
	Uchar	sect_per_trk;
	Uchar	bytes_per_phys_sect[2];
	Uchar	ncyl[2];
	Uchar	start_precomp[2];
	Uchar	start_red_wcurrent[2];
	Uchar	step_rate[2];
	Uchar	step_pulse_width;
	Uchar	head_settle_delay[2];
	Uchar	motor_on_delay;
	Uchar	motor_off_delay;
	Ucbit	trdy		: 1;
	Ucbit	ssn		: 1;
	Ucbit	mo		: 1;
	Ucbit			: 5;
	Ucbit			: 4;
	Ucbit	spc		: 4;
	Uchar	write_compensation;
	Uchar	head_load_delay;
	Uchar	head_unload_delay;
	Ucbit	pin_34_use	: 4;
	Ucbit	pin_2_use	: 4;
	Ucbit	pin_4_use	: 4;
	Ucbit	pin_1_use	: 4;
	Uchar	rotation_rate[2];
	Uchar	res[2];
};
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_07 {		/* Verify Error recovery */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x0A = 12 Bytes */
	Ucbit	disa_correction	: 1;	/* Byte 2 */
	Ucbit	term_on_rec_err	: 1;
	Ucbit	report_rec_err	: 1;
	Ucbit	en_early_corr	: 1;
	Ucbit	res		: 4;	/* Byte 2 */
	Uchar	ve_retry_count;		/* Byte 3 */
	Uchar	ve_correction_span;
	char	res2[5];		/* Byte 5 */
	Uchar	ve_recov_timelim[2];	/* Byte 10 */
};

#else					/* Motorola byteorder */

struct scsi_mode_page_07 {		/* Verify Error recovery */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x0A = 12 Bytes */
	Ucbit	res		: 4;	/* Byte 2 */
	Ucbit	en_early_corr	: 1;
	Ucbit	report_rec_err	: 1;
	Ucbit	term_on_rec_err	: 1;
	Ucbit	disa_correction	: 1;	/* Byte 2 */
	Uchar	ve_retry_count;		/* Byte 3 */
	Uchar	ve_correction_span;
	char	res2[5];		/* Byte 5 */
	Uchar	ve_recov_timelim[2];	/* Byte 10 */
};
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_08 {		/* Caching Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x0A = 12 Bytes */
	Ucbit	disa_rd_cache	: 1;	/* Byte 2 */
	Ucbit	muliple_fact	: 1;
	Ucbit	en_wt_cache	: 1;
	Ucbit	res		: 5;	/* Byte 2 */
	Ucbit	wt_ret_pri	: 4;	/* Byte 3 */
	Ucbit	demand_rd_ret_pri: 4;	/* Byte 3 */
	Uchar	disa_pref_tr_len[2];	/* Byte 4 */
	Uchar	min_pref[2];		/* Byte 6 */
	Uchar	max_pref[2];		/* Byte 8 */
	Uchar	max_pref_ceiling[2];	/* Byte 10 */
};

#else					/* Motorola byteorder */

struct scsi_mode_page_08 {		/* Caching Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x0A = 12 Bytes */
	Ucbit	res		: 5;	/* Byte 2 */
	Ucbit	en_wt_cache	: 1;
	Ucbit	muliple_fact	: 1;
	Ucbit	disa_rd_cache	: 1;	/* Byte 2 */
	Ucbit	demand_rd_ret_pri: 4;	/* Byte 3 */
	Ucbit	wt_ret_pri	: 4;
	Uchar	disa_pref_tr_len[2];	/* Byte 4 */
	Uchar	min_pref[2];		/* Byte 6 */
	Uchar	max_pref[2];		/* Byte 8 */
	Uchar	max_pref_ceiling[2];	/* Byte 10 */
};
#endif

struct scsi_mode_page_09 {		/* Peripheral device Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* >= 0x06 = 8 Bytes */
	Uchar	interface_id[2];	/* Byte 2 */
	Uchar	res[4];			/* Byte 4 */
	Uchar	vendor_specific[1];	/* Byte 8 */
};

#define	PDEV_SCSI	0x0000		/* scsi interface */
#define	PDEV_SMD	0x0001		/* SMD interface */
#define	PDEV_ESDI	0x0002		/* ESDI interface */
#define	PDEV_IPI2	0x0003		/* IPI-2 interface */
#define	PDEV_IPI3	0x0004		/* IPI-3 interface */

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_0A {		/* Common device Control Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x06 = 8 Bytes */
	Ucbit	rep_log_exeption: 1;	/* Byte 2 */
	Ucbit	res		: 7;	/* Byte 2 */
	Ucbit	dis_queuing	: 1;	/* Byte 3 */
	Ucbit	queuing_err_man	: 1;
	Ucbit	res2		: 2;
	Ucbit	queue_alg_mod	: 4;	/* Byte 3 */
	Ucbit	EAENP		: 1;	/* Byte 4 */
	Ucbit	UAENP		: 1;
	Ucbit	RAENP		: 1;
	Ucbit	res3		: 4;
	Ucbit	en_ext_cont_all	: 1;	/* Byte 4 */
	Ucbit	res4		: 8;
	Uchar	ready_aen_hold_per[2];	/* Byte 6 */
};

#else					/* Motorola byteorder */

struct scsi_mode_page_0A {		/* Common device Control Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x06 = 8 Bytes */
	Ucbit	res		: 7;	/* Byte 2 */
	Ucbit	rep_log_exeption: 1;	/* Byte 2 */
	Ucbit	queue_alg_mod	: 4;	/* Byte 3 */
	Ucbit	res2		: 2;
	Ucbit	queuing_err_man	: 1;
	Ucbit	dis_queuing	: 1;	/* Byte 3 */
	Ucbit	en_ext_cont_all	: 1;	/* Byte 4 */
	Ucbit	res3		: 4;
	Ucbit	RAENP		: 1;
	Ucbit	UAENP		: 1;
	Ucbit	EAENP		: 1;	/* Byte 4 */
	Ucbit	res4		: 8;
	Uchar	ready_aen_hold_per[2];	/* Byte 6 */
};
#endif

#define	CTRL_QMOD_RESTRICT	0x0
#define	CTRL_QMOD_UNRESTRICT	0x1


struct scsi_mode_page_0B {		/* Medium Types Supported Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x06 = 8 Bytes */
	Uchar	res[2];			/* Byte 2 */
	Uchar	medium_one_supp;	/* Byte 4 */
	Uchar	medium_two_supp;	/* Byte 5 */
	Uchar	medium_three_supp;	/* Byte 6 */
	Uchar	medium_four_supp;	/* Byte 7 */
};

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_0C {		/* Notch & Partition Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x16 = 24 Bytes */
	Ucbit	res		: 6;	/* Byte 2 */
	Ucbit	logical_notch	: 1;
	Ucbit	notched_drive	: 1;	/* Byte 2 */
	Uchar	res2;			/* Byte 3 */
	Uchar	max_notches[2];		/* Byte 4  */
	Uchar	active_notch[2];	/* Byte 6  */
	Uchar	starting_boundary[4];	/* Byte 8  */
	Uchar	ending_boundary[4];	/* Byte 12 */
	Uchar	pages_notched[8];	/* Byte 16 */
};

#else					/* Motorola byteorder */

struct scsi_mode_page_0C {		/* Notch & Partition Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x16 = 24 Bytes */
	Ucbit	notched_drive	: 1;	/* Byte 2 */
	Ucbit	logical_notch	: 1;
	Ucbit	res		: 6;	/* Byte 2 */
	Uchar	res2;			/* Byte 3 */
	Uchar	max_notches[2];		/* Byte 4  */
	Uchar	active_notch[2];	/* Byte 6  */
	Uchar	starting_boundary[4];	/* Byte 8  */
	Uchar	ending_boundary[4];	/* Byte 12 */
	Uchar	pages_notched[8];	/* Byte 16 */
};
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_mode_page_0D {		/* CD-ROM Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x06 = 8 Bytes */
	Uchar	res;			/* Byte 2 */
	Ucbit	inact_timer_mult: 4;	/* Byte 3 */
	Ucbit	res2		: 4;	/* Byte 3 */
	Uchar	s_un_per_m_un[2];	/* Byte 4  */
	Uchar	f_un_per_s_un[2];	/* Byte 6  */
};

#else					/* Motorola byteorder */

struct scsi_mode_page_0D {		/* CD-ROM Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x06 = 8 Bytes */
	Uchar	res;			/* Byte 2 */
	Ucbit	res2		: 4;	/* Byte 3 */
	Ucbit	inact_timer_mult: 4;	/* Byte 3 */
	Uchar	s_un_per_m_un[2];	/* Byte 4  */
	Uchar	f_un_per_s_un[2];	/* Byte 6  */
};
#endif

struct sony_mode_page_20 {		/* Sony Format Mode Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x0A = 12 Bytes */
	Uchar	format_mode;
	Uchar	format_type;
#define	num_bands	user_band_size	/* Gilt bei Type 1 */
	Uchar	user_band_size[4];	/* Gilt bei Type 0 */
	Uchar	spare_band_size[2];
	Uchar	res[2];
};

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct toshiba_mode_page_20 {		/* Toshiba Speed Control Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x01 = 3 Bytes */
	Ucbit	speed		: 1;
	Ucbit	res		: 7;
};

#else					/* Motorola byteorder */

struct toshiba_mode_page_20 {		/* Toshiba Speed Control Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x01 = 3 Bytes */
	Ucbit	res		: 7;
	Ucbit	speed		: 1;
};
#endif

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct ccs_mode_page_38 {		/* CCS Caching Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x0E = 14 Bytes */

	Ucbit	cache_table_size: 4;	/* Byte 3 */
	Ucbit	cache_en	: 1;
	Ucbit	res2		: 1;
	Ucbit	wr_index_en	: 1;
	Ucbit	res		: 1;	/* Byte 3 */
	Uchar	threshold;		/* Byte 4 Prefetch threshold */
	Uchar	max_prefetch;		/* Byte 5 Max. prefetch */
	Uchar	max_multiplier;		/* Byte 6 Max. prefetch multiplier */
	Uchar	min_prefetch;		/* Byte 7 Min. prefetch */
	Uchar	min_multiplier;		/* Byte 8 Min. prefetch multiplier */
	Uchar	res3[8];		/* Byte 9 */
};

#else					/* Motorola byteorder */

struct ccs_mode_page_38 {		/* CCS Caching Parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x0E = 14 Bytes */

	Ucbit	res		: 1;	/* Byte 3 */
	Ucbit	wr_index_en	: 1;
	Ucbit	res2		: 1;
	Ucbit	cache_en	: 1;
	Ucbit	cache_table_size: 4;	/* Byte 3 */
	Uchar	threshold;		/* Byte 4 Prefetch threshold */
	Uchar	max_prefetch;		/* Byte 5 Max. prefetch */
	Uchar	max_multiplier;		/* Byte 6 Max. prefetch multiplier */
	Uchar	min_prefetch;		/* Byte 7 Min. prefetch */
	Uchar	min_multiplier;		/* Byte 8 Min. prefetch multiplier */
	Uchar	res3[8];		/* Byte 9 */
};
#endif

#if defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct cd_mode_page_05 {		/* write parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x32 = 50 Bytes */
	Ucbit	write_type	: 4;	/* Session write type (PACKET/TAO...)*/
	Ucbit	test_write	: 1;	/* Do not actually write data	    */
	Ucbit	LS_V		: 1;	/* Link size valid		    */
	Ucbit	BUFE		: 1;	/* Enable Bufunderrun free rec.	    */
	Ucbit	res_2_7		: 1;
	Ucbit	track_mode	: 4;	/* Track mode (Q-sub control nibble) */
	Ucbit	copy		: 1;	/* 1st higher gen of copy prot track ~*/
	Ucbit	fp		: 1;	/* Fixed packed (if in packet mode) */
	Ucbit	multi_session	: 2;	/* Multi session write type	    */
	Ucbit	dbtype		: 4;	/* Data block type		    */
	Ucbit	res_4		: 4;	/* Reserved			    */
	Uchar	link_size;		/* Link Size (default is 7)	    */
	Uchar	res_6;			/* Reserved			    */
	Ucbit	host_appl_code	: 6;	/* Host application code of disk    */
	Ucbit	res_7		: 2;	/* Reserved			    */
	Uchar	session_format;		/* Session format (DA/CDI/XA)	    */
	Uchar	res_9;			/* Reserved			    */
	Uchar	packet_size[4];		/* # of user datablocks/fixed packet */
	Uchar	audio_pause_len[2];	/* # of blocks where index is zero  */
	Uchar	media_cat_number[16];	/* Media catalog Number (MCN)	    */
	Uchar	ISRC[14];		/* ISRC for this track		    */
	Uchar	sub_header[4];
	Uchar	vendor_uniq[4];
};

#else				/* Motorola byteorder */

struct cd_mode_page_05 {		/* write parameters */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x32 = 50 Bytes */
	Ucbit	res_2_7		: 1;
	Ucbit	BUFE		: 1;	/* Enable Bufunderrun free rec.	    */
	Ucbit	LS_V		: 1;	/* Link size valid		    */
	Ucbit	test_write	: 1;	/* Do not actually write data	    */
	Ucbit	write_type	: 4;	/* Session write type (PACKET/TAO...)*/
	Ucbit	multi_session	: 2;	/* Multi session write type	    */
	Ucbit	fp		: 1;	/* Fixed packed (if in packet mode) */
	Ucbit	copy		: 1;	/* 1st higher gen of copy prot track */
	Ucbit	track_mode	: 4;	/* Track mode (Q-sub control nibble) */
	Ucbit	res_4		: 4;	/* Reserved			    */
	Ucbit	dbtype		: 4;	/* Data block type		    */
	Uchar	link_size;		/* Link Size (default is 7)	    */
	Uchar	res_6;			/* Reserved			    */
	Ucbit	res_7		: 2;	/* Reserved			    */
	Ucbit	host_appl_code	: 6;	/* Host application code of disk    */
	Uchar	session_format;		/* Session format (DA/CDI/XA)	    */
	Uchar	res_9;			/* Reserved			    */
	Uchar	packet_size[4];		/* # of user datablocks/fixed packet */
	Uchar	audio_pause_len[2];	/* # of blocks where index is zero  */
	Uchar	media_cat_number[16];	/* Media catalog Number (MCN)	    */
	Uchar	ISRC[14];		/* ISRC for this track		    */
	Uchar	sub_header[4];
	Uchar	vendor_uniq[4];
};

#endif

#if defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct cd_wr_speed_performance {
	Uchar	res0;			/* Reserved			    */
	Ucbit	rot_ctl_sel	: 2;	/* Rotational control selected	    */
	Ucbit	res_1_27	: 6;	/* Reserved			    */
	Uchar	wr_speed_supp[2];	/* Supported write speed	    */
};

struct cd_mode_page_2A {		/* CD Cap / mech status */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x14 = 20 Bytes (MMC) */
					/* 0x18 = 24 Bytes (MMC-2) */
					/* 0x1C >= 28 Bytes (MMC-3) */
	Ucbit	cd_r_read	: 1;	/* Reads CD-R  media		    */
	Ucbit	cd_rw_read	: 1;	/* Reads CD-RW media		    */
	Ucbit	method2		: 1;	/* Reads fixed packet method2 media */
	Ucbit	dvd_rom_read	: 1;	/* Reads DVD ROM media		    */
	Ucbit	dvd_r_read	: 1;	/* Reads DVD-R media		    */
	Ucbit	dvd_ram_read	: 1;	/* Reads DVD-RAM media		    */
	Ucbit	res_2_67	: 2;	/* Reserved			    */
	Ucbit	cd_r_write	: 1;	/* Supports writing CD-R  media	    */
	Ucbit	cd_rw_write	: 1;	/* Supports writing CD-RW media	    */
	Ucbit	test_write	: 1;	/* Supports emulation write	    */
	Ucbit	res_3_3		: 1;	/* Reserved			    */
	Ucbit	dvd_r_write	: 1;	/* Supports writing DVD-R media	    */
	Ucbit	dvd_ram_write	: 1;	/* Supports writing DVD-RAM media   */
	Ucbit	res_3_67	: 2;	/* Reserved			    */
	Ucbit	audio_play	: 1;	/* Supports Audio play operation    */
	Ucbit	composite	: 1;	/* Deliveres composite A/V stream   */
	Ucbit	digital_port_2	: 1;	/* Supports digital output on port 2 */
	Ucbit	digital_port_1	: 1;	/* Supports digital output on port 1 */
	Ucbit	mode_2_form_1	: 1;	/* Reads Mode-2 form 1 media (XA)   */
	Ucbit	mode_2_form_2	: 1;	/* Reads Mode-2 form 2 media	    */
	Ucbit	multi_session	: 1;	/* Reads multi-session media	    */
	Ucbit	BUF		: 1;	/* Supports Buffer under. free rec. */
	Ucbit	cd_da_supported	: 1;	/* Reads audio data with READ CD cmd */
	Ucbit	cd_da_accurate	: 1;	/* READ CD data stream is accurate   */
	Ucbit	rw_supported	: 1;	/* Reads R-W sub channel information */
	Ucbit	rw_deint_corr	: 1;	/* Reads de-interleved R-W sub chan  */
	Ucbit	c2_pointers	: 1;	/* Supports C2 error pointers	    */
	Ucbit	ISRC		: 1;	/* Reads ISRC information	    */
	Ucbit	UPC		: 1;	/* Reads media catalog number (UPC) */
	Ucbit	read_bar_code	: 1;	/* Supports reading bar codes	    */
	Ucbit	lock		: 1;	/* PREVENT/ALLOW may lock media	    */
	Ucbit	lock_state	: 1;	/* Lock state 0=unlocked 1=locked   */
	Ucbit	prevent_jumper	: 1;	/* State of prev/allow jumper 0=pres */
	Ucbit	eject		: 1;	/* Ejects disc/cartr with STOP LoEj  */
	Ucbit	res_6_4		: 1;	/* Reserved			    */
	Ucbit	loading_type	: 3;	/* Loading mechanism type	    */
	Ucbit	sep_chan_vol	: 1;	/* Vol controls each channel separat */
	Ucbit	sep_chan_mute	: 1;	/* Mute controls each channel separat*/
	Ucbit	disk_present_rep: 1;	/* Changer supports disk present rep */
	Ucbit	sw_slot_sel	: 1;	/* Load empty slot in changer	    */
	Ucbit	side_change	: 1;	/* Side change capable		    */
	Ucbit	pw_in_lead_in	: 1;	/* Reads raw P-W sucode from lead in */
	Ucbit	res_7		: 2;	/* Reserved			    */
	Uchar	max_read_speed[2];	/* Max. read speed in KB/s	    */
	Uchar	num_vol_levels[2];	/* # of supported volume levels	    */
	Uchar	buffer_size[2];		/* Buffer size for the data in KB   */
	Uchar	cur_read_speed[2];	/* Current read speed in KB/s	    */
	Uchar	res_16;			/* Reserved			    */
	Ucbit	res_17_0	: 1;	/* Reserved			    */
	Ucbit	BCK		: 1;	/* Data valid on falling edge of BCK */
	Ucbit	RCK		: 1;	/* Set: HIGH high LRCK=left channel  */
	Ucbit	LSBF		: 1;	/* Set: LSB first Clear: MSB first  */
	Ucbit	length		: 2;	/* 0=32BCKs 1=16BCKs 2=24BCKs 3=24I2c*/
	Ucbit	res_17		: 2;	/* Reserved			    */
	Uchar	max_write_speed[2];	/* Max. write speed supported in KB/s*/
	Uchar	cur_write_speed[2];	/* Current write speed in KB/s	    */

					/* Byte 22 ... Only in MMC-2	    */
	Uchar	copy_man_rev[2];	/* Copy management revision supported*/
	Uchar	res_24;			/* Reserved			    */
	Uchar	res_25;			/* Reserved			    */

					/* Byte 26 ... Only in MMC-3	    */
	Uchar	res_26;			/* Reserved			    */
	Ucbit	res_27_27	: 6;	/* Reserved			    */
	Ucbit	rot_ctl_sel	: 2;	/* Rotational control selected	    */
	Uchar	v3_cur_write_speed[2];	/* Current write speed in KB/s	    */
	Uchar	num_wr_speed_des[2];	/* # of wr speed perf descr. tables */
	struct cd_wr_speed_performance
		wr_speed_des[1];	/* wr speed performance descriptor  */
					/* Actually more (num_wr_speed_des) */
};

#else				/* Motorola byteorder */

struct cd_wr_speed_performance {
	Uchar	res0;			/* Reserved			    */
	Ucbit	res_1_27	: 6;	/* Reserved			    */
	Ucbit	rot_ctl_sel	: 2;	/* Rotational control selected	    */
	Uchar	wr_speed_supp[2];	/* Supported write speed	    */
};

struct cd_mode_page_2A {		/* CD Cap / mech status */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x14 = 20 Bytes (MMC) */
					/* 0x18 = 24 Bytes (MMC-2) */
					/* 0x1C >= 28 Bytes (MMC-3) */
	Ucbit	res_2_67	: 2;	/* Reserved			    */
	Ucbit	dvd_ram_read	: 1;	/* Reads DVD-RAM media		    */
	Ucbit	dvd_r_read	: 1;	/* Reads DVD-R media		    */
	Ucbit	dvd_rom_read	: 1;	/* Reads DVD ROM media		    */
	Ucbit	method2		: 1;	/* Reads fixed packet method2 media */
	Ucbit	cd_rw_read	: 1;	/* Reads CD-RW media		    */
	Ucbit	cd_r_read	: 1;	/* Reads CD-R  media		    */
	Ucbit	res_3_67	: 2;	/* Reserved			    */
	Ucbit	dvd_ram_write	: 1;	/* Supports writing DVD-RAM media   */
	Ucbit	dvd_r_write	: 1;	/* Supports writing DVD-R media	    */
	Ucbit	res_3_3		: 1;	/* Reserved			    */
	Ucbit	test_write	: 1;	/* Supports emulation write	    */
	Ucbit	cd_rw_write	: 1;	/* Supports writing CD-RW media	    */
	Ucbit	cd_r_write	: 1;	/* Supports writing CD-R  media	    */
	Ucbit	BUF		: 1;	/* Supports Buffer under. free rec. */
	Ucbit	multi_session	: 1;	/* Reads multi-session media	    */
	Ucbit	mode_2_form_2	: 1;	/* Reads Mode-2 form 2 media	    */
	Ucbit	mode_2_form_1	: 1;	/* Reads Mode-2 form 1 media (XA)   */
	Ucbit	digital_port_1	: 1;	/* Supports digital output on port 1 */
	Ucbit	digital_port_2	: 1;	/* Supports digital output on port 2 */
	Ucbit	composite	: 1;	/* Deliveres composite A/V stream   */
	Ucbit	audio_play	: 1;	/* Supports Audio play operation    */
	Ucbit	read_bar_code	: 1;	/* Supports reading bar codes	    */
	Ucbit	UPC		: 1;	/* Reads media catalog number (UPC) */
	Ucbit	ISRC		: 1;	/* Reads ISRC information	    */
	Ucbit	c2_pointers	: 1;	/* Supports C2 error pointers	    */
	Ucbit	rw_deint_corr	: 1;	/* Reads de-interleved R-W sub chan */
	Ucbit	rw_supported	: 1;	/* Reads R-W sub channel information */
	Ucbit	cd_da_accurate	: 1;	/* READ CD data stream is accurate   */
	Ucbit	cd_da_supported	: 1;	/* Reads audio data with READ CD cmd */
	Ucbit	loading_type	: 3;	/* Loading mechanism type	    */
	Ucbit	res_6_4		: 1;	/* Reserved			    */
	Ucbit	eject		: 1;	/* Ejects disc/cartr with STOP LoEj */
	Ucbit	prevent_jumper	: 1;	/* State of prev/allow jumper 0=pres */
	Ucbit	lock_state	: 1;	/* Lock state 0=unlocked 1=locked   */
	Ucbit	lock		: 1;	/* PREVENT/ALLOW may lock media	    */
	Ucbit	res_7		: 2;	/* Reserved			    */
	Ucbit	pw_in_lead_in	: 1;	/* Reads raw P-W sucode from lead in */
	Ucbit	side_change	: 1;	/* Side change capable		    */
	Ucbit	sw_slot_sel	: 1;	/* Load empty slot in changer	    */
	Ucbit	disk_present_rep: 1;	/* Changer supports disk present rep */
	Ucbit	sep_chan_mute	: 1;	/* Mute controls each channel separat*/
	Ucbit	sep_chan_vol	: 1;	/* Vol controls each channel separat */
	Uchar	max_read_speed[2];	/* Max. read speed in KB/s	    */
	Uchar	num_vol_levels[2];	/* # of supported volume levels	    */
	Uchar	buffer_size[2];		/* Buffer size for the data in KB   */
	Uchar	cur_read_speed[2];	/* Current read speed in KB/s	    */
	Uchar	res_16;			/* Reserved			    */
	Ucbit	res_17		: 2;	/* Reserved			    */
	Ucbit	length		: 2;	/* 0=32BCKs 1=16BCKs 2=24BCKs 3=24I2c*/
	Ucbit	LSBF		: 1;	/* Set: LSB first Clear: MSB first  */
	Ucbit	RCK		: 1;	/* Set: HIGH high LRCK=left channel */
	Ucbit	BCK		: 1;	/* Data valid on falling edge of BCK */
	Ucbit	res_17_0	: 1;	/* Reserved			    */
	Uchar	max_write_speed[2];	/* Max. write speed supported in KB/s*/
	Uchar	cur_write_speed[2];	/* Current write speed in KB/s	    */

					/* Byte 22 ... Only in MMC-2	    */
	Uchar	copy_man_rev[2];	/* Copy management revision supported*/
	Uchar	res_24;			/* Reserved			    */
	Uchar	res_25;			/* Reserved			    */

					/* Byte 26 ... Only in MMC-3	    */
	Uchar	res_26;			/* Reserved			    */
	Ucbit	res_27_27	: 6;	/* Reserved			    */
	Ucbit	rot_ctl_sel	: 2;	/* Rotational control selected	    */
	Uchar	v3_cur_write_speed[2];	/* Current write speed in KB/s	    */
	Uchar	num_wr_speed_des[2];	/* # of wr speed perf descr. tables */
	struct cd_wr_speed_performance
		wr_speed_des[1];	/* wr speed performance descriptor  */
					/* Actually more (num_wr_speed_des) */
};

#endif

#define	LT_CADDY	0
#define	LT_TRAY		1
#define	LT_POP_UP	2
#define	LT_RES3		3
#define	LT_CHANGER_IND	4
#define	LT_CHANGER_CART	5
#define	LT_RES6		6
#define	LT_RES7		7


struct scsi_mode_data {
	struct scsi_mode_header		header;
	struct scsi_mode_blockdesc	blockdesc;
	union	pagex	{
		struct acb_mode_data		acb;
		struct scsi_mode_page_01	page1;
		struct scsi_mode_page_02	page2;
		struct scsi_mode_page_03	page3;
		struct scsi_mode_page_04	page4;
		struct scsi_mode_page_05	page5;
		struct scsi_mode_page_07	page7;
		struct scsi_mode_page_08	page8;
		struct scsi_mode_page_09	page9;
		struct scsi_mode_page_0A	pageA;
		struct scsi_mode_page_0B	pageB;
		struct scsi_mode_page_0C	pageC;
		struct scsi_mode_page_0D	pageD;
		struct sony_mode_page_20	sony20;
		struct toshiba_mode_page_20	toshiba20;
		struct ccs_mode_page_38		ccs38;
	} pagex;
};

struct scsi_capacity {
	Int32_t	c_baddr;		/* must convert byteorder!! */
	Int32_t	c_bsize;		/* must convert byteorder!! */
};

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_def_header {
	Ucbit		: 8;
	Ucbit	format	: 3;
	Ucbit	gdl	: 1;
	Ucbit	mdl	: 1;
	Ucbit		: 3;
	Uchar	length[2];
};

#else					/* Motorola byteorder */

struct scsi_def_header {
	Ucbit		: 8;
	Ucbit		: 3;
	Ucbit	mdl	: 1;
	Ucbit	gdl	: 1;
	Ucbit	format	: 3;
	Uchar	length[2];
};
#endif


#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_format_header {
	Ucbit	res		: 8;	/* Adaptec 5500: 1 --> format track */
	Ucbit	vu		: 1;	/* Vendor Unique		    */
	Ucbit	immed		: 1;	/* Return Immediately from Format   */
	Ucbit	tryout		: 1;	/* Check if format parameters OK    */
	Ucbit	ipattern	: 1;	/* Init patter descriptor present   */
	Ucbit	serr		: 1;	/* Stop on error		    */
	Ucbit	dcert		: 1;	/* Disable certification	    */
	Ucbit	dmdl		: 1;	/* Disable manufacturer defect list */
	Ucbit	enable		: 1;	/* Enable to use the next 3 bits    */
	Uchar	length[2];		/* Length of following list in bytes*/
};

#else					/* Motorola byteorder */

struct scsi_format_header {
	Ucbit	res		: 8;	/* Adaptec 5500: 1 --> format track */
	Ucbit	enable		: 1;	/* Enable to use the next 3 bits    */
	Ucbit	dmdl		: 1;	/* Disable manufacturer defect list */
	Ucbit	dcert		: 1;	/* Disable certification	    */
	Ucbit	serr		: 1;	/* Stop on error		    */
	Ucbit	ipattern	: 1;	/* Init patter descriptor present   */
	Ucbit	tryout		: 1;	/* Check if format parameters OK    */
	Ucbit	immed		: 1;	/* Return Immediately from Format   */
	Ucbit	vu		: 1;	/* Vendor Unique		    */
	Uchar	length[2];		/* Length of following list in bytes*/
};
#endif

struct	scsi_def_bfi {
	Uchar	cyl[3];
	Uchar	head;
	Uchar	bfi[4];
};

struct	scsi_def_phys {
	Uchar	cyl[3];
	Uchar	head;
	Uchar	sec[4];
};

struct	scsi_def_list {
	struct	scsi_def_header	hd;
	union {
			Uchar		list_block[1][4];
		struct	scsi_def_bfi	list_bfi[1];
		struct	scsi_def_phys	list_phys[1];
	} def_list;
};

struct	scsi_format_data {
	struct scsi_format_header hd;
	union {
			Uchar		list_block[1][4];
		struct	scsi_def_bfi	list_bfi[1];
		struct	scsi_def_phys	list_phys[1];
	} def_list;
};

#define	def_block	def_list.list_block
#define	def_bfi		def_list.list_bfi
#define	def_phys	def_list.list_phys

#define	SC_DEF_BLOCK	0
#define	SC_DEF_BFI	4
#define	SC_DEF_PHYS	5
#define	SC_DEF_VU	6
#define	SC_DEF_RES	7

struct scsi_format_cap_header {
	Uchar	res[3];			/* Reserved			*/
	Uchar	len;			/* Len (a multiple of 8)	*/
};

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct scsi_format_cap_desc {
	Uchar	nblock[4];		/* Number of blocks		*/
	Ucbit	desc_type	: 2;	/* Descriptor type		*/
	Ucbit	fmt_type	: 6;	/* Format Type			*/
	Uchar	blen[3];		/* Logical block length		*/
};

#else					/* Motorola byteorder */

struct scsi_format_cap_desc {
	Uchar	nblock[4];		/* Number of blocks		*/
	Ucbit	fmt_type	: 6;	/* Format Type			*/
	Ucbit	desc_type	: 2;	/* Descriptor type		*/
	Uchar	blen[3];		/* Logical block length		*/
};
#endif

/*
 * Defines for 'fmt_type'.
 */
#define	FCAP_TYPE_FULL		0x00	/* Full Format			*/
#define	FCAP_TYPE_EXPAND_SPARE	0x01	/* Spare area expansion		*/
#define	FCAP_TYPE_ZONE_REFORMAT	0x04	/* DVD-RAM Zone Reformat	*/
#define	FCAP_TYPE_ZONE_FORMAT	0x05	/* DVD-RAM Zone Format		*/
#define	FCAP_TYPE_CDRW_FULL	0x10	/* CD-RW/DVD-RW Full Format	*/
#define	FACP_TYPE_CDRW_GROW_SES	0x11	/* CD-RW/DVD-RW grow session	*/
#define	FACP_TYPE_CDRW_ADD_SES	0x12	/* CD-RW/DVD-RW add session	*/
#define	FACP_TYPE_DVDRW_QGROW	0x13	/* DVD-RW quick grow last border*/
#define	FACP_TYPE_DVDRW_QADD_SES 0x14	/* DVD-RW quick add session	*/
#define	FACP_TYPE_DVDRW_QUICK	0x15	/* DVD-RW quick interm. session	*/
#define	FCAP_TYPE_FULL_SPARE	0x20	/* Full Format with sparing	*/
#define	FCAP_TYPE_MRW_FULL	0x24	/* CD-RW/DVD+RW Full Format	*/
#define	FCAP_TYPE_DVDPLUS_BASIC	0x26	/* DVD+RW Basic Format		*/
#define	FCAP_TYPE_DVDPLUS_FULL	0x26	/* DVD+RW Full Format		*/
#define	FCAP_TYPE_BDRE_SPARE	0x30	/* BD-RE Full Format with spare	*/
#define	FCAP_TYPE_BDRE		0x31	/* BD-RE Full Format without spare*/
#define	FCAP_TYPE_BDR_SPARE	0x32	/* BD-R Full Format with spare	*/

/*
 * Defines for 'desc_type'.
 * In case of FCAP_DESC_RES, the descriptor is a formatted capacity descriptor
 * and the 'blen' field is type dependent.
 * For all other cases, this is the Current/Maximum Capacity descriptor and
 * the value of 'fmt_type' is reserved and must be zero.
 */
#define	FCAP_DESC_RES		0	/* Reserved			*/
#define	FCAP_DESC_UNFORM	1	/* Unformatted Media		*/
#define	FCAP_DESC_FORM		2	/* Formatted Media		*/
#define	FCAP_DESC_NOMEDIA	3	/* No Media			*/

struct	scsi_cap_data {
	struct scsi_format_cap_header	hd;
	struct scsi_format_cap_desc	list[1];
};


struct	scsi_send_diag_cmd {
	Uchar	cmd;
	Uchar	addr[4];
	Ucbit		: 8;
};

#if	defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct	scsi_sector_header {
	Uchar	cyl[2];
	Uchar	head;
	Uchar	sec;
	Ucbit		: 5;
	Ucbit	rp	: 1;
	Ucbit	sp	: 1;
	Ucbit	dt	: 1;
};

#else					/* Motorola byteorder */

struct	scsi_sector_header {
	Uchar	cyl[2];
	Uchar	head;
	Uchar	sec;
	Ucbit	dt	: 1;
	Ucbit	sp	: 1;
	Ucbit	rp	: 1;
	Ucbit		: 5;
};
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SCG_SCSIREG_H */
