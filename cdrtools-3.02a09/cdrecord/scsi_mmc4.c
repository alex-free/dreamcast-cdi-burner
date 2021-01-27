/* @(#)scsi_mmc4.c	1.6 09/07/10 Copyright 2002-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)scsi_mmc4.c	1.6 09/07/10 Copyright 2002-2000 J. Schilling";
#endif
/*
 *	SCSI command functions for cdrecord
 *	covering MMC-4 level and above
 *
 *	Copyright (c) 2002-2009 J. Schilling
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

/* #define	DEBUG */
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

#include <scg/scgcmd.h>
#include <scg/scsidefs.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "scsimmc.h"
#include "cdrecord.h"

EXPORT  int	get_supported_cdrw_media_types	__PR((SCSI *scgp));

/*
 * Retrieve list of supported cd-rw media types (feature 0x37)
 */
EXPORT int
get_supported_cdrw_media_types(scgp)
    SCSI    *scgp;
{
	Uchar   cbuf[16];
	int	ret;
	fillbytes(cbuf, sizeof (cbuf), '\0');

	scgp->silent++;
	ret = get_configuration(scgp, (char *)cbuf, sizeof (cbuf), 0x37, 2);
	scgp->silent--;

	if (ret < 0)
		return (-1);

	if (cbuf[3] < 12)	/* Couldn't retrieve feature 0x37	*/
		return (-1);

	return (int)(cbuf[13]);
}
