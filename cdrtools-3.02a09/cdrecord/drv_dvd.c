/* @(#)drv_dvd.c	1.167 13/12/10 Copyright 1998-2013 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)drv_dvd.c	1.167 13/12/10 Copyright 1998-2013 J. Schilling";
#endif
/*
 *	DVD-R device implementation for
 *	SCSI-3/mmc-2 conforming drives
 *	Currently it only supports the Pioneer DVD-R S101
 *	as for the near future, there are no other drives.
 *
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
 *	Copyright (c) 1998-2013 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifndef	DEBUG
#define	DEBUG
#endif
#include <schily/mconfig.h>

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>	/* Include sys/types.h to make off_t available */
#include <schily/standard.h>
#include <schily/string.h>

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

LOCAL	cdr_t	*identify_dvd		__PR((SCSI *scgp, cdr_t *, struct scsi_inquiry *));
LOCAL	int	attach_dvd		__PR((SCSI *scgp, cdr_t *));
LOCAL	void	di_to_dstat		__PR((struct disk_info *dip, dstat_t *dsp));
LOCAL	int	init_dvd		__PR((SCSI *scgp, cdr_t *dp));
LOCAL	int	getdisktype_dvd		__PR((SCSI *scgp, cdr_t *dp));
LOCAL	int	prdiskstatus_dvd	__PR((SCSI *scgp, cdr_t *dp));
LOCAL	int	speed_select_dvd	__PR((SCSI *scgp, cdr_t *dp, int *speedp));
LOCAL	int 	session_offset_dvd	__PR((SCSI *scgp, long *offp));
LOCAL	int	next_wr_addr_dvd	__PR((SCSI *scgp, track_t *trackp, long *ap));
LOCAL	int	open_track_dvd		__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	long	rzone_size		__PR((track_t *trackp));
LOCAL	int	close_track_dvd		__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	open_session_dvd	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	waitformat		__PR((SCSI *scgp, int secs));
LOCAL	int	fixate_dvd		__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	blank_dvd		__PR((SCSI *scgp, cdr_t *dp, long addr, int blanktype));
LOCAL	int	stats_dvd		__PR((SCSI *scgp, cdr_t *dp));
#ifdef	__needed__
LOCAL	int	read_rzone_info		__PR((SCSI *scgp, caddr_t bp, int cnt));
LOCAL	int	reserve_rzone		__PR((SCSI *scgp, long size));
#endif
/*LOCAL	int	send_dvd_structure	__PR((SCSI *scgp, caddr_t bp, int cnt));*/
#ifdef	__needed__
LOCAL	int	set_layerbreak		__PR((SCSI *scgp, long	tsize, Int32_t lbreak));
#endif
LOCAL	void	print_dvd00		__PR((struct dvd_structure_00 *dp));
LOCAL	void	print_dvd01		__PR((struct dvd_structure_01 *dp));
LOCAL	void	print_dvd04		__PR((struct dvd_structure_04 *dp));
LOCAL	void	print_dvd05		__PR((struct dvd_structure_05 *dp));
LOCAL	void	print_dvd0D		__PR((struct dvd_structure_0D *dp));
LOCAL	void	print_dvd0E		__PR((struct dvd_structure_0E *dp));
LOCAL	void	print_dvd0F		__PR((struct dvd_structure_0F *dp));
LOCAL	void	print_dvd20		__PR((struct dvd_structure_20 *dp));
LOCAL	void	print_dvd22		__PR((struct dvd_structure_22 *dp));
LOCAL	void	print_dvd23		__PR((struct dvd_structure_23 *dp));
#ifdef	__needed__
LOCAL	void	send_dvd0F		__PR((SCSI *scgp));
#endif
/*LOCAL	void	print_dvd_info		__PR((SCSI *scgp));*/
EXPORT	void	print_dvd_info		__PR((SCSI *scgp));
LOCAL	void	print_laserlog		__PR((SCSI *scgp));

cdr_t	cdr_dvd = {
	0, 0, 0,
	CDR_DVD|CDR_SWABAUDIO,
	CDR2_NOCD,
	CDR_CDRW_ALL,
	WM_SAO,
	1000, 1000,
	"mmc_dvd",
	"generic SCSI-3/mmc-2 DVD-R/DVD-RW/DVD-RAM driver",
	0,
	(dstat_t *)0,
	identify_dvd,
	attach_dvd,
	init_dvd,
	getdisktype_dvd,
	prdiskstatus_dvd,
	scsi_load,
	scsi_unload,
	read_buff_cap,
	cmd_dummy,					/* recovery_needed */
	(int(*)__PR((SCSI *, cdr_t *, int)))cmd_dummy,	/* recover	*/
	speed_select_dvd,
	select_secsize,
	next_wr_addr_dvd,
	(int(*)__PR((SCSI *, Ulong)))cmd_ill,		/* reserve_track */
	scsi_cdr_write,
	(int(*)__PR((track_t *, void *, BOOL)))cmd_dummy,	/* gen_cue */
	(int(*)__PR((SCSI *scgp, cdr_t *, track_t *)))cmd_dummy, /* send_cue */
	(int(*)__PR((SCSI *, cdr_t *, track_t *)))cmd_dummy, /* leadin */
	open_track_dvd,
	close_track_dvd,
	open_session_dvd,
	cmd_dummy,
	cmd_dummy,					/* abort	*/
	session_offset_dvd,
	fixate_dvd,
	stats_dvd,
	blank_dvd,
	format_dummy,
	(int(*)__PR((SCSI *, caddr_t, int, int)))NULL,	/* no OPC	*/
	cmd_dummy,					/* opt1		*/
	cmd_dummy,					/* opt2		*/
};


LOCAL cdr_t *
identify_dvd(scgp, dp, ip)
	SCSI			*scgp;
	cdr_t			*dp;
	struct scsi_inquiry	*ip;
{
	BOOL	dvd	 = FALSE;	/* DVD writer	*/
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
		return (NULL);	/* Pre SCSI-2/mmc drive		*/

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

	dvd = mp->dvd_r_write;		/* Mode page 0x2A DVD-R writer */

