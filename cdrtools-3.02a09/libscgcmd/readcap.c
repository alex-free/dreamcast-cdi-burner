/* @(#)readcap.c	1.158 09/07/10 Copyright 1995-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)readcap.c	1.158 09/07/10 Copyright 1995-2009 J. Schilling";
#endif
/*
 *	SCSI command functions for read capacity
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

EXPORT	int	read_capacity	__PR((SCSI *scgp));
EXPORT	void	print_capacity	__PR((SCSI *scgp, FILE *f));


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
	fprintf(f, "Capacity: %ld Blocks = %ld kBytes = %ld MBytes = %ld prMB\n",
		(long)scgp->cap->c_baddr+1, kb, mb, prmb);
	fprintf(f, "Sectorsize: %ld Bytes\n", (long)scgp->cap->c_bsize);
}
