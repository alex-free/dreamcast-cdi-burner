/* @(#)scsi-apollo.c	1.8 11/03/07 Copyright 1997,2000 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-apollo.c	1.8 11/03/07 Copyright 1997,2000 J. Schilling";
#endif
/*
 *	Code to support Apollo Domain/OS 10.4.1
 *
 *	Copyright (c) 1997,2000 J. Schilling
 *	Apollo support code written by Paul Walker.
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

#include <apollo/base.h>
#include <apollo/scsi.h>
#include <assert.h>
#define	DomainScsiTimeout	100000

/*
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 */
LOCAL	char	_scg_trans_version[] = "scsi-apollo.c-1.8";	/* The version for this transport */


#define	MAX_SCG		1	/* Max # of SCSI controllers */
#define	MAX_TGT		1	/* Max # of SCSI targets */
#define	MAX_LUN		1	/* Max # of SCSI logical units */

struct scg_local {
	scsi_$handle_t	handle;
	unsigned char	*DomainSensePointer;
	short		scgfiles[MAX_SCG][MAX_TGT][MAX_LUN];
};

#define	scglocal(p)	((struct scg_local *)((p)->local))

#ifndef	SG_MAX_SENSE
#define	SG_MAX_SENSE	16	/* Too small for CCS / SCSI-2	 */
#endif				/* But cannot be changed	 */

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
			return ("Paul Walker");
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
	__scg_help(f, "scsi_$do_command_2", "SCSI transport from Apollo DomainOS drivers",
		"", "DomainOS driver name", "A DomainOS device node name", FALSE, TRUE);
	return (0);
}

LOCAL int
scgo_open(scgp, device)
	SCSI	*scgp;
	char	*device;
{
	register int	nopen = 0;
	status_$t	status;

	if (scgp->debug > 1)
		printf("Entered scsi_open, scgp=%p, device='%s'\n", scgp, device);
	if (scgp->local == NULL) {
		scgp->local = malloc(sizeof (struct scg_local));
		if (scgp->local == NULL)
			return (0);
	}
	if (device == NULL || *device == '\0') {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE, "Must give device name");
		return (0);
	}

	scsi_$acquire(device, strlen(device), &scglocal(scgp)->handle, &status);
	if (status.all) {
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Cannot open '%s', status %08x",
				device, status.all);
		return (0);
	}
	/*
	 * Allocate the sense buffer
	 */
	scglocal(scgp)->DomainSensePointer = (Uchar *)valloc((size_t) (SCG_MAX_SENSE + getpagesize()));
	assert(status.all == 0);
	/*
	 * Wire the sense buffer
	 */
	scsi_$wire(scglocal(scgp)->handle,
		(caddr_t)(scglocal(scgp)->DomainSensePointer),
		SCG_MAX_SENSE, &status);
	assert(status.all == 0);

	if (scglocal(scgp)->scgfiles[0][0][0] == -1)
		scglocal(scgp)->scgfiles[0][0][0] = 1;
	scg_settarget(scgp, 0, 0, 0);
	return (++nopen);
}

LOCAL int
scgo_close(scgp)
	SCSI	*scgp;
{
	status_$t	status;

	if (scgp->debug > 1)
		printf("Entering scsi_close\n");
	scsi_$release(scglocal(scgp)->handle, &status);
	/*
	 * should also unwire the sense buffer
	 */
	return (status.all);
}


LOCAL long
scgo_maxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	status_$t	status;
	scsi_$info_t	info;

	scsi_$get_info(scglocal(scgp)->handle, sizeof (info), &info, &status);
	if (status.all) {
		/*
		 * Should have some better error handling here
		 */
		printf("scsi_$get_info returned %08x\n", status.all);
		return (0);
	}
	return (info.max_xfer);
}


LOCAL void *
scgo_getbuf(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	void	*ret;

	if (scgp->debug > 1)
		printf("scsi_getbuf: %ld bytes\n", amt);
	ret = valloc((size_t)amt);
	if (ret == NULL)
		return (ret);
	scgp->bufbase = ret;
	return (ret);
}

