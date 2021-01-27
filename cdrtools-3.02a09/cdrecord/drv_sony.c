/* @(#)drv_sony.c	1.89 12/03/16 Copyright 1997-2012 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)drv_sony.c	1.89 12/03/16 Copyright 1997-2012 J. Schilling";
#endif
/*
 *	CDR device implementation for
 *	Sony
 *
 *	Copyright (c) 1997-2012 J. Schilling
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

/*#define	SONY_DEBUG*/

#include <schily/mconfig.h>

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>	/* Include sys/types.h to make off_t available */
#include <schily/standard.h>
#include <schily/fcntl.h>
#include <schily/errno.h>
#include <schily/string.h>
#include <schily/time.h>

#include <schily/utypes.h>
#include <schily/btorder.h>
#include <schily/intcvt.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

#include <scg/scgcmd.h>
#include <scg/scsidefs.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "cdrecord.h"

#ifdef	SONY_DEBUG
#	define		inc_verbose()	scgp->verbose++
#	define		dec_verbose()	scgp->verbose--
#else
#	define		inc_verbose()
#	define		dec_verbose()
#endif

extern	int	debug;
extern	int	lverbose;

#if defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct sony_924_mode_page_20 {	/* mastering information */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x06 = 6 Bytes */
	Uchar	subcode_header_off;
	Ucbit	res3_0		: 1;
	Ucbit	speudo		: 1;
	Ucbit	res3_2		: 1;
	Ucbit	c2po		: 1;
	Ucbit	subcode_ecc	: 1;
	Ucbit	res3_567	: 3;
	Uchar	res_4;
	Uchar	cue_sheet_opt;
	Uchar	res[2];
};

#else				/* Motorola byteorder */

struct sony_924_mode_page_20 {	/* mastering information */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x06 = 6 Bytes */
	Uchar	subcode_header_off;
	Ucbit	res3_567	: 3;
	Ucbit	subcode_ecc	: 1;
	Ucbit	c2po		: 1;
	Ucbit	res3_2		: 1;
	Ucbit	speudo		: 1;
	Ucbit	res3_0		: 1;
	Uchar	res_4;
	Uchar	cue_sheet_opt;
	Uchar	res[2];
};
#endif

struct sony_924_mode_page_22 {	/* disk information */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x1E = 30 Bytes */
	Uchar	disk_style;
	Uchar	disk_type;
	Uchar	first_track;
	Uchar	last_track;
	Uchar	numsess;
	Uchar	res_7;
	Uchar	disk_appl_code[4];
	Uchar	last_start_time[4];
	Uchar	disk_status;
	Uchar	num_valid_nra;
	Uchar	track_info_track;
	Uchar	post_gap;
	Uchar	disk_id_code[4];
	Uchar	lead_in_start[4];
	Uchar	res[4];
};

struct sony_924_mode_page_23 {	/* track information */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x22 = 34 Bytes */
	Uchar	res_2;
	Uchar	track_num;
	Uchar	data_form;
	Uchar	write_method;
	Uchar	session;
	Uchar	track_status;
	Uchar	start_lba[4];
	Uchar	next_recordable_addr[4];
	Uchar	blank_area_cap[4];
	Uchar	fixed_packet_size[4];
	Uchar	res_24;
	Uchar	starting_msf[3];
	Uchar	res_28;
	Uchar	ending_msf[3];
	Uchar	res_32;
	Uchar	next_rec_time[3];
};

struct sony_924_mode_page_31 {	/* drive speed */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x02 = 2 Bytes */
	Uchar	speed;
	Uchar	res;
};

struct cdd_52x_mode_data {
	struct scsi_mode_header	header;
	union cdd_pagex	{
		struct sony_924_mode_page_20	page_s20;
		struct sony_924_mode_page_22	page_s22;
		struct sony_924_mode_page_23	page_s23;
		struct sony_924_mode_page_31	page_s31;
	} pagex;
};

struct sony_write_parameter {
	Uchar	res0;			/* Reserved (must be zero)	*/
	Uchar	len;			/* Parameter length 0x32 == 52	*/
	Uchar	res2;			/* Reserved (must be zero)	*/
#if defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */
	Ucbit	res3_05		: 6;	/* Reserved			*/
	Ucbit	ms		: 2;	/* Multi session mode		*/
#else				/* Motorola byteorder */
	Ucbit	ms		: 2;	/* Multi session mode		*/
	Ucbit	res3_05		: 6;	/* Reserved			*/
#endif
	Uchar	resx[12];
#if defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */
	Ucbit	res16_06	: 7;	/* Reserved			*/
	Ucbit	mcval		: 1;	/* MCN valid			*/
#else				/* Motorola byteorder */
	Ucbit	mcval		: 1;	/* MCN valid			*/
	Ucbit	res16_06	: 7;	/* Reserved			*/
#endif
	Uchar	mcn[15];
#if defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */
	Ucbit	res32_06	: 7;	/* Reserved			*/
	Ucbit	icval		: 1;	/* ISRC valid			*/
#else				/* Motorola byteorder */
	Ucbit	icval		: 1;	/* ISRC valid			*/
	Ucbit	res32_06	: 7;	/* Reserved			*/
#endif
	Uchar	isrc[15];
	Uchar	subheader[4];
};

struct sony_cue {
	Uchar	cs_ctladr;		/* CTL/ADR for this track	*/
	Uchar	cs_tno;			/* This track number		*/
	Uchar	cs_index;		/* Index within this track	*/
	Uchar	cs_dataform;		/* Data form 			*/
					/* Bit 0..5 Main channel Format	*/
					/* Bit 6..7 SubChannel format	*/
	Uchar	cs_zero;		/* Reserved or MCN/ISRC		*/
	Uchar	cs_min;			/* Absolute time minutes	*/
	Uchar	cs_sec;			/* Absolute time seconds	*/
	Uchar	cs_frame;		/* Absolute time frames		*/
};