	/*
	 * Be careful, Lite-ON drives are lying in mode page 0x2A.
	 * We need to check the MMC-3 profile list too.
	 */
	profile = get_curprofile(scgp);
	if (profile >= 0x11 && profile <= 0x19)
		dvd = TRUE;

	if (!dvd)
		get_wproflist(scgp, NULL, &dvd, NULL, NULL);

	if (!dvd)			/* Any DVD- writer		*/
		return (NULL);

	return (dp);
}

LOCAL int
attach_dvd(scgp, dp)
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
		if (p != NULL && *p != '\0') {
			char	*ep;
			Llong	ll;
			Int32_t	lb;

			ep = astoll(p, &ll);
			lb = ll;
			if ((*ep != '\0' && *ep != ',') ||
			    ll <= 0 || ll != lb) {
				errmsgno(EX_BAD,
					_("Bad layer break value '%s'.\n"), p);
				return (-1);
			}
			dp->cdr_dstat->ds_layer_break = lb;
		} else {
			p = hasdrvopt(driveropts, "layerbreak");
			if (p != NULL && *p == '1')
				dp->cdr_dstat->ds_layer_break = 0;
		}
		if (dp->cdr_dstat->ds_layer_break >= 0 &&
		    (dp->cdr_flags & CDR_LAYER_JUMP) == 0) {
			errmsgno(EX_BAD,
			_("Cannot set layer break on this drive/medium.\n"));
			return (-1);
		}
		if (dp->cdr_dstat->ds_layer_break != -1 &&
		    dp->cdr_dstat->ds_layer_break !=
		    roundup(dp->cdr_dstat->ds_layer_break, 16)) {
			errmsgno(EX_BAD,
			_("Layer break at %u is not properly aligned.\n"),
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
init_dvd(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	return (speed_select_dvd(scgp, dp, NULL));
}

LOCAL int
getdisktype_dvd(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
extern	char	*buf;
	dstat_t	*dsp = dp->cdr_dstat;
	struct disk_info *dip;
	Uchar	mode[0x100];
	struct rzone_info rz;
	struct rzone_info *rp;
	struct dvd_structure_00 *sp;
	int	profile;
	int	len;
	BOOL	did_dummy = FALSE;

	if (lverbose > 0)
		print_laserlog(scgp);

	if (lverbose > 2)
		print_logpages(scgp);


	if (dsp->ds_type == DST_UNKNOWN) {
		profile = get_curprofile(scgp);
		if (profile >= 0)
			dsp->ds_type = profile;
	}

	if ((dp->cdr_dstat->ds_cdrflags & RF_PRATIP) != 0) {
		if (((dsp->ds_cdrflags & (RF_WRITE|RF_BLANK)) == 0) ||
				lverbose > 1) {
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
	if (dip->disk_status == DS_COMPLETE &&
			(dsp->ds_cdrflags & (RF_WRITE|RF_BLANK)) == RF_WRITE) {
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
			speed_select_dvd(scgp, dp, &xspeed);
			dp->cdr_cmdflags = oflags;
			did_dummy = TRUE;
			goto again;
		}
		/*
		 * Trying to clear drive status did not work...
		 */
		reload_media(scgp, dp);
	}
	if (get_diskinfo(scgp, dip, sizeof (*dip)) < 0)
		return (-1);
	di_to_dstat(dip, dsp);

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
	movebytes(mode, (caddr_t)&rz, sizeof (struct rzone_info));
#ifdef	nonono_old_code
	/*
	 * Old code used with the S101 seems to be a bad idea....
	 * The new code seems to work with all DVD drives.
	 */
	dsp->ds_maxblocks = a_to_u_4_byte(rp->rzone_size);
	if (dsp->ds_maxblocks == 0)
#endif
	dsp->ds_maxblocks = a_to_u_4_byte(rp->free_blocks);
	if (rp->nwa_v)
		dsp->ds_maxblocks += a_to_u_4_byte(rp->next_recordable_addr);

	/*
	 * This information is based on the physical pre recorded information.
	 * First try to find the len supported by the actual drive.
	 */
	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	if (read_dvd_structure(scgp, (caddr_t)mode, 2, 0, 0, 0, 0) < 0) {
		errmsgno(EX_BAD, _("Cannot read DVD structure.\n"));
		return (-1);
	}
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
	read_dvd_structure(scgp, (caddr_t)sp, len, 0, 0, 0, 0);
/*	if (lverbose > 1)*/
/*		print_dvd00(sp);*/
	if (((struct dvd_structure_00 *)sp)->book_type == 1) {
		dsp->ds_type = DST_DVD_RAM;
	} else {
		dsp->ds_type = DST_UNKNOWN;
	}
	if (get_curprofile(scgp) == 0x12) {
		dsp->ds_type = DST_DVD_RAM;
	}
	if (dsp->ds_type == DST_DVD_RAM)
		dsp->ds_maxblocks = a_to_u_4_byte(rz.rzone_size);
	/*
	 * Bei Pioneer ist Phys End ist nur bei dem S101 != 0.
	 * Bei Panasonic ist Phys End == Phys Start.
	 * Do not print this for -msinfo
	 */
	if ((dp->cdr_cmdflags & F_MSINFO) == 0 &&
	    (a_to_u_3_byte(sp->phys_end) != 0) &&
			(dsp->ds_maxblocks !=
			(long)(a_to_u_3_byte(sp->phys_end) - a_to_u_3_byte(sp->phys_start) + 1))) {
		/*
		 * NEC 'DVD_RW ND-3500AG' mit 'MCC 03RG20  ' DVD-R (leer):
		 * dsp->ds_maxblocks:	2298496
		 * sp->phys_start:	196608 (0x30000)
		 * sp->phys_end:	2495103
		 *			2298496 = 2495103 - 196608 +1
		 * Bei diesen Parametern gibt es keine Warnung.
		 */

		printf(_("WARNING: Phys disk size %ld differs from rzone size %ld! Prerecorded disk?\n"),
			(long)(a_to_u_3_byte(sp->phys_end) - a_to_u_3_byte(sp->phys_start) + 1),
			(long)dsp->ds_maxblocks);
		printf(_("WARNING: Phys start: %ld Phys end %ld\n"),
			(long)a_to_u_3_byte(sp->phys_start),
			(long)a_to_u_3_byte(sp->phys_end));

		/*
		 * Workaround for some drive media combinations.
		 * At least the drive	'HL-DT-ST' 'DVD-RAM GH22NP20' '1.02'
		 * does not report maxblocks correctly with 'MCC 03RG20  ' media.
		 * Use the information from ADIP instead.
		 */
		if (dsp->ds_maxblocks == 0) {
			printf(_("WARNING: Drive returns zero media size. Using media size from ADIP.\n"));
			dsp->ds_maxblocks = a_to_u_3_byte(sp->phys_end) - a_to_u_3_byte(sp->phys_start) + 1;
		}
	}

	return (drive_getdisktype(scgp, dp));
}

LOCAL int
prdiskstatus_dvd(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	return (prdiskstatus(scgp, dp, FALSE));
}

LOCAL int
speed_select_dvd(scgp, dp, speedp)
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
		scg_prbytes(_("CD write parameter:"), (Uchar *)mode, len);
#endif

	if (dp->cdr_dstat->ds_type == DST_DVD_RAM && dummy != 0) {
		errmsgno(EX_BAD, _("DVD-RAM has no -dummy mode.\n"));
		return (-1);
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
	if (dp->cdr_dstat->ds_layer_break >= 0)
		mp->write_type = WT_LAYER_JUMP;

#ifdef	DEBUG
	if (lverbose > 1)
		scg_prbytes(_("CD write parameter:"), (Uchar *)mode, len);
#endif
	if (!set_mode_params(scgp, _("CD write parameter"), mode, len, 0, -1))
		return (-1);

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
	val = 0;
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

LOCAL int
session_offset_dvd(scgp, offp)
	SCSI	*scgp;
	long	*offp;
{
	return (sessstatus(scgp, FALSE, offp, (long *)NULL));
}

LOCAL long	dvd_next_addr;

LOCAL int
next_wr_addr_dvd(scgp, trackp, ap)
	SCSI	*scgp;
	track_t	*trackp;
	long	*ap;
{
	struct disk_info	di;
	struct rzone_info	rz;
	int			tracks;
	long			next_addr = -1;

	/*
	 * If the track pointer is set to NULL, our caller likes to get
	 * the next writable address for the next (unwritten) session.
	 */
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
		if (track_base(trackp)->tracktype & TOCF_MULTI)
			get_trackinfo(scgp, (caddr_t)&rz, TI_TYPE_TRACK, trackp->trackno, sizeof (rz));
		else
			read_rzone_info(scgp, (caddr_t)&rz, sizeof (struct rzone_info));
		dvd_next_addr = a_to_4_byte(rz.next_recordable_addr);
		if (lverbose > 1)
			printf(_("next writable addr: %ld valid: %d\n"), dvd_next_addr, rz.nwa_v);
	}
	if (ap)
		*ap = dvd_next_addr;
	return (0);
}

LOCAL int
open_track_dvd(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t *trackp;
{
	Uchar	mode[0x100];
	int	len;
	long	sectors;
	struct	cd_mode_page_05 *mp;

	if (trackp->track > 1)	/* XXX Hack to make one 'track' from several */
		return (0);	/* XXX files in Disk at once mode only.	    */

	if (dp->cdr_dstat->ds_type == DST_DVD_RAM) {
		/*
		 * Compile vitual track list
		 */
		sectors = rzone_size(trackp);
		if (sectors < 0)
			return (-1);
		return (0);	/* No further setup needed */
	}

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
		scg_prbytes(_("CD write parameter:"), (Uchar *)mode, len);
#endif
	if (!set_mode_params(scgp, _("CD write parameter"), mode, len, 0, trackp->secsize))
		return (-1);

	/*
	 * Compile vitual track list
	 */
	sectors = rzone_size(trackp);
	if (sectors < 0)
		return (-1);
	return (reserve_tr_rzone(scgp, sectors));
}

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
close_track_dvd(scgp, dp, trackp)
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
	if (dp->cdr_dstat->ds_layer_break >= 0)
		mp->write_type = WT_LAYER_JUMP;


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

	/*
	 * 05 32 40 c5  08 10 00 00
	 *	 FUFE + PACKET
	 *	    MULTI + NO FP + TM_DATA
	 *		DB_ROM_MODE1
	 *		   LINKSIZE == 16
	 * mp->session_format SES_DA_ROM
	 */
	mp->multi_session = (track_base(trackp)->tracktype & TOCF_MULTI) ?
			MS_MULTI : MS_NONE;

	/*
	 * If we are writing in multi-border mode, we need to write in packet
	 * mode even if we have been told to write on SAO mode.
	 */
	if (track_base(trackp)->tracktype & TOCF_MULTI) {
		mp->write_type = WT_PACKET;
		mp->track_mode = TM_DATA;
		mp->track_mode |= TM_INCREMENTAL;
		mp->fp = 0;
		i_to_4_byte(mp->packet_size, 0);
		mp->link_size = 16;
	}

#ifdef	DEBUG
	if (lverbose > 1)
		scg_prbytes(_("CD write parameter:"), (Uchar *)mode, len);
#endif
	if (!set_mode_params(scgp, _("CD write parameter"), mode, len, 0, -1))
		return (-1);

	return (0);
}

LOCAL int
waitformat(scgp, secs)
	SCSI	*scgp;
	int	secs;
{
#ifdef	DVD_DEBUG
	Uchar   sensebuf[CCS_SENSE_LEN];
#endif
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
#ifdef	DVD_DEBUG
		request_sense_b(scgp, (caddr_t)sensebuf, sizeof (sensebuf));
#ifdef	XXX
		scg_prbytes(_("Sense:"), sensebuf, sizeof (sensebuf));
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

		if (sensebuf[15] & 0x80) {
			error(_("operation %d%% done\n"),
				(100*(sensebuf[16] << 8 |
					sensebuf[17]))/(unsigned)65536);
		}
#endif	/* DVD_DEBUG */
		sleep(W_SLEEP);
	}
	scgp->silent--;
	return (-1);
#undef	W_SLEEP
}

LOCAL int
fixate_dvd(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	int	oldtimeout = scgp->deftimeout;
	int	ret = 0;
	int	trackno;

	/*
	 * This is only valid for DAO recording.
	 * XXX Check this if the DVR-S101 supports more.
	 * XXX flush cache currently makes sure that
	 * XXX at least ~ 800 MBytes written to the track.
	 * XXX flush cache triggers writing the lead out.
	 */
	if (scgp->deftimeout < 1000)
		scg_settimeout(scgp, 1000);

	if (scsi_flush_cache(scgp, FALSE) < 0) {
		printf(_("Trouble flushing the cache\n"));
		scg_settimeout(scgp, oldtimeout);
		return (-1);
	}
	waitformat(scgp, 100);
	trackno = trackp->trackno;
	if (trackno <= 0)
		trackno = 1;

	scg_settimeout(scgp, oldtimeout);

	if (dp->cdr_dstat->ds_type == DST_DVD_RAM) {
		/*
		 * XXX make sure we do not forget DVD-RAM once we did add
		 * XXX support for multi-border recording.
		 */
		return (ret);
	}
	if (track_base(trackp)->tracktype & TOCF_MULTI)
		scsi_close_tr_session(scgp, CL_TYPE_SESSION, trackno, TRUE);

	return (ret);
}

LOCAL int
blank_dvd(scgp, dp, addr, blanktype)
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

	mmc_check(scgp, &cdrr, &cdwr, &cdrrw, &cdwrw, NULL, &dvdwr);
	/*
	 * If the drive supports MMC-3, check for a DVD-RW medium.
	 */
	profile = get_curprofile(scgp);
	if (profile > 0)
		dvdwr = (profile == 0x13) || (profile == 0x14) ||
			(profile == 0x17);

	if (profile == 0x12)	/* DVD-RAM */
		return (blank_simul(scgp, dp, addr, blanktype));

	if (!dvdwr)
		return (blank_dummy(scgp, dp, addr, blanktype));

	if (lverbose) {
		printf(_("Blanking %s\n"), blank_types[blanktype & 0x07]);
		flush();
	}

	return (scsi_blank(scgp, addr, blanktype, FALSE));
}

LOCAL int
stats_dvd(scgp, dp)
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

#ifdef	__needed__
LOCAL int
read_rzone_info(scgp, bp, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x52;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
/*	g1_cdbaddr(&scmd->cdb.g1_cdb, addr);*/
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "read rzone info";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

LOCAL int
reserve_rzone(scgp, size)
	SCSI	*scgp;
	long	size;		/* number of blocks */
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)0;
	scmd->size = 0;
	scmd->flags = SCG_DISRE_ENA|SCG_CMD_RETRY;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x53;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);

	i_to_4_byte(&scmd->cdb.g1_cdb.addr[3], size);

	scgp->cmdname = "reserve_rzone";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}
