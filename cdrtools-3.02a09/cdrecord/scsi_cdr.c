/* @(#)scsi_cdr.c	1.160 12/03/16 Copyright 1995-2012 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)scsi_cdr.c	1.160 12/03/16 Copyright 1995-2012 J. Schilling";
#endif
/*
 *	SCSI command functions for cdrecord
 *	covering pre-MMC standard functions up to MMC-2
 *
 *	Copyright (c) 1995-2012 J. Schilling
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
 * NOTICE:	The Philips CDD 521 has several firmware bugs.
 *		One of them is not to respond to a SCSI selection
 *		within 200ms if the general load on the
 *		SCSI bus is high. To deal with this problem
 *		most of the SCSI commands are send with the
 *		SCG_CMD_RETRY flag enabled.
 */
#include <schily/mconfig.h>

#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
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

#include "scsimmc.h"
#include "cdrecord.h"

#define	strbeg(s1, s2)	(strstr((s2), (s1)) == (s2))

EXPORT	BOOL	unit_ready	__PR((SCSI *scgp));
EXPORT	BOOL	wait_unit_ready	__PR((SCSI *scgp, int secs));
EXPORT	BOOL	scsi_in_progress __PR((SCSI *scgp));
EXPORT	BOOL	cdr_underrun	__PR((SCSI *scgp));
EXPORT	int	test_unit_ready	__PR((SCSI *scgp));
EXPORT	int	rezero_unit	__PR((SCSI *scgp));
EXPORT	int	request_sense	__PR((SCSI *scgp));
EXPORT	int	request_sense_b	__PR((SCSI *scgp, caddr_t bp, int cnt));
EXPORT	int	inquiry		__PR((SCSI *scgp, caddr_t, int));
EXPORT	int	read_capacity	__PR((SCSI *scgp));
EXPORT	void	print_capacity	__PR((SCSI *scgp, FILE *f));
EXPORT	int	scsi_load_unload __PR((SCSI *scgp, int));
EXPORT	int	scsi_prevent_removal __PR((SCSI *scgp, int));
EXPORT	int	scsi_start_stop_unit __PR((SCSI *scgp, int, int, BOOL immed));
EXPORT	int	scsi_set_speed	__PR((SCSI *scgp, int readspeed, int writespeed, int rotctl));
EXPORT	int	scsi_get_speed	__PR((SCSI *scgp, int *readspeedp, int *writespeedp));
EXPORT	int	qic02		__PR((SCSI *scgp, int));
EXPORT	int	write_xscsi	__PR((SCSI *scgp, caddr_t, long, long, int));
EXPORT	int	write_xg0	__PR((SCSI *scgp, caddr_t, long, long, int));
EXPORT	int	write_xg1	__PR((SCSI *scgp, caddr_t, long, long, int));
EXPORT	int	write_xg5	__PR((SCSI *scgp, caddr_t, long, long, int));
EXPORT	int	seek_scsi	__PR((SCSI *scgp, long addr));
EXPORT	int	seek_g0		__PR((SCSI *scgp, long addr));
EXPORT	int	seek_g1		__PR((SCSI *scgp, long addr));
EXPORT	int	scsi_flush_cache __PR((SCSI *scgp, BOOL immed));
EXPORT	int	read_buffer	__PR((SCSI *scgp, caddr_t bp, int cnt, int mode));
EXPORT	int	write_buffer	__PR((SCSI *scgp, char *buffer, long length, int mode, int bufferid, long offset));
EXPORT	int	read_subchannel	__PR((SCSI *scgp, caddr_t bp, int track,
					int cnt, int msf, int subq, int fmt));
EXPORT	int	read_toc	__PR((SCSI *scgp, caddr_t, int, int, int, int));
EXPORT	int	read_toc_philips __PR((SCSI *scgp, caddr_t, int, int, int, int));
EXPORT	int	read_header	__PR((SCSI *scgp, caddr_t, long, int, int));
EXPORT	int	read_disk_info	__PR((SCSI *scgp, caddr_t, int));
EXPORT	int	read_track_info	__PR((SCSI *scgp, caddr_t, int type, int addr, int cnt));
EXPORT	int	get_trackinfo	__PR((SCSI *scgp, caddr_t, int type, int addr, int cnt));
EXPORT	int	read_rzone_info	__PR((SCSI *scgp, caddr_t bp, int cnt));
EXPORT	int	reserve_tr_rzone __PR((SCSI *scgp, long size));
EXPORT	int	read_dvd_structure __PR((SCSI *scgp, caddr_t bp, int cnt, int mt, int addr, int layer, int fmt));
EXPORT	int	send_dvd_structure __PR((SCSI *scgp, caddr_t bp, int cnt, int fmt));
EXPORT	int	send_opc	__PR((SCSI *scgp, caddr_t, int cnt, int doopc));
EXPORT	int	read_track_info_philips	__PR((SCSI *scgp, caddr_t, int, int));
EXPORT	int	scsi_close_tr_session __PR((SCSI *scgp, int type, int track, BOOL immed));
EXPORT	int	read_master_cue	__PR((SCSI *scgp, caddr_t bp, int sheet, int cnt));
EXPORT	int	send_cue_sheet	__PR((SCSI *scgp, caddr_t bp, long size));
EXPORT	int	read_buff_cap	__PR((SCSI *scgp, long *, long *));
EXPORT	int	scsi_blank	__PR((SCSI *scgp, long addr, int blanktype, BOOL immed));
EXPORT	BOOL	allow_atapi	__PR((SCSI *scgp, BOOL new));
EXPORT	int	mode_select	__PR((SCSI *scgp, Uchar *, int, int, int));
EXPORT	int	mode_sense	__PR((SCSI *scgp, Uchar *dp, int cnt, int page, int pcf));
EXPORT	int	mode_select_sg0	__PR((SCSI *scgp, Uchar *, int, int, int));
EXPORT	int	mode_sense_sg0	__PR((SCSI *scgp, Uchar *dp, int cnt, int page, int pcf));
EXPORT	int	mode_select_g0	__PR((SCSI *scgp, Uchar *, int, int, int));
EXPORT	int	mode_select_g1	__PR((SCSI *scgp, Uchar *, int, int, int));
EXPORT	int	mode_sense_g0	__PR((SCSI *scgp, Uchar *dp, int cnt, int page, int pcf));
EXPORT	int	mode_sense_g1	__PR((SCSI *scgp, Uchar *dp, int cnt, int page, int pcf));
EXPORT	int	read_tochdr	__PR((SCSI *scgp, cdr_t *, int *, int *));
EXPORT	int	read_cdtext	__PR((SCSI *scgp));
EXPORT	int	read_trackinfo	__PR((SCSI *scgp, int, long *, struct msf *, int *, int *, int *));
EXPORT	int	read_B0		__PR((SCSI *scgp, BOOL isbcd, long *b0p, long *lop));
EXPORT	int	read_session_offset __PR((SCSI *scgp, long *));
EXPORT	int	read_session_offset_philips __PR((SCSI *scgp, long *));
EXPORT	int	sense_secsize	__PR((SCSI *scgp, int current));
EXPORT	int	select_secsize	__PR((SCSI *scgp, int));
EXPORT	BOOL	is_cddrive	__PR((SCSI *scgp));
EXPORT	BOOL	is_unknown_dev	__PR((SCSI *scgp));
EXPORT	int	read_scsi	__PR((SCSI *scgp, caddr_t, long, int));
EXPORT	int	read_g0		__PR((SCSI *scgp, caddr_t, long, int));
EXPORT	int	read_g1		__PR((SCSI *scgp, caddr_t, long, int));
EXPORT	BOOL	getdev		__PR((SCSI *scgp, BOOL));
EXPORT	void	printinq	__PR((SCSI *scgp, FILE *f));
EXPORT	void	printdev	__PR((SCSI *scgp));
EXPORT	BOOL	do_inquiry	__PR((SCSI *scgp, BOOL));
EXPORT	BOOL	recovery_needed	__PR((SCSI *scgp, cdr_t *));
EXPORT	int	scsi_load	__PR((SCSI *scgp, cdr_t *));
EXPORT	int	scsi_unload	__PR((SCSI *scgp, cdr_t *));
EXPORT	int	scsi_cdr_write	__PR((SCSI *scgp, caddr_t bp, long sectaddr, long size, int blocks, BOOL islast));
EXPORT	struct cd_mode_page_2A * mmc_cap __PR((SCSI *scgp, Uchar *modep));
EXPORT	void	mmc_getval	__PR((struct cd_mode_page_2A *mp,
					BOOL *cdrrp, BOOL *cdwrp,
					BOOL *cdrrwp, BOOL *cdwrwp,
					BOOL *dvdp, BOOL *dvdwp));
EXPORT	BOOL	is_mmc		__PR((SCSI *scgp, BOOL *cdwp, BOOL *dvdwp));
EXPORT	BOOL	mmc_check	__PR((SCSI *scgp, BOOL *cdrrp, BOOL *cdwrp,
					BOOL *cdrrwp, BOOL *cdwrwp,
					BOOL *dvdp, BOOL *dvdwp));
LOCAL	void	print_speed	__PR((char *fmt, int val));
EXPORT	void	print_capabilities __PR((SCSI *scgp));
extern	int	verify		__PR((SCSI *scgp, long start, int count, long *bad_block));

EXPORT BOOL
unit_ready(scgp)
	SCSI	*scgp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (test_unit_ready(scgp) >= 0)		/* alles OK */
		return (TRUE);
	else if (scmd->error >= SCG_FATAL)	/* nicht selektierbar */
		return (FALSE);

	if (scg_sense_key(scgp) == SC_UNIT_ATTENTION) {
		if (test_unit_ready(scgp) >= 0)	/* alles OK */
			return (TRUE);
	}
	if ((scg_cmd_status(scgp) & ST_BUSY) != 0) {
		/*
		 * Busy/reservation_conflict
		 */
		usleep(500000);
		if (test_unit_ready(scgp) >= 0)	/* alles OK */
			return (TRUE);
	}
	if (scg_sense_key(scgp) == -1) {	/* non extended Sense */
		if (scg_sense_code(scgp) == 4)	/* NOT_READY */
			return (FALSE);
		return (TRUE);
	}
						/* FALSE wenn NOT_READY */
	return (scg_sense_key(scgp) != SC_NOT_READY);
}

EXPORT BOOL
wait_unit_ready(scgp, secs)
	SCSI	*scgp;
	int	secs;
{
	int	i;
	int	c;
	int	k;
	int	ret;
	int	err;

	seterrno(0);
	scgp->silent++;
	ret = test_unit_ready(scgp);		/* eat up unit attention */
	if (ret < 0) {
		err = geterrno();

		if (err == EPERM || err == EACCES) {
			scgp->silent--;
			return (FALSE);
		}
		ret = test_unit_ready(scgp);	/* got power on condition? */
	}
	scgp->silent--;

	if (ret >= 0)				/* success that's enough */
		return (TRUE);

	scgp->silent++;
	for (i = 0; i < secs && (ret = test_unit_ready(scgp)) < 0; i++) {
		if (scgp->scmd->scb.busy != 0) {
			sleep(1);
			continue;
		}
		c = scg_sense_code(scgp);
		k = scg_sense_key(scgp);
		/*
		 * Abort quickly if it does not make sense to wait.
		 * 0x30 == Cannot read medium
		 * 0x3A == Medium not present
		 */
		if ((k == SC_NOT_READY && (c == 0x3A || c == 0x30)) ||
		    (k == SC_MEDIUM_ERROR)) {
			if (scgp->silent <= 1)
				scg_printerr(scgp);
			scgp->silent--;
			return (FALSE);
		}
		sleep(1);
	}
	scgp->silent--;
	if (ret < 0)
		return (FALSE);
	return (TRUE);
}

