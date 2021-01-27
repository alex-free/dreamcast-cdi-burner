/* @(#)scsi-osf.c	1.28 09/06/30 Copyright 1998-2009 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-osf.c	1.28 09/06/30 Copyright 1998-2009 J. Schilling";
#endif
/*
 *	Interface for Digital UNIX (OSF/1 generic SCSI implementation (/dev/cam).
 *
 *	Created out of the hacks from:
 *		Stefan Traby <stefan@sime.com> and
 *		Bruno Achauer <bruno@tk.uni-linz.ac.at>
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

#include <schily/types.h>
#include <io/common/iotypes.h>
#include <io/cam/cam.h>
#include <io/cam/uagt.h>

/*
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 */
LOCAL	char	_scg_trans_version[] = "scsi-osf.c-1.28";	/* The version for this transport*/

#define	MAX_SCG		16	/* Max # of SCSI controllers */
#define	MAX_TGT		16
#define	MAX_LUN		8

struct scg_local {
	int	scgfile;	/* Used for ioctl()	*/
	short	scgfiles[MAX_SCG][MAX_TGT][MAX_LUN];
};
#define	scglocal(p)	((struct scg_local *)((p)->local))

LOCAL	BOOL	scsi_checktgt	__PR((SCSI *scgp, int f, int busno, int tgt, int tlun));

/*
 * I don't have any documentation about CAM
 */
#define	MAX_DMA_OSF_CAM	(64*1024)

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
		int	busno	= scg_scsibus(scgp);
		int	tgt	= scg_target(scgp);
		int	tlun	= scg_lun(scgp);
	register int	b;
	register int	t;
	register int	l;

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
		scglocal(scgp)->scgfile = -1;

		for (b = 0; b < MAX_SCG; b++) {
			for (t = 0; t < MAX_TGT; t++) {
				for (l = 0; l < MAX_LUN; l++)
					scglocal(scgp)->scgfiles[b][t][l] = 0;
			}
		}
	}

	if (scglocal(scgp)->scgfile != -1)	/* multiple opens ??? */
		return (1);			/* not yet ready .... */

	if ((scglocal(scgp)->scgfile = open("/dev/cam", O_RDWR, 0)) < 0) {
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Cannot open '/dev/cam'");
		return (-1);
	}

	if (busno >= 0 && tgt >= 0 && tlun >= 0) {
		/* scsi_checktgt() ??? */
		for (l = 0; l < MAX_LUN; l++)
			scglocal(scgp)->scgfiles[b][t][l] = 1;
		return (1);
	}
	/*
	 * There seems to be no clean way to check whether
	 * a SCSI bus is present in the current system.
	 * scsi_checktgt() is used as a workaround for this problem.
	 */
	for (b = 0; b < MAX_SCG; b++) {
		for (t = 0; t < MAX_TGT; t++) {
			if (scsi_checktgt(scgp, scglocal(scgp)->scgfile, b, t, 0)) {
				for (l = 0; l < MAX_LUN; l++)
					scglocal(scgp)->scgfiles[b][t][l] = 1;
				/*
				 * Found a target on this bus.
				 * Comment the 'break' for a complete scan.
				 */
				break;
			}
		}
	}
	return (1);
}

LOCAL int
scgo_close(scgp)
	SCSI	*scgp;
{
	if (scgp->local == NULL)
		return (-1);

	if (scglocal(scgp)->scgfile >= 0)
		close(scglocal(scgp)->scgfile);
	scglocal(scgp)->scgfile = -1;
	return (0);
}

/*
 * We send a test unit ready command to the target to check whether the
 * OS is considering this target to be valid.
 * XXX Is this really needed? We should rather let the cmd fail later.
 */
LOCAL BOOL
scsi_checktgt(scgp, f, busno, tgt, tlun)
	SCSI	*scgp;
	int	f;
	int	busno;
	int	tgt;
	int	tlun;
{
	struct scg_cmd	*sp = scgp->scmd;
	struct scg_cmd	sc;
	int	ret;
	int	ofd  = scgp->fd;
	int	obus = scg_scsibus(scgp);
	int	otgt = scg_target(scgp);
	int	olun = scg_lun(scgp);

	scg_settarget(scgp, busno, tgt, tlun);
	scgp->fd = f;

	sc = *sp;
	fillbytes((caddr_t)sp, sizeof (*sp), '\0');
	sp->addr = (caddr_t)0;
	sp->size = 0;
	sp->flags = SCG_DISRE_ENA | SCG_SILENT;
	sp->cdb_len = SC_G0_CDBLEN;
	sp->sense_len = CCS_SENSE_LEN;
	sp->cdb.g0_cdb.cmd = SC_TEST_UNIT_READY;
	sp->cdb.g0_cdb.lun = scg_lun(scgp);

	scgo_send(scgp);
	scg_settarget(scgp, obus, otgt, olun);
	scgp->fd = ofd;

	if (sp->error != SCG_FATAL)
		return (TRUE);
	ret = sp->ux_errno != EINVAL;
	*sp = sc;
	return (ret);
}


LOCAL long
scgo_maxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	long maxdma = MAX_DMA_OSF_CAM;

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

	if (busno < 0 || busno >= MAX_SCG)
		return (FALSE);

	if (scgp->local == NULL)
		return (FALSE);

	for (t = 0; t < MAX_TGT; t++) {
		if (scglocal(scgp)->scgfiles[busno][t][0] != 0)
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
	if (scgp->local == NULL)
		return (-1);

	return ((busno < 0 || busno >= MAX_SCG) ? -1 : scglocal(scgp)->scgfile);
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
	errno = EINVAL;
	return (-1);
}

