/* @(#)drv_dvdplus.c	1.65 12/03/16 Copyright 2003-2012 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)drv_dvdplus.c	1.65 12/03/16 Copyright 2003-2012 J. Schilling";
#endif
/*
 *	Copyright (c) 2003-2012 J. Schilling
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

/*
 *	DVD+R/RW device implementation for
 *	SCSI-3/mmc-3 conforming drives
 *
 * XXX Aktualisieren:
 *		Check recovery		- DUMMY
 *		Load Media		- OK
 *		Get Disktype		- Disk Status & Size (read ATIP -> DVD structure)
 *		Check Session ??	- Nicht vorhanden
 *		Check Disk size		- Nach Status & size + Next wr. Addr.
 *		Set Speed/Dummy		- Speed auf DVD -> dummy
 *		Open Session		- Set Write Parameter & ??? -> DAO
 *		LOOP
 *			Open Track	- Set Write Parameter ???
 *			Get Next wr. Addr.	-> DUMMY "0" ???
 *			Write Track Data	OK
 *			Close Track	- Flush Cache -> DUMMY
 *		END
 *		Fixate			- Close Track/Session -> Flush Cache
 *		Unload Media		- OK
 *
 *	Verbose levels:
 *			0		silent
 *			1		print laser log & track sizes
 *			2		print disk info & write parameters
 *			3		print log pages & dvd structure
 *
 *	Copyright (c) 2003-2009 J. Schilling
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

/*#define	DVDPLUS_DEBUG*/
#ifndef	DEBUG
#define	DEBUG
#endif
#include <schily/mconfig.h>

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>	/* Include sys/types.h to make off_t available */
#include <schily/standard.h>
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

#include "scsimmc.h"
#include "scsilog.h"
#include "mmcvendor.h"
#include "cdrecord.h"


extern	char	*driveropts;

extern	int	lverbose;
extern	int	xdebug;

#define	strbeg(s1, s2)	(strstr((s2), (s1)) == (s2))

LOCAL	cdr_t	*identify_dvdplus	__PR((SCSI *scgp, cdr_t *, struct scsi_inquiry *));
LOCAL	int	attach_dvdplus		__PR((SCSI *scgp, cdr_t *));
LOCAL	void	di_to_dstat		__PR((struct disk_info *dip, dstat_t *dsp));
LOCAL	int	init_dvdplus		__PR((SCSI *scgp, cdr_t *dp));
LOCAL	int	getdisktype_dvdplus	__PR((SCSI *scgp, cdr_t *dp));
LOCAL	int	prdiskstatus_dvdplus	__PR((SCSI *scgp, cdr_t *dp));
LOCAL	int	speed_select_dvdplus	__PR((SCSI *scgp, cdr_t *dp, int *speedp));
LOCAL	int	next_wr_addr_dvdplusrw	__PR((SCSI *scgp, track_t *trackp, long *ap));
LOCAL	int	next_wr_addr_dvdplusr	__PR((SCSI *scgp, track_t *trackp, long *ap));
LOCAL	int	open_track_dvdplus	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	long	rzone_size		__PR((track_t *trackp));
LOCAL	int	close_track_dvdplus	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
/*LOCAL	int	open_session_dvdplus	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));*/
LOCAL	int	fixate_dvdplusrw	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	fixate_dvdplusr		__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	BOOL	dvdplus_ricohbased	__PR((SCSI *scgp));
LOCAL	int	blank_dvdplus		__PR((SCSI *scgp, cdr_t *dp, long addr, int blanktype));
LOCAL	int	format_dvdplus		__PR((SCSI *scgp, cdr_t *dp, int fmtflags));
LOCAL	int	waitformat		__PR((SCSI *scgp, int secs));
LOCAL	int	stats_dvdplus		__PR((SCSI *scgp, cdr_t *dp));

EXPORT	int	format_unit		__PR((SCSI *scgp,
						void *fmt, int length,
						int list_format, int dgdl,
						int interlv, int pattern, int timeout));
LOCAL	int	set_p_layerbreak	__PR((SCSI *scgp, long tsize, Int32_t lbreak));

#define	CHANGE_SETTING	1
#define	DVD_PLUS_R	2
#define	DC_ERASE	4
LOCAL	int	ricoh_dvdsetting	__PR((SCSI *scgp, int erase_size, int flags, int immed));

#ifdef	__needed__
LOCAL	int	dummy_plextor		__PR((SCSI *scgp, int modecode));
#endif

/*
 * SCSI-3/mmc-3 conformant DVD+R or DVD+RW writer
 * Checks the current medium and then returns either
 * cdr_dvdplusr or cdr_dvdplusrw
 */
cdr_t	cdr_dvdplus = {
	0, 0, 0,
	CDR_DVD|CDR_SWABAUDIO,
	CDR2_NOCD,
	CDR_CDRW_ALL,
	WM_NONE,
	1000, 1000,
	"mmc_dvdplus",
	"generic SCSI-3/mmc-3 DVD+R/DVD+RW driver (checks media)",
	0,
	(dstat_t *)0,
	identify_dvdplus,
	attach_dvdplus,
	(int(*)__PR((SCSI *scgp, cdr_t *)))cmd_ill,	/* init */
	drive_getdisktype,
	no_diskstatus,
	scsi_load,
	scsi_unload,
	read_buff_cap,
	cmd_dummy,					/* recovery_needed */
	(int(*)__PR((SCSI *, cdr_t *, int)))cmd_dummy,	/* recover	*/
	(int(*)__PR((SCSI *scgp, cdr_t *, int *)))cmd_dummy,		/* Speed */
	select_secsize,
	(int(*)__PR((SCSI *scgp, track_t *, long *)))cmd_ill,	/* next_wr_addr	 */
	(int(*)__PR((SCSI *, Ulong)))cmd_ill,			/* reserve_track */
	scsi_cdr_write,
	(int(*)__PR((track_t *, void *, BOOL)))cmd_dummy,	/* gen_cue */
	no_sendcue,
	(int(*)__PR((SCSI *, cdr_t *, track_t *)))cmd_dummy,		/* leadin */
	(int(*)__PR((SCSI *scgp, cdr_t *, track_t *)))cmd_dummy,	/* open track */
	(int(*)__PR((SCSI *scgp, cdr_t *, track_t *)))cmd_dummy,	/* close track */
	(int(*)__PR((SCSI *scgp, cdr_t *, track_t *)))cmd_dummy,	/* open session */
	cmd_dummy,					/* close session */
	cmd_dummy,					/* abort */
	read_session_offset,
	(int(*)__PR((SCSI *scgp, cdr_t *, track_t *)))cmd_dummy,	/* fixation */
	cmd_dummy,					/* stats	*/
	blank_dummy,
	format_dummy,
	(int(*)__PR((SCSI *, caddr_t, int, int)))NULL,	/* no OPC	*/
	cmd_dummy,					/* opt1		*/
	cmd_dummy,					/* opt2		*/
};

/*
 * SCSI-3/mmc-3 conformant DVD+R writer
 */
cdr_t	cdr_dvdplusr = {
	0, 0, 0,
	CDR_DVD|CDR_SWABAUDIO,
	CDR2_NOCD,
	CDR_CDRW_ALL,
	WM_SAO,
	1000, 1000,
	"mmc_dvdplusr",
	"generic SCSI-3/mmc-3 DVD+R driver",
	0,
	(dstat_t *)0,
	identify_dvdplus,
	attach_dvdplus,
	init_dvdplus,
	getdisktype_dvdplus,
	prdiskstatus_dvdplus,
	scsi_load,
	scsi_unload,
	read_buff_cap,
	cmd_dummy,					/* recovery_needed */
	(int(*)__PR((SCSI *, cdr_t *, int)))cmd_dummy,	/* recover	*/
	speed_select_dvdplus,
	select_secsize,
	next_wr_addr_dvdplusr,
	(int(*)__PR((SCSI *, Ulong)))cmd_ill,		/* reserve_track */
	scsi_cdr_write,
	(int(*)__PR((track_t *, void *, BOOL)))cmd_dummy,	/* gen_cue */
	no_sendcue,
	(int(*)__PR((SCSI *, cdr_t *, track_t *)))cmd_dummy, /* leadin */
	open_track_dvdplus,
	close_track_dvdplus,
	(int(*)__PR((SCSI *scgp, cdr_t *, track_t *)))cmd_dummy,	/* open session */
	cmd_dummy,					/* close session */
	cmd_dummy,					/* abort	*/
	read_session_offset,
	fixate_dvdplusr,
	stats_dvdplus,
	blank_dvdplus,
	format_dummy,
	(int(*)__PR((SCSI *, caddr_t, int, int)))NULL,	/* no OPC	*/
	cmd_dummy,					/* opt1		*/
	cmd_dummy,					/* opt2		*/
};

