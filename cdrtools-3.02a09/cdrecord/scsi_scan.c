/* @(#)scsi_scan.c	1.38 17/08/06 Copyright 1997-2017 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)scsi_scan.c	1.38 17/08/06 Copyright 1997-2017 J. Schilling";
#endif
/*
 *	Scan SCSI Bus.
 *	Stolen from sformat. Need a more general form to
 *	re-use it in sformat too.
 *
 *	Copyright (c) 1997-2017 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/standard.h>
#include <schily/btorder.h>
#include <schily/errno.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

#include <scg/scgcmd.h>
#include <scg/scsidefs.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "scsi_scan.h"
#include "cdrecord.h"

LOCAL	void	print_product		__PR((FILE *f, struct scsi_inquiry *ip));
EXPORT	int	select_target		__PR((SCSI *scgp, FILE *f));
EXPORT	int	find_target		__PR((SCSI *scgp, int type, int idx));
LOCAL	int	_select_target		__PR((SCSI *scgp, FILE *f, int type, int idx));
#ifdef	__ready__
LOCAL	int	select_unit		__PR((SCSI *scgp, FILE *f));
#endif

LOCAL void
print_product(f, ip)
	FILE			*f;
	struct	scsi_inquiry	*ip;
{
	fprintf(f, "'%.8s' ", ip->inq_vendor_info);
	fprintf(f, "'%.16s' ", ip->inq_prod_ident);
	fprintf(f, "'%.4s' ", ip->inq_prod_revision);
	if (ip->add_len < 31) {
		fprintf(f, "NON CCS ");
	}
	scg_fprintdev(f, ip);
}

EXPORT int
select_target(scgp, f)
	SCSI	*scgp;
	FILE	*f;
{
	return (_select_target(scgp, f, -1, -1));
}

EXPORT int
find_target(scgp, type, idx)
	SCSI	*scgp;
	int	type;
	int	idx;
{
	return (_select_target(scgp, (FILE *)NULL, type, idx));
}

LOCAL int
_select_target(scgp, f, type, idx)
	SCSI	*scgp;
	FILE	*f;
	int	type;
	int	idx;
{
	int	initiator;
	int	cscsibus = scg_scsibus(scgp);
	int	ctarget  = scg_target(scgp);
	int	clun	 = scg_lun(scgp);
	int	numbus	 = scg_numbus(scgp);
	int	n;
	int	low	= -1;
	int	high	= -1;
	int	amt	= -1;
	int	bus;
	int	tgt;
	int	lun = 0;
	int	err;
	BOOL	have_tgt;

	if (numbus < 0)
		numbus = 1024;
	scgp->silent++;

	for (bus = 0; bus < numbus; bus++) {
		scg_settarget(scgp, bus, 0, 0);

		if (!scg_havebus(scgp, bus))
			continue;

		initiator = scg_initiator_id(scgp);
		if (f)
			fprintf(f, "scsibus%d:\n", bus);

		for (tgt = 0; tgt < 16; tgt++) {
			n = bus*100 + tgt;

			scg_settarget(scgp, bus, tgt, lun);
			seterrno(0);
			have_tgt = unit_ready(scgp) || scgp->scmd->error != SCG_FATAL;
			err = geterrno();
			if (err == EPERM || err == EACCES)
				return (-1);

			if (!have_tgt && tgt > 7) {
				if (scgp->scmd->ux_errno == EINVAL)
					break;
				continue;
			}
			if (f) {
#ifdef	FMT
				if (print_disknames(bus, tgt, -1) < 8)
					fprintf(f, "\t");
				else
					fprintf(f, " ");
#else
				fprintf(f, "\t");
#endif
				if (fprintf(f, "%d,%d,%d", bus, tgt, lun) < 8)
					fprintf(f, "\t");
				else
					fprintf(f, " ");
				fprintf(f, "%3d) ", n);
			}
			if (tgt == initiator) {
				if (f)
					fprintf(f, "HOST ADAPTOR\n");
				continue;
			}
			if (!have_tgt) {
				/*
				 * Hack: fd -> -2 means no access
				 */
				if (f) {
					fprintf(f, "%c\n",
						scgp->fd == -2 ? '?':'*');
				}
				continue;
			}
			if (low < 0)
				low = n;
			high = n;

			getdev(scgp, FALSE);
			if (f)
				print_product(f, scgp->inq);
			if (type >= 0 && scgp->inq->type == type) {
				amt++;
				if (amt == 0)	/* amt starts at -1 */
					amt++;
				if (amt == idx) {
					scgp->silent--;
					return (amt);
				}
			} else if (type < 0) {
				amt++;
			}
			if (amt == 0)	/* amt starts at -1 */
				amt++;
		}
	}
	scgp->silent--;

	if (low < 0) {
		errmsgno(EX_BAD, _("No target found.\n"));
		return (0);
	}
	n = -1;
#ifdef	FMT
	getint(_("Select target"), &n, low, high);
	bus = n/100;
	tgt = n%100;
	scg_settarget(scgp, bus, tgt, lun);
	return (select_unit(scgp));
#else
	scg_settarget(scgp, cscsibus, ctarget, clun);
	return (amt);
#endif
}

#ifdef	__ready__
LOCAL int
select_unit(scgp, f)
	SCSI	*scgp;
	FILE	*f;
{
	int	initiator;
	int	clun	= scg_lun(scgp);
	int	low	= -1;
	int	high	= -1;
	int	lun;

	scgp->silent++;

	fprintf(f, _("scsibus%d target %d:\n"), scg_scsibus(scgp), scg_target(scgp));

	initiator = scg_initiator_id(scgp);
	for (lun = 0; lun < 8; lun++) {

#ifdef	FMT
		if (print_disknames(scg_scsibus(scgp), scg_target(scgp), lun) < 8)
			fprintf(f, "\t");
		else
			fprintf(f, " ");
#else
		fprintf(f, "\t");
#endif
		if (fprintf(f, "%d,%d,%d", scg_scsibus(scgp), scg_target(scgp), lun) < 8)
			fprintf(f, "\t");
		else
			fprintf(f, " ");
		fprintf(f, "%3d) ", lun);
		if (scg_target(scgp) == initiator) {
			fprintf(f, "HOST ADAPTOR\n");
			continue;
		}
		scg_settarget(scgp, scg_scsibus(scgp), scg_target(scgp), lun);
		if (!unit_ready(scgp) && scgp->scmd->error == SCG_FATAL) {
			fprintf(f, "*\n");
			continue;
		}
		if (unit_ready(scgp)) {
			/* non extended sense illegal lun */
			if (scgp->scmd->sense.code == 0x25) {
				fprintf(f, "BAD UNIT\n");
				continue;
			}
		}
		if (low < 0)
			low = lun;
		high = lun;

		getdev(scgp, FALSE);
		print_product(f, scgp->inq);
	}
	scgp->silent--;

	if (low < 0) {
		errmsgno(EX_BAD, _("No lun found.\n"));
		return (0);
	}
	lun = -1;
#ifdef	FMT
	getint(_("Select lun"), &lun, low, high);
	scg_settarget(scgp, scg_scsibus(scgp), scg_target(scgp), lun);
	format_one(scgp);
	return (1);
#else
	scg_settarget(scgp, scg_scsibus(scgp), scg_target(scgp), clun);
	return (1);
#endif
}
#endif	/* __ready__ */