LOCAL void
scgo_freebuf(scgp)
	SCSI	*scgp;
{
	if (scgp->debug > 1)
		printf("Entering scsi_freebuf\n");

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

	if (scgp->debug > 1)
		printf("Entered scsi_havebus:  scgp=%p, busno=%d\n", scgp, busno);

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
	if (scgp->debug > 1)
		printf("Entered scsi_fileno:  scgp=%p, busno=%d, tgt=%d, tlun=%d\n", scgp, busno, tgt, tlun);
	if (busno < 0 || busno >= MAX_SCG ||
		tgt < 0 || tgt >= MAX_TGT ||
		tlun < 0 || tlun >= MAX_LUN)
		return (-1);

	if (scgp->local == NULL)
		return (-1);
	if (scgp->debug > 1)
		printf("exiting scsi_fileno, returning %d\n", scglocal(scgp)->scgfiles[busno][tgt][tlun]);
	return ((int) scglocal(scgp)->scgfiles[busno][tgt][tlun]);
}

LOCAL int
scgo_initiator_id(scgp)
	SCSI	*scgp;
{
	if (scgp->debug > 1)
		printf("Entering scsi_initiator\n");

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
	status_$t	status;

	if (scgp->debug > 0)
		printf("Entering scsi_reset\n");

	if (what == SCG_RESET_NOP)
		return (0);

	if (what == SCG_RESET_TGT) {
		scsi_$reset_device(scglocal(scgp)->handle, &status);
		if (status.all)
			printf("Error - scsi_$reset_device failed, status is 0x%08x\n", status.all);
		return (status.all);
	} else {
		errno = EINVAL;
		return (-1);
	}
}

LOCAL void
scsi_do_sense(scgp, sp)
	SCSI		*scgp;
	struct scg_cmd	*sp;
{
	scsi_$op_status_t	op_status;
	static scsi_$cdb_t	sense_cdb;
	static linteger		sense_cdb_size;
	static linteger		sense_buffer_size;
	static scsi_$operation_id_t sense_op_id;
	static status_$t	sense_status;
	static pinteger		sense_return_count;
		int		i;

	/*
	 * Issue the sense command (wire, issue, wait, unwire
	 */
	sense_buffer_size = sp->sense_len;
	sense_cdb_size = SC_G0_CDBLEN;
	memset(sense_cdb.all, 0, sense_cdb_size);	/* Assuming Apollo sense */
							/* structure is correct */
							/* size */
	sense_cdb.g0.cmd = SC_REQUEST_SENSE;
	sense_cdb.g0.lun = sp->cdb.g0_cdb.lun;
	sense_cdb.g0.len = sp->sense_len;
	scsi_$do_command_2(scglocal(scgp)->handle, sense_cdb, sense_cdb_size,
		(caddr_t) (scglocal(scgp)->DomainSensePointer),
		sense_buffer_size, scsi_read, &sense_op_id, &sense_status);
	if (sense_status.all) {
		printf("Error executing sense command, status is 0x%08x\n", sense_status.all);
	}
	scsi_$wait(scglocal(scgp)->handle, DomainScsiTimeout, true,
		sense_op_id, 1, &op_status, &sense_return_count,
		&sense_status);
	/*
	 * Print the sense information if debug is on, or if the information is
	 * "unusual"
	 */
	if (scgp->debug > 0 ||
		/*
		 * I don't prinqqt info for sense codes 0, 2, 5, 6 because
		 * they aren't interesting
		 */
		(((u_char *) scglocal(scgp)->DomainSensePointer)[2] == 1) ||
		(((u_char *) scglocal(scgp)->DomainSensePointer)[2] == 3) ||
		(((u_char *) scglocal(scgp)->DomainSensePointer)[2] == 4) ||
		(((u_char *) scglocal(scgp)->DomainSensePointer)[2] >= 7)) {
		printf(" Sense dump:\n");
		for (i = 0; i < sp->sense_len; i++)
			printf(" %02x", ((u_char *) scglocal(scgp)->DomainSensePointer)[i]);
		printf("\n");
	}
	if (((u_char *) scglocal(scgp)->DomainSensePointer)[2] == 5) {
		/*
		 * Illegal command
		 */
		printf("Illegal command detected, ASC=0x%02x, ASQ=0x%02x\n",
			((u_char *) scglocal(scgp)->DomainSensePointer)[12],
			((u_char *) scglocal(scgp)->DomainSensePointer)[13]);
	}
	/*
	 * Copy the sense information to the driver
	 */
	memcpy(sp->u_sense.cmd_sense, scglocal(scgp)->DomainSensePointer, sp->sense_len);
	sp->sense_count = sp->sense_len;
}


