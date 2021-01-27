/* @(#)scsi-os2.c	1.26 06/11/26 Copyright 1998 J. Schilling, C. Wohlgemuth */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-os2.c	1.26 06/11/26 Copyright 1998 J. Schilling, C. Wohlgemuth";
#endif
/*
 *	Interface for the OS/2 ASPI-Router ASPIROUT.SYS ((c) D. Dorau).
 *		This additional driver is a prerequisite for using cdrecord.
 *		Get it from HOBBES or LEO.
 *
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 *
 *	XXX it currently uses static SRB and for this reason is not reentrant
 *
 *	Copyright (c) 1998 J. Schilling
 *	Copyright (c) 1998 C. Wohlgemuth for this interface.
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

/*#define	DEBUG*/

/* For AspiRouter */
#include "scg/srb_os2.h"

/*
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 */
LOCAL	char	_scg_trans_version[] = "scsi-os2.c-1.26";	/* The version for this transport*/

#define	FILE_OPEN			0x0001
#define	OPEN_SHARE_DENYREADWRITE	0x0010
#define	OPEN_ACCESS_READWRITE		0x0002
#define	DC_SEM_SHARED			0x01
#define	OBJ_TILE			0x0040
#define	PAG_READ			0x0001
#define	PAG_WRITE			0x0002
#define	PAG_COMMIT			0x0010

typedef unsigned long LHANDLE;
typedef unsigned long ULONG;
typedef unsigned char *PSZ;
typedef unsigned short USHORT;
typedef unsigned char UCHAR;

typedef LHANDLE	HFILE;
typedef ULONG	HEV;

#define	MAX_SCG		16	/* Max # of SCSI controllers */
#define	MAX_TGT		16
#define	MAX_LUN		8

struct scg_local {
	int	dummy;
};
#define	scglocal(p)	((struct scg_local *)((p)->local))

#define	MAX_DMA_OS2	(63*1024) /* ASPI-Router allows up to 64k */

LOCAL	void	*buffer		= NULL;
LOCAL	HFILE	driver_handle	= 0;
LOCAL	HEV	postSema	= 0;

LOCAL	BOOL	open_driver	__PR((SCSI *scgp));
LOCAL	BOOL	close_driver	__PR((void));
LOCAL	ULONG	wait_post	__PR((ULONG ulTimeOut));
LOCAL	BOOL 	init_buffer	__PR((void* mem));
LOCAL	void	exit_func	__PR((void));
LOCAL	void	set_error	__PR((SRB *srb, struct scg_cmd *sp));


LOCAL void
exit_func()
{
	if (!close_driver())
		js_fprintf(stderr, "Cannot close OS/2-ASPI-Router!\n");
}

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
	__scg_help(f, "ASPI", "Generic transport independent SCSI",
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
	}

	if (!open_driver(scgp))	/* Try to open ASPI-Router */
		return (-1);
	atexit(exit_func);	/* Install Exit Function which closes the ASPI-Router */

	/*
	 * Success after all
	 */
	return (1);
}

LOCAL int
scgo_close(scgp)
	SCSI	*scgp;
{
	exit_func();
	return (0);
}

LOCAL long
scgo_maxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	long maxdma = MAX_DMA_OS2;
	return (maxdma);
}

LOCAL void *
scgo_getbuf(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	ULONG rc;

#ifdef DEBUG
	js_fprintf((FILE *)scgp->errfile, "scgo_getbuf: %ld bytes\n", amt);
#endif
	rc = DosAllocMem(&buffer, amt, OBJ_TILE | PAG_READ | PAG_WRITE | PAG_COMMIT);

	if (rc) {
		js_fprintf((FILE *)scgp->errfile, "Cannot allocate buffer.\n");
		return ((void *)0);
	}
	scgp->bufbase = buffer;

#ifdef DEBUG
	js_fprintf((FILE *)scgp->errfile, "Buffer allocated at: 0x%x\n", scgp->bufbase);
#endif

	/* Lock memory */
	if (init_buffer(scgp->bufbase))
		return (scgp->bufbase);

	js_fprintf((FILE *)scgp->errfile, "Cannot lock memory buffer.\n");
	return ((void *)0); /* Error */
}