EXPORT BOOL
scsi_in_progress(scgp)
	SCSI	*scgp;
{
	if (scg_sense_key(scgp) == SC_NOT_READY &&
		/*
		 * Logigal unit not ready operation/long_write in progress
		 */
	    scg_sense_code(scgp) == 0x04 &&
	    (scg_sense_qual(scgp) == 0x04 || /* CyberDr. "format in progress"*/
	    scg_sense_qual(scgp) == 0x07 || /* "operation in progress"	    */
	    scg_sense_qual(scgp) == 0x08)) { /* "long write in progress"    */
		return (TRUE);
	} else {
		if (scgp->silent <= 1)
			scg_printerr(scgp);
	}
	return (FALSE);
}

EXPORT BOOL
cdr_underrun(scgp)
	SCSI	*scgp;
{
	if ((scg_sense_key(scgp) != SC_ILLEGAL_REQUEST &&
	    scg_sense_key(scgp) != SC_MEDIUM_ERROR))
		return (FALSE);

	if ((scg_sense_code(scgp) == 0x21 &&
	    (scg_sense_qual(scgp) == 0x00 ||	/* logical block address out of range */
	    scg_sense_qual(scgp) == 0x02)) ||	/* invalid address for write */

	    (scg_sense_code(scgp) == 0x0C &&
	    scg_sense_qual(scgp) == 0x09)) {	/* write error - loss of streaming */
		return (TRUE);
	}
	/*
	 * XXX Bei manchen Brennern kommt mach dem der Brennvorgang bereits
	 * XXX eine Weile gelaufen ist ein 5/24/0 Invalid field in CDB.
	 * XXX Daher sollte man testen ob schon geschrieben wurde...
	 */
	return (FALSE);
}

EXPORT int
test_unit_ready(scgp)
	SCSI	*scgp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)0;
	scmd->size = 0;
	scmd->flags = SCG_DISRE_ENA | (scgp->silent ? SCG_SILENT:0);
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = SC_TEST_UNIT_READY;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);

	scgp->cmdname = "test unit ready";

	return (scg_cmd(scgp));
}

EXPORT int
rezero_unit(scgp)
	SCSI	*scgp;
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

	scgp->cmdname = "rezero unit";

	return (scg_cmd(scgp));
}

EXPORT int
request_sense(scgp)
	SCSI	*scgp;
{
		char	sensebuf[CCS_SENSE_LEN];
		char	*cmdsave;

	cmdsave = scgp->cmdname;

	if (request_sense_b(scgp, sensebuf, sizeof (sensebuf)) < 0)
		return (-1);
	scgp->cmdname = cmdsave;
	scg_prsense((Uchar *)sensebuf, CCS_SENSE_LEN - scg_getresid(scgp));
	return (0);
}

EXPORT int
request_sense_b(scgp, bp, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;


	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = SC_REQUEST_SENSE;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	scmd->cdb.g0_cdb.count = cnt;

	scgp->cmdname = "request_sense";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int
inquiry(scgp, bp, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes(bp, cnt, '\0');
	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = SC_INQUIRY;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	scmd->cdb.g0_cdb.count = cnt;

	scgp->cmdname = "inquiry";

	if (scg_cmd(scgp) < 0)
		return (-1);
	if (scgp->verbose)
		scg_prbytes(_("Inquiry Data   :"), (Uchar *)bp, cnt - scg_getresid(scgp));
	return (0);
}

EXPORT int
read_capacity(scgp)
	SCSI	*scgp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)scgp->cap;
	scmd->size = sizeof (struct scsi_capacity);
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x25;	/* Read Capacity */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdblen(&scmd->cdb.g1_cdb, 0); /* Full Media */

	scgp->cmdname = "read capacity";

	if (scg_cmd(scgp) < 0) {
		return (-1);
	} else {
		long	cbsize;
		long	cbaddr;

		/*
		 * c_bsize & c_baddr are signed Int32_t
		 * so we use signed int conversion here.
		 */
		cbsize = a_to_4_byte(&scgp->cap->c_bsize);
		cbaddr = a_to_4_byte(&scgp->cap->c_baddr);
		scgp->cap->c_bsize = cbsize;
		scgp->cap->c_baddr = cbaddr;
	}
	return (0);
}

EXPORT void
print_capacity(scgp, f)
	SCSI	*scgp;
	FILE	*f;
{
	long	kb;
	long	mb;
	long	prmb;
	double	dkb;

	dkb =  (scgp->cap->c_baddr+1.0) * (scgp->cap->c_bsize/1024.0);
	kb = dkb;
	mb = dkb / 1024.0;
	prmb = dkb / 1000.0 * 1.024;
	fprintf(f, _("Capacity: %ld Blocks = %ld kBytes = %ld MBytes = %ld prMB\n"),
		(long)scgp->cap->c_baddr+1, kb, mb, prmb);
	fprintf(f, _("Sectorsize: %ld Bytes\n"), (long)scgp->cap->c_bsize);
}

EXPORT int
scsi_load_unload(scgp, load)
	SCSI	*scgp;
	int	load;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xA6;
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	scmd->cdb.g5_cdb.addr[1] = load?3:2;
	scmd->cdb.g5_cdb.count[2] = 0; /* slot # */

	scgp->cmdname = "medium load/unload";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int
scsi_prevent_removal(scgp, prevent)
	SCSI	*scgp;
	int	prevent;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = 0x1E;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	scmd->cdb.g0_cdb.count = prevent & 1;

	scgp->cmdname = "prevent/allow medium removal";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}


EXPORT int
scsi_start_stop_unit(scgp, flg, loej, immed)
	SCSI	*scgp;
	int	flg;
	int	loej;
	BOOL	immed;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = 0x1B;	/* Start Stop Unit */
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	scmd->cdb.g0_cdb.count = (flg ? 1:0) | (loej ? 2:0);

	if (immed)
		scmd->cdb.cmd_cdb[1] |= 0x01;

	scgp->cmdname = "start/stop unit";

	return (scg_cmd(scgp));
}

EXPORT int
scsi_set_speed(scgp, readspeed, writespeed, rotctl)
	SCSI	*scgp;
	int	readspeed;
	int	writespeed;
	int	rotctl;
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

	scmd->cdb.cmd_cdb[1] |= rotctl & 0x03;

	scgp->cmdname = "set cd speed";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int
scsi_get_speed(scgp, readspeedp, writespeedp)
	SCSI	*scgp;
	int	*readspeedp;
	int	*writespeedp;
{
	struct	cd_mode_page_2A *mp;
	Uchar	m[256];
	int	val;

	scgp->silent++;
	mp = mmc_cap(scgp, m); /* Get MMC capabilities in allocated mp */
	scgp->silent--;
	if (mp == NULL)
		return (-1);	/* Pre SCSI-3/mmc drive		*/

	val = a_to_u_2_byte(mp->cur_read_speed);
	if (readspeedp)
		*readspeedp = val;

	if (mp->p_len >= 28)
		val = a_to_u_2_byte(mp->v3_cur_write_speed);
	else
		val = a_to_u_2_byte(mp->cur_write_speed);
	if (writespeedp)
		*writespeedp = val;

	return (0);
}


EXPORT int
qic02(scgp, cmd)
	SCSI	*scgp;
	int	cmd;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)0;
	scmd->size = 0;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = DEF_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = 0x0D;	/* qic02 Sysgen SC4000 */
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	scmd->cdb.g0_cdb.mid_addr = cmd;

	scgp->cmdname = "qic 02";
	return (scg_cmd(scgp));
}

#define	G0_MAXADDR	0x1FFFFFL

EXPORT int
write_xscsi(scgp, bp, addr, size, cnt)
	SCSI	*scgp;
	caddr_t	bp;		/* address of buffer */
	long	addr;		/* disk address (sector) to put */
	long	size;		/* number of bytes to transfer */
	int	cnt;		/* sectorcount */
{
	if (addr <= G0_MAXADDR)
		return (write_xg0(scgp, bp, addr, size, cnt));
	else
		return (write_xg1(scgp, bp, addr, size, cnt));
}

EXPORT int
write_xg0(scgp, bp, addr, size, cnt)
	SCSI	*scgp;
	caddr_t	bp;		/* address of buffer */
	long	addr;		/* disk address (sector) to put */
	long	size;		/* number of bytes to transfer */
	int	cnt;		/* sectorcount */
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = size;
	scmd->flags = SCG_DISRE_ENA|SCG_CMD_RETRY;
/*	scmd->flags = SCG_DISRE_ENA;*/
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = SC_WRITE;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	g0_cdbaddr(&scmd->cdb.g0_cdb, addr);
	scmd->cdb.g0_cdb.count = cnt;

	scgp->cmdname = "write_g0";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (size - scg_getresid(scgp));
}

EXPORT int
write_xg1(scgp, bp, addr, size, cnt)
	SCSI	*scgp;
	caddr_t	bp;		/* address of buffer */
	long	addr;		/* disk address (sector) to put */
	long	size;		/* number of bytes to transfer */
	int	cnt;		/* sectorcount */
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = size;
	scmd->flags = SCG_DISRE_ENA|SCG_CMD_RETRY;
/*	scmd->flags = SCG_DISRE_ENA;*/
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = SC_EWRITE;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdbaddr(&scmd->cdb.g1_cdb, addr);
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "write_g1";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (size - scg_getresid(scgp));
}

EXPORT int
write_xg5(scgp, bp, addr, size, cnt)
	SCSI	*scgp;
	caddr_t	bp;		/* address of buffer */
	long	addr;		/* disk address (sector) to put */
	long	size;		/* number of bytes to transfer */
	int	cnt;		/* sectorcount */
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = size;
	scmd->flags = SCG_DISRE_ENA|SCG_CMD_RETRY;
/*	scmd->flags = SCG_DISRE_ENA;*/
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g5_cdb.cmd = 0xAA;
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	g5_cdbaddr(&scmd->cdb.g5_cdb, addr);
	g5_cdblen(&scmd->cdb.g5_cdb, cnt);

	scgp->cmdname = "write_g5";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (size - scg_getresid(scgp));
}

EXPORT int
seek_scsi(scgp, addr)
	SCSI	*scgp;
	long	addr;
{
	if (addr <= G0_MAXADDR)
		return (seek_g0(scgp, addr));
	else
		return (seek_g1(scgp, addr));
}

EXPORT int
seek_g0(scgp, addr)
	SCSI	*scgp;
	long	addr;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = 0x0B;	/* Seek */
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	g0_cdbaddr(&scmd->cdb.g0_cdb, addr);

	scgp->cmdname = "seek_g0";

	return (scg_cmd(scgp));
}

EXPORT int
seek_g1(scgp, addr)
	SCSI	*scgp;
	long	addr;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x2B;	/* Seek G1 */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdbaddr(&scmd->cdb.g1_cdb, addr);

	scgp->cmdname = "seek_g1";

	return (scg_cmd(scgp));
}

EXPORT int
scsi_flush_cache(scgp, immed)
	SCSI	*scgp;
	BOOL	immed;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->timeout = 2 * 60;		/* Max: sizeof (CDR-cache)/150KB/s */
	scmd->cdb.g1_cdb.cmd = 0x35;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);

	if (immed)
		scmd->cdb.cmd_cdb[1] |= 0x02;

	scgp->cmdname = "flush cache";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int