/*
 * SCSI-3/mmc-3 conformant DVD+RW writer
 */
cdr_t	cdr_dvdplusrw = {
	0, 0, 0,
	CDR_DVD|CDR_SWABAUDIO,
	CDR2_NOCD,
	CDR_CDRW_ALL,
	WM_SAO,
	1000, 1000,
	"mmc_dvdplusrw",
	"generic SCSI-3/mmc-3 DVD+RW driver",
	0,
	(dstat_t *)0,
	identify_dvdplus,
	attach_dvdplus,
	init_dvdplus,
	getdisktype_dvdplus,
	prdiskstatus_dvdplus,
	scsi_load,
	scsi_unload,
	read_buff_cap,
	cmd_dummy,					/* recovery_needed */
	(int(*)__PR((SCSI *, cdr_t *, int)))cmd_dummy,	/* recover	*/
	speed_select_dvdplus,
	select_secsize,
	next_wr_addr_dvdplusrw,
	(int(*)__PR((SCSI *, Ulong)))cmd_ill,		/* reserve_track */
	scsi_cdr_write,
	(int(*)__PR((track_t *, void *, BOOL)))cmd_dummy,	/* gen_cue */
	no_sendcue,
	(int(*)__PR((SCSI *, cdr_t *, track_t *)))cmd_dummy, /* leadin */
	open_track_dvdplus,
	close_track_dvdplus,
	(int(*)__PR((SCSI *scgp, cdr_t *, track_t *)))cmd_dummy,	/* open session */
	cmd_dummy,					/* close session */
	cmd_dummy,					/* abort	*/
	read_session_offset,
	fixate_dvdplusrw,
	stats_dvdplus,
	blank_dvdplus,
	format_dvdplus,
	(int(*)__PR((SCSI *, caddr_t, int, int)))NULL,	/* no OPC	*/
	cmd_dummy,					/* opt1		*/
	cmd_dummy,					/* opt2		*/
};

LOCAL cdr_t *
identify_dvdplus(scgp, dp, ip)
	SCSI			*scgp;
	cdr_t			*dp;
	struct scsi_inquiry	*ip;
{
	Uchar	mode[0x100];
	struct	cd_mode_page_2A *mp;
	int	profile;

	if (ip->type != INQ_WORM && ip->type != INQ_ROMD)
		return ((cdr_t *)0);

	allow_atapi(scgp, TRUE); /* Try to switch to 10 byte mode cmds */

	scgp->silent++;
	mp = mmc_cap(scgp, mode); /* Get MMC capabilities */
	scgp->silent--;
	if (mp == NULL)
		return (NULL);	/* Pre SCSI-3/mmc drive		*/

	/*
	 * At this point we know that we have a SCSI-3/mmc compliant drive.
	 * Unfortunately ATAPI drives violate the SCSI spec in returning
	 * a response data format of '1' which from the SCSI spec would
	 * tell us not to use the "PF" bit in mode select. As ATAPI drives
	 * require the "PF" bit to be set, we 'correct' the inquiry data.
	 *
	 * XXX xxx_identify() should not have any side_effects ??
	 */
	if (ip->data_format < 2)
		ip->data_format = 2;

	profile = get_curprofile(scgp);
	if (xdebug)
		printf(_("Current profile: 0x%04X\n"), profile);

	if (profile == 0x001A)
		dp = &cdr_dvdplusrw;
	if ((profile == 0x001B) || (profile == 0x002B))
		dp = &cdr_dvdplusr;
#ifdef	XX
	if (!dvd)			/* SCSI-3/mmc CD drive		*/
		return (NULL);
#endif

	return (dp);
}

LOCAL int
attach_dvdplus(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	Uchar	mode[0x100];
	struct	cd_mode_page_2A *mp;
	struct	ricoh_mode_page_30 *rp = NULL;
	Ulong	xspeed;
	Ulong	mp2Aspeed;


	allow_atapi(scgp, TRUE); /* Try to switch to 10 byte mode cmds */

	scgp->silent++;
	mp = mmc_cap(scgp, NULL); /* Get MMC capabilities in allocated mp */
	scgp->silent--;
	if (mp == NULL)
		return (-1);	/* Pre SCSI-3/mmc drive		*/

	dp->cdr_cdcap = mp;	/* Store MMC cap pointer	*/

	/*
	 * XXX hier sollte drive max write speed & drive cur write speed
	 * XXX gesetzt werden.
	 */
	dp->cdr_dstat->ds_dr_max_rspeed = a_to_u_2_byte(mp->max_read_speed)/1385;
	if (dp->cdr_dstat->ds_dr_max_rspeed == 0)
		dp->cdr_dstat->ds_dr_max_rspeed = 47;
	dp->cdr_dstat->ds_dr_cur_rspeed = a_to_u_2_byte(mp->cur_read_speed)/1385;
	if (dp->cdr_dstat->ds_dr_cur_rspeed == 0)
		dp->cdr_dstat->ds_dr_cur_rspeed = 47;

	dp->cdr_dstat->ds_dr_max_wspeed = a_to_u_2_byte(mp->max_write_speed)/1385;
	if (mp->p_len >= 28)
		dp->cdr_dstat->ds_dr_cur_wspeed = a_to_u_2_byte(mp->v3_cur_write_speed)/1385;
	else
		dp->cdr_dstat->ds_dr_cur_wspeed = a_to_u_2_byte(mp->cur_write_speed)/1385;

	/*
	 * NEC drives incorrectly return CD speed values in mode page 2A.
	 * Try MMC3 get performance in hope that values closer to DVD speeds
	 * are always more correct than what is found in mode page 2A.
	 */
	xspeed = 0;
	scsi_get_perf_maxspeed(scgp, NULL, &xspeed, NULL);

	mp2Aspeed = a_to_u_2_byte(mp->max_write_speed);

	if (lverbose > 2) {
		printf(_("max page 2A speed %lu (%lux), max perf speed %lu (%lux)\n"),
			mp2Aspeed, mp2Aspeed/1385,
			xspeed, xspeed/1385);
	}

	if ((is_cdspeed(mp2Aspeed) && !is_cdspeed(xspeed)) ||
	    (mp2Aspeed < 10000 && xspeed > 10000)) {
		dp->cdr_dstat->ds_dr_max_wspeed = xspeed/1385;
		xspeed = 0;
		scsi_get_perf_curspeed(scgp, NULL, &xspeed, NULL);
		if (xspeed > 0)
			dp->cdr_dstat->ds_dr_cur_wspeed = xspeed / 1385;
	}

	if (dp->cdr_speedmax > dp->cdr_dstat->ds_dr_max_wspeed)
		dp->cdr_speedmax = dp->cdr_dstat->ds_dr_max_wspeed;

	if (dp->cdr_speeddef > dp->cdr_speedmax)
		dp->cdr_speeddef = dp->cdr_speedmax;

	rp = get_justlink_ricoh(scgp, mode);

	if (mp->p_len >= 28)
		dp->cdr_flags |= CDR_MMC3;
	if (get_curprofile(scgp) >= 0)
		dp->cdr_flags |= CDR_MMC3;
	if (mp->p_len >= 24)
		dp->cdr_flags |= CDR_MMC2;
	dp->cdr_flags |= CDR_MMC;

	if (mp->loading_type == LT_TRAY)
		dp->cdr_flags |= CDR_TRAYLOAD;
	else if (mp->loading_type == LT_CADDY)
		dp->cdr_flags |= CDR_CADDYLOAD;

	if (mp->BUF != 0)
		dp->cdr_flags |= CDR_BURNFREE;

	check_writemodes_mmc(scgp, dp);
	/*
	 * To avoid that silly people try to call cdrecord will write modes
	 * that are illegal for DVDs, we clear anything that does now work.
	 */
	dp->cdr_flags &= ~(CDR_RAW|CDR_RAW16|CDR_RAW96P|CDR_RAW96R|CDR_SRAW96P|CDR_SRAW96R);
	dp->cdr_flags &= ~(CDR_TAO);

	if (scgp->inq != NULL) {
		if (strbeg("PIONEER", scgp->inq->inq_vendor_info)) {
			if (strbeg("DVD-RW  DVR-103", scgp->inq->inq_prod_ident) ||
			    strbeg("DVD-R DVD-R7322", scgp->inq->inq_prod_ident)) {
				mp->BUF = 1;
			}
		}
	}
	if (mp->BUF != 0) {
		dp->cdr_flags |= CDR_BURNFREE;
	} else if (rp) {
		if ((dp->cdr_cmdflags & F_DUMMY) && rp->TWBFS && rp->BUEFS)
			dp->cdr_flags |= CDR_BURNFREE;

		if (rp->BUEFS)
			dp->cdr_flags |= CDR_BURNFREE;
	}

	if (rp && rp->AWSCS)
		dp->cdr_flags |= CDR_FORCESPEED;


	if ((dp->cdr_flags & (CDR_SAO)) != (CDR_SAO)) {
		/*
		 * XXX Ist dies ueberhaupt noch notwendig seit wir nicht
		 * XXX mehr CDR_TAO vorgaukeln muessen?
		 *
		 * Das Panasonic DVD-R mag check_writemodes_mmc() nicht
		 * hilft das vielleicht?
		 */
		dp->cdr_flags |= CDR_SAO;
	}

	if (driveropts != NULL) {
		char	*p;

		if (strcmp(driveropts, "help") == 0) {
			mmc_opthelp(scgp, dp, 0);
		}

		p = hasdrvopt(driveropts, "burnfree");
		if (p == NULL)
			p = hasdrvopt(driveropts, "burnproof");
		if (p != NULL && (dp->cdr_flags & CDR_BURNFREE) != 0) {
			if (*p == '1') {
				dp->cdr_dstat->ds_cdrflags |= RF_BURNFREE;
			} else if (*p == '0') {
				dp->cdr_dstat->ds_cdrflags &= ~RF_BURNFREE;
			}
		}

		p = hasdrvopt(driveropts, "forcespeed");
		if (p != NULL && *p == '1' && (dp->cdr_flags & CDR_FORCESPEED) != 0) {
			dp->cdr_dstat->ds_cdrflags |= RF_FORCESPEED;
		}

		p = hasdrvoptx(driveropts, "layerbreak", 0);
		if (p != NULL) {
			char	*ep;
			Llong	ll;
			Int32_t	lb;

			ep = astoll(p, &ll);
			lb = ll;
			if ((*ep != '\0' && *ep != ',') || *p == '\0' ||
			    ll <= 0 || ll != lb) {
				errmsgno(EX_BAD,
					_("Bad layer break value '%s'.\n"), p);
				return (-1);
			}
			dp->cdr_dstat->ds_layer_break = lb;
		}
		if (dp->cdr_dstat->ds_layer_break >= 0 &&
		    get_curprofile(scgp) != 0x2B) {
			errmsgno(EX_BAD,
			_("Cannot set layer break on this drive/medium.\n"));
			return (-1);
		}
		if (dp->cdr_dstat->ds_layer_break != -1 &&
		    dp->cdr_dstat->ds_layer_break !=
		    roundup(dp->cdr_dstat->ds_layer_break, 16)) {
			errmsgno(EX_BAD,
			_("Layer break at %d is not properly aligned.\n"),
				dp->cdr_dstat->ds_layer_break);
			return (-1);
		}
	}

	/*
	 * Raise the default timeout.
	 * The first write takes a long time as it writes the lead in.
	 */
	if (scgp->deftimeout < 100)
		scg_settimeout(scgp, 100);	/* 1:40			*/


	return (0);
}

