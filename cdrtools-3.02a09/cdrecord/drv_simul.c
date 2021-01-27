/* @(#)drv_simul.c	1.61 10/12/19 Copyright 1998-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)drv_simul.c	1.61 10/12/19 Copyright 1998-2010 J. Schilling";
#endif
/*
 *	Simulation device driver
 *
 *	Copyright (c) 1998-2010 J. Schilling
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
#include <schily/schily.h>
#include <schily/nlsdefs.h>

/*#include <scgio.h>*/
#include <scg/scsidefs.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include <schily/libport.h>

#include "cdrecord.h"

extern	int	silent;
extern	int	verbose;
extern	int	lverbose;

LOCAL	int	simul_load		__PR((SCSI *scgp, cdr_t *));
LOCAL	int	simul_unload		__PR((SCSI *scgp, cdr_t *));
LOCAL	cdr_t	*identify_simul		__PR((SCSI *scgp, cdr_t *, struct scsi_inquiry *));
LOCAL	int	init_simul		__PR((SCSI *scgp, cdr_t *dp));
LOCAL	int	getdisktype_simul	__PR((SCSI *scgp, cdr_t *dp));
LOCAL	int	speed_select_simul	__PR((SCSI *scgp, cdr_t *dp, int *speedp));
LOCAL	int	next_wr_addr_simul	__PR((SCSI *scgp, track_t *trackp, long *ap));
LOCAL	int	cdr_write_simul		__PR((SCSI *scgp, caddr_t bp, long sectaddr, long size, int blocks, BOOL islast));
LOCAL	int	open_track_simul	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	close_track_simul	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	open_session_simul	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	int	fixate_simul		__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
LOCAL	void	tv_sub			__PR((struct timeval *tvp1, struct timeval *tvp2));

LOCAL int
simul_load(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	return (0);
}

LOCAL int
simul_unload(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	return (0);
}

cdr_t	cdr_cdr_simul = {
	0, 0, 0,
	CDR_TAO|CDR_SAO|CDR_PACKET|CDR_RAW|CDR_RAW16|CDR_RAW96P|CDR_RAW96R|CDR_SRAW96P|CDR_SRAW96R|CDR_TRAYLOAD|CDR_SIMUL,
	0,
	CDR_CDRW_ALL,
	WM_SAO,
	40, 372,
	"cdr_simul",
	"simulation CD-R driver for timing/speed tests",
	0,
	(dstat_t *)0,
	identify_simul,
	drive_attach,
	init_simul,
	getdisktype_simul,
	no_diskstatus,
	simul_load,
	simul_unload,
	buf_dummy,
	cmd_dummy,					/* recovery_needed */
	(int(*)__PR((SCSI *, cdr_t *, int)))cmd_dummy,	/* recover	*/
	speed_select_simul,
	select_secsize,
	next_wr_addr_simul,
	(int(*)__PR((SCSI *, Ulong)))cmd_ill,	/* reserve_track	*/
	cdr_write_simul,
	(int(*)__PR((track_t *, void *, BOOL)))cmd_dummy,	/* gen_cue */
	(int(*)__PR((SCSI *scgp, cdr_t *, track_t *)))cmd_dummy, /* send_cue */
	(int(*)__PR((SCSI *, cdr_t *, track_t *)))cmd_dummy, /* leadin */
	open_track_simul,
	close_track_simul,
	open_session_simul,
	cmd_dummy,
	cmd_dummy,					/* abort	*/
	read_session_offset,
	fixate_simul,
	cmd_dummy,					/* stats	*/
	blank_dummy,
	format_dummy,
	(int(*)__PR((SCSI *, caddr_t, int, int)))NULL,	/* no OPC	*/
	cmd_dummy,					/* opt1		*/
	cmd_dummy,					/* opt2		*/
};