#endif

#ifdef	_is_this_pioneer_vendor_unique_
LOCAL int
send_dvd_structure(scgp, bp, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->timeout = 4 * 60;		/* Needs up to 2 minutes ??? */
	scmd->cdb.g5_cdb.cmd = 0xFB;
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	g5_cdblen(&scmd->cdb.g5_cdb, cnt);

	scgp->cmdname = "read dvd structure";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}
#endif

#ifdef	__needed__
LOCAL int
set_layerbreak(scgp, tsize, lbreak)
	SCSI	*scgp;
	long	tsize;
	Int32_t	lbreak;
{
#ifdef	USE_STRUC_22
	struct dvd_structure_22	jz;
#endif
	struct dvd_structure_23	lb;
	int			ret;
	UInt32_t		dsize;
#ifdef	USE_STRUC_22
	UInt32_t		jump_size;
#endif
	UInt32_t		jump_lba;

#ifdef	USE_STRUC_22
	/*
	 * Jump interval size 0x22
	 */
	fillbytes((caddr_t)&jz, sizeof (jz), '\0');
	ret = read_dvd_structure(scgp, (caddr_t)&jz, sizeof (jz), 0, 0, 0, 0x22);
	if (ret < 0)
		return (ret);

	jump_size = a_to_u_4_byte(jz.jump_interval_size);

	if (jump_size != 0) {

#ifdef	AAAA
		dsize = roundup(tsize, 16);
		jump_lba = dsize / 2;
		jump_lba = roundup(jump_lba, 16);
/*		jump_lba -= 1;*/
		error("jump lba %d\n", jump_lba);
		i_to_4_byte(lb.jump_lba, jump_lba);
#else
		i_to_4_byte(jz.jump_interval_size, 0);
#endif
		ret = send_dvd_structure(scgp, (caddr_t)&jz, sizeof (jz), 0x22);
		if (ret < 0)
			return (ret);
	}
#endif	/* USE_STRUC_22 */

	/*
	 * Jump logical block address 0x23
	 */
	fillbytes((caddr_t)&lb, sizeof (lb), '\0');
	ret = read_dvd_structure(scgp, (caddr_t)&lb, sizeof (lb), 0, 0, 0, 0x23);
	if (ret < 0)
		return (ret);

	jump_lba = a_to_u_4_byte(lb.jump_lba);
	if (lbreak > 0 && lbreak > jump_lba) {
		errmsgno(EX_BAD, _("Manual layer break %d > %u not allowed.\n"),
							lbreak, jump_lba);
		return (-1);
	}
	dsize = roundup(tsize, 16);
	if (lbreak <= 0 && dsize <= (jump_lba+1)) {
		/*
		 * Allow to write DL media with less than single layer size
		 * in case of manual layer break set up.
		 */
		errmsgno(EX_BAD,
			_("Layer 0 size %u is bigger than expected disk size %u.\n"),
			(jump_lba+1), dsize);
		errmsgno(EX_BAD, _("Use single layer medium.\n"));
		return (-1);
	}
	jump_lba = dsize / 2;
	jump_lba = roundup(jump_lba, 16);
	if (lbreak > 0 && lbreak < jump_lba) {
		errmsgno(EX_BAD, _("Manual layer break %d < %u not allowed.\n"),
							lbreak, jump_lba);
		return (-1);
	}
	if (lbreak > 0)
		jump_lba = lbreak;
	jump_lba -= 1;
	i_to_4_byte(lb.jump_lba, jump_lba);

	ret = send_dvd_structure(scgp, (caddr_t)&lb, sizeof (lb), 0x23);
	return (ret);
}
#endif

