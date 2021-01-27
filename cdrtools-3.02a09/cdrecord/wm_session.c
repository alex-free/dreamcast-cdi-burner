/* @(#)wm_session.c	1.8 09/07/10 Copyright 1995, 1997, 2001-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)wm_session.c	1.8 09/07/10 Copyright 1995, 1997, 2001-2009 J. Schilling";
#endif
/*
 *	CDR write method abtraction layer
 *	session at once / disk at once writing intercace routines
 *
 *	Copyright (c) 1995, 1997, 2001-2009 J. Schilling
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
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/standard.h>
#include <schily/utypes.h>

#include <scg/scsitransp.h>
#include "cdrecord.h"

extern	int	debug;
extern	int	verbose;
extern	int	lverbose;

extern	char	*buf;			/* The transfer buffer */

EXPORT	int	write_session_data __PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
