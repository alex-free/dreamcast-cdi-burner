/* @(#)scgops.h	1.6 06/11/26 Copyright 2000 J. Schilling */
/*
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

#ifndef	_SCG_SCGOPS_H
#define	_SCG_SCGOPS_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct scg_ops {
	int	(*scgo_send)		__PR((SCSI *scgp));

	char *	(*scgo_version)		__PR((SCSI *scgp, int what));
#ifdef	EOF	/* stdio.h has been included */
	int	(*scgo_help)		__PR((SCSI *scgp, FILE *f));
#else
	int	(*scgo_help)		__PR((SCSI *scgp, void *f));
#endif
	int	(*scgo_open)		__PR((SCSI *scgp, char *device));
	int	(*scgo_close)		__PR((SCSI *scgp));
	long	(*scgo_maxdma)		__PR((SCSI *scgp, long amt));
	void *	(*scgo_getbuf)		__PR((SCSI *scgp, long amt));
	void	(*scgo_freebuf)		__PR((SCSI *scgp));


	int	(*scgo_numbus)		__PR((SCSI *scgp));
	BOOL	(*scgo_havebus)		__PR((SCSI *scgp, int busno));
	int	(*scgo_fileno)		__PR((SCSI *scgp, int busno, int tgt, int tlun));
	int	(*scgo_initiator_id)	__PR((SCSI *scgp));
	int	(*scgo_isatapi)		__PR((SCSI *scgp));
	int	(*scgo_reset)		__PR((SCSI *scgp, int what));
} scg_ops_t;

#define	SCGO_SEND(scgp)				(*(scgp)->ops->scgo_send)(scgp)
#define	SCGO_VERSION(scgp, what)		(*(scgp)->ops->scgo_version)(scgp, what)
#define	SCGO_HELP(scgp, f)			(*(scgp)->ops->scgo_help)(scgp, f)
#define	SCGO_OPEN(scgp, device)			(*(scgp)->ops->scgo_open)(scgp, device)
#define	SCGO_CLOSE(scgp)			(*(scgp)->ops->scgo_close)(scgp)
#define	SCGO_MAXDMA(scgp, amt)			(*(scgp)->ops->scgo_maxdma)(scgp, amt)
#define	SCGO_GETBUF(scgp, amt)			(*(scgp)->ops->scgo_getbuf)(scgp, amt)
#define	SCGO_FREEBUF(scgp)			(*(scgp)->ops->scgo_freebuf)(scgp)
#define	SCGO_NUMBUS(scgp)			(*(scgp)->ops->scgo_numbus)(scgp)
#define	SCGO_HAVEBUS(scgp, busno)		(*(scgp)->ops->scgo_havebus)(scgp, busno)
#define	SCGO_FILENO(scgp, busno, tgt, tlun)	(*(scgp)->ops->scgo_fileno)(scgp, busno, tgt, tlun)
#define	SCGO_INITIATOR_ID(scgp)			(*(scgp)->ops->scgo_initiator_id)(scgp)
#define	SCGO_ISATAPI(scgp)			(*(scgp)->ops->scgo_isatapi)(scgp)
#define	SCGO_RESET(scgp, what)			(*(scgp)->ops->scgo_reset)(scgp, what)

#ifdef	__cplusplus
}
#endif

#endif	/* _SCG_SCGOPS_H */