LOCAL	char	ill_booktype[] = "reserved book type";
char	*book_types[] = {
	"DVD-ROM",
	"DVD-RAM",
	"DVD-R",
	"DVD-RW",
	"HD DVD-ROM",
	"HD DVD-RAM",
	"HD DVD-R",
	ill_booktype,
	ill_booktype,
	"DVD+RW",
	"DVD+R",
	ill_booktype,
	ill_booktype,
	"DVD+RW/DL",
	"DVD+R/DL",
	ill_booktype,
};

LOCAL	char	res_bvers[] = "reserved book version";
LOCAL char	*R_vers[] = {
	"0.9x",
	"1.0x",
	"1.1x",
	res_bvers,
	"1.9x",
	"2.0x",
	"> 2.0x",
	res_bvers,
	res_bvers,
	res_bvers,
	res_bvers,
	res_bvers,
	res_bvers,
	res_bvers,
	res_bvers,
	res_bvers,
};

LOCAL char	*RW_vers[] = {
	"0.9x",
	"1.0x",
	"1.1x",
	"> 1.1x",
	res_bvers,
	res_bvers,
	res_bvers,
	res_bvers,
	res_bvers,
	res_bvers,
	res_bvers,
	res_bvers,
	res_bvers,
	res_bvers,
	res_bvers,
	res_bvers,
};