cdr_t	cdr_dvd_simul = {
	0, 0, 0,
	CDR_TAO|CDR_SAO|CDR_PACKET|CDR_RAW|CDR_RAW16|CDR_RAW96P|CDR_RAW96R|CDR_SRAW96P|CDR_SRAW96R|CDR_DVD|CDR_TRAYLOAD|CDR_SIMUL,
	CDR2_NOCD,
	CDR_CDRW_ALL,
	WM_SAO,
	2, 1000,
	"dvd_simul",
	"simulation DVD-R driver for timing/speed tests",
	0,
	(dstat_t *)0,
	identify_simul,
	drive_attach,
	init_simul,
	getdisktype_simul,
	no_diskstatus,
	simul_load,
	simul_unload,
	buf_dummy,
	cmd_dummy,					/* recovery_needed */
	(int(*)__PR((SCSI *, cdr_t *, int)))cmd_dummy,	/* recover	*/
	speed_select_simul,
	select_secsize,
	next_wr_addr_simul,
	(int(*)__PR((SCSI *, Ulong)))cmd_ill,	/* reserve_track	*/
	cdr_write_simul,
	(int(*)__PR((track_t *, void *, BOOL)))cmd_dummy,	/* gen_cue */
	(int(*)__PR((SCSI *scgp, cdr_t *, track_t *)))cmd_dummy, /* send_cue */
	(int(*)__PR((SCSI *, cdr_t *, track_t *)))cmd_dummy, /* leadin */
	open_track_simul,
	close_track_simul,
	open_session_simul,
	cmd_dummy,
	cmd_dummy,					/* abort	*/
	read_session_offset,
	fixate_simul,
	cmd_dummy,					/* stats	*/
	blank_dummy,
	format_dummy,
	(int(*)__PR((SCSI *, caddr_t, int, int)))NULL,	/* no OPC	*/
	cmd_dummy,					/* opt1		*/
	cmd_dummy,					/* opt2		*/
};

cdr_t	cdr_bd_simul = {
	0, 0, 0,
	CDR_TAO|CDR_SAO|CDR_PACKET|CDR_TRAYLOAD|CDR_SIMUL,
	CDR2_NOCD|CDR2_BD,
	CDR_CDRW_ALL,
	WM_SAO,
	2, 1000,
	"bd_simul",
	"simulation BD-R driver for timing/speed tests",
	0,
	(dstat_t *)0,
	identify_simul,
	drive_attach,
	init_simul,
	getdisktype_simul,
	no_diskstatus,
	simul_load,
	simul_unload,
	buf_dummy,
	cmd_dummy,					/* recovery_needed */
	(int(*)__PR((SCSI *, cdr_t *, int)))cmd_dummy,	/* recover	*/
	speed_select_simul,
	select_secsize,
	next_wr_addr_simul,
	(int(*)__PR((SCSI *, Ulong)))cmd_ill,	/* reserve_track	*/
	cdr_write_simul,
	(int(*)__PR((track_t *, void *, BOOL)))cmd_dummy,	/* gen_cue */
	(int(*)__PR((SCSI *scgp, cdr_t *, track_t *)))cmd_dummy, /* send_cue */
	(int(*)__PR((SCSI *, cdr_t *, track_t *)))cmd_dummy, /* leadin */
	open_track_simul,
	close_track_simul,
	open_session_simul,
	cmd_dummy,
	cmd_dummy,					/* abort	*/
	read_session_offset,
	fixate_simul,
	cmd_dummy,					/* stats	*/
	blank_dummy,
	format_dummy,
	(int(*)__PR((SCSI *, caddr_t, int, int)))NULL,	/* no OPC	*/
	cmd_dummy,					/* opt1		*/
	cmd_dummy,					/* opt2		*/
};

LOCAL cdr_t *
identify_simul(scgp, dp, ip)
	SCSI			*scgp;
	cdr_t			*dp;
	struct scsi_inquiry	*ip;
{
	return (dp);
}