LOCAL void
scgo_freebuf(scgp)
	SCSI	*scgp;
{
	if (scgp->bufbase && DosFreeMem(scgp->bufbase)) {
		js_fprintf((FILE *)scgp->errfile,
		"Cannot free buffer memory for ASPI-Router!\n"); /* Free our memory buffer if not already done */
	}
	if (buffer == scgp->bufbase)
		buffer = NULL;
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

	return (TRUE);
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

	/*
	 * Return fake
	 */
	return (1);
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
	ULONG	rc;				/* return value */
	ULONG	cbreturn;
	ULONG	cbParam;
	BOOL	success;
static	SRB	SRBlock;			/* XXX makes it non reentrant */

	if (what == SCG_RESET_NOP)
		return (0);
	if (what != SCG_RESET_BUS) {
		errno = EINVAL;
		return (-1);
	}
	/*
	 * XXX Does this reset TGT or BUS ???
	 */
	SRBlock.cmd		= SRB_Reset;		/* reset device		*/
	SRBlock.ha_num		= scg_scsibus(scgp);	/* host adapter number	*/
	SRBlock.flags		= SRB_Post;		/* posting enabled	*/
	SRBlock.u.res.target	= scg_target(scgp);	/* target id		*/
	SRBlock.u.res.lun	= scg_lun(scgp);	/* target LUN		*/

	rc = DosDevIOCtl(driver_handle, 0x92, 0x02, (void*) &SRBlock, sizeof (SRB), &cbParam,
			(void*) &SRBlock, sizeof (SRB), &cbreturn);
	if (rc) {
		js_fprintf((FILE *)scgp->errfile,
				"DosDevIOCtl() failed in resetDevice.\n");
		return (1);			/* DosDevIOCtl failed */
	} else {
		success = wait_post(40000);	/** wait for SRB being processed */
		if (success)
			return (2);
	}
	if (SRBlock.status != SRB_Done)
		return (3);
#ifdef DEBUG
	js_fprintf((FILE *)scgp->errfile,
		"resetDevice of host: %d target: %d lun: %d successful.\n", scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp));
	js_fprintf((FILE *)scgp->errfile,
		"SRBlock.ha_status: 0x%x, SRBlock.target_status: 0x%x, SRBlock.satus: 0x%x\n",
				SRBlock.u.cmd.ha_status, SRBlock.u.cmd.target_status, SRBlock.status);
#endif
	return (0);
}

/*
 * Set error flags
 */
LOCAL void
set_error(srb, sp)
	SRB	*srb;
	struct scg_cmd	*sp;
{
	switch (srb->status) {

	case SRB_InvalidCmd:		/* 0x80 Invalid SCSI request	    */
	case SRB_InvalidHA:		/* 0x81 Invalid host adapter number */
	case SRB_BadDevice:		/* 0x82 SCSI device not installed   */
		sp->error = SCG_FATAL;
		sp->ux_errno = EINVAL;	/* Should we ever return != EIO	    */
		sp->ux_errno = EIO;
		break;


	case SRB_Busy:			/* 0x00 SCSI request in progress    */
	case SRB_Aborted:		/* 0x02 SCSI aborted by host	    */
	case SRB_BadAbort:		/* 0x03 Unable to abort SCSI request */
	case SRB_Error:			/* 0x04 SCSI request completed with error */
	default:
		sp->error = SCG_RETRYABLE;
		sp->ux_errno = EIO;
		break;
	}
}

