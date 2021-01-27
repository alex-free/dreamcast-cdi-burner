/* @(#)scsiopts.c	1.1 16/01/21 Copyright 2016 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)scsiopts.c	1.1 16/01/21 Copyright 2016 J. Schilling";
#endif
/*
 *	SCSI option parser
 *
 *	Copyright (c) 2016 J. Schilling
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
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/fcntl.h>
#include <schily/errno.h>
#include <schily/string.h>
#include <schily/time.h>

#include <schily/utypes.h>
#include <schily/btorder.h>
#include <schily/schily.h>

#include <scg/scsitransp.h>

EXPORT	int	scg_opts	__PR((SCSI *scgp, const char *optstr));
LOCAL	void	scg_optusage	__PR((void));

EXPORT int
scg_opts(scgp, optstr)
	SCSI		*scgp;
	const char	*optstr;
{
	char	*ep;
	char	*np;
	int	optlen;
	int	flags;
	BOOL	not = FALSE;

	flags = scgp->flags & SCGF_USER_FLAGS;
	while (*optstr) {
		if ((ep = strchr(optstr, ',')) != NULL) {
			optlen = ep - optstr;
			np = ep + 1;
		} else {
			optlen = strlen(optstr);
			np = (char *)optstr + optlen;
		}
		if (optstr[0] == '!') {
			optstr++;
			optlen--;
			not = TRUE;
		}
		if (strncmp(optstr, "not", optlen) == 0 ||
				strncmp(optstr, "!", optlen) == 0) {
			not = TRUE;
		} else if (strncmp(optstr, "ignore-resid", optlen) == 0) {
			flags |= SCGF_IGN_RESID;
		} else if (strncmp(optstr, "disable", optlen) == 0) {
			flags = 0;
		} else if (strncmp(optstr, "help", optlen) == 0) {
			scg_optusage();
			return (0);
		} else {
			errmsgno(EX_BAD, "Unknown option '%s'.\n", optstr);
			scg_optusage();
			return (-1);
		}
		optstr = np;
	}
	if (not)
		flags = (~flags) & SCGF_USER_FLAGS;
	scgp->flags |= flags;
	return (1);
}

LOCAL void
scg_optusage()
{
	error("scg options:\n");
	error("help		print this help\n");
	error("ignore-resid	ignore DMA residual count (needed for broken kernel drivers)\n");
}