LOCAL	char	ill_dsize[] = "illegal size";
char	*disc_sizes[] = {
	"120mm",
	"80mm",
	ill_dsize,
	ill_dsize,
	ill_dsize,
	ill_dsize,
	ill_dsize,
	ill_dsize,
	ill_dsize,
	ill_dsize,
	ill_dsize,
	ill_dsize,
	ill_dsize,
	ill_dsize,
	ill_dsize,
	ill_dsize,
};

LOCAL	char	ill_rate[] = "illegal rate";
char	*tr_rates[] = {
	"2.52 MB/s",
	"5.04 MB/s",
	"10.08 MB/s",
	"20.16 MB/s",
	"30.24 MB/s",
	ill_rate,
	ill_rate,
	ill_rate,
	ill_rate,
	ill_rate,
	ill_rate,
	ill_rate,
	ill_rate,
	ill_rate,
	ill_rate,
	"Not specified",
};

LOCAL	char	ill_layer[] = "illegal layer type";
char	*layer_types[] = {
	"Embossed Data",
	"Recordable Area",
	"Rewritable Area",
	ill_layer,
	ill_layer,
	ill_layer,
	ill_layer,
	ill_layer,
	ill_layer,
	ill_layer,
	ill_layer,
	ill_layer,
	ill_layer,
	ill_layer,
	ill_layer,
	ill_layer,
};

LOCAL	char	ill_dens[] = "illegal density";
char	*ldensities[] = {
	"0.267 µm/bit",
	"0.293 µm/bit",
	"0.409-0.435 µm/bit",
	"0.280-0.291 µm/bit",
	"0.353 µm/bit",
	ill_dens,
	ill_dens,
	ill_dens,
	ill_dens,
	ill_dens,
	ill_dens,
	ill_dens,
	ill_dens,
	ill_dens,
	ill_dens,
	ill_dens,
};

char	*tdensities[] = {
	"0.74 µm/track",
	"0.80 µm/track",
	"0.615 µm/track",
	"0.40 µm/track",
	"0.34 µm/track",
	ill_dens,
	ill_dens,
	ill_dens,
	ill_dens,
	ill_dens,
	ill_dens,
	ill_dens,
	ill_dens,
	ill_dens,
	ill_dens,
	ill_dens,
};

LOCAL void
print_dvd00(dp)
	struct dvd_structure_00 *dp;
{
	int	len = a_to_2_byte(dp->data_len)+2;
	long	lbr;
	char	*vers = "";
	char	*ext_vers = "";
	char	ev[8];
	int	evers = 0;

	ev[0] = '\0';
	if (len >= (27+4))
		evers = ((char *)dp)[27+4] & 0xFF;

	if (dp->book_type == 2) {		/* DVD-R */
		vers = R_vers[dp->book_version];
		if ((dp->book_version == 5 ||
		    dp->book_version == 6) &&
		    evers != 0) {
			snprintf(ev, sizeof (ev),
					" -> %d.%d",
					(evers >> 4) & 0x0F,
					evers & 0x0F);
			ext_vers = ev;
		}

	} else if (dp->book_type == 3) {	/* DVD-RW */
		vers = RW_vers[dp->book_version];
		if ((dp->book_version == 2 ||
		    dp->book_version == 3) &&
		    evers != 0) {
			snprintf(ev, sizeof (ev),
					" -> %d.%d",
					(evers >> 4) & 0x0F,
					evers & 0x0F);
			ext_vers = ev;
		}
	}

	printf(_("book type:       %s, Version %s%s%s(%d.%d)\n"),
					book_types[dp->book_type],
					vers, ext_vers, *vers ? " ":"",
					dp->book_type,
					dp->book_version);
	printf(_("disc size:       %s (%d)\n"), disc_sizes[dp->disc_size], dp->disc_size);
	printf(_("maximum rate:    %s (%d)\n"), tr_rates[dp->maximum_rate], dp->maximum_rate);
	printf(_("number of layers:%d\n"), dp->numlayers+1);
	printf(_("track path:      %s Track Path (%d)\n"),
					dp->track_path?_("Opposite"):_("Parallel"),
					dp->track_path);
	printf(_("layer type:      %s (%d)\n"), layer_types[dp->layer_type],
					dp->layer_type);
	printf(_("linear density:  %s (%d)\n"), ldensities[dp->linear_density],
					dp->linear_density);
	printf(_("track density:   %s (%d)\n"), tdensities[dp->track_density],
					dp->track_density);
	printf(_("phys start:      %ld (0x%lX) \n"),
					a_to_u_3_byte(dp->phys_start),
					a_to_u_3_byte(dp->phys_start));
	printf(_("phys end:        %ld\n"), a_to_u_3_byte(dp->phys_end));
	printf(_("end layer 0:     %ld\n"), a_to_u_3_byte(dp->end_layer0));
	printf(_("bca:             %d\n"), dp->bca);
	printf(_("phys size:...    %ld\n"), a_to_u_3_byte(dp->phys_end) - a_to_u_3_byte(dp->phys_start) + 1);
	lbr = a_to_u_3_byte(dp->end_layer0) - a_to_u_3_byte(dp->phys_start) + 1;
	if (lbr > 0)
		printf(_("layer break at:  %ld\n"), lbr);
}