LOCAL int
scgo_send(scgp)
	SCSI		*scgp;
{
	linteger	buffer_length;
	linteger	cdb_len;
	scsi_$operation_id_t operation;
	scsi_$wait_index_t wait_index;
	scsi_$op_status_t op_status;
	pinteger	return_count;
	status_$t	status;
	char	*ascii_wait_status;
	int		i;
	struct scg_cmd *sp = scgp->scmd;

	if (scgp->fd < 0) {
		sp->error = SCG_FATAL;
		return (0);
	}

	if (scgp->debug > 0) {
		printf("Entered scgo_send, scgp=%p, sp=%p\n", scgp, sp);
		printf("scgcmd dump:\n");
		printf("  addr=%p\n", sp->addr);
		printf("  size=0x%x\n", sp->size);
		printf("  flags=0x%x\n", sp->flags);
		printf("  cdb_len=%d\n", sp->cdb_len);
		printf("  sense_len=%d\n", sp->sense_len);
		printf("  timeout=%d\n", sp->timeout);
		printf("  kdebug=%d\n", sp->kdebug);
		printf("  CDB:");
		for (i = 0; i < sp->cdb_len; i++)
				printf(" %02x", sp->cdb.cmd_cdb[i]);
		printf("\n");
	}

	/*
	 * Assume complete transfer, so residual count = 0
	 */
	sp->resid = 0;
	buffer_length = sp->size;
	if (sp->addr) {
		if (scgp->debug > 0)
			printf(" wiring 0x%x bytes at 0x%x\n", buffer_length, sp->addr);
		scsi_$wire(scglocal(scgp)->handle, sp->addr, buffer_length, &status);
		if (status.all) {
			/*
			 * Need better error handling
			 */
			printf("scsi_$wire failed, 0x%08x\n", status.all);
		}
	}
	cdb_len = sp->cdb_len;
	scsi_$do_command_2(scglocal(scgp)->handle,		/* device handle*/
			*(scsi_$cdb_t *) &(sp->cdb.cmd_cdb[0]),	/* SCSI CDB	*/
			cdb_len,				/* CDB len	*/
			sp->addr,				/* DMA buf	*/
			buffer_length,				/* DMA len	*/
			(sp->flags & SCG_RECV_DATA) ? scsi_read : scsi_write,
			&operation,				/* OP ID	*/
			&status);				/* Status ret	*/

	if (status.all) {
		/*
		 * Need better error handling
		 */
		printf("scsi_$do_command failed, 0x%08x\n", status.all);
		sp->error = SCG_FATAL;
		sp->ux_errno = EIO;
		return (0);
	} else if (scgp->debug > 0) {
		printf("Command submitted, operation=0x%x\n", operation);
	}
	wait_index = scsi_$wait(scglocal(scgp)->handle,		/* device handle*/
				sp->timeout * 1000,		/* timeout	*/
				0,				/* async enable	*/
				operation,			/* OP ID	*/
				1,				/* max count	*/
				&op_status,			/* status list	*/
				&return_count,			/* count ret	*/
				&status);			/* Status ret	*/
	if (status.all) {
		/*
		 * Need better error handling
		 */
		printf("scsi_$wait failed, 0x%08x\n", status.all);
		sp->error = SCG_FATAL;
		sp->ux_errno = EIO;
		return (0);
	} else {
		if (scgp->debug > 0) {
			printf(
			"wait_index=%d, return_count=%d, op_status: op=0x%x, cmd_status=0x%x, op_status=0x%x\n",
				wait_index, return_count, op_status.op,
				op_status.cmd_status, op_status.op_status);
		}
		switch (wait_index) {

		case scsi_device_advance:
			ascii_wait_status = "scsi_device_advance";
			break;
		case scsi_timeout:
			ascii_wait_status = "scsi_timeout";
			break;
		case scsi_async_fault:
			ascii_wait_status = "scsi_async_fault";
			break;
		default:
			ascii_wait_status = "unknown";
			break;
		}
		/*
		 * See if the scsi_$wait terminated "abnormally"
		 */
		if (wait_index != scsi_device_advance) {
			printf("scsi_$wait terminated abnormally, status='%s'\n", ascii_wait_status);
			sp->error = SCG_FATAL;
			sp->ux_errno = EIO;
			return (0);
		}
		/*
		 * Normal termination, what's the scoop?
		 */
		assert(return_count == 1);
		switch (op_status.cmd_status.all) {

		case status_$ok:
			switch (op_status.op_status) {

			case scsi_good_status:
				sp->error = SCG_NO_ERROR;
				sp->ux_errno = 0;
				break;
			case scsi_busy:
				sp->error = SCG_NO_ERROR;
				sp->ux_errno = 0;
				break;
			case scsi_check_condition:
				if (scgp->debug > 0)
					printf("SCSI ERROR - CheckCondition\n");
				scsi_do_sense(scgp, sp);
				/*
				 * If this was a media error, then call it retryable,
				 * instead of no error
				 */
				if ((((u_char *) scglocal(scgp)->DomainSensePointer)[0] == 0xf0) &&
					(((u_char *) scglocal(scgp)->DomainSensePointer)[2] == 0x03)) {
					if (scgp->debug > 0)
						printf("  (retryable)\n");
					sp->error = SCG_RETRYABLE;
					sp->ux_errno = EIO;
				} else {
				/* printf("  (no error)\n"); */
					sp->error = SCG_NO_ERROR;
					sp->ux_errno = 0;
				}
				break;
			default:
				/*
				 * I fault out in this case because I want to know
				 * about this error, and this guarantees that it will
				 * get attention.
				 */
				printf("Unhandled Domain/OS op_status error:  status=%08x\n",
									op_status.op_status);
				exit(EXIT_FAILURE);
			}
			/*
			 * Is this the right place to copy the SCSI status byte?
			 */
			sp->u_scb.cmd_scb[0] = op_status.op_status;
			break;
		/*
		 * Handle recognized error conditions by copying the error
		 * code over
		 */
		case scsi_$operation_timeout:
			printf("SCSI ERROR - Timeout\n");
			scsi_do_sense(scgp, sp);
			sp->error = SCG_TIMEOUT;
			sp->ux_errno = EIO;
			break;
		case scsi_$dma_underrun:
			/*
			 * This condition seems to occur occasionaly.  I no longer
			 *  complain because it doesn't seem to matter.
			 */
			if (scgp->debug > 0)
				printf("SCSI ERROR - Underrun\n");
			scsi_do_sense(scgp, sp);
			sp->resid = sp->size;	/* We don't have the right number */
			sp->error = SCG_RETRYABLE;
			sp->ux_errno = EIO;
			break;
		case scsi_$hdwr_failure:	/* received when both scanners were active */
			printf("SCSI ERROR - Hardware Failure\n");
			scsi_do_sense(scgp, sp);
			sp->error = SCG_RETRYABLE;
			sp->ux_errno = EIO;
			break;
		default:
			printf("\nUnhandled Domain/OS cmd_status error:  status=%08x\n",
				op_status.cmd_status.all);
			error_$print(op_status.cmd_status);
			exit(EXIT_FAILURE);
		}
	}
	if (sp->addr) {
		if (scgp->debug > 0)
			printf(" unwiring buffer\n");
		scsi_$unwire(scglocal(scgp)->handle, sp->addr, buffer_length,
					sp->flags & SCG_RECV_DATA, &status);
		if (status.all) {
			/*
			 * Need better error handling
			 */
			printf("scsi_$unwire failed, 0x%08x\n", status.all);
			sp->error = SCG_FATAL;
			sp->ux_errno = EIO;
			return (0);
		}
	}
	return (0);
}
