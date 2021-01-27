/* @(#)scsi-amigaos.c	1.8 06/11/26 Copyright 1997,2000-2003 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-amigaos.c	1.8 06/11/26 Copyright 1997,2000-2003 J. Schilling";
#endif
/*
 *	Interface for the AmigaOS generic SCSI implementation.
 *
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 *
 *	Copyright (c) 1997, 2000-2003 J. Schilling
 *	AmigaOS support code written by T. Langer
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

#define	BOOL	int

#include <schily/strdefs.h>
#include <exec/ports.h>
#include <exec/io.h>
#include <exec/errors.h>
#include <devices/scsidisk.h>
#include <devices/timer.h>
#include <exec/semaphores.h>
#include <exec/memory.h>
#include <exec/execbase.h>
#include <clib/exec_protos.h>
#include <clib/alib_protos.h>

/*
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 */
LOCAL	char	_scg_trans_version[] = "scsi-amigaos.c-1.8";	/* The version for this transport */
LOCAL	char	_scg_auth[] = "T. Langer";

#define	MAX_SCG		8	/* Max # of SCSI controllers */
#define	MAX_TGT		8
#define	MAX_LUN		8
#define	MAX_DEV		MAX_SCG*MAX_TGT*MAX_LUN

struct scg_local {
	int	scgfiles[MAX_SCG][MAX_TGT][MAX_LUN];
};

#define	scglocal(p)	((struct scg_local *)((p)->local))

#define	MAX_DMA_AMIGAOS (64*1024)

LOCAL struct IOReq {
	struct IOStdReq *ioReq;
	int		ref_count;
} request[MAX_DEV];

LOCAL	char			*devs[MAX_SCG];
LOCAL	struct MsgPort		*ioMsgPort = NULL;
LOCAL	struct timerequest	*timer_io = NULL;
LOCAL	struct MsgPort		*timerMsgPort = NULL;
LOCAL	int			initialized = 0;
LOCAL	int			last_bus = -1;
/* my private var: for debug purpose only */
LOCAL	int			ami_debug = 0;

extern	struct ExecBase		*SysBase;
#define	IOERR_TIMEOUT		(-8)
#define	CHECK_CONDITION		0x02
#define	DUNIT(b, t, l)		(100 * b) + (10 * (l < 0 ? 0:l)) + t

