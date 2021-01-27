/* @(#)ready.c	1.158 09/07/10 Copyright 1995-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)ready.c	1.158 09/07/10 Copyright 1995-2009 J. Schilling";
#endif
/*
 *	SCSI command functions related to test unit ready
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

#include <scg/scgcmd.h>
#include <scg/scsidefs.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "libscgcmd.h"

EXPORT	BOOL	unit_ready	__PR((SCSI *scgp));
EXPORT	BOOL	wait_unit_ready	__PR((SCSI *scgp, int secs));
EXPORT	int	test_unit_ready	__PR((SCSI *scgp));

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
