/* @(#)scsi-vms.c	1.36 17/07/17 Copyright 1997,2017 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-vms.c	1.36 17/07/17 Copyright 1997,2017 J. Schilling";
#endif
/*
 *	Interface for the VMS generic SCSI implementation.
 *
 *	The idea for an elegant mapping to VMS device dontroller names
 *	is from Chip Dancy Chip.Dancy@hp.com. This allows up to
 *	26 IDE controllers (DQ[A-Z][0-1]).
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
 *	Copyright (c) 1997,2017 J. Schilling
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

#include <iodef.h>
#include <ssdef.h>
#include <descrip.h>
#include <starlet.h>
#include <string.h>
#include <LIB$ROUTINES.H>

/*
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 */
LOCAL	char	_scg_trans_version[] = "scsi-vms.c-1.36";	/* The version for this transport*/

#define	VMS_MAX_DK	4		/* DK[A-D] VMS device controllers */
#define	VMS_MAX_GK	4		/* GK[A-D] VMS device controllers */
#define	VMS_MAX_DQ	26		/* DQ[A-Z] VMS device controllers */

#define	VMS_DKRANGE_MAX	VMS_MAX_DK
#define	VMS_GKRANGE_MAX	(VMS_DKRANGE_MAX + VMS_MAX_GK)
#define	VMS_DQRANGE_MAX	(VMS_GKRANGE_MAX + VMS_MAX_DQ)

#define	MAX_SCG 	VMS_DQRANGE_MAX	/* Max # of SCSI controllers */
#define	MAX_TGT 	16
#define	MAX_LUN 	8

#define	MAX_DMA_VMS	(63*1024)	/* Check if this is not too big */
#define	MAX_PHSTMO_VMS	300
#define	MAX_DSCTMO_VMS	((64*1024)-1)	/* max value for OpenVMS/AXP 7.1 ehh*/

/*
 * Define a mapping from the scsi busno to the three character
 * VMS device controller.
 * The valid busno values are broken into three ranges, one for each of
 * the three supported devices: dk, gk, and dq.
 * The vmschar[] and vmschar1[] arrays are subscripted by an offset
 * corresponding to each of the three ranges [0,1,2] to provide the
 * two characters of the VMS device.
 * The offset of the busno value within its range is used to define the
 * third character, using the vmschar2[] array.
 */
LOCAL	char	vmschar[]	= {'d', 'g', 'd'};
LOCAL	char	vmschar1[]	= {'k', 'k', 'q'};
LOCAL	char	vmschar2[]	= {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
				    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
				    'u', 'v', 'w', 'x', 'y', 'z'};


LOCAL	int	do_scg_cmd	__PR((SCSI *scgp, struct scg_cmd *sp));
LOCAL	int	do_scg_sense	__PR((SCSI *scgp, struct scg_cmd *sp));

#define	DEVICE_NAMELEN 8

struct SCSI$DESC {
	Uint	SCSI$L_OPCODE;		/* SCSI Operation Code */
	Uint	SCSI$L_FLAGS;		/* SCSI Flags Bit Map */
	char	*SCSI$A_CMD_ADDR;	/* ->SCSI command buffer */
	Uint	SCSI$L_CMD_LEN; 	/* SCSI command length, bytes */
	char	*SCSI$A_DATA_ADDR;	/* ->SCSI data buffer */
	Uint	SCSI$L_DATA_LEN;	/* SCSI data length, bytes */
	Uint	SCSI$L_PAD_LEN; 	/* SCSI pad length, bytes */
	Uint	SCSI$L_PH_CH_TMOUT;	/* SCSI phase change timeout */
	Uint	SCSI$L_DISCON_TMOUT;	/* SCSI disconnect timeout */
	Uint	SCSI$L_RES_1;		/* Reserved */
	Uint	SCSI$L_RES_2;		/* Reserved */
	Uint	SCSI$L_RES_3;		/* Reserved */
	Uint	SCSI$L_RES_4;		/* Reserved */
	Uint	SCSI$L_RES_5;		/* Reserved */
	Uint	SCSI$L_RES_6;		/* Reserved */
};

#ifdef __ALPHA
#pragma member_alignment save
#pragma nomember_alignment
#endif

struct SCSI$IOSB {
	Ushort	SCSI$W_VMS_STAT;	/* VMS status code */
	Ulong	SCSI$L_IOSB_TFR_CNT;	/* Actual #bytes transferred */
	char	SCSI$B_IOSB_FILL_1;
	Uchar	SCSI$B_IOSB_STS;	/* SCSI device status */
};

#ifdef __ALPHA
#pragma member_alignment restore
#endif

