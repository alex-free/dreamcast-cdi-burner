/* @(#)mmcvendor.h	1.4 06/09/13 Copyright 2002-2004 J. Schilling */
/*
 *	Copyright (c) 2002-2004 J. Schilling
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

#ifndef	_MMCVENDOR_H
#define	_MMCVENDOR_H

#include <schily/utypes.h>
#include <schily/btorder.h>

#if defined(_BIT_FIELDS_LTOH)	/* Intel bitorder */

struct ricoh_mode_page_30 {
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0xE = 14 Bytes */
	Ucbit	BUEFS		:1;	/* Burn-Free supported	*/
	Ucbit	TWBFS		:1;	/* Test Burn-Free sup.	*/
	Ucbit	res_2_23	:2;
	Ucbit	ARSCS		:1;	/* Auto read speed control supp. */
	Ucbit	AWSCS		:1;	/* Auto write speed control supp. */
	Ucbit	res_2_67	:2;
	Ucbit	BUEFE		:1;	/* Burn-Free enabled	*/
	Ucbit	res_2_13	:3;
	Ucbit	ARSCE		:1;	/* Auto read speed control enabled */
	Ucbit	AWSCD		:1;	/* Auto write speed control disabled */
	Ucbit	res_3_67	:2;
	Uchar	link_counter[2];	/* Burn-Free link counter */
	Uchar	res[10];		/* Padding up to 16 bytes */
};

#else				/* Motorola bitorder */

struct ricoh_mode_page_30 {
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0xE = 14 Bytes */
	Ucbit	res_2_67	:2;
	Ucbit	AWSCS		:1;	/* Auto write speed control supp. */
	Ucbit	ARSCS		:1;	/* Auto read speed control supp. */
	Ucbit	res_2_23	:2;
	Ucbit	TWBFS		:1;	/* Test Burn-Free sup.	*/
	Ucbit	BUEFS		:1;	/* Burn-Free supported	*/
	Ucbit	res_3_67	:2;
	Ucbit	AWSCD		:1;	/* Auto write speed control disabled */
	Ucbit	ARSCE		:1;	/* Auto read speed control enabled */
	Ucbit	res_2_13	:3;
	Ucbit	BUEFE		:1;	/* Burn-Free enabled	*/
	Uchar	link_counter[2];	/* Burn-Free link counter */
	Uchar	res[10];		/* Padding up to 16 bytes */
};
#endif

struct cd_mode_vendor {
	struct scsi_mode_header header;
	union cd_v_pagex {
		struct ricoh_mode_page_30 page30;
	} pagex;
};


#endif	/* _MMCVENDOR_H */