LOCAL void
print_dvd01(dp)
	struct dvd_structure_01 *dp;
{
	printf(_("copyr prot type: %d\n"), dp->copyr_prot_type);
	printf(_("region mgt info: %d\n"), dp->region_mgt_info);
}

LOCAL void
print_dvd04(dp)
	struct dvd_structure_04 *dp;
{
	if (cmpnullbytes(dp->man_info, sizeof (dp->man_info)) <
						sizeof (dp->man_info)) {
		printf(_("Manufacturing info: '%.2048s'\n"), dp->man_info);
	}
}

LOCAL void
print_dvd05(dp)
	struct dvd_structure_05 *dp;
{
	printf(_("cpm:             %d\n"), dp->cpm);
	printf(_("cgms:            %d\n"), dp->cgms);
}

LOCAL void
print_dvd0D(dp)
	struct dvd_structure_0D *dp;
{
	printf(_("last rma sector: %d\n"), a_to_u_2_byte(dp->last_rma_sector));
}

LOCAL void
print_dvd0E(dp)
	struct dvd_structure_0E *dp;
{
	int	i;
	int	len = 44;
	int	c;
	char	*p = (char *)dp;

	if (dp->field_id != 1)
	printf(_("field id:        %d\n"), dp->field_id);
	printf(_("application code:%d\n"), dp->application_code);
	printf(_("physical code:   %d\n"), dp->phys_data);
	printf(_("last rec address:%ld\n"), a_to_u_3_byte(dp->last_recordable_addr));
	printf(_("part v./ext code:%X/%X\n"), (Uint)(dp->res_a[0] & 0xF0) >> 4,
						dp->res_a[0] & 0xF);

	if (dp->field_id_2 != 2)
	printf(_("field id2:       %d\n"), dp->field_id_2);
	printf(_("ind wr. power:   %d\n"), dp->ind_wr_power);
	printf(_("wavelength code: %d\n"), dp->ind_wavelength);
	scg_fprbytes(stdout, _("write str. code:"), dp->opt_wr_strategy, 4);

	if (dp->field_id_3 != 3)
	printf(_("field id3:       %d\n"), dp->field_id_3);
	if (dp->field_id_4 != 4)
	printf(_("field id4:       %d\n"), dp->field_id_4);

	printf(_("Manufacturer:   '"));
	for (i = 0; i < 6; i++) {
		c = dp->man_id[i];
		if (c >= ' ' && c < 0177)
			printf("%c", c);
		else if (c != 0)
			printf(".");
	}
	for (i = 0; i < 6; i++) {
		c = dp->man_id2[i];
		if (c >= ' ' && c < 0177)
			printf("%c", c);
		else if (c != 0)
			printf(".");
	}
	printf("'\n");
	/*
	 * Next field: opt wr str. II or Manufacturer part III
	 */
/*	scg_prbytes("write str. code:", dp->opt_wr_strategy, 4);*/

	if (lverbose <= 1)
		return;
	printf(_("Prerecorded info   : "));
	for (i = 0; i < len; i++) {
		c = p[i];
		if (c >= ' ' && c < 0177)
			printf("%c", c);
		else
			printf(".");
	}
	printf("\n");
}

LOCAL void
print_dvd0F(dp)
	struct dvd_structure_0F *dp;
{
	printf(_("random:          %d\n"), a_to_u_2_byte(dp->random));
	printf(_("year:            %.4s\n"), dp->year);
	printf(_("month:           %.2s\n"), dp->month);
	printf(_("day:             %.2s\n"), dp->day);
	printf(_("hour:            %.2s\n"), dp->hour);
	printf(_("minute:          %.2s\n"), dp->minute);
	printf(_("second:          %.2s\n"), dp->second);
}


#ifdef	__needed__
LOCAL void
send_dvd0F(scgp)
	SCSI	*scgp;
{
	struct dvd_structure_0F_w d;

	strncpy((char *)d.year,		"1998", 4);
	strncpy((char *)d.month,	"05", 2);
	strncpy((char *)d.day,		"12", 2);
	strncpy((char *)d.hour,		"22", 2);
	strncpy((char *)d.minute,	"59", 2);
	strncpy((char *)d.second,	"00", 2);
/*	send_dvd_structure(scgp, (caddr_t)&d, sizeof (d));*/
}
#endif

LOCAL void
print_dvd20(dp)
	struct dvd_structure_20 *dp;
{
	printf(_("L0 init status:  %d\n"), dp->res47[0] & 0x80 ? 1 : 0);
	printf(_("L0 data areacap: %ld\n"), a_to_u_4_byte(dp->l0_area_cap));
}

LOCAL void
print_dvd22(dp)
	struct dvd_structure_22 *dp;
{
	printf(_("Jump intervalsz: %ld\n"), a_to_u_4_byte(dp->jump_interval_size));
}

LOCAL void
print_dvd23(dp)
	struct dvd_structure_23 *dp;
{
	printf(_("Jump LBA:        %ld\n"), a_to_u_4_byte(dp->jump_lba));
}