LOCAL	void	amiga_init		__PR((void));
LOCAL	int	amiga_open_scsi		__PR((int bus, int tgt, int lun, SCSI *scgp));
LOCAL	void	amiga_close_scsi	__PR((int fd));
LOCAL	void	amiga_close_scsi_all	__PR((void));
LOCAL	void	amiga_scan_devices	__PR((void));
LOCAL	int	amiga_find_device	__PR((char *device));
LOCAL	int	amiga_open_timer	__PR((void));
LOCAL	void	amiga_close_timer	__PR((void));
LOCAL	int	amiga_get_scsi_bus	__PR((char *device));

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
/*			return (_scg_auth_schily);*/
			return (_scg_auth);
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
	__scg_help(f, "Amiga SCSI", "Generic SCSI",
		"", "bus,target,lun or xxx.device:b,t,l", "1,2,0 or scsi.device:1,2,0", TRUE, FALSE);
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

	if (initialized == 0) {
		amiga_init();
		initialized = 1;
	}

	if (busno >= MAX_SCG || tgt >= MAX_TGT || tlun >= MAX_LUN) {
		errno = EINVAL;
		if (scgp->errstr) {
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Illegal value for busno, target or lun '%d,%d,%d'",
				busno, tgt, tlun);
		}
		return (-1);
	}
	if (scgp->local == NULL) {
		scgp->local = malloc(sizeof (struct scg_local));
		if (scgp->local == NULL)
			return (0);

		for (b = 0; b < MAX_SCG; b++) {
			for (t = 0; t < MAX_TGT; t++) {
				for (l = 0; l < MAX_LUN; l++) {
					scglocal(scgp)->scgfiles[b][t][l] = -1;
				}
			}
		}
	}

	if (device == NULL || *device == '\0') {

		if (last_bus == -1) {
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"No scsi device found");
			return (-1);
		}
		if (busno < 0 && tgt < 0 && tlun < 0) {
			/* cdrecord -scanbus */
			for (b = 0; b <= last_bus; b++) {
				for (t = 0; t < MAX_TGT; t++) {
					for (l = 0; l < MAX_LUN; l++) {
						f = amiga_open_scsi(b, t, l, scgp);
						if (f != -1) {
							scglocal(scgp)->scgfiles[b][t][l] = f;
							nopen++;
						}
					}
				}
			}
		} else {
			/* cdrecord [-scanbus] dev=b,t,l */
			f = amiga_open_scsi(busno, tgt, tlun, scgp);
			if (f != -1) {
				scglocal(scgp)->scgfiles[busno][tgt][tlun] = f;
				nopen++;
			}
		}
	} else {
		if (busno < 0 && tgt < 0 && tlun < 0) {
			/* cdrecord -scanbus dev=xxx.device */
			b = amiga_get_scsi_bus(device);
			if (b != -1) {
				for (t = 0; t < MAX_TGT; t++) {
					for (l = 0; l < MAX_LUN; l++) {
						f = amiga_open_scsi(b, t, l, scgp);
						if (f != -1) {
							scglocal(scgp)->scgfiles[b][t][l] = f;
							nopen++;
						}
					}
				}
			} else {
				js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
					"Scsi device %s not found", device);
			}
		} else {
			/* cdrecord [-scanbus] dev=xxx.device:b,t,l */
			/*
			 * this is a special case, in which the scsi device is accessed just by
			 * name, bus parameter from the command is ignored.
			 */
			b = amiga_get_scsi_bus(device);
			if (b != -1) {
				f = amiga_open_scsi(b, tgt, tlun, scgp);
				if (f != -1) {
					scglocal(scgp)->scgfiles[busno][tgt][tlun] = f;
					nopen++;
				}
			} else {
				js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
					"Scsi device %s not found", device);
			}
		}
	}

	return (nopen);
}

