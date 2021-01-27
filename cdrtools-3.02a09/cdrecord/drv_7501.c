/* @(#)drv_7501.c	1.30 10/12/19 Copyright 2003-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)drv_7501.c	1.30 10/12/19 Copyright 2003-2010 J. Schilling";
#endif
/*
 *	Device driver for the Masushita CW-7501
 *
 *	Copyright (c) 2003-2010 J. Schilling
 *
 * Mode Pages:
 *	0x01	error recovery		Seite 100
 *	0x02	disconnect/reconnect	Seite 107
 *	0x0D	CD-ROM device parameter	Seite 110
 *	0x0E	CD-ROM Audio control	Seite 112
 *	0x20	Speed & Tray position	Seite 115
 *	0x21	Media catalog number	Seite 124
 *	0x22	ISRC			Seite 125
 *	0x23	Dummy/Write Information	Seite 126
 *	0x24	CD-R disk information	Seite 127
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

#ifndef	DEBUG
#define	DEBUG
#endif
#include <schily/mconfig.h>

#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
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

#include <schily/libport.h>

#include "cdrecord.h"

extern	int	silent;
extern	int	debug;
extern	int	verbose;
extern	int	lverbose;
extern	int	xdebug;

#if defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct cw7501_mode_page_20 {	/* Speed control */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x02 = 2 Bytes */
	Uchar	speed;
	Ucbit	res		: 7;
	Ucbit	traypos		: 1;
};

#else				/* Motorola byteorder */

struct cw7501_mode_page_20 {	/* Speed control */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x02 = 2 Bytes */
	Uchar	speed;
	Ucbit	traypos		: 1;
	Ucbit	res		: 7;
};
#endif

#if defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct cw7501_mode_page_21 {	/* MCN */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x12 = 20 Bytes */
	Ucbit	control		: 4;
	Ucbit	addr		: 4;
	Uchar	res_3;
	Uchar	res_4;
	Uchar	mcn[15];
};

#else				/* Motorola byteorder */

struct cw7501_mode_page_21 {	/* MCN */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x12 = 20 Bytes */
	Ucbit	addr		: 4;
	Ucbit	control		: 4;
	Uchar	res_3;
	Uchar	res_4;
	Uchar	mcn[15];
};
#endif


#if defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct cw7501_mode_page_22 {	/* ISRC */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x12 = 20 Bytes */
	Ucbit	control		: 4;
	Ucbit	addr		: 4;
	Uchar	trackno;
	Uchar	res_4;
	Uchar	isrc[15];
};

#else				/* Motorola byteorder */

struct cw7501_mode_page_22 {	/* ISRC */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x12 = 20 Bytes */
	Ucbit	addr		: 4;
	Ucbit	control		: 4;
	Uchar	trackno;
	Uchar	res_4;
	Uchar	isrc[15];
};
#endif

#if defined(_BIT_FIELDS_LTOH)	/* Intel byteorder */

struct cw7501_mode_page_23 {	/* Dummy / Write information */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x02 = 2 Bytes */
	Uchar	res;
	Ucbit	autopg		: 1;
	Ucbit	dummy		: 1;
	Ucbit	res3_72		: 6;
};

#else				/* Motorola byteorder */

struct cw7501_mode_page_23 {	/* Dummy / Write information */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x02 = 2 Bytes */
	Uchar	res;
	Ucbit	res3_72		: 6;
	Ucbit	dummy		: 1;
	Ucbit	autopg		: 1;
};
#endif

struct cw7501_mode_page_24 {	/* CD-R Disk information */
		MP_P_CODE;		/* parsave & pagecode */
	Uchar	p_len;			/* 0x0A = 12 Bytes */
	Uchar	disktype;
	Uchar	res;
	Uchar	appl_code[4];
	Uchar	disk_id[4];
};

struct cw7501_mode_data {
	struct scsi_mode_header	header;
	union cdd_pagex	{
		struct cw7501_mode_page_20	page20;
		struct cw7501_mode_page_21	page21;
		struct cw7501_mode_page_22	page22;
		struct cw7501_mode_page_23	page23;
		struct cw7501_mode_page_24	page24;
	} pagex;
};