#define	strbeg(s1, s2)	(strstr((s2), (s1)) == (s2))

LOCAL	int	write_start_sony	__PR((SCSI *scgp, caddr_t bp, int size));
LOCAL	int	write_continue_sony	__PR((SCSI *scgp, caddr_t bp, long sectaddr, long size, int blocks, BOOL islast));
LOCAL	int	discontinue_sony	__PR((SCSI *scgp));
LOCAL	int	write_track_sony	__PR((SCSI *scgp, long track, int sectype));
LOCAL	int	close_track_sony	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
#ifdef	__needed__
LOCAL	int	flush_sony		__PR((SCSI *scgp, int track));
#endif
LOCAL	int	finalize_sony		__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	recover_sony		__PR((SCSI *scgp, cdr_t *dp, int track));
#ifdef	__needed__
LOCAL	int	set_wr_parameter_sony	__PR((SCSI *scgp, caddr_t bp, int size));
#endif
LOCAL	int	next_wr_addr_sony	__PR((SCSI *scgp, track_t *trackp, long *ap));
LOCAL	int	reserve_track_sony	__PR((SCSI *scgp, unsigned long len));
LOCAL	int	init_sony		__PR((SCSI *scgp, cdr_t *dp));
LOCAL	int	getdisktype_sony	__PR((SCSI *scgp, cdr_t *dp));
LOCAL	void	di_to_dstat_sony	__PR((struct sony_924_mode_page_22 *dip, dstat_t *dsp));
LOCAL	int	speed_select_sony	__PR((SCSI *scgp, cdr_t *dp, int *speedp));
LOCAL	int	next_writable_address_sony __PR((SCSI *scgp, long *ap, int track, int sectype, int tracktype));
LOCAL	int	new_track_sony		__PR((SCSI *scgp, int track, int sectype, int tracktype));
LOCAL	int	open_track_sony		__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	open_session_sony	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	abort_session_sony	__PR((SCSI *scgp, cdr_t *dp));
LOCAL	int	get_page22_sony		__PR((SCSI *scgp, char *mode));
LOCAL	int	gen_cue_sony		__PR((track_t *trackp, void *vcuep, BOOL needgap));
LOCAL	void	fillcue			__PR((struct sony_cue *cp, int ca, int tno, int idx, int dataform, int scms, msf_t *mp));
LOCAL	int	send_cue_sony		__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	write_leadin_sony	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	sony_attach		__PR((SCSI *scgp, cdr_t *dp));
#ifdef	SONY_DEBUG
LOCAL	void	print_sony_mp22		__PR((struct sony_924_mode_page_22 *xp, int len));
LOCAL	void	print_sony_mp23		__PR((struct sony_924_mode_page_23 *xp, int len));
#endif
LOCAL	int	buf_cap_sony		__PR((SCSI *scgp, long *, long *));

cdr_t	cdr_sony_cdu924 = {
	0, 0, 0,
	CDR_TAO|CDR_SAO|CDR_CADDYLOAD|CDR_SWABAUDIO,
	0,
	CDR_CDRW_NONE,
	WM_SAO,
	2, 4,
	"sony_cdu924",
	"driver for Sony CDU-924 / CDU-948",
	0,
	(dstat_t *)0,
	drive_identify,
	sony_attach,
	init_sony,
	getdisktype_sony,
	no_diskstatus,
	scsi_load,
	scsi_unload,
	buf_cap_sony,
	cmd_dummy,					/* recovery_needed */
	recover_sony,
	speed_select_sony,
	select_secsize,
	next_wr_addr_sony,
	reserve_track_sony,
	write_continue_sony,
	gen_cue_sony,
	send_cue_sony,
	write_leadin_sony,
	open_track_sony,
	close_track_sony,
	open_session_sony,
	cmd_dummy,
	abort_session_sony,
	read_session_offset_philips,
	finalize_sony,
	cmd_dummy,					/* stats	*/
	blank_dummy,
	format_dummy,
	(int(*)__PR((SCSI *, caddr_t, int, int)))NULL,	/* no OPC	*/
	cmd_dummy,					/* opt1		*/
	cmd_dummy,					/* opt2		*/
};

LOCAL int
write_start_sony(scgp, bp, size)
	SCSI	*scgp;
	caddr_t	bp;		/* address of CUE buffer */
	int	size;		/* number of bytes in CUE buffer */
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = size;
	scmd->flags = SCG_DISRE_ENA|SCG_CMD_RETRY;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->sense_len = 26;
	scmd->cdb.g1_cdb.cmd = 0xE0;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g0_cdbaddr(&scmd->cdb.g0_cdb, size); /* Hack, but Sony is silly */

	scgp->cmdname = "write_start";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

LOCAL int
write_continue_sony(scgp, bp, sectaddr, size, blocks, islast)
	SCSI	*scgp;
	caddr_t	bp;		/* address of buffer */
	long	sectaddr;	/* disk address (sector) to put */
	long	size;		/* number of bytes to transfer */
	int	blocks;		/* sector count */
	BOOL	islast;		/* last write for track */
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = size;
	scmd->flags = SCG_DISRE_ENA|SCG_CMD_RETRY;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xE1;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g0_cdbaddr(&scmd->cdb.g0_cdb, size); /* Hack, but Sony is silly */

	scgp->cmdname = "write_continue";

	if (scg_cmd(scgp) < 0) {
		/*
		 * XXX This seems to happen only sometimes.
		 */
		if (scg_sense_code(scgp) != 0x80)
			return (-1);
	}
	return (size - scg_getresid(scgp));
}

