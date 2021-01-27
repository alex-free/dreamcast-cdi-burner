/* @(#)modesense.c	1.158 09/07/10 Copyright 1995-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)modesense.c	1.158 09/07/10 Copyright 1995-2009 J. Schilling";
#endif
/*
 *	SCSI command functions for mode sense/mode select handling.
 *
 *	Copyright (c) 1995-2009 J. Schilling
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

#include <schily/stdio.h>
#include <schily/standard.h>

#include <schily/utypes.h>
#include <schily/btorder.h>
#include <schily/intcvt.h>
#include <schily/schily.h>

#include <scg/scgcmd.h>
#include <scg/scsidefs.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "libscgcmd.h"

EXPORT	BOOL	__is_atapi	__PR((void));
EXPORT	BOOL	allow_atapi	__PR((SCSI *scgp, BOOL new));
EXPORT	int	mode_select	__PR((SCSI *scgp, Uchar *, int, int, int));
EXPORT	int	mode_sense	__PR((SCSI *scgp, Uchar *dp, int cnt, int page, int pcf));
EXPORT	int	mode_select_sg0	__PR((SCSI *scgp, Uchar *, int, int, int));
EXPORT	int	mode_sense_sg0	__PR((SCSI *scgp, Uchar *dp, int cnt, int page, int pcf));
EXPORT	int	mode_select_g0	__PR((SCSI *scgp, Uchar *, int, int, int));
EXPORT	int	mode_select_g1	__PR((SCSI *scgp, Uchar *, int, int, int));
EXPORT	int	mode_sense_g0	__PR((SCSI *scgp, Uchar *dp, int cnt, int page, int pcf));
EXPORT	int	mode_sense_g1	__PR((SCSI *scgp, Uchar *dp, int cnt, int page, int pcf));

/*
 * XXX First try to handle ATAPI:
 * XXX ATAPI cannot handle SCSI 6 byte commands.
 * XXX We try to simulate 6 byte mode sense/select.
 */
LOCAL BOOL	is_atapi;

EXPORT BOOL
__is_atapi()
{
	return (is_atapi);
}

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

	if (scgp->verbose) scg_prbytes("Mode Parameters (un-converted)", dp, cnt);

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

	if (scgp->verbose) scg_prbytes("Mode Sense Data (converted)", dp, amt);
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
		error("%s ", smp?"Save":"Set ");
		scg_prbytes("Mode Parameters", dp, cnt);
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
		printf("%s ", smp?"Save":"Set ");
		scg_prbytes("Mode Parameters", dp, cnt);
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
	if (scgp->verbose) scg_prbytes("Mode Sense Data", dp, cnt - scg_getresid(scgp));
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
	if (scgp->verbose) scg_prbytes("Mode Sense Data", dp, cnt - scg_getresid(scgp));
	return (0);
}