/*
 * Mode for read track information
 */
#define	TI_TRACKINFO_R	0
#define	TI_NWA		1
#define	TI_PMA		2
#define	TI_TRACKINFO	3

struct cw7501_nwa {
	Uchar	nwa_length[2];
	Uchar	nwa_res;
	Uchar	nwa_trackno;
	Uchar	nwa_nwa[4];
	Uchar	nwa_freeblocks[4];
};

struct cw7501_cue {
	Uchar	cs_ctladr;		/* CTL/ADR for this track	*/
	Uchar	cs_tno;			/* This track number		*/
	Uchar	cs_index;		/* Index within this track	*/
	Uchar	cs_dataform;		/* Data form 			*/
					/* Bit 0..3 Physical Format	*/
					/* Bit 4 Alt Copy (SCMS)	*/
					/* Bit 5 SubC Audio + RAW96 sub */
	Uchar	cs_extension;		/* Reserved or MCN/ISRC		*/
	Uchar	cs_min;			/* Absolute time minutes	*/
	Uchar	cs_sec;			/* Absolute time seconds	*/
	Uchar	cs_frame;		/* Absolute time frames		*/
};


LOCAL	int	cw7501_attach		__PR((SCSI *scgp, cdr_t *dp));
LOCAL	int	cw7501_init		__PR((SCSI *scgp, cdr_t *dp));
LOCAL	int	cw7501_getdisktype	__PR((SCSI *scgp, cdr_t *dp));
LOCAL	int	cw7501_speed_select	__PR((SCSI *scgp, cdr_t *dp, int *speedp));
LOCAL	int	cw7501_next_wr_addr	__PR((SCSI *scgp, track_t *trackp, long *ap));
LOCAL	int	cw7501_write		__PR((SCSI *scgp, caddr_t bp, long sectaddr, long size, int blocks, BOOL islast));
LOCAL	int	cw7501_write_leadin	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	cw7501_open_track	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	cw7501_close_track	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	cw7501_open_session	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	cw7501_gen_cue		__PR((track_t *trackp, void *vcuep, BOOL needgap));
LOCAL	void	fillcue			__PR((struct cw7501_cue *cp, int ca, int tno, int idx, int dataform, int scms, msf_t *mp));
LOCAL	int	cw7501_send_cue		__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	cw7501_fixate		__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	cw7501_rezero		__PR((SCSI *scgp, int reset, int dwreset));
LOCAL	int	cw7501_read_trackinfo	__PR((SCSI *scgp, Uchar *bp, int count, int track, int mode));
LOCAL	int	cw7501_write_dao	__PR((SCSI *scgp, Uchar *bp, int len, int disktype));
LOCAL	int	cw7501_reserve_track	__PR((SCSI *scgp, unsigned long));
LOCAL	int	cw7501_set_mode		__PR((SCSI *scgp, int phys_form, int control,
						int subc, int alt, int trackno, int tindex,
						int packet_size, int write_mode));
LOCAL	int	cw7501_finalize		__PR((SCSI *scgp, int pad, int fixed));


cdr_t	cdr_cw7501 = {
	0, 0, 0,
	/*
	 * Prinzipiell geht auch: CDR_PACKET & CDR_SRAW96R
	 */
	CDR_TAO|CDR_SAO|CDR_TRAYLOAD,
	0,
	CDR_CDRW_NONE,
	WM_SAO,
	2, 2,
	"cw_7501",
	"driver for Matsushita/Panasonic CW-7501",
	0,
	(dstat_t *)0,
	drive_identify,
	cw7501_attach,
	cw7501_init,
	cw7501_getdisktype,
	no_diskstatus,
	scsi_load,
	scsi_unload,
	buf_dummy,				/* RD buffer cap not supp. */
	cmd_dummy,					/* recovery_needed */
	(int(*)__PR((SCSI *, cdr_t *, int)))cmd_dummy,	/* recover	*/
	cw7501_speed_select,
	select_secsize,
	cw7501_next_wr_addr,
	cw7501_reserve_track,
	cw7501_write,
	cw7501_gen_cue,
	cw7501_send_cue,
	cw7501_write_leadin,
	cw7501_open_track,
	cw7501_close_track,
	cw7501_open_session,
	cmd_dummy,				/* close seession	*/
	cmd_dummy,					/* abort	*/
	read_session_offset,
	cw7501_fixate,
	cmd_dummy,					/* stats	*/
	blank_dummy,
	format_dummy,
	(int(*)__PR((SCSI *, caddr_t, int, int)))NULL,	/* no OPC	*/
	cmd_dummy,					/* opt1		*/
	cmd_dummy,					/* opt2		*/
};