read_buffer(scgp, bp, cnt, mode)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
	int	mode;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->dma_read = 1;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x3C;	/* Read Buffer */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.cmd_cdb[1] |= (mode & 7);
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "read buffer";

	return (scg_cmd(scgp));
}

EXPORT int
write_buffer(scgp, buffer, length, mode, bufferid, offset)
	SCSI	*scgp;
	char	*buffer;
	long	length;
	int	mode;
	int	bufferid;
	long	offset;
{
	register struct	scg_cmd	*scmd = scgp->scmd;
	char			*cdb;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = buffer;
	scmd->size = length;
	scmd->flags = SCG_DISRE_ENA|SCG_CMD_RETRY;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;

	cdb = (char *)scmd->cdb.cmd_cdb;

	cdb[0] = 0x3B;
	cdb[1] = mode & 7;
	cdb[2] = bufferid;
	cdb[3] = offset >> 16;
	cdb[4] = (offset >> 8) & 0xff;
	cdb[5] = offset & 0xff;
	cdb[6] = length >> 16;
	cdb[7] = (length >> 8) & 0xff;
	cdb[8] = length & 0xff;

	scgp->cmdname = "write_buffer";

	if (scg_cmd(scgp) >= 0)
		return (1);
	return (0);
}

EXPORT int
read_subchannel(scgp, bp, track, cnt, msf, subq, fmt)
	SCSI	*scgp;
	caddr_t	bp;
	int	track;
	int	cnt;
	int	msf;
	int	subq;
	int	fmt;

{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x42;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	if (msf)
		scmd->cdb.g1_cdb.res = 1;
	if (subq)
		scmd->cdb.g1_cdb.addr[0] = 0x40;
	scmd->cdb.g1_cdb.addr[1] = fmt;
	scmd->cdb.g1_cdb.res6 = track;
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "read subchannel";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int
read_toc(scgp, bp, track, cnt, msf, fmt)
	SCSI	*scgp;
	caddr_t	bp;
	int	track;
	int	cnt;
	int	msf;
	int	fmt;

{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x43;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	if (msf)
		scmd->cdb.g1_cdb.res = 1;
	scmd->cdb.g1_cdb.addr[0] = fmt & 0x0F;
	scmd->cdb.g1_cdb.res6 = track;
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "read toc";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int
read_toc_philips(scgp, bp, track, cnt, msf, fmt)
	SCSI	*scgp;
	caddr_t	bp;
	int	track;
	int	cnt;
	int	msf;
	int	fmt;

{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->timeout = 4 * 60;		/* May last  174s on a TEAC CD-R55S */
	scmd->cdb.g1_cdb.cmd = 0x43;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	if (msf)
		scmd->cdb.g1_cdb.res = 1;
	scmd->cdb.g1_cdb.res6 = track;
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	if (fmt & 1)
		scmd->cdb.g1_cdb.vu_96 = 1;
	if (fmt & 2)
		scmd->cdb.g1_cdb.vu_97 = 1;

	scgp->cmdname = "read toc";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int
read_header(scgp, bp, addr, cnt, msf)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
	int	msf;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x44;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	if (msf)
		scmd->cdb.g1_cdb.res = 1;
	g1_cdbaddr(&scmd->cdb.g1_cdb, addr);
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "read header";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int
read_disk_info(scgp, bp, cnt)
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
	scmd->timeout = 4 * 60;		/* Needs up to 2 minutes */
	scmd->cdb.g1_cdb.cmd = 0x51;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "read disk info";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int
read_track_info(scgp, bp, type, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	int	type;
	int	addr;
	int	cnt;

{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->timeout = 4 * 60;		/* Needs up to 2 minutes */
	scmd->cdb.g1_cdb.cmd = 0x52;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
/*	scmd->cdb.cmd_cdb[1] = type & 0x03;*/
	scmd->cdb.cmd_cdb[1] = type;
	g1_cdbaddr(&scmd->cdb.g1_cdb, addr);	/* LBA/Track/Session */
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "read track info";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int
get_trackinfo(scgp, bp, type, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	int	type;
	int	addr;
	int	cnt;
{
	int	len;
	int	ret;

	fillbytes(bp, cnt, '\0');

	/*
	 * Used to be 2 instead of 4 (now). But some Y2k ATAPI drives as used
	 * by IOMEGA create a DMA overrun if we try to transfer only 2 bytes.
	 */
	if (read_track_info(scgp, bp, type, addr, 4) < 0)
		return (-1);

	len = a_to_u_2_byte(bp);
	len += 2;
	if (len > cnt)
		len = cnt;
	ret = read_track_info(scgp, bp, type, addr, len);

#ifdef	DEBUG
	if (lverbose > 1)
		scg_prbytes(_("Track info:"), (Uchar *)bp,
				len-scg_getresid(scgp));
#endif
	return (ret);
}

EXPORT int
read_rzone_info(scgp, bp, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
{
	return (get_trackinfo(scgp, bp, TI_TYPE_LBA, 0, cnt));
}

EXPORT int
reserve_tr_rzone(scgp, size)
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

	i_to_4_byte(&scmd->cdb.cmd_cdb[5], size);

	scgp->cmdname = "reserve_track_rzone";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int
read_dvd_structure(scgp, bp, cnt, mt, addr, layer, fmt)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
	int	mt;
	int	addr;
	int	layer;
	int	fmt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->timeout = 4 * 60;		/* Needs up to 2 minutes ??? */
	scmd->cdb.g5_cdb.cmd = 0xAD;
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	scmd->cdb.cmd_cdb[1] |= (mt & 0x0F);	/* Media Type */
	g5_cdbaddr(&scmd->cdb.g5_cdb, addr);
	g5_cdblen(&scmd->cdb.g5_cdb, cnt);
	scmd->cdb.g5_cdb.count[0] = layer;
	scmd->cdb.g5_cdb.count[1] = fmt;

	scgp->cmdname = "read dvd structure";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int
send_dvd_structure(scgp, bp, cnt, fmt)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
	int	fmt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->timeout = 4 * 60;		/* Needs up to 2 minutes ??? */
	scmd->cdb.g5_cdb.cmd = 0xBF;
	scmd->cdb.g5_cdb.lun = scg_lun(scgp);
	g5_cdblen(&scmd->cdb.g5_cdb, cnt);
	scmd->cdb.g5_cdb.count[0] = 0;
	scmd->cdb.g5_cdb.count[1] = fmt;

	scgp->cmdname = "send dvd structure";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int
send_opc(scgp, bp, cnt, doopc)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
	int	doopc;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->timeout = 60;
	scmd->cdb.g1_cdb.cmd = 0x54;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.reladr = doopc?1:0;
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "send opc";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int
read_track_info_philips(scgp, bp, track, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	int	track;
	int	cnt;

{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0xE5;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdbaddr(&scmd->cdb.g1_cdb, track);
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "read track info";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int
scsi_close_tr_session(scgp, type, track, immed)
	SCSI	*scgp;
	int	type;
	int	track;
	BOOL	immed;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->timeout = 8 * 60;		/* Needs up to 4 minutes */
	scmd->cdb.g1_cdb.cmd = 0x5B;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.addr[0] = type;
	scmd->cdb.g1_cdb.addr[3] = track;

	if (immed)
		scmd->cdb.g1_cdb.reladr = 1;
/*		scmd->cdb.cmd_cdb[1] |= 0x01;*/
#ifdef	nono
	scmd->cdb.g1_cdb.reladr = 1;	/* IMM hack to test Mitsumi behaviour*/
#endif

	scgp->cmdname = "close track/session";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int
read_master_cue(scgp, bp, sheet, cnt)
	SCSI	*scgp;
	caddr_t	bp;		/* address of master cue sheet	*/
	int	sheet;		/* Sheet number			*/
	int	cnt;		/* Transfer count		*/
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x59;		/* Read master cue */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.addr[2] = sheet;
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "read master cue";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (0);
}

EXPORT int
send_cue_sheet(scgp, bp, size)
	SCSI	*scgp;
	caddr_t	bp;		/* address of cue sheet buffer */
	long	size;		/* number of bytes to transfer */
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = size;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x5D;	/* Send CUE sheet */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdblen(&scmd->cdb.g1_cdb, size);

	scgp->cmdname = "send_cue_sheet";

	if (scg_cmd(scgp) < 0)
		return (-1);
	return (size - scmd->resid);
}

EXPORT int
read_buff_cap(scgp, sp, fp)
	SCSI	*scgp;
	long	*sp;	/* Size pointer */
	long	*fp;	/* Free pointer */
{
	char	resp[12];
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
	scmd->cdb.g1_cdb.cmd = 0x5C;		/* Read buffer cap */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdblen(&scmd->cdb.g1_cdb, sizeof (resp));

	scgp->cmdname = "read buffer cap";

	if (scg_cmd(scgp) < 0)
		return (-1);

	bufsize   = a_to_u_4_byte(&resp[4]);
	freespace = a_to_u_4_byte(&resp[8]);
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

EXPORT int
scsi_blank(scgp, addr, blanktype, immed)
	SCSI	*scgp;
	long	addr;
	int	blanktype;
	BOOL	immed;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G5_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->timeout = 160 * 60; /* full blank at 1x could take 80 minutes */
	scmd->cdb.g5_cdb.cmd = 0xA1;	/* Blank */
	scmd->cdb.g0_cdb.high_addr = blanktype;
	g1_cdbaddr(&scmd->cdb.g5_cdb, addr);

	if (immed)
		scmd->cdb.g5_cdb.res |= 8;
/*		scmd->cdb.cmd_cdb[1] |= 0x10;*/

	scgp->cmdname = "blank unit";

	return (scg_cmd(scgp));
}

/*
 * XXX First try to handle ATAPI:
 * XXX ATAPI cannot handle SCSI 6 byte commands.
 * XXX We try to simulate 6 byte mode sense/select.
 */
LOCAL BOOL	is_atapi;

EXPORT BOOL
allow_atapi(scgp, new)
	SCSI	*scgp;
	BOOL	new;
{
	BOOL	old = is_atapi;
	Uchar	mode[256];

	if (new == old)
		return (old);

	scgp->silent++;
	/*
	 * If a bad drive has been reset before, we may need to fire up two
	 * test unit ready commands to clear status.
	 */
	(void) unit_ready(scgp);
	if (new &&
	    mode_sense_g1(scgp, mode, 8, 0x3F, 0) < 0) {	/* All pages current */
		new = FALSE;
	}
	scgp->silent--;

	is_atapi = new;
	return (old);
}

EXPORT int
mode_select(scgp, dp, cnt, smp, pf)
	SCSI	*scgp;
	Uchar	*dp;
	int	cnt;
	int	smp;
	int	pf;
{
	if (is_atapi)
		return (mode_select_sg0(scgp, dp, cnt, smp, pf));
	return (mode_select_g0(scgp, dp, cnt, smp, pf));
}

EXPORT int
mode_sense(scgp, dp, cnt, page, pcf)
	SCSI	*scgp;
	Uchar	*dp;
	int	cnt;
	int	page;
	int	pcf;
{
	if (is_atapi)
		return (mode_sense_sg0(scgp, dp, cnt, page, pcf));
	return (mode_sense_g0(scgp, dp, cnt, page, pcf));
}

/*
 * Simulate mode select g0 with mode select g1.
 */
EXPORT int
mode_select_sg0(scgp, dp, cnt, smp, pf)
	SCSI	*scgp;
	Uchar	*dp;
	int	cnt;
	int	smp;
	int	pf;
{
	Uchar	xmode[256+4];
	int	amt = cnt;

	if (amt < 1 || amt > 255) {
		/* XXX clear SCSI error codes ??? */
		return (-1);
	}

	if (amt < 4) {		/* Data length. medium type & VU */
		amt += 1;
	} else {
		amt += 4;
		movebytes(&dp[4], &xmode[8], cnt-4);
	}
	xmode[0] = 0;
	xmode[1] = 0;
	xmode[2] = dp[1];
	xmode[3] = dp[2];
	xmode[4] = 0;
	xmode[5] = 0;
	i_to_2_byte(&xmode[6], (unsigned int)dp[3]);

	if (scgp->verbose) scg_prbytes(_("Mode Parameters (un-converted)"), dp, cnt);

	return (mode_select_g1(scgp, xmode, amt, smp, pf));
}

/*
 * Simulate mode sense g0 with mode sense g1.
 */
EXPORT int
mode_sense_sg0(scgp, dp, cnt, page, pcf)
	SCSI	*scgp;
	Uchar	*dp;
	int	cnt;
	int	page;
	int	pcf;
{
	Uchar	xmode[256+4];
	int	amt = cnt;
	int	len;

	if (amt < 1 || amt > 255) {
		/* XXX clear SCSI error codes ??? */
		return (-1);
	}

	fillbytes((caddr_t)xmode, sizeof (xmode), '\0');
	if (amt < 4) {		/* Data length. medium type & VU */
		amt += 1;
	} else {
		amt += 4;
	}
	if (mode_sense_g1(scgp, xmode, amt, page, pcf) < 0)
		return (-1);

	amt = cnt - scg_getresid(scgp);
/*
 * For tests: Solaris 8 & LG CD-ROM always returns resid == amt
 */
/*	amt = cnt;*/
	if (amt > 4)
		movebytes(&xmode[8], &dp[4], amt-4);
	len = a_to_u_2_byte(xmode);
	if (len == 0) {
		dp[0] = 0;
	} else if (len < 6) {
		if (len > 2)
			len = 2;
		dp[0] = len;
	} else {
		dp[0] = len - 3;
	}
	dp[1] = xmode[2];
	dp[2] = xmode[3];
	len = a_to_u_2_byte(&xmode[6]);
	dp[3] = len;

	if (scgp->verbose) scg_prbytes(_("Mode Sense Data (converted)"), dp, amt);
	return (0);
}

EXPORT int
mode_select_g0(scgp, dp, cnt, smp, pf)
	SCSI	*scgp;
	Uchar	*dp;
	int	cnt;
	int	smp;
	int	pf;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)dp;
	scmd->size = cnt;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = SC_MODE_SELECT;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	scmd->cdb.g0_cdb.high_addr = smp ? 1 : 0 | pf ? 0x10 : 0;
	scmd->cdb.g0_cdb.count = cnt;

	if (scgp->verbose) {
		error("%s ", smp?_("Save"):_("Set "));
		scg_prbytes(_("Mode Parameters"), dp, cnt);
	}

	scgp->cmdname = "mode select g0";

	return (scg_cmd(scgp));
}

EXPORT int
mode_select_g1(scgp, dp, cnt, smp, pf)
	SCSI	*scgp;
	Uchar	*dp;
	int	cnt;
	int	smp;
	int	pf;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)dp;
	scmd->size = cnt;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x55;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g0_cdb.high_addr = smp ? 1 : 0 | pf ? 0x10 : 0;
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	if (scgp->verbose) {
		printf("%s ", smp?_("Save"):_("Set "));
		scg_prbytes(_("Mode Parameters"), dp, cnt);
	}

	scgp->cmdname = "mode select g1";

	return (scg_cmd(scgp));
}

EXPORT int
mode_sense_g0(scgp, dp, cnt, page, pcf)
	SCSI	*scgp;
	Uchar	*dp;
	int	cnt;
	int	page;
	int	pcf;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)dp;
	scmd->size = 0xFF;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = SC_MODE_SENSE;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
#ifdef	nonono
	scmd->cdb.g0_cdb.high_addr = 1<<4;	/* DBD Disable Block desc. */
#endif
	scmd->cdb.g0_cdb.mid_addr = (page&0x3F) | ((pcf<<6)&0xC0);
	scmd->cdb.g0_cdb.count = page ? 0xFF : 24;
	scmd->cdb.g0_cdb.count = cnt;

	scgp->cmdname = "mode sense g0";

	if (scg_cmd(scgp) < 0)
		return (-1);
	if (scgp->verbose) scg_prbytes(_("Mode Sense Data"), dp, cnt - scg_getresid(scgp));
	return (0);
}

EXPORT int
mode_sense_g1(scgp, dp, cnt, page, pcf)
	SCSI	*scgp;
	Uchar	*dp;
	int	cnt;
	int	page;
	int	pcf;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)dp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x5A;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
