/* @(#)scsi-hpux.c	1.34 09/10/09 Copyright 1997 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-hpux.c	1.34 09/10/09 Copyright 1997 J. Schilling";
#endif
/*
 *	Interface for the HP-UX generic SCSI implementation.
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

#undef	sense
#include <sys/scsi.h>

/*
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 */
LOCAL	char	_scg_trans_version[] = "scsi-hpux.c-1.34";	/* The version for this transport*/

#define	MAX_SCG		16	/* Max # of SCSI controllers */
#define	MAX_TGT		16
#define	MAX_LUN		8

struct scg_local {
	short	scgfiles[MAX_SCG][MAX_TGT][MAX_LUN];
};
#define	scglocal(p)	((struct scg_local *)((p)->local))

#ifdef	SCSI_MAXPHYS
#define	MAX_DMA_HP	SCSI_MAXPHYS
#else
#define	MAX_DMA_HP	(63*1024)	/* Check if this is not too big */
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
	__scg_help(f, "SIOC", "Generic SCSI",
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
				"/dev/rscsi/c%xt%xl%x", busno, tgt, tlun);
		f = open(devname, O_RDWR);
		if (f < 0)
			return (-1);
		scglocal(scgp)->scgfiles[busno][tgt][tlun] = f;
		return (1);
	} else {
		for (b = 0; b < MAX_SCG; b++) {
			for (t = 0; t < MAX_TGT; t++) {
/*				for (l = 0; l < MAX_LUN; l++) {*/
				for (l = 0; l < 1; l++) {
					js_snprintf(devname, sizeof (devname),
							"/dev/rscsi/c%xt%xl%x", b, t, l);
/*error("name: '%s'\n", devname);*/
					f = open(devname, O_RDWR);
					if (f >= 0) {
						scglocal(scgp)->scgfiles[b][t][l] = (short)f;
						nopen++;
					} else if (scgp->debug > 0) {
						errmsg("open '%s'\n", devname);
					}
				}
			}
		}
	}
	return (nopen);
}

LOCAL int
scgo_close(scgp)
	SCSI	*scgp;
{
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
	return (0);
}

LOCAL long
scgo_maxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	return	(MAX_DMA_HP);
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
	if (busno < 0 || busno >= MAX_SCG ||
	    tgt < 0 || tgt >= MAX_TGT ||
	    tlun < 0 || tlun >= MAX_LUN)
		return (-1);

	if (scgp->local == NULL)
		return (-1);

	return ((int)scglocal(scgp)->scgfiles[busno][tgt][tlun]);
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
	if (what == SCG_RESET_NOP)
		return (0);
	if (what != SCG_RESET_BUS) {
		errno = EINVAL;
		return (-1);
	}
	return (ioctl(scgp->fd, SIOC_RESET_BUS, 0));
}

LOCAL int
scgo_send(scgp)
	SCSI		*scgp;
{
	struct scg_cmd	*sp = scgp->scmd;
	int		ret;
	int		flags;
	struct sctl_io	sctl_io;

	if ((scgp->fd < 0) || (sp->cdb_len > sizeof (sctl_io.cdb))) {
		sp->error = SCG_FATAL;
		return (0);
	}

	fillbytes((caddr_t)&sctl_io, sizeof (sctl_io), '\0');

	flags = 0;
/*	flags = SCTL_INIT_WDTR|SCTL_INIT_SDTR;*/
	if (sp->flags & SCG_RECV_DATA)
		flags |= SCTL_READ;
	if ((sp->flags & SCG_DISRE_ENA) == 0)
		flags |= SCTL_NO_ATN;

	sctl_io.flags		= flags;

	movebytes(&sp->cdb, sctl_io.cdb, sp->cdb_len);
	sctl_io.cdb_length	= sp->cdb_len;

	sctl_io.data_length	= sp->size;
	sctl_io.data		= sp->addr;

	if (sp->timeout == 0)
		sctl_io.max_msecs = 0;
	else
		sctl_io.max_msecs = (sp->timeout * 1000) + 500;

	errno		= 0;
	sp->error	= SCG_NO_ERROR;
	sp->sense_count	= 0;
	sp->u_scb.cmd_scb[0] = 0;
	sp->resid	= 0;

	ret = ioctl(scgp->fd, SIOC_IO, &sctl_io);
	if (ret < 0) {
		sp->error = SCG_FATAL;
		sp->ux_errno = errno;
		return (ret);
	}
if (scgp->debug > 0)
error("cdb_status: %X, size: %d xfer: %d senselen: %d sensexfer: %d\n",
sctl_io.cdb_status, sctl_io.data_length, sctl_io.data_xfer, sp->sense_len, sctl_io.sense_xfer);

	if (sctl_io.cdb_status == 0 || sctl_io.cdb_status == 0x02)
		sp->resid = sp->size - sctl_io.data_xfer;

	if (sctl_io.cdb_status & SCTL_SELECT_TIMEOUT ||
			sctl_io.cdb_status & SCTL_INVALID_REQUEST) {
		sp->error = SCG_FATAL;
#ifdef	SCTL_POWERFAIL
	} else if (sctl_io.cdb_status & SCTL_POWERFAIL) {	/* Cannot select ATA */
		sp->error = SCG_FATAL;
#endif
	} else if (sctl_io.cdb_status & SCTL_INCOMPLETE) {
		sp->error = SCG_TIMEOUT;
	} else if (sctl_io.cdb_status > 0xFF) {
		errmsgno(EX_BAD, "SCSI problems: cdb_status: %X\n", sctl_io.cdb_status);

	} else if ((sctl_io.cdb_status & 0xFF) != 0) {
/*		sp->error = SCG_RETRYABLE;*/
		sp->ux_errno = EIO;

		sp->u_scb.cmd_scb[0] = sctl_io.cdb_status & 0xFF;

		sp->sense_count = sctl_io.sense_xfer;
		if (sp->sense_count > SCG_MAX_SENSE)
			sp->sense_count = SCG_MAX_SENSE;

		if (sctl_io.sense_status != S_GOOD) {
			sp->sense_count = 0;
		} else {
			movebytes(sctl_io.sense, sp->u_sense.cmd_sense, sp->sense_count);
		}

	}
	return (ret);
}
#define	sense	u_sense.Sense