#include "adip.h"
/*LOCAL void*/
EXPORT void
print_dvd_info(scgp)
	SCSI	*scgp;
{
	Uchar	mode[4096];
	int	ret;
	int	i;

	if (lverbose > 2)
		printf(_("Entering DVD info....\n"));
	/*
	 * The ACARD TECH AEC-7720 ATAPI<->SCSI adaptor
	 * chokes if we try to transfer odd byte counts (rounds up to
	 * even byte counts and thus causes a DMA overflow and a
	 * bus reset), so make the byte count even.
	 * It also chokes if we try to transfer more than 0x40 bytes with
	 * mode_sense of all pages. As we don't want to avoid this
	 * command here, we need to wait until the drive recoveres
	 * from the reset?? it receives after the adapter dies.
	 */
/*	mode_sense(scgp, mode, 255, 0x3F, 0);*/
/*	if (lverbose > 2)*/
/*		scg_prbytes("Mode: ", mode, 255 - scg_getresid(scgp));*/

	if (lverbose > 1)
		mode_sense(scgp, mode, 250, 0x3F, 0);
	if (lverbose > 2)
		scg_prbytes(_("Mode: "), mode, 250 - scg_getresid(scgp));
	wait_unit_ready(scgp, 120);
	if (lverbose > 1) {
		printf(_("Supported DVD (readable) structures:"));
		scgp->silent++;
		for (i = 0; i <= 255; i++) {
			fillbytes((caddr_t)mode, sizeof (mode), '\0');
			ret = read_dvd_structure(scgp, (caddr_t)mode, sizeof (mode), 0, 0, 0, i);
			if (ret >= 0 && (sizeof (mode) - scg_getresid(scgp)) > 4)
				printf(" %02X", i);
		}
		scgp->silent--;
		printf("\n");
/*		printf("Page: %d ret: %d len: %d\n", i, ret, sizeof (mode) - scg_getresid(scgp));*/
		if (lverbose > 2)
			scg_prbytes(_("Page FF: "), mode, sizeof (mode) - scg_getresid(scgp));
		if (sizeof (mode) - scg_getresid(scgp) > 4) {
			int	len = a_to_u_2_byte(mode) - 2;
			Uchar	*p = &mode[4];
			int	m;

			len /= 4;
			for (i = 0; i < len; i++) {
				m = p[1] & 0xC0;
				printf(_("Page %02X %s  (%02X) len %d\n"),
					*p & 0xFF,
					m == 0xC0 ?
					_("read/write") :
					(m == 0x80 ? _("     write") :
					(m == 0x40 ? _("read      ") : _("unknown   "))),
					p[1] & 0xFF,
					a_to_u_2_byte(&p[2]));
				p += 4;
			}
		}
	}
	wait_unit_ready(scgp, 120);

	/*
	 * Physical Format information 0x00
	 */
	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	scgp->silent++;
	ret = read_dvd_structure(scgp, (caddr_t)mode, sizeof (mode), 0, 0, 0, 0);
	scgp->silent--;
	if (ret >= 0) {
		if (lverbose > 2) {
			scg_prbytes(_("DVD structure[0]: "),
				mode, sizeof (mode) - scg_getresid(scgp));
/*			scg_prascii("DVD structure[0]: ", mode, sizeof (mode) - scg_getresid(scgp));*/
		}
		print_dvd00((struct dvd_structure_00 *)mode);
		ret = get_curprofile(scgp);
		if (ret == 0x001A || ret == 0x001B) {
			/*profile >= 0x0018 && profile < 0x0020*/
			printf(_("Manufacturer:    '%.8s'\n"), &mode[23]);
			printf(_("Media type:      '%.3s'\n"), &mode[23+8]);
		}
	}

	/*
	 * ADIP information 0x11
	 */
	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	scgp->silent++;
	ret = read_dvd_structure(scgp, (caddr_t)mode, sizeof (mode), 0, 0, 0, 0x11);
	scgp->silent--;
	if (ret >= 0) {
		adip_t	*adp;
		if (lverbose > 2) {
			scg_prbytes(_("DVD structure[11]: "),
				mode, sizeof (mode) - scg_getresid(scgp));
			scg_prascii(_("DVD structure[11]: "),
				mode, sizeof (mode) - scg_getresid(scgp));
		}
/*		print_dvd0F((struct dvd_structure_0F *)mode);*/
		adp = (adip_t *)&mode[4];
#ifndef	offsetof
#define	offsetof(TYPE, MEMBER)  ((size_t) &((TYPE *)0)->MEMBER)
#endif
/*		printf("size %d %d\n", sizeof (adip_t), offsetof(adip_t, res_controldat));*/
		printf(_("Category/Version	%02X\n"), adp->cat_vers);
		printf(_("Disk size		%02X\n"), adp->disk_size);
		printf(_("Disk structure		%02X\n"), adp->disk_struct);
		printf(_("Recoding density	%02X\n"), adp->density);

		printf(_("Manufacturer:		'%.8s'\n"), adp->man_id);
		printf(_("Media type:		'%.3s'\n"), adp->media_id);
		printf(_("Product revision	%u\n"), adp->prod_revision);
		printf(_("ADIP numbytes		%u\n"), adp->adip_numbytes);
		printf(_("Reference speed		%u\n"), adp->ref_speed);
		printf(_("Max speed		%u\n"), adp->max_speed);
	}

	/*
	 * Layer boundary information 0x20
	 */
	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	scgp->silent++;
	ret = read_dvd_structure(scgp, (caddr_t)mode, sizeof (mode), 0, 0, 0, 0x20);
	scgp->silent--;
	if (ret >= 0) {
		if (lverbose > 2) {
			scg_prbytes(_("DVD structure[20]: "),
				mode, sizeof (mode) - scg_getresid(scgp));
			scg_prascii(_("DVD structure[20]: "),
				mode, sizeof (mode) - scg_getresid(scgp));
		}
		print_dvd20((struct dvd_structure_20 *)mode);
	}

	/*
	 * Jump interval size 0x22
	 */
	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	scgp->silent++;
	ret = read_dvd_structure(scgp, (caddr_t)mode, sizeof (mode), 0, 0, 0, 0x22);
	scgp->silent--;
	if (ret >= 0) {
		if (lverbose > 2) {
			scg_prbytes(_("DVD structure[22]: "),
				mode, sizeof (mode) - scg_getresid(scgp));
			scg_prascii(_("DVD structure[22]: "),
				mode, sizeof (mode) - scg_getresid(scgp));
		}
		print_dvd22((struct dvd_structure_22 *)mode);
	}

	/*
	 * Jump logical block address 0x23
	 */
	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	scgp->silent++;
	ret = read_dvd_structure(scgp, (caddr_t)mode, sizeof (mode), 0, 0, 0, 0x23);
	scgp->silent--;
	if (ret >= 0) {
		if (lverbose > 2) {
			scg_prbytes(_("DVD structure[23]: "),
				mode, sizeof (mode) - scg_getresid(scgp));
			scg_prascii(_("DVD structure[23]: "),
				mode, sizeof (mode) - scg_getresid(scgp));
		}
		print_dvd23((struct dvd_structure_23 *)mode);
	}

	/*
	 * Copyright information 0x01
	 */
	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	scgp->silent++;
	ret = read_dvd_structure(scgp, (caddr_t)mode, sizeof (mode), 0, 0, 0, 1);
	scgp->silent--;
	if (ret >= 0) {
		if (lverbose > 2) {
			scg_prbytes(_("DVD structure[1]: "),
				mode, sizeof (mode) - scg_getresid(scgp));
		}
		print_dvd01((struct dvd_structure_01 *)mode);
	}

	/*
	 * Manufacturer information 0x04
	 */
	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	scgp->silent++;
	ret = read_dvd_structure(scgp, (caddr_t)mode, sizeof (mode), 0, 0, 0, 4);
	scgp->silent--;
	if (ret >= 0) {
		if (lverbose > 2) {
			scg_prbytes(_("DVD structure[4]: "),
				mode, sizeof (mode) - scg_getresid(scgp));
		}
		print_dvd04((struct dvd_structure_04 *)mode);
	}

	/*
	 * Copyright management information 0x05
	 */
	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	scgp->silent++;
	ret = read_dvd_structure(scgp, (caddr_t)mode, sizeof (mode), 0, 0, 0, 5);
	scgp->silent--;
	if (ret >= 0) {
		if (lverbose > 2) {
			scg_prbytes(_("DVD structure[5]: "),
				mode, sizeof (mode) - scg_getresid(scgp));
		}
		print_dvd05((struct dvd_structure_05 *)mode);
	}

	/*
	 * Recording Management Area Data information 0x0D
	 */
	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	scgp->silent++;
	ret = read_dvd_structure(scgp, (caddr_t)mode, sizeof (mode), 0, 0, 0, 0xD);
	scgp->silent--;
	if (ret >= 0) {
		if (lverbose > 2) {
			scg_prbytes(_("DVD structure[D]: "),
				mode, sizeof (mode) - scg_getresid(scgp));
		}
		print_dvd0D((struct dvd_structure_0D *)mode);
	}

	/*
	 * Prerecorded information 0x0E
	 */
	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	scgp->silent++;
	ret = read_dvd_structure(scgp, (caddr_t)mode, sizeof (mode), 0, 0, 0, 0xE);
	scgp->silent--;
	if (ret >= 0) {
		if (lverbose > 2) {
			scg_prbytes(_("DVD structure[E]: "),
				mode, sizeof (mode) - scg_getresid(scgp));
		}
		print_dvd0E((struct dvd_structure_0E *)mode);
	}

	if (lverbose <= 1)
		return;

	/*
	 * Unique Disk identifier 0x0F
	 */
/*	send_dvd0F();*/
	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	scgp->silent++;
	ret = read_dvd_structure(scgp, (caddr_t)mode, sizeof (mode), 0, 0, 0, 0xF);
	scgp->silent--;
	if (ret >= 0) {
		if (lverbose > 2) {
			scg_prbytes(_("DVD structure[F]: "),
				mode, sizeof (mode) - scg_getresid(scgp));
		}
		print_dvd0F((struct dvd_structure_0F *)mode);
	}

	fillbytes((caddr_t)mode, sizeof (mode), '\0');
	read_rzone_info(scgp, (caddr_t)mode, sizeof (mode));
	if (lverbose > 2)
		scg_prbytes(_("Rzone info: "), mode, sizeof (mode) - scg_getresid(scgp));
	przone((struct rzone_info *)mode);

	scgp->verbose++;
	log_sense(scgp, (caddr_t)mode, 255, 0x3, 1, 0);
	log_sense(scgp, (caddr_t)mode, 255, 0x31, 1, 0);
	scgp->verbose--;

	if (lverbose > 2)
		printf(_("Leaving DVD info.\n"));
}