LOCAL int
scgo_close(scgp)
	SCSI	*scgp;
{
	register int    b;
	register int    t;
	register int    l;

	if (scgp->local == NULL)
		return (-1);

	for (b = 0; b < MAX_SCG; b++) {
		for (t = 0; t < MAX_TGT; t++) {
			for (l = 0; l < MAX_LUN; l++) {
				if (scglocal(scgp)->scgfiles[b][t][l] >= 0)
					amiga_close_scsi(scglocal(scgp)->scgfiles[b][t][l]);
				scglocal(scgp)->scgfiles[b][t][l] = -1;
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
	return (MAX_DMA_AMIGAOS);
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
			if (scglocal(scgp)->scgfiles[busno][t][l] != -1)
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
	/* XXX Is there really no reset function on AmigaOS? */
	errno = EINVAL;
	return (-1);
}

LOCAL int
scgo_send(scgp)
	SCSI		*scgp;
{
	register struct IOStdReq *ioreq = NULL;
	struct SCSICmd	Cmd;
	int		ret = 0;
	struct scg_cmd	*sp = scgp->scmd;

	sp->error = SCG_NO_ERROR;
	sp->sense_count = 0;
	sp->u_scb.cmd_scb[0] = 0;
	sp->resid = 0;

	if (scgp->fd < 0) {
		sp->error = SCG_FATAL;
		return (0);
	}
	ioreq = request[scgp->fd].ioReq;
	if (ioreq == NULL) {
		sp->error = SCG_FATAL;
		return (0);
	}
	ioreq->io_Length = sizeof (struct SCSICmd);
	ioreq->io_Data = &Cmd;
	ioreq->io_Command = HD_SCSICMD;

	Cmd.scsi_Flags = SCSIF_AUTOSENSE;	/* We set the SCSI cmd len */
	if (sp->flags & SCG_RECV_DATA)
		Cmd.scsi_Flags |= SCSIF_READ;
	else if (sp->size > 0)
		Cmd.scsi_Flags |= SCSIF_WRITE;

	Cmd.scsi_Command = sp->cdb.cmd_cdb;
	Cmd.scsi_CmdLength = sp->cdb_len;
	Cmd.scsi_Data = (UWORD *) sp->addr;
	Cmd.scsi_Length = sp->size;
	Cmd.scsi_Actual = 0;

	Cmd.scsi_SenseData = sp->u_sense.cmd_sense;
	Cmd.scsi_SenseLength = sp->sense_len;
	Cmd.scsi_SenseActual = 0;
	Cmd.scsi_Status = 0;

	do_scsi_cmd(ioreq, sp->timeout);

	fillbytes(&sp->scb, sizeof (sp->scb), '\0');
	sp->resid = Cmd.scsi_Length - Cmd.scsi_Actual;

	if (sp->resid < 0)
		sp->resid = 0;
	sp->sense_count = Cmd.scsi_SenseActual;
	if (sp->sense_count < 0)
		sp->sense_count;

	if (sp->sense_count > SCG_MAX_SENSE)
		sp->sense_count = SCG_MAX_SENSE;
	sp->u_scb.cmd_scb[0] = Cmd.scsi_Status;

	if (ioreq->io_Error)
		sp->ux_errno = EIO;

	if (ami_debug)
		printf("ioreq->io_Error: %ld; status %ld\n", ioreq->io_Error, Cmd.scsi_Status);

	switch (ioreq->io_Error) {
		case 0:
			sp->error = SCG_NO_ERROR;
			break;
		case IOERR_TIMEOUT:
			sp->error = SCG_TIMEOUT;
			break;
		case HFERR_DMA:
		case IOERR_UNITBUSY:
		case HFERR_Phase:
		case HFERR_Parity:
		case HFERR_BadStatus:
			if (Cmd.scsi_Status == CHECK_CONDITION) {
				sp->error = SCG_NO_ERROR;
			} else {
				sp->error = SCG_RETRYABLE;
			}
			break;
		default:
			/* XXX was kann sonst noch passieren? */
			sp->error = SCG_FATAL;
			break;
	}

	return (ret);
}

LOCAL int
do_scsi_cmd(scsi_io, timeout)
	struct IOStdReq	*scsi_io;
	int		timeout;
{
	ULONG	scsi_flag = 0;
	ULONG	timer_flag = 0;
	ULONG	wait_sigs = 0;
	ULONG	wait_ret = 0;
	int		ret = 0;

	scsi_flag = 1L<<scsi_io->io_Message.mn_ReplyPort->mp_SigBit;
	wait_sigs = scsi_flag;

	SetSignal(0L, scsi_flag);
	SendIO((struct IORequest *)scsi_io);
	if (timer_io) {
		timer_flag = 1L<<timerMsgPort->mp_SigBit;
		wait_sigs |= timer_flag;
		timer_io->tr_time.tv_secs = timeout;
		SetSignal(0L, timer_flag);
		SendIO((struct IORequest *)timer_io);
	}

	wait_ret = Wait(wait_sigs);

	if (wait_ret & scsi_flag) {
		WaitIO((struct IORequest *)scsi_io);
		if (timer_io) {
			if (!CheckIO((struct IORequest *) timer_io)) {
				AbortIO((struct IORequest *)timer_io);
				WaitIO((struct IORequest *)timer_io);
			}
		}
	} else if (wait_ret & timer_flag) {
		WaitIO((struct IORequest *)timer_io);
		if (!CheckIO((struct IORequest *) scsi_io)) {
			AbortIO((struct IORequest *)scsi_io);
			if (scsi_io->io_Error == IOERR_ABORTED) {
				WaitIO((struct IORequest *)scsi_io);
			}
		}
		scsi_io->io_Error = IOERR_TIMEOUT;
	}

	return (scsi_io->io_Error);
}

/*--------------------------------------------------------------------------*/

/* strlwr: seems not to be implemented in ixemul */
LOCAL char *
strlwr(s)
	char *s;
{
	unsigned char *s1;

	s1 = (unsigned char *)s;
	while (*s1) {
		if ((*s1 > ('A'-1)) && (*s1 < ('Z'+1)))
			*s1 += 'a'-'A';
		s1++;
	}
	return (s);
}

/*
 * amiga specific functions
 */
LOCAL void
amiga_init()
{
	memset(request, 0, sizeof (request));
	amiga_scan_devices();
	if (ami_debug)
		printf("scanning bus. %ld device(s) found\n", last_bus + 1);

	atexit(amiga_close_scsi_all);
}

LOCAL void
amiga_scan_devices()
{
	/*
	 * searching all known scsi devices
	 * for first there is only a (full) support for scsi.device and amithlon.device.
	 * This are devices i tested.
	 * All the other devices have to be specified by name (i.e. cdrecord dev=blabla.device:0,0,0)
	 * but didn't appear in the scanbus list. Later they should/may be also added to the
	 * main_dev_list (after testing).
	 */
	const char 	*main_dev_list[] = { "scsi.device", "amithlon.device", NULL };

	int	i;
	char	**main_dev;

	main_dev = (char **)main_dev_list;
	while (*main_dev) {
		if (last_bus == MAX_SCG - 1)
			break;
		if (amiga_find_device(*main_dev)) {
			last_bus++;
			devs[last_bus] = strdup(*main_dev);
			for (i = 2; i < MAX_SCG; i++) {
				char tmp[256];
				if (last_bus == MAX_SCG - 1)
					break;
				if (i == 2) {
					sprintf(tmp, "2nd.%s", *main_dev);
				} else if (i == 3) {
					sprintf(tmp, "3rd.%s", *main_dev);
				} else {
					sprintf(tmp, "%ldth.%s", i, *main_dev);
				}
				if (amiga_find_device(tmp)) {
					last_bus++;
					devs[last_bus] = strdup(tmp);
				} else {
					break;
				}
			}
		}
		main_dev++;
	}
}

LOCAL void
amiga_close_scsi(fd)
	int	fd;
{
	if (request[fd].ref_count > 0) {
		request[fd].ref_count--;
		if (request[fd].ref_count == 0) {
			CloseDevice((struct IORequest *) request[fd].ioReq);
			DeleteStdIO(request[fd].ioReq);
			request[fd].ioReq = NULL;
			if (ami_debug) {
				printf("closing device fd %ld\n", fd);
			}
		}
	}
}

LOCAL void
amiga_close_scsi_all()
{
	int	i;

	for (i = 0; i < MAX_DEV; i++) {
		if (request[i].ioReq != NULL) {
			if (ami_debug) {
				printf("closing device fd %ld\n", i);
			}
			CloseDevice((struct IORequest *) request[i].ioReq);
			DeleteStdIO(request[i].ioReq);
			request[i].ioReq = NULL;
		}
	}

	if (ioMsgPort) {
		DeletePort(ioMsgPort);
		ioMsgPort = NULL;
	}
	amiga_close_timer();

	for (i = 0; i < MAX_SCG; i++) {
		free(devs[i]);
	}
}

LOCAL int
amiga_open_scsi(bus, tgt, lun, scgp)
	int	bus;
	int	tgt;
	int	lun;
	SCSI	*scgp;
{
	int	fd = bus * MAX_TGT * MAX_LUN + tgt * MAX_LUN + lun;
	int	unit = DUNIT(bus, tgt, lun);
	char	*dev = devs[bus];

	if (dev == NULL) {
		if (scgp->errstr) {
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"No scsi device found at bus %ld\n", bus);
		}
		return (-1);
	}

	if (ioMsgPort == NULL) {
		ioMsgPort = CreatePort(NULL, 0);
	}

	if (ioMsgPort != NULL) {
		if (request[fd].ioReq == NULL) {
			request[fd].ioReq = CreateStdIO(ioMsgPort);
		}

		if (request[fd].ioReq != NULL) {
			if (ami_debug)
				printf("trying %s, unit %ld\n", dev, unit);
			if (OpenDevice(dev, unit, (struct IORequest *)request[fd].ioReq, 0L)) {
				if (scgp->errstr) {
					js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
						"Cannot open %s\n",	dev);
				}
				if (request[fd].ref_count == 0) {
					DeleteStdIO(request[fd].ioReq);
					request[fd].ioReq = NULL;
				}
				fd = -1;
			} else {
				request[fd].ref_count++;
				if (ami_debug)
					printf("opening %s, unit %ld as fd %ld count %ld\n", dev, unit, fd, request[fd].ref_count);
			}
		} else {
			if (scgp->errstr) {
				js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
					"Cannot create IOReq");
			}
		}
	} else {
		if (scgp->errstr) {
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Cannot open Message Port");
		}
	}

	return (fd);
}