static const char *sd_cw7501_error_str[] = {
	"\100\201diagnostic failure on ROM",			/* 40 81 */
	"\100\202diagnostic failure on CPU internal RAM",	/* 40 82 */
	"\100\203diagnostic failure on BUFFER RAM",		/* 40 83 */
	"\100\204diagnostic failure on internal SCSI controller",	/* 40 84 */
	"\100\205diagnostic failure on system mechanism",	/* 40 85 */

	"\210\000Illegal Que Sheet (DAO parameter)",		/* 88 00 */
	"\211\000Inappropriate command",			/* 89 00 */

	"\250\000Audio Play operation Not in Progress",		/* A8 00 */
	"\251\000Buffer Overrun",				/* A9 00 */

	"\300\000Unrecordable Disk",				/* C0 00 */
	"\301\000Illegal Track Status",				/* C1 00 */
	"\302\000Reserved track Status",			/* C2 00 */
	"\304\000Illegal Reserve Length for Reserve Track Command",	/* C4 00 */
	"\304\001Illegal Data Form for Reserve Track Command",	/* C4 01 */
	"\304\002Unable to Reserve Track, Because Track Mode has been Changed",	/* C4 02 */

	"\305\000Buffer error during recording",		/* C5 00 */
	"\307\000Disk Style mismatch",				/* C7 00 */
	"\312\000Power Calibration error",			/* CA 00 */
	"\313\000Write error (Fatal Error/Time out)",		/* CB 00 */
	"\314\000Not enough space (Leadin/Leadout space)",	/* CC 00 */
	"\315\000No track present to finalize",			/* CD 00 */
	"\317\000Unable to recover damaged disk",		/* CF 00 */

	"\320\000PMA area full (1000 blocks)",			/* D0 00 */
	"\321\000PCA area full (100 counts)",			/* D1 00 */
	"\322\000Recovery failed",				/* D2 00 */
	"\323\000Recovery needed",				/* D3 00 */
	NULL
};

LOCAL int
cw7501_attach(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	scg_setnonstderrs(scgp, sd_cw7501_error_str);
	return (0);
}

LOCAL int
cw7501_init(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	return (cw7501_speed_select(scgp, dp, NULL));
}

LOCAL int
cw7501_getdisktype(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	Ulong	maxb = 0;
	Uchar	buf[256];
	int	ret;
	dstat_t	*dsp = dp->cdr_dstat;

	if (xdebug > 0) {
		scgp->silent++;
		fillbytes((caddr_t)buf, sizeof (buf), '\0');
		ret = cw7501_read_trackinfo(scgp, buf, 32, 0, 0);
		if (ret >= 0)
		scg_prbytes("TI EXIST-R   (0): ", buf, 32 -scg_getresid(scgp));

		fillbytes((caddr_t)buf, sizeof (buf), '\0');
		ret = cw7501_read_trackinfo(scgp, buf, 32, 0, 1);
		if (ret >= 0)
		scg_prbytes("TI NWA       (1): ", buf, 32 -scg_getresid(scgp));

		fillbytes((caddr_t)buf, sizeof (buf), '\0');
		ret = cw7501_read_trackinfo(scgp, buf, 32, 0, 2);
		if (ret >= 0)
		scg_prbytes("TI PMA       (2): ", buf, 32 -scg_getresid(scgp));

		fillbytes((caddr_t)buf, sizeof (buf), '\0');
		ret = cw7501_read_trackinfo(scgp, buf, 32, 0, 3);
		if (ret >= 0)
		scg_prbytes("TI EXIST-ROM (3): ", buf, 32 -scg_getresid(scgp));
		scgp->silent--;
	}

	fillbytes((caddr_t)buf, sizeof (buf), '\0');

	scgp->silent++;
	ret = cw7501_read_trackinfo(scgp, buf, 12, 0, TI_NWA);
	if (ret < 0 &&
			(dsp->ds_cdrflags & (RF_WRITE|RF_BLANK)) == RF_WRITE) {

		/*
		 * Try to clear the dummy bit to reset the virtual
		 * drive status. Not all drives support it even though
		 * it is mentioned in the MMC standard.
		 */
		if (lverbose)
			printf(_("Trying to clear drive status.\n"));
		cw7501_rezero(scgp, 0, 1);
		wait_unit_ready(scgp, 60);
		ret = cw7501_read_trackinfo(scgp, buf, 12, 0, TI_NWA);
	}
	scgp->silent--;

	if (ret >= 0) {
		maxb = a_to_u_4_byte(&buf[8]);
		if (maxb != 0)
			maxb -= 150;
	}
	dsp->ds_maxblocks = maxb;

	return (drive_getdisktype(scgp, dp));
}


