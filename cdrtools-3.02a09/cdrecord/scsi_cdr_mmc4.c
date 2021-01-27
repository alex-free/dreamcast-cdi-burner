/* @(#)scsi_cdr_mmc4.c	1.7 10/12/19 Copyright 1995-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)scsi_cdr_mmc4.c	1.7 10/12/19 Copyright 1995-2010 J. Schilling";
#endif
/*
 *	SCSI command functions for cdrecord
 *	covering MMC-4
 *
 *	Copyright (c) 1995-2010 J. Schilling
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

EXPORT	void	print_capabilities_mmc4 __PR((SCSI *scgp));

#define	DOES(what, flag)	printf(_("  Does %s%s\n"), flag?"":_("not "), what)


EXPORT void
print_capabilities_mmc4(scgp)
	SCSI	*scgp;
{
	int	cdrw_types;

	if (scgp->inq->type != INQ_ROMD)
		return;

	cdrw_types = get_supported_cdrw_media_types(scgp);
	if (cdrw_types != -1) {
		printf(_("\nSupported CD-RW media types according to MMC-4 feature 0x37:\n"));
		DOES(_("write multi speed       CD-RW media"), (cdrw_types & CDR_CDRW_MULTI));
		DOES(_("write high  speed       CD-RW media"), (cdrw_types & CDR_CDRW_HIGH));
		DOES(_("write ultra high speed  CD-RW media"), (cdrw_types & CDR_CDRW_ULTRA));
		DOES(_("write ultra high speed+ CD-RW media"), (cdrw_types & CDR_CDRW_ULTRAP));
	}
}