LOCAL int
scgo_send(scgp)
	SCSI	*scgp;
{
	struct scg_cmd	*sp = scgp->scmd;
	ULONG	rc;				/* return value */
static	SRB	SRBlock;			/* XXX makes it non reentrant */
	Ulong	cbreturn;
	Ulong	cbParam;
	UCHAR*	ptr;

	if (scgp->fd < 0) {			/* Set in scgo_open() */
		sp->error = SCG_FATAL;
		return (0);
	}

	if (sp->cdb_len > sizeof (SRBlock.u.cmd.cdb_st)) { /* commandsize too big */
		sp->error = SCG_FATAL;
		sp->ux_errno = EINVAL;
		js_fprintf((FILE *)scgp->errfile,
			"sp->cdb_len > SRBlock.u.cmd.cdb_st. Fatal error in scgo_send, exiting...\n");
		return (-1);
	}

	/* clear command block */
	fillbytes((caddr_t)&SRBlock.u.cmd.cdb_st, sizeof (SRBlock.u.cmd.cdb_st), '\0');
	/* copy cdrecord command into SRB */
	movebytes(&sp->cdb, &SRBlock.u.cmd.cdb_st, sp->cdb_len);

	/* Build SRB command block */
	SRBlock.cmd = SRB_Command;
	SRBlock.ha_num = scg_scsibus(scgp);	/* host adapter number */

	SRBlock.flags = SRB_Post;		/* flags */

	SRBlock.u.cmd.target	= scg_target(scgp); /* Target SCSI ID */
	SRBlock.u.cmd.lun	= scg_lun(scgp); /* Target SCSI LUN */
	SRBlock.u.cmd.data_len	= sp->size;	/* # of bytes transferred */
	SRBlock.u.cmd.data_ptr	= 0;		/* pointer to data buffer */
	SRBlock.u.cmd.sense_len	= sp->sense_len; /* length of sense buffer */

	SRBlock.u.cmd.link_ptr	= 0;		/* pointer to next SRB */
	SRBlock.u.cmd.cdb_len	= sp->cdb_len;	/* SCSI command length */

	/* Specify direction */
	if (sp->flags & SCG_RECV_DATA) {
		SRBlock.flags |= SRB_Read;
	} else {
		if (sp->size > 0) {
			SRBlock.flags |= SRB_Write;
			if (scgp->bufbase != sp->addr) { /* Copy only if data not in ASPI-Mem */
				movebytes(sp->addr, scgp->bufbase, sp->size);
			}
		} else {
			SRBlock.flags |= SRB_NoTransfer;
		}
	}
	sp->error	= SCG_NO_ERROR;
	sp->sense_count	= 0;
	sp->u_scb.cmd_scb[0] = 0;
	sp->resid	= 0;

	/* execute SCSI	command */
	rc = DosDevIOCtl(driver_handle, 0x92, 0x02,
			(void*) &SRBlock, sizeof (SRB), &cbParam,
			(void*) &SRBlock, sizeof (SRB), &cbreturn);

	if (rc) {		/* An error occured */
		js_fprintf((FILE *)scgp->errfile,
				"DosDevIOCtl() in sendCommand failed.\n");
		sp->error = SCG_FATAL;
		sp->ux_errno = EIO;	/* Später vielleicht errno einsetzen */
		return (rc);
	} else {
		/* Wait until the command is processed */
		rc = wait_post(sp->timeout*1000);
		if (rc) {	/* An error occured */
			if (rc == 640) {
				/* Timeout */
				sp->error = SCG_TIMEOUT;
				sp->ux_errno = EIO;
				js_fprintf((FILE *)scgp->errfile,
						"Timeout during SCSI-Command.\n");
				return (1);
			}
			sp->error = SCG_FATAL;
			sp->ux_errno = EIO;
			js_fprintf((FILE *)scgp->errfile,
					"Fatal Error during DosWaitEventSem().\n");
			return (1);
		}

		/* The command is processed */
		if (SRBlock.status == SRB_Done) {	/* succesful completion */
#ifdef DEBUG
			js_fprintf((FILE *)scgp->errfile,
				"Command successful finished. SRBlock.status=0x%x\n\n", SRBlock.status);
#endif
			sp->sense_count = 0;
			sp->resid = 0;
			if (sp->flags & SCG_RECV_DATA) {	/* We read data */
				if (sp->addr && sp->size) {
					if (scgp->bufbase != sp->addr)	/* Copy only if data not in ASPI-Mem */
						movebytes(scgp->bufbase, sp->addr, SRBlock.u.cmd.data_len);
					ptr = (UCHAR*)sp->addr;
					sp->resid = sp->size - SRBlock.u.cmd.data_len; /*nicht übertragene bytes. Korrekt berechnet???*/
				}
			}	/* end of if (sp->flags & SCG_RECV_DATA) */
			if (SRBlock.u.cmd.target_status == SRB_CheckStatus) { /* Sense data valid */
				sp->sense_count	= (int)SRBlock.u.cmd.sense_len;
				if (sp->sense_count > sp->sense_len)
					sp->sense_count = sp->sense_len;

				ptr = (UCHAR*)&SRBlock.u.cmd.cdb_st;
				ptr += SRBlock.u.cmd.cdb_len;

				fillbytes(&sp->u_sense.Sense, sizeof (sp->u_sense.Sense), '\0');
				movebytes(ptr, &sp->u_sense.Sense, sp->sense_len);

				sp->u_scb.cmd_scb[0] = SRBlock.u.cmd.target_status;
				sp->ux_errno = EIO;	/* Später differenzieren? */
			}
			return (0);
		}
		/* SCSI-Error occured */
		set_error(&SRBlock, sp);

		if (SRBlock.u.cmd.target_status == SRB_CheckStatus) {	/* Sense data valid */
			sp->sense_count	= (int)SRBlock.u.cmd.sense_len;
			if (sp->sense_count > sp->sense_len)
				sp->sense_count = sp->sense_len;

			ptr = (UCHAR*)&SRBlock.u.cmd.cdb_st;
			ptr += SRBlock.u.cmd.cdb_len;

			fillbytes(&sp->u_sense.Sense, sizeof (sp->u_sense.Sense), '\0');
			movebytes(ptr, &sp->u_sense.Sense, sp->sense_len);

			sp->u_scb.cmd_scb[0] = SRBlock.u.cmd.target_status;
		}
		if (sp->flags & SCG_RECV_DATA) {
			if (sp->addr && sp->size) {
				if (scgp->bufbase != sp->addr)	/* Copy only if data not in ASPI-Mem */
					movebytes(scgp->bufbase, sp->addr, SRBlock.u.cmd.data_len);
			}
		}
#ifdef	really
		sp->resid	= SRBlock.u.cmd.data_len; /* XXXXX Got no Data ????? */
#else
		sp->resid	= sp->size - SRBlock.u.cmd.data_len;
#endif
		return (1);
	}
}