LOCAL void
di_to_dstat(dip, dsp)
	struct disk_info	*dip;
	dstat_t	*dsp;
{
	dsp->ds_diskid = a_to_u_4_byte(dip->disk_id);

	dsp->ds_flags |= DSF_NOCD|DSF_DVD;	/* This is a DVD */

	if (dip->did_v)
		dsp->ds_flags |= DSF_DID_V;
	dsp->ds_disktype = dip->disk_type;
	dsp->ds_diskstat = dip->disk_status;
	dsp->ds_sessstat = dip->sess_status;
	if (dip->erasable)
		dsp->ds_flags |= DSF_ERA;

	dsp->ds_trfirst	   = dip->first_track;
	dsp->ds_trlast	   = dip->last_track_ls;
	dsp->ds_trfirst_ls = dip->first_track_ls;

#ifdef	nono
	/*
	 * On DVD systems, there is no lead out start time
	 * in the disk info because there is no time based data.
	 */
	dsp->ds_maxblocks = msf_to_lba(dip->last_lead_out[1],
					dip->last_lead_out[2],
					dip->last_lead_out[3], TRUE);
#endif
}

LOCAL int
init_dvdplus(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	return (speed_select_dvdplus(scgp, dp, NULL));
}

LOCAL int
getdisktype_dvdplus(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
extern	char	*buf;
	dstat_t	*dsp = dp->cdr_dstat;
	struct disk_info *dip;
	Uchar	mode[0x100];
	struct rzone_info *rp;
	struct dvd_structure_00 *sp;
	int	len;
	BOOL	did_dummy = FALSE;
	BOOL	did_reload = FALSE;
	int	profile;
	Int32_t	maxblocks;
	Ulong	end_lba;

/*	if (lverbose > 0)*/
/*		print_laserlog(scgp);*/

/*	if (lverbose > 2)*/
/*		print_logpages(scgp);*/


	if (dsp->ds_type == DST_UNKNOWN) {
		profile = get_curprofile(scgp);
		if (profile >= 0)
			dsp->ds_type = profile;
	}

	if ((dp->cdr_dstat->ds_cdrflags & RF_PRATIP) != 0) {
		if (((dsp->ds_cdrflags & (RF_WRITE|RF_BLANK)) == 0) ||
				lverbose > 1) {
			extern	void print_dvd_info __PR((SCSI *_scgp));

			/*
			 * Das DVD Medieninfo ist so lang, daﬂ wir es
			 * beim Schreiben mit -v noch nicht ausgeben.
			 */
			print_dvd_info(scgp);
		}
	}

again:
	dip = (struct disk_info *)buf;
	if (get_diskinfo(scgp, dip, sizeof (*dip)) < 0)
		return (-1);

	/*
	 * Check for non writable disk first.
	 */
#ifdef	DVDPLUS_DEBUG
error("DISK STATUS %X\n", dip->disk_status);
#endif
	if (dip->disk_status == DS_COMPLETE &&
			(dsp->ds_cdrflags & (RF_WRITE|RF_BLANK)) == RF_WRITE) {

		profile = get_curprofile(scgp);
		if (profile == 0x001A) {	/* This is a DVD+RW */
			if (dp->cdr_cmdflags & F_FIX)
				return (0);
		}


		if (!did_dummy) {
			int	xspeed = 0xFFFF;
			int	oflags = dp->cdr_cmdflags;

			/*
			 * Try to clear the dummy bit to reset the virtual
			 * drive status. Not all drives support it even though
			 * it is mentioned in the MMC standard.
			 */
			if (lverbose)
				printf(_("Trying to clear drive status.\n"));

			dp->cdr_cmdflags &= ~F_DUMMY;
			speed_select_dvdplus(scgp, dp, &xspeed);
			dp->cdr_cmdflags = oflags;
			scgp->silent++;
			rezero_unit(scgp);
			scgp->silent--;
			did_dummy = TRUE;
			goto again;
		}
		if (!did_reload && profile == 0x001A) {	/* This is a DVD+RW */
			scgp->silent++;
			len = read_scsi(scgp, buf, 0, 1);
			scgp->silent--;
			if (len < 0) {
				/*
				 * DVD+RW "complete" but unreadable
				 * Trying to clear drive status did not work...
				 * This happens if the DVD+RW media was erased
				 * by the Ricoh Vendor Unique command
				 */
				reload_media(scgp, dp);
				did_reload = TRUE;
				goto again;
			}
		}
		/*
		 * Trying to clear drive status did not work...
		 * XXX This most likely makes no sense with DVD+R
		 */
/*		reload_media(scgp, dp);*/
	}
	if (get_diskinfo(scgp, dip, sizeof (*dip)) < 0)
		return (-1);
	di_to_dstat(dip, dsp);

#ifdef	DVDPLUS_DEBUG
error("dtype %d era %d sess stat %d disk stat %d\n",
dip->dtype,
dip->erasable,
dip->sess_status,
dip->disk_status);
error("Dirty %d Format status %d\n", dip->dbit, dip->bg_format_stat);
error("disk type %X\n",
dip->disk_type);
{
	long nwa = -1;

	next_wr_addr_dvdplusr(scgp, (track_t *)NULL, &nwa);
	error("NWA: %ld\n", nwa);
}
#endif


	profile = get_curprofile(scgp);
	if (profile == 0x001A) {
		dsp->ds_flags |= DSF_DVD_PLUS_RW;	/* This is a DVD+RW */
		if (dip->disk_status == DS_EMPTY) {	/* Unformatted	    */
			if (dip->disk_type != SES_UNDEF) {	/* Not a CD */
				printf(_("WARNING: Drive returns illegal Disk type %s.\n"),
							get_ses_type(dip->disk_type));
			}
			dsp->ds_flags |= DSF_NEED_FORMAT;
			if ((dp->cdr_dstat->ds_cdrflags & RF_PRATIP) != 0)
				print_format_capacities(scgp);
			return (0);
		}
	} else if (profile == 0x001B) {
		dsp->ds_flags |= DSF_DVD_PLUS_R;	/* This is a DVD+R */
	}
#ifdef	DVDPLUS_DEBUG
error("profile %X dsp->ds_flags 0x%X\n", profile, dsp->ds_flags);
#endif

	/*
	 * This information is based on a logical recording zone
	 * and may not always be correct.
	 * XXX Check this if we want to support anything else but
	 * XXX one data track on DAO mode. The current firmware
	 * XXX of the recorder supports only this method but future
	 * XXX releases may support more.
	 */
	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	rp = (struct rzone_info *)mode;
	read_rzone_info(scgp, (caddr_t)rp, sizeof (struct rzone_info));

	if ((dp->cdr_dstat->ds_cdrflags & RF_PRATIP) != 0) {
		if (((dsp->ds_cdrflags & (RF_WRITE|RF_BLANK)) == 0) ||
				lverbose > 1) {
			przone(rp);
			print_format_capacities(scgp);
		}
	}
#define	needed_for_pioneer_a06
#ifdef	needed_for_pioneer_a06
	/*
	 * Old code used with the S101 seems to be needed for DVD+RW
	 * and the Pioneer A06 if the medium is not yet fully formatted.
	 */
	dsp->ds_maxblocks = a_to_u_4_byte(rp->rzone_size);
#ifdef	DVDPLUS_DEBUG
error("MAXBLO %d from rzone size\n", dsp->ds_maxblocks);
#endif
	if (dsp->ds_maxblocks == 0)
#endif
	dsp->ds_maxblocks = a_to_u_4_byte(rp->free_blocks);
#ifdef	DVDPLUS_DEBUG
error("MAXBLO %d\n", dsp->ds_maxblocks);
error("MAXBLO %d from free_blocks\n", (int)a_to_u_4_byte(rp->free_blocks));
#endif
	if (rp->nwa_v)
		dsp->ds_maxblocks += a_to_u_4_byte(rp->next_recordable_addr);
#ifdef	DVDPLUS_DEBUG
error("NWAv %d Next rec addr %d\n", rp->nwa_v, (int)a_to_u_4_byte(rp->next_recordable_addr));
#endif
	maxblocks = dsp->ds_maxblocks;

	/*
	 * XXX this was: if (dip->disk_status == DS_EMPTY)
	 * The '_NEC    ' 'DVD_RW ND-3500AG' may return written DVD+RW with
	 * status DS_COMPLETE although they may be overwritten.
	 */
	if (dip->disk_status == DS_COMPLETE && profile != 0x001A)
		return (drive_getdisktype(scgp, dp));

	/*
	 * This information is based on the physical pre recorded information.
	 * First try to find the len supported by the actual drive.
	 */
	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	scgp->silent++;
	if (read_dvd_structure(scgp, (caddr_t)mode, 2, 0, 0, 0, 0) < 0) {
		scgp->silent--;
		if (lverbose > 2)
			errmsgno(EX_BAD, _("Cannot read DVD structure 00.\n"));
		return (drive_getdisktype(scgp, dp));
	}
	scgp->silent--;
	len = a_to_u_2_byte(mode);
	len += 2;			/* Data len is not included */

	if (len > sizeof (struct dvd_structure_00)) {
		len = sizeof (struct dvd_structure_00);
		/*
		 * The ACARD TECH AEC-7720 ATAPI<->SCSI adaptor
		 * chokes if we try to transfer odd byte counts (rounds up to
		 * even byte counts and thus causes a DMA overflow and a
		 * bus reset), so make the byte count even.
		 */
		len += 1;
		len &= ~1;
	}
	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	sp = (struct dvd_structure_00 *)mode;
	if (read_dvd_structure(scgp, (caddr_t)sp, len, 0, 0, 0, 0) < 0)
		return (drive_getdisktype(scgp, dp));

/*	if (lverbose > 1)*/
/*		print_dvd00(sp);*/
	/*
	 * Bei Pioneer ist Phys End ist nur bei dem S101 != 0.
	 * Bei Panasonic ist Phys End == Phys Start.
	 */
	if ((profile != 0x001A) &&
	    (dp->cdr_cmdflags & F_MSINFO) == 0 &&
	    (a_to_u_3_byte(sp->phys_end) != 0) &&
			(dsp->ds_maxblocks !=
			(long)(a_to_u_3_byte(sp->phys_end) - a_to_u_3_byte(sp->phys_start) + 1))) {
#ifdef	__nonono__
		printf("WARNING: Phys disk size %ld differs from rzone size %ld! Prerecorded disk?\n",
			(long)(a_to_u_3_byte(sp->phys_end) - a_to_u_3_byte(sp->phys_start) + 1),
			(long)dsp->ds_maxblocks);
		printf("WARNING: Phys start: %ld Phys end %ld\n",
			(long)a_to_u_3_byte(sp->phys_start),
			(long)a_to_u_3_byte(sp->phys_end));
#endif
		/*
		 * Workaround for some drive media combinations.
		 * At least the drive	'HL-DT-ST' 'DVD-RAM GH22NP20' '1.02'
		 * does not report maxblocks correctly with 'CMC MAG' 'M01'
		 * DVD+R media.
		 * Use the information from ADIP instead.
		 */
#ifdef	DVDPLUS_DEBUG
error("MAXBLOx1 %d\n", dsp->ds_maxblocks);
#endif
		if ((long)dsp->ds_maxblocks == 0) {
			printf(_("WARNING: Drive returns zero media size. Using media size from ADIP.\n"));
			dsp->ds_maxblocks = a_to_u_3_byte(sp->phys_end) - a_to_u_3_byte(sp->phys_start) + 1;
		}
	}
#ifdef	DVDPLUS_DEBUG
error("MAXBLOx %d\n", dsp->ds_maxblocks);
error("phys start %d phys end %d\n", a_to_u_3_byte(sp->phys_start), a_to_u_3_byte(sp->phys_end));
error("MAXBLO %d from phys end - phys start\n", (int)(a_to_u_3_byte(sp->phys_end) - a_to_u_3_byte(sp->phys_start) + 1));
#endif

	end_lba = 0L;
	scsi_get_perf_maxspeed(scgp, (Ulong *)NULL, (Ulong *)NULL, &end_lba);
#ifdef	DVDPLUS_DEBUG
error("end_lba: %lu\n", end_lba);
#endif
	/*
	 * XXX Note that end_lba is unsigned and dsp->ds_maxblocks is signed.
	 */
	if ((Int32_t)end_lba > dsp->ds_maxblocks) {
		if (maxblocks == 0)
			printf(_("WARNING: Drive returns zero media size, correcting.\n"));
		dsp->ds_maxblocks = end_lba + 1;
	}

	return (drive_getdisktype(scgp, dp));
}

