/* @(#)scsi-beos.c	1.29 09/06/30 Copyright 1998-2009 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-beos.c	1.29 09/06/30 Copyright 1998-2009 J. Schilling";
#endif
/*
 *	Interface for the BeOS user-land raw SCSI implementation.
 *
 *	This is a hack, that tries to emulate the functionality
 *	of the scg driver.
 *
 *	First version done by swetland@be.com
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


/*
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 */
LOCAL	char	_scg_trans_version[] = "scsi-beos.c-1.29";	/* The version for this transport*/

/*
 * There are also defines for:
 *	B_BEOS_VERSION_4
 *	B_BEOS_VERSION_4_5
 *
 * in BeOS 5
 */
#ifdef	B_BEOS_VERSION_5
#define	NEW_BEOS
#endif
#ifdef	__HAIKU__
#define	NEW_BEOS
#endif

#ifndef	NEW_BEOS
/*
 * New BeOS seems to include <be/kernel/OS.h> from device/scsi.h
 */

/* nasty hack to avoid broken def of bool in SupportDefs.h */
#define	_SUPPORT_DEFS_H

#ifndef _SYS_TYPES_H
typedef unsigned long			ulong;
typedef unsigned int			uint;
typedef unsigned short			ushort;
#endif	/* _SYS_TYPES_H */

#include <BeBuild.h>
#include <schily/types.h>
#include <Errors.h>


/*-------------------------------------------------------------*/
/*----- Shorthand type formats --------------------------------*/

typedef	signed char			int8;
typedef unsigned char			uint8;
typedef volatile signed char		vint8;
typedef volatile unsigned char		vuint8;

typedef	short				int16;
typedef unsigned short			uint16;
typedef volatile short			vint16;
typedef volatile unsigned short		vuint16;

typedef	long				int32;
typedef unsigned long			uint32;
typedef volatile long			vint32;
typedef volatile unsigned long		vuint32;

typedef	long long			int64;
typedef unsigned long long		uint64;
typedef volatile long long		vint64;
typedef volatile unsigned long long	vuint64;

typedef volatile long			vlong;
typedef volatile int			vint;
typedef volatile short			vshort;
typedef volatile char			vchar;

typedef volatile unsigned long		vulong;
typedef volatile unsigned int		vuint;
typedef volatile unsigned short		vushort;
typedef volatile unsigned char		vuchar;

typedef unsigned char			uchar;
typedef unsigned short			unichar;



/*-------------------------------------------------------------*/
/*----- Descriptive formats -----------------------------------*/
typedef int32					status_t;
typedef int64					bigtime_t;
typedef uint32					type_code;
typedef uint32					perform_code;

/* end nasty hack */

#endif	/* ! B_BEOS_VERSION_5 */


#include <schily/stdlib.h>
#include <schily/stdio.h>
#include <schily/string.h>
#include <schily/unistd.h>
#include <schily/stat.h>
#include <scg/scgio.h>

/* this is really really dumb (tm) */
/*#undef sense*/
/*#undef scb*/
#include <device/scsi.h>

#undef bool
#ifdef	__HAIKU__	/* Probaby already since BeOS 5 */
#include <CAM.h>
#else
#include <drivers/CAM.h>
#endif

struct _fdmap_ {
	struct _fdmap_ *next;
	int bus;
	int targ;
	int lun;
	int fd;
};

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
	__scg_help(f, "CAM", "Generic transport independent SCSI (BeOS CAM variant)",
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
#ifdef	nonono
	int	tlun	= scg_lun(scgp);
#endif

#ifdef	nonono
	if (busno >= MAX_SCG || tgt >= MAX_TGT || tlun >= MAX_LUN) {
		errno = EINVAL;
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Illegal value for busno, target or lun '%d,%d,%d'",
				busno, tgt, tlun);
		return (-1);
	}
#endif

	if ((device != NULL && *device != '\0') || (busno == -2 && tgt == -2)) {
		errno = EINVAL;
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Open by 'devname' not supported on this OS");
		return (-1);
	}
	return (1);
}

LOCAL int
scgo_close(scgp)
	SCSI	*scgp;
{
	struct _fdmap_	*f;
	struct _fdmap_	*fnext;

	for (f = (struct _fdmap_ *)scgp->local; f; f = fnext) {
		scgp->local = 0;
		fnext = f->next;
		close(f->fd);
		free(f);
	}
	return (0);
}

LOCAL long
scgo_maxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	return (128*1024);
	return (256*1024);
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
	return (16);	/* XXX we need a better way to find the # of busses */
}

