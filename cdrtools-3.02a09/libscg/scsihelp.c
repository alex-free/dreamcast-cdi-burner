/* @(#)scsihelp.c	1.7 09/07/11 Copyright 2002-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)scsihelp.c	1.7 09/07/11 Copyright 2002-2009 J. Schilling";
#endif
/*
 *	scg Library
 *	Help subsystem
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

#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/schily.h>

#include <scg/scsitransp.h>

EXPORT	void	__scg_help	__PR((FILE *f, char *name, char *tcomment,
					char *tind,
					char *tspec,
					char *texample,
					BOOL mayscan,
					BOOL bydev));

EXPORT void
__scg_help(f, name, tcomment, tind, tspec, texample, mayscan, bydev)
	FILE	*f;
	char	*name;
	char	*tcomment;
	char	*tind;
	char	*tspec;
	char	*texample;
	BOOL	mayscan;
	BOOL	bydev;
{
	fprintf(f, "\nTransport name:		%s\n", name);
	fprintf(f, "Transport descr.:	%s\n", tcomment);
	fprintf(f, "Transp. layer ind.:	%s\n", tind);
	fprintf(f, "Target specifier:	%s\n", tspec);
	fprintf(f, "Target example:		%s\n", texample);
	fprintf(f, "SCSI Bus scanning:	%ssupported\n", mayscan? "":"not ");
	fprintf(f, "Open via UNIX device:	%ssupported\n", bydev? "":"not ");
}
