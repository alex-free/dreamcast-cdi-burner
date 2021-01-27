/* @(#)scsi-bsd-os.c	1.29 06/11/26 Copyright 1997 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-bsd-os.c	1.29 06/11/26 Copyright 1997 J. Schilling";
#endif
/*
 *	Interface for the BSD/OS user-land raw SCSI implementation.
 *
 *	This is a hack, that tries to emulate the functionality
 *	of the scg driver.
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

#define	scsi_sense	bsdi_scsi_sense
#define	scsi_inquiry	bsdi_scsi_inquiry

/*
 * Must use -I/sys...
 * The next two files are in /sys/dev/scsi
 */
#include <dev/scsi/scsi.h>
#include <dev/scsi/scsi_ioctl.h>

/*
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 */
LOCAL	char	_scg_trans_version[] = "scsi-bsd-os.c-1.29";	/* The version for this transport*/

#define	MAX_SCG		16	/* Max # of SCSI controllers */
#define	MAX_TGT		16
#define	MAX_LUN		8

struct scg_local {
	short	scgfiles[MAX_SCG][MAX_TGT][MAX_LUN];
};
#define	scglocal(p)	((struct scg_local *)((p)->local))

#include <machine/param.h>

#define	MAX_DMA_BSDI	MAXPHYS		/* More makes problems */


LOCAL	BOOL	scg_setup	__PR((SCSI *scgp, int f, int busno, int tgt, int tlun));

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
	__scg_help(f, "SCSIRAWCDB", "Generic SCSI for devices known by BSDi",
		"", "devname:@,lun", "/dev/rsr0a:@,0", FALSE, TRUE);
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

	if ((device != NULL && *device != '\0') || (busno == -2 && tgt == -2))
		goto openbydev;

	if (busno >= 0 && tgt >= 0 && tlun >= 0) {

		js_snprintf(devname, sizeof (devname),
					"/dev/su%d-%d-%d", busno, tgt, tlun);
		f = open(devname, O_RDWR|O_NONBLOCK);
		if (f < 0) {
			goto openbydev;
		}
		scglocal(scgp)->scgfiles[busno][tgt][tlun] = f;
		return (1);

	} else for (b = 0; b < MAX_SCG; b++) {
		for (t = 0; t < MAX_TGT; t++) {
			for (l = 0; l < MAX_LUN; l++) {
				js_snprintf(devname, sizeof (devname),
						"/dev/su%d-%d-%d", b, t, l);
				f = open(devname, O_RDWR|O_NONBLOCK);
/*				error("open (%s) = %d\n", devname, f);*/

				if (f < 0) {
					if (errno != ENOENT &&
					    errno != ENXIO &&
					    errno != ENODEV) {
						if (scgp->errstr)
							js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
								"Cannot open '%s'",
								devname);
						return (0);
					}
				} else {
					if (scg_setup(scgp, f, b, t, l))
						nopen++;
				}
			}
		}
	}
	/*
	 * Could not open /dev/su-* or got dev=devname:b,l,l / dev=devname:@,l
	 * We do the apropriate tests and try our best.
	 */
openbydev:
	if (nopen == 0) {
		if (device == NULL || device[0] == '\0')
			return (0);
		f = open(device, O_RDWR|O_NONBLOCK);
		if (f < 0) {
			if (scgp->errstr)
				js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
					"Cannot open '%s'",
					device);
			return (0);
		}
		if (tlun == -2) {	/* If 'lun' is not known, we reject */
			close(f);
			errno = EINVAL;
			return (0);
		}
		busno = 0;		/* use fake number, we cannot get it */
		tgt   = 0;		/* use fake number, we cannot get it */
		scg_settarget(scgp, busno, tgt, tlun);
		/* 'lun' has been specified on command line */
		if (scg_setup(scgp, f, busno, tgt, tlun))
			nopen++;
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

