/* @(#)scsi-syllable.c	1.2 12/12/02 Copyright 1998-2012 J. Schilling, 2005 Kristian Van Der Vliet */
#ifndef lint
static	char sccsid[] =
	"@(#)scsi-syllable.c	1.2 12/12/02 Copyright 1998-2012 J. Schilling, 2005 Kristian Van Der Vliet";
#endif
/*
 *	Interface for the Syllable "SCSI" implementation.
 *
 *	Current versions of Syllable do not implement a complete SCSI layer,
 *	but the ATAPI portion of the ATA driver has a simple interface that
 *	can be used to send raw ATAPI packets to the device.
 *
 *	This drives can be seen only as a first hack. As it does not support
 *	SCSI bus scanning, there is not yet support for the auto-target
 *	feature os higher level applications.
 *
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 *
 *	Copyright (c) 1998-2012 J. Schilling
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
LOCAL	char	_scg_trans_version[] = "scsi-syllable.c-1.2"; /* The version for this transport */

#include <schily/stdlib.h>
#include <schily/stdio.h>
#include <schily/string.h>
#include <schily/unistd.h>
#include <schily/stat.h>
#ifdef	__PYRO__		/* Pyro */
#include <pyro/types.h>
#include <pyro/cdrom.h>
#else				/* Syllable and AtheOS */
#include <atheos/types.h>
#include <atheos/cdrom.h>
#endif

#define	MAX_SCG		16	/* Max # of SCSI controllers */

struct scg_local {
	int		fd;
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
		case SCG_AUTHOR:
			return ("vanders");
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
	__scg_help(f, "ATA", "ATA Packet SCSI transport",
		"ATAPI:", "device", "/dev/disk/ata/cdc/raw", FALSE, TRUE);
	return (0);
}

LOCAL int
scgo_open(scgp, device)
	SCSI	*scgp;
	char	*device;
{
	int	fd;

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
	return (MAX_SCG);
}

LOCAL BOOL
scgo_havebus(scgp, busno)
	SCSI	*scgp;
	int	busno;
{
	return (TRUE);
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
	return (TRUE);
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
	int			ret;
	cdrom_packet_cmd_s	cmd;

	if (scgp->fd < 0) {
		sp->error = SCG_FATAL;
		return (-1);
	}

	/*
	 * As long as we only support ATAPI, we limit the SCSI CDB size to 12.
	 */
	if (sp->cdb_len > 12) {
		if (scgp->debug > 0)
			error("sp->cdb_len > 12!\n");
		sp->error = SCG_FATAL;
		sp->ux_errno = EIO;
		return (0);
	}

	/* initialize */
	fillbytes((caddr_t) & cmd, sizeof (cmd), '\0');
	fillbytes((caddr_t) & sp->u_sense.cmd_sense,
					sizeof (sp->u_sense.cmd_sense), '\0');

	memcpy(cmd.nCommand, &(sp->cdb), sp->cdb_len);
	cmd.nCommandLength = sp->cdb_len;
	cmd.pnData = (uint8 *)sp->addr;
	cmd.nDataLength = sp->size;
	cmd.pnSense = (uint8 *)sp->u_sense.cmd_sense;
	cmd.nSenseLength = sp->sense_len;
	cmd.nFlags = 0;

	if (sp->size > 0) {
		if (sp->flags & SCG_RECV_DATA)
			cmd.nDirection = READ;
		else
			cmd.nDirection = WRITE;
	} else {
		cmd.nDirection = NO_DATA;
	}

	if (scgp->debug > 0) {
		error("SEND(%d): cmd %02x, cdb = %d, data = %d, sense = %d\n",
			scgp->fd, cmd.nCommand[0], cmd.nCommandLength,
			cmd.nDataLength, cmd.nSenseLength);
	}
	ret = ioctl(scgp->fd, CD_PACKET_COMMAND, &cmd, sizeof (cmd));
	if (ret < 0)
		sp->ux_errno = geterrno();

	if (ret < 0 && scgp->debug > 4) {
		js_fprintf((FILE *) scgp->errfile,
			"ioctl(CD_PACKET_COMMAND) ret: %d\n", ret);
	}

	if (ret < 0 && cmd.nError) {
		sp->u_scb.cmd_scb[0] = ST_CHK_COND;
		if (scgp->debug > 0)
			error("result: scsi %02x\n", cmd.nError);

		switch (cmd.nError) {

		case SC_UNIT_ATTENTION:
		case SC_NOT_READY:
			sp->error = SCG_RETRYABLE;	/* may be BUS_BUSY */
			sp->u_scb.cmd_scb[0] |= ST_BUSY;
			break;
		case SC_ILLEGAL_REQUEST:
			break;
		default:
			break;
		}
	} else {
		sp->u_scb.cmd_scb[0] = 0x00;
	}

	return (0);
}
