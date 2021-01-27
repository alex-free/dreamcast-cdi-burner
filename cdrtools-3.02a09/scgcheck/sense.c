/* @(#)sense.c	1.11 10/12/19 Copyright 2001-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)sense.c	1.11 10/12/19 Copyright 2001-2010 J. Schilling";
#endif
/*
 *	Copyright (c) 2001-2010 J. Schilling
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
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>
#include <schily/standard.h>

#include <schily/utypes.h>
#include <schily/btorder.h>
#include <scg/scgcmd.h>
#include <scg/scsidefs.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "cdrecord.h"
#include "scgcheck.h"

extern	char	*buf;			/* The transfer buffer */
extern	long	bufsize;		/* The size of the transfer buffer */

extern	FILE	*logfile;
extern	char	unavail[];

LOCAL	BOOL	inq_nofail = FALSE;


EXPORT	void	sensetest	__PR((SCSI *scgp));
LOCAL	int	sensecount	__PR((SCSI *scgp, int sensecnt));
LOCAL	int	badinquiry	__PR((SCSI *scgp, caddr_t bp, int cnt, int sensecnt));
LOCAL	int	bad_unit_ready	__PR((SCSI *scgp, int sensecnt));

EXPORT void
sensetest(scgp)
	SCSI	*scgp;
{
	char	abuf[2];
	int	ret;
	int	sense_count = 0;
	BOOL	passed = TRUE;
	BOOL	needload = FALSE;

	printf(_("Ready to start test for failing command? Enter <CR> to continue: "));
	(void) chkgetline(abuf, sizeof (abuf));
	chkprint(_("**********> Testing for failed SCSI command.\n"));
/*	scgp->verbose++;*/
	fillbytes(buf, sizeof (struct scsi_inquiry), '\0');
	fillbytes((caddr_t)scgp->scmd, sizeof (*scgp->scmd), '\0');
	ret = badinquiry(scgp, buf, sizeof (struct scsi_inquiry), CCS_SENSE_LEN);
	scg_vsetup(scgp);
	scg_errfflush(scgp, logfile);
	if (ret >= 0 || !scg_cmd_err(scgp)) {
		inq_nofail = TRUE;
		chkprint(_("Inquiry did not fail.\n"));
		printf(_("This may be because the firmware in your drive is buggy.\n"));
		printf(_("If the current drive is not a CD-ROM drive please restart\n"));
		printf(_("the test utility. Otherwise remove any medium from the drive.\n"));

		printf(_("Ready to start test for failing command? Enter <CR> to continue: "));
		(void) chkgetline(abuf, sizeof (abuf));
		ret = test_unit_ready(scgp);
		scg_vsetup(scgp);
		scg_errfflush(scgp, logfile);

#ifdef	XXX
/*
 * Another try to let the command fail with "illegal field in cdb"
 */
if (0) {
	register struct scg_cmd *scmd = scgp->scmd;

	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = (caddr_t)scgp->cap;
	scmd->size = sizeof (struct scsi_capacity);
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x25;	/* Read Capacity */
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	g1_cdblen(&scmd->cdb.g1_cdb, 0); /* Full Media */

scmd->cdb.cmd_cdb[2] = 0xFF;
scmd->cdb.cmd_cdb[9] = 0xFF;

	scgp->cmdname = "read capacity";

	if (scg_cmd(scgp) < 0)
		;

}
#endif


		if (ret >= 0 || !scg_cmd_err(scgp)) {
			printf(_("Test Unit Ready did not fail.\n"));

			printf(_("Ready to eject tray? Enter <CR> to continue: "));
			flushit();
			(void) getline(abuf, sizeof (abuf));
			if (abuf[0] != 'n') {
				scsi_unload(scgp, (cdr_t *)0);
				needload = TRUE;
				ret = test_unit_ready(scgp);
				scg_vsetup(scgp);
				scg_errfflush(scgp, logfile);
			}
		}
	}
#ifdef	nonono
fprintf(logfile, "XXX\n");
	scg_vsetup(scgp);
	scg_errfflush(scgp, logfile);
#endif
/*	scgp->verbose--;*/
	if (ret < 0 &&
	    scgp->scmd->error == SCG_NO_ERROR &&
	    scgp->scmd->ux_errno != 0 &&
	    *(Uchar *)&scgp->scmd->scb != 0) {
		chkprint(_("----------> SCSI failed command test PASSED\n"));
	} else {
		if (ret >= 0) {
			chkprint(_("---------->	scg_cmd() returns not -1 (%d)\n"), ret);
		}
		if (scgp->scmd->error != SCG_NO_ERROR) {
			chkprint(_("---------->	SCSI Transport return != SCG_NO_ERROR (%d)\n"), scgp->scmd->error);
		}
		if (scgp->scmd->ux_errno == 0) {
			chkprint(_("---------->	UNIX errno set to 0\n"));
		}
		if (*(Uchar *)&scgp->scmd->scb == 0) {
			chkprint(_("---------->	SCSI status byte set to 0 (0x%x)\n"), *(Uchar *)&scgp->scmd->scb & 0xFF);
		}
		chkprint(_("----------> SCSI failed command test FAILED\n"));
	}
	if (needload)
		scsi_start_stop_unit(scgp, 1, 1, 0);

	printf(_("Ready to start test for sense data count? Enter <CR> to continue: "));
	(void) chkgetline(abuf, sizeof (abuf));
	chkprint(_("**********> Testing for SCSI sense data count.\n"));
	chkprint(_("**********> Testing if at least CCS_SENSE_LEN (%d) is supported...\n"), CCS_SENSE_LEN);
	ret = sensecount(scgp, CCS_SENSE_LEN);
	if (ret > sense_count)
		sense_count = ret;
	if (ret == CCS_SENSE_LEN) {
		chkprint(_("---------->	Wanted %d sense bytes, got it.\n"), CCS_SENSE_LEN);
	}
	if (ret != CCS_SENSE_LEN) {
		chkprint(_("---------->	Minimum standard (CCS) sense length failed\n"));
		chkprint(_("---------->	Wanted %d sense bytes, got (%d)\n"), CCS_SENSE_LEN, ret);
	}
	if (ret != scgp->scmd->sense_count) {
		passed = FALSE;
		chkprint(_("---------->	Libscg says %d sense bytes but got (%d)\n"), scgp->scmd->sense_count, ret);
	}
	chkprint(_("**********> Testing for %d bytes of sense data...\n"), SCG_MAX_SENSE);
	ret = sensecount(scgp, SCG_MAX_SENSE);
	if (ret > sense_count)
		sense_count = ret;
	if (ret == SCG_MAX_SENSE) {
		chkprint(_("---------->	Wanted %d sense bytes, got it.\n"), SCG_MAX_SENSE);
	}
	if (ret != SCG_MAX_SENSE) {
		chkprint(_("---------->	Wanted %d sense bytes, got (%d)\n"), SCG_MAX_SENSE, ret);
	}
	if (ret != scgp->scmd->sense_count) {
		passed = FALSE;
		chkprint(_("---------->	Libscg says %d sense bytes but got (%d)\n"), scgp->scmd->sense_count, ret);
	}

	chkprint(_("----------> Got a maximum of %d sense bytes\n"), sense_count);
	if (passed && sense_count >= CCS_SENSE_LEN) {
		chkprint(_("----------> SCSI sense count test PASSED\n"));
	} else {
		chkprint(_("----------> SCSI sense count test FAILED\n"));
	}
}