LOCAL int
cw7501_speed_select(scgp, dp, speedp)
	SCSI	*scgp;
	cdr_t	*dp;
	int	*speedp;
{
	struct	scsi_mode_page_header	*mp;
	char				mode[256];
	int				len = 20;
	int				page = 0x20;
	struct cw7501_mode_page_20	*xp20;
	struct cw7501_mode_data		md;
	int				count;
	int				speed = 1;
	BOOL				dummy = (dp->cdr_cmdflags & F_DUMMY) != 0;

	if (speedp) {
		speed = *speedp;
	} else {
		fillbytes((caddr_t)mode, sizeof (mode), '\0');

		if (!get_mode_params(scgp, page, _("Speed information"),
			(Uchar *)mode, (Uchar *)0, (Uchar *)0, (Uchar *)0, &len)) {
			return (-1);
		}
		if (len == 0)
			return (-1);

		mp = (struct scsi_mode_page_header *)
			(mode + sizeof (struct scsi_mode_header) +
			((struct scsi_mode_header *)mode)->blockdesc_len);

		xp20  = (struct cw7501_mode_page_20 *)mp;
		speed = xp20->speed;
	}

	fillbytes((caddr_t)&md, sizeof (md), '\0');

	count  = sizeof (struct scsi_mode_header) +
		sizeof (struct cw7501_mode_page_20);

	md.pagex.page20.p_code = 0x20;
	md.pagex.page20.p_len =  0x02;
	md.pagex.page20.speed = speed;

	if (mode_select(scgp, (Uchar *)&md, count, 0, scgp->inq->data_format >= 2) < 0)
		return (-1);

	fillbytes((caddr_t)&md, sizeof (md), '\0');

	count  = sizeof (struct scsi_mode_header) +
		sizeof (struct cw7501_mode_page_23);

	md.pagex.page23.p_code = 0x23;
	md.pagex.page23.p_len =  0x02;
	md.pagex.page23.dummy = dummy?1:0;

	return (mode_select(scgp, (Uchar *)&md, count, 0, scgp->inq->data_format >= 2));
}

LOCAL int
cw7501_next_wr_addr(scgp, trackp, ap)
	SCSI	*scgp;
	track_t	*trackp;
	long	*ap;
{
	struct cw7501_nwa	*nwa;
	Uchar	buf[256];
	long	next_addr;
	int	result = -1;


	/*
	 * Reading info for current track may require doing the read_track_info
	 * with either the track number (if the track is currently being written)
	 * or with 0 (if the track hasn't been started yet and is invisible
	 */
	nwa = (struct cw7501_nwa *)buf;

	if (trackp != 0 && trackp->track > 0 && is_packet(trackp)) {
		fillbytes((caddr_t)buf, sizeof (buf), '\0');

		scgp->silent++;
		result = cw7501_read_trackinfo(scgp, buf, sizeof (*nwa),
							trackp->trackno,
							TI_NWA);
		scgp->silent--;
	}

	if (result < 0) {
		if (cw7501_read_trackinfo(scgp, buf, sizeof (*nwa),
							0, TI_NWA) < 0)
			return (-1);
	}
	if (scgp->verbose)
		scg_prbytes(_("track info:"), buf,
				12-scg_getresid(scgp));
	next_addr = a_to_4_byte(nwa->nwa_nwa);
	/*
	 * XXX Für TAO definitiv notwendig.
	 * XXX ABhängig von Auto-Pregap?
	 */
	/* XXX */ next_addr += 150;
	if (ap)
		*ap = next_addr;
	return (0);
}