LOCAL int
discontinue_sony(scgp)
	SCSI	*scgp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA|SCG_CMD_RETRY;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xE2;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);

	scgp->cmdname = "discontinue";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

LOCAL int
write_track_sony(scgp, track, sectype)
	SCSI	*scgp;
	long	track;		/* track number 0 == new track */
	int	sectype;	/* no sectype for Sony write track */
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA|SCG_CMD_RETRY;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xF5;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdbaddr(&scmd->cdb.g1_cdb, track);

	scgp->cmdname = "write_track";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

/* XXX NOCH NICHT FERTIG */
LOCAL int
close_track_sony(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;
	int	track = 0;

	if (!is_tao(trackp) && !is_packet(trackp))
		return (0);

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xF0;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdbaddr(&scmd->cdb.g1_cdb, track);
/* XXX Padding ??? (bit 0 in addr[0] / CDB[2]) */

	scgp->cmdname = "close_track";

	if (scg_cmd(scgp) < 0)
		return (-1);

	/*
	 * Clear the silly "error situation" from Sony´ dummy write end
	 * but notify if real errors occurred.
	 */
	scgp->silent++;
	if (test_unit_ready(scgp) < 0 && scg_sense_code(scgp) != 0xD4) {
		scgp->cmdname = "close_track/test_unit_ready";
		scg_printerr(scgp);
	}
	scgp->silent--;

	return (0);
}

LOCAL int
finalize_sony(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;
	int	dummy = track_base(trackp)->tracktype & TOCF_DUMMY;

	if (!is_tao(trackp) && !is_packet(trackp)) {
		wait_unit_ready(scgp, 240);
		return (0);
	}
	if (dummy) {
		printf(_("Fixating is not possible in dummy write mode.\n"));
		return (0);
	}
	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->timeout = 8 * 60;		/* Needs up to 4 minutes */
	scmd->cdb.g1_cdb.cmd = 0xF1;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.count[1] = ((track_base(trackp)->tracktype & TOCF_MULTI) ? 1 : 0);
/* XXX Padding ??? (bit 0 in addr[0] / CDB[2]) */

	scgp->cmdname = "finalize";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

#ifdef	__needed__
LOCAL int
flush_sony(scgp, track)
	SCSI	*scgp;
	int	track;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->timeout = 8 * 60;		/* Needs up to 4 minutes */
	scmd->cdb.g1_cdb.cmd = 0xF2;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.cmd_cdb[5] = track;
/* XXX POE ???	   (bit 1 in addr[0] / CDB[2]) */
/* XXX Padding ??? (bit 0 in addr[0] / CDB[2]) */
/* XXX Partial flush ??? (CDB[3]) */

	scgp->cmdname = "flush";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}
#endif

LOCAL int
recover_sony(scgp, dp, track)
	SCSI	*scgp;
	cdr_t	*dp;
	int	track;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xF6;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.addr[3] = track;

	scgp->cmdname = "recover";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

#ifdef	__needed__
LOCAL int
set_wr_parameter_sony(scgp, bp, size)
	SCSI	*scgp;
	caddr_t	bp;
	int	size;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = size;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xF8;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdblen(&scmd->cdb.g1_cdb, size);

	scgp->cmdname = "set_write_parameter";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}
#endif

LOCAL int
next_wr_addr_sony(scgp, trackp, ap)
	SCSI	*scgp;
	track_t	*trackp;
	long	*ap;
{
	if (next_writable_address_sony(scgp, ap, 0, 0, 0) < 0)
		return (-1);
	return (0);
}

LOCAL int
reserve_track_sony(scgp, len)
	SCSI	*scgp;
	unsigned long len;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xF3;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	i_to_4_byte(&scmd->cdb.cmd_cdb[5], len);

	scgp->cmdname = "reserve_track";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

LOCAL int
init_sony(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	return (speed_select_sony(scgp, dp, NULL));
}


#define	IS(what, flag)	printf(_("  Is %s%s\n"), flag?"":_("not "), what);

LOCAL int
getdisktype_sony(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	dstat_t	*dsp = dp->cdr_dstat;
	long	dummy;
	long	lst;
	msf_t	msf;

	char			mode[256];
	struct scsi_mode_page_header	*mp;
	struct sony_924_mode_page_22	*xp;

	dummy = get_page22_sony(scgp, mode);
	if (dummy >= 0) {
		mp = (struct scsi_mode_page_header *)
			(mode + sizeof (struct scsi_mode_header) +
			((struct scsi_mode_header *)mode)->blockdesc_len);

		xp = (struct sony_924_mode_page_22 *)mp;

		if (xp->disk_appl_code[0] == 0xFF)
			dummy = -1;
	} else {
		return (drive_getdisktype(scgp, dp));
	}

	if ((dp->cdr_dstat->ds_cdrflags & RF_PRATIP) != 0 && dummy >= 0) {

		printf(_("ATIP info from disk:\n"));
		printf(_("  Indicated writing power: %d\n"),
				(unsigned)(xp->disk_appl_code[1] & 0x70) >> 4);
		IS(_("unrestricted"), xp->disk_appl_code[2] & 0x40);
		printf(_("  Disk application code: %d\n"), xp->disk_appl_code[2] & 0x3F);
		msf.msf_min = xp->lead_in_start[1];
		msf.msf_sec = xp->lead_in_start[2];
		msf.msf_frame = xp->lead_in_start[3];
		lst = msf_to_lba(msf.msf_min, msf.msf_sec, msf.msf_frame, FALSE);
		if (lst  < -150) {
			/*
			 * The Sony CDU 920 seems to deliver 00:00/00 for
			 * lead-in start time, dont use it.
			 */
			printf(_("  ATIP start of lead in:  %ld (%02d:%02d/%02d)\n"),
				msf_to_lba(msf.msf_min, msf.msf_sec, msf.msf_frame, FALSE),
				msf.msf_min, msf.msf_sec, msf.msf_frame);
		}
		msf.msf_min = xp->last_start_time[1];
		msf.msf_sec = xp->last_start_time[2];
		msf.msf_frame = xp->last_start_time[3];
		printf(_("  ATIP start of lead out: %ld (%02d:%02d/%02d)\n"),
			msf_to_lba(msf.msf_min, msf.msf_sec, msf.msf_frame, TRUE),
			msf.msf_min, msf.msf_sec, msf.msf_frame);
		if (lst  < -150) {
			/*
			 * The Sony CDU 920 seems to deliver 00:00/00 for
			 * lead-in start time, dont use it.
			 */
			msf.msf_min = xp->lead_in_start[1];
			msf.msf_sec = xp->lead_in_start[2];
			msf.msf_frame = xp->lead_in_start[3];
			pr_manufacturer(&msf,
					FALSE,	/* Always not erasable */
					(xp->disk_appl_code[2] & 0x40) != 0);
		}
	}
	if (dummy >= 0)
		di_to_dstat_sony(xp, dsp);
	return (drive_getdisktype(scgp, dp));
}