LOCAL int
prdiskstatus_dvdplus(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	return (prdiskstatus(scgp, dp, FALSE));
}


LOCAL int
speed_select_dvdplus(scgp, dp, speedp)
	SCSI	*scgp;
	cdr_t	*dp;
	int	*speedp;
{
	Uchar	mode[0x100];
	Uchar	moder[0x100];
	int	len;
	struct	cd_mode_page_05 *mp;
	struct	ricoh_mode_page_30 *rp = NULL;
	int	val;
	Ulong	ul;
	BOOL	forcespeed = FALSE;
	BOOL	dummy = (dp->cdr_cmdflags & F_DUMMY) != 0;
	int	curspeed = 1;

	if (speedp)
		curspeed = *speedp;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	if (!get_mode_params(scgp, 0x05, _("CD write parameter"),
			mode, (Uchar *)0, (Uchar *)0, (Uchar *)0, &len))
		return (-1);
	if (len == 0)
		return (-1);

	mp = (struct cd_mode_page_05 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);
#ifdef	DEBUG
	if (lverbose > 1)
		scg_prbytes("CD write parameter:", (Uchar *)mode, len);
#endif


	if (dummy != 0) {
		errmsgno(EX_BAD, _("DVD+R/DVD+RW has no -dummy mode.\n"));
		return (-1);
	}
	if (get_curprofile(scgp) == 0x001A) {	/* This is a DVD+RW */
		if (dp->cdr_cmdflags & F_FIX)
			return (0);
	}
	mp->test_write = dummy != 0;
	/*
	 * Set default values:
	 * Write type = 02 (disk at once)
	 * Track mode = 00 Reserved on Pioneer DVR-S101
	 * Data block type = 00 Reserved on Pioneer DVR-S101
	 * Session format = 00 Reserved on Pioneer DVR-S101
	 * XXX DVR-S101 uses ls_v and link size violating
	 * XXX the current MMC2 spec.
	 */
	mp->write_type = WT_SAO;

#ifdef	DEBUG
	if (lverbose > 1)
		scg_prbytes("CD write parameter:", (Uchar *)mode, len);
#endif
	if (!set_mode_params(scgp, _("CD write parameter"), mode, len, 0, -1)) {
		return (-1);
	}

	/*
	 * Neither set nor get speed.
	 */
	if (speedp == 0)
		return (0);


	rp = get_justlink_ricoh(scgp, moder);
	if ((dp->cdr_flags & CDR_FORCESPEED) != 0) {
		forcespeed = rp && rp->AWSCD != 0;
	}

	if (lverbose && (dp->cdr_flags & CDR_FORCESPEED) != 0)
		printf(_("Forcespeed is %s.\n"), forcespeed?_("ON"):_("OFF"));

	if (!forcespeed && (dp->cdr_dstat->ds_cdrflags & RF_FORCESPEED) != 0) {
		printf(_("Turning forcespeed on\n"));
		forcespeed = TRUE;
	}
	if (forcespeed && (dp->cdr_dstat->ds_cdrflags & RF_FORCESPEED) == 0) {
		printf(_("Turning forcespeed off\n"));
		forcespeed = FALSE;
	}
	if ((dp->cdr_flags & CDR_FORCESPEED) != 0) {

		if (rp) {
			rp->AWSCD = forcespeed?1:0;
			set_mode_params(scgp, _("Ricoh Vendor Page"), moder, moder[0]+1, 0, -1);
			rp = get_justlink_ricoh(scgp, moder);
		}
	}

	/*
	 * DVD single speed is 1385 kB/s
	 * Rounding down is guaranteed.
	 */
	val = curspeed*1390;
	if (val > 0x7FFFFFFF)
		val = 0x7FFFFFFF;
	if (dp->cdr_flags & CDR_MMC3) {
		if (speed_select_mdvd(scgp, -1, val) < 0)
			errmsgno(EX_BAD, _("MMC-3 speed select did not work.\n"));
	} else {
		if (val > 0xFFFF)
			val = 0xFFFF;
		scgp->silent++;
		if (scsi_set_speed(scgp, -1, val, ROTCTL_CLV) < 0) {
			/*
			 * Don't complain if it does not work,
			 * DVD drives may not have speed setting.
			 * The DVR-S101 and the DVR-S201 difinitely
			 * don't allow to set the speed.
			 */
		}
		scgp->silent--;
	}

	scgp->silent++;
	if (scsi_get_speed(scgp, 0, &val) >= 0) {
		if (val > 0) {
			curspeed = val / 1385;
			*speedp = curspeed;
		}
	}
	/*
	 * NEC drives incorrectly return CD speed values in mode page 2A.
	 * Try MMC3 get performance in hope that values closer to DVD speeds
	 * are always more correct than what is found in mode page 2A.
	 */
	ul = 0;
	if (scsi_get_perf_curspeed(scgp, NULL, &ul, NULL) >= 0) {
		if (is_cdspeed(val) && !is_cdspeed(ul)) {
			curspeed = ul / 1385;
			*speedp = curspeed;
		}
	}
	scgp->silent--;
	return (0);
}
/*--------------------------------------------------------------------------*/

