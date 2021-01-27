/* @(#)scsi-sgi.c	1.38 09/11/28 Copyright 1997 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-sgi.c	1.38 09/11/28 Copyright 1997 J. Schilling";
#endif
/*
 *	Interface for the SGI generic SCSI implementation.
 *
 *	First Hacky implementation
 *	(needed libds, did not support bus scanning and had no error checking)
 *	from "Frank van Beek" <frank@neogeo.nl>
 *
 *	Actual implementation supports all scg features.
 *
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
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

#include <dslib.h>

/*
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 */
LOCAL	char	_scg_trans_version[] = "scsi-sgi.c-1.38";	/* The version for this transport*/

#ifdef	USE_DSLIB

struct dsreq	*dsp = 0;
#define	MAX_SCG		1	/* Max # of SCSI controllers */

#else

#define	MAX_SCG		16	/* Max # of SCSI controllers */
#define	MAX_TGT		16
#define	MAX_LUN		8

struct scg_local {
	short	scgfiles[MAX_SCG][MAX_TGT][MAX_LUN];
};
#define	scglocal(p)	((struct scg_local *)((p)->local))

#endif

#define	MAX_DMA_SGI	(256*1024)	/* Check if this is not too big */


#ifndef	USE_DSLIB
LOCAL	int	scg_sendreq	__PR((SCSI *scgp, struct scg_cmd *sp, struct dsreq *dsp));
#endif


/*
 * Return version information for the low level SCSI transport code.
 * This has been introduced to make it easier to trace down problems
 * in applications.
 */
LOCAL char *
scgo_version(scgp, what)
	SCSI	*scgp;
	int	what;
{
	if (scgp != (SCSI *)0) {
		switch (what) {

		case SCG_VERSION:
			return (_scg_trans_version);
		/*
		 * If you changed this source, you are not allowed to
		 * return "schily" for the SCG_AUTHOR request.
		 */
		case SCG_AUTHOR:
			return (_scg_auth_schily);
		case SCG_SCCS_ID:
			return (__sccsid);
		}
	}
	return ((char *)0);
}

LOCAL int
scgo_help(scgp, f)
	SCSI	*scgp;
	FILE	*f;
{
	__scg_help(f, "DS", "Generic SCSI",
		"", "bus,target,lun", "1,2,0", TRUE, FALSE);
	return (0);
}

LOCAL int
scgo_open(scgp, device)
	SCSI	*scgp;
	char	*device;
{
		int	busno	= scg_scsibus(scgp);
		int	tgt	= scg_target(scgp);
		int	tlun	= scg_lun(scgp);
	register int	f;
	register int	b;
	register int	t;
	register int	l;
	register int	nopen = 0;
	char		devname[64];

	if (busno >= MAX_SCG || tgt >= MAX_TGT || tlun >= MAX_LUN) {
		errno = EINVAL;
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Illegal value for busno, target or lun '%d,%d,%d'",
				busno, tgt, tlun);
		return (-1);
	}

	if ((device != NULL && *device != '\0') || (busno == -2 && tgt == -2)) {
		errno = EINVAL;
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Open by 'devname' not supported on this OS");
		return (-1);
	}

	if (scgp->local == NULL) {
		scgp->local = malloc(sizeof (struct scg_local));
		if (scgp->local == NULL)
			return (0);

		for (b = 0; b < MAX_SCG; b++) {
			for (t = 0; t < MAX_TGT; t++) {
				for (l = 0; l < MAX_LUN; l++)
					scglocal(scgp)->scgfiles[b][t][l] = (short)-1;
			}
		}
	}

	if (busno >= 0 && tgt >= 0 && tlun >= 0) {

		js_snprintf(devname, sizeof (devname),
				"/dev/scsi/sc%dd%dl%d", busno, tgt, tlun);
#ifdef	USE_DSLIB
		dsp = dsopen(devname, O_RDWR);
		if (dsp == 0)
			return (-1);
#else
		f = open(devname, O_RDWR);
		if (f < 0) {
			if (scgp->errstr)
				js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
					"Cannot open '%s'",
					devname);
			return (-1);
		}
		scglocal(scgp)->scgfiles[busno][tgt][tlun] = f;