LOCAL int
cw7501_write(scgp, bp, sectaddr, size, blocks, islast)
	SCSI	*scgp;
	caddr_t	bp;		/* address of buffer */
	long	sectaddr;	/* disk address (sector) to put */
	long	size;		/* number of bytes to transfer */
	int	blocks;		/* sector count */
	BOOL	islast;		/* last write for track */
{
	if (lverbose > 1 && islast)
		printf(_("\nWriting last record for this track.\n"));

	return (write_xg0(scgp, bp, 0, size, blocks));
}

LOCAL int
cw7501_write_leadin(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t *trackp;
{
	Uint	i;
	long	startsec = 0L;

	if (wm_base(dp->cdr_dstat->ds_wrmode) == WM_SAO) {
		if (debug || lverbose) {
			printf(_("Sending CUE sheet...\n"));
			flush();
		}
		if ((*dp->cdr_send_cue)(scgp, dp, trackp) < 0) {
			errmsgno(EX_BAD, _("Cannot send CUE sheet.\n"));
			return (-1);
		}

		/*
		 * Next writable address function does not work in DAO
		 * mode for this writer, so we just assume -150.
		 */
		startsec = -150;
		if (debug)
			printf(_("SAO startsec: %ld\n"), startsec);

		if (trackp[0].flags & TI_TEXT) {
			errmsgno(EX_BAD, _("CD-Text unsupported in CW-7501 - ignoring.\n"));
		} else for (i = 1; i <= trackp->tracks; i++) {
			trackp[i].trackstart += startsec +150;
		}
	}
	return (0);
}

LOCAL Uchar	db2phys[] = {
	0x00,			/*  0 2352 bytes of raw data			*/
	0xFF,			/*  1 2368 bytes (raw data + P/Q Subchannel)	*/
	0xFF,			/*  2 2448 bytes (raw data + P-W Subchannel)	*/
	0xFF,			/*  3 2448 bytes (raw data + P-W raw Subchannel)*/
	0xFF,			/*  4 -    Reserved				*/
	0xFF,			/*  5 -    Reserved				*/
	0xFF,			/*  6 -    Reserved				*/
	0xFF,			/*  7 -    Vendor specific			*/
	0x02,			/*  8 2048 bytes Mode 1 (ISO/IEC 10149)		*/
	0x03,			/*  9 2336 bytes Mode 2 (ISO/IEC 10149)		*/
	0xFF,			/* 10 2048 bytes Mode 2 (CD-ROM XA form 1)	*/
	0x04,			/* 11 2056 bytes Mode 2 (CD-ROM XA form 1)	*/
	0xFF,			/* 12 2324 bytes Mode 2 (CD-ROM XA form 2)	*/
	0x08,			/* 13 2332 bytes Mode 2 (CD-ROM XA 1/2+subhdr)	*/
	0xFF,			/* 14 -    Reserved				*/
	0xFF,			/* 15 -    Vendor specific			*/
};

LOCAL int
cw7501_open_track(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t *trackp;
{
	struct	scsi_mode_page_header	*mp;
	Uchar				mode[256];
	int				len = 0;
	int				page = 0x23;
	struct cw7501_mode_page_23	*xp23;

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

	if (!get_mode_params(scgp, page, _("Dummy/autopg information"),
			(Uchar *)mode, (Uchar *)0, (Uchar *)0, (Uchar *)0, &len)) {
		return (-1);
	}
	if (len == 0)
		return (-1);

	mp = (struct scsi_mode_page_header *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	xp23  = (struct cw7501_mode_page_23 *)mp;
	xp23->autopg = 1;
	if (!set_mode_params(scgp, _("Dummy/autopg page"), mode, len, 0, trackp->secsize))
		return (-1);

	/*
	 * Set write modes for next track.
	 */
	if (cw7501_set_mode(scgp, db2phys[trackp->dbtype & 0x0F],
			st2mode[trackp->sectype&ST_MASK] | (is_copy(trackp) ? TM_ALLOW_COPY : 0),
			0, is_scms(trackp) ? 1 : 0,
			trackp->trackno, 1, 0,
			/* write mode TAO */ 0x01) < 0)
		return (-1);

	return (0);
}


LOCAL int
cw7501_close_track(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	if (!is_tao(trackp) && !is_packet(trackp)) {
		return (0);
	}
	return (scsi_flush_cache(scgp, FALSE));
}

LOCAL int
cw7501_open_session(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	struct cw7501_mode_data		md;
	int				count;

	if (select_secsize(scgp, 2048) < 0)
		return (-1);

	/*
	 * Disable Auto Pregap when writing in SAO mode.
	 */
	if (!is_tao(trackp) && !is_packet(trackp)) {
		struct	scsi_mode_page_header	*mp;
		Uchar				mode[256];
		int				len = 0;
		int				page = 0x23;
		struct cw7501_mode_page_23	*xp23;

		if (!get_mode_params(scgp, page, _("Dummy/autopg information"),
				(Uchar *)mode, (Uchar *)0, (Uchar *)0, (Uchar *)0, &len)) {
			return (-1);
		}
		if (len == 0)
			return (-1);

		mp = (struct scsi_mode_page_header *)
			(mode + sizeof (struct scsi_mode_header) +
			((struct scsi_mode_header *)mode)->blockdesc_len);

		xp23  = (struct cw7501_mode_page_23 *)mp;
		xp23->autopg = 0;
		if (!set_mode_params(scgp, _("Dummy/autopg page"), mode, len, 0, trackp->secsize))
			return (-1);

		return (0);
	}

	/*
	 * Set Disk Type and Disk ID.
	 */
	fillbytes((caddr_t)&md, sizeof (md), '\0');

	count  = sizeof (struct scsi_mode_header) +
		sizeof (struct cw7501_mode_page_24);

	md.pagex.page24.p_code = 0x24;
	md.pagex.page24.p_len =  0x0A;
	md.pagex.page24.disktype = toc2sess[track_base(trackp)->tracktype & TOC_MASK];
	i_to_4_byte(md.pagex.page24.disk_id, 0x12345);

	return (mode_select(scgp, (Uchar *)&md, count, 0, scgp->inq->data_format >= 2));
}

LOCAL int
cw7501_fixate(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	if (!is_tao(trackp) && !is_packet(trackp)) {
		return (scsi_flush_cache(scgp, FALSE));
	}
	/*
	 * 0x00	Finalize Disk (not appendable)
	 * 0x01	Finalize Session (allow next session)
	 * 0x10	Finalize track (variable packet writing) - Must fluch cache before
	 */
	return (cw7501_finalize(scgp, 0, (track_base(trackp)->tracktype & TOCF_MULTI) ? 0x01 : 0x00));
}

/*--------------------------------------------------------------------------*/

LOCAL int
cw7501_gen_cue(trackp, vcuep, needgap)
	track_t	*trackp;
	void	*vcuep;
	BOOL	needgap;
{
	int	tracks = trackp->tracks;
	int	i;
	struct cw7501_cue	**cuep = vcuep;
	struct cw7501_cue	*cue;
	struct cw7501_cue	*cp;
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
		df = db2phys[trackp[i].dbtype & 0x0F];

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
		if (i == 0) {	/* Lead in */
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
	df = db2phys[trackp[tracks+1].dbtype & 0x0F];
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
	struct cw7501_cue *cp;	/* The target cue entry		*/
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
	if (scms != 0)
		dataform |= 0x10;
	cp->cs_dataform = dataform;
	cp->cs_extension = 0;
	cp->cs_min = to_bcd(mp->msf_min);
	cp->cs_sec = to_bcd(mp->msf_sec);
	cp->cs_frame = to_bcd(mp->msf_frame);
}

LOCAL int
cw7501_send_cue(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	struct cw7501_cue *cp;
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
	ret = cw7501_write_dao(scgp, (Uchar *)cp, ncue*8, disktype);
	scgp->silent--;
	free(cp);
	if (ret < 0) {
		errmsgno(EX_BAD, _("CUE sheet not accepted. Retrying with minimum pregapsize = 1.\n"));
		ncue = (*dp->cdr_gen_cue)(trackp, &cp, TRUE);
		ret  = cw7501_write_dao(scgp, (Uchar *)cp, ncue*8, disktype);
		free(cp);
	}
	if (ret >= 0 && lverbose) {
		gettimeofday(&stoptime, (struct timezone *)0);
		prtimediff(_("Write Lead-in time: "), &starttime, &stoptime);
	}
	return (ret);
}

/*--------------------------------------------------------------------------*/
LOCAL int
cw7501_rezero(scgp, reset, dwreset)
	SCSI	*scgp;
	int	reset;
	int	dwreset;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)0;
	scmd->size = 0;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = SC_REZERO_UNIT;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	scmd->cdb.cmd_cdb[5] |= reset ? 0x80 : 0;
	scmd->cdb.cmd_cdb[5] |= dwreset ? 0x40 : 0;

	scgp->cmdname = "cw7501 rezero";

	return (scg_cmd(scgp));
}


LOCAL int
cw7501_read_trackinfo(scgp, bp, count, track, mode)
	SCSI	*scgp;
	Uchar	*bp;
	int	count;
	int	track;
	int	mode;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t) scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)bp;
	scmd->size = count;
	scmd->flags = SCG_RECV_DATA | SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xE9;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.cmd_cdb[6] = track;
	g1_cdblen(&scmd->cdb.g1_cdb, count);
	scmd->cdb.cmd_cdb[9] = (mode & 3) << 6;

	scgp->cmdname = "cw7501 read_track_information";

	if (scg_cmd(scgp) < 0)
		return (-1);

	return (0);
}