#ifdef	nonono
	scmd->cdb.g0_cdb.high_addr = 1<<4;	/* DBD Disable Block desc. */
#endif
	scmd->cdb.g1_cdb.addr[0] = (page&0x3F) | ((pcf<<6)&0xC0);
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "mode sense g1";

	if (scg_cmd(scgp) < 0)
		return (-1);
	if (scgp->verbose) scg_prbytes(_("Mode Sense Data"), dp, cnt - scg_getresid(scgp));
	return (0);
}

struct trackdesc {
	Uchar	res0;

#if defined(_BIT_FIELDS_LTOH)		/* Intel byteorder */
	Ucbit	control		: 4;
	Ucbit	adr		: 4;
#else					/* Motorola byteorder */
	Ucbit	adr		: 4;
	Ucbit	control		: 4;
#endif

	Uchar	track;
	Uchar	res3;
	Uchar	addr[4];
};

struct diskinfo {
	struct tocheader	hd;
	struct trackdesc	desc[1];
};

struct siheader {
	Uchar	len[2];
	Uchar	finished;
	Uchar	unfinished;
};

struct sidesc {
	Uchar	sess_number;
	Uchar	res1;
	Uchar	track;
	Uchar	res3;
	Uchar	addr[4];
};

struct sinfo {
	struct siheader	hd;
	struct sidesc	desc[1];
};

struct trackheader {
	Uchar	mode;
	Uchar	res[3];
	Uchar	addr[4];
};
#define	TRM_ZERO	0
#define	TRM_USER_ECC	1	/* 2048 bytes user data + 288 Bytes ECC/EDC */
#define	TRM_USER	2	/* All user data (2336 bytes) */


EXPORT	int
read_tochdr(scgp, dp, fp, lp)
	SCSI	*scgp;
	cdr_t	*dp;
	int	*fp;
	int	*lp;
{
	struct	tocheader *tp;
	char	xb[256];
	int	len;

	tp = (struct tocheader *)xb;

	fillbytes((caddr_t)xb, sizeof (xb), '\0');
	if (read_toc(scgp, xb, 0, sizeof (struct tocheader), 0, FMT_TOC) < 0) {
		if (scgp->silent == 0)
			errmsgno(EX_BAD, _("Cannot read TOC header\n"));
		return (-1);
	}
	len = a_to_u_2_byte(tp->len) + sizeof (struct tocheader)-2;
	if (len >= 4) {
		if (fp)
			*fp = tp->first;
		if (lp)
			*lp = tp->last;
		return (0);
	}
	return (-1);
}

EXPORT	int
read_cdtext(scgp)
	SCSI	*scgp;
{
	struct	tocheader *tp;
	char	xb[256];
	int	len;
	char	xxb[10000];

	tp = (struct tocheader *)xb;

	fillbytes((caddr_t)xb, sizeof (xb), '\0');
	if (read_toc(scgp, xb, 0, sizeof (struct tocheader), 0, FMT_CDTEXT) < 0) {
		if (scgp->silent == 0 || scgp->verbose > 0)
			errmsgno(EX_BAD, _("Cannot read CD-Text header\n"));
		return (-1);
	}
	len = a_to_u_2_byte(tp->len) + sizeof (struct tocheader)-2;
	printf(_("CD-Text len: %d\n"), len);

	if (read_toc(scgp, xxb, 0, len, 0, FMT_CDTEXT) < 0) {
		if (scgp->silent == 0)
			errmsgno(EX_BAD, _("Cannot read CD-Text\n"));
		return (-1);
	}
	{
		FILE	*f = fileopen("cdtext.dat", "wctb");
		filewrite(f, xxb, len);
	}
	return (0);
}

EXPORT	int
read_trackinfo(scgp, track, offp, msfp, adrp, controlp, modep)
	SCSI	*scgp;
	int	track;
	long	*offp;
	struct msf *msfp;
	int	*adrp;
	int	*controlp;
	int	*modep;
{
	struct	diskinfo *dp;
	char	xb[256];
	int	len;
	long	off;

	dp = (struct diskinfo *)xb;

	fillbytes((caddr_t)xb, sizeof (xb), '\0');
	if (read_toc(scgp, xb, track, sizeof (struct diskinfo), 0, FMT_TOC) < 0) {
		if (scgp->silent <= 0)
			errmsgno(EX_BAD, _("Cannot read TOC\n"));
		return (-1);
	}
	len = a_to_u_2_byte(dp->hd.len) + sizeof (struct tocheader)-2;
	if (len <  (int)sizeof (struct diskinfo))
		return (-1);

	off = a_to_4_byte(dp->desc[0].addr);
	if (offp)
		*offp = off;
	if (adrp)
		*adrp = dp->desc[0].adr;
	if (controlp)
		*controlp = dp->desc[0].control;

	if (msfp) {
		scgp->silent++;
		if (read_toc(scgp, xb, track, sizeof (struct diskinfo), 1, FMT_TOC)
									>= 0) {
			msfp->msf_min = dp->desc[0].addr[1];
			msfp->msf_sec = dp->desc[0].addr[2];
			msfp->msf_frame = dp->desc[0].addr[3];
		} else if (read_toc(scgp, xb, track, sizeof (struct diskinfo), 0, FMT_TOC)
									>= 0) {
			/*
			 * Some drives (e.g. the Philips CDD-522) don't support
			 * to read the TOC in MSF mode.
			 */
			long moff = a_to_4_byte(dp->desc[0].addr);

			lba_to_msf(moff, msfp);
		} else {
			msfp->msf_min = 0;
			msfp->msf_sec = 0;
			msfp->msf_frame = 0;
		}
		scgp->silent--;
	}

	if (modep == NULL)
		return (0);

	if (track == 0xAA) {
		*modep = -1;
		return (0);
	}

	fillbytes((caddr_t)xb, sizeof (xb), '\0');

	scgp->silent++;
	if (read_header(scgp, xb, off, 8, 0) >= 0) {
		*modep = xb[0];
	} else if (read_track_info_philips(scgp, xb, track, 14) >= 0) {
		*modep = xb[0xb] & 0xF;
	} else {
		*modep = -1;
	}
	scgp->silent--;
	return (0);
}