LOCAL int
sensecount(scgp, sensecnt)
	SCSI	*scgp;
	int	sensecnt;
{
	int	maxcnt;
	int	i;
	Uchar	*p;

	if (sensecnt > SCG_MAX_SENSE)
		sensecnt = SCG_MAX_SENSE;

/*	scgp->verbose++;*/
	scgp->silent++;
	fillbytes(buf, sizeof (struct scsi_inquiry), '\0');
	fillbytes((caddr_t)scgp->scmd, sizeof (*scgp->scmd), '\0');
	fillbytes((caddr_t)scgp->scmd->u_sense.cmd_sense, sensecnt, 0x00);
	if (inq_nofail)
		bad_unit_ready(scgp, sensecnt);
	else
		badinquiry(scgp, buf, sizeof (struct scsi_inquiry), sensecnt);
	scg_fprbytes(stdout,  _("Sense Data:"), (Uchar *)scgp->scmd->u_sense.cmd_sense, sensecnt);
	scg_fprbytes(logfile, _("Sense Data:"), (Uchar *)scgp->scmd->u_sense.cmd_sense, sensecnt);
	p = (Uchar *)scgp->scmd->u_sense.cmd_sense;
	for (i = sensecnt-1; i >= 0; i--) {
		if (p[i] != 0x00) {
			break;
		}
	}
	i++;
	maxcnt = i;
chkprint(_("---------->     Method 0x00: expected: %d reported: %d max found: %d\n"), sensecnt, scgp->scmd->sense_count, maxcnt);

	fillbytes(buf, sizeof (struct scsi_inquiry), '\0');
	fillbytes((caddr_t)scgp->scmd, sizeof (*scgp->scmd), '\0');
	fillbytes((caddr_t)scgp->scmd->u_sense.cmd_sense, sensecnt, 0xFF);
	if (inq_nofail)
		bad_unit_ready(scgp, sensecnt);
	else
		badinquiry(scgp, buf, sizeof (struct scsi_inquiry), sensecnt);
	scg_fprbytes(stdout,  _("Sense Data:"), (Uchar *)scgp->scmd->u_sense.cmd_sense, sensecnt);
	scg_fprbytes(logfile, _("Sense Data:"), (Uchar *)scgp->scmd->u_sense.cmd_sense, sensecnt);
	p = (Uchar *)scgp->scmd->u_sense.cmd_sense;
	for (i = sensecnt-1; i >= 0; i--) {
		if (p[i] != 0xFF) {
			break;
		}
	}
	i++;
	if (i > maxcnt)
		maxcnt = i;
chkprint(_("---------->     Method 0xFF: expected: %d reported: %d max found: %d\n"), sensecnt, scgp->scmd->sense_count, i);

/*	scgp->verbose--;*/
	scgp->silent--;
/*	scg_vsetup(scgp);*/
/*	scg_errfflush(scgp, logfile);*/

	return (maxcnt);
}