/***************************************************************************
 *									   *
 *  BOOL open_driver()							   *
 *									   *
 *  Opens the ASPI Router device driver and sets device_handle.		   *
 *  Returns:								   *
 *    TRUE - Success							   *
 *    FALSE - Unsuccessful opening of device driver			   *
 *									   *
 *  Preconditions: ASPI Router driver has be loaded			   *
 *									   *
 ***************************************************************************/
LOCAL BOOL
open_driver(scgp)
	SCSI	*scgp;
{
	ULONG	rc;			/* return value */
	ULONG	ActionTaken;		/* return value	*/
	USHORT	openSemaReturn;		/* return value	*/
	ULONG	cbreturn;
	ULONG	cbParam;

	if (driver_handle)		/* ASPI-Router already opened	*/
		return (TRUE);

	rc = DosOpen((PSZ) "aspirou$",	/* open driver*/
			&driver_handle,
			&ActionTaken,
			0,
			0,
			FILE_OPEN,
			OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE,
			NULL);
	if (rc) {
		js_fprintf((FILE *)scgp->errfile,
				"Cannot open ASPI-Router!\n");

		return (FALSE);		/* opening failed -> return false*/
	}

	/* Init semaphore */
	if (DosCreateEventSem(NULL, &postSema,	/* create event semaphore */
				DC_SEM_SHARED, 0)) {
		DosClose(driver_handle);
		js_fprintf((FILE *)scgp->errfile,
				"Cannot create event semaphore!\n");

		return (FALSE);
	}
	rc = DosDevIOCtl(driver_handle, 0x92, 0x03,	/* pass semaphore handle */
			(void*) &postSema, sizeof (HEV), /* to driver		 */
			&cbParam, (void*) &openSemaReturn,
			sizeof (USHORT), &cbreturn);

	if (rc||openSemaReturn) {			/* Error */
		DosCloseEventSem(postSema);
		DosClose(driver_handle);
		return (FALSE);
	}
	return (TRUE);
}

/***************************************************************************
 *									   *
 *  BOOL close_driver()							   *
 *									   *
 *  Closes the device driver						   *
 *  Returns:								   *
 *    TRUE - Success							   *
 *    FALSE - Unsuccessful closing of device driver			   *
 *									   *
 *  Preconditions: ASPI Router driver has be opened with open_driver	   *
 *									   *
 ***************************************************************************/
LOCAL BOOL
close_driver()
{
	ULONG rc;				/* return value */

	if (driver_handle) {
		rc = DosClose(driver_handle);
		if (rc)
			return (FALSE);		/* closing failed -> return false */
		driver_handle = 0;
		if (DosCloseEventSem(postSema))
			js_fprintf(stderr, "Cannot close event semaphore!\n");
		if (buffer && DosFreeMem(buffer)) {
			js_fprintf(stderr,
			"Cannot free buffer memory for ASPI-Router!\n"); /* Free our memory buffer if not already done */
		}
		buffer = NULL;
	}
	return (TRUE);
}

LOCAL ULONG
wait_post(ULONG ulTimeOut)
{
	ULONG count = 0;
	ULONG rc;					/* return value */

/*	rc = DosWaitEventSem(postSema, -1);*/		/* wait forever*/
	rc = DosWaitEventSem(postSema, ulTimeOut);
	DosResetEventSem(postSema, &count);
	return (rc);
}

LOCAL BOOL
init_buffer(mem)
	void	*mem;
{
	ULONG	rc;					/* return value */
	USHORT lockSegmentReturn;			/* return value */
	Ulong	cbreturn;
	Ulong	cbParam;

	rc = DosDevIOCtl(driver_handle, 0x92, 0x04,	/* pass buffers pointer */
			(void*) mem, sizeof (void*),	/* to driver */
			&cbParam, (void*) &lockSegmentReturn,
			sizeof (USHORT), &cbreturn);
	if (rc)
		return (FALSE);				/* DosDevIOCtl failed */
	if (lockSegmentReturn)
		return (FALSE);				/* Driver could not lock segment */
	return (TRUE);
}
#define	sense	u_sense.Sense
