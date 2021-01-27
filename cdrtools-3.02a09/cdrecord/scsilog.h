/* @(#)scsilog.h	1.5 06/09/13 Copyright 1998-2004 J. Schilling */
/*
 *	Definitions for SCSI log structures
 *
 *	Copyright (c) 1998-2004 J. Schilling
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

#ifndef	_SCSILOG_H
#define	_SCSILOG_H

#include <schily/utypes.h>
#include <schily/btorder.h>

/*
 * Definitions for the Page Control field
 */
#define	LOG_TRESH	0		/* (Current) Treshold values	*/
#define	LOG_CUMUL	1		/* (Current) Cumulative values	*/
#define	LOG_DFL_TRESH	2		/* Default Treshold values	*/
#define	LOG_DFL_CUMUL	3		/* Default Cumulatice values	*/

#if defined(_BIT_FIELDS_LTOH)	/* Intel bitorder */

typedef struct scsi_log_header {	/* Log Page header		*/
	Ucbit	p_code	:6;		/* Page code			*/
	Ucbit	res	:2;		/* Reserved			*/
	Uchar	res1;			/* Reserved			*/
	Uchar	p_len[2];		/* Page len (n-3)		*/
} scsi_log_hdr;

#else				/* Motorola bitorder */

typedef struct scsi_log_header {	/* Log Page header		*/
	Ucbit	res	:2;		/* Reserved			*/
	Ucbit	p_code	:6;		/* Page code			*/
	Uchar	res1;			/* Reserved			*/
	Uchar	p_len[2];		/* Page len (n-3)		*/
} scsi_log_hdr;

#endif

#if defined(_BIT_FIELDS_LTOH)	/* Intel bitorder */

typedef struct scsi_logp_header {	/* Log Parameter header		*/
	Uchar	p_code[2];		/* Parameter code		*/
	Ucbit	lp	:1;		/* List Parameter		*/
	Ucbit	res2_1	:1;		/* Reserved			*/
	Ucbit	tmc	:2;		/* Treshold met Criteria	*/
	Ucbit	etc	:1;		/* Enable Treshold Comarison	*/
	Ucbit	tds	:1;		/* Target Save Disable		*/
	Ucbit	ds	:1;		/* Disble Save			*/
	Ucbit	du	:1;		/* Disable Update		*/
	Uchar	p_len;			/* Parameter len (n-3)		*/
} scsi_logp_hdr;

#else				/* Motorola bitorder */

typedef struct scsi_logp_header {	/* Log Parameter header		*/
	Uchar	p_code[2];		/* Parameter code		*/
	Ucbit	du	:1;		/* Disable Update		*/
	Ucbit	ds	:1;		/* Disble Save			*/
	Ucbit	tds	:1;		/* Target Save Disable		*/
	Ucbit	etc	:1;		/* Enable Treshold Comarison	*/
	Ucbit	tmc	:2;		/* Treshold met Criteria	*/
	Ucbit	res2_1	:1;		/* Reserved			*/
	Ucbit	lp	:1;		/* List Parameter		*/
	Uchar	p_len;			/* Parameter len (n-3)		*/
} scsi_logp_hdr;

#endif

struct scsi_logpage_0 {
	scsi_log_hdr	hdr;		/* Log Page header		*/
	Uchar		p_code[1];	/* List of supported log pages	*/
};

/*
 * Log Pages of the Pioneer DVD-R S101
 */
struct pioneer_logpage_30_0 {
	scsi_logp_hdr	hdr;		/* Log Page header		*/
	Uchar		total_poh[4];	/* Total time powered on (hours)*/
};

struct pioneer_logpage_30_1 {
	scsi_logp_hdr	hdr;		/* Log Page header		*/
	Uchar		laser_poh[4];	/* Total time of laser turned on*/
};

struct pioneer_logpage_30_2 {
	scsi_logp_hdr	hdr;		/* Log Page header		*/
	Uchar		record_poh[4];	/* Total time of recording	*/
};

extern	int	log_sense		__PR((SCSI *scgp, caddr_t bp, int cnt, int page, int pc, int pp));
extern	BOOL	has_log_page		__PR((SCSI *scgp, int page, int pc));
extern	int	get_log			__PR((SCSI *scgp, caddr_t bp, int *lenp, int page, int pc, int pp));
extern	void	print_logpages		__PR((SCSI *scgp));

#endif	/* _SCSILOG_H */
