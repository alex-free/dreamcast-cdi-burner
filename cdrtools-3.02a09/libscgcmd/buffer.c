/* @(#)buffer.c	1.158 09/07/10 Copyright 1995-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)buffer.c	1.158 09/07/10 Copyright 1995-2009 J. Schilling";
#endif
/*
 *	SCSI command functions for read_buffer/write_buffer
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

EXPORT	int	read_buffer	__PR((SCSI *scgp, caddr_t bp, int cnt, int mode));
EXPORT	int	write_buffer	__PR((SCSI *scgp, char *buffer, long length, int mode, int bufferid, long offset));


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
