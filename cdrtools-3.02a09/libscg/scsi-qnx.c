/* @(#)scsi-qnx.c	1.5 09/06/30 Copyright 1998-2009 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-qnx.c	1.5 09/06/30 Copyright 1998-2009 J. Schilling";
#endif
/*
 *	Interface for QNX (Neutrino generic SCSI implementation).
 *	First version adopted from the OSF-1 version by
 *	Kevin Chiles <kchiles@qnx.com>
 *
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 *
 *	Copyright (c) 1998-2009 J. Schilling
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

#include <schily/mman.h>
#include <schily/types.h>
#include <sys/dcmd_cam.h>
#include <sys/cam_device.h>

/*
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 */
LOCAL	char	_scg_trans_version[] = "scsi-qnx.c-1.5";	/* The version for this transport*/

#define	MAX_SCG		16	/* Max # of SCSI controllers */
#define	MAX_TGT		16
#define	MAX_LUN		8

struct scg_local {
	int		fd;
};

#define	scglocal(p)	((struct scg_local *)((p)->local))
#define	QNX_CAM_MAX_DMA	(32*1024)

#ifndef	AUTO_SENSE_LEN
#	define	AUTO_SENSE_LEN	32	/* SCG_MAX_SENSE */
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
			return ("Initial Version adopted from OSF-1 by QNX-people");
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
	__scg_help(f, "CAM", "Generic transport independent SCSI (Common Access Method)",
		"", "bus,target,lun", "1,2,0", TRUE, FALSE);
	return (0);
}

LOCAL int
scgo_open(scgp, device)
	SCSI	*scgp;
	char	*device;
{
	int fd;

	if (device == NULL || *device == '\0') {
		errno = EINVAL;
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"'devname' must be specified on this OS");
		return (-1);
	}

	if (scgp->local == NULL) {
		scgp->local = malloc(sizeof (struct scg_local));
		if (scgp->local == NULL)
			return (0);
		scglocal(scgp)->fd = -1;
	}

	if (scglocal(scgp)->fd != -1)	/* multiple open? */
		return (1);

	if ((scglocal(scgp)->fd = open(device, O_RDONLY, 0)) < 0) {
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Cannot open '%s'", device);
		return (-1);
	}

	scg_settarget(scgp, 0, 0, 0);

	return (1);
}

LOCAL int
scgo_close(scgp)
	SCSI	*scgp;
{
	if (scgp->local == NULL)
		return (-1);

	if (scglocal(scgp)->fd >= 0)
		close(scglocal(scgp)->fd);
	scglocal(scgp)->fd = -1;
	return (0);
}

LOCAL long
scgo_maxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	long maxdma = QNX_CAM_MAX_DMA;

	return (maxdma);
}

LOCAL void *
scgo_getbuf(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	void	*addr;

	if (scgp->debug > 0) {
		js_fprintf((FILE *)scgp->errfile, "scgo_getbuf: %ld bytes\n", amt);
	}

	if ((addr = mmap(NULL, amt, PROT_READ | PROT_WRITE | PROT_NOCACHE,
						MAP_ANON | MAP_PHYS | MAP_NOX64K, NOFD, 0)) == MAP_FAILED) {
		return (NULL);
	}

	scgp->bufbase = addr;
	return (addr);
}

LOCAL void
scgo_freebuf(scgp)
	SCSI	*scgp;
{
	if (scgp->bufbase)
		munmap(scgp->bufbase, QNX_CAM_MAX_DMA);
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
	return (FALSE);
}


LOCAL int
scgo_fileno(scgp, busno, tgt, tlun)
	SCSI	*scgp;
	int	busno;
	int	tgt;
	int	tlun;
{
	if (scgp->local == NULL)
		return (-1);

	return ((busno < 0 || busno >= MAX_SCG) ? -1 : scglocal(scgp)->fd);
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
	cam_devinfo_t	cinfo;

	if (devctl(scgp->fd, DCMD_CAM_DEVINFO, &cinfo, sizeof (cinfo), NULL) != EOK) {
		return (TRUE);		/* default to ATAPI */
	}
	return ((cinfo.flags & DEV_ATAPI) ? TRUE : FALSE);
}

LOCAL int
scgo_reset(scgp, what)
	SCSI	*scgp;
	int	what;
{
	errno = EINVAL;
	return (-1);
}

LOCAL int
scgo_send(scgp)
	SCSI		*scgp;
{
	int		i;
	struct scg_cmd	*sp;
	int		icnt;
	iov_t   	iov[3];
	CAM_PASS_THRU	cpt;

	icnt	= 1;
	sp	= scgp->scmd;
	if (scgp->fd < 0) {
		sp->error = SCG_FATAL;
		return (0);
	}

	memset(&cpt, 0, sizeof (cpt));

	sp->sense_count	= 0;
	sp->ux_errno	= 0;
	sp->error	= SCG_NO_ERROR;
	cpt.cam_timeout	= sp->timeout;
	cpt.cam_cdb_len = sp->cdb_len;
	memcpy(cpt.cam_cdb, sp->cdb.cmd_cdb, sp->cdb_len);

	if (sp->sense_len != -1) {
		cpt.cam_sense_len	= sp->sense_len;
		cpt.cam_sense_ptr	= sizeof (cpt);	/* XXX Offset from start of struct to data ??? */
		icnt++;
	} else {
		cpt.cam_flags |= CAM_DIS_AUTOSENSE;
	}

	if (cpt.cam_dxfer_len = sp->size) {
		icnt++;
		cpt.cam_data_ptr	= (paddr_t)sizeof (cpt) + cpt.cam_sense_len;
		if (sp->flags & SCG_RECV_DATA) {
			cpt.cam_flags |= CAM_DIR_IN;
		} else {
			cpt.cam_flags |= CAM_DIR_OUT;
		}
	} else {
		cpt.cam_flags |= CAM_DIR_NONE;
	}

	SETIOV(&iov[0], &cpt, sizeof (cpt));
	SETIOV(&iov[1], sp->u_sense.cmd_sense, cpt.cam_sense_len);
	SETIOV(&iov[2], sp->addr, sp->size);
	if (devctlv(scglocal(scgp)->fd, DCMD_CAM_PASS_THRU, icnt, icnt, iov, iov, NULL)) {
		sp->ux_errno = geterrno();
		sp->error = SCG_FATAL;
		if (scgp->debug > 0) {
			errmsg("cam_io failed\n");
		}
		return (0);
	}

	sp->resid		= cpt.cam_resid;
	sp->u_scb.cmd_scb[0]	= cpt.cam_scsi_status;

	switch (cpt.cam_status & CAM_STATUS_MASK) {
		case CAM_REQ_CMP:
			break;

		case CAM_SEL_TIMEOUT:
			sp->error	= SCG_FATAL;
			sp->ux_errno	= EIO;
			break;

		case CAM_CMD_TIMEOUT:
			sp->error	= SCG_TIMEOUT;
			sp->ux_errno	= EIO;
			break;

		default:
			sp->error	= SCG_RETRYABLE;
			sp->ux_errno	= EIO;
			break;
	}

	if (cpt.cam_status & CAM_AUTOSNS_VALID) {
		sp->sense_count = min(cpt.cam_sense_len - cpt.cam_sense_resid,
							SCG_MAX_SENSE);
		sp->sense_count = min(sp->sense_count, sp->sense_len);
		if (sp->sense_len < 0)
			sp->sense_count = 0;
	}

	return (0);
}