LOCAL	long	simul_nwa;
LOCAL	int	simul_speed = 1;
LOCAL	int	simul_dummy;
LOCAL	int	simul_isdvd;
LOCAL	int	simul_isbd;
LOCAL	int	simul_bufsize = 1024;
LOCAL	Uint	sleep_rest;
LOCAL	Uint	sleep_max;
LOCAL	Uint	sleep_min;

LOCAL int
init_simul(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	return (speed_select_simul(scgp, dp, NULL));
}

LOCAL int
getdisktype_simul(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	dstat_t	*dsp = dp->cdr_dstat;

	if (strcmp(dp->cdr_drname, cdr_cdr_simul.cdr_drname) == 0) {
		dsp->ds_maxblocks = 333000;
		simul_isdvd = FALSE;
	} else if (strcmp(dp->cdr_drname, cdr_dvd_simul.cdr_drname) == 0) {
		dsp->ds_maxblocks = 2464153;	/* 4.7 GB  */
/*		dsp->ds_maxblocks = 1927896;*/	/* 3.95 GB */
		dsp->ds_flags |= DSF_NOCD|DSF_DVD;
		simul_isdvd = TRUE;
	} else {
		dsp->ds_maxblocks = 12219392;	/* 25 GB  */
		dsp->ds_flags |= DSF_NOCD|DSF_BD;
		simul_isbd = TRUE;
	}
	return (drive_getdisktype(scgp, dp));
}


LOCAL int
speed_select_simul(scgp, dp, speedp)
	SCSI	*scgp;
	cdr_t	*dp;
	int	*speedp;
{
	long	val;
	char	*p;
	BOOL	dummy = (dp->cdr_cmdflags & F_DUMMY) != 0;

	if (speedp)
		simul_speed = *speedp;
	simul_dummy = dummy;

	if ((p = getenv("CDR_SIMUL_BUFSIZE")) != NULL) {
		if (getnum(p, &val) == 1)
			simul_bufsize = val / 1024;
	}

	/*
	 * sleep_max is the time to empty the drive's buffer in µs.
	 * sector size is from 2048 bytes to 2352 bytes.
	 * If sector size is 2048 bytes, 1k takes 6.666 ms.
	 * If sector size is 2352 bytes, 1k takes 5.805 ms.
	 * We take the 6 ms as an average between both values.
	 * simul_bufsize is the number of kilobytes in drive buffer.
	 */
	sleep_max = 6 * 1000 * simul_bufsize / simul_speed;

	/*
	 * DVD single speed is 1385 * 1000 Bytes/s (676.27 sectors/s)
	 */
	if ((dp->cdr_flags & CDR_DVD) != 0)
		sleep_max = 739 * simul_bufsize / simul_speed;

	/*
	 * BD single speed is 4495.5 * 1000 Bytes/s (2195.07 sectors/s)
	 */
	if ((dp->cdr_flags2 & CDR2_BD) != 0)
		sleep_max = 228 * simul_bufsize / simul_speed;

	if (lverbose) {
		printf(_("Simulation drive buffer size: %d KB\n"), simul_bufsize);
		printf(_("Maximum reserve time in drive buffer: %d.%3.3d ms for speed %dx\n"),
					sleep_max / 1000,
					sleep_max % 1000,
					simul_speed);
	}
	return (0);
}

LOCAL int
next_wr_addr_simul(scgp, trackp, ap)
	SCSI	*scgp;
	track_t	*trackp;
	long	*ap;
{
	/*
	 * This will most likely not 100% correct for TAO CDs
	 * but it is better than returning 0 in all cases.
	 */
	if (ap)
		*ap = simul_nwa;
	return (0);
}