LOCAL BOOL
scg_setup(scgp, f, busno, tgt, tlun)
	SCSI	*scgp;
	int	f;
	int	busno;
	int	tgt;
	int	tlun;
{
	int	Bus;
	int	Target;
	int	Lun;
	BOOL	onetarget = FALSE;

	if (scg_scsibus(scgp) >= 0 && scg_target(scgp) >= 0 && scg_lun(scgp) >= 0)
		onetarget = TRUE;

	/*
	 * Unfortunately there is no way to get the right values from kernel.
	 */
	Bus	= busno;
	Target	= tgt;
	Lun	= tlun;

	if (scgp->debug > 0) {
		js_fprintf((FILE *)scgp->errfile,
			"Bus: %d Target: %d Lun: %d\n", Bus, Target, Lun);
	}

	if (Bus >= MAX_SCG || Target >= MAX_TGT || Lun >= MAX_LUN) {
		close(f);
		return (FALSE);
	}

	if (scglocal(scgp)->scgfiles[Bus][Target][Lun] == (short)-1)
		scglocal(scgp)->scgfiles[Bus][Target][Lun] = (short)f;

	if (onetarget) {
		if (Bus == busno && Target == tgt && Lun == tlun) {
			return (TRUE);
		} else {
			scglocal(scgp)->scgfiles[Bus][Target][Lun] = (short)-1;
			close(f);
		}
	}
	return (FALSE);
}

LOCAL long
scgo_maxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	long maxdma = MAX_DMA_BSDI;

	return (maxdma);
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
	scgp->bufbase = malloc((size_t)(amt));
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
	/*
	 * Cannot reset on BSD/OS
	 */
	errno = EINVAL;
	return (-1);
}

LOCAL int
scgo_send(scgp)
	SCSI		*scgp;
{
	struct scg_cmd	*sp = scgp->scmd;
	scsi_user_cdb_t	suc;
	int		ret = 0;

/*	js_fprintf((FILE *)scgp->errfile, "f: %d\n", f);*/
	if (scgp->fd < 0) {
		sp->error = SCG_FATAL;
		return (0);
	}

	/* Zero the structure... */
	fillbytes(&suc, sizeof (suc), '\0');

	/* Read or write? */
	if (sp->flags & SCG_RECV_DATA) {
		suc.suc_flags |= SUC_READ;
	} else if (sp->size > 0) {
		suc.suc_flags |= SUC_WRITE;
	}

	suc.suc_timeout = sp->timeout;

	suc.suc_cdblen = sp->cdb_len;
	movebytes(sp->cdb.cmd_cdb, suc.suc_cdb, suc.suc_cdblen);

	suc.suc_datalen = sp->size;
	suc.suc_data = sp->addr;

	if (ioctl(scgp->fd, SCSIRAWCDB, &suc) < 0) {
		ret  = -1;
		sp->ux_errno = geterrno();
		if (sp->ux_errno != ENOTTY)
			ret = 0;
	} else {
		sp->ux_errno = 0;
		if (suc.suc_sus.sus_status != STS_GOOD)
			sp->ux_errno = EIO;
	}
	fillbytes(&sp->scb, sizeof (sp->scb), '\0');
	fillbytes(&sp->u_sense.cmd_sense, sizeof (sp->u_sense.cmd_sense), '\0');
#if 0
	/*
	 * Unfortunalety, BSD/OS has no idea of DMA residual count.
	 */
	sp->resid = req.datalen - req.datalen_used;
	sp->sense_count = req.senselen_used;
#else
	sp->resid = 0;
	sp->sense_count = sizeof (suc.suc_sus.sus_sense);
#endif
	if (sp->sense_count > SCG_MAX_SENSE)
		sp->sense_count = SCG_MAX_SENSE;
	movebytes(suc.suc_sus.sus_sense, sp->u_sense.cmd_sense, sp->sense_count);
	sp->u_scb.cmd_scb[0] = suc.suc_sus.sus_status;

	switch (suc.suc_sus.sus_status) {

	case STS_GOOD:
				sp->error = SCG_NO_ERROR;	break;
	case STS_CMD_TERMINATED:sp->error = SCG_TIMEOUT;	break;
	case STS_BUSY:		sp->error = SCG_RETRYABLE;	break;
	case STS_CHECKCOND:	sp->error = SCG_RETRYABLE;	break;
	case STS_QUEUE_FULL:	sp->error = SCG_RETRYABLE;	break;
	default:		sp->error = SCG_FATAL;		break;
	}

	return (ret);
}

#define	sense	u_sense.Sense

#undef scsi_sense
#undef scsi_inquiry
