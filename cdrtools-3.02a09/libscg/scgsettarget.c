/* @(#)scgsettarget.c	1.4 08/12/22 Copyright 2000 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	const char _sccsid[] =
	"@(#)scgsettarget.c	1.4 08/12/22 Copyright 2000 J. Schilling";
#endif
/*
 *	scg Library
 *	set target SCSI address
 *
 *	This is the only place in libscg that is allowed to assign
 *	values to the scg address structure.
 *
 *	Copyright (c) 2000 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * The following exceptions apply:
 * CDDL §3.6 needs to be replaced by: "You may create a Larger Work by
 * combining Covered Software with other code if all other code is governed by
 * the terms of a license that is OSI approved (see www.opensource.org) and
 * you may distribute the Larger Work as a single product. In such a case,
 * You must make sure the requirements of this License are fulfilled for
 * the Covered Software."
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/mconfig.h>
#include <schily/standard.h>
#include <schily/schily.h>

#include <scg/scsitransp.h>

EXPORT	int	scg_settarget	__PR((SCSI *scgp, int, int, int));

EXPORT int
scg_settarget(scgp, busno, tgt, tlun)
	SCSI	*scgp;
	int	busno;
	int	tgt;
	int	tlun;
{
	int fd = -1;

	if (scgp->ops != NULL)
		fd = SCGO_FILENO(scgp, busno, tgt, tlun);
	scgp->fd = fd;
	scg_scsibus(scgp) = busno;
	scg_target(scgp)  = tgt;
	scg_lun(scgp)	  = tlun;
	return (fd);
}