LOCAL void
di_to_dstat_sony(dip, dsp)
	struct sony_924_mode_page_22	*dip;
	dstat_t	*dsp;
{
	msf_t	msf;

	dsp->ds_diskid = a_to_u_4_byte(dip->disk_id_code);
#ifdef	PROTOTYPES
	if (dsp->ds_diskid != 0xFFFFFFFFUL)
#else
	if (dsp->ds_diskid != (Ulong)0xFFFFFFFF)
#endif
		dsp->ds_flags |= DSF_DID_V;
	dsp->ds_diskstat = (dip->disk_status >> 6) & 0x03;
#ifdef	XXX
	/*
	 * There seems to be no MMC equivalent...
	 */
	dsp->ds_sessstat = dip->sess_status;
#endif

	dsp->ds_maxblocks = msf_to_lba(dip->last_start_time[1],
					dip->last_start_time[2],
					dip->last_start_time[3], TRUE);
	/*
	 * Check for 0xFF:0xFF/0xFF which is an indicator for a complete disk
	 */
	if (dsp->ds_maxblocks == 716730)
		dsp->ds_maxblocks = -1L;

	if (dsp->ds_first_leadin == 0) {
		dsp->ds_first_leadin = msf_to_lba(dip->lead_in_start[1],
						dip->lead_in_start[2],
						dip->lead_in_start[3], FALSE);
		/*
		 * Check for illegal values (> 0)
		 * or for empty field (-150) with CDU-920.
		 */
		if (dsp->ds_first_leadin > 0 || dsp->ds_first_leadin == -150)
			dsp->ds_first_leadin = 0;
	}

	if (dsp->ds_last_leadout == 0 && dsp->ds_maxblocks >= 0)
		dsp->ds_last_leadout = dsp->ds_maxblocks;

	msf.msf_min = dip->lead_in_start[1];
	msf.msf_sec = dip->lead_in_start[2];
	msf.msf_frame = dip->lead_in_start[3];
	dsp->ds_maxrblocks = disk_rcap(&msf, dsp->ds_maxblocks,
					FALSE,	/* Always not erasable */
					(dip->disk_appl_code[2] & 0x40) != 0);
}


int	sony_speeds[] = {
		-1,		/* Speed null is not allowed */
		0,		/* Single speed */
		1,		/* Double speed */
		-1,		/* Three times */
		3,		/* Quad speed */
};

LOCAL int
speed_select_sony(scgp, dp, speedp)
	SCSI	*scgp;
	cdr_t	*dp;
	int	*speedp;
{
	struct cdd_52x_mode_data md;
	int	count;
	int	err;
	int	speed = 1;
	BOOL	dummy = (dp->cdr_cmdflags & F_DUMMY) != 0;

	if (speedp) {
		speed = *speedp;
		if (speed < 1 || speed > 4 || sony_speeds[speed] < 0)
			return (-1);
	}

	fillbytes((caddr_t)&md, sizeof (md), '\0');

	count  = sizeof (struct scsi_mode_header) +
		sizeof (struct sony_924_mode_page_20);

	md.pagex.page_s20.p_code = 0x20;
	md.pagex.page_s20.p_len =  0x06;
	md.pagex.page_s20.speudo = dummy?1:0;

	/*
	 * Set Cue sheet option. This is documented for the 924 and
	 * seems to be supported for the 948 too.
	 */
	md.pagex.page_s20.cue_sheet_opt = 0x03;

	err = mode_select(scgp, (Uchar *)&md, count, 0, 1);
	if (err < 0)
		return (err);

	if (speedp == 0)
		return (0);

	fillbytes((caddr_t)&md, sizeof (md), '\0');

	count  = sizeof (struct scsi_mode_header) +
		sizeof (struct sony_924_mode_page_31);

	md.pagex.page_s31.p_code = 0x31;
	md.pagex.page_s31.p_len =  0x02;
	md.pagex.page_s31.speed = sony_speeds[speed];

	return (mode_select(scgp, (Uchar *)&md, count, 0, 1));
}

