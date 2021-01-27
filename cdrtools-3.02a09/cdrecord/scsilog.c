/* @(#)scsilog.c	1.25 10/12/19 Copyright 1998-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)scsilog.c	1.25 10/12/19 Copyright 1998-2010 J. Schilling";
#endif
/*
 *	SCSI log page handling
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

#include <schily/mconfig.h>

#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/btorder.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

#include <scg/scgcmd.h>
#include <scg/scsidefs.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "scsilog.h"

extern	int	lverbose;

EXPORT	int	log_sense		__PR((SCSI *scgp, caddr_t bp, int cnt, int page, int pc, int pp));
EXPORT	BOOL	has_log_page		__PR((SCSI *scgp, int page, int pc));
EXPORT	int	get_log			__PR((SCSI *scgp, caddr_t bp, int *lenp, int page, int pc, int pp));
EXPORT	void	print_logpages		__PR((SCSI *scgp));

/*
 * Currently not implemented: PPC, SP
 */
EXPORT int
log_sense(scgp, bp, cnt, page, pc, pp)
	SCSI	*scgp;
	caddr_t	bp;
	int	cnt;
	int	page;	/* Page code */
	int	pc;	/* Page control */
	int	pp;	/* Parameter Pointer */
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	fillbytes(bp, cnt, '\0');
	fillbytes((caddr_t)scmd, sizeof (*scmd), '\0');
	scmd->addr = bp;
	scmd->size = cnt;
	scmd->flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	scmd->cdb_len = SC_G1_CDBLEN;
	scmd->sense_len = CCS_SENSE_LEN;
	scmd->cdb.g1_cdb.cmd = 0x4D;
	scmd->cdb.g1_cdb.lun = scg_lun(scgp);
	scmd->cdb.g1_cdb.addr[0] = (pc << 6) | (page & 0x3f);
	i_to_2_byte(&scmd->cdb.cmd_cdb[5], pp);
	g1_cdblen(&scmd->cdb.g1_cdb, cnt);

	scgp->cmdname = "log sense";

	if (scg_cmd(scgp) < 0)
		return (-1);
	if (scgp->verbose)
		scg_prbytes(_("Log sense Data   :"), (Uchar *)bp, cnt - scg_getresid(scgp));
	return (0);
}

EXPORT BOOL
has_log_page(scgp, page, pc)
	SCSI	*scgp;
	int	page;
	int	pc;
{
	Uchar	log[0x3F + sizeof (scsi_log_hdr)];
	int	len;
	int	i;
	struct scsi_logpage_0 *sp = (struct scsi_logpage_0 *)log;

	scgp->silent++;
	if (log_sense(scgp, (caddr_t)log, sizeof (scsi_log_hdr), 0, pc, 0) < 0) {
		scgp->silent--;
		return (FALSE);
	}

	len = a_to_u_2_byte(sp->hdr.p_len);

	if (log_sense(scgp, (caddr_t)log, len + sizeof (scsi_log_hdr), 0, pc, 0) < 0) {
		scgp->silent--;
		return (FALSE);
	}
	scgp->silent--;

	len -= scg_getresid(scgp);

	for (i = 0; i < len; i++) {
		if (page == sp->p_code[i])
			return (TRUE);
	}
	return (FALSE);
}

EXPORT int
get_log(scgp, bp, lenp, page, pc, pp)
	SCSI	*scgp;
	caddr_t	bp;
	int	*lenp;
	int	page;
	int	pc;
	int	pp;
{
	Uchar		log[sizeof (scsi_log_hdr)];
	scsi_log_hdr	*hp = (scsi_log_hdr *)log;
	int		maxlen = *lenp;
	int		len;

	if (log_sense(scgp, (caddr_t)log, sizeof (scsi_log_hdr), page, pc, pp) < 0)
		return (-1);
	len = a_to_u_2_byte(hp->p_len);
	*lenp = len + sizeof (scsi_log_hdr);
	if ((len + (int)sizeof (scsi_log_hdr)) > maxlen)
		len = maxlen - sizeof (scsi_log_hdr);

	if (log_sense(scgp, bp, len + sizeof (scsi_log_hdr), page, pc, pp) < 0)
		return (-1);
	return (0);
}

EXPORT void
print_logpages(scgp)
	SCSI	*scgp;
{
	Uchar	log[0x3F + sizeof (scsi_log_hdr)];
	int	len;
	int	i;
	struct scsi_logpage_0 *sp = (struct scsi_logpage_0 *)log;

	scgp->silent++;
	if (log_sense(scgp, (caddr_t)log, sizeof (scsi_log_hdr), 0, LOG_CUMUL, 0) < 0) {
		scgp->silent--;
		return;
	}

	len = a_to_u_2_byte(sp->hdr.p_len);

	if (log_sense(scgp, (caddr_t)log, len + sizeof (scsi_log_hdr), 0, LOG_CUMUL,  0) < 0) {
		scgp->silent--;
		return;
	}
	scgp->silent--;

	len -= scg_getresid(scgp);

	printf(_("Supported log pages:"));

	for (i = 0; i < len; i++) {
		printf(" %X", sp->p_code[i]);
	}
	printf("\n");
}
