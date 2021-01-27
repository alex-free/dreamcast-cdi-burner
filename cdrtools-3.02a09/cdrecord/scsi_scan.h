/* @(#)scsi_scan.h	1.5 06/11/26 Copyright 1997 J. Schilling */
/*
 *	Interface to scan SCSI Bus.
 *
 *	Copyright (c) 1997 J. Schilling
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

#ifndef	_SCSI_SCAN_H
#define	_SCSI_SCAN_H

extern	int	select_target		__PR((SCSI *scgp, FILE *f));
extern	int	find_target		__PR((SCSI *scgp, int type, int idx));

#endif	/* _SCSI_SCAN_H */