EXPORT	int
read_B0(scgp, isbcd, b0p, lop)
	SCSI	*scgp;
	BOOL	isbcd;
	long	*b0p;
	long	*lop;
{
	struct	fdiskinfo *dp;
	struct	ftrackdesc *tp;
	char	xb[8192];
	char	*pe;
	int	len;
	long	l;

	dp = (struct fdiskinfo *)xb;

	fillbytes((caddr_t)xb, sizeof (xb), '\0');
	if (read_toc_philips(scgp, xb, 1, sizeof (struct tocheader), 0, FMT_FULLTOC) < 0) {
		return (-1);
	}
	len = a_to_u_2_byte(dp->hd.len) + sizeof (struct tocheader)-2;
	if (len <  (int)sizeof (struct fdiskinfo))
		return (-1);
	if (read_toc_philips(scgp, xb, 1, len, 0, FMT_FULLTOC) < 0) {
		return (-1);
	}
	if (scgp->verbose) {
		scg_prbytes(_("TOC data: "), (Uchar *)xb,
			len > (int)sizeof (xb) - scg_getresid(scgp) ?
				sizeof (xb) - scg_getresid(scgp) : len);

		tp = &dp->desc[0];
		pe = &xb[len];

		while ((char *)tp < pe) {
			scg_prbytes(_("ENT: "), (Uchar *)tp, 11);
			tp++;
		}
	}
	tp = &dp->desc[0];
	pe = &xb[len];

	for (; (char *)tp < pe; tp++) {
		if (tp->sess_number != dp->hd.last)
			continue;
		if (tp->point != 0xB0)
			continue;
		if (scgp->verbose)
			scg_prbytes("B0: ", (Uchar *)tp, 11);
		if (isbcd) {
			l = msf_to_lba(from_bcd(tp->amin),
				from_bcd(tp->asec),
				from_bcd(tp->aframe), TRUE);
		} else {
			l = msf_to_lba(tp->amin,
				tp->asec,
				tp->aframe, TRUE);
		}
		if (b0p)
			*b0p = l;

		if (scgp->verbose)
			printf(_("B0 start: %ld\n"), l);

		if (isbcd) {
			l = msf_to_lba(from_bcd(tp->pmin),
				from_bcd(tp->psec),
				from_bcd(tp->pframe), TRUE);
		} else {
			l = msf_to_lba(tp->pmin,
				tp->psec,
				tp->pframe, TRUE);
		}

		if (scgp->verbose)
			printf(("B0 lout: %ld\n"), l);
		if (lop)
			*lop = l;
		return (0);
	}
	return (-1);
}


/*
 * Return address of first track in last session (SCSI-3/mmc version).
 */
EXPORT int
read_session_offset(scgp, offp)
	SCSI	*scgp;
	long	*offp;
{
	struct	diskinfo *dp;
	char	xb[256];
	int	len;

	dp = (struct diskinfo *)xb;

	fillbytes((caddr_t)xb, sizeof (xb), '\0');
	if (read_toc(scgp, (caddr_t)xb, 0, sizeof (struct tocheader), 0, FMT_SINFO) < 0)
		return (-1);

	if (scgp->verbose)
		scg_prbytes(_("tocheader: "),
		(Uchar *)xb, sizeof (struct tocheader) - scg_getresid(scgp));

	len = a_to_u_2_byte(dp->hd.len) + sizeof (struct tocheader)-2;
	if (len > (int)sizeof (xb)) {
		errmsgno(EX_BAD, _("Session info too big.\n"));
		return (-1);
	}
	if (read_toc(scgp, (caddr_t)xb, 0, len, 0, FMT_SINFO) < 0)
		return (-1);

	if (scgp->verbose)
		scg_prbytes(_("tocheader: "),
			(Uchar *)xb, len - scg_getresid(scgp));

	dp = (struct diskinfo *)xb;
	if (offp)
		*offp = a_to_u_4_byte(dp->desc[0].addr);
	return (0);
}

/*
 * Return address of first track in last session (pre SCSI-3 version).
 */
EXPORT int
read_session_offset_philips(scgp, offp)
	SCSI	*scgp;
	long	*offp;
{
	struct	sinfo *sp;
	char	xb[256];
	int	len;

	sp = (struct sinfo *)xb;

	fillbytes((caddr_t)xb, sizeof (xb), '\0');
	if (read_toc_philips(scgp, (caddr_t)xb, 0, sizeof (struct siheader), 0, FMT_SINFO) < 0)
		return (-1);
	len = a_to_u_2_byte(sp->hd.len) + sizeof (struct siheader)-2;
	if (len > (int)sizeof (xb)) {
		errmsgno(EX_BAD, _("Session info too big.\n"));
		return (-1);
	}
	if (read_toc_philips(scgp, (caddr_t)xb, 0, len, 0, FMT_SINFO) < 0)
		return (-1);
	/*
	 * Old drives return the number of finished sessions in first/finished
	 * a descriptor is returned for each session.
	 * New drives return the number of the first and last session
	 * one descriptor for the last finished session is returned
	 * as in SCSI-3
	 * In all cases the lowest session number is set to 1.
	 */
	sp = (struct sinfo *)xb;
	if (offp)
		*offp = a_to_u_4_byte(sp->desc[sp->hd.finished-1].addr);
	return (0);
}

EXPORT int
sense_secsize(scgp, current)
	SCSI	*scgp;
	int	current;
{
	Uchar	mode[0x100];
	Uchar	*p;
	Uchar	*ep;
	int	len;
	int	secsize = -1;

	scgp->silent++;
	(void) unit_ready(scgp);
	scgp->silent--;

	/* XXX Quick and dirty, musz verallgemeinert werden !!! */

	fillbytes(mode, sizeof (mode), '\0');
	scgp->silent++;

	len =	sizeof (struct scsi_mode_header) +
		sizeof (struct scsi_mode_blockdesc);
	/*
	 * Wenn wir hier get_mode_params() nehmen bekommen wir die Warnung:
	 * Warning: controller returns wrong page 1 for All pages page (3F).
	 */
	if (mode_sense(scgp, mode, len, 0x3F, current?0:2) < 0) {
		fillbytes(mode, sizeof (mode), '\0');
		if (mode_sense(scgp, mode, len, 0, current?0:2) < 0)	{ /* VU (block desc) */
			scgp->silent--;
			return (-1);
		}
	}
	if (mode[3] == 8) {
		if (scgp->debug) {
			printf(_("Density: 0x%X\n"), mode[4]);
			printf(_("Blocks:  %ld\n"), a_to_u_3_byte(&mode[5]));
			printf(_("Blocklen:%ld\n"), a_to_u_3_byte(&mode[9]));
		}
		secsize = a_to_u_3_byte(&mode[9]);
	}
	fillbytes(mode, sizeof (mode), '\0');
	/*
	 * The ACARD TECH AEC-7720 ATAPI<->SCSI adaptor
	 * chokes if we try to transfer more than 0x40 bytes with
	 * mode_sense of all pages. So try to avoid to run this
	 * command if possible.
	 */
	if (scgp->debug &&
	    mode_sense(scgp, mode, 0xFE, 0x3F, current?0:2) >= 0) {	/* All Pages */

		ep = mode+mode[0];	/* Points to last byte of data */
		p = &mode[4];
		p += mode[3];
		printf(_("Pages: "));
		while (p < ep) {
			printf("0x%X ", *p&0x3F);
			p += p[1]+2;
		}
		printf("\n");
	}
	scgp->silent--;

	return (secsize);
}

EXPORT int
select_secsize(scgp, secsize)
	SCSI	*scgp;
	int	secsize;
{
	struct scsi_mode_data md;
	int	count = sizeof (struct scsi_mode_header) +
			sizeof (struct scsi_mode_blockdesc);

	(void) test_unit_ready(scgp);	/* clear any error situation */

	fillbytes((caddr_t)&md, sizeof (md), '\0');
	md.header.blockdesc_len = 8;
	i_to_3_byte(md.blockdesc.lblen, secsize);

	return (mode_select(scgp, (Uchar *)&md, count, 0, scgp->inq->data_format >= 2));
}

EXPORT BOOL
is_cddrive(scgp)
	SCSI	*scgp;
{
	return (scgp->inq->type == INQ_ROMD || scgp->inq->type == INQ_WORM);
}

EXPORT BOOL
is_unknown_dev(scgp)
	SCSI	*scgp;
{
	return (scgp->dev == DEV_UNKNOWN);
}

#ifndef	DEBUG
#define	DEBUG
#endif
#ifdef	DEBUG

EXPORT int
read_scsi(scgp, bp, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	if (addr <= G0_MAXADDR && cnt < 256 && !is_atapi)
		return (read_g0(scgp, bp, addr, cnt));
	else
		return (read_g1(scgp, bp, addr, cnt));
}

EXPORT int
read_g0(scgp, bp, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (scgp->cap->c_bsize <= 0)
		raisecond("capacity_not_set", 0L);

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt*scgp->cap->c_bsize;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g0_cdb.cmd = SC_READ;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	g0_cdbaddr(&scmd->cdb.g0_cdb, addr);
	scmd->cdb.g0_cdb.count = cnt;
/*	scmd->cdb.g0_cdb.vu_56 = 1;*/

	scgp->cmdname = "read_g0";

	return (scg_cmd(scgp));
}

EXPORT int
read_g1(scgp, bp, addr, cnt)
	SCSI	*scgp;
	caddr_t	bp;
	long	addr;
	int	cnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (scgp->cap->c_bsize <= 0)
		raisecond("capacity_not_set", 0L);

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt*scgp->cap->c_bsize;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = SC_EREAD;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdbaddr(&scmd->cdb.g1_cdb, addr);
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "read_g1";

	return (scg_cmd(scgp));
}
#endif	/* DEBUG */