#endif
		return (1);
	} else {
#ifdef	USE_DSLIB
		return (-1);
#else
		for (b = 0; b < MAX_SCG; b++) {
			for (t = 0; t < MAX_TGT; t++) {
/*				for (l = 0; l < MAX_LUN; l++) {*/
				for (l = 0; l < 1; l++) {
					js_snprintf(devname, sizeof (devname),
							"/dev/scsi/sc%dd%dl%d", b, t, l);
					f = open(devname, O_RDWR);
					if (f >= 0) {
						scglocal(scgp)->scgfiles[b][t][l] = (short)f;
						nopen++;
					}
				}
			}
		}
#endif
	}
	return (nopen);
}

LOCAL int
scgo_close(scgp)
	SCSI	*scgp;
{
#ifndef	USE_DSLIB
	register int	f;
	register int	b;
	register int	t;
	register int	l;

	if (scgp->local == NULL)
		return (-1);

	for (b = 0; b < MAX_SCG; b++) {
		for (t = 0; t < MAX_TGT; t++) {
			for (l = 0; l < MAX_LUN; l++) {
				f = scglocal(scgp)->scgfiles[b][t][l];
				if (f >= 0)
					close(f);
				scglocal(scgp)->scgfiles[b][t][l] = (short)-1;
			}
		}
	}
#else
	dsclose(dsp);
#endif
	return (0);
}

LOCAL long
scgo_maxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	return (MAX_DMA_SGI);
}

LOCAL void *
scgo_getbuf(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	if (scgp->debug > 0) {
		js_fprintf((FILE *)scgp->errfile,
				"scgo_getbuf: %ld bytes\n", amt);
	}
	scgp->bufbase = valloc((size_t)(amt));
	return (scgp->bufbase);
}

LOCAL void
scgo_freebuf(scgp)
	SCSI	*scgp;
{
	if (scgp->bufbase)
		free(scgp->bufbase);
	scgp->bufbase = NULL;
}

LOCAL int
scgo_numbus(scgp)
	SCSI	*scgp;
{
	return (MAX_SCG);
}

LOCAL BOOL
scgo_havebus(scgp, busno)
	SCSI	*scgp;
	int	busno;
{
	register int	t;
	register int	l;

	if (busno < 0 || busno >= MAX_SCG)
		return (FALSE);

	if (scgp->local == NULL)
		return (FALSE);

	for (t = 0; t < MAX_TGT; t++) {
		for (l = 0; l < MAX_LUN; l++)
			if (scglocal(scgp)->scgfiles[busno][t][l] >= 0)
				return (TRUE);
	}
	return (FALSE);
}

LOCAL int
scgo_fileno(scgp, busno, tgt, tlun)
	SCSI	*scgp;
	int	busno;
	int	tgt;
	int	tlun;
{
#ifdef	USE_DSLIB
	if (dsp == NULL)
		return (-1);
	return (getfd(dsp));
#else
	if (busno < 0 || busno >= MAX_SCG ||
	    tgt < 0 || tgt >= MAX_TGT ||
	    tlun < 0 || tlun >= MAX_LUN)
		return (-1);

	if (scgp->local == NULL)
		return (-1);

	return ((int)scglocal(scgp)->scgfiles[busno][tgt][tlun]);
#endif
}

LOCAL int
scgo_initiator_id(scgp)
	SCSI	*scgp;
{
	return (-1);
}

LOCAL int
scgo_isatapi(scgp)
	SCSI	*scgp;
{
	return (FALSE);
}

LOCAL int
scgo_reset(scgp, what)
	SCSI	*scgp;
	int	what;
{
	/*
	 * Do we have a SCSI reset on SGI?
	 */
#ifdef	DS_RESET
	if (what == SCG_RESET_NOP)
		return (0);
	if (what != SCG_RESET_BUS) {
		errno = EINVAL;
		return (-1);
	}
	/*
	 * XXX Does this reset TGT or BUS ???
	 */
	return (ioctl(scgp->fd, DS_RESET, 0));
#else
	return (-1);
#endif
}

#ifndef	USE_DSLIB
LOCAL int
scg_sendreq(scgp, sp, dsp)
	SCSI		*scgp;
	struct scg_cmd	*sp;
	struct dsreq	*dsp;
{
	int	ret;
	int	retries = 4;
	Uchar	status;

/*	if ((sp->flags & SCG_CMD_RETRY) == 0)*/
/*		retries = 0;*/