LOCAL int
next_writable_address_sony(scgp, ap, track, sectype, tracktype)
	SCSI	*scgp;
	long	*ap;
	int	track;
	int	sectype;
	int	tracktype;
{
	struct	scsi_mode_page_header *mp;
	char			mode[256];
	int			len = 0x30;
	int			page = 0x23;
	struct sony_924_mode_page_23	*xp;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	inc_verbose();
	if (!get_mode_params(scgp, page, _("CD track information"),
			(Uchar *)mode, (Uchar *)0, (Uchar *)0, (Uchar *)0, &len)) {
		dec_verbose();
		return (-1);
	}
	dec_verbose();
	if (len == 0)
		return (-1);

	mp = (struct scsi_mode_page_header *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);


	xp = (struct sony_924_mode_page_23 *)mp;

#ifdef	SONY_DEBUG
	print_sony_mp23(xp, len);
#endif
	if (ap)
		*ap = a_to_4_byte(xp->next_recordable_addr);
	return (0);
}


LOCAL int
new_track_sony(scgp, track, sectype, tracktype)
	SCSI	*scgp;
	int	track;
	int	sectype;
	int	tracktype;
{
	struct	scsi_mode_page_header *mp;
	char			mode[256];
	int			len = 0x30;
	int			page = 0x23;
	struct sony_924_mode_page_23	*xp;
	int	i;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	get_page22_sony(scgp, mode);

	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	inc_verbose();
	if (!get_mode_params(scgp, page, _("CD track information"),
			(Uchar *)mode, (Uchar *)0, (Uchar *)0, (Uchar *)0, &len)) {
		dec_verbose();
		return (-1);
	}
	dec_verbose();
	if (len == 0)
		return (-1);

	mp = (struct scsi_mode_page_header *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);


	xp = (struct sony_924_mode_page_23 *)mp;

#ifdef	SONY_DEBUG
	print_sony_mp23(xp, len);
#endif

	xp->write_method = 0;	/* Track at one recording */

	if (sectype & ST_AUDIOMASK) {
		xp->data_form = (sectype & ST_MASK) == ST_AUDIO_PRE ? 0x02 : 0x00;
	} else {
		if (tracktype == TOC_ROM) {
			xp->data_form = (sectype & ST_MASK) == ST_ROM_MODE1 ? 0x10 : 0x11;
		} else if (tracktype == TOC_XA1) {
			xp->data_form = 0x12;
		} else if (tracktype == TOC_XA2) {
			xp->data_form = 0x12;
		} else if (tracktype == TOC_CDI) {
			xp->data_form = 0x12;
		}
	}

	((struct scsi_modesel_header *)mode)->sense_data_len	= 0;
	((struct scsi_modesel_header *)mode)->res2		= 0;

	i = ((struct scsi_mode_header *)mode)->blockdesc_len;
	if (i > 0) {
		i_to_3_byte(
			((struct scsi_mode_data *)mode)->blockdesc.nlblock,
								0);
	}

	if (mode_select(scgp, (Uchar *)mode, len, 0, scgp->inq->data_format >= 2) < 0) {
		return (-1);
	}

	return (0);
}