LOCAL int
amiga_open_timer()
{
	int	ret = 0;

	/* we need the timer to catch scsi timeouts */
	if (timer_io == NULL) {
		if (ami_debug)
			printf("opening timer\n");

		timerMsgPort = CreatePort(NULL, 0);
		if (timerMsgPort) {
			timer_io = (struct timerequest *)CreateExtIO(timerMsgPort, sizeof (struct timerequest));
			if (timer_io) {
				if (OpenDevice("timer.device", UNIT_MICROHZ, (struct IORequest *)timer_io, 0)) {
					printf("Warning: can't open timer device\n");
					DeleteExtIO((struct IORequest *) timer_io);
					DeletePort(timerMsgPort);
					timer_io = NULL;
					timerMsgPort = NULL;
					ret = 1;
				} else {
					timer_io->tr_node.io_Command = TR_ADDREQUEST;
					timer_io->tr_time.tv_micro = 0;
				}
			} else {
				printf("Warning: can't create timer request\n");
				DeletePort(timerMsgPort);
				timerMsgPort = NULL;
				ret = 1;
			}
		} else {
			printf("Warning: can't create timer port\n");
			ret = 1;
		}
	}
	return (ret);
}

LOCAL void
amiga_close_timer()
{
	if (timer_io) {
		if (ami_debug)
			printf("closing timer\n");

		AbortIO((struct IORequest *) timer_io);
		WaitIO((struct IORequest *) timer_io);
		CloseDevice((struct IORequest *)timer_io);
		DeleteExtIO((struct IORequest *) timer_io);
		DeletePort(timerMsgPort);
		timer_io = NULL;
		timerMsgPort = NULL;
	}
}

LOCAL int
amiga_get_scsi_bus(device)
	char	*device;
{
	int 	i;
	char	*tmp = strdup(device);

	strlwr(tmp);
	for (i = 0; i <= last_bus; i++) {
		if (strcmp(devs[i], tmp) == 0) {
			return (i);
		}
	}

	if (amiga_find_device(tmp)) {
		devs[i] = tmp;
		last_bus = i;
		return (i);
	}

	return (-1);
}

LOCAL int
amiga_find_device(device)
	char	*device;
{
	char		tmp[256];
	struct Node	*DeviceLibNode = SysBase->DeviceList.lh_Head;

	if (ami_debug)
		printf("looking for %s ", device);

	Forbid();
	while (DeviceLibNode->ln_Succ) {
		DeviceLibNode = DeviceLibNode->ln_Succ;
		strcpy(tmp, DeviceLibNode->ln_Name);
		strlwr(tmp);
		if (strcmp(tmp, device) == 0) {
			if (ami_debug)
				printf(" ... found\n");
			return (1);
		}
	}
	Permit();

	if (ami_debug)
		printf(" ... not found\n");

	return (0);
}