LOCAL int
scgo_send(scgp)
	SCSI		*scgp;
{
	struct scg_cmd	*sp = scgp->scmd;
	CCB_SCSIIO	ccb;
	UAGT_CAM_CCB	ua;
	unsigned char	*cdb;
	CCB_RELSIM	relsim;
	UAGT_CAM_CCB	relua;
	int		i;

	if (scgp->fd < 0) {
		sp->error = SCG_FATAL;
		return (0);
	}

	fillbytes(&ua, sizeof (UAGT_CAM_CCB), 0);
	fillbytes(&ccb, sizeof (CCB_SCSIIO), 0);

	ua.uagt_ccb = (CCB_HEADER *) &ccb;
	ua.uagt_ccblen = sizeof (CCB_SCSIIO);
	ccb.cam_ch.my_addr = (CCB_HEADER *) &ccb;
	ccb.cam_ch.cam_ccb_len = sizeof (CCB_SCSIIO);

	ua.uagt_snsbuf = ccb.cam_sense_ptr = sp->u_sense.cmd_sense;
	ua.uagt_snslen = ccb.cam_sense_len = AUTO_SENSE_LEN;

	cdb = (unsigned char *) ccb.cam_cdb_io.cam_cdb_bytes;

	ccb.cam_timeout = sp->timeout;

	ccb.cam_data_ptr = ua.uagt_buffer = (Uchar *) sp->addr;
	ccb.cam_dxfer_len = ua.uagt_buflen = sp->size;
	ccb.cam_ch.cam_func_code = XPT_SCSI_IO;
	ccb.cam_ch.cam_flags = 0;	/* CAM_DIS_CALLBACK; */

	if (sp->size == 0) {
		ccb.cam_data_ptr = ua.uagt_buffer = (Uchar *) NULL;
		ccb.cam_ch.cam_flags |= CAM_DIR_NONE;
	} else {
		if (sp->flags & SCG_RECV_DATA) {
			ccb.cam_ch.cam_flags |= CAM_DIR_IN;
		} else {
			ccb.cam_ch.cam_flags |= CAM_DIR_OUT;
		}
	}

	ccb.cam_cdb_len = sp->cdb_len;
	for (i = 0; i < sp->cdb_len; i++)
		cdb[i] = sp->cdb.cmd_cdb[i];

	ccb.cam_ch.cam_path_id	  = scg_scsibus(scgp);
	ccb.cam_ch.cam_target_id  = scg_target(scgp);
	ccb.cam_ch.cam_target_lun = scg_lun(scgp);

	sp->sense_count = 0;
	sp->ux_errno = 0;
	sp->error = SCG_NO_ERROR;


	if (ioctl(scgp->fd, UAGT_CAM_IO, (caddr_t) &ua) < 0) {
		sp->ux_errno = geterrno();
		sp->error = SCG_FATAL;
		if (scgp->debug > 0) {
			errmsg("ioctl(fd, UAGT_CAM_IO, dev=%d,%d,%d) failed.\n",
					scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp));
		}
		return (0);
	}
	if (scgp->debug > 0) {
		errmsgno(EX_BAD, "cam_status = 0x%.2X scsi_status = 0x%.2X dev=%d,%d,%d\n",
					ccb.cam_ch.cam_status,
					ccb.cam_scsi_status,
					scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp));
		fflush(stderr);
	}
	switch (ccb.cam_ch.cam_status & CAM_STATUS_MASK) {

	case CAM_REQ_CMP:	break;

	case CAM_SEL_TIMEOUT:	sp->error = SCG_FATAL;
				sp->ux_errno = EIO;
				break;

	case CAM_CMD_TIMEOUT:	sp->error = SCG_TIMEOUT;
				sp->ux_errno = EIO;
				break;

	default:		sp->error = SCG_RETRYABLE;
				sp->ux_errno = EIO;
				break;
	}

	sp->u_scb.cmd_scb[0] = ccb.cam_scsi_status;

	if (ccb.cam_ch.cam_status & CAM_AUTOSNS_VALID) {
		sp->sense_count = MIN(ccb.cam_sense_len - ccb.cam_sense_resid,
			SCG_MAX_SENSE);
		sp->sense_count = MIN(sp->sense_count, sp->sense_len);
		if (sp->sense_len < 0)
			sp->sense_count = 0;
	}
	sp->resid = ccb.cam_resid;


	/*
	 * this is perfectly wrong.
	 * But without this, we hang...
	 */
	if (ccb.cam_ch.cam_status & CAM_SIM_QFRZN) {
		fillbytes(&relsim, sizeof (CCB_RELSIM), 0);
		relsim.cam_ch.cam_ccb_len = sizeof (CCB_SCSIIO);
		relsim.cam_ch.cam_func_code = XPT_REL_SIMQ;
		relsim.cam_ch.cam_flags = CAM_DIR_IN | CAM_DIS_CALLBACK;
		relsim.cam_ch.cam_path_id	= scg_scsibus(scgp);
		relsim.cam_ch.cam_target_id	= scg_target(scgp);
		relsim.cam_ch.cam_target_lun	= scg_lun(scgp);

		relua.uagt_ccb = (struct ccb_header *) & relsim;	/* wrong cast */
		relua.uagt_ccblen = sizeof (relsim);
		relua.uagt_buffer = NULL;
		relua.uagt_buflen = 0;

		if (ioctl(scgp->fd, UAGT_CAM_IO, (caddr_t) & relua) < 0)
			errmsg("DEC CAM -> LMA\n");
	}
	return (0);
}