LOCAL void
print_laserlog(scgp)
	SCSI	*scgp;
{
	Uchar	log[256];
	Uchar	*p;
	int	len = sizeof (log);
	long	val;

	if (!has_log_page(scgp, 0x30, LOG_CUMUL))
		return;

	p = log + sizeof (scsi_log_hdr);

	/*
	 * This is Pioneer specific so other drives will not have it.
	 * Not all values may be available with newer Pioneer drives.
	 */
	scgp->silent++;
	fillbytes((caddr_t)log, sizeof (log), '\0');
	if (get_log(scgp, (caddr_t)log, &len, 0x30, LOG_CUMUL, 0) < 0) {
		scgp->silent--;
		return;
	}
	scgp->silent--;

	val = a_to_u_4_byte(((struct pioneer_logpage_30_0 *)p)->total_poh);
	if (((struct scsi_logp_header *)log)->p_len > 0)
		printf(_("Total power on  hours: %ld\n"), val);

	scgp->silent++;
	fillbytes((caddr_t)log, sizeof (log), '\0');
	if (get_log(scgp, (caddr_t)log, &len, 0x30, LOG_CUMUL, 1) < 0) {
		scgp->silent--;
		return;
	}
	scgp->silent--;

	val = a_to_u_4_byte(((struct pioneer_logpage_30_1 *)p)->laser_poh);
	if (((struct scsi_logp_header *)log)->p_len > 0)
		printf(_("Total laser on  hours: %ld\n"), val);

	scgp->silent++;
	fillbytes((caddr_t)log, sizeof (log), '\0');
	if (get_log(scgp, (caddr_t)log, &len, 0x30, LOG_CUMUL, 2) < 0) {
		scgp->silent--;
		return;
	}
	scgp->silent--;

	val = a_to_u_4_byte(((struct pioneer_logpage_30_2 *)p)->record_poh);
	if (((struct scsi_logp_header *)log)->p_len > 0)
		printf(_("Total recording hours: %ld\n"), val);
}