#define	SCSI$K_GOOD_STATUS		0
#define	SCSI$K_CHECK_CONDITION		0x2
#define	SCSI$K_CONDITION_MET		0x4
#define	SCSI$K_BUSY			0x8
#define	SCSI$K_INTERMEDIATE		0x10
#define	SCSI$K_INTERMEDIATE_C_MET	0x14
#define	SCSI$K_RESERVATION_CONFLICT	0x18
#define	SCSI$K_COMMAND_TERMINATED	0x22
#define	SCSI$K_QUEUE_FULL		0x28


#define	SCSI$K_WRITE		0X0	/* direction of transfer=write */
#define	SCSI$K_READ		0X1	/* direction of transfer=read */
#define	SCSI$K_FL_ENAB_DIS	0X2	/* enable disconnects */
#define	SCSI$K_FL_ENAB_SYNC	0X4	/* enable sync */
#define	GK_EFN			0	/* Event flag number */

static char	gk_device[8];		/* XXX JS hoffentlich gibt es keinen Ueberlauf */
static Ushort	gk_chan;
static Ushort	transfer_length;
static int	i;
static int	status;
static $DESCRIPTOR(gk_device_desc, gk_device);
static struct SCSI$IOSB gk_iosb;
static struct SCSI$DESC gk_desc;
static FILE *fp;


struct scg_local {
	Ushort	gk_chan;
};
#define	scglocal(p)	((struct scg_local *)((p)->local))

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
	__scg_help(f, "IO$_DIAGNOSE", "Generic SCSI",
		"", "bus,target,lun", "1,2,0", FALSE, FALSE);
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
	char	devname[DEVICE_NAMELEN];
	char	buschar;
	char	buschar1;
	char	buschar2;
	int	range;
	int	range_offset;

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
	if (busno < 0 || tgt < 0 || tlun < 0) {
		/*
		 * There is no real reason why we cannot scan on VMS,
		 * but for now it is not possible.
		 */
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Unable to scan on VMS");
		return (0);
	}

	if (scgp->local == NULL) {
		scgp->local = malloc(sizeof (struct scg_local));
		if (scgp->local == NULL)
			return (0);
	}

	if (busno < VMS_DKRANGE_MAX) {			/* in the dk range?   */
		range = 0;
		range_offset = busno;
	} else if (busno < VMS_GKRANGE_MAX) {		/* in the gk range?   */
		range = 1;
		range_offset = busno - VMS_DKRANGE_MAX;
	} else if (busno < VMS_DQRANGE_MAX) {		/* in the dq range?   */
		range = 2;
		range_offset = busno - VMS_GKRANGE_MAX;
	} else {
		errno = EINVAL;
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Illegal value for busno '%d'", busno);
		return (-1);
	}
	buschar = vmschar[range];			/* get first device char*/
	buschar1 = vmschar1[range];			/* get 2nd device char*/
	buschar2 = vmschar2[range_offset];		/* get controller char*/

	js_snprintf(devname, sizeof (devname), "%c%c%c%d0%d:",
					buschar, buschar1, buschar2,
					tgt, tlun);
	strcpy(gk_device, devname);
	status = sys$assign(&gk_device_desc, &gk_chan, 0, 0);
	if (!(status & 1)) {
		js_fprintf((FILE *)scgp->errfile,
			"Unable to access scsi-device \"%s\"\n", &gk_device[0]);
		return (-1);
	}
	if (scgp->debug > 0) {
		fp = fopen("cdrecord_io.log", "w", "rfm=stmlf", "rat=cr");
		if (fp == NULL) {
			perror("Failing opening i/o-logfile");
			exit(SS$_NORMAL);
		}
	}
	return (status);
}

LOCAL int
scgo_close(scgp)
	SCSI	*scgp;
{
	/*
	 * XXX close gk_chan ???
	 */
	/*
	 * sys$dassgn()
	 */

	status = sys$dassgn(gk_chan);

	return (status);
}

LOCAL long
scgo_maxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	return (MAX_DMA_VMS);
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
	if (gk_chan == 0)
		return (FALSE);
	return (TRUE);
}

