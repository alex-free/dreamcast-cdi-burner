/* @(#)drv_mmc.c	1.200 12/03/16 Copyright 1997-2012 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)drv_mmc.c	1.200 12/03/16 Copyright 1997-2012 J. Schilling";
#endif
/*
 *	CDR device implementation for
 *	SCSI-3/mmc conforming drives
 *	e.g. Yamaha CDR-400, Ricoh MP6200
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

/*#define	DEBUG*/
#define	PRINT_ATIP
#include <schily/mconfig.h>

#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/fcntl.h>
#include <schily/errno.h>
#include <schily/string.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
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
#include "mmcvendor.h"
#include "cdrecord.h"

extern	char	*driveropts;

extern	int	debug;
extern	int	lverbose;
extern	int	xdebug;

LOCAL	int	curspeed = 1;

LOCAL	char	clv_to_speed[16] = {
/*		0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 */
		0, 2, 4, 6, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

LOCAL	char	hs_clv_to_speed[16] = {
/*		0  1  2  3  4  5  6  7   8  9 10 11 12 13 14 15 */
		0, 2, 4, 6, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

LOCAL	char	us_clv_to_speed[16] = {
/*		0  1  2  3  4  5   6  7  8   9   10  11 12 13 14 15 */
		0, 2, 4, 8, 0, 0, 16, 0, 24, 32, 40, 48, 0, 0, 0, 0
};

#ifdef	__needed__
LOCAL	int	mmc_load		__PR((SCSI *scgp, cdr_t *dp));
LOCAL	int	mmc_unload		__PR((SCSI *scgp, cdr_t *dp));
#endif
EXPORT	void	mmc_opthelp		__PR((SCSI *scgp, cdr_t *dp, int excode));
EXPORT	char	*hasdrvopt		__PR((char *optstr, char *optname));
EXPORT	char	*hasdrvoptx		__PR((char *optstr, char *optname, int flag));
LOCAL	cdr_t	*identify_mmc		__PR((SCSI *scgp, cdr_t *, struct scsi_inquiry *));
LOCAL	int	attach_mmc		__PR((SCSI *scgp, cdr_t *));
EXPORT	int	check_writemodes_mmc	__PR((SCSI *scgp, cdr_t *dp));
LOCAL	int	deflt_writemodes_mmc	__PR((SCSI *scgp, BOOL reset_dummy));
LOCAL	void	di_to_dstat		__PR((struct disk_info *dip, dstat_t *dsp));
LOCAL	int	get_atip		__PR((SCSI *scgp, struct atipinfo *atp));
#ifdef	PRINT_PMA
LOCAL	int	get_pma			__PR((SCSI *scgp));
#endif
LOCAL	int	init_mmc		__PR((SCSI *scgp, cdr_t *dp));
LOCAL	int	getdisktype_mmc		__PR((SCSI *scgp, cdr_t *dp));
LOCAL	int	prdiskstatus_mmc	__PR((SCSI *scgp, cdr_t *dp));
LOCAL	int	speed_select_mmc	__PR((SCSI *scgp, cdr_t *dp, int *speedp));
LOCAL	int	mmc_set_speed		__PR((SCSI *scgp, int readspeed, int writespeed, int rotctl));
LOCAL	int	next_wr_addr_mmc	__PR((SCSI *scgp, track_t *trackp, long *ap));
LOCAL	int	write_leadin_mmc	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	open_track_mmc		__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	close_track_mmc		__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	open_session_mmc	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	waitfix_mmc		__PR((SCSI *scgp, int secs));
LOCAL	int	fixate_mmc		__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	blank_mmc		__PR((SCSI *scgp, cdr_t *dp, long addr, int blanktype));
LOCAL	int	send_opc_mmc		__PR((SCSI *scgp, caddr_t, int cnt, int doopc));
LOCAL	int	opt1_mmc		__PR((SCSI *scgp, cdr_t *dp));
LOCAL	int	opt2_mmc		__PR((SCSI *scgp, cdr_t *dp));
LOCAL	int	scsi_sony_write		__PR((SCSI *scgp, caddr_t bp, long sectaddr, long size, int blocks, BOOL islast));
LOCAL	int	gen_cue_mmc		__PR((track_t *trackp, void *vcuep, BOOL needgap));
LOCAL	void	fillcue			__PR((struct mmc_cue *cp, int ca, int tno, int idx, int dataform, int scms, msf_t *mp));
LOCAL	int	send_cue_mmc		__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL int	stats_mmc		__PR((SCSI *scgp, cdr_t *dp));
LOCAL BOOL	mmc_isplextor		__PR((SCSI *scgp));
LOCAL BOOL	mmc_isyamaha		__PR((SCSI *scgp));
LOCAL void	do_varirec_plextor	__PR((SCSI *scgp));
LOCAL int	do_gigarec_plextor	__PR((SCSI *scgp));
LOCAL int	drivemode_plextor	__PR((SCSI *scgp, caddr_t bp, int cnt, int modecode, void *modeval));
LOCAL int	drivemode2_plextor	__PR((SCSI *scgp, caddr_t bp, int cnt, int modecode, void *modeval));
LOCAL int	check_varirec_plextor	__PR((SCSI *scgp));
LOCAL int	check_gigarec_plextor	__PR((SCSI *scgp));
LOCAL int	varirec_plextor		__PR((SCSI *scgp, BOOL on, int val));
LOCAL int	gigarec_plextor		__PR((SCSI *scgp, int val));
LOCAL Int32_t	gigarec_mult		__PR((int code, Int32_t	val));
LOCAL int	check_ss_hide_plextor	__PR((SCSI *scgp));
LOCAL int	check_speed_rd_plextor	__PR((SCSI *scgp));
LOCAL int	check_powerrec_plextor	__PR((SCSI *scgp));
LOCAL int	ss_hide_plextor		__PR((SCSI *scgp, BOOL do_ss, BOOL do_hide));
LOCAL int	speed_rd_plextor	__PR((SCSI *scgp, BOOL do_speedrd));
LOCAL int	powerrec_plextor	__PR((SCSI *scgp, BOOL do_powerrec));
LOCAL int	get_speeds_plextor	__PR((SCSI *scgp, int *selp, int *maxp, int *lastp));
LOCAL int	bpc_plextor		__PR((SCSI *scgp, int mode, int *bpp));
LOCAL int	plextor_enable		__PR((SCSI *scgp));
LOCAL int	plextor_disable		__PR((SCSI *scgp));
LOCAL int	plextor_getauth		__PR((SCSI *scgp, void *dp, int cnt));
LOCAL int	plextor_setauth		__PR((SCSI *scgp, void *dp, int cnt));
LOCAL int	set_audiomaster_yamaha	__PR((SCSI *scgp, cdr_t *dp, BOOL keep_mode));

EXPORT struct ricoh_mode_page_30 * get_justlink_ricoh	__PR((SCSI *scgp, Uchar *mode));
LOCAL int	force_speed_yamaha	__PR((SCSI *scgp, int readspeed, int writespeed));
LOCAL BOOL	get_tattoo_yamaha	__PR((SCSI *scgp, BOOL print, Int32_t *irp, Int32_t *orp));
LOCAL int	do_tattoo_yamaha	__PR((SCSI *scgp, FILE *f));
LOCAL int	yamaha_write_buffer	__PR((SCSI *scgp, int mode, int bufferid, long offset,
							long parlen, void *buffer, long buflen));

#ifdef	__needed__
LOCAL int
mmc_load(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	return (scsi_load_unload(scgp, 1));
}

LOCAL int
mmc_unload(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	return (scsi_load_unload(scgp, 0));
}
#endif

/*
 * MMC CD-writer
 */
cdr_t	cdr_mmc = {
	0, 0, 0,
	CDR_SWABAUDIO,
	0,
	CDR_CDRW_ALL,
	WM_SAO,
	372, 372,
	"mmc_cdr",
	"generic SCSI-3/mmc   CD-R/CD-RW driver",
	0,
	(dstat_t *)0,
	identify_mmc,
	attach_mmc,
	init_mmc,
	getdisktype_mmc,
	prdiskstatus_mmc,
	scsi_load,
	scsi_unload,
	read_buff_cap,
	cmd_dummy,					/* recovery_needed */
	(int(*)__PR((SCSI *, cdr_t *, int)))cmd_dummy,	/* recover	*/
	speed_select_mmc,
	select_secsize,
	next_wr_addr_mmc,
	(int(*)__PR((SCSI *, Ulong)))cmd_ill,	/* reserve_track	*/
	scsi_cdr_write,
	gen_cue_mmc,
	send_cue_mmc,
	write_leadin_mmc,
	open_track_mmc,
	close_track_mmc,
	open_session_mmc,
	cmd_dummy,
	cmd_dummy,					/* abort	*/
	read_session_offset,
	fixate_mmc,
	stats_mmc,
	blank_mmc,
	format_dummy,
	send_opc_mmc,
	opt1_mmc,
	opt2_mmc,
};

/*
 * Sony MMC CD-writer
 */
cdr_t	cdr_mmc_sony = {
	0, 0, 0,
	CDR_SWABAUDIO,
	0,
	CDR_CDRW_ALL,
	WM_SAO,
	372, 372,
	"mmc_cdr_sony",
	"generic SCSI-3/mmc   CD-R/CD-RW driver (Sony 928 variant)",
	0,
	(dstat_t *)0,
	identify_mmc,
	attach_mmc,
	init_mmc,
	getdisktype_mmc,
	prdiskstatus_mmc,
	scsi_load,
	scsi_unload,
	read_buff_cap,
	cmd_dummy,					/* recovery_needed */
	(int(*)__PR((SCSI *, cdr_t *, int)))cmd_dummy,	/* recover	*/
	speed_select_mmc,
	select_secsize,
	next_wr_addr_mmc,
	(int(*)__PR((SCSI *, Ulong)))cmd_ill,	/* reserve_track	*/
	scsi_sony_write,
	gen_cue_mmc,
	send_cue_mmc,
	write_leadin_mmc,
	open_track_mmc,
	close_track_mmc,
	open_session_mmc,
	cmd_dummy,
	cmd_dummy,					/* abort	*/
	read_session_offset,
	fixate_mmc,
	cmd_dummy,				/* stats		*/
	blank_mmc,
	format_dummy,
	send_opc_mmc,
	opt1_mmc,
	opt2_mmc,
};

/*
 * SCSI-3/mmc conformant CD-ROM drive
 */
cdr_t	cdr_cd = {
	0, 0, 0,
	CDR_ISREADER|CDR_SWABAUDIO,
	0,
	CDR_CDRW_NONE,
	WM_NONE,
	372, 372,
	"mmc_cd",
	"generic SCSI-3/mmc   CD-ROM driver",
	0,
	(dstat_t *)0,
	identify_mmc,
	attach_mmc,
	cmd_dummy,
	drive_getdisktype,
	prdiskstatus_mmc,
	scsi_load,
	scsi_unload,
	read_buff_cap,
	cmd_dummy,					/* recovery_needed */
	(int(*)__PR((SCSI *, cdr_t *, int)))cmd_dummy,	/* recover	*/
	speed_select_mmc,
	select_secsize,
	(int(*)__PR((SCSI *scgp, track_t *, long *)))cmd_ill,	/* next_wr_addr		*/
	(int(*)__PR((SCSI *, Ulong)))cmd_ill,	/* reserve_track	*/
	scsi_cdr_write,
	(int(*)__PR((track_t *, void *, BOOL)))cmd_dummy,	/* gen_cue */
	no_sendcue,
	(int(*)__PR((SCSI *, cdr_t *, track_t *)))cmd_dummy, /* leadin */
	open_track_mmc,
	close_track_mmc,
	(int(*)__PR((SCSI *scgp, cdr_t *, track_t *)))cmd_dummy,
	cmd_dummy,
	cmd_dummy,					/* abort	*/
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
 * Old pre SCSI-3/mmc CD drive
 */
cdr_t	cdr_oldcd = {
	0, 0, 0,
	CDR_ISREADER,
	0,
	CDR_CDRW_NONE,
	WM_NONE,
	372, 372,
	"scsi2_cd",
	"generic SCSI-2       CD-ROM driver",
	0,
	(dstat_t *)0,
	identify_mmc,
	drive_attach,
	cmd_dummy,
	drive_getdisktype,
	prdiskstatus_mmc,
	scsi_load,
	scsi_unload,
	buf_dummy,
	cmd_dummy,					/* recovery_needed */
	(int(*)__PR((SCSI *, cdr_t *, int)))cmd_dummy,	/* recover	*/
	speed_select_mmc,
	select_secsize,
	(int(*)__PR((SCSI *scg, track_t *, long *)))cmd_ill,	/* next_wr_addr		*/
	(int(*)__PR((SCSI *, Ulong)))cmd_ill,	/* reserve_track	*/
	scsi_cdr_write,
	(int(*)__PR((track_t *, void *, BOOL)))cmd_dummy,	/* gen_cue */
	no_sendcue,
	(int(*)__PR((SCSI *, cdr_t *, track_t *)))cmd_dummy, /* leadin */
	open_track_mmc,
	close_track_mmc,
	(int(*)__PR((SCSI *scgp, cdr_t *, track_t *)))cmd_dummy,
	cmd_dummy,
	cmd_dummy,					/* abort	*/
	read_session_offset_philips,
	(int(*)__PR((SCSI *scgp, cdr_t *, track_t *)))cmd_dummy,	/* fixation */
	cmd_dummy,					/* stats	*/
	blank_dummy,
	format_dummy,
	(int(*)__PR((SCSI *, caddr_t, int, int)))NULL,	/* no OPC	*/
	cmd_dummy,					/* opt1		*/
	cmd_dummy,					/* opt2		*/
};

/*
 * SCSI-3/mmc conformant CD, DVD or BE writer
 * Checks the current medium and then returns either cdr_mmc or cdr_dvd
 */
cdr_t	cdr_cd_dvd = {
	0, 0, 0,
	CDR_SWABAUDIO,
	0,
	CDR_CDRW_ALL,
	WM_NONE,
	372, 372,
	"mmc_cd_dvd",
	"generic SCSI-3/mmc   CD/DVD/BD driver (checks media)",
	0,
	(dstat_t *)0,
	identify_mmc,
	attach_mmc,
	cmd_dummy,
	drive_getdisktype,
	no_diskstatus,
	scsi_load,
	scsi_unload,
	read_buff_cap,
	cmd_dummy,					/* recovery_needed */
	(int(*)__PR((SCSI *, cdr_t *, int)))cmd_dummy,	/* recover	*/
	speed_select_mmc,
	select_secsize,
	(int(*)__PR((SCSI *scgp, track_t *, long *)))cmd_ill,	/* next_wr_addr		*/
	(int(*)__PR((SCSI *, Ulong)))cmd_ill,	/* reserve_track	*/
	scsi_cdr_write,
	(int(*)__PR((track_t *, void *, BOOL)))cmd_dummy,	/* gen_cue */
	no_sendcue,
	(int(*)__PR((SCSI *, cdr_t *, track_t *)))cmd_dummy, /* leadin */
	open_track_mmc,
	close_track_mmc,
	(int(*)__PR((SCSI *scgp, cdr_t *, track_t *)))cmd_dummy,
	cmd_dummy,
	cmd_dummy,					/* abort	*/
	read_session_offset,
	(int(*)__PR((SCSI *scgp, cdr_t *, track_t *)))cmd_dummy,	/* fixation */
	cmd_dummy,					/* stats	*/
	blank_dummy,
	format_dummy,
	(int(*)__PR((SCSI *, caddr_t, int, int)))NULL,	/* no OPC	*/
	cmd_dummy,					/* opt1		*/
	cmd_dummy,					/* opt2		*/
};

EXPORT void
mmc_opthelp(scgp, dp, excode)
	SCSI	*scgp;
	cdr_t	*dp;
	int	excode;
{
	BOOL	haveopts = FALSE;

	error(_("Driver options:\n"));
	if (dp->cdr_flags & CDR_BURNFREE) {
		error("burnfree	Prepare writer to use BURN-Free technology\n");
		error("noburnfree	Disable using BURN-Free technology\n");
		haveopts = TRUE;
	}
	if (dp->cdr_flags & CDR_VARIREC) {
		error("varirec=val	Set VariRec Laserpower to -2, -1, 0, 1, 2\n");
		error("		Only works for audio and if speed is set to 4\n");
		haveopts = TRUE;
	}
	if (dp->cdr_flags & CDR_GIGAREC) {
		error("gigarec=val	Set GigaRec capacity ratio to 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.4\n");
		haveopts = TRUE;
	}
	if (dp->cdr_flags & CDR_AUDIOMASTER) {
		error("audiomaster	Turn Audio Master feature on (SAO CD-R Audio/Data only)\n");
		haveopts = TRUE;
	}
	if (dp->cdr_flags & CDR_FORCESPEED) {
		error("forcespeed	Tell the drive to force speed even for low quality media\n");
		haveopts = TRUE;
	}
	if (dp->cdr_flags & CDR_SPEEDREAD) {
		error("speedread	Tell the drive to read as fast as possible\n");
		error("nospeedread	Disable to read as fast as possible\n");
		haveopts = TRUE;
	}
	if (dp->cdr_flags & CDR_DISKTATTOO) {
		error("tattooinfo	Print image size info for DiskT@2 feature\n");
		error("tattoofile=name	Use 'name' as DiskT@2 image file\n");
		haveopts = TRUE;
	}
	if (dp->cdr_flags & CDR_SINGLESESS) {
		error("singlesession	Tell the drive to behave as single session only drive\n");
		error("nosinglesession	Disable single session only mode\n");
		haveopts = TRUE;
	}
	if (dp->cdr_flags & CDR_HIDE_CDR) {
		error("hidecdr		Tell the drive to hide CD-R media\n");
		error("nohidecdr	Disable hiding CD-R media\n");
		haveopts = TRUE;
	}
	if (dp->cdr_flags & CDR_LAYER_JUMP) {
		error("layerbreak	Write DVD-R/DL media in automatic layer jump mode\n");
		haveopts = TRUE;
	}
	if ((dp->cdr_flags & CDR_LAYER_JUMP) || get_curprofile(scgp) == 0x2B) {
		error("layerbreak=val	Set jayer jump address for DVD+-R/DL media\n");
		haveopts = TRUE;
	}
	if (!haveopts) {
		error("None supported for this drive.\n");
	}
	exit(excode);
}

EXPORT char *
hasdrvopt(optstr, optname)
	char	*optstr;
	char	*optname;
{
	return (hasdrvoptx(optstr, optname, 1));
}

EXPORT char *
hasdrvoptx(optstr, optname, flag)
	char	*optstr;
	char	*optname;
	int	flag;
{
	char	*ep;
	char	*np;
	char	*ret = NULL;
	int	optnamelen;
	int	optlen;
	BOOL	not = FALSE;

	if (optstr == NULL)
		return (ret);

	optnamelen = strlen(optname);

	while (*optstr) {
		not = FALSE;			/* Reset before every token */
		if ((ep = strchr(optstr, ',')) != NULL) {
			optlen = ep - optstr;
			np = &ep[1];
		} else {
			optlen = strlen(optstr);
			np = &optstr[optlen];
		}
		if ((ep = strchr(optstr, '=')) != NULL) {
			if (ep < np)
				optlen = ep - optstr;
		}
		if (optstr[0] == '!') {
			optstr++;
			optlen--;
			not = TRUE;
		}
		if (strncmp(optstr, "no", 2) == 0) {
			optstr += 2;
			optlen -= 2;
			not = TRUE;
		}
		if (optlen == optnamelen &&
		    strncmp(optstr, optname, optlen) == 0) {
			ret = &optstr[optlen];
			break;
		}
		optstr = np;
	}
	if (ret != NULL) {
		if (*ret == ',' || *ret == '\0') {
			if (flag) {
				if (not)
					return ("0");
				return ("1");
			}
			if (not)
				return (NULL);
			return ("");
		}
		if (*ret == '=') {
			if (not)
				return (NULL);
			return (++ret);
		}
	}
	return (ret);
}

LOCAL cdr_t *
identify_mmc(scgp, dp, ip)
	SCSI	*scgp;
	cdr_t			*dp;
	struct scsi_inquiry	*ip;
{
	BOOL	cdrr	 = FALSE;	/* Read CD-R	*/
	BOOL	cdwr	 = FALSE;	/* Write CD-R	*/
	BOOL	cdrrw	 = FALSE;	/* Read CD-RW	*/
	BOOL	cdwrw	 = FALSE;	/* Write CD-RW	*/
	BOOL	dvdwr	 = FALSE;	/* DVD writer	*/
	BOOL	is_dvd	 = FALSE;	/* use DVD driver*/
	Uchar	mode[0x100];
	struct	cd_mode_page_2A *mp;
	int	profile;

	if (ip->type != INQ_WORM && ip->type != INQ_ROMD)
		return ((cdr_t *)0);

	allow_atapi(scgp, TRUE); /* Try to switch to 10 byte mode cmds */

	scgp->silent++;
	mp = mmc_cap(scgp, mode);	/* Get MMC capabilities */
	scgp->silent--;
	if (mp == NULL)
		return (&cdr_oldcd);	/* Pre SCSI-3/mmc drive		*/

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

	/*
	 * First handle exceptions....
	 */
	if (strncmp(ip->inq_vendor_info, "SONY", 4) == 0 &&
	    strncmp(ip->inq_prod_ident, "CD-R   CDU928E", 14) == 0) {
		return (&cdr_mmc_sony);
	}

	/*
	 * Now try to do it the MMC-3 way....
	 */
	profile = get_curprofile(scgp);
	if (xdebug)
		printf("Current profile: 0x%04X\n", profile);
	if (profile == 0) {
		if (xdebug)
			print_profiles(scgp);
		/*
		 * If the current profile is 0x0000, then the
		 * drive does not know about the media. First
		 * close the tray and then try to issue the
		 * get_curprofile() command again.
		 */
		scgp->silent++;
		load_media(scgp, dp, FALSE);
		scgp->silent--;
		profile = get_curprofile(scgp);
		scsi_prevent_removal(scgp, 0);
		if (xdebug)
			printf("Current profile: 0x%04X\n", profile);
	}
	if (profile >= 0) {
		if (lverbose)
			print_profiles(scgp);
		if (lverbose > 1)
			print_features(scgp);
		/*	    NONE	    DVD-MINUS-END */
		if (profile == 0 || profile > 0x19) {
			is_dvd = FALSE;
			dp = &cdr_cd;

			if (profile == 0) {		/* No Medium */
				BOOL	is_cdr = FALSE;

				/*
				 * Check for CD-writer
				 */
				get_wproflist(scgp, &is_cdr, NULL,
							NULL, NULL);
				if (is_cdr)
					return (&cdr_mmc);
				/*
				 * Other MMC-3 drive without media
				 */
				return (dp);
			}
			if (profile <= 0x1F ||	/* DVD+RW DVD+R */
				    profile == 0x2B) {	/* DVD+R/DL */
				extern	cdr_t	cdr_dvdplus;

				dp = &cdr_dvdplus;
				dp = dp->cdr_identify(scgp, dp, scgp->inq);
				return (dp);
			} else if (profile >= 0x40 && profile <= 0x4F) {
				extern	cdr_t	cdr_bd;

				dp = &cdr_bd;
				dp = dp->cdr_identify(scgp, dp, scgp->inq);
				return (dp);
			} else if (profile >= 0x50 && profile <= 0x5F) {
				errmsgno(EX_BAD,
				"Found unsupported HD-DVD 0x%X profile.\n", profile);
				errmsgno(EX_BAD,
				"If you need HD-DVD support, sponsor a sample drive.\n");
				return ((cdr_t *)0);
			} else {
				errmsgno(EX_BAD,
				"Found unsupported 0x%X profile.\n", profile);
				return ((cdr_t *)0);
			}
		} else if (profile >= 0x10 && profile <= 0x19) {
			extern	cdr_t	cdr_dvd;

			dp = &cdr_dvd;
			dp = dp->cdr_identify(scgp, dp, scgp->inq);
			return (dp);
		}
	} else {
		/*
		 * profile < 0
		 */
		if (xdebug)
			printf("Drive is pre MMC-3\n");
	}

	mmc_getval(mp, &cdrr, &cdwr, &cdrrw, &cdwrw, NULL, &dvdwr);

	if (!cdwr && !cdwrw) {	/* SCSI-3/mmc CD drive		*/
		/*
		 * If the drive does not support to write CD's, we select the
		 * CD-ROM driver here. If we have DVD-R/DVD-RW support compiled
		 * in, we may later decide to switch to the DVD driver.
		 */
		dp = &cdr_cd;
	} else {
		/*
		 * We need to set the driver to cdr_mmc because we may come
		 * here with driver set to cdr_cd_dvd which is not a driver
		 * that may be used for actual CD/DVD writing.
		 */
		dp = &cdr_mmc;
	}

/*#define	DVD_DEBUG*/
#ifdef	DVD_DEBUG
	if (1) {	/* Always check for DVD media in debug mode */
#else
	if (profile < 0 && ((cdwr || cdwrw) && dvdwr)) {
#endif
		char	xb[32];

		/*
		 * Be careful here. It has been reported that the
		 * Pioneer DVR-110 will lock up in case that
		 * read_dvd_structure() is called if a non-DVD medium is
		 * loaded. For this reason, we only come here with
		 * Pre MMC-3 drives.
		 */
#ifndef	DVD_DEBUG
		scgp->silent++;
#else
		error("identify_dvd: checking for DVD media\n");
#endif
		if (read_dvd_structure(scgp, (caddr_t)xb, 32, 0, 0, 0, 0) >= 0) {
			/*
			 * If read DVD structure is supported and works, then
			 * we must have a DVD media in the drive. Signal to
			 * use the DVD driver.
			 */
			is_dvd = TRUE;
		} else {
			if (scg_sense_key(scgp) == SC_NOT_READY) {
				/*
				 * If the SCSI sense key is NOT READY, then the
				 * drive does not know about the media. First
				 * close the tray and then try to issue the
				 * read_dvd_structure() command again.
				 */
				load_media(scgp, dp, FALSE);
				if (read_dvd_structure(scgp, (caddr_t)xb, 32, 0, 0, 0, 0) >= 0) {
					is_dvd = TRUE;
				}
				scsi_prevent_removal(scgp, 0);
			}
		}
#ifndef	DVD_DEBUG
		scgp->silent--;
#else
		error("identify_dvd: is_dvd: %d\n", is_dvd);
#endif
	}
	if (is_dvd) {
		extern	cdr_t	cdr_dvd;

		dp = &cdr_dvd;
		dp = dp->cdr_identify(scgp, dp, scgp->inq);
		return (dp);
	}
	return (dp);
}

LOCAL int
attach_mmc(scgp, dp)
	SCSI	*scgp;
	cdr_t			*dp;
{
	int	ret;
	Uchar	mode[0x100];
	struct	cd_mode_page_2A *mp;
	struct	ricoh_mode_page_30 *rp = NULL;

	allow_atapi(scgp, TRUE); /* Try to switch to 10 byte mode cmds */

	scgp->silent++;
	mp = mmc_cap(scgp, NULL); /* Get MMC capabilities in allocated mp */
	scgp->silent--;
	if (mp == NULL)
		return (-1);	/* Pre SCSI-3/mmc drive		*/

	dp->cdr_cdcap = mp;	/* Store MMC cap pointer	*/

	dp->cdr_dstat->ds_dr_max_rspeed = a_to_u_2_byte(mp->max_read_speed)/176;
	if (dp->cdr_dstat->ds_dr_max_rspeed == 0)
		dp->cdr_dstat->ds_dr_max_rspeed = 372;
	dp->cdr_dstat->ds_dr_cur_rspeed = a_to_u_2_byte(mp->cur_read_speed)/176;
	if (dp->cdr_dstat->ds_dr_cur_rspeed == 0)
		dp->cdr_dstat->ds_dr_cur_rspeed = 372;

	dp->cdr_dstat->ds_dr_max_wspeed = a_to_u_2_byte(mp->max_write_speed)/176;
	if (mp->p_len >= 28)
		dp->cdr_dstat->ds_dr_cur_wspeed = a_to_u_2_byte(mp->v3_cur_write_speed)/176;
	else
		dp->cdr_dstat->ds_dr_cur_wspeed = a_to_u_2_byte(mp->cur_write_speed)/176;

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

	if (mp->BUF != 0) {
		dp->cdr_flags |= CDR_BURNFREE;
	} else if (rp) {
		if ((dp->cdr_cmdflags & F_DUMMY) && rp->TWBFS && rp->BUEFS)
			dp->cdr_flags |= CDR_BURNFREE;

		if (rp->BUEFS)
			dp->cdr_flags |= CDR_BURNFREE;
	}

	if (mmc_isplextor(scgp)) {
		char	*p;

		p = hasdrvopt(driveropts, "plexdisable");
		if (p != NULL && *p == '1') {
			plextor_disable(scgp);
		} else {
			p = NULL;
		}

		if (check_varirec_plextor(scgp) >= 0)
			dp->cdr_flags |= CDR_VARIREC;

		if (check_gigarec_plextor(scgp) < 0 && p == NULL)
			plextor_enable(scgp);

		if (check_gigarec_plextor(scgp) >= 0)
			dp->cdr_flags |= CDR_GIGAREC;

		if (check_ss_hide_plextor(scgp) >= 0)
			dp->cdr_flags |= CDR_SINGLESESS|CDR_HIDE_CDR;

		if (check_powerrec_plextor(scgp) >= 0)
			dp->cdr_flags |= CDR_FORCESPEED;

		if (check_speed_rd_plextor(scgp) >= 0)
			dp->cdr_flags |= CDR_SPEEDREAD;

		/*
		 * Newer Plextor drives
		 */
		if (set_audiomaster_yamaha(scgp, dp, FALSE) >= 0)
			dp->cdr_flags |= CDR_AUDIOMASTER;
	}
	if (mmc_isyamaha(scgp)) {
		if (set_audiomaster_yamaha(scgp, dp, FALSE) >= 0)
			dp->cdr_flags |= CDR_AUDIOMASTER;

		/*
		 * Starting with CRW 2200 / CRW 3200
		 */
		if ((mp->p_len+2) >= (unsigned)28)
			dp->cdr_flags |= CDR_FORCESPEED;

		if (get_tattoo_yamaha(scgp, FALSE, 0, 0))
			dp->cdr_flags |= CDR_DISKTATTOO;
	}

	if (rp && rp->AWSCS)
		dp->cdr_flags |= CDR_FORCESPEED;

#ifdef	FUTURE_ROTCTL
	if (mp->p_len >= 28) {
		int	val;

		val = dp->cdr_dstat->ds_dr_cur_wspeed;
		if (val == 0)
			val = 372;

		scgp->verbose++;
		if (scsi_set_speed(scgp, -1, val, ROTCTL_CAV) < 0) {
			error("XXX\n");
		}
		scgp->verbose--;
	}
#endif

	check_writemodes_mmc(scgp, dp);

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

		p = hasdrvopt(driveropts, "varirec");
		if (p != NULL && (dp->cdr_flags & CDR_VARIREC) != 0) {
			dp->cdr_dstat->ds_cdrflags |= RF_VARIREC;
		}

		p = hasdrvopt(driveropts, "gigarec");
		if (p != NULL && (dp->cdr_flags & CDR_GIGAREC) != 0) {
			dp->cdr_dstat->ds_cdrflags |= RF_GIGAREC;
		}

		p = hasdrvopt(driveropts, "audiomaster");
		if (p != NULL && *p == '1' && (dp->cdr_flags & CDR_AUDIOMASTER) != 0) {
			dp->cdr_dstat->ds_cdrflags |= RF_AUDIOMASTER;
			dp->cdr_dstat->ds_cdrflags &= ~RF_BURNFREE;
		}
		p = hasdrvopt(driveropts, "forcespeed");
		if (p != NULL && *p == '1' && (dp->cdr_flags & CDR_FORCESPEED) != 0) {
			dp->cdr_dstat->ds_cdrflags |= RF_FORCESPEED;
		}
		p = hasdrvopt(driveropts, "tattooinfo");
		if (p != NULL && *p == '1' && (dp->cdr_flags & CDR_DISKTATTOO) != 0) {
			get_tattoo_yamaha(scgp, TRUE, 0, 0);
		}
		p = hasdrvopt(driveropts, "tattoofile");
		if (p != NULL && (dp->cdr_flags & CDR_DISKTATTOO) != 0) {
			FILE	*f;

			if ((f = fileopen(p, "rb")) == NULL)
				comerr("Cannot open '%s'.\n", p);

			if (do_tattoo_yamaha(scgp, f) < 0)
				errmsgno(EX_BAD, "Cannot do DiskT@2.\n");
			fclose(f);
		}
		p = hasdrvopt(driveropts, "singlesession");
		if (p != NULL && (dp->cdr_flags & CDR_SINGLESESS) != 0) {
			if (*p == '1') {
				dp->cdr_dstat->ds_cdrflags |= RF_SINGLESESS;
			} else if (*p == '0') {
				dp->cdr_dstat->ds_cdrflags &= ~RF_SINGLESESS;
			}
		}
		p = hasdrvopt(driveropts, "hidecdr");
		if (p != NULL && (dp->cdr_flags & CDR_HIDE_CDR) != 0) {
			if (*p == '1') {
				dp->cdr_dstat->ds_cdrflags |= RF_HIDE_CDR;
			} else if (*p == '0') {
				dp->cdr_dstat->ds_cdrflags &= ~RF_HIDE_CDR;
			}
		}
		p = hasdrvopt(driveropts, "speedread");
		if (p != NULL && (dp->cdr_flags & CDR_SPEEDREAD) != 0) {
			if (*p == '1') {
				dp->cdr_dstat->ds_cdrflags |= RF_SPEEDREAD;
			} else if (*p == '0') {
				dp->cdr_dstat->ds_cdrflags &= ~RF_SPEEDREAD;
			}
		}
	}

	if ((ret = get_supported_cdrw_media_types(scgp)) < 0) {
		dp->cdr_cdrw_support = CDR_CDRW_ALL;
		return (0);
	}
	dp->cdr_cdrw_support = ret;
	if (lverbose > 1)
		printf("Supported CD-RW media types: %02X\n", dp->cdr_cdrw_support);

	return (0);
}


EXPORT int
check_writemodes_mmc(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	Uchar	mode[0x100];
	int	len;
	struct	cd_mode_page_05 *mp;

	if (xdebug)
		printf("Checking possible write modes: ");

	/*
	 * Reset mp->test_write (-dummy) here.
	 */
	deflt_writemodes_mmc(scgp, TRUE);

	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	scgp->silent++;
	if (!get_mode_params(scgp, 0x05, "CD write parameter",
			mode, (Uchar *)0, (Uchar *)0, (Uchar *)0, &len)) {
		scgp->silent--;
		return (-1);
	}
	if (len == 0) {
		scgp->silent--;
		return (-1);
	}

	mp = (struct cd_mode_page_05 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);
#ifdef	DEBUG
	scg_prbytes("CD write parameter:", (Uchar *)mode, len);
#endif

	/*
	 * mp->test_write has already been reset in deflt_writemodes_mmc()
	 * Do not reset mp->test_write (-dummy) here. It should be set
	 * only at one place and only one time.
	 */

	mp->write_type = WT_TAO;
	mp->track_mode = TM_DATA;
	mp->dbtype = DB_ROM_MODE1;

	if (set_mode_params(scgp, "CD write parameter", mode, len, 0, -1)) {
		dp->cdr_flags |= CDR_TAO;
		if (xdebug)
			printf("TAO ");
	} else
		dp->cdr_flags &= ~CDR_TAO;

	mp->write_type = WT_PACKET;
	mp->track_mode |= TM_INCREMENTAL;
/*	mp->fp = (trackp->pktsize > 0) ? 1 : 0;*/
/*	i_to_4_byte(mp->packet_size, trackp->pktsize);*/
	mp->fp = 0;
	i_to_4_byte(mp->packet_size, 0);

	if (set_mode_params(scgp, "CD write parameter", mode, len, 0, -1)) {
		dp->cdr_flags |= CDR_PACKET;
		if (xdebug)
			printf("PACKET ");
	} else
		dp->cdr_flags &= ~CDR_PACKET;
	mp->fp = 0;
	i_to_4_byte(mp->packet_size, 0);
	mp->track_mode = TM_DATA;


	mp->write_type = WT_SAO;

	if (set_mode_params(scgp, "CD write parameter", mode, len, 0, -1)) {
		dp->cdr_flags |= CDR_SAO;
		if (xdebug)
			printf("SAO ");
	} else
		dp->cdr_flags &= ~CDR_SAO;

	if (dp->cdr_flags & CDR_SAO) {
		mp->dbtype = DB_RAW_PQ;

#ifdef	__needed__
		if (set_mode_params(scgp, "CD write parameter", mode, len, 0, -1)) {
			dp->cdr_flags |= CDR_SRAW16;
			if (xdebug)
				printf("SAO/R16 ");
		}
#endif

		mp->dbtype = DB_RAW_PW;

		if (set_mode_params(scgp, "CD write parameter", mode, len, 0, -1)) {
			dp->cdr_flags |= CDR_SRAW96P;
			if (xdebug)
				printf("SAO/R96P ");
		}

		mp->dbtype = DB_RAW_PW_R;

		if (set_mode_params(scgp, "CD write parameter", mode, len, 0, -1)) {
			dp->cdr_flags |= CDR_SRAW96R;
			if (xdebug)
				printf("SAO/R96R ");
		}
	}

	mp->write_type = WT_RAW;
	mp->dbtype = DB_RAW_PQ;

	if (set_mode_params(scgp, "CD write parameter", mode, len, 0, -1)) {
		dp->cdr_flags |= CDR_RAW;
		dp->cdr_flags |= CDR_RAW16;
		if (xdebug)
			printf("RAW/R16 ");
	}

	mp->dbtype = DB_RAW_PW;

	if (set_mode_params(scgp, "CD write parameter", mode, len, 0, -1)) {
		dp->cdr_flags |= CDR_RAW;
		dp->cdr_flags |= CDR_RAW96P;
		if (xdebug)
			printf("RAW/R96P ");
	}

	mp->dbtype = DB_RAW_PW_R;

	if (set_mode_params(scgp, "CD write parameter", mode, len, 0, -1)) {
		dp->cdr_flags |= CDR_RAW;
		dp->cdr_flags |= CDR_RAW96R;
		if (xdebug)
			printf("RAW/R96R ");
	}

	mp->track_mode = TM_DATA;
	mp->write_type = WT_LAYER_JUMP;

	if (set_mode_params(scgp, "CD write parameter", mode, len, 0, -1) &&
	    has_profile(scgp, 0x16) == 1) {
		dp->cdr_flags |= CDR_LAYER_JUMP;
		if (xdebug)
			printf("LAYER JUMP ");
	} else
		dp->cdr_flags &= ~CDR_LAYER_JUMP;

	if (xdebug)
		printf("\n");

	/*
	 * Reset mp->test_write (-dummy) here.
	 */
	deflt_writemodes_mmc(scgp, TRUE);
	scgp->silent--;

	return (0);
}

LOCAL int
deflt_writemodes_mmc(scgp, reset_dummy)
	SCSI	*scgp;
	BOOL	reset_dummy;
{
	Uchar	mode[0x100];
	int	len;
	struct	cd_mode_page_05 *mp;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	scgp->silent++;
	if (!get_mode_params(scgp, 0x05, "CD write parameter",
			mode, (Uchar *)0, (Uchar *)0, (Uchar *)0, &len)) {
		scgp->silent--;
		return (-1);
	}
	if (len == 0) {
		scgp->silent--;
		return (-1);
	}

	mp = (struct cd_mode_page_05 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);
#ifdef	DEBUG
	scg_prbytes("CD write parameter:", (Uchar *)mode, len);
	error("Audio pause len: %d\n", a_to_2_byte(mp->audio_pause_len));
#endif

	/*
	 * This is the only place where we reset mp->test_write (-dummy)
	 */
	if (reset_dummy)
		mp->test_write = 0;

	/*
	 * Set default values:
	 * Write type = 01 (track at once)
	 * Track mode = 04 (CD-ROM)
	 * Data block type = 08 (CD-ROM)
	 * Session format = 00 (CD-ROM)
	 *
	 * XXX Note:	the same code appears in check_writemodes_mmc() and
	 * XXX		in speed_select_mmc().
	 */
	mp->write_type = WT_TAO;
	mp->track_mode = TM_DATA;
	mp->dbtype = DB_ROM_MODE1;
	mp->session_format = SES_DA_ROM; /* Matsushita has illegal def. value */

	i_to_2_byte(mp->audio_pause_len, 150);	/* LG has illegal def. value */

#ifdef	DEBUG
	scg_prbytes("CD write parameter:", (Uchar *)mode, len);
#endif
	if (!set_mode_params(scgp, "CD write parameter", mode, len, 0, -1)) {

		mp->write_type	= WT_SAO;
		mp->LS_V	= 0;
		mp->copy	= 0;
		mp->fp		= 0;
		mp->multi_session  = MS_NONE;
		mp->host_appl_code = 0;

		if (!set_mode_params(scgp, "CD write parameter", mode, len, 0, -1)) {
			scgp->silent--;
			return (-1);
		}
	}
	scgp->silent--;
	return (0);
}

#ifdef	PRINT_ATIP
LOCAL	void	atip_printspeed		__PR((char *fmt, int speedindex, char speedtab[]));
LOCAL	void	print_atip		__PR((SCSI *scgp, struct atipinfo *atp));
#endif	/* PRINT_ATIP */

LOCAL void
di_to_dstat(dip, dsp)
	struct disk_info	*dip;
	dstat_t	*dsp;
{
	dsp->ds_diskid = a_to_u_4_byte(dip->disk_id);
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

	dsp->ds_maxblocks = msf_to_lba(dip->last_lead_out[1],
					dip->last_lead_out[2],
					dip->last_lead_out[3], TRUE);
	/*
	 * Check for 0xFF:0xFF/0xFF which is an indicator for a complete disk
	 */
	if (dsp->ds_maxblocks == 1166730)
		dsp->ds_maxblocks = -1L;

	dsp->ds_first_leadin = msf_to_lba(dip->last_lead_in[1],
					dip->last_lead_in[2],
					dip->last_lead_in[3], FALSE);
	if (dsp->ds_first_leadin > 0)
		dsp->ds_first_leadin = 0;

	if (dsp->ds_last_leadout == 0 && dsp->ds_maxblocks >= 0)
		dsp->ds_last_leadout = dsp->ds_maxblocks;
}

LOCAL int
get_atip(scgp, atp)
	SCSI		*scgp;
	struct atipinfo *atp;
{
	int	len;
	int	ret;

	fillbytes((caddr_t)atp, sizeof (*atp), '\0');

	/*
	 * Used to be 2 instead of sizeof (struct tocheader), but all
	 * other places in the code use sizeof (struct tocheader) too and
	 * some Y2k ATAPI drives as used by IOMEGA create a DMA overrun if we
	 * try to transfer only 2 bytes.
	 */
	if (read_toc(scgp, (caddr_t)atp, 0, sizeof (struct tocheader), 0, FMT_ATIP) < 0)
		return (-1);
	len = a_to_u_2_byte(atp->hd.len);
	len += 2;
	ret = read_toc(scgp, (caddr_t)atp, 0, len, 0, FMT_ATIP);

#ifdef	DEBUG
	scg_prbytes("ATIP info:", (Uchar *)atp,
				len-scg_getresid(scgp));
#endif
	/*
	 * Yamaha sometimes returns zeroed ATIP info for disks without ATIP
	 */
	if (atp->desc.lead_in[1] == 0 &&
			atp->desc.lead_in[2] == 0 &&
			atp->desc.lead_in[3] == 0 &&
			atp->desc.lead_out[1] == 0 &&
			atp->desc.lead_out[2] == 0 &&
			atp->desc.lead_out[3] == 0)
		return (-1);

	if (atp->desc.lead_in[1] >= 0x90 && debug) {
		/*
		 * Only makes sense with buggy Ricoh firmware.
		 */
		errmsgno(EX_BAD, "Converting ATIP from BCD\n");
		atp->desc.lead_in[1] = from_bcd(atp->desc.lead_in[1]);
		atp->desc.lead_in[2] = from_bcd(atp->desc.lead_in[2]);
		atp->desc.lead_in[3] = from_bcd(atp->desc.lead_in[3]);

		atp->desc.lead_out[1] = from_bcd(atp->desc.lead_out[1]);
		atp->desc.lead_out[2] = from_bcd(atp->desc.lead_out[2]);
		atp->desc.lead_out[3] = from_bcd(atp->desc.lead_out[3]);
	}

	return (ret);
}

#ifdef	PRINT_PMA

LOCAL int
/*get_pma(atp)*/
get_pma(scgp)
	SCSI	*scgp;
/*	struct atipinfo *atp;*/
{
	int	len;
	int	ret;
char	atp[1024];

	fillbytes((caddr_t)atp, sizeof (*atp), '\0');

	/*
	 * Used to be 2 instead of sizeof (struct tocheader), but all
	 * other places in the code use sizeof (struct tocheader) too and
	 * some Y2k ATAPI drives as used by IOMEGA create a DMA overrun if we
	 * try to transfer only 2 bytes.
	 */
/*	if (read_toc(scgp, (caddr_t)atp, 0, 2, 1, FMT_PMA) < 0)*/
/*	if (read_toc(scgp, (caddr_t)atp, 0, 2, 0, FMT_PMA) < 0)*/
	if (read_toc(scgp, (caddr_t)atp, 0, sizeof (struct tocheader), 0, FMT_PMA) < 0)
		return (-1);
/*	len = a_to_u_2_byte(atp->hd.len);*/
	len = a_to_u_2_byte(atp);
	len += 2;
/*	ret = read_toc(scgp, (caddr_t)atp, 0, len, 1, FMT_PMA);*/
	ret = read_toc(scgp, (caddr_t)atp, 0, len, 0, FMT_PMA);

#ifdef	DEBUG
	scg_prbytes("PMA:", (Uchar *)atp,
				len-scg_getresid(scgp));
#endif
	ret = read_toc(scgp, (caddr_t)atp, 0, len, 1, FMT_PMA);

#ifdef	DEBUG
	scg_prbytes("PMA:", (Uchar *)atp,
				len-scg_getresid(scgp));
#endif
	return (ret);
}

#endif	/* PRINT_PMA */

LOCAL int
init_mmc(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	return (speed_select_mmc(scgp, dp, NULL));
}

LOCAL int
getdisktype_mmc(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
extern	char	*buf;
	dstat_t	*dsp = dp->cdr_dstat;
	struct disk_info *dip;
	Uchar	mode[0x100];
	msf_t	msf;
	BOOL	did_atip = FALSE;
	BOOL	did_dummy = FALSE;
	int	profile;

	msf.msf_min = msf.msf_sec = msf.msf_frame = 0;

	if (dsp->ds_type == DST_UNKNOWN) {
		profile = get_curprofile(scgp);
		if (profile >= 0)
			dsp->ds_type = profile;
	}

	/*
	 * It seems that there are drives that do not support to
	 * read ATIP (e.g. HP 7100)
	 * Also if a NON CD-R media is inserted, this will never work.
	 * For this reason, make a failure non-fatal.
	 */
	scgp->silent++;
	if (get_atip(scgp, (struct atipinfo *)mode) >= 0) {
		struct atipinfo *atp = (struct atipinfo *)mode;

		msf.msf_min =		mode[8];
		msf.msf_sec =		mode[9];
		msf.msf_frame =		mode[10];
		if (atp->desc.erasable) {
			dsp->ds_flags |= DSF_ERA;
			if (atp->desc.sub_type == 1)
				dsp->ds_flags |= DSF_HIGHSP_ERA;
			else if (atp->desc.sub_type == 2)
				dsp->ds_flags |= DSF_ULTRASP_ERA;
			else if (atp->desc.sub_type == 3)
				dsp->ds_flags |= DSF_ULTRASP_ERA | DSF_ULTRASPP_ERA;
		}
		if (atp->desc.a1_v) {
			if (atp->desc.clv_low != 0)
				dsp->ds_at_min_speed = clv_to_speed[atp->desc.clv_low];
			if (atp->desc.clv_high != 0)
				dsp->ds_at_max_speed = clv_to_speed[atp->desc.clv_high];

			if (atp->desc.erasable && atp->desc.sub_type == 1) {
				if (atp->desc.clv_high != 0)
					dsp->ds_at_max_speed = hs_clv_to_speed[atp->desc.clv_high];
			}
		}
		if (atp->desc.a2_v && atp->desc.erasable && (atp->desc.sub_type == 2 || atp->desc.sub_type == 3)) {
			Uint	vlow;
			Uint	vhigh;

			vlow = (atp->desc.a2[0] >> 4) & 0x07;
			vhigh = atp->desc.a2[0] & 0x0F;
			if (vlow != 0)
				dsp->ds_at_min_speed = us_clv_to_speed[vlow];
			if (vhigh != 0)
				dsp->ds_at_max_speed = us_clv_to_speed[vhigh];
		}
		did_atip = TRUE;
	}
	scgp->silent--;

#ifdef	PRINT_ATIP
	if ((dp->cdr_dstat->ds_cdrflags & RF_PRATIP) != 0 && did_atip) {
		print_atip(scgp, (struct atipinfo *)mode);
		pr_manufacturer(&msf,
			((struct atipinfo *)mode)->desc.erasable,
			((struct atipinfo *)mode)->desc.uru);
	}
#endif
	if ((dp->cdr_dstat->ds_cdrflags & RF_PRATIP) != 0) {
		print_format_capacities(scgp);
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
				printf("Trying to clear drive status.\n");

			dp->cdr_cmdflags &= ~F_DUMMY;
			speed_select_mmc(scgp, dp, &xspeed);
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
	if (!did_atip && dsp->ds_first_leadin < 0)
		lba_to_msf(dsp->ds_first_leadin, &msf);

	if ((dp->cdr_dstat->ds_cdrflags & RF_PRATIP) != 0 && !did_atip) {
		print_min_atip(dsp->ds_first_leadin, dsp->ds_last_leadout);
		if (dsp->ds_first_leadin < 0)
				pr_manufacturer(&msf,
				dip->erasable,
				dip->uru);
	}
	dsp->ds_maxrblocks = disk_rcap(&msf, dsp->ds_maxblocks,
				dip->erasable,
				dip->uru);


#ifdef	PRINT_ATIP
#ifdef	DEBUG
	if (get_atip(scgp, (struct atipinfo *)mode) < 0)
		return (-1);
	/*
	 * Get pma gibt Ärger mit CW-7502
	 * Wenn Die Disk leer ist, dann stuerzt alles ab.
	 * Firmware 4.02 kann nicht get_pma
	 */
	if (dip->disk_status != DS_EMPTY) {
#ifdef	PRINT_PMA
/*		get_pma();*/
#endif
	}
	printf("ATIP lead in:  %ld (%02d:%02d/%02d)\n",
		msf_to_lba(mode[8], mode[9], mode[10], FALSE),
		mode[8], mode[9], mode[10]);
	printf("ATIP lead out: %ld (%02d:%02d/%02d)\n",
		msf_to_lba(mode[12], mode[13], mode[14], TRUE),
		mode[12], mode[13], mode[14]);
	print_diskinfo(dip, TRUE);
	print_atip(scgp, (struct atipinfo *)mode);
#endif
#endif	/* PRINT_ATIP */
	return (drive_getdisktype(scgp, dp));
}

LOCAL int
prdiskstatus_mmc(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	return (prdiskstatus(scgp, dp, TRUE));
}

#ifdef	PRINT_ATIP

#define	IS(what, flag)		printf("Disk Is %s%s\n", flag?"":"not ", what);

char	*cdr_subtypes[] = {
	"Normal Rewritable (CLV) media",
	"High speed Rewritable (CAV) media",
	"Medium Type A, low Beta category (A-)",
	"Medium Type A, high Beta category (A+)",
	"Medium Type B, low Beta category (B-)",
	"Medium Type B, high Beta category (B+)",
	"Medium Type C, low Beta category (C-)",
	"Medium Type C, high Beta category (C+)",
};

char	*cdrw_subtypes[] = {
	"Normal Rewritable (CLV) media",
	"High speed Rewritable (CAV) media",
	"Ultra High speed Rewritable media",
	"Ultra High speed+ Rewritable media",
	"Medium Type B, low Beta category (B-)",
	"Medium Type B, high Beta category (B+)",
	"Medium Type C, low Beta category (C-)",
	"Medium Type C, high Beta category (C+)",
};

LOCAL void
atip_printspeed(fmt, speedindex, speedtab)
	char	*fmt;
	int	speedindex;
	char	speedtab[];
{
	printf("%s:", fmt);
	if (speedtab[speedindex] == 0) {
		printf(" %2d (reserved val %2d)",
			speedtab[speedindex], speedindex);
	} else {
		printf(" %2d", speedtab[speedindex]);
	}
}

LOCAL void
print_atip(scgp, atp)
	SCSI		*scgp;
	struct atipinfo *atp;
{
	char	*sub_type;
	char	*speedvtab = clv_to_speed;

	if (scgp->verbose)
		scg_prbytes("ATIP info: ", (Uchar *)atp, sizeof (*atp));

	printf("ATIP info from disk:\n");
	printf("  Indicated writing power: %d\n", atp->desc.ind_wr_power);
	if (atp->desc.erasable || atp->desc.ref_speed)
		printf("  Reference speed: %d\n", clv_to_speed[atp->desc.ref_speed]);
	IS("unrestricted", atp->desc.uru);
/*	printf("  Disk application code: %d\n", atp->desc.res5_05);*/
	IS("erasable", atp->desc.erasable);
	if (atp->desc.erasable)
		sub_type = cdrw_subtypes[atp->desc.sub_type];
	else
		sub_type = cdr_subtypes[atp->desc.sub_type];
	if (atp->desc.sub_type)
		printf("  Disk sub type: %s (%d)\n", sub_type, atp->desc.sub_type);
	printf("  ATIP start of lead in:  %ld (%02d:%02d/%02d)\n",
		msf_to_lba(atp->desc.lead_in[1],
		atp->desc.lead_in[2],
		atp->desc.lead_in[3], FALSE),
		atp->desc.lead_in[1],
		atp->desc.lead_in[2],
		atp->desc.lead_in[3]);
	printf("  ATIP start of lead out: %ld (%02d:%02d/%02d)\n",
		msf_to_lba(atp->desc.lead_out[1],
		atp->desc.lead_out[2],
		atp->desc.lead_out[3], TRUE),
		atp->desc.lead_out[1],
		atp->desc.lead_out[2],
		atp->desc.lead_out[3]);
	if (atp->desc.a1_v) {
		if (atp->desc.erasable && atp->desc.sub_type == 1) {
			speedvtab = hs_clv_to_speed;
		}
		if (atp->desc.a2_v && (atp->desc.sub_type == 2 || atp->desc.sub_type == 3)) {
			speedvtab = us_clv_to_speed;
		}
		if (atp->desc.clv_low != 0 || atp->desc.clv_high != 0) {
			atip_printspeed("  1T speed low",
				atp->desc.clv_low, speedvtab);
			atip_printspeed(" 1T speed high",
				atp->desc.clv_high, speedvtab);
			printf("\n");
		}
	}
	if (atp->desc.a2_v) {
		Uint	vlow;
		Uint	vhigh;

		vlow = (atp->desc.a2[0] >> 4) & 0x07;
		vhigh = atp->desc.a2[0] & 0x0F;

		if (vlow != 0 || vhigh != 0) {
			atip_printspeed("  2T speed low",
					vlow, speedvtab);
			atip_printspeed(" 2T speed high",
					vhigh, speedvtab);
			printf("\n");
		}
	}
	if (atp->desc.a1_v) {
		printf("  power mult factor: %d %d\n", atp->desc.power_mult, atp->desc.tgt_y_pow);
		if (atp->desc.erasable)
			printf("  recommended erase/write power: %d\n", atp->desc.rerase_pwr_ratio);
	}
	if (atp->desc.a1_v) {
		printf("  A1 values: %02X %02X %02X\n",
				(&atp->desc.res15)[1],
				(&atp->desc.res15)[2],
				(&atp->desc.res15)[3]);
	}
	if (atp->desc.a2_v) {
		printf("  A2 values: %02X %02X %02X\n",
				atp->desc.a2[0],
				atp->desc.a2[1],
				atp->desc.a2[2]);
	}
	if (atp->desc.a3_v) {
		printf("  A3 values: %02X %02X %02X\n",
				atp->desc.a3[0],
				atp->desc.a3[1],
				atp->desc.a3[2]);
	}
}
#endif	/* PRINT_ATIP */

LOCAL int
speed_select_mmc(scgp, dp, speedp)
	SCSI	*scgp;
	cdr_t 	*dp;
	int	*speedp;
{
	Uchar	mode[0x100];
	Uchar	moder[0x100];
	int	len;
	struct	cd_mode_page_05 *mp;
	struct	ricoh_mode_page_30 *rp = NULL;
	int	val;
	BOOL	forcespeed = FALSE;
	BOOL	dummy = (dp->cdr_cmdflags & F_DUMMY) != 0;

	if (speedp)
		curspeed = *speedp;

	/*
	 * Do not reset mp->test_write (-dummy) here.
	 */
	deflt_writemodes_mmc(scgp, FALSE);

	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	if (!get_mode_params(scgp, 0x05, "CD write parameter",
			mode, (Uchar *)0, (Uchar *)0, (Uchar *)0, &len))
		return (-1);
	if (len == 0)
		return (-1);

	mp = (struct cd_mode_page_05 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);
#ifdef	DEBUG
	scg_prbytes("CD write parameter:", (Uchar *)mode, len);
#endif


	mp->test_write = dummy != 0;

#ifdef	DEBUG
	scg_prbytes("CD write parameter:", (Uchar *)mode, len);
#endif
	if (!set_mode_params(scgp, "CD write parameter", mode, len, 0, -1))
		return (-1);

	/*
	 * Neither set nor get speed.
	 */
	if (speedp == 0)
		return (0);


	rp = get_justlink_ricoh(scgp, moder);
	if (mmc_isyamaha(scgp)) {
		forcespeed = FALSE;
	} else if (mmc_isplextor(scgp) && (dp->cdr_flags & CDR_FORCESPEED) != 0) {
		int	pwr;

		pwr = check_powerrec_plextor(scgp);
		if (pwr >= 0)
			forcespeed = (pwr == 0);
	} else if ((dp->cdr_flags & CDR_FORCESPEED) != 0) {
		forcespeed = rp && rp->AWSCD != 0;
	}

	if (lverbose && (dp->cdr_flags & CDR_FORCESPEED) != 0)
		printf("Forcespeed is %s.\n", forcespeed?"ON":"OFF");

	if (!forcespeed && (dp->cdr_dstat->ds_cdrflags & RF_FORCESPEED) != 0) {
		printf("Turning forcespeed on\n");
		forcespeed = TRUE;
	}
	if (forcespeed && (dp->cdr_dstat->ds_cdrflags & RF_FORCESPEED) == 0) {
		printf("Turning forcespeed off\n");
		forcespeed = FALSE;
	}
	if (mmc_isplextor(scgp) && (dp->cdr_flags & CDR_FORCESPEED) != 0) {
		powerrec_plextor(scgp, !forcespeed);
	}
	if (!mmc_isyamaha(scgp) && (dp->cdr_flags & CDR_FORCESPEED) != 0) {

		if (rp) {
			rp->AWSCD = forcespeed?1:0;
			set_mode_params(scgp, "Ricoh Vendor Page", moder, moder[0]+1, 0, -1);
			rp = get_justlink_ricoh(scgp, moder);
		}
	}

	/*
	 * 44100 * 2 * 2 =  176400 bytes/s
	 *
	 * The right formula would be:
	 * tmp = (((long)curspeed) * 1764) / 10;
	 *
	 * But the standard is rounding the wrong way.
	 * Furtunately rounding down is guaranteed.
	 */
	val = curspeed*177;
	if (val > 0xFFFF)
		val = 0xFFFF;
	if (mmc_isyamaha(scgp) && forcespeed) {
		if (force_speed_yamaha(scgp, -1, val) < 0)
			return (-1);
	} else if (mmc_set_speed(scgp, -1, val, ROTCTL_CLV) < 0) {
		return (-1);
	}

	if (scsi_get_speed(scgp, 0, &val) >= 0) {
		if (val > 0) {
			curspeed = val / 176;
			*speedp = curspeed;
		}
	}
	return (0);
}

/*
 * Some drives do not round up when writespeed is e.g. 1 and
 * the minimum write speed of the drive is higher. Try to increment
 * the write speed unti it gets accepted by the drive.
 */
LOCAL int
mmc_set_speed(scgp, readspeed, writespeed, rotctl)
	SCSI	*scgp;
	int	readspeed;
	int	writespeed;
	int	rotctl;
{
	int	rs;
	int	ws;
	int	ret = -1;
	int	c;
	int	k;

	if (scsi_get_speed(scgp, &rs, &ws) >= 0) {
		if (readspeed < 0)
			readspeed = rs;
		if (writespeed < 0)
			writespeed = ws;
	}
	if (writespeed < 0 || writespeed > 0xFFFF)
		return (ret);

	scgp->silent++;
	while (writespeed <= 0xFFFF) {
		ret = scsi_set_speed(scgp, readspeed, writespeed, rotctl);
		if (ret >= 0)
			break;
		c = scg_sense_code(scgp);
		k = scg_sense_key(scgp);
		/*
		 * Abort quickly if it does not make sense to repeat.
		 * 0x24 == Invalid field in cdb
		 * 0x24 means illegal speed.
		 */
		if ((k != SC_ILLEGAL_REQUEST) || (c != 0x24)) {
			if (scgp->silent <= 1)
				scg_printerr(scgp);
			scgp->silent--;
			return (-1);
		}
		writespeed += 177;
	}
	if (ret < 0 && scgp->silent <= 1)
		scg_printerr(scgp);
	scgp->silent--;

	return (ret);
}

LOCAL int
next_wr_addr_mmc(scgp, trackp, ap)
	SCSI	*scgp;
	track_t	*trackp;
	long	*ap;
{
	struct	track_info	track_info;
	long	next_addr;
	int	result = -1;


	/*
	 * Reading info for current track may require doing the get_trackinfo
	 * with either the track number (if the track is currently being written)
	 * or with 0xFF (if the track hasn't been started yet and is invisible
	 */

	if (trackp != 0 && trackp->track > 0 && is_packet(trackp)) {
		scgp->silent++;
		result = get_trackinfo(scgp, (caddr_t)&track_info, TI_TYPE_TRACK,
							trackp->trackno,
							sizeof (track_info));
		scgp->silent--;
	}

	if (result < 0) {
		if (get_trackinfo(scgp, (caddr_t)&track_info, TI_TYPE_TRACK, 0xFF,
						sizeof (track_info)) < 0) {
			errmsgno(EX_BAD, "Cannot get next writable address for 'invisible' track.\n");
			errmsgno(EX_BAD, "This means that we are checking recorded media.\n");
			errmsgno(EX_BAD, "This media cannot be written in streaming mode anymore.\n");
			errmsgno(EX_BAD, "If you like to write to 'preformatted' RW media, try to blank the media first.\n");
			return (-1);
		}
	}
	if (scgp->verbose)
		scg_prbytes("track info:", (Uchar *)&track_info,
				sizeof (track_info)-scg_getresid(scgp));
	next_addr = a_to_4_byte(track_info.next_writable_addr);
	if (ap)
		*ap = next_addr;
	return (0);
}

LOCAL int
write_leadin_mmc(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t *trackp;
{
	Uint	i;
	long	startsec = 0L;

/*	if (flags & F_SAO) {*/
	if (wm_base(dp->cdr_dstat->ds_wrmode) == WM_SAO) {
		if (debug || lverbose) {
			printf("Sending CUE sheet...\n");
			flush();
		}
		if ((*dp->cdr_send_cue)(scgp, dp, trackp) < 0) {
			errmsgno(EX_BAD, "Cannot send CUE sheet.\n");
			return (-1);
		}

		(*dp->cdr_next_wr_address)(scgp, &trackp[0], &startsec);
		if (trackp[0].flags & TI_TEXT) {
			startsec = dp->cdr_dstat->ds_first_leadin;
			printf("SAO startsec: %ld\n", startsec);
		} else if (startsec <= 0 && startsec != -150) {
			errmsgno(EX_BAD, "WARNING: Drive returns wrong startsec (%ld) using -150\n",
					startsec);
			startsec = -150;
		}
		if (debug)
			printf("SAO startsec: %ld\n", startsec);

		if (trackp[0].flags & TI_TEXT) {
			if (startsec > 0) {
				errmsgno(EX_BAD, "CD-Text must be in first session.\n");
				return (-1);
			}
			if (debug || lverbose)
				printf("Writing lead-in...\n");
			if (write_cdtext(scgp, dp, startsec) < 0)
				return (-1);

			dp->cdr_dstat->ds_cdrflags |= RF_LEADIN;
		} else for (i = 1; i <= trackp->tracks; i++) {
			trackp[i].trackstart += startsec +150;
		}
#ifdef	XXX
		if (debug || lverbose)
			printf("Writing lead-in...\n");

		pad_track(scgp, dp, &trackp[1], -150, (Llong)0,
					FALSE, 0);
#endif
	}
/*	if (flags & F_RAW) {*/
	if (wm_base(dp->cdr_dstat->ds_wrmode) == WM_RAW) {
		/*
		 * In RAW write mode, we now write the lead in (TOC).
		 */
		(*dp->cdr_next_wr_address)(scgp, &trackp[0], &startsec);
		if (startsec > -4500) {
			/*
			 * There must be at least 1 minute lead-in.
			 */
			errmsgno(EX_BAD, "WARNING: Drive returns wrong startsec (%ld) using %ld from ATIP\n",
					startsec, (long)dp->cdr_dstat->ds_first_leadin);
			startsec = dp->cdr_dstat->ds_first_leadin;
		}
		if (startsec > -4500) {
			errmsgno(EX_BAD, "Illegal startsec (%ld)\n", startsec);
			return (-1);
		}
		if (debug || lverbose)
			printf("Writing lead-in at sector %ld\n", startsec);
		if (write_leadin(scgp, dp, trackp, startsec) < 0)
			return (-1);
		dp->cdr_dstat->ds_cdrflags |= RF_LEADIN;
	}
	return (0);
}

int	st2mode[] = {
	0,		/* 0			*/
	TM_DATA,	/* 1 ST_ROM_MODE1	*/
	TM_DATA,	/* 2 ST_ROM_MODE2	*/
	0,		/* 3			*/
	0,		/* 4 ST_AUDIO_NOPRE	*/
	TM_PREEM,	/* 5 ST_AUDIO_PRE	*/
	0,		/* 6			*/
	0,		/* 7			*/
};

LOCAL int
open_track_mmc(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t *trackp;
{
	Uchar	mode[0x100];
	int	len;
	struct	cd_mode_page_05 *mp;

	if (!is_tao(trackp) && !is_packet(trackp)) {
		if (trackp->pregapsize > 0 && (trackp->flags & TI_PREGAP) == 0) {
			if (lverbose) {
				printf("Writing pregap for track %d at %ld\n",
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

	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	if (!get_mode_params(scgp, 0x05, "CD write parameter",
			mode, (Uchar *)0, (Uchar *)0, (Uchar *)0, &len))
		return (-1);
	if (len == 0)
		return (-1);

	mp = (struct cd_mode_page_05 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);


/*	mp->track_mode = ???;*/
	mp->track_mode = st2mode[trackp->sectype & ST_MASK];
/*	mp->copy = ???;*/
	mp->dbtype = trackp->dbtype;

/*i_to_short(mp->audio_pause_len, 300);*/
/*i_to_short(mp->audio_pause_len, 150);*/
/*i_to_short(mp->audio_pause_len, 0);*/

	if (is_packet(trackp)) {
		mp->write_type = WT_PACKET;
		mp->track_mode |= TM_INCREMENTAL;
		mp->fp = (trackp->pktsize > 0) ? 1 : 0;
		i_to_4_byte(mp->packet_size, trackp->pktsize);
	} else if (is_tao(trackp)) {
		mp->write_type = WT_TAO;
		mp->fp = 0;
		i_to_4_byte(mp->packet_size, 0);
	} else {
		errmsgno(EX_BAD, "Unknown write mode.\n");
		return (-1);
	}
	if (trackp->isrc) {
		mp->ISRC[0] = 0x80;	/* Set ISRC valid */
		strncpy((char *)&mp->ISRC[1], trackp->isrc, 12);

	} else {
		fillbytes(&mp->ISRC[0], sizeof (mp->ISRC), '\0');
	}

#ifdef	DEBUG
	scg_prbytes("CD write parameter:", (Uchar *)mode, len);
#endif
	if (!set_mode_params(scgp, "CD write parameter", mode, len, 0, trackp->secsize))
		return (-1);

	return (0);
}

LOCAL int
close_track_mmc(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	int	ret;

	if (!is_tao(trackp) && !is_packet(trackp))
		return (0);

	if (scsi_flush_cache(scgp, (dp->cdr_cmdflags&F_IMMED) != 0) < 0) {
		printf("Trouble flushing the cache\n");
		return (-1);
	}
	wait_unit_ready(scgp, 300);		/* XXX Wait for ATAPI */
	if (is_packet(trackp) && !is_noclose(trackp)) {
			/* close the incomplete track */
		ret = scsi_close_tr_session(scgp, CL_TYPE_TRACK, 0xFF,
				(dp->cdr_cmdflags&F_IMMED) != 0);
		wait_unit_ready(scgp, 300);	/* XXX Wait for ATAPI */
		return (ret);
	}
	return (0);
}

int	toc2sess[] = {
	SES_DA_ROM,	/* CD-DA		 */
	SES_DA_ROM,	/* CD-ROM		 */
	SES_XA,		/* CD-ROM XA mode 1	 */
	SES_XA,		/* CD-ROM XA MODE 2	 */
	SES_CDI,	/* CDI			 */
	SES_DA_ROM,	/* Invalid - use default */
	SES_DA_ROM,	/* Invalid - use default */
};

LOCAL int
open_session_mmc(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	Uchar	mode[0x100];
	int	len;
	struct	cd_mode_page_05 *mp;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	if (!get_mode_params(scgp, 0x05, "CD write parameter",
			mode, (Uchar *)0, (Uchar *)0, (Uchar *)0, &len))
		return (-1);
	if (len == 0)
		return (-1);

	mp = (struct cd_mode_page_05 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	mp->write_type = WT_TAO; /* fix to allow DAO later */
	/*
	 * We need to set the right dbtype here because Sony drives
	 * don't like multi session in to be set with DB_ROM_MODE1
	 * which is set by us at the beginning as default as some drives
	 * have illegal default values.
	 */
	mp->track_mode = st2mode[trackp[0].sectype & ST_MASK];
	mp->dbtype = trackp[0].dbtype;

	if (!is_tao(trackp) && !is_packet(trackp)) {
		mp->write_type = WT_SAO;
		if (dp->cdr_dstat->ds_cdrflags & RF_AUDIOMASTER)
			mp->write_type = 8;
		mp->track_mode = 0;
		mp->dbtype = DB_RAW;
	}
	if (is_raw(trackp)) {
		mp->write_type = WT_RAW;
		mp->track_mode = 0;

		if (is_raw16(trackp)) {
			mp->dbtype = DB_RAW_PQ;
		} else if (is_raw96r(trackp)) {
			mp->dbtype = DB_RAW_PW_R;
		} else {
			mp->dbtype = DB_RAW_PW;
		}
	}

	mp->multi_session = (track_base(trackp)->tracktype & TOCF_MULTI) ?
				MS_MULTI : MS_NONE;
	mp->session_format = toc2sess[track_base(trackp)->tracktype & TOC_MASK];

	if (trackp->isrc) {
		mp->media_cat_number[0] = 0x80;	/* Set MCN valid */
		strncpy((char *)&mp->media_cat_number[1], trackp->isrc, 13);

	} else {
		fillbytes(&mp->media_cat_number[0], sizeof (mp->media_cat_number), '\0');
	}
#ifdef	DEBUG
	scg_prbytes("CD write parameter:", (Uchar *)mode, len);
#endif
	if (!set_mode_params(scgp, "CD write parameter", mode, len, 0, -1))
		return (-1);

	return (0);
}

LOCAL int
waitfix_mmc(scgp, secs)
	SCSI	*scgp;
	int	secs;
{
	char	dibuf[16];
	int	i;
	int	key;
#define	W_SLEEP	2

	scgp->silent++;
	for (i = 0; i < secs/W_SLEEP; i++) {
		if (read_disk_info(scgp, dibuf, sizeof (dibuf)) >= 0) {
			scgp->silent--;
			return (0);
		}
		key = scg_sense_key(scgp);
		if (key != SC_UNIT_ATTENTION && key != SC_NOT_READY)
			break;
		sleep(W_SLEEP);
	}
	scgp->silent--;
	return (-1);
#undef	W_SLEEP
}

LOCAL int
fixate_mmc(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	int	ret = 0;
	int	key = 0;
	int	code = 0;
	struct timeval starttime;
	struct timeval stoptime;
	int	dummy = (track_base(trackp)->tracktype & TOCF_DUMMY) != 0;

	starttime.tv_sec = 0;
	starttime.tv_usec = 0;
	stoptime = starttime;
	gettimeofday(&starttime, (struct timezone *)0);

	if (dummy && lverbose)
		printf("WARNING: Some drives don't like fixation in dummy mode.\n");

	scgp->silent++;
	if (is_tao(trackp) || is_packet(trackp)) {
		ret = scsi_close_tr_session(scgp, CL_TYPE_SESSION, 0,
				(dp->cdr_cmdflags&F_IMMED) != 0);
	} else {
		if (scsi_flush_cache(scgp, (dp->cdr_cmdflags&F_IMMED) != 0) < 0) {
			if (!scsi_in_progress(scgp))
				printf("Trouble flushing the cache\n");
		}
	}
	scgp->silent--;
	key = scg_sense_key(scgp);
	code = scg_sense_code(scgp);

	scgp->silent++;
	if (debug && !unit_ready(scgp)) {
		error("Early return from fixating. Ret: %d Key: %d, Code: %d\n", ret, key, code);
	}
	scgp->silent--;

	if (ret >= 0) {
		wait_unit_ready(scgp, 420/curspeed);	/* XXX Wait for ATAPI */
		waitfix_mmc(scgp, 420/curspeed);	/* XXX Wait for ATAPI */
		return (ret);
	}

	if ((dummy != 0 && (key != SC_ILLEGAL_REQUEST)) ||
		/*
		 * Try to suppress messages from drives that don't like fixation
		 * in -dummy mode.
		 */
		((dummy == 0) &&
		(((key != SC_UNIT_ATTENTION) && (key != SC_NOT_READY)) ||
				((code != 0x2E) && (code != 0x04))))) {
		/*
		 * UNIT ATTENTION/2E seems to be a magic for old Mitsumi ATAPI drives
		 * NOT READY/ code 4 qual 7 (logical unit not ready, operation in progress)
		 * seems to be a magic for newer Mitsumi ATAPI drives
		 * NOT READY/ code 4 qual 8 (logical unit not ready, long write in progress)
		 * seems to be a magic for SONY drives
		 * when returning early from fixating.
		 * Try to supress the error message in this case to make
		 * simple minded users less confused.
		 */
		scg_printerr(scgp);
		scg_printresult(scgp);	/* XXX restore key/code in future */
	}

	if (debug && !unit_ready(scgp)) {
		error("Early return from fixating. Ret: %d Key: %d, Code: %d\n", ret, key, code);
	}

	wait_unit_ready(scgp, 420);	 /* XXX Wait for ATAPI */
	waitfix_mmc(scgp, 420/curspeed); /* XXX Wait for ATAPI */

	if (!dummy &&
		(ret >= 0 || (key == SC_UNIT_ATTENTION && code == 0x2E))) {
		/*
		 * Some ATAPI drives (e.g. Mitsumi) imply the
		 * IMMED bit in the SCSI cdb. As there seems to be no
		 * way to properly check for the real end of the
		 * fixating process we wait for the expected time.
		 */
		gettimeofday(&stoptime, (struct timezone *)0);
		timevaldiff(&starttime, &stoptime);
		if (stoptime.tv_sec < (220 / curspeed)) {
			unsigned secs;

			if (lverbose) {
				printf("Actual fixating time: %ld seconds\n",
							(long)stoptime.tv_sec);
			}
			secs = (280 / curspeed) - stoptime.tv_sec;
			if (lverbose) {
				printf("ATAPI early return: sleeping %d seconds.\n",
								secs);
			}
			sleep(secs);
		}
	}
	return (ret);
}

char	*blank_types[] = {
	"entire disk",
	"PMA, TOC, pregap",
	"incomplete track",
	"reserved track",
	"tail of track",
	"closing of last session",
	"last session",
	"reserved blanking type",
};

LOCAL int
blank_mmc(scgp, dp, addr, blanktype)
	SCSI	*scgp;
	cdr_t	*dp;
	long	addr;
	int	blanktype;
{
	BOOL	cdrr	 = FALSE;	/* Read CD-R	*/
	BOOL	cdwr	 = FALSE;	/* Write CD-R	*/
	BOOL	cdrrw	 = FALSE;	/* Read CD-RW	*/
	BOOL	cdwrw	 = FALSE;	/* Write CD-RW	*/
	int	ret;

	mmc_check(scgp, &cdrr, &cdwr, &cdrrw, &cdwrw, NULL, NULL);
	if (!cdwrw)
		return (blank_dummy(scgp, dp, addr, blanktype));

	if (lverbose) {
		printf("Blanking %s\n", blank_types[blanktype & 0x07]);
		flush();
	}

	ret = scsi_blank(scgp, addr, blanktype, (dp->cdr_cmdflags&F_IMMED) != 0);
	if (ret < 0)
		return (ret);

	wait_unit_ready(scgp, 90*60/curspeed);	/* XXX Wait for ATAPI */
	waitfix_mmc(scgp, 90*60/curspeed);	/* XXX Wait for ATAPI */
	return (ret);
}

LOCAL int
send_opc_mmc(scgp, bp, cnt, doopc)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
	int	doopc;
{
	int	ret;

	scgp->silent++;
	ret = send_opc(scgp, bp, cnt, doopc);
	scgp->silent--;

	if (ret >= 0)
		return (ret);

	/* BEGIN CSTYLED */
	/*
	 * Hack for a mysterioys drive ....
	 * Device type    : Removable CD-ROM
	 * Version        : 0
	 * Response Format: 1
	 * Vendor_info    : 'RWD     '
	 * Identifikation : 'RW2224          '
	 * Revision       : '2.53'
	 * Device seems to be: Generic mmc CD-RW.
	 *
	 * Performing OPC...
	 * CDB:  54 01 00 00 00 00 00 00 00 00
	 * Sense Bytes: 70 00 06 00 00 00 00 0A 00 00 00 00 5A 03 00 00
	 * Sense Key: 0x6 Unit Attention, Segment 0
	 * Sense Code: 0x5A Qual 0x03 (operator selected write permit) Fru 0x0
	 * Sense flags: Blk 0 (not valid)
	 */
	/* END CSTYLED */
	if (scg_sense_key(scgp) == SC_UNIT_ATTENTION &&
	    scg_sense_code(scgp) == 0x5A &&
	    scg_sense_qual(scgp) == 0x03)
		return (0);

	/*
	 * Do not make the condition:
	 * "Power calibration area almost full" a fatal error.
	 * It just flags that we have a single and last chance to write now.
	 */
	if ((scg_sense_key(scgp) == SC_RECOVERABLE_ERROR ||
	    scg_sense_key(scgp) == SC_MEDIUM_ERROR) &&
	    scg_sense_code(scgp) == 0x73 &&
	    scg_sense_qual(scgp) == 0x01)
		return (0);

	/*
	 * Send OPC is optional.
	 */
	if (scg_sense_key(scgp) != SC_ILLEGAL_REQUEST) {
		if (scgp->silent <= 0)
			scg_printerr(scgp);
		return (ret);
	}
	return (0);
}

LOCAL int
opt1_mmc(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	int	oflags = dp->cdr_dstat->ds_cdrflags;

	if ((dp->cdr_dstat->ds_cdrflags & RF_AUDIOMASTER) != 0) {
		printf("Turning Audio Master Q. R. on\n");
		if (set_audiomaster_yamaha(scgp, dp, TRUE) < 0)
			return (-1);
		if (!debug && lverbose <= 1)
			dp->cdr_dstat->ds_cdrflags &= ~RF_PRATIP;
		if (getdisktype_mmc(scgp, dp) < 0) {
			dp->cdr_dstat->ds_cdrflags = oflags;
			return (-1);
		}
		dp->cdr_dstat->ds_cdrflags = oflags;
		if (oflags & RF_PRATIP) {
			msf_t   msf;
			lba_to_msf(dp->cdr_dstat->ds_first_leadin, &msf);
			printf("New start of lead in: %ld (%02d:%02d/%02d)\n",
				(long)dp->cdr_dstat->ds_first_leadin,
				msf.msf_min,
				msf.msf_sec,
				msf.msf_frame);
			lba_to_msf(dp->cdr_dstat->ds_maxblocks, &msf);
			printf("New start of lead out: %ld (%02d:%02d/%02d)\n",
				(long)dp->cdr_dstat->ds_maxblocks,
				msf.msf_min,
				msf.msf_sec,
				msf.msf_frame);
		}
	}
	if (mmc_isplextor(scgp)) {
		int	gcode;

		if ((dp->cdr_flags & (CDR_SINGLESESS|CDR_HIDE_CDR)) != 0) {
			if (ss_hide_plextor(scgp,
			    (dp->cdr_dstat->ds_cdrflags & RF_SINGLESESS) != 0,
			    (dp->cdr_dstat->ds_cdrflags & RF_HIDE_CDR) != 0) < 0)
				return (-1);
		}

		if ((dp->cdr_flags & CDR_SPEEDREAD) != 0) {
			if (speed_rd_plextor(scgp,
			    (dp->cdr_dstat->ds_cdrflags & RF_SPEEDREAD) != 0) < 0)
				return (-1);
		}

		if ((dp->cdr_cmdflags & F_SETDROPTS) ||
		    (wm_base(dp->cdr_dstat->ds_wrmode) == WM_SAO) ||
		    (wm_base(dp->cdr_dstat->ds_wrmode) == WM_RAW))
			gcode = do_gigarec_plextor(scgp);
		else
			gcode = gigarec_plextor(scgp, 0);
		if (gcode != 0) {
			msf_t   msf;

			dp->cdr_dstat->ds_first_leadin =
					gigarec_mult(gcode, dp->cdr_dstat->ds_first_leadin);
			dp->cdr_dstat->ds_maxblocks =
					gigarec_mult(gcode, dp->cdr_dstat->ds_maxblocks);

			if (oflags & RF_PRATIP) {
				lba_to_msf(dp->cdr_dstat->ds_first_leadin, &msf);
				printf("New start of lead in: %ld (%02d:%02d/%02d)\n",
					(long)dp->cdr_dstat->ds_first_leadin,
					msf.msf_min,
					msf.msf_sec,
					msf.msf_frame);
				lba_to_msf(dp->cdr_dstat->ds_maxblocks, &msf);
				printf("New start of lead out: %ld (%02d:%02d/%02d)\n",
					(long)dp->cdr_dstat->ds_maxblocks,
					msf.msf_min,
					msf.msf_sec,
					msf.msf_frame);
			}
		}
	}
	return (0);
}

LOCAL int
opt2_mmc(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	Uchar	mode[0x100];
	Uchar	moder[0x100];
	int	len;
	struct	cd_mode_page_05 *mp;
	struct	ricoh_mode_page_30 *rp = NULL;
	BOOL	burnfree = FALSE;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	if (!get_mode_params(scgp, 0x05, "CD write parameter",
			mode, (Uchar *)0, (Uchar *)0, (Uchar *)0, &len))
		return (-1);
	if (len == 0)
		return (-1);

	mp = (struct cd_mode_page_05 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);


	rp = get_justlink_ricoh(scgp, moder);

	if (dp->cdr_cdcap->BUF != 0) {
		burnfree = mp->BUFE != 0;
	} else if ((dp->cdr_flags & CDR_BURNFREE) != 0) {
		burnfree = rp && rp->BUEFE != 0;
	}

	if (lverbose && (dp->cdr_flags & CDR_BURNFREE) != 0)
		printf("BURN-Free is %s.\n", burnfree?"ON":"OFF");

	if (!burnfree && (dp->cdr_dstat->ds_cdrflags & RF_BURNFREE) != 0) {
		printf("Turning BURN-Free on\n");
		burnfree = TRUE;
	}
	if (burnfree && (dp->cdr_dstat->ds_cdrflags & RF_BURNFREE) == 0) {
		printf("Turning BURN-Free off\n");
		burnfree = FALSE;
	}
	if (dp->cdr_cdcap->BUF != 0) {
		mp->BUFE = burnfree?1:0;
	} else if ((dp->cdr_flags & CDR_BURNFREE) != 0) {

		if (rp)
			rp->BUEFE = burnfree?1:0;
	}
	if (rp) {
		/*
		 * Clear Just-Link counter
		 */
		i_to_2_byte(rp->link_counter, 0);
		if (xdebug)
			scg_prbytes("Mode Select Data ", moder, moder[0]+1);

		if (!set_mode_params(scgp, "Ricoh Vendor Page", moder, moder[0]+1, 0, -1))
			return (-1);
		rp = get_justlink_ricoh(scgp, moder);
	}

#ifdef	DEBUG
	scg_prbytes("CD write parameter:", (Uchar *)mode, len);
#endif
	if (!set_mode_params(scgp, "CD write parameter", mode, len, 0, -1))
		return (-1);

	if (mmc_isplextor(scgp)) {
		/*
		 * Clear Burn-Proof counter
		 */
		scgp->silent++;
		bpc_plextor(scgp, 1, NULL);
		scgp->silent--;

		do_varirec_plextor(scgp);
	}

	return (0);
}

LOCAL int
scsi_sony_write(scgp, bp, sectaddr, size, blocks, islast)
	SCSI	*scgp;
	caddr_t	bp;		/* address of buffer */
	long	sectaddr;	/* disk address (sector) to put */
	long	size;		/* number of bytes to transfer */
	int	blocks;		/* sector count */
	BOOL	islast;		/* last write for track */
{
	return (write_xg5(scgp, bp, sectaddr, size, blocks));
}

Uchar	db2df[] = {
	0x00,			/*  0 2352 bytes of raw data			*/
	0xFF,			/*  1 2368 bytes (raw data + P/Q Subchannel)	*/
	0xFF,			/*  2 2448 bytes (raw data + P-W Subchannel)	*/
	0xFF,			/*  3 2448 bytes (raw data + P-W raw Subchannel)*/
	0xFF,			/*  4 -    Reserved				*/
	0xFF,			/*  5 -    Reserved				*/
	0xFF,			/*  6 -    Reserved				*/
	0xFF,			/*  7 -    Vendor specific			*/
	0x10,			/*  8 2048 bytes Mode 1 (ISO/IEC 10149)		*/
	0x30,			/*  9 2336 bytes Mode 2 (ISO/IEC 10149)		*/
	0xFF,			/* 10 2048 bytes Mode 2! (CD-ROM XA form 1)	*/
	0xFF,			/* 11 2056 bytes Mode 2 (CD-ROM XA form 1)	*/
	0xFF,			/* 12 2324 bytes Mode 2 (CD-ROM XA form 2)	*/
	0xFF,			/* 13 2332 bytes Mode 2 (CD-ROM XA 1/2+subhdr)	*/
	0xFF,			/* 14 -    Reserved				*/
	0xFF,			/* 15 -    Vendor specific			*/
};

LOCAL int
gen_cue_mmc(trackp, vcuep, needgap)
	track_t	*trackp;
	void	*vcuep;
	BOOL	needgap;
{
	int	tracks = trackp->tracks;
	int	i;
	struct mmc_cue	**cuep = vcuep;
	struct mmc_cue	*cue;
	struct mmc_cue	*cp;
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
		if (is_quadro(&trackp[i]))
			ctl |= TM_QUADRO << 4;
		df = db2df[trackp[i].dbtype & 0x0F];
		if (trackp[i].tracktype == TOC_XA2 &&
		    trackp[i].sectype   == (SECT_MODE_2_MIX|ST_MODE_RAW)) {
			/*
			 * Hack for CUE with MODE2/CDI and
			 * trackp[i].dbtype == DB_RAW
			 */
			df = 0x21;
		}

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
			df &= ~7;	/* Mask off data size & nonRAW subch */
			if (df < 0x10)
				df |= 1;
			else
				df |= 4;
			if (trackp[0].flags & TI_TEXT)	/* CD-Text in Lead-in*/
				df |= 0x40;
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
	if (is_copy(&trackp[i]))
		ctl |= TM_ALLOW_COPY << 4;
	if (is_quadro(&trackp[i]))
		ctl |= TM_QUADRO << 4;
	df = db2df[trackp[tracks+1].dbtype & 0x0F];
	if (trackp[i].tracktype == TOC_XA2 &&
	    trackp[i].sectype   == (SECT_MODE_2_MIX|ST_MODE_RAW)) {
		/*
		 * Hack for CUE with MODE2/CDI and
		 * trackp[i].dbtype == DB_RAW
		 */
		df = 0x21;
	}
	df &= ~7;	/* Mask off data size & nonRAW subch */
	if (df < 0x10)
		df |= 1;
	else
		df |= 4;
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
	struct mmc_cue	*cp;	/* The target cue entry		*/
	int	ca;		/* Control/adr for this entry	*/
	int	tno;		/* Track number for this entry	*/
	int	idx;		/* Index for this entry		*/
	int	dataform;	/* Data format for this entry	*/
	int	scms;		/* Serial copy management	*/
	msf_t	*mp;		/* MSF value for this entry	*/
{
	cp->cs_ctladr = ca;		/* XXX wie lead in */
	cp->cs_tno = tno;
	cp->cs_index = idx;
	cp->cs_dataform = dataform;	/* XXX wie lead in */
	cp->cs_scms = scms;
	cp->cs_min = mp->msf_min;
	cp->cs_sec = mp->msf_sec;
	cp->cs_frame = mp->msf_frame;
}

LOCAL int
send_cue_mmc(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	struct mmc_cue	*cp;
	int		ncue;
	int		ret;
	Uint		i;

	for (i = 1; i <= trackp->tracks; i++) {
		if (trackp[i].tracksize < (tsize_t)0) {
			errmsgno(EX_BAD, "Track %d has unknown length.\n", i);
			return (-1);
		}
	}
	ncue = (*dp->cdr_gen_cue)(trackp, &cp, FALSE);

	scgp->silent++;
	ret = send_cue_sheet(scgp, (caddr_t)cp, ncue*8);
	scgp->silent--;
	free(cp);
	if (ret < 0) {
		errmsgno(EX_BAD, "CUE sheet not accepted. Retrying with minimum pregapsize = 1.\n");
		ncue = (*dp->cdr_gen_cue)(trackp, &cp, TRUE);
		ret = send_cue_sheet(scgp, (caddr_t)cp, ncue*8);
		if (ret < 0) {
			errmsgno(EX_BAD,
			"CUE sheet still not accepted. Please try to write in RAW (-raw96r) mode.\n");
		}
		free(cp);
	}
	return (ret);
}

LOCAL int
stats_mmc(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	Uchar mode[256];
	struct	ricoh_mode_page_30 *rp;
	UInt32_t count;

	if (mmc_isplextor(scgp) && lverbose) {
		int	sels;
		int	maxs;
		int	lasts;

		/*
		 * Run it in silent mode as old drives do not support it.
		 * As this function looks to be a part of the PowerRec
		 * features, we may want to check
		 * dp->cdr_flags & CDR_FORCESPEED
		 */
		scgp->silent++;
		if (get_speeds_plextor(scgp, &sels, &maxs, &lasts) >= 0) {
			printf("Last selected write speed: %dx\n",
						sels / 176);
			printf("Max media write speed:     %dx\n",
						maxs / 176);
			printf("Last actual write speed:   %dx\n",
						lasts / 176);
		}
		scgp->silent--;
	}

	if ((dp->cdr_dstat->ds_cdrflags & RF_BURNFREE) == 0)
		return (0);

	if (mmc_isplextor(scgp)) {
		int	i = 0;
		int	ret;

		/*
		 * Read Burn-Proof counter
		 */
		scgp->silent++;
		ret = bpc_plextor(scgp, 2, &i);
		scgp->silent--;
		if (ret < 0)
			return (-1);
		count = i;
		/*
		 * Clear Burn-Proof counter
		 */
		bpc_plextor(scgp, 1, NULL);
	} else {
		rp = get_justlink_ricoh(scgp, mode);
		if (rp)
			count = a_to_u_2_byte(rp->link_counter);
		else
			return (-1);
	}
	if (lverbose) {
		if (count == 0)
			printf("BURN-Free was never needed.\n");
		else
			printf("BURN-Free was %d times used.\n",
				(int)count);
	}
	return (0);
}
/*--------------------------------------------------------------------------*/
LOCAL BOOL
mmc_isplextor(scgp)
	SCSI	*scgp;
{
	if (scgp->inq != NULL &&
			strncmp(scgp->inq->inq_vendor_info, "PLEXTOR", 7) == 0) {
		return (TRUE);
	}
	return (FALSE);
}

LOCAL BOOL
mmc_isyamaha(scgp)
	SCSI	*scgp;
{
	if (scgp->inq != NULL &&
			strncmp(scgp->inq->inq_vendor_info, "YAMAHA", 6) == 0) {
		return (TRUE);
	}
	return (FALSE);
}

LOCAL void
do_varirec_plextor(scgp)
	SCSI	*scgp;
{
	char	*p;
	int	voff;

	p = hasdrvopt(driveropts, "varirec");
	if (p == NULL || curspeed != 4) {
		if (check_varirec_plextor(scgp) >= 0)
			varirec_plextor(scgp, FALSE, 0);
	} else {
		char	*ep;
		ep = astoi(p, &voff);
		if (*ep != '\0' && *ep != ',')
			comerrno(EX_BAD,
				"Bad varirec value '%s'.\n", p);
		if (check_varirec_plextor(scgp) < 0)
			comerrno(EX_BAD, "Drive does not support VariRec.\n");
		varirec_plextor(scgp, TRUE, voff);
	}
}

/*
 * GigaRec value table
 */
struct gr {
	Uchar	val;
	char	vadd;
	char	*name;
} gr[] = {
	{ 0x00,	0,  "off", },
	{ 0x00,	0,  "1.0", },
	{ 0x01,	2,  "1.2", },
	{ 0x02,	3,  "1.3", },
	{ 0x03,	4,  "1.4", },
	{ 0x04,	1,  "1.1", },
	{ 0x81,	-2, "0.8", },
	{ 0x82,	-3, "0.7", },
	{ 0x83,	-4, "0.6", },
	{ 0x84,	-1, "0.9", },
	{ 0x00,	0,  NULL, },
};

LOCAL int
do_gigarec_plextor(scgp)
	SCSI	*scgp;
{
	char	*p;
	int	val = 0;	/* Make silly GCC happy */

	p = hasdrvopt(driveropts, "gigarec");
	if (p == NULL) {
		if (check_gigarec_plextor(scgp) >= 0)
			gigarec_plextor(scgp, 0);
	} else {
		struct gr *gp = gr;

		if (strlen(p) >= 3) {
			for (; gp->name != NULL; gp++) {
				if (strncmp(p, gp->name, 3) == 0) {
					if (p[3] != '\0' && p[3] != ',')
						continue;
					val = gp->val;
					break;
				}
			}
		}
		if (gp->name == NULL)
			comerrno(EX_BAD,
				"Bad gigarec value '%s'.\n", p);
		if (check_gigarec_plextor(scgp) < 0)
			comerrno(EX_BAD, "Drive does not support GigaRec.\n");
		return (gigarec_plextor(scgp, val));
	}
	return (0);
}

LOCAL int
drivemode_plextor(scgp, bp, cnt, modecode, modeval)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
	int	modecode;
	void	*modeval;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	if (modeval == NULL) {
		scmd->flags |= SCG_RECV_DATA;
		scmd->addr = bp;
		scmd->size = cnt;
	} else {
		scmd->cdb.g5_cdb.res = 0x08;
	}
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xE9;
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.addr[0] = modecode;
	if (modeval)
		movebytes(modeval, &scmd->cdb.g1_cdb.addr[1], 6);
	else
		i_to_2_byte(&scmd->cdb.cmd_cdb[9], cnt);

	scgp->cmdname = "plextor drive mode";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

/*
 * #defines for drivemode_plextor()...
 */
#define	MODE_CODE_SH	0x01	/* Mode code for Single Session & Hide-CDR */
#define	MB1_SS		0x01	/* Single Session Mode			   */
#define	MB1_HIDE_CDR	0x02	/* Hide CDR Media			   */

#define	MODE_CODE_VREC	0x02	/* Mode code for Vari Rec		   */

#define	MODE_CODE_GREC	0x04	/* Mode code for Giga Rec		   */

#define	MODE_CODE_SPEED	0xbb	/* Mode code for Speed Read		   */
#define	MBbb_SPEAD_READ	0x01	/* Spead Read				   */
				/* Danach Speed auf 0xFFFF 0xFFFF setzen   */

LOCAL int
drivemode2_plextor(scgp, bp, cnt, modecode, modeval)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
	int	modecode;
	void	*modeval;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	if (modeval == NULL) {
		scmd->flags |= SCG_RECV_DATA;
		scmd->addr = bp;
		scmd->size = cnt;
	} else {
		scmd->cdb.g5_cdb.res = 0x08;
	}
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xED;
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.addr[0] = modecode;
	if (modeval)
		scmd->cdb.g5_cdb.reladr = *(char *)modeval != 0 ? 1 : 0;
	else
		i_to_2_byte(&scmd->cdb.cmd_cdb[8], cnt);

	scgp->cmdname = "plextor drive mode2";

	if (scg_cmd(scgp) < 0)
		return (-1);

	return (0);
}

LOCAL int
check_varirec_plextor(scgp)
	SCSI	*scgp;
{
	int	modecode = 2;
	Uchar	pgetmode[8];

	fillbytes(pgetmode, sizeof (pgetmode), '\0');
	scgp->silent++;
	if (drivemode_plextor(scgp, (caddr_t)pgetmode, sizeof (pgetmode), modecode, NULL) < 0) {
		scgp->silent--;
		return (-1);
	}
	scgp->silent--;

	return (0);
}

LOCAL int
check_gigarec_plextor(scgp)
	SCSI	*scgp;
{
	int	modecode = 4;
	Uchar	pgetmode[8];

	fillbytes(pgetmode, sizeof (pgetmode), '\0');
	scgp->silent++;
	if (drivemode_plextor(scgp, (caddr_t)pgetmode, sizeof (pgetmode), modecode, NULL) < 0) {
		scgp->silent--;
		return (-1);
	}
	scgp->silent--;

	return (0);
}

LOCAL int
varirec_plextor(scgp, on, val)
	SCSI	*scgp;
	BOOL	on;
	int	val;
{
	int	modecode = 2;
	Uchar	psetmode[8];
	Uchar	pgetmode[8];

	fillbytes(pgetmode, sizeof (pgetmode), '\0');
	scgp->silent++;
	if (drivemode_plextor(scgp, (caddr_t)pgetmode, sizeof (pgetmode), modecode, NULL) < 0) {
		scgp->silent--;
		return (-1);
	}
	scgp->silent--;

	if (lverbose > 1)
		scg_prbytes("Modes", pgetmode, sizeof (pgetmode));


	fillbytes(psetmode, sizeof (psetmode), '\0');
	psetmode[0] = on?1:0;
	if (on) {
		/*
		 * Before -2 .. +2
		 * Since PX-716: -4 .. +4
		 */
		if (val < -2 || val > 2)
			comerrno(EX_BAD, "Bad VariRec offset %d\n", val);
		printf("Turning Varirec on.\n");
		printf("Varirec offset is %d.\n", val);

		if (val > 0) {
			psetmode[1] = val & 0x7F;
		} else {
			psetmode[1] = (-val) & 0x7F;
			psetmode[1] |= 0x80;
		}
	}

	if (drivemode_plextor(scgp, NULL, 0, modecode, psetmode) < 0)
		return (-1);

	fillbytes(pgetmode, sizeof (pgetmode), '\0');
	if (drivemode_plextor(scgp, (caddr_t)pgetmode, sizeof (pgetmode), modecode, NULL) < 0)
		return (-1);

	if (lverbose > 1)
		scg_prbytes("Modes", pgetmode, sizeof (pgetmode));

	return (0);
}

LOCAL int
gigarec_plextor(scgp, val)
	SCSI	*scgp;
	int	val;
{
	int	modecode = 4;
	Uchar	psetmode[8];
	Uchar	pgetmode[8];

	fillbytes(pgetmode, sizeof (pgetmode), '\0');
	scgp->silent++;
	if (drivemode_plextor(scgp, (caddr_t)pgetmode, sizeof (pgetmode), modecode, NULL) < 0) {
		scgp->silent--;
		return (-1);
	}
	scgp->silent--;

	if (lverbose > 1)
		scg_prbytes("Modes", pgetmode, sizeof (pgetmode));


	fillbytes(psetmode, sizeof (psetmode), '\0');
	psetmode[1] = val;

	if (drivemode_plextor(scgp, NULL, 0, modecode, psetmode) < 0)
		return (-1);

	fillbytes(pgetmode, sizeof (pgetmode), '\0');
	if (drivemode_plextor(scgp, (caddr_t)pgetmode, sizeof (pgetmode), modecode, NULL) < 0)
		return (-1);

	if (lverbose > 1)
		scg_prbytes("Modes", pgetmode, sizeof (pgetmode));

	{
		struct gr *gp = gr;

		for (; gp->name != NULL; gp++) {
			if (pgetmode[3] == gp->val)
				break;
		}
		if (gp->name == NULL)
			printf("Unknown GigaRec value 0x%X.\n", pgetmode[3]);
		else
			printf("GigaRec %sis %s.\n", gp->val?"value ":"", gp->name);
	}
	return (pgetmode[3]);
}

LOCAL Int32_t
gigarec_mult(code, val)
	int	code;
	Int32_t	val;
{
	Int32_t	add;
	struct gr *gp = gr;

	for (; gp->name != NULL; gp++) {
		if (code == gp->val)
			break;
	}
	if (gp->vadd == 0)
		return (val);

	add = val * gp->vadd / 10;
	return (val + add);
}

LOCAL int
check_ss_hide_plextor(scgp)
	SCSI	*scgp;
{
	int	modecode = 1;
	Uchar	pgetmode[8];

	fillbytes(pgetmode, sizeof (pgetmode), '\0');
	scgp->silent++;
	if (drivemode_plextor(scgp, (caddr_t)pgetmode, sizeof (pgetmode), modecode, NULL) < 0) {
		scgp->silent--;
		return (-1);
	}
	scgp->silent--;

	return (pgetmode[2] & 0x03);
}

LOCAL int
check_speed_rd_plextor(scgp)
	SCSI	*scgp;
{
	int	modecode = 0xBB;
	Uchar	pgetmode[8];

	fillbytes(pgetmode, sizeof (pgetmode), '\0');
	scgp->silent++;
	if (drivemode_plextor(scgp, (caddr_t)pgetmode, sizeof (pgetmode), modecode, NULL) < 0) {
		scgp->silent--;
		return (-1);
	}
	scgp->silent--;

	return (pgetmode[2] & 0x01);
}

LOCAL int
check_powerrec_plextor(scgp)
	SCSI	*scgp;
{
	int	modecode = 0;
	Uchar	pgetmode[8];

	fillbytes(pgetmode, sizeof (pgetmode), '\0');
	scgp->silent++;
	if (drivemode2_plextor(scgp, (caddr_t)pgetmode, sizeof (pgetmode), modecode, NULL) < 0) {
		scgp->silent--;
		return (-1);
	}
	scgp->silent--;

	if (pgetmode[2] & 1)
		return (1);

	return (0);
}

LOCAL int
ss_hide_plextor(scgp, do_ss, do_hide)
	SCSI	*scgp;
	BOOL	do_ss;
	BOOL	do_hide;
{
	int	modecode = 1;
	Uchar	psetmode[8];
	Uchar	pgetmode[8];
	BOOL	is_ss;
	BOOL	is_hide;

	fillbytes(pgetmode, sizeof (pgetmode), '\0');
	scgp->silent++;
	if (drivemode_plextor(scgp, (caddr_t)pgetmode, sizeof (pgetmode), modecode, NULL) < 0) {
		scgp->silent--;
		return (-1);
	}
	scgp->silent--;

	if (lverbose > 1)
		scg_prbytes("Modes", pgetmode, sizeof (pgetmode));


	is_ss = (pgetmode[2] & MB1_SS) != 0;
	is_hide = (pgetmode[2] & MB1_HIDE_CDR) != 0;

	if (lverbose > 0) {
		printf("Single session is %s.\n", is_ss ? "ON":"OFF");
		printf("Hide CDR is %s.\n", is_hide ? "ON":"OFF");
	}

	fillbytes(psetmode, sizeof (psetmode), '\0');
	psetmode[0] = pgetmode[2];		/* Copy over old values */
	if (do_ss >= 0) {
		if (do_ss)
			psetmode[0] |= MB1_SS;
		else
			psetmode[0] &= ~MB1_SS;
	}
	if (do_hide >= 0) {
		if (do_hide)
			psetmode[0] |= MB1_HIDE_CDR;
		else
			psetmode[0] &= ~MB1_HIDE_CDR;
	}

	if (do_ss >= 0 && do_ss != is_ss)
		printf("Turning single session %s.\n", do_ss?"on":"off");
	if (do_hide >= 0 && do_hide != is_hide)
		printf("Turning hide CDR %s.\n", do_hide?"on":"off");

	if (drivemode_plextor(scgp, NULL, 0, modecode, psetmode) < 0)
		return (-1);

	fillbytes(pgetmode, sizeof (pgetmode), '\0');
	if (drivemode_plextor(scgp, (caddr_t)pgetmode, sizeof (pgetmode), modecode, NULL) < 0)
		return (-1);

	if (lverbose > 1)
		scg_prbytes("Modes", pgetmode, sizeof (pgetmode));

	return (0);
}

LOCAL int
speed_rd_plextor(scgp, do_speedrd)
	SCSI	*scgp;
	BOOL	do_speedrd;
{
	int	modecode = 0xBB;
	Uchar	psetmode[8];
	Uchar	pgetmode[8];
	BOOL	is_speedrd;

	fillbytes(pgetmode, sizeof (pgetmode), '\0');
	scgp->silent++;
	if (drivemode_plextor(scgp, (caddr_t)pgetmode, sizeof (pgetmode), modecode, NULL) < 0) {
		scgp->silent--;
		return (-1);
	}
	scgp->silent--;

	if (lverbose > 1)
		scg_prbytes("Modes", pgetmode, sizeof (pgetmode));


	is_speedrd = (pgetmode[2] & MBbb_SPEAD_READ) != 0;

	if (lverbose > 0)
		printf("Speed-Read is %s.\n", is_speedrd ? "ON":"OFF");

	fillbytes(psetmode, sizeof (psetmode), '\0');
	psetmode[0] = pgetmode[2];		/* Copy over old values */
	if (do_speedrd >= 0) {
		if (do_speedrd)
			psetmode[0] |= MBbb_SPEAD_READ;
		else
			psetmode[0] &= ~MBbb_SPEAD_READ;
	}

	if (do_speedrd >= 0 && do_speedrd != is_speedrd)
		printf("Turning Speed-Read %s.\n", do_speedrd?"on":"off");

	if (drivemode_plextor(scgp, NULL, 0, modecode, psetmode) < 0)
		return (-1);

	fillbytes(pgetmode, sizeof (pgetmode), '\0');
	if (drivemode_plextor(scgp, (caddr_t)pgetmode, sizeof (pgetmode), modecode, NULL) < 0)
		return (-1);

	if (lverbose > 1)
		scg_prbytes("Modes", pgetmode, sizeof (pgetmode));

	/*
	 * Set current read speed to new max value.
	 */
	if (do_speedrd >= 0 && do_speedrd != is_speedrd)
		scsi_set_speed(scgp, 0xFFFF, -1, ROTCTL_CAV);

	return (0);
}

LOCAL int
powerrec_plextor(scgp, do_powerrec)
	SCSI	*scgp;
	BOOL	do_powerrec;
{
	int	modecode = 0;
	Uchar	psetmode[8];
	Uchar	pgetmode[8];
	BOOL	is_powerrec;
	int	speed;

	fillbytes(pgetmode, sizeof (pgetmode), '\0');
	scgp->silent++;
	if (drivemode2_plextor(scgp, (caddr_t)pgetmode, sizeof (pgetmode), modecode, NULL) < 0) {
		scgp->silent--;
		return (-1);
	}
	scgp->silent--;

	if (lverbose > 1)
		scg_prbytes("Modes", pgetmode, sizeof (pgetmode));


	is_powerrec = (pgetmode[2] & 1) != 0;

	speed = a_to_u_2_byte(&pgetmode[4]);

	if (lverbose > 0) {
		printf("Power-Rec is %s.\n", is_powerrec ? "ON":"OFF");
		printf("Power-Rec write speed:     %dx (recommended)\n", speed / 176);
	}

	fillbytes(psetmode, sizeof (psetmode), '\0');
	psetmode[0] = pgetmode[2];		/* Copy over old values */
	if (do_powerrec >= 0) {
		if (do_powerrec)
			psetmode[0] |= 1;
		else
			psetmode[0] &= ~1;
	}

	if (do_powerrec >= 0 && do_powerrec != is_powerrec)
		printf("Turning Power-Rec %s.\n", do_powerrec?"on":"off");

	if (drivemode2_plextor(scgp, NULL, 0, modecode, psetmode) < 0)
		return (-1);

	fillbytes(pgetmode, sizeof (pgetmode), '\0');
	if (drivemode2_plextor(scgp, (caddr_t)pgetmode, sizeof (pgetmode), modecode, NULL) < 0)
		return (-1);

	if (lverbose > 1)
		scg_prbytes("Modes", pgetmode, sizeof (pgetmode));

	return (0);
}

LOCAL int
get_speeds_plextor(scgp, selp, maxp, lastp)
	SCSI	*scgp;
	int	*selp;
	int	*maxp;
	int	*lastp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;
	char	buf[10];
	int	i;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	fillbytes((caddr_t)buf, sizeof (buf), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->flags |= SCG_RECV_DATA;
	scmd->addr = buf;
	scmd->size = sizeof (buf);
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xEB;
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);

	i_to_2_byte(&scmd->cdb.cmd_cdb[8], sizeof (buf));

	scgp->cmdname = "plextor get speedlist";

	if (scg_cmd(scgp) < 0)
		return (-1);

	i = a_to_u_2_byte(&buf[4]);
	if (selp)
		*selp = i;

	i = a_to_u_2_byte(&buf[6]);
	if (maxp)
		*maxp = i;

	i = a_to_u_2_byte(&buf[8]);
	if (lastp)
		*lastp = i;

	return (0);
}

LOCAL int
bpc_plextor(scgp, mode, bpp)
	SCSI	*scgp;
	int	mode;
	int	*bpp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;
	char	buf[4];
	int	i;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	fillbytes((caddr_t)buf, sizeof (buf), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->flags |= SCG_RECV_DATA;
	scmd->addr = buf;
	scmd->size = sizeof (buf);
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xF5;
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);

	scmd->cdb.g5_cdb.addr[1] = 0x08;
	scmd->cdb.g5_cdb.addr[2] = mode;

	i_to_2_byte(&scmd->cdb.cmd_cdb[8], sizeof (buf));

	scgp->cmdname = "plextor read bpc";

	if (scg_cmd(scgp) < 0)
		return (-1);

	if (scg_getresid(scgp) > 2)
		return (0);

	i = a_to_u_2_byte(buf);
	if (bpp)
		*bpp = i;

	return (0);
}

LOCAL int
plextor_enable(scgp)
	SCSI	*scgp;
{
	int		ret;
	UInt32_t	key[4];

	scgp->silent++;
	ret = plextor_getauth(scgp, key, sizeof (key));
	scgp->silent--;
	if (ret < 0)
		return (ret);

	scgp->silent++;
	ret = plextor_setauth(scgp, key, sizeof (key));
	scgp->silent--;
	return (ret);
}

LOCAL int
plextor_disable(scgp)
	SCSI	*scgp;
{
	return (plextor_setauth(scgp, NULL, 0));
}

LOCAL int
plextor_getauth(scgp, dp, cnt)
	SCSI	*scgp;
	void	*dp;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->flags |= SCG_RECV_DATA;
	scmd->addr = dp;
	scmd->size = cnt;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xD4;
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	scmd->cdb.cmd_cdb[10] = cnt;

	scgp->cmdname = "plextor getauth";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

LOCAL int
plextor_setauth(scgp, dp, cnt)
	SCSI	*scgp;
	void	*dp;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->addr = dp;
	scmd->size = cnt;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xD5;
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	scmd->cdb.cmd_cdb[1] = 0x01;
	if (cnt != 0)				/* If cnt == 0 clearauth */
		scmd->cdb.cmd_cdb[2] = 0x01;
	scmd->cdb.cmd_cdb[10] = cnt;

	scgp->cmdname = "plextor setauth";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

LOCAL int
set_audiomaster_yamaha(scgp, dp, keep_mode)
	SCSI	*scgp;
	cdr_t	*dp;
	BOOL	keep_mode;
{
	Uchar	mode[0x100];
	int	len;
	int	ret = 0;
	struct	cd_mode_page_05 *mp;

	if (xdebug && !keep_mode)
		printf("Checking for Yamaha Audio Master feature: ");

	/*
	 * Do not reset mp->test_write (-dummy) here.
	 */
	deflt_writemodes_mmc(scgp, FALSE);

	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	scgp->silent++;
	if (!get_mode_params(scgp, 0x05, "CD write parameter",
			mode, (Uchar *)0, (Uchar *)0, (Uchar *)0, &len)) {
		scgp->silent--;
		return (-1);
	}
	if (len == 0) {
		scgp->silent--;
		return (-1);
	}

	mp = (struct cd_mode_page_05 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);
#ifdef	DEBUG
	scg_prbytes("CD write parameter:", (Uchar *)mode, len);
#endif

	/*
	 * Do not set mp->test_write (-dummy) here. It should be set
	 * only at one place and only one time.
	 */
	mp->BUFE = 0;

	mp->write_type = 8;
	mp->track_mode = 0;
	mp->dbtype = DB_RAW;

	if (!set_mode_params(scgp, "CD write parameter", mode, len, 0, -1))
		ret = -1;

	/*
	 * Do not reset mp->test_write (-dummy) here.
	 */
	if (!keep_mode || ret < 0)
		deflt_writemodes_mmc(scgp, FALSE);
	scgp->silent--;

	return (ret);
}

EXPORT struct ricoh_mode_page_30 *
get_justlink_ricoh(scgp, mode)
	SCSI	*scgp;
	Uchar	*mode;
{
	Uchar	modec[0x100];
	int	len;
	struct	ricoh_mode_page_30 *mp;

	scgp->silent++;
	if (!get_mode_params(scgp, 0x30, "Ricoh Vendor Page", mode, modec, NULL, NULL, &len)) {
		scgp->silent--;
		return ((struct ricoh_mode_page_30 *)0);
	}
	scgp->silent--;

	/*
	 * SCSI mode header + 6 bytes mode page 30.
	 * This is including the Burn-Free counter.
	 */
	if (len < 10)
		return ((struct ricoh_mode_page_30 *)0);

	if (xdebug) {
		error("Mode len: %d\n", len);
		scg_prbytes("Mode Sense Data ", mode, len);
		scg_prbytes("Mode Sence CData", modec, len);
	}

	mp = (struct ricoh_mode_page_30 *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	/*
	 * 6 bytes mode page 30.
	 * This is including the Burn-Free counter.
	 */
	if ((len - ((Uchar *)mp - mode) -1) < 5)
		return ((struct ricoh_mode_page_30 *)0);

	if (xdebug) {
		error("Burnfree counter: %d\n", a_to_u_2_byte(mp->link_counter));
	}
	return (mp);
}

LOCAL int
force_speed_yamaha(scgp, readspeed, writespeed)
	SCSI	*scgp;
	int	readspeed;
	int	writespeed;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xBB;
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);

	if (readspeed < 0)
		i_to_2_byte(&scmd->cdb.g5_cdb.addr[0], 0xFFFF);
	else
		i_to_2_byte(&scmd->cdb.g5_cdb.addr[0], readspeed);
	if (writespeed < 0)
		i_to_2_byte(&scmd->cdb.g5_cdb.addr[2], 0xFFFF);
	else
		i_to_2_byte(&scmd->cdb.g5_cdb.addr[2], writespeed);

	scmd->cdb.cmd_cdb[11] = 0x80;

	scgp->cmdname = "yamaha force cd speed";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

LOCAL BOOL
get_tattoo_yamaha(scgp, print, irp, orp)
	SCSI	*scgp;
	BOOL	print;
	Int32_t	*irp;
	Int32_t	*orp;
{
	Uchar	mode[0x100];
	int	len;
	UInt32_t ival;
	UInt32_t oval;
	Uchar	*mp;

	scgp->silent++;
	if (!get_mode_params(scgp, 0x31, "Yamaha Tattoo Page", mode, NULL, NULL, NULL, &len)) {
		scgp->silent--;
		return (FALSE);
	}
	scgp->silent--;

	/*
	 * SCSI mode header + 16 bytes mode page 31.
	 * This is including the Burn-Free counter.
	 */
	if (len < 20)
		return (FALSE);

	mp = (Uchar *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	/*
	 * 10 bytes mode page 31.
	 * This is including the Burn-Free counter.
	 */
	if ((len - ((Uchar *)mp - mode) -1) < 10)
		return (FALSE);

	ival = a_to_u_3_byte(&mp[4]);
	oval = a_to_u_3_byte(&mp[7]);

	if (irp)
		*irp = ival;
	if (orp)
		*orp = oval;

	if (print && ival > 0 && oval > 0) {
		printf("DiskT@2 inner r: %d\n", (int)ival);
		printf("DiskT@2 outer r: %d\n", (int)oval);
		printf("DiskT@2 image size: 3744 x %d pixel.\n",
						(int)(oval-ival)+1);
	}

	return (TRUE);
}

LOCAL int
do_tattoo_yamaha(scgp, f)
	SCSI	*scgp;
	FILE	*f;
{
	Int32_t ival = 0;
	Int32_t oval = 0;
	Int32_t	lines;
	off_t	fsize;
	char	*buf = scgp->bufptr;
	long	bufsize = scgp->maxbuf;
	long	nsecs;
	long	amt;

	nsecs = bufsize / 2048;
	bufsize = nsecs * 2048;

	if (!get_tattoo_yamaha(scgp, FALSE, &ival, &oval)) {
		errmsgno(EX_BAD, "Cannot get DiskT@2 info.\n");
		return (-1);
	}

	if (ival == 0 || oval == 0) {
		errmsgno(EX_BAD, "DiskT@2 info not valid.\n");
		return (-1);
	}

	lines = oval - ival + 1;
	fsize = filesize(f);
	if ((fsize % 3744) != 0 || fsize < (lines*3744)) {
		errmsgno(EX_BAD, "Illegal DiskT@2 file size.\n");
		return (-1);
	}
	if (fsize > (lines*3744))
		fsize = lines*3744;

	if (lverbose)
		printf("Starting to write DiskT@2 data.\n");
	fillbytes(buf, bufsize, '\0');
	if ((amt = fileread(f, buf, bufsize)) <= 0) {
		errmsg("DiskT@2 file read error.\n");
		return (-1);
	}

	if (yamaha_write_buffer(scgp, 1, 0, ival, amt/2048, buf, amt) < 0) {
		errmsgno(EX_BAD, "DiskT@2 1st write error.\n");
		return (-1);
	}
	amt = (amt+2047) / 2048 * 2048;
	fsize -= amt;

	while (fsize > 0) {
		fillbytes(buf, bufsize, '\0');
		if ((amt = fileread(f, buf, bufsize)) <= 0) {
			errmsg("DiskT@2 file read error.\n");
			return (-1);
		}
		amt = (amt+2047) / 2048 * 2048;
		fsize -= amt;
		if (yamaha_write_buffer(scgp, 1, 0, 0, amt/2048, buf, amt) < 0) {
			errmsgno(EX_BAD, "DiskT@2 write error.\n");
			return (-1);
		}
	}

	if (yamaha_write_buffer(scgp, 1, 0, oval, 0, buf, 0) < 0) {
		errmsgno(EX_BAD, "DiskT@2 final error.\n");
		return (-1);
	}

	wait_unit_ready(scgp, 1000);	/* Wait for DiskT@2 */
	waitfix_mmc(scgp, 1000);	/* Wait for DiskT@2 */

	return (0);
}

/*
 * Yamaha specific version of 'write buffer' that offers an additional
 * Parameter Length 'parlen' parameter.
 */
LOCAL int
yamaha_write_buffer(scgp, mode, bufferid, offset, parlen, buffer, buflen)
	SCSI	*scgp;
	int	mode;
	int	bufferid;
	long	offset;
	long	parlen;
	void	*buffer;
	long	buflen;
{
	register struct	scg_cmd	*scmd = scgp->scmd;
		Uchar	*CDB;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = buffer;
	scmd->size = buflen;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x3B;

	CDB    = (Uchar *)scmd->cdb.cmd_cdb;
	CDB[1] = mode & 7;
	CDB[2] = bufferid;
	i_to_3_byte(&CDB[3], offset);
	i_to_3_byte(&CDB[6], parlen);

	scgp->cmdname = "write_buffer";

	if (scg_cmd(scgp) >= 0)
		return (1);
	return (0);
}