LOCAL long	dvd_next_addr;

LOCAL int
next_wr_addr_dvdplusrw(scgp, trackp, ap)
	SCSI	*scgp;
	track_t	*trackp;
	long	*ap;
{
	if (trackp == 0 || trackp->track <= 1) {
		dvd_next_addr = 0;
	}
	if (ap)
		*ap = dvd_next_addr;
	return (0);
}

LOCAL int
next_wr_addr_dvdplusr(scgp, trackp, ap)
	SCSI	*scgp;
	track_t	*trackp;
	long	*ap;
{
	struct disk_info	di;
	struct rzone_info	rz;
	int			tracks;
	long			next_addr = -1;

	if (trackp == 0) {
		fillbytes((caddr_t)&di, sizeof (di), '\0');
		if (get_diskinfo(scgp, &di, sizeof (di)) < 0)
			return (-1);

		tracks = di.last_track_ls + di.last_track_ls_msb * 256;
		fillbytes((caddr_t)&rz, sizeof (rz), '\0');
		if (get_trackinfo(scgp, (caddr_t)&rz, TI_TYPE_TRACK, tracks, sizeof (rz)) < 0)
			return (-1);
		if (!rz.nwa_v)
			return (-1);
		next_addr = a_to_4_byte(rz.next_recordable_addr);
		if (ap)
			*ap = next_addr;
		return (0);
	}
	if (trackp->track <= 1) {
		/*
		 * XXX This is a workaround for the filesize > 2GB problem.
		 * XXX Check this if we support more than one track DAO
		 * XXX or if we give up this hack in favour of real 64bit
		 * XXX filesize support.
		 */
		fillbytes((caddr_t)&rz, sizeof (rz), '\0');
		read_rzone_info(scgp, (caddr_t)&rz, sizeof (struct rzone_info));
		dvd_next_addr = a_to_4_byte(rz.next_recordable_addr);
#ifdef	DVDPLUS_DEBUG
error("NWA: %ld valid: %d\n", dvd_next_addr, rz.nwa_v);
#endif
		if (lverbose > 1)
			printf(_("next writable addr: %ld valid: %d\n"), dvd_next_addr, rz.nwa_v);
	}
	if (ap)
		*ap = dvd_next_addr;
	return (0);
}