LOCAL int
cdr_write_simul(scgp, bp, sectaddr, size, blocks, islast)
	SCSI	*scgp;
	caddr_t	bp;		/* address of buffer */
	long	sectaddr;	/* disk address (sector) to put */
	long	size;		/* number of bytes to transfer */
	int	blocks;		/* sector count */
	BOOL	islast;		/* last write for track */
{
	Uint	sleep_time;
	Uint	sleep_diff;

	struct timeval	tv1;
static	struct timeval	tv2;

	if (lverbose > 1 && islast)
		printf(_("\nWriting last record for this track.\n"));

	simul_nwa += blocks;

	gettimeofday(&tv1, (struct timezone *)0);
	if (tv2.tv_sec != 0) {		/* Already did gettimeofday(&tv2) */
		tv_sub(&tv1, &tv2);
		if (sleep_rest != 0) {
			sleep_diff = tv1.tv_sec * 1000000 + tv1.tv_usec;

			if (sleep_min > (sleep_rest - sleep_diff))
				sleep_min = (sleep_rest - sleep_diff);

			if (sleep_diff > sleep_rest) {
				printf(_("Buffer underrun: actual delay was %d.%3.3d ms, max delay was %d.%3.3d ms.\n"),
						sleep_diff / 1000,
						sleep_diff % 1000,
						sleep_rest / 1000,
						sleep_rest % 1000);
				if (!simul_dummy)
					return (-1);
			}
			/*
			 * If we spent time outside the write function
			 * subtract this time.
			 */
			sleep_diff = tv1.tv_sec * 1000000 + tv1.tv_usec;
			if (sleep_rest >= sleep_diff)
				sleep_rest -= sleep_diff;
			else
				sleep_rest = 0;
		}
	}
	/*
	 * Speed 1 ist 150 Sektoren/s
	 * Bei DVD 676.27 Sektoren/s
	 * Bei BD 2195.07 Sektoren/s
	 */
	sleep_time = 1000000 * blocks / 75 / simul_speed;
	if (simul_isdvd)
		sleep_time = 1000000 * blocks / 676 / simul_speed;
	if (simul_isbd)
		sleep_time = 1000000 * blocks / 2195 / simul_speed;

	sleep_time += sleep_rest;

	if (sleep_time > sleep_max) {
		int	mod;
		long	rsleep;

		sleep_rest = sleep_max;
		sleep_time -= sleep_rest;
		mod = sleep_time % 20000;
		sleep_rest += mod;
		sleep_time -= mod;
		if (sleep_time > 0) {
			gettimeofday(&tv1, (struct timezone *)0);
			usleep(sleep_time);
			gettimeofday(&tv2, (struct timezone *)0);
			tv2.tv_sec -= tv1.tv_sec;
			tv2.tv_usec -= tv1.tv_usec;
			rsleep = tv2.tv_sec * 1000000 + tv2.tv_usec;
			sleep_rest -= rsleep - sleep_time;
		}
	} else {
		sleep_rest = sleep_time;
	}

	gettimeofday(&tv2, (struct timezone *)0);
	return (size);
}

LOCAL int
open_track_simul(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t *trackp;
{
	sleep_min = 999 * 1000000;
	return (0);
}

LOCAL int
close_track_simul(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	if (lverbose) {
		printf(_("Remaining reserve time in drive buffer: %d.%3.3d ms\n"),
					sleep_rest / 1000,
					sleep_rest % 1000);
		printf(_("Minimum reserve time in drive buffer: %d.%3.3d ms\n"),
					sleep_min / 1000,
					sleep_min % 1000);
	}
	usleep(sleep_rest);
	sleep_rest = 0;
	return (0);
}

LOCAL int
open_session_simul(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	simul_nwa = 0L;
	return (0);
}

LOCAL int
fixate_simul(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	return (0);
}

LOCAL void
tv_sub(tvp1, tvp2)
	struct timeval	*tvp1;
	struct timeval	*tvp2;
{
	tvp1->tv_sec -= tvp2->tv_sec;
	tvp1->tv_usec -= tvp2->tv_usec;

	while (tvp1->tv_usec < 0) {
		tvp1->tv_usec += 1000000;
		tvp1->tv_sec -= 1;
	}
}