LOCAL int
badinquiry(scgp, bp, cnt, sensecnt)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
	int	sensecnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

/*	fillbytes(bp, cnt, '\0');*/
/*	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');*/
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G0_CDBLEN;
/*	scmd->sense_len = CCS_SENSE_LEN;*/
	scmd->sense_len = sensecnt;
	scmd->cdb.g0_cdb.cmd = SC_INQUIRY;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);
	scmd->cdb.g0_cdb.count = cnt;

scmd->cdb.cmd_cdb[3] = 0xFF;

	scgp->cmdname = "inquiry";

	if (scg_cmd(scgp) < 0)
		return (-1);
	if (scgp->verbose)
		scg_prbytes(_("Inquiry Data   :"), (Uchar *)bp, cnt - scg_getresid(scgp));
	return (0);
}

LOCAL int
bad_unit_ready(scgp, sensecnt)
	SCSI	*scgp;
	int	sensecnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

/*	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');*/
	scmd->addr = (caddr_t)0;
	scmd->size = 0;
	scmd->flags = SCG_DISRE_ENA | (scgp->silent ? SCG_SILENT:0);
	scmd->cdb_len = SC_G0_CDBLEN;
	scmd->sense_len = sensecnt;
	scmd->cdb.g0_cdb.cmd = SC_TEST_UNIT_READY;
	scmd->cdb.g0_cdb.lun = scg_lun(scgp);

	scgp->cmdname = "test unit ready";

	return (scg_cmd(scgp));
}