LOCAL int
cw7501_write_dao(scgp, bp, len, disktype)
	SCSI	*scgp;
	Uchar	*bp;
	int	len;
	int	disktype;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)bp;
	scmd->size = len;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xE6;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.cmd_cdb[2] = disktype;
	g1_cdblen(&scmd->cdb.g1_cdb, len);

	scgp->cmdname = "cw7501 write_dao";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

/*
 * XXX CW-7501 also needs "control", so we need to make a different
 * XXX driver interface.
 */
LOCAL int
cw7501_reserve_track(scgp, len)
	SCSI	*scgp;
	unsigned long len;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xE7;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
/*	scmd->cdb.cmd_cdb[2] = control & 0x0F;*/
	i_to_4_byte(&scmd->cdb.cmd_cdb[5], len);

	scgp->cmdname = "cw7501 reserve_track";

	comerrno(EX_BAD, _("Control (as in set mode) missing.\n"));

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

LOCAL int
cw7501_set_mode(scgp, phys_form, control, subc, alt, trackno, tindex, packet_size, write_mode)
	SCSI	*scgp;
	int	phys_form;
	int	control;
	int	subc;
	int	alt;
	int	trackno;
	int	tindex;
	int	packet_size;
	int	write_mode;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xE2;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.cmd_cdb[2] = phys_form & 0x0F;
	scmd->cdb.cmd_cdb[3] = (control & 0x0F) << 4;
	scmd->cdb.cmd_cdb[3] |= subc ? 2 : 0;
	scmd->cdb.cmd_cdb[3] |= alt ? 1 : 0;
	scmd->cdb.cmd_cdb[4] = trackno;
	scmd->cdb.cmd_cdb[5] = tindex;
	i_to_3_byte(&scmd->cdb.cmd_cdb[6], packet_size);
	scmd->cdb.cmd_cdb[9] = (write_mode & 0x03) << 6;

	scgp->cmdname = "cw7501 set_mode";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

LOCAL int
cw7501_finalize(scgp, pad, fixed)
	SCSI	*scgp;
	int	pad;
	int	fixed;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->timeout = 8 * 60;		/* Needs up to 4 minutes */
	scmd->cdb.g1_cdb.cmd = 0xE3;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.cmd_cdb[1] = pad ? 1 : 0;
	scmd->cdb.cmd_cdb[8] = fixed & 0x03;

	scgp->cmdname = "cw7501 finalize";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}
