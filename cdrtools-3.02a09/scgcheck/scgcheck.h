/* @(#)scgcheck.h	1.2 08/01/06 Copyright 2001 J. Schilling */
/*
 *	Copyright (c) 2001 J. Schilling
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

/*
 * scgcheck.c
 */
extern	void	flushit		__PR((void));
extern	int	countopen	__PR((void));
extern	int	chkprint	__PR((const char *, ...)) __printflike__(1, 2);
extern	int	chkgetline	__PR((char *, int));

/*
 * sense.c
 */
extern	void	sensetest	__PR((SCSI *scgp));

/*
 * dmaresid.c
 */
extern	void	dmaresid	__PR((SCSI *scgp));