	while (--retries > 0) {
		ret = ioctl(scgp->fd, DS_ENTER, dsp);
		if (ret < 0)  {
			RET(dsp) = DSRT_DEVSCSI;
			return (-1);
		}
		/*
		 * SGI implementattion botch!!!
		 * If a target does not select the bus,
		 * the return code is DSRT_TIMEOUT
		 */
		if (RET(dsp) == DSRT_TIMEOUT) {
			struct timeval to;

			to.tv_sec = TIME(dsp)/1000;
			to.tv_usec = TIME(dsp)%1000;
			__scg_times(scgp);

			if (sp->cdb.g0_cdb.cmd == SC_TEST_UNIT_READY &&
			    (scgp->cmdstop->tv_sec < to.tv_sec ||
			    (scgp->cmdstop->tv_sec == to.tv_sec &&
				scgp->cmdstop->tv_usec < to.tv_usec))) {

				RET(dsp) = DSRT_NOSEL;
				return (-1);
			}
		}
		if (RET(dsp) == DSRT_NOSEL)
			continue;		/* retry noselect 3X */

		status = STATUS(dsp);
		switch (status) {

		case 0x08:		/*  BUSY */
		case 0x18:		/*  RESERV CONFLICT */
			if (retries > 0)
				sleep(2);
			continue;
		case 0x00:		/*  GOOD */
		case 0x02:		/*  CHECK CONDITION */
		case 0x10:		/*  INTERM/GOOD */
		default:
			return (status);
		}
	}
	return (-1);	/* failed retry limit */
}
#endif

LOCAL int
scgo_send(scgp)
	SCSI		*scgp;
{
	struct scg_cmd	*sp = scgp->scmd;
	int	ret;
	int flags;
#ifndef	USE_DSLIB
	struct dsreq	ds;
	struct dsreq	*dsp = &ds;

	dsp->ds_iovbuf = 0;
	dsp->ds_iovlen = 0;
#endif

	if (scgp->fd < 0) {
		sp->error = SCG_FATAL;
		return (0);
	}

	flags = DSRQ_SENSE;
	if (sp->flags & SCG_RECV_DATA)
		flags |= DSRQ_READ;
	else if (sp->size > 0)
		flags |= DSRQ_WRITE;

	dsp->ds_flags	= flags;
	dsp->ds_link	= 0;
	dsp->ds_synch	= 0;
	dsp->ds_ret  	= 0;

	DATABUF(dsp) 	= sp->addr;
	DATALEN(dsp)	= sp->size;
	CMDBUF(dsp)	= (void *) &sp->cdb;
	CMDLEN(dsp)	= sp->cdb_len;
	SENSEBUF(dsp)	= (caddr_t)sp->u_sense.cmd_sense;
	SENSELEN(dsp)	= sizeof (sp->u_sense.cmd_sense);
	TIME(dsp)	= (sp->timeout * 1000) + 100;

	errno		= 0;
	sp->ux_errno	= 0;
	sp->sense_count	= 0;

#ifdef	USE_DSLIB
	ret = doscsireq(scgp->fd, dsp);
#else
	ret = scg_sendreq(scgp, sp, dsp);
#endif

	if (RET(dsp) != DSRT_DEVSCSI)
		ret = 0;

	if (RET(dsp)) {
		if (RET(dsp) == DSRT_SHORT) {
			sp->resid = DATALEN(dsp)- DATASENT(dsp);
		} else if (errno) {
			sp->ux_errno = errno;
		} else {
			sp->ux_errno = EIO;
		}

		sp->u_scb.cmd_scb[0] = STATUS(dsp);

		sp->sense_count = SENSESENT(dsp);
		if (sp->sense_count > SCG_MAX_SENSE)
			sp->sense_count = SCG_MAX_SENSE;

	}
	switch (RET(dsp)) {

	default:
		sp->error = SCG_RETRYABLE;	break;

	case 0:			/* OK					*/
	case DSRT_SHORT:	/* not implemented			 */
		sp->error = SCG_NO_ERROR;	break;

	case DSRT_UNIMPL:	/* not implemented			*/
	case DSRT_REVCODE:	/* software obsolete must recompile 	*/
	case DSRT_NOSEL:
		sp->u_scb.cmd_scb[0] = 0;
		sp->error = SCG_FATAL;		break;

	case DSRT_TIMEOUT:
		sp->u_scb.cmd_scb[0] = 0;
		sp->error = SCG_TIMEOUT;	break;
	}
	return (ret);
}