EXPORT BOOL
getdev(scgp, print)
	SCSI	*scgp;
	BOOL	print;
{
		BOOL	got_inquiry = TRUE;
		char	vendor_info[8+1];
		char	prod_ident[16+1];
		char	prod_revision[4+1];
		int	inq_len = 0;
	register struct	scg_cmd	*scmd = scgp->scmd;
	register struct scsi_inquiry *inq = scgp->inq;


	fillbytes((caddr_t)inq, sizeof (*inq), '\0');
	scgp->dev = DEV_UNKNOWN;
	scgp->silent++;
	(void) unit_ready(scgp);
	if (scmd->error >= SCG_FATAL &&
				!(scmd->scb.chk && scmd->sense_count > 0)) {
		scgp->silent--;
		return (FALSE);
	}


/*	if (scmd->error < SCG_FATAL || scmd->scb.chk && scmd->sense_count > 0){*/

	if (inquiry(scgp, (caddr_t)inq, sizeof (*inq)) < 0) {
		got_inquiry = FALSE;
	} else {
		inq_len = sizeof (*inq) - scg_getresid(scgp);
	}
	if (!got_inquiry) {
		if (scgp->verbose) {
			printf(
		_("error: %d scb.chk: %d sense_count: %d sense.code: 0x%x\n"),
				scmd->error, scmd->scb.chk,
				scmd->sense_count, scmd->sense.code);
		}
			/*
			 * Folgende Kontroller kennen das Kommando
			 * INQUIRY nicht:
			 *
			 * ADAPTEC	ACB-4000, ACB-4010, ACB 4070
			 * SYSGEN	SC4000
			 *
			 * Leider reagieren ACB40X0 und ACB5500 identisch
			 * wenn drive not ready (code == not ready),
			 * sie sind dann nicht zu unterscheiden.
			 */

		if (scmd->scb.chk && scmd->sense_count == 4) {
			/* Test auf SYSGEN				 */
			(void) qic02(scgp, 0x12);	/* soft lock on  */
			if (qic02(scgp, 1) < 0) {	/* soft lock off */
				scgp->dev = DEV_ACB40X0;
/*				scgp->dev = acbdev();*/
			} else {
				scgp->dev = DEV_SC4000;
				inq->type = INQ_SEQD;
				inq->removable = 1;
			}
		}
	} else if (scgp->verbose) {
		int	i;
		int	len = inq->add_len + 5;
		Uchar	ibuf[256+5];
		Uchar	*ip = (Uchar *)inq;
		Uchar	c;

		if (len > (int)sizeof (*inq) &&
				inquiry(scgp, (caddr_t)ibuf, inq->add_len+5) >= 0) {
			len = inq->add_len+5 - scg_getresid(scgp);
			ip = ibuf;
		} else {
			len = sizeof (*inq);
		}
		printf(_("Inquiry Data   : "));
		for (i = 0; i < len; i++) {
			c = ip[i];
			if (c >= ' ' && c < 0177)
				printf("%c", c);
			else
				printf(".");
		}
		printf("\n");
	}

	strncpy(vendor_info, inq->inq_vendor_info, sizeof (inq->inq_vendor_info));
	strncpy(prod_ident, inq->inq_prod_ident, sizeof (inq->inq_prod_ident));
	strncpy(prod_revision, inq->inq_prod_revision, sizeof (inq->inq_prod_revision));

	vendor_info[sizeof (inq->inq_vendor_info)] = '\0';
	prod_ident[sizeof (inq->inq_prod_ident)] = '\0';
	prod_revision[sizeof (inq->inq_prod_revision)] = '\0';

	switch (inq->type) {

	case INQ_DASD:
		if (inq->add_len == 0 && inq->inq_vendor_info[0] != '\0') {
			Uchar	*p;
			/*
			 * NT-4.0 creates fake inquiry data for IDE disks.
			 * Unfortunately, it does not set add_len wo we
			 * check if vendor_info, prod_ident and prod_revision
			 * contains valid chars for a CCS inquiry.
			 */
			if (inq_len >= 36)
				inq->add_len = 31;

			for (p = (Uchar *)&inq->inq_vendor_info[0];
					p < (Uchar *)&inq->inq_prod_revision[4];
									p++) {
				if (*p < 0x20 || *p > 0x7E) {
					inq->add_len = 0;
					break;
				}
			}
		}
		if (inq->add_len == 0) {
			if (scgp->dev == DEV_UNKNOWN && got_inquiry) {
				scgp->dev = DEV_ACB5500;
				strncpy(inq->inq_info_space,
					"ADAPTEC ACB-5500        FAKE",
					sizeof (inq->inq_info_space));

			} else switch (scgp->dev) {

			case DEV_ACB40X0:
				strncpy(inq->inq_info_space,
					"ADAPTEC ACB-40X0        FAKE",
					sizeof (inq->inq_info_space));
				break;
			case DEV_ACB4000:
				strncpy(inq->inq_info_space,
					"ADAPTEC ACB-4000        FAKE",
					sizeof (inq->inq_info_space));
				break;
			case DEV_ACB4010:
				strncpy(inq->inq_info_space,
					"ADAPTEC ACB-4010        FAKE",
					sizeof (inq->inq_info_space));
				break;
			case DEV_ACB4070:
				strncpy(inq->inq_info_space,
					"ADAPTEC ACB-4070        FAKE",
					sizeof (inq->inq_info_space));
				break;
			}
		} else if (inq->add_len < 31) {
			scgp->dev = DEV_NON_CCS_DSK;

		} else if (strbeg("EMULEX", vendor_info)) {
			if (strbeg("MD21", prod_ident))
				scgp->dev = DEV_MD21;
			if (strbeg("MD23", prod_ident))
				scgp->dev = DEV_MD23;
			else
				scgp->dev = DEV_CCS_GENDISK;
		} else if (strbeg("ADAPTEC", vendor_info)) {
			if (strbeg("ACB-4520", prod_ident))
				scgp->dev = DEV_ACB4520A;
			if (strbeg("ACB-4525", prod_ident))
				scgp->dev = DEV_ACB4525;
			else
				scgp->dev = DEV_CCS_GENDISK;
		} else if (strbeg("SONY", vendor_info) &&
					strbeg("SMO-C501", prod_ident)) {
			scgp->dev = DEV_SONY_SMO;
		} else {
			scgp->dev = DEV_CCS_GENDISK;
		}
		break;

	case INQ_SEQD:
		if (scgp->dev == DEV_SC4000) {
			strncpy(inq->inq_info_space,
				"SYSGEN  SC4000          FAKE",
					sizeof (inq->inq_info_space));
		} else if (inq->add_len == 0 &&
					inq->removable &&
						inq->ansi_version == 1) {
			scgp->dev = DEV_MT02;
			strncpy(inq->inq_info_space,
				"EMULEX  MT02            FAKE",
					sizeof (inq->inq_info_space));
		}
		break;

/*	case INQ_OPTD:*/
	case INQ_ROMD:
	case INQ_WORM:
		if (strbeg("RXT-800S", prod_ident))
			scgp->dev = DEV_RXT800S;

		/*
		 * Start of CD-Recorders:
		 */
		if (strbeg("ACER", vendor_info)) {
			if (strbeg("CR-4020C", prod_ident))
				scgp->dev = DEV_RICOH_RO_1420C;

		} else if (strbeg("CREATIVE", vendor_info)) {
			if (strbeg("CDR2000", prod_ident))
				scgp->dev = DEV_RICOH_RO_1060C;

		} else if (strbeg("GRUNDIG", vendor_info)) {
			if (strbeg("CDR100IPW", prod_ident))
				scgp->dev = DEV_CDD_2000;

		} else if (strbeg("JVC", vendor_info)) {
			if (strbeg("XR-W2001", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;
			else if (strbeg("XR-W2010", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;
			else if (strbeg("R2626", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;

		} else if (strbeg("MITSBISH", vendor_info)) {

#ifdef	XXXX_REALLY
			/* It's MMC compliant */
			if (strbeg("CDRW226", prod_ident))
				scgp->dev = DEV_MMC_CDRW;
#else
			/* EMPTY */
#endif

		} else if (strbeg("MITSUMI", vendor_info)) {
			/* Don't know any product string */
			scgp->dev = DEV_CDD_522;

		} else if (strbeg("OPTIMA", vendor_info)) {
			if (strbeg("CD-R 650", prod_ident))
				scgp->dev = DEV_SONY_CDU_924;

		} else if (strbeg("PHILIPS", vendor_info) ||
				strbeg("IMS", vendor_info) ||
				strbeg("KODAK", vendor_info) ||
				strbeg("HP", vendor_info)) {

			if (strbeg("CDD521/00", prod_ident))
				scgp->dev = DEV_CDD_521_OLD;
			else if (strbeg("CDD521/02", prod_ident))
				scgp->dev = DEV_CDD_521_OLD;		/* PCD 200R? */
			else if (strbeg("CDD521", prod_ident))
				scgp->dev = DEV_CDD_521;

			if (strbeg("CDD522", prod_ident))
				scgp->dev = DEV_CDD_522;
			if (strbeg("PCD225", prod_ident))
				scgp->dev = DEV_CDD_522;
			if (strbeg("KHSW/OB", prod_ident))	/* PCD600 */
				scgp->dev = DEV_PCD_600;
			if (strbeg("CDR-240", prod_ident))
				scgp->dev = DEV_CDD_2000;

			if (strbeg("CDD20", prod_ident))
				scgp->dev = DEV_CDD_2000;
			if (strbeg("CDD26", prod_ident))
				scgp->dev = DEV_CDD_2600;

			if (strbeg("C4324/C4325", prod_ident))
				scgp->dev = DEV_CDD_2000;
			if (strbeg("CD-Writer 6020", prod_ident))
				scgp->dev = DEV_CDD_2600;

		} else if (strbeg("PINNACLE", vendor_info)) {
			if (strbeg("RCD-1000", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;
			if (strbeg("RCD5020", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;
			if (strbeg("RCD5040", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;
			if (strbeg("RCD 4X4", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;

		} else if (strbeg("PIONEER", vendor_info)) {
			if (strbeg("CD-WO DW-S114X", prod_ident))
				scgp->dev = DEV_PIONEER_DW_S114X;
			else if (strbeg("CD-WO DR-R504X", prod_ident))	/* Reoprt from philip@merge.com */
				scgp->dev = DEV_PIONEER_DW_S114X;
			else if (strbeg("DVD-R DVR-S101", prod_ident))
				scgp->dev = DEV_PIONEER_DVDR_S101;

		} else if (strbeg("PLASMON", vendor_info)) {
			if (strbeg("RF4100", prod_ident))
				scgp->dev = DEV_PLASMON_RF_4100;
			else if (strbeg("CDR4220", prod_ident))
				scgp->dev = DEV_CDD_2000;

		} else if (strbeg("PLEXTOR", vendor_info)) {
			if (strbeg("CD-R   PX-R24CS", prod_ident))
				scgp->dev = DEV_RICOH_RO_1420C;

		} else if (strbeg("RICOH", vendor_info)) {
			if (strbeg("RO-1420C", prod_ident))
				scgp->dev = DEV_RICOH_RO_1420C;
			if (strbeg("RO1060C", prod_ident))
				scgp->dev = DEV_RICOH_RO_1060C;

		} else if (strbeg("SAF", vendor_info)) {	/* Smart & Friendly */
			if (strbeg("CD-R2004", prod_ident) ||
			    strbeg("CD-R2006 ", prod_ident))
				scgp->dev = DEV_SONY_CDU_924;
			else if (strbeg("CD-R2006PLUS", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;
			else if (strbeg("CD-RW226", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;
			else if (strbeg("CD-R4012", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;

		} else if (strbeg("SANYO", vendor_info)) {
			if (strbeg("CD-WO CRD-R24S", prod_ident))
				scgp->dev = DEV_CDD_521;

		} else if (strbeg("SONY", vendor_info)) {
			if (strbeg("CD-R   CDU92", prod_ident) ||
			    strbeg("CD-R   CDU94", prod_ident))
				scgp->dev = DEV_SONY_CDU_924;

		} else if (strbeg("TEAC", vendor_info)) {
			if (strbeg("CD-R50S", prod_ident) ||
			    strbeg("CD-R55S", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;

		} else if (strbeg("TRAXDATA", vendor_info) ||
				strbeg("Traxdata", vendor_info)) {
			if (strbeg("CDR4120", prod_ident))
				scgp->dev = DEV_TEAC_CD_R50S;

		} else if (strbeg("T.YUDEN", vendor_info)) {
			if (strbeg("CD-WO EW-50", prod_ident))
				scgp->dev = DEV_TYUDEN_EW50;

		} else if (strbeg("WPI", vendor_info)) {	/* Wearnes */
			if (strbeg("CDR-632P", prod_ident))
				scgp->dev = DEV_CDD_2600;

		} else if (strbeg("YAMAHA", vendor_info)) {
			if (strbeg("CDR10", prod_ident))
				scgp->dev = DEV_YAMAHA_CDR_100;
			if (strbeg("CDR200", prod_ident))
				scgp->dev = DEV_YAMAHA_CDR_400;
			if (strbeg("CDR400", prod_ident))
				scgp->dev = DEV_YAMAHA_CDR_400;

		} else if (strbeg("MATSHITA", vendor_info)) {
			if (strbeg("CD-R   CW-7501", prod_ident))
				scgp->dev = DEV_MATSUSHITA_7501;
			if (strbeg("CD-R   CW-7502", prod_ident))
				scgp->dev = DEV_MATSUSHITA_7502;
		}
		if (scgp->dev == DEV_UNKNOWN) {
			/*
			 * We do not have Manufacturer strings for
			 * the following drives.
			 */
			if (strbeg("CDS615E", prod_ident))	/* Olympus */
				scgp->dev = DEV_SONY_CDU_924;
		}
		if (scgp->dev == DEV_UNKNOWN && inq->type == INQ_ROMD) {
			BOOL	cdrr	 = FALSE;
			BOOL	cdwr	 = FALSE;
			BOOL	cdrrw	 = FALSE;
			BOOL	cdwrw	 = FALSE;
			BOOL	dvd	 = FALSE;
			BOOL	dvdwr	 = FALSE;

			scgp->dev = DEV_CDROM;

			if (mmc_check(scgp, &cdrr, &cdwr, &cdrrw, &cdwrw,
								&dvd, &dvdwr))
				scgp->dev = DEV_MMC_CDROM;
			if (cdwr)
				scgp->dev = DEV_MMC_CDR;
			if (cdwrw)
				scgp->dev = DEV_MMC_CDRW;
			if (dvd)
				scgp->dev = DEV_MMC_DVD;
			if (dvdwr)
				scgp->dev = DEV_MMC_DVD_WR;
		}
		break;

	case INQ_PROCD:
		if (strbeg("BERTHOLD", vendor_info)) {
			if (strbeg("", prod_ident))
				scgp->dev = DEV_HRSCAN;
		}
		break;

	case INQ_SCAN:
		scgp->dev = DEV_MS300A;
		break;
	}
	scgp->silent--;
	if (!print)
		return (TRUE);

	if (scgp->dev == DEV_UNKNOWN && !got_inquiry) {
#ifdef	PRINT_INQ_ERR
		scg_printerr(scgp);
#endif
		return (FALSE);
	}

	printinq(scgp, stdout);
	return (TRUE);
}

EXPORT void
printinq(scgp, f)
	SCSI	*scgp;
	FILE	*f;
{
	register struct scsi_inquiry *inq = scgp->inq;

	fprintf(f, _("Device type    : "));
	scg_fprintdev(f, inq);
	fprintf(f, ("Version        : %d\n"), inq->ansi_version);
	fprintf(f, _("Response Format: %d\n"), inq->data_format);
	if (inq->data_format >= 2) {
		fprintf(f, _("Capabilities   : "));
		if (inq->aenc)		fprintf(f, "AENC ");
		if (inq->termiop)	fprintf(f, "TERMIOP ");
		if (inq->reladr)	fprintf(f, "RELADR ");
		if (inq->wbus32)	fprintf(f, "WBUS32 ");
		if (inq->wbus16)	fprintf(f, "WBUS16 ");
		if (inq->sync)		fprintf(f, "SYNC ");
		if (inq->linked)	fprintf(f, "LINKED ");
		if (inq->cmdque)	fprintf(f, "CMDQUE ");
		if (inq->softreset)	fprintf(f, "SOFTRESET ");
		fprintf(f, "\n");
	}
	if (inq->add_len >= 31 ||
			inq->inq_vendor_info[0] ||
			inq->inq_prod_ident[0] ||
			inq->inq_prod_revision[0]) {
		fprintf(f, _("Vendor_info    : '%.8s'\n"), inq->inq_vendor_info);
		fprintf(f, _("Identifikation : '%.16s'\n"), inq->inq_prod_ident);
		fprintf(f, _("Revision       : '%.4s'\n"), inq->inq_prod_revision);
	}
}

EXPORT void
printdev(scgp)
	SCSI	*scgp;
{
	printf(_("Device seems to be: "));

	switch (scgp->dev) {

	case DEV_UNKNOWN:	printf(_("unknown"));		break;
	case DEV_ACB40X0:	printf("Adaptec 4000/4010/4070"); break;
	case DEV_ACB4000:	printf("Adaptec 4000");		break;
	case DEV_ACB4010:	printf("Adaptec 4010");		break;
	case DEV_ACB4070:	printf("Adaptec 4070");		break;
	case DEV_ACB5500:	printf("Adaptec 5500");		break;
	case DEV_ACB4520A:	printf("Adaptec 4520A");	break;
	case DEV_ACB4525:	printf("Adaptec 4525");		break;
	case DEV_MD21:		printf("Emulex MD21");		break;
	case DEV_MD23:		printf("Emulex MD23");		break;
	case DEV_NON_CCS_DSK:	printf("Generic NON CCS Disk");	break;
	case DEV_CCS_GENDISK:	printf("Generic CCS Disk");	break;
	case DEV_SONY_SMO:	printf("Sony SMO-C501");	break;
	case DEV_MT02:		printf("Emulex MT02");		break;
	case DEV_SC4000:	printf("Sysgen SC4000");	break;
	case DEV_RXT800S:	printf("Maxtor RXT800S");	break;
	case DEV_HRSCAN:	printf("Berthold HR-Scanner");	break;
	case DEV_MS300A:	printf("Microtek MS300A");	break;

	case DEV_CDROM:		printf("Generic CD-ROM");	break;
	case DEV_MMC_CDROM:	printf("Generic mmc CD-ROM");	break;
	case DEV_MMC_CDR:	printf("Generic mmc CD-R");	break;
	case DEV_MMC_CDRW:	printf("Generic mmc CD-RW");	break;
	case DEV_MMC_DVD:	printf("Generic mmc2 DVD-ROM");	break;
	case DEV_MMC_DVD_WR:	printf("Generic mmc2 DVD-R/DVD-RW/DVD-RAM"); break;
	case DEV_CDD_521_OLD:	printf("Philips old CDD-521");	break;
	case DEV_CDD_521:	printf("Philips CDD-521");	break;
	case DEV_CDD_522:	printf("Philips CDD-522");	break;
	case DEV_PCD_600:	printf("Kodak PCD-600");	break;
	case DEV_CDD_2000:	printf("Philips CDD-2000");	break;
	case DEV_CDD_2600:	printf("Philips CDD-2600");	break;
	case DEV_YAMAHA_CDR_100:printf("Yamaha CDR-100");	break;
	case DEV_YAMAHA_CDR_400:printf("Yamaha CDR-400");	break;
	case DEV_PLASMON_RF_4100:printf("Plasmon RF-4100");	break;
	case DEV_SONY_CDU_924:	printf("Sony CDU-924S");	break;
	case DEV_RICOH_RO_1060C:printf("Ricoh RO-1060C");	break;
	case DEV_RICOH_RO_1420C:printf("Ricoh RO-1420C");	break;
	case DEV_TEAC_CD_R50S:	printf("Teac CD-R50S");		break;
	case DEV_MATSUSHITA_7501:printf("Matsushita CW-7501");	break;
	case DEV_MATSUSHITA_7502:printf("Matsushita CW-7502");	break;

	case DEV_PIONEER_DW_S114X: printf("Pioneer DW-S114X");	break;
	case DEV_PIONEER_DVDR_S101:printf("Pioneer DVD-R S101"); break;

	default:		printf(_("Missing Entry for dev %d"),
						scgp->dev);	break;

	}
	printf(".\n");

}

EXPORT BOOL
do_inquiry(scgp, print)
	SCSI	*scgp;
	int	print;
{
	if (getdev(scgp, print)) {
		if (print)
			printdev(scgp);
		return (TRUE);
	} else {
		return (FALSE);
	}
}

EXPORT BOOL
recovery_needed(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
		int err;
	register struct	scg_cmd	*scmd = scgp->scmd;

	scgp->silent++;
	err = test_unit_ready(scgp);
	scgp->silent--;

	if (err >= 0)
		return (FALSE);
	else if (scmd->error >= SCG_FATAL)	/* nicht selektierbar */
		return (FALSE);

	if (scmd->sense.code < 0x70)		/* non extended Sense */
		return (FALSE);

						/* XXX Old Philips code */
	return (((struct scsi_ext_sense *)&scmd->sense)->sense_code == 0xD0);
}

EXPORT int
scsi_load(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	int	key;
	int	code;

	if (dp && (dp->cdr_flags & CDR_CADDYLOAD) == 0) {
		if (scsi_start_stop_unit(scgp, 1, 1, dp->cdr_cmdflags&F_IMMED) >= 0)
			return (0);
	}

	if (wait_unit_ready(scgp, 60))
		return (0);

	key = scg_sense_key(scgp);
	code = scg_sense_code(scgp);

	if (key == SC_NOT_READY && (code == 0x3A || code == 0x30)) {
		errmsgno(EX_BAD, _("Cannot load media with %s drive!\n"),
			dp && (dp->cdr_flags & CDR_CADDYLOAD) ? _("caddy") : _("this"));
		errmsgno(EX_BAD, _("Try to load media by hand.\n"));
	}
	return (-1);
}

EXPORT int
scsi_unload(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	return (scsi_start_stop_unit(scgp, 0, 1, dp && (dp->cdr_cmdflags&F_IMMED)));
}

EXPORT int
scsi_cdr_write(scgp, bp, sectaddr, size, blocks, islast)
	SCSI	*scgp;
	caddr_t	bp;		/* address of buffer */
	long	sectaddr;	/* disk address (sector) to put */
	long	size;		/* number of bytes to transfer */
	int	blocks;		/* sector count */
	BOOL	islast;		/* last write for track */
{
	return (write_xg1(scgp, bp, sectaddr, size, blocks));
}

EXPORT struct cd_mode_page_2A *
mmc_cap(scgp, modep)
	SCSI	*scgp;
	Uchar	*modep;
{
	int	len;
	int	val;
	Uchar	mode[0x100];
	struct	cd_mode_page_2A *mp;
	struct	cd_mode_page_2A *mp2;


retry:
	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	if (!get_mode_params(scgp, 0x2A, _("CD capabilities"),
			mode, (Uchar *)0, (Uchar *)0, (Uchar *)0, &len)) {

		if (scg_sense_key(scgp) == SC_NOT_READY) {
			if (wait_unit_ready(scgp, 60))
				goto retry;
		}
		return (NULL);		/* Pre SCSI-3/mmc drive		*/
	}

	if (len == 0)			/* Pre SCSI-3/mmc drive		*/
		return (NULL);

	mp = (struct cd_mode_page_2A *)
		(mode + sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len);

	/*
	 * Do some heuristics against pre SCSI-3/mmc VU page 2A
	 * We should test for a minimum p_len of 0x14, but some
	 * buggy CD-ROM readers ommit the write speed values.
	 */
	if (mp->p_len < 0x10)
		return (NULL);

	val = a_to_u_2_byte(mp->max_read_speed);
	if (val != 0 && val < 176)
		return (NULL);

	val = a_to_u_2_byte(mp->cur_read_speed);
	if (val != 0 && val < 176)
		return (NULL);

	len -= sizeof (struct scsi_mode_header) +
		((struct scsi_mode_header *)mode)->blockdesc_len;
	if (modep)
		mp2 = (struct cd_mode_page_2A *)modep;
	else
		mp2 = malloc(len);
	if (mp2)
		movebytes(mp, mp2, len);

	return (mp2);
}

EXPORT void
mmc_getval(mp, cdrrp, cdwrp, cdrrwp, cdwrwp, dvdp, dvdwp)
	struct	cd_mode_page_2A *mp;
	BOOL	*cdrrp;				/* CD ROM		*/
	BOOL	*cdwrp;				/* CD-R writer		*/
	BOOL	*cdrrwp;			/* CD-RW reader		*/
	BOOL	*cdwrwp;			/* CD-RW writer		*/
	BOOL	*dvdp;				/* DVD reader		*/
	BOOL	*dvdwp;				/* DVD writer		*/
{
	BOOL	isdvd;				/* Any DVD reader	*/
	BOOL	isdvd_wr;			/* DVD writer (R / RAM)	*/
	BOOL	iscd_wr;			/* CD  writer		*/

	iscd_wr = (mp->cd_r_write != 0) ||	/* SCSI-3/mmc CD-R	*/
		    (mp->cd_rw_write != 0);	/* SCSI-3/mmc CD-RW	*/

	if (cdrrp)
		*cdrrp = (mp->cd_r_read != 0);	/* SCSI-3/mmc CD	*/
	if (cdwrp)
		*cdwrp = (mp->cd_r_write != 0);	/* SCSI-3/mmc CD-R	*/

	if (cdrrwp)
		*cdrrwp = (mp->cd_rw_read != 0); /* SCSI-3/mmc CD	*/
	if (cdwrwp)
		*cdwrwp = (mp->cd_rw_write != 0); /* SCSI-3/mmc CD-RW	*/

	isdvd =					/* SCSI-3/mmc2 DVD 	*/
		(mp->dvd_ram_read + mp->dvd_r_read  +
		    mp->dvd_rom_read) != 0;

	isdvd_wr =				/* SCSI-3/mmc2 DVD writer*/
		(mp->dvd_ram_write + mp->dvd_r_write) != 0;

	if (dvdp)
		*dvdp = isdvd;
	if (dvdwp)
		*dvdwp = isdvd_wr;
}

EXPORT BOOL
is_mmc(scgp, cdwp, dvdwp)
	SCSI	*scgp;
	BOOL	*cdwp;				/* CD  writer		*/
	BOOL	*dvdwp;				/* DVD writer		*/
{
	BOOL	cdwr	= FALSE;
	BOOL	cdwrw	= FALSE;

	if (cdwp)
		*cdwp = FALSE;
	if (dvdwp)
		*dvdwp = FALSE;

	if (!mmc_check(scgp, NULL, &cdwr, NULL, &cdwrw, NULL, dvdwp))
		return (FALSE);

	if (cdwp)
		*cdwp = cdwr | cdwrw;

	return (TRUE);
}

EXPORT BOOL
mmc_check(scgp, cdrrp, cdwrp, cdrrwp, cdwrwp, dvdp, dvdwp)
	SCSI	*scgp;
	BOOL	*cdrrp;				/* CD ROM		*/
	BOOL	*cdwrp;				/* CD-R writer		*/
	BOOL	*cdrrwp;			/* CD-RW reader		*/
	BOOL	*cdwrwp;			/* CD-RW writer		*/
	BOOL	*dvdp;				/* DVD reader		*/
	BOOL	*dvdwp;				/* DVD writer		*/
{
	Uchar	mode[0x100];
	BOOL	was_atapi;
	struct	cd_mode_page_2A *mp;

	if (scgp->inq->type != INQ_ROMD)
		return (FALSE);

	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	was_atapi = allow_atapi(scgp, TRUE);
	scgp->silent++;
	mp = mmc_cap(scgp, mode);
	scgp->silent--;
	allow_atapi(scgp, was_atapi);
	if (mp == NULL)
		return (FALSE);

	mmc_getval(mp, cdrrp, cdwrp, cdrrwp, cdwrwp, dvdp, dvdwp);

	return (TRUE);			/* Generic SCSI-3/mmc CD	*/
}

LOCAL void
print_speed(fmt, val)
	char	*fmt;
	int	val;
{
	printf("  %s: %5d kB/s", fmt, val);
	printf(" (CD %3ux,", val/176);
	printf(" DVD %2ux,", val/1385);
	printf(" BD %2ux)\n", val/4495);
}

#define	DOES(what, flag)	printf(_("  Does %s%s\n"), flag?"":_("not "), what)
#define	IS(what, flag)		printf(_("  Is %s%s\n"), flag?"":_("not "), what)
#define	VAL(what, val)		printf(_("  %s: %d\n"), what, val[0]*256 + val[1])
#define	SVAL(what, val)		printf(_("  %s: %s\n"), what, val)

EXPORT void
print_capabilities(scgp)
	SCSI	*scgp;
{
	BOOL	was_atapi;
	Uchar	mode[0x100];
	struct	cd_mode_page_2A *mp;
static	const	char	*bclk[4] = {"32", "16", "24", "24 (I2S)"};
static	const	char	*load[8] = {"caddy", "tray", "pop-up", "reserved(3)",
				"disc changer", "cartridge changer",
				"reserved(6)", "reserved(7)" };
static	const	char	*rotctl[4] = {"CLV/PCAV", "CAV", "reserved(2)", "reserved(3)"};


	if (scgp->inq->type != INQ_ROMD)
		return;

	fillbytes((caddr_t)mode, sizeof (mode), '\0');

	was_atapi = allow_atapi(scgp, TRUE);	/* Try to switch to 10 byte mode cmds */
	scgp->silent++;
	mp = mmc_cap(scgp, mode);
	scgp->silent--;
	allow_atapi(scgp, was_atapi);
	if (mp == NULL)
		return;

	printf(_("\nDrive capabilities, per"));
	if (mp->p_len >= 28)
		printf(" MMC-3");
	else if (mp->p_len >= 24)
		printf(" MMC-2");
	else
		printf(" MMC");
	printf(_(" page 2A:\n\n"));

	DOES(_("read CD-R media"), mp->cd_r_read);
	DOES(_("write CD-R media"), mp->cd_r_write);
	DOES(_("read CD-RW media"), mp->cd_rw_read);
	DOES(_("write CD-RW media"), mp->cd_rw_write);
	DOES(_("read DVD-ROM media"), mp->dvd_rom_read);
	DOES(_("read DVD-R media"), mp->dvd_r_read);
	DOES(_("write DVD-R media"), mp->dvd_r_write);
	DOES(_("read DVD-RAM media"), mp->dvd_ram_read);
	DOES(_("write DVD-RAM media"), mp->dvd_ram_write);
	DOES(_("support test writing"), mp->test_write);
	printf("\n");
	DOES(_("read Mode 2 Form 1 blocks"), mp->mode_2_form_1);
	DOES(_("read Mode 2 Form 2 blocks"), mp->mode_2_form_2);
	DOES(_("read digital audio blocks"), mp->cd_da_supported);
	if (mp->cd_da_supported)
		DOES(_("restart non-streamed digital audio reads accurately"), mp->cd_da_accurate);
	DOES(_("support Buffer-Underrun-Free recording"), mp->BUF);
	DOES(_("read multi-session CDs"), mp->multi_session);
	DOES(_("read fixed-packet CD media using Method 2"), mp->method2);
	DOES(_("read CD bar code"), mp->read_bar_code);
	DOES(_("read R-W subcode information"), mp->rw_supported);
	if (mp->rw_supported)
		DOES(_("return R-W subcode de-interleaved and error-corrected"), mp->rw_deint_corr);
	DOES(_("read raw P-W subcode data from lead in"), mp->pw_in_lead_in);
	DOES(_("return CD media catalog number"), mp->UPC);
	DOES(_("return CD ISRC information"), mp->ISRC);
	DOES(_("support C2 error pointers"), mp->c2_pointers);
	DOES(_("deliver composite A/V data"), mp->composite);
	printf("\n");
	DOES(_("play audio CDs"), mp->audio_play);
	if (mp->audio_play) {
		VAL(_("Number of volume control levels"), mp->num_vol_levels);
		DOES(_("support individual volume control setting for each channel"), mp->sep_chan_vol);
		DOES(_("support independent mute setting for each channel"), mp->sep_chan_mute);
		DOES(_("support digital output on port 1"), mp->digital_port_1);
		DOES(_("support digital output on port 2"), mp->digital_port_2);
		if (mp->digital_port_1 || mp->digital_port_2) {
			DOES(_("send digital data LSB-first"), mp->LSBF);
			DOES(_("set LRCK high for left-channel data"), mp->RCK);
			DOES(_("have valid data on falling edge of clock"), mp->BCK);
			SVAL(_("Length of data in BCLKs"), bclk[mp->length]);
		}
	}
	printf("\n");
	SVAL(_("Loading mechanism type"), load[mp->loading_type]);
	DOES(_("support ejection of CD via START/STOP command"), mp->eject);
	DOES(_("lock media on power up via prevent jumper"), mp->prevent_jumper);
	DOES(_("allow media to be locked in the drive via PREVENT/ALLOW command"), mp->lock);
	IS(_("currently in a media-locked state"), mp->lock_state);
	DOES(_("support changing side of disk"), mp->side_change);
	DOES(_("have load-empty-slot-in-changer feature"), mp->sw_slot_sel);
	DOES(_("support Individual Disk Present feature"), mp->disk_present_rep);
	printf("\n");
	print_speed(_("Maximum read  speed"), a_to_u_2_byte(mp->max_read_speed));
	print_speed(_("Current read  speed"), a_to_u_2_byte(mp->cur_read_speed));
	print_speed(_("Maximum write speed"), a_to_u_2_byte(mp->max_write_speed));
	if (mp->p_len >= 28)
		print_speed(_("Current write speed"), a_to_u_2_byte(mp->v3_cur_write_speed));
	else
		print_speed(_("Current write speed"), a_to_u_2_byte(mp->cur_write_speed));
	if (mp->p_len >= 28) {
		SVAL(_("Rotational control selected"), rotctl[mp->rot_ctl_sel]);
	}
	VAL(_("Buffer size in KB"), mp->buffer_size);

	if (mp->p_len >= 24) {
		VAL(_("Copy management revision supported"), mp->copy_man_rev);
	}

	if (mp->p_len >= 28) {
		struct cd_wr_speed_performance *pp;
		Uint	ndesc;
		Uint	i;
		Uint	n;

		ndesc = a_to_u_2_byte(mp->num_wr_speed_des);
		pp = mp->wr_speed_des;
		printf(_("  Number of supported write speeds: %d\n"), ndesc);
		for (i = 0; i < ndesc; i++, pp++) {
			printf(_("  Write speed # %d:"), i);
			n = a_to_u_2_byte(pp->wr_speed_supp);
			printf(" %5d kB/s", n);
			printf(" %s", rotctl[pp->rot_ctl_sel]);
			printf(" (CD %3ux,", n/176);
			printf(" DVD %2ux,", n/1385);
			printf(" BD %2ux)\n", n/4495);
		}
	}

	/* Generic SCSI-3/mmc CD	*/
}

EXPORT int
verify(scgp, start, count, bad_block)
	SCSI	*scgp;
	long	start;
	int	count;
	long	*bad_block;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)0;
	scmd->size = 0;
	scmd->flags = SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x2F;	/* Verify */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdbaddr(&scmd->cdb.g1_cdb, start);
	g1_cdblen(&scmd->cdb.g1_cdb, count);

	scgp->cmdname = "verify";

	if (scg_cmd(scgp) < 0) {
		if (scmd->sense.code >= 0x70) {	/* extended Sense */
			*bad_block =
				a_to_4_byte(&((struct scsi_ext_sense *)
							&scmd->sense)->info_1);
		} else {
			*bad_block = a_to_u_3_byte(&scmd->sense.high_addr);
		}
		return (-1);
	}
	return (0);
}