LOCAL BOOL
scgo_havebus(scgp, busno)
	SCSI	*scgp;
	int	busno;
{
	struct stat	sb;
	char		buf[128];

	if (busno < 8)
		js_snprintf(buf, sizeof (buf), "/dev/bus/scsi/%d", busno);
	else
#ifdef	__HAIKU__
		js_snprintf(buf, sizeof (buf), "/dev/disk/atapi/%d", busno-8);
#else
		js_snprintf(buf, sizeof (buf), "/dev/disk/ide/atapi/%d", busno-8);
#endif
	if (stat(buf, &sb))
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
	struct _fdmap_	*f;
	char		buf[128];
	int		fd;

	for (f = (struct _fdmap_ *)scgp->local; f; f = f->next) {
		if (f->bus == busno && f->targ == tgt && f->lun == tlun)
			return (f->fd);
	}
	if (busno < 8) {
		js_snprintf(buf, sizeof (buf),
					"/dev/bus/scsi/%d/%d/%d/raw",
					busno, tgt, tlun);
	} else {
		char *tgtstr = (tgt == 0) ? "master" : (tgt == 1) ? "slave" : "dummy";
		js_snprintf(buf, sizeof (buf),
#ifdef	__HAIKU__
					"/dev/disk/atapi/%d/%s/raw",
					busno-8, tgtstr);
#else
					"/dev/disk/ide/atapi/%d/%s/%d/raw",
					busno-8, tgtstr, tlun);
#endif
	}
	fd = open(buf, 0);

	if (fd >= 0) {
		f = (struct _fdmap_ *) malloc(sizeof (struct _fdmap_));
		f->bus = busno;
		f->targ = tgt;
		f->lun = tlun;
		f->fd = fd;
		f->next = (struct _fdmap_ *)scgp->local;
		scgp->local = f;
	}
	return (fd);
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
	/*
	 * XXX Should check for ATAPI
	 */
	return (-1);
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
	SCSI	*scgp;
{
	struct scg_cmd		*sp = scgp->scmd;
	int			e;
	int 			scsi_error;
	int			cam_error;
	raw_device_command	rdc;

	if (scgp->fd < 0) {
		sp->error = SCG_FATAL;
		return (0);
	}

	memcpy(rdc.command, &(sp->cdb), 12);
	rdc.command_length = sp->cdb_len;
	rdc.data = sp->addr;
	rdc.data_length = sp->size;
	rdc.sense_data_length = sp->sense_len;
	rdc.sense_data = sp->u_sense.cmd_sense;
	rdc.flags = sp->flags & SCG_RECV_DATA ? B_RAW_DEVICE_DATA_IN : 0;
	if (sp->size > 0)
		rdc.flags |= B_RAW_DEVICE_REPORT_RESIDUAL;
	rdc.timeout = sp->timeout * 1000000;

	sp->error		= SCG_NO_ERROR;
	sp->sense_count		= 0;
	sp->u_scb.cmd_scb[0]	= 0;
	sp->resid		= 0;

	if (scgp->debug > 0) {
		error("SEND(%d): cmd %02x, cdblen = %d, datalen = %ld, senselen = %ld\n",
			scgp->fd, rdc.command[0], rdc.command_length,
			rdc.data_length, rdc.sense_data_length);
	}
	e = ioctl(scgp->fd, B_RAW_DEVICE_COMMAND, &rdc, sizeof (rdc));
	if (scgp->debug > 0) {
		error("SEND(%d): -> %d CAM Status %02X SCSI status %02X\n", e, rdc.cam_status, rdc.scsi_status);
	}
	sp->ux_errno = 0;
#ifdef	DEBUG
	error("err %d errno %x CAM %X SL %d DL %d/%d FL %X\n",
		e, geterrno(), rdc.cam_status,
		rdc.sense_data_length, rdc.data_length, sp->size, rdc.flags);
#endif
	if (!e) {
		cam_error = rdc.cam_status;
		scsi_error = rdc.scsi_status;
		sp->u_scb.cmd_scb[0] = scsi_error;
		if (sp->size > 0)
			sp->resid = sp->size - rdc.data_length;

		switch (cam_error & CAM_STATUS_MASK) {

		case CAM_REQ_CMP:
			sp->error = SCG_NO_ERROR;
			break;

		case CAM_REQ_CMP_ERR:
			sp->sense_count = sp->sense_len; /* XXX */
			sp->error = SCG_NO_ERROR;
			sp->ux_errno = EIO;
			break;

		case CAM_CMD_TIMEOUT:
			sp->error = SCG_TIMEOUT;
			sp->ux_errno = EIO;

		case CAM_SEL_TIMEOUT:
			sp->error = SCG_FATAL;
			sp->ux_errno = EIO;
			break;

		default:
			sp->error = SCG_RETRYABLE;
			sp->ux_errno = EIO;
		}
	} else {
		sp->error = SCG_FATAL;
		sp->ux_errno = geterrno();
		sp->resid = sp->size;
	}
	return (0);
}