LOCAL int
scgo_fileno(scgp, busno, tgt, tlun)
	SCSI	*scgp;
	int	busno;
	int	tgt;
	int	tlun;
{
	if (gk_chan == 0)
		return (-1);
	return (gk_chan);
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
	int	busno = scg_scsibus(scgp);

	if (busno >= 8)
		return (TRUE);

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

LOCAL void *
scgo_getbuf(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	if (scgp->debug > 0) {
		js_fprintf((FILE *)scgp->errfile,
				"scgo_getbuf: %ld bytes\n", amt);
	}
	scgp->bufbase = malloc((size_t)(amt));	/* XXX JS XXX valloc() ??? */
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
do_scg_cmd(scgp, sp)
	SCSI		*scgp;
	struct scg_cmd	*sp;
{
	char		*cmdadr;
	int		notcmdretry;
	int		len;
	Uchar		scsi_sts;
	int		severity;

	/* XXX JS XXX This cannot be OK */
	notcmdretry = (sp->flags & SCG_CMD_RETRY)^SCG_CMD_RETRY;
	/* error corrected ehh	*/
/*
 * XXX JS Wenn das notcmdretry Flag bei VMS auch 0x08 ist und Du darauf hoffst,
 * XXX	Dasz ich den Wert nie aendere, dann ist das richtig.
 * XXX Siehe unten: Das gleiche gilt fuer SCG_RECV_DATA und SCG_DISRE_ENA !!!
 */

	cmdadr = (char *)sp->cdb.cmd_cdb;
	/* XXX JS XXX This cannot be OK */
	gk_desc.SCSI$L_FLAGS = ((sp->flags & SCG_RECV_DATA) |
				(sp->flags & SCG_DISRE_ENA)|
				notcmdretry);
				/* XXX siehe oben, das ist ein bitweises oder!!! */
	gk_desc.SCSI$A_DATA_ADDR = sp->addr;
	gk_desc.SCSI$L_DATA_LEN = sp->size;
	gk_desc.SCSI$A_CMD_ADDR = cmdadr;
	gk_desc.SCSI$L_CMD_LEN = sp->cdb_len;
	gk_desc.SCSI$L_PH_CH_TMOUT = sp->timeout;
	gk_desc.SCSI$L_DISCON_TMOUT = sp->timeout;
	if (gk_desc.SCSI$L_PH_CH_TMOUT > MAX_PHSTMO_VMS)
	    gk_desc.SCSI$L_PH_CH_TMOUT = MAX_PHSTMO_VMS;
	if (gk_desc.SCSI$L_DISCON_TMOUT > MAX_DSCTMO_VMS)
	    gk_desc.SCSI$L_DISCON_TMOUT = MAX_DSCTMO_VMS;
	gk_desc.SCSI$L_OPCODE = 1;	/* SCSI Operation Code */
	gk_desc.SCSI$L_PAD_LEN = 0;	/* SCSI pad length, bytes */
	gk_desc.SCSI$L_RES_1 = 0;	/* Reserved */
	gk_desc.SCSI$L_RES_2 = 0;	/* Reserved */
	gk_desc.SCSI$L_RES_3 = 0;	/* Reserved */
	gk_desc.SCSI$L_RES_4 = 0;	/* Reserved */
	gk_desc.SCSI$L_RES_5 = 0;	/* Reserved */
	gk_desc.SCSI$L_RES_6 = 0;	/* Reserved */
	if (scgp->debug > 0) {
		js_fprintf(fp, "***********************************************************\n");
		js_fprintf(fp, "SCSI VMS-I/O parameters\n");
		js_fprintf(fp, "OPCODE: %d", gk_desc.SCSI$L_OPCODE);
		js_fprintf(fp, " FLAGS: %d\n", gk_desc.SCSI$L_FLAGS);
		js_fprintf(fp, "CMD:");
		for (i = 0; i < gk_desc.SCSI$L_CMD_LEN; i++) {
			js_fprintf(fp, "%x ", sp->cdb.cmd_cdb[i]);
		}
		js_fprintf(fp, "\n");
		js_fprintf(fp, "DATA_LEN: %d\n", gk_desc.SCSI$L_DATA_LEN);
		js_fprintf(fp, "PH_CH_TMOUT: %d", gk_desc.SCSI$L_PH_CH_TMOUT);
		js_fprintf(fp, " DISCON_TMOUT: %d\n", gk_desc.SCSI$L_DISCON_TMOUT);
	}
	status = sys$qiow(GK_EFN, gk_chan, IO$_DIAGNOSE, &gk_iosb, 0, 0,
			&gk_desc, sizeof (gk_desc), 0, 0, 0, 0);


	if (scgp->debug > 0) {
		js_fprintf(fp, "qiow-status: %i\n", status);
		js_fprintf(fp, "VMS status code %i\n", gk_iosb.SCSI$W_VMS_STAT);
		js_fprintf(fp, "Actual #bytes transferred %i\n", gk_iosb.SCSI$L_IOSB_TFR_CNT);
		js_fprintf(fp, "SCSI device status %i\n", gk_iosb.SCSI$B_IOSB_STS);
		if (gk_iosb.SCSI$L_IOSB_TFR_CNT != gk_desc.SCSI$L_DATA_LEN) {
			js_fprintf(fp, "#bytes transferred != DATA_LEN\n");
		}
	}

	if (!(status & 1)) {		/* Fehlerindikation fuer sys$qiow() */
		sp->ux_errno = geterrno();
		/* schwerwiegender nicht SCSI bedingter Fehler => return (-1) */
		if (sp->ux_errno == ENOTTY || sp->ux_errno == ENXIO ||
		    sp->ux_errno == EINVAL || sp->ux_errno == EACCES) {
			return (-1);
		}
		if (sp->ux_errno == 0)
			sp->ux_errno == EIO;
	} else {
		sp->ux_errno = 0;
	}

	sp->resid = gk_desc.SCSI$L_DATA_LEN - gk_iosb.SCSI$L_IOSB_TFR_CNT;

	if (scgo_isatapi(scgp)) {
		scsi_sts = ((gk_iosb.SCSI$B_IOSB_STS >> 4) & 0x7);
	} else {
		scsi_sts = gk_iosb.SCSI$B_IOSB_STS;
	}

	if (gk_iosb.SCSI$W_VMS_STAT == SS$_NORMAL && scsi_sts == 0) {
		sp->error = SCG_NO_ERROR;
		if (scgp->debug > 0) {
			js_fprintf(fp, "scsi_sts == 0\n");
			js_fprintf(fp, "gk_iosb.SCSI$B_IOSB_STS == 0\n");
			js_fprintf(fp, "sp->error %i\n", sp->error);
			js_fprintf(fp, "sp->resid %i\n", sp->resid);
		}
		return (0);
	}

	severity = gk_iosb.SCSI$W_VMS_STAT & 0x7;

	if (severity == 4) {
		sp->error = SCG_FATAL;
		if (scgp->debug > 0) {
			js_fprintf(fp, "scsi_sts & 2\n");
			js_fprintf(fp, "gk_iosb.SCSI$B_IOSB_STS & 2\n");
			js_fprintf(fp, "gk_iosb.SCSI$W_VMS_STAT & 0x7 == SS$_FATAL\n");
			js_fprintf(fp, "sp->error %i\n", sp->error);
		}
		return (0);
	}
	if (gk_iosb.SCSI$W_VMS_STAT == SS$_TIMEOUT) {
		sp->error = SCG_TIMEOUT;
		if (scgp->debug > 0) {
			js_fprintf(fp, "scsi_sts & 2\n");
			js_fprintf(fp, "gk_iosb.SCSI$B_IOSB_STS & 2\n");
			js_fprintf(fp, "gk_iosb.SCSI$W_VMS_STAT == SS$_TIMEOUT\n");
			js_fprintf(fp, "sp->error %i\n", sp->error);
		}
		return (0);
	}
	sp->error = SCG_RETRYABLE;
	sp->u_scb.cmd_scb[0] = scsi_sts;
	if (scgp->debug > 0) {
		js_fprintf(fp, "scsi_sts & 2\n");
		js_fprintf(fp, "gk_iosb.SCSI$B_IOSB_STS & 2\n");
		js_fprintf(fp, "gk_iosb.SCSI$W_VMS_STAT != 0\n");
		js_fprintf(fp, "sp->error %i\n", sp->error);
	}
	return (0);
}

LOCAL int
do_scg_sense(scgp, sp)
	SCSI		*scgp;
	struct scg_cmd	*sp;
{
	int		ret;
	struct scg_cmd	s_cmd;

	fillbytes((caddr_t)&s_cmd, sizeof (s_cmd), '\0');
	s_cmd.addr = (char *)sp->u_sense.cmd_sense;
	s_cmd.size = sp->sense_len;
	s_cmd.flags = SCG_RECV_DATA|SCG_DISRE_ENA;
	s_cmd.cdb_len = SC_G0_CDBLEN;
	s_cmd.sense_len = CCS_SENSE_LEN;
	s_cmd.cdb.g0_cdb.cmd = SC_REQUEST_SENSE;
	s_cmd.cdb.g0_cdb.lun = sp->cdb.g0_cdb.lun;
	s_cmd.cdb.g0_cdb.count = sp->sense_len;
	ret = do_scg_cmd(scgp, &s_cmd);

	if (ret < 0)
		return (ret);
	if (s_cmd.u_scb.cmd_scb[0] & 02) {
		/* XXX ??? Check condition on request Sense ??? */
	}
	sp->sense_count = sp->sense_len - s_cmd.resid;
	return (ret);
}

LOCAL int
scgo_send(scgp)
	SCSI		*scgp;
{
	struct scg_cmd	*sp = scgp->scmd;
	int	error = sp->error;
	Uchar	status = sp->u_scb.cmd_scb[0];
	int	ret;

	if (scgp->fd < 0) {
		sp->error = SCG_FATAL;
		return (0);
	}
	ret = do_scg_cmd(scgp, sp);
	if (ret >= 0) {
		if (sp->u_scb.cmd_scb[0] & 02)
			ret = do_scg_sense(scgp, sp);
	}
	sp->error = error;
	sp->u_scb.cmd_scb[0] = status;
	return (ret);
}