LOCAL int
open_track_dvdplus(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t *trackp;
{
	Uchar	mode[0x100];
	int	len;
	long	sectors;
	struct	cd_mode_page_05 *mp;
	int	profile;

	if (trackp->track > 1)	/* XXX Hack to make one 'track' from several */
		return (0);	/* XXX files in Disk at once mode only.	    */

	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	if (!get_mode_params(scgp, 0x05, _("CD write parameter"),
			mode, (Uchar *)0, (Uchar *)0, (Uchar *)0, &len))
		return (-1);
	if (len == 0)
		return (-1);

	mp = (struct cd_mode_page_05 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	/*
	 * XXX as long as the Pioneer DVR-S101 only supports a single
	 * XXX data track in DAO mode,
	 * XXX do not set:
	 * XXX track_mode
	 * XXX copy
	 * XXX dbtype
	 *
	 * Track mode = 00 Reserved on Pioneer DVR-S101
	 * Data block type = 00 Reserved on Pioneer DVR-S101
	 * Session format = 00 Reserved on Pioneer DVR-S101
	 * XXX DVR-S101 uses ls_v and link size violating
	 * XXX the current MMC2 spec.
	 */
	/* XXX look into drv_mmc.c for re-integration of above settings */

#ifdef	DEBUG
	if (lverbose > 1)
		scg_prbytes("CD write parameter:", (Uchar *)mode, len);
#endif
	if (!set_mode_params(scgp, _("CD write parameter"), mode, len, 0, trackp->secsize))
		return (-1);

	/*
	 * Compile vitual track list
	 */
	sectors = rzone_size(trackp);
#ifdef	__needed_for_dvdplus__
	if (sectors < 0)
		return (-1);
#endif
	profile = get_curprofile(scgp);
	if (profile == 0x2B) {
		if (set_p_layerbreak(scgp, sectors,
				dp->cdr_dstat->ds_layer_break) < 0) {
			return (-1);
		}
	}
#ifdef	__needed_for_dvdplus__
	return (reserve_tr_rzone(scgp, sectors));
#else
	return (0);
#endif
}

/*
 * XXX rzone_size(trackp) muﬂ aufgerufen werden, daher muﬂ
 * XXX auch f¸r DVD+RW eine non-dummy open_track() Funktion da sein.
 */
/*
 * XXX Hack to make one 'track' from several
 * XXX files in Disk at once mode only.
 * XXX Calculate track size and reserve rzone.
 */
LOCAL long
rzone_size(trackp)
	track_t *trackp;	/* Called with &track[1] */
{
	int	i;
	BOOL	vtracks = FALSE;
	long	sectors = 0L;
	Llong	ttrsize = 0L;
	Llong	tamount = 0L;
	Llong	amount;
	long	secsize = trackp->secsize;

	for (i = 0; i < MAX_TRACK; i++) {
		if (is_last(&trackp[i]))
			break;
	}
	if (i >= 1)
		vtracks = TRUE;
	if (vtracks && lverbose)
		printf(_("Compiling virtual track list ...\n"));

	for (i = 0; i < MAX_TRACK; i++) {
		if (trackp[i].tracksize < (tsize_t)0) {
			errmsgno(EX_BAD, _("VTrack %d has unknown length.\n"), i);
			return (-1);
		}
		amount = roundup(trackp[i].tracksize, secsize);
		amount += (Llong)trackp[i].padsecs * secsize;
		sectors += amount/secsize;
		ttrsize += trackp[i].tracksize;
		tamount += amount;
		if (vtracks && lverbose)
			printf(_("Vtrack:  %d size: %lld bytes %lld rounded (%lld sectors)\n"),
				(int)trackp[i].track, (Llong)trackp[i].tracksize,
				amount, amount / (Llong)secsize);

		if (is_last(&trackp[i]))
			break;

		/*
		 * XXX Is it possible for a DVD that input sector size
		 * XXX differes from output sector size?
		 * XXX I believe that not.
		 */
		if (trackp[i].tracksize % secsize) {
			comerrno(EX_BAD, _("Virtual track %d is not a multiple of secsize.\n"), (int)trackp[i].track);
		}
	}

	if (vtracks && lverbose)
		printf(_("Vtracks: %d size: %lld bytes %lld rounded (%ld sectors) total\n"),
			i+1, ttrsize, tamount, sectors);

	return (sectors);
}

LOCAL int
close_track_dvdplus(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	long	sectors = 0L;
	Llong	amount;
	long	secsize = trackp->secsize;

	/*
	 * Compute the start of the next "track" for the hack
	 * that allows to have a track in more than one file.
	 * XXX Check this if the vtrack code is removed.
	 */
	amount = roundup(trackp->tracksize, secsize);
	amount += (Llong)trackp->padsecs * secsize;
	sectors += amount/secsize;

	dvd_next_addr += sectors;

	return (0);
}

#ifdef	__needed_for_dvd_plusr__
LOCAL int
open_session_dvd(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	Uchar	mode[0x100];
	Uchar	moder[0x100];
	int	len;
	struct	cd_mode_page_05 *mp;
	struct	ricoh_mode_page_30 *rp = NULL;
	BOOL	burnfree = FALSE;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	if (!get_mode_params(scgp, 0x05, _("CD write parameter"),
			mode, (Uchar *)0, (Uchar *)0, (Uchar *)0, &len))
		return (-1);
	if (len == 0)
		return (-1);

	mp = (struct cd_mode_page_05 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	/*
	 * XXX as long as the Pioneer DVR-S101 only supports a single
	 * XXX data track in DAO mode,
	 * XXX do not set:
	 * XXX multi_session
	 * XXX sessipon_format
	 *
	 * Track mode = 00 Reserved on Pioneer DVR-S101
	 * Data block type = 00 Reserved on Pioneer DVR-S101
	 * Session format = 00 Reserved on Pioneer DVR-S101
	 * XXX DVR-S101 uses ls_v and link size violating
	 * XXX the current MMC2 spec.
	 */
	/* XXX look into drv_mmc.c for re-integration of above settings */
	mp->write_type = WT_SAO;


	rp = get_justlink_ricoh(scgp, moder);

	if (dp->cdr_cdcap->BUF != 0) {
		burnfree = mp->BUFE != 0;
	} else if ((dp->cdr_flags & CDR_BURNFREE) != 0) {
		burnfree = rp && rp->BUEFE != 0;
	}

	if (lverbose && (dp->cdr_flags & CDR_BURNFREE) != 0)
		printf(_("BURN-Free is %s.\n"), burnfree?_("ON"):_("OFF"));

	if (!burnfree && (dp->cdr_dstat->ds_cdrflags & RF_BURNFREE) != 0) {
		printf(_("Turning BURN-Free on\n"));
		burnfree = TRUE;
	}
	if (burnfree && (dp->cdr_dstat->ds_cdrflags & RF_BURNFREE) == 0) {
		printf(_("Turning BURN-Free off\n"));
		burnfree = FALSE;
	}
	if (dp->cdr_cdcap->BUF != 0) {
		mp->BUFE = burnfree?1:0;
	} else if ((dp->cdr_flags & CDR_BURNFREE) != 0) {

		if (rp)
			rp->BUEFE = burnfree?1:0;
	}
	if (rp) {
		i_to_2_byte(rp->link_counter, 0);
		if (xdebug)
			scg_prbytes(_("Mode Select Data "), moder, moder[0]+1);

		set_mode_params(scgp, _("Ricoh Vendor Page"), moder, moder[0]+1, 0, -1);
		rp = get_justlink_ricoh(scgp, moder);
	}

#ifdef	DEBUG
	if (lverbose > 1)
		scg_prbytes("CD write parameter:", (Uchar *)mode, len);
#endif
	if (!set_mode_params(scgp, _("CD write parameter"), mode, len, 0, -1))
		return (-1);

	return (0);
}
#endif

/*
 * We need to loop here because of a firmware bug in Pioneer DVD writers.
 */
LOCAL int
fixate_dvdplusrw(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	int	oldtimeout = scgp->deftimeout;
	int	ret = 0;
	int	i;
#define	MAX_TRIES	15

	/*
	 * XXX Is this timeout needed for DVD+RW too?
	 * XXX It was introduced for DVD-R that writes at least 800 MB
	 */
	if (scgp->deftimeout < 1000)
		scg_settimeout(scgp, 1000);

/*scgp->verbose++;*/
	scgp->silent++;
	for (i = 0; i <= MAX_TRIES; i++) {
		if (scsi_flush_cache(scgp, TRUE) < 0) {
			if (!scsi_in_progress(scgp) || i >= MAX_TRIES) {
				if (scgp->verbose <= 0) {
					scg_printerr(scgp);
					scg_printresult(scgp);	/* XXX restore key/code in future */
				}
				printf(_("Trouble flushing the cache\n"));
				scgp->silent--;
				scg_settimeout(scgp, oldtimeout);
				return (-1);
			}
			sleep(1);
		} else {
			break;
		}
	}
	scgp->silent--;
	waitformat(scgp, 300);

	scgp->silent++;
	for (i = 0; i <= MAX_TRIES; i++) {
		if (scsi_close_tr_session(scgp, CL_TYPE_SESSION, 0, TRUE) < 0) {
			if (!scsi_in_progress(scgp) || i >= MAX_TRIES) {
				if (scgp->verbose <= 0) {
					scg_printerr(scgp);
					scg_printresult(scgp);	/* XXX restore key/code in future */
				}
				printf(_("Trouble closing the session\n"));
				break;
			}
			sleep(1);
		} else {
			break;
		}
	}
	scgp->silent--;
	waitformat(scgp, 300);
/*scgp->verbose--;*/

	scg_settimeout(scgp, oldtimeout);
	return (ret);
#undef	MAX_TRIES
}


LOCAL int
fixate_dvdplusr(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	int	oldtimeout = scgp->deftimeout;
	int	ret = 0;
	int	key = 0;
	int	code = 0;
	int	trackno;
	int	i;
#define	MAX_TRIES	15

	/*
	 * XXX Is this timeout needed for DVD+R too?
	 * XXX It was introduced for DVD-R that writes at least 800 MB
	 */
	if (scgp->deftimeout < 1000)
		scg_settimeout(scgp, 1000);

/*scgp->verbose++;*/
	scgp->silent++;
	for (i = 0; i <= MAX_TRIES; i++) {
		if (scsi_flush_cache(scgp, TRUE) < 0) {
			if (!scsi_in_progress(scgp) || i >= MAX_TRIES) {
				if (scgp->verbose <= 0) {
					scg_printerr(scgp);
					scg_printresult(scgp);	/* XXX restore key/code in future */
				}
				printf(_("Trouble flushing the cache\n"));
				scgp->silent--;
				scg_settimeout(scgp, oldtimeout);
				return (-1);
			}
			sleep(1);
		} else {
			break;
		}
	}
	scgp->silent--;
	key = scg_sense_key(scgp);
	code = scg_sense_code(scgp);
	waitformat(scgp, 600);
	/*
	 * With CDs we used to close the invisible track (0xFF) but
	 * with DVDs this may not work anymore if the unit is MMC-3
	 * or above. Use the real track/border number instead.
	 * We always use the "IMMED" flag - is this OK?
	 */
	trackno = trackp->trackno;
	if (trackno <= 0)
		trackno = 1;
	scgp->silent++;
	for (i = 0; i <= MAX_TRIES; i++) {
		if (scsi_close_tr_session(scgp, CL_TYPE_TRACK, trackno, TRUE) < 0) {
			if (!scsi_in_progress(scgp) || i >= MAX_TRIES) {
				if (scgp->verbose <= 0) {
					scg_printerr(scgp);
					scg_printresult(scgp);	/* XXX restore key/code in future */
				}
				printf(_("Trouble closing the track\n"));
				break;
			}
			sleep(1);
		} else {
			break;
		}
	}
	scgp->silent--;
	key = scg_sense_key(scgp);
	code = scg_sense_code(scgp);
	waitformat(scgp, 600);

	scgp->silent++;
	for (i = 0; i <= MAX_TRIES; i++) {
		if (scsi_close_tr_session(scgp, 0x06, 0, TRUE) < 0) {
			if (!scsi_in_progress(scgp) || i >= MAX_TRIES) {
				if (scgp->verbose <= 0) {
					scg_printerr(scgp);
					scg_printresult(scgp);	/* XXX restore key/code in future */
				}
				printf(_("Trouble closing the last session\n"));
				break;
			}
			sleep(1);
		} else {
			break;
		}
	}
	scgp->silent--;
	key = scg_sense_key(scgp);
	code = scg_sense_code(scgp);
	waitformat(scgp, 600);
/*scgp->verbose--;*/

	scg_settimeout(scgp, oldtimeout);
	return (ret);
#undef	MAX_TRIES
}


/*--------------------------------------------------------------------------*/

LOCAL BOOL
dvdplus_ricohbased(scgp)
	SCSI	*scgp;
{
	if (scgp->inq == NULL)
		return (FALSE);

	if (strncmp(scgp->inq->inq_vendor_info, "RICOH", 5) == 0) {
		if (strbeg("DVD+RW MP5120", scgp->inq->inq_prod_ident) ||
		    strbeg("DVD+RW MP5125", scgp->inq->inq_prod_ident))
			return (TRUE);
	}
	if (strncmp(scgp->inq->inq_vendor_info, "HP", 2) == 0) {
		if (strbeg("DVD Writer 100j", scgp->inq->inq_prod_ident) ||
		    strbeg("DVD Writer 200j", scgp->inq->inq_prod_ident))
			return (TRUE);
	}
	return (FALSE);
}

LOCAL int
blank_dvdplus(scgp, dp, addr, blanktype)
	SCSI	*scgp;
	cdr_t	*dp;
	long	addr;
	int	blanktype;
{
/*XXX*/extern char *blank_types[];

	BOOL	cdrr	 = FALSE;	/* Read CD-R	*/
	BOOL	cdwr	 = FALSE;	/* Write CD-R	*/
	BOOL	cdrrw	 = FALSE;	/* Read CD-RW	*/
	BOOL	cdwrw	 = FALSE;	/* Write CD-RW	*/
	BOOL	dvdwr	 = FALSE;	/* DVD writer	*/
	int	profile;

	int	flags = DC_ERASE;
	int	blanksize = 0x30000;
	int	ret;

	mmc_check(scgp, &cdrr, &cdwr, &cdrrw, &cdwrw, NULL, &dvdwr);

	profile = get_curprofile(scgp);
	/*
	 * XXX How to check for DVD drives that support DVD-RW
	 */
/*	if (!dvdwr)*/
/*		return (blank_dummy(scgp, dp, addr, blanktype));*/

	if (!dvdplus_ricohbased(scgp)) {
		errmsgno(EX_BAD, _("Cannot blank DVD+RW media with non Ricoh based drive.\n"));
		if (profile == 0x1A || profile == 0x2A) {
			ret = blank_simul(scgp, dp, addr, blanktype);
			waitformat(scgp, 600);
			scsi_flush_cache(scgp, TRUE);
			waitformat(scgp, 600);
			return (ret);
		}
		return (-1);
	}


	if (blanktype == BLANK_DISC) {
		blanksize = 0x26c090;
	} else {
		blanktype = BLANK_MINIMAL;
		blanksize = 0x30000;
	}
	if (lverbose) {
		printf(_("Blanking %s\n"), blank_types[blanktype & 0x07]);
		flush();
	}
	if (driveropts != NULL) {
		char	*p;

		p = hasdrvopt(driveropts, "erase");
		if (p == NULL)
			p = hasdrvopt(driveropts, "blank");
		if (p != NULL) {
			if (*p == '1')
				flags |= DC_ERASE;
			else
				flags &= ~DC_ERASE;
		}
		p = hasdrvopt(driveropts, "set");
		if (p != NULL) {
			if (*p == '1')
				flags |= CHANGE_SETTING;
			else
				flags &= ~CHANGE_SETTING;
		}
		p = hasdrvopt(driveropts, "dvdr");
		if (p != NULL) {
			if (*p == '1')
				flags |= DVD_PLUS_R;
			else
				flags &= ~DVD_PLUS_R;
		}
	}

/*Size Min is 0x30000, Size Max is: 0x26c090 */
#ifdef	DVDPLUS_DEBUG
error("Flags: %d blanksize: %d (0x%X)\n", flags, blanksize, blanksize);
#endif

	ret = ricoh_dvdsetting(scgp, blanksize, flags, TRUE);
	waitformat(scgp, 2000);
	return (ret);
}

LOCAL int
format_dvdplus(scgp, dp, fmtflags)
	SCSI	*scgp;
	cdr_t	*dp;
	int	fmtflags;
{
	/*
	 * 1.3 Formatting
	 * --------------
	 *
	 * - For efficiency, please use the Format command only on blank discs
	 *
	 * - The command is:
	 *	- "04 11 00 00 00 00 00 00 00 00 00 00
	 *	    ^^^
	 *	    0x10	Format Data
	 *	    0x01	Format Code == 1
	 *	00 82 00 08 00 23 05 40 98 00 00 01"
	 *	^^^^^^^^^^^ ^^^^^^^^^^^^^^^^^^^^^^^
	 *	Format		Format
	 *	header		descriptor
	 *	0x80 FOV
	 *	0x20 IMMED
	 *	len == 0x08
	 */
	char	cap_buf[4096];
	char	fmt_buf[128];
	int	len;
	int	i;
	struct scsi_cap_data		*cp;
	struct scsi_format_cap_desc	*lp;
	struct scsi_format_header	*fhp;
	long	blocks = 0;
	struct timeval starttime;
	struct timeval stoptime;
#ifdef	DVDPLUS_DEBUG
	struct timeval stoptime2;
#endif

	len = get_format_capacities(scgp, cap_buf, sizeof (cap_buf));
	if (len < 0)
		return (-1);

#ifdef	DVDPLUS_DEBUG
	error("Cap len: %d\n", len);
	scg_prbytes("Format cap:", (Uchar *)cap_buf, len);
#endif

	cp = (struct scsi_cap_data *)cap_buf;
	lp = cp->list;
	len -= sizeof (struct scsi_format_cap_header);
	if (lp->desc_type == 2) {
		if ((dp->cdr_cmdflags & F_FORCE) == 0) {
			errmsgno(EX_BAD, _("Medium is already formatted.\n"));
			return (-1);
		}
	}
#ifdef	DVDPLUS_DEBUG
	error("hd len: %d len: %d\n", cp->hd.len, len);
#endif

	for (i = len; i >= sizeof (struct scsi_format_cap_desc);
			i -= sizeof (struct scsi_format_cap_desc), lp++) {
#ifdef	DVDPLUS_DEBUG
		error("blocks %ld fmt 0x%X desc %d blen: %ld\n",
			(long)a_to_u_4_byte(lp->nblock),
			lp->fmt_type,
			lp->desc_type,
			(long)a_to_u_3_byte(lp->blen));
#endif
		if (lp->fmt_type == FCAP_TYPE_DVDPLUS_FULL)
			blocks = a_to_u_4_byte(lp->nblock);
	}
	if (blocks == 0) {
		errmsgno(EX_BAD, _("DVD+RW Full format capacity not found.\n"));
		return (-1);
	}

	fhp = (struct scsi_format_header *)fmt_buf;
	lp = (struct scsi_format_cap_desc *)&fmt_buf[4];
	fillbytes((caddr_t)fmt_buf, sizeof (fmt_buf), '\0');

	fhp->enable = 1;
	fhp->immed = 1;
	i_to_2_byte(fhp->length, 8);
	i_to_4_byte(lp->nblock, blocks);
	lp->fmt_type = FCAP_TYPE_DVDPLUS_FULL;
	i_to_3_byte(lp->blen, 0);
/*	i_to_3_byte(lp->blen, 1);*/

#ifdef	DVDPLUS_DEBUG
	scg_prbytes("Format desc:", (Uchar *)fmt_buf, 12);
#endif

	if (lverbose) {
		/*
		 * XXX evt. restart Format ansagen...
		 */
		printf(_("Formatting media\n"));
		flush();
	}
	starttime.tv_sec = 0;
	starttime.tv_usec = 0;
	stoptime = starttime;
	gettimeofday(&starttime, (struct timezone *)0);

	if (format_unit(scgp, fhp, /*fhp->length*/ 8 + sizeof (struct scsi_format_header),
				1, 0, 0, 0, 3800) < 0) {
		return (-1);
	}


#ifdef	DVDPLUS_DEBUG
/*	if (ret >= 0 && lverbose) {*/
	if (1) {
		gettimeofday(&stoptime, (struct timezone *)0);
		prtimediff("Format time: ", &starttime, &stoptime);
	}
#endif
	waitformat(scgp, 300);

#ifdef	DVDPLUS_DEBUG
	gettimeofday(&stoptime2, (struct timezone *)0);
	prtimediff("Format WAIT time: ", &stoptime, &stoptime2);
	prtimediff("Format time TOTAL: ", &starttime, &stoptime2);
#endif


#ifdef	DVDPLUS_DEBUG
error("------------> STOP DE-ICE\n");
#endif
	scsi_close_tr_session(scgp, CL_TYPE_STOP_DEICE, 0, TRUE);
	waitformat(scgp, 300);

#ifdef	DVDPLUS_DEBUG
	gettimeofday(&stoptime2, (struct timezone *)0);
	prtimediff("Format time TOTAL 2 : ", &starttime, &stoptime2);
#endif

#ifdef	DVDPLUS_DEBUG
error("------------> CLOSE SESSION\n");
#endif
	scsi_close_tr_session(scgp, CL_TYPE_SESSION, 0, TRUE);
	waitformat(scgp, 300);

#ifdef	DVDPLUS_DEBUG
	gettimeofday(&stoptime2, (struct timezone *)0);
	prtimediff("Format time TOTAL 3: ", &starttime, &stoptime2);
#endif
	return (0);
}

LOCAL int
waitformat(scgp, secs)
	SCSI	*scgp;
	int	secs;
{
	Uchar   sensebuf[CCS_SENSE_LEN];
	int	printed = 0;
	int	i;
	int	key;
#define	W_SLEEP	2

	scgp->silent++;
	for (i = 0; i < secs/W_SLEEP; i++) {
		if (test_unit_ready(scgp) >= 0) {
			scgp->silent--;
			return (0);
		}
		key = scg_sense_key(scgp);
		if (key != SC_UNIT_ATTENTION && key != SC_NOT_READY)
			break;
request_sense_b(scgp, (caddr_t)sensebuf, sizeof (sensebuf));
#ifdef	XXX
scg_prbytes("Sense:", sensebuf, sizeof (sensebuf));
scgp->scmd->u_scb.cmd_scb[0] = 2;
movebytes(sensebuf, scgp->scmd->u_sense.cmd_sense, sizeof (sensebuf));
scgp->scmd->sense_count = sizeof (sensebuf);
scg_printerr(scgp);
#endif
/*
 * status: 0x2 (CHECK CONDITION)
 * Sense Bytes: F0 00 00 00 24 1C 10 0C 00 00 00 00 04 04 00 80 03 F6
 * Sense Key: 0x0 No Additional Sense, Segment 0
 * Sense Code: 0x04 Qual 0x04 (logical unit not ready, format in progress) Fru 0x0
 * Sense flags: Blk 2366480 (valid)
 * cmd finished after 0.000s timeout 100s
 * Das Fehlt:
 * operation 1% done
 */

		if (lverbose && (sensebuf[15] & 0x80)) {
			printed++;
			error(_("operation %d%% done\r"),
				(100*(sensebuf[16] << 8 |
					sensebuf[17]))/(unsigned)65536);
		}
		sleep(W_SLEEP);
	}
	scgp->silent--;
	if (printed)
		error("\n");
	return (-1);
#undef	W_SLEEP
}

LOCAL int
stats_dvdplus(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	Uchar mode[256];
	struct	ricoh_mode_page_30 *rp;
	UInt32_t count;

	if ((dp->cdr_dstat->ds_cdrflags & RF_BURNFREE) == 0)
		return (0);

	rp = get_justlink_ricoh(scgp, mode);
	if (rp) {
		count = a_to_u_2_byte(rp->link_counter);
		if (lverbose) {
			if (count == 0)
				printf(_("BURN-Free was not used.\n"));
			else
				printf(_("BURN-Free was %d times used.\n"),
					(int)count);
		}
	}
	return (0);
}

#define	FMTDAT	0x10
#define	CMPLST	0x08

EXPORT int
format_unit(scgp, fmt, length, list_format, dgdl, interlv, pattern, timeout)
	SCSI	*scgp;
	void	*fmt;
	int	length;
	int	list_format;	/* 0 if fmt == 0			*/
	int	dgdl;		/* disable grown defect list		*/
	int	interlv;
	int	pattern;
	int	timeout;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)fmt;
	scmd->size = length;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	if (timeout < 0)
		timeout = 24*3600;	/* Kein Timeout XXX kann haengen */
	scmd->timeout = timeout;
	scmd->cdb.g0_cdb.cmd = 0x04;	/* Format Unit */
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	scmd->cdb.g0_cdb.high_addr = (fmt?FMTDAT:0)|(dgdl?CMPLST:0)|list_format;
	scmd->cdb.g0_cdb.mid_addr = pattern;
	scmd->cdb.g0_cdb.count = interlv;

#ifdef	DVDPLUS_DEBUG
	scg_prbytes("Format CDB: ", (u_char *)scmd->cdb.cmd_cdb, scmd->cdb_len);

/*	if (scgp->verbose && fmt)*/
		scg_prbytes("Format Data:", (u_char *)fmt, length);
#endif

	scgp->cmdname = "format unit";

	return (scg_cmd(scgp));
}

LOCAL int
set_p_layerbreak(scgp, tsize, lbreak)
	SCSI	*scgp;
	long	tsize;
	Int32_t	lbreak;
{
	struct dvd_structure_20	lb;
	int			ret;
	UInt32_t		dsize;
	UInt32_t		l0_cap;

	/*
	 * Layer boundary information 0x20
	 */
	fillbytes((caddr_t)&lb, sizeof (lb), '\0');
	ret = read_dvd_structure(scgp, (caddr_t)&lb, sizeof (lb), 0, 0, 0, 0x20);
	if (ret < 0)
		return (ret);

	if (lb.res47[0] & 0x80) {
		errmsgno(EX_BAD, _("Layer 0 zone capacity already set.\n"));
		return (-1);
	}

	l0_cap = a_to_u_4_byte(lb.l0_area_cap);
	if (lbreak > 0 && lbreak > l0_cap) {
		errmsgno(EX_BAD, _("Manual layer break %d > %u not allowed.\n"),
							lbreak, l0_cap);
		return (-1);
	}
	dsize = roundup(tsize, 16);
	if (lbreak <= 0 && dsize <= l0_cap) {
		/*
		 * Allow to write DL media with less than single layer size
		 * in case of manual layer break set up.
		 */
		errmsgno(EX_BAD,
			_("Layer 0 size %u is bigger than expected disk size %u.\n"),
			l0_cap, dsize);
		errmsgno(EX_BAD, _("Use single layer medium.\n"));
		return (-1);
	}
	l0_cap = dsize / 2;
	l0_cap = roundup(l0_cap, 16);
	if (lbreak > 0 && lbreak < l0_cap) {
		errmsgno(EX_BAD, _("Manual layer break %d < %u not allowed.\n"),
							lbreak, l0_cap);
		return (-1);
	}
	if (lbreak > 0)
		l0_cap = lbreak;
	i_to_4_byte(lb.l0_area_cap, l0_cap);

	ret = send_dvd_structure(scgp, (caddr_t)&lb, sizeof (lb), 0x20);
	return (ret);
}

LOCAL int
ricoh_dvdsetting(scgp, erase_size, flags, immed)
	SCSI	*scgp;
	int	erase_size;
	int	flags;
	int	immed;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->timeout = 3600;
	scmd->cdb.g1_cdb.cmd = 0xEA;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.cmd_cdb[1] = immed ? 1 : 0;
	scmd->cdb.cmd_cdb[2] = flags;
	i_to_3_byte(&scmd->cdb.cmd_cdb[3], erase_size);

	scgp->cmdname = "set_dummy_dvdr_setttings";

	return (scg_cmd(scgp));
}

#ifdef	__needed__
/*
 * 0x21 Emulation write is OFF
 * 0x20 Emulation write is ON
 * 0xFF Disk is loading
 */
LOCAL int
dummy_plextor(scgp, modecode)
	SCSI	*scgp;
	int	modecode;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xF3;
	scmd->cdb.cmd_cdb[1] = 0x1F;
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	scmd->cdb.cmd_cdb[2] = 0x16;
	scmd->cdb.cmd_cdb[3] = modecode;

	scgp->cmdname = "plextor dummy";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}
#endif