LOCAL int
open_track_sony(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t *trackp;
{
	if (!is_tao(trackp) && !is_packet(trackp)) {
		if (trackp->pregapsize > 0 && (trackp->flags & TI_PREGAP) == 0) {
			if (lverbose) {
				printf(_("Writing pregap for track %d at %ld\n"),
					(int)trackp->trackno,
					trackp->trackstart-trackp->pregapsize);
			}
			if (trackp->track == 1 && is_hidden(trackp)) {
				pad_track(scgp, dp, trackp,
					trackp->trackstart-trackp->pregapsize,
					(Llong)(trackp->pregapsize-trackp->trackstart)*trackp->secsize,
					FALSE, 0);
				if (write_track_data(scgp, dp, track_base(trackp)) < 0)
					return (-1);
			} else {
				/*
				 * XXX Do we need to check isecsize too?
				 */
				pad_track(scgp, dp, trackp,
					trackp->trackstart-trackp->pregapsize,
					(Llong)trackp->pregapsize*trackp->secsize,
					FALSE, 0);
			}
		}
		return (0);
	}

	if (select_secsize(scgp, trackp->secsize) < 0)
		return (-1);

	if (new_track_sony(scgp, trackp->trackno, trackp->sectype, trackp->tracktype & TOC_MASK) < 0)
		return (-1);

	if (write_track_sony(scgp, 0L, trackp->sectype) < 0)
		return (-1);

	return (0);
}

LOCAL int
open_session_sony(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	struct	scsi_mode_page_header *mp;
	char			mode[256];
	int	i;
	int	len = 0x30;
	struct sony_924_mode_page_22	*xp;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	if ((len = get_page22_sony(scgp, mode)) < 0)
		return (-1);

	mp = (struct scsi_mode_page_header *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	xp = (struct sony_924_mode_page_22 *)mp;

	xp->disk_type = toc2sess[track_base(trackp)->tracktype & TOC_MASK];

	if (is_tao(track_base(trackp))) {
#ifdef	__needed__
		if ((track_base(trackp)->tracktype & TOC_MASK) == TOC_DA)
			xp->disk_style = 0x80;
		else
			xp->disk_style = 0xC0;
#endif
	} else if (is_sao(track_base(trackp))) {
		/*
		 * We may only change this value if the disk is empty.
		 * i.e. when disk_status & 0xC0 == 0x00
		 */
		if ((xp->disk_status & 0xC0) != 0) {
			if (xp->disk_style != 0x00)
				errmsgno(EX_BAD, _("Cannot change disk stile for recorded disk.\n"));
		}
		xp->disk_style = 0x00;
	}

	((struct scsi_modesel_header *)mode)->sense_data_len	= 0;
	((struct scsi_modesel_header *)mode)->res2		= 0;

	i = ((struct scsi_mode_header *)mode)->blockdesc_len;
	if (i > 0) {
		i_to_3_byte(
			((struct scsi_mode_data *)mode)->blockdesc.nlblock,
								0);
	}

	if (mode_select(scgp, (Uchar *)mode, len, 0, scgp->inq->data_format >= 2) < 0) {
		return (-1);
	}
/*
 * XXX set write parameter für SAO mit Multi Session (948 only?)
 * XXX set_wr_parameter_sony(scgp, bp, size);
 */
	return (0);
}

LOCAL int
abort_session_sony(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	return (discontinue_sony(scgp));
}

LOCAL int
get_page22_sony(scgp, mode)
	SCSI	*scgp;
	char	*mode;
{
	struct	scsi_mode_page_header *mp;
	int	len = 0x30;
	int	page = 0x22;
	struct sony_924_mode_page_22	*xp;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	inc_verbose();
	if (!get_mode_params(scgp, page, _("CD disk information"),
			(Uchar *)mode, (Uchar *)0, (Uchar *)0, (Uchar *)0, &len)) {
		dec_verbose();
		return (-1);
	}
	dec_verbose();
	if (len == 0)
		return (-1);

	mp = (struct scsi_mode_page_header *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	xp = (struct sony_924_mode_page_22 *)mp;

#ifdef	SONY_DEBUG
	print_sony_mp22(xp, len);
#endif
	return (len);
}

/*--------------------------------------------------------------------------*/

LOCAL Uchar	db2df[] = {
	0x01,			/*  0 2352 bytes of raw data			*/
	0xFF,			/*  1 2368 bytes (raw data + P/Q Subchannel)	*/
	0xFF,			/*  2 2448 bytes (raw data + P-W Subchannel)	*/
	0xFF,			/*  3 2448 bytes (raw data + P-W raw Subchannel)*/
	0xFF,			/*  4 -    Reserved				*/
	0xFF,			/*  5 -    Reserved				*/
	0xFF,			/*  6 -    Reserved				*/
	0xFF,			/*  7 -    Vendor specific			*/
	0x11,			/*  8 2048 bytes Mode 1 (ISO/IEC 10149)		*/
	0xFF,			/*  9 2336 bytes Mode 2 (ISO/IEC 10149)		*/
	0xFF,			/* 10 2048 bytes Mode 2! (CD-ROM XA form 1)	*/
	0xFF,			/* 11 2056 bytes Mode 2 (CD-ROM XA form 1)	*/
	0xFF,			/* 12 2324 bytes Mode 2 (CD-ROM XA form 2)	*/
	0xFF,			/* 13 2332 bytes Mode 2 (CD-ROM XA 1/2+subhdr)	*/
	0xFF,			/* 14 -    Reserved				*/
	0xFF,			/* 15 -    Vendor specific			*/
};

LOCAL int
gen_cue_sony(trackp, vcuep, needgap)
	track_t	*trackp;
	void	*vcuep;
	BOOL	needgap;
{
	int	tracks = trackp->tracks;
	int	i;
	struct sony_cue	**cuep = vcuep;
	struct sony_cue	*cue;
	struct sony_cue	*cp;
	int	ncue = 0;
	int	icue = 0;
	int	pgsize;
	msf_t	m;
	int	ctl;
	int	df;
	int	scms;

	cue = malloc(1);

	for (i = 0; i <= tracks; i++) {
		ctl = (st2mode[trackp[i].sectype & ST_MASK]) << 4;
		if (is_copy(&trackp[i]))
			ctl |= TM_ALLOW_COPY << 4;
		df = db2df[trackp[i].dbtype & 0x0F];

#ifdef	__supported__
		if (trackp[i].isrc) {	/* MCN or ISRC */
			ncue += 2;
			cue = realloc(cue, ncue * sizeof (*cue));
			cp = &cue[icue++];
			if (i == 0) {
				cp->cs_ctladr = 0x02;
				movebytes(&trackp[i].isrc[0], &cp->cs_tno, 7);
				cp = &cue[icue++];
				cp->cs_ctladr = 0x02;
				movebytes(&trackp[i].isrc[7], &cp->cs_tno, 7);
			} else {
				cp->cs_ctladr = 0x03;
				cp->cs_tno = i;
				movebytes(&trackp[i].isrc[0], &cp->cs_index, 6);
				cp = &cue[icue++];
				cp->cs_ctladr = 0x03;
				cp->cs_tno = i;
				movebytes(&trackp[i].isrc[6], &cp->cs_index, 6);
			}
		}
#endif
		if (i == 0) {	/* Lead in */
			df &= ~7;
			if (trackp[0].flags & TI_TEXT)	/* CD-Text in Lead-in*/
				df |= 0xC0;
			lba_to_msf(-150, &m);
			cue = realloc(cue, ++ncue * sizeof (*cue));
			cp = &cue[icue++];
			fillcue(cp, ctl|0x01, i, 0, df, 0, &m);
		} else {
			scms = 0;

			if (is_scms(&trackp[i]))
				scms = 0x80;
			pgsize = trackp[i].pregapsize;
			if (pgsize == 0 && needgap)
				pgsize++;
			lba_to_msf(trackp[i].trackstart-pgsize, &m);
			cue = realloc(cue, ++ncue * sizeof (*cue));
			cp = &cue[icue++];
			fillcue(cp, ctl|0x01, i, 0, df, scms, &m);

			if (trackp[i].nindex == 1) {
				lba_to_msf(trackp[i].trackstart, &m);
				cue = realloc(cue, ++ncue * sizeof (*cue));
				cp = &cue[icue++];
				fillcue(cp, ctl|0x01, i, 1, df, scms, &m);
			} else {
				int	idx;
				long	*idxlist;

				ncue += trackp[i].nindex;
				idxlist = trackp[i].tindex;
				cue = realloc(cue, ncue * sizeof (*cue));

				for (idx = 1; idx <= trackp[i].nindex; idx++) {
					lba_to_msf(trackp[i].trackstart + idxlist[idx], &m);
					cp = &cue[icue++];
					fillcue(cp, ctl|0x01, i, idx, df, scms, &m);
				}
			}
		}
	}
	/* Lead out */
	ctl = (st2mode[trackp[tracks+1].sectype & ST_MASK]) << 4;
	df = db2df[trackp[tracks+1].dbtype & 0x0F];
	df &= ~7;
	lba_to_msf(trackp[tracks+1].trackstart, &m);
	cue = realloc(cue, ++ncue * sizeof (*cue));
	cp = &cue[icue++];
	fillcue(cp, ctl|0x01, 0xAA, 1, df, 0, &m);

	if (lverbose > 1) {
		for (i = 0; i < ncue; i++) {
			scg_prbytes("", (Uchar *)&cue[i], 8);
		}
	}
	if (cuep)
		*cuep = cue;
	else
		free(cue);
	return (ncue);
}


LOCAL void
fillcue(cp, ca, tno, idx, dataform, scms, mp)
	struct sony_cue *cp;	/* The target cue entry		*/
	int	ca;		/* Control/adr for this entry	*/
	int	tno;		/* Track number for this entry	*/
	int	idx;		/* Index for this entry		*/
	int	dataform;	/* Data format for this entry	*/
	int	scms;		/* Serial copy management	*/
	msf_t	*mp;		/* MSF value for this entry	*/
{
	cp->cs_ctladr = ca;
	if (tno <= 99)
		cp->cs_tno = to_bcd(tno);
	else
		cp->cs_tno = tno;
	cp->cs_index = to_bcd(idx);
	cp->cs_dataform = dataform;
	cp->cs_zero = scms;
	cp->cs_min = to_bcd(mp->msf_min);
	cp->cs_sec = to_bcd(mp->msf_sec);
	cp->cs_frame = to_bcd(mp->msf_frame);
}

LOCAL int
send_cue_sony(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	struct sony_cue *cp;
	int		ncue;
	int		ret;
	Uint		i;
	struct timeval starttime;
	struct timeval stoptime;
	int		disktype;

	disktype = toc2sess[track_base(trackp)->tracktype & TOC_MASK];

	for (i = 1; i <= trackp->tracks; i++) {
		if (trackp[i].tracksize < (tsize_t)0) {
			errmsgno(EX_BAD, _("Track %d has unknown length.\n"), i);
			return (-1);
		}
	}
	ncue = (*dp->cdr_gen_cue)(trackp, &cp, FALSE);

	starttime.tv_sec = 0;
	starttime.tv_usec = 0;
	stoptime = starttime;
	gettimeofday(&starttime, (struct timezone *)0);

	scgp->silent++;
	ret  = write_start_sony(scgp, (caddr_t)cp, ncue*8);
	scgp->silent--;
	free(cp);
	if (ret < 0) {
		errmsgno(EX_BAD, _("CUE sheet not accepted. Retrying with minimum pregapsize = 1.\n"));
		ncue = (*dp->cdr_gen_cue)(trackp, &cp, TRUE);
		ret  = write_start_sony(scgp, (caddr_t)cp, ncue*8);
		free(cp);
	}
	if (ret >= 0 && lverbose) {
		gettimeofday(&stoptime, (struct timezone *)0);
		prtimediff(_("Write Lead-in time: "), &starttime, &stoptime);
	}
	return (ret);
}

LOCAL int
write_leadin_sony(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t *trackp;
{
	Uint	i;
	long	startsec = 0L;

/*	if (flags & F_SAO) {*/
	if (wm_base(dp->cdr_dstat->ds_wrmode) == WM_SAO) {
		if (debug || lverbose) {
			printf(_("Sending CUE sheet...\n"));
			flush();
		}
		if (trackp[0].flags & TI_TEXT) {
			if (dp->cdr_speeddef != 4) {
				errmsgno(EX_BAD,
				_("The CDU-924 does not support CD-Text, disabling.\n"));

				trackp[0].flags &= ~TI_TEXT;
			}
		}
		if ((*dp->cdr_send_cue)(scgp, dp, trackp) < 0) {
			errmsgno(EX_BAD, _("Cannot send CUE sheet.\n"));
			return (-1);
		}

		if (trackp[0].flags & TI_TEXT) {
			startsec = dp->cdr_dstat->ds_first_leadin;
			printf(_("SAO startsec: %ld\n"), startsec);
		} else {
			startsec = -150;
		}
		if (debug)
			printf(_("SAO startsec: %ld\n"), startsec);

		if (trackp[0].flags & TI_TEXT) {
			if (startsec > 0) {
				errmsgno(EX_BAD, _("CD-Text must be in first session.\n"));
				return (-1);
			}
			if (debug || lverbose)
				printf(_("Writing lead-in...\n"));
			if (write_cdtext(scgp, dp, startsec) < 0)
				return (-1);

			dp->cdr_dstat->ds_cdrflags |= RF_LEADIN;
		} else for (i = 1; i <= trackp->tracks; i++) {
			trackp[i].trackstart += startsec +150;
		}
	}
	return (0);
}

/*--------------------------------------------------------------------------*/

static const char *sd_cdu_924_error_str[] = {

	"\200\000write complete",				/* 80 00 */
	"\201\000logical unit is reserved",			/* 81 00 */
	"\205\000audio address not valid",			/* 85 00 */
	"\210\000illegal cue sheet",				/* 88 00 */
	"\211\000inappropriate command",			/* 89 00 */

	"\266\000media load mechanism failed",			/* B6 00 */
	"\271\000audio play operation aborted",			/* B9 00 */
	"\277\000buffer overflow for read all subcodes command", /* BF 00 */
	"\300\000unrecordable disk",				/* C0 00 */
	"\301\000illegal track status",				/* C1 00 */
	"\302\000reserved track present",			/* C2 00 */
	"\303\000buffer data size error",			/* C3 00 */
	"\304\001illegal data form for reserve track command",	/* C4 01 */
	"\304\002unable to reserve track, because track mode has been changed",	/* C4 02 */
	"\305\000buffer error during at once recording",	/* C5 00 */
	"\306\001unwritten area encountered",			/* C6 01 */
	"\306\002link blocks encountered",			/* C6 02 */
	"\306\003nonexistent block encountered",		/* C6 03 */
	"\307\000disk style mismatch",				/* C7 00 */
	"\310\000no table of contents",				/* C8 00 */
	"\311\000illegal block length for write command",	/* C9 00 */
	"\312\000power calibration error",			/* CA 00 */
	"\313\000write error",					/* CB 00 */
	"\313\001write error track recovered",			/* CB 01 */
	"\314\000not enough space",				/* CC 00 */
	"\315\000no track present to finalize",			/* CD 00 */
	"\316\000unrecoverable track descriptor encountered",	/* CE 00 */
	"\317\000damaged track present",			/* CF 00 */
	"\320\000pma area full",				/* D0 00 */
	"\321\000pca area full",				/* D1 00 */
	"\322\000unrecoverable damaged track cause too small writing area",	/* D2 00 */
	"\323\000no bar code",					/* D3 00 */
	"\323\001not enough bar code margin",			/* D3 01 */
	"\323\002no bar code start pattern",			/* D3 02 */
	"\323\003illegal bar code length",			/* D3 03 */
	"\323\004illegal bar code format",			/* D3 04 */
	"\324\000exit from pseudo track at once recording",	/* D4 00 */
	NULL
};

LOCAL int
sony_attach(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	if (scgp->inq != NULL) {
		if (strbeg("CD-R   CDU94", scgp->inq->inq_prod_ident)) {
			dp->cdr_speeddef = 4;
		}
	}
	scg_setnonstderrs(scgp, sd_cdu_924_error_str);
	return (0);
}

#ifdef	SONY_DEBUG
LOCAL void
print_sony_mp22(xp, len)
	struct sony_924_mode_page_22	*xp;
	int				len;
{
	printf("disk style: %X\n", xp->disk_style);
	printf("disk type: %X\n", xp->disk_type);
	printf("first track: %X\n", xp->first_track);
	printf("last track: %X\n", xp->last_track);
	printf("numsess:    %X\n", xp->numsess);
	printf("disk appl code: %lX\n", a_to_u_4_byte(xp->disk_appl_code));
	printf("last start time: %lX\n", a_to_u_4_byte(xp->last_start_time));
	printf("disk status: %X\n", xp->disk_status);
	printf("num valid nra: %X\n", xp->num_valid_nra);
	printf("track info track: %X\n", xp->track_info_track);
	printf("post gap: %X\n", xp->post_gap);
	printf("disk id code: %lX\n", a_to_u_4_byte(xp->disk_id_code));
	printf("lead in start: %lX\n", a_to_u_4_byte(xp->lead_in_start));
}

LOCAL void
print_sony_mp23(xp, len)
	struct sony_924_mode_page_23	*xp;
	int				len;
{
	printf("len: %d\n", len);

	printf("track num: %X\n", xp->track_num);
	printf("data form: %X\n", xp->data_form);
	printf("write method: %X\n", xp->write_method);
	printf("session: %X\n", xp->session);
	printf("track status: %X\n", xp->track_status);

/*
 * XXX Check for signed/unsigned a_to_*() conversion.
 */
	printf("start lba: %lX\n", a_to_4_byte(xp->start_lba));
	printf("next recordable addr: %lX\n", a_to_4_byte(xp->next_recordable_addr));
	printf("blank area cap: %lX\n", a_to_u_4_byte(xp->blank_area_cap));
	printf("fixed packet size: %lX\n", a_to_u_4_byte(xp->fixed_packet_size));
	printf("starting msf: %lX\n", a_to_u_4_byte(xp->starting_msf));
	printf("ending msf: %lX\n", a_to_u_4_byte(xp->ending_msf));
	printf("next rec time: %lX\n", a_to_u_4_byte(xp->next_rec_time));
}
#endif

LOCAL int
buf_cap_sony(scgp, sp, fp)
	SCSI	*scgp;
	long	*sp;	/* Size pointer */
	long	*fp;	/* Free pointer */
{
	char	resp[8];
	Ulong	freespace;
	Ulong	bufsize;
	int	per;
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)resp;
	scmd->size = sizeof (resp);
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xEC;		/* Read buffer cap */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);

	scgp->cmdname = "read buffer cap sony";

	if (scg_cmd(scgp) < 0)
		return (-1);

	bufsize   = a_to_u_3_byte(&resp[1]);
	freespace = a_to_u_3_byte(&resp[5]);
	if (sp)
		*sp = bufsize;
	if (fp)
		*fp = freespace;

	if (scgp->verbose || (sp == 0 && fp == 0))
		printf(_("BFree: %ld K BSize: %ld K\n"), freespace >> 10, bufsize >> 10);

	if (bufsize == 0)
		return (0);
	per = (100 * (bufsize - freespace)) / bufsize;
	if (per < 0)
		return (0);
	if (per > 100)
		return (100);
	return (per);
}
