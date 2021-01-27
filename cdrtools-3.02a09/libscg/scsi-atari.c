/* @(#)scsi-atari.c	1.3 09/07/13 Copyright 1997,2000,2009 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-atari.c	1.3 09/07/13 Copyright 1997,2000,2009 J. Schilling";
#endif
/*
 *	Interface for Atari generic SCSI implementation.
 *
 *	Copyright (c) 1997, 2000, 2009 J. Schilling
 *	Atari systems support code written by Yvan Doyeux
 *
 *
 *  SCSIDRV driver is needed to enable libscg on Atari computers.
 *  I need to pass through assembler to avoid using of -mshort GCC option.
 *  Actually SCSIDRV requires real 16-bit word for several function arguments.
 *
 *  If you are using HDDRIVER, it already includes the SCSIDRV programming interface,
 *  otherwise you must run SCSIDRV.PRG before.
 *
 *  The following bus-numbers are reserved for this:
 *  0 (ACSI)
 *  1 (Standard SCSI: Falcon, TT, Medusa, MagicMac...)
 *  2 (IDE in Atari-compatibles)
 *
 *  For further information, see packages SCSIDRV.ZIP and CBHD502.ZIP.
 *
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
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

#include <schily/stdio.h>
#include <schily/string.h>
#include <osbind.h>
#include <schily/signal.h>
#include <schily/stdlib.h>

#define	cdecl

#define	BOOLEAN	long
#define	BYTE	char			/* Signed byte			*/
#define	UBYTE	unsigned char		/* Unsigned byte		*/
#define	WORD	short			/* Signed word (16 bits)	*/
#define	UWORD	unsigned short		/* Unsigned word		*/

#define	SCSIRevision 0x0101

#define	MAXBUSNO	31
#define	MAX_SCSIDRV_HANDLE    32

#define	MAX_SCG		3	/* Max # of SCSI controllers */
#define	MAX_TGT		8
#define	MAX_LUN		1

/*
 * SCSIDRV return values
 */
#define	CHECKCONDITION	 2L
#define	NOSCSIERROR	 0L
#define	SELECTERROR	-1L
#define	STATUSERROR	-2L
#define	PHASEERROR	-3L
#define	BSYERROR	-4L
#define	BUSERROR	-5L
#define	TRANSERROR	-6L
#define	FREEERROR	-7L
#define	TIMEOUTERROR	-8L
#define	DATATOOLONG	-9L
#define	LINKERROR	-10L
#define	TIMEOUTARBIT	-11L
#define	PENDINGERROR	-12L
#define	PARITYERROR	-13L

LOCAL	char	_scg_trans_version[] = "scsi-atari.c-1.3";	/* The version for this transport */
LOCAL	char	_scg_auth[] = "Yvan Doyeux";


typedef struct {
	unsigned long	hi;
	unsigned long	lo;
} DLONG;

typedef struct {
	unsigned long	BusIds;
	BYTE		resrvd[28];
} tPrivate;

typedef WORD *tHandle;

typedef struct {
	tHandle		Handle;
	BYTE		*Cmd;
	UWORD		CmdLen;
	void		*Buffer;
	unsigned long	TransferLen;
	BYTE		*SenseBuffer;
	unsigned long	Timeout;
	UWORD		Flags;
#define	Disconnect	0x10
} tSCSICmd;
typedef tSCSICmd *tpSCSICmd;


typedef struct {
	tPrivate	Private;
	char		BusName[20];
	UWORD		BusNo;
	UWORD		Features;
#define	cArbit		0x01
#define	cAllCmds	0x02
#define	cTargCtrl	0x04
#define	cTarget		0x08
#define	cCanDisconnect	0x10
#define	cScatterGather	0x20
	unsigned long	MaxLen;
} tBusInfo;


typedef struct {
	BYTE	Private[32];
	DLONG	SCSIId;
} tDevInfo;


typedef struct ttargethandler {
	struct  ttargethandler *next;
	BOOLEAN cdecl (*TSel) 	(WORD bus, UWORD CSB, UWORD CSD);
	BOOLEAN cdecl (*TCmd) 	(WORD bus, BYTE	*Cmd);
	UWORD   cdecl (*TCmdLen) (WORD bus, UWORD Cmd);
	void	cdecl (*TReset)	(UWORD bus);
	void	cdecl (*TEOP) 	(UWORD bus);
	void	cdecl (*TPErr)	(UWORD bus);
	void	cdecl (*TPMism)	(UWORD bus);
	void	cdecl (*TBLoss)	(UWORD bus);
	void	cdecl (*TUnknownInt) (UWORD bus);
} tTargetHandler;

typedef tTargetHandler *tpTargetHandler;

typedef BYTE tReqData[18];

typedef struct
{
	UWORD	Version;

				/* Routinen als Initiator */
	long	cdecl (*In)	(tpSCSICmd Parms);
	long	cdecl (*Out)	(tpSCSICmd Parms);

	long	cdecl (*InquireSCSI)	(WORD what, tBusInfo *Info);
#define	cInqFirst	0
#define	cInqNext	1
	long	cdecl (*InquireBus)	(WORD what, WORD BusNo, tDevInfo *Dev);
	long	cdecl (*CheckDev)	(WORD BusNo, const DLONG *SCSIId, char *Name, UWORD *Features);
	long	cdecl (*RescanBus)	(WORD BusNo);
	long	cdecl (*Open)		(WORD BusNo, const DLONG *SCSIId, unsigned long *MaxLen);
	long	cdecl (*Close)		(tHandle handle);
	long	cdecl (*Error)		(tHandle handle, WORD rwflag, WORD ErrNo);
#define	cErrRead	0
#define	cErrWrite	1
#define	cErrMediach	0
#define	cErrReset	1

	long	cdecl (*Install)	(WORD bus, tpTargetHandler Handler);
	long	cdecl (*Deinstall)	(WORD bus, tpTargetHandler Handler);
	long	cdecl (*GetCmd)		(WORD bus, BYTE *Cmd);
	long	cdecl (*SendData)	(WORD bus, BYTE *Buffer, unsigned long Len);
	long	cdecl (*GetData)	(WORD bus, void *Buffer, unsigned long Len);
	long	cdecl (*SendStatus)	(WORD bus, UWORD Status);
	long	cdecl (*SendMsg)	(WORD bus, UWORD Msg);
	long	cdecl (*GetMsg)		(WORD bus, UWORD *Msg);

	tReqData	*ReqData;
} tScsiCall;

typedef tScsiCall *tpScsiCall;


LOCAL	int	num_handle_opened = 0;
LOCAL	tHandle tab_handle_opened[MAX_SCSIDRV_HANDLE];
LOCAL	SCSI	*scgp_local;

tpScsiCall scsicall;

struct bus_struct {
	tBusInfo bus;
	tDevInfo *dev[MAX_TGT];
};

struct	scg_local {
		struct bus_struct *atari_bus[MAX_SCG];
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
	__scg_help(f, "SCSIDRV Interface", "Atari generic SCSI implementation",
		"", "bus,target,lun", "1,2,0", TRUE, FALSE);
	printf("\nSCSIDRV driver is needed to enable libscg on Atari computers.\n");
	printf("If you are using HDDRIVER, it already includes the SCSIDRV\n");
	printf("programming interface otherwise you must run SCSIDRV.PRG before.\n");
	printf("The following bus-numbers are reserved for this:\n");
	printf("0 (ACSI)\n");
	printf("1 (Standard SCSI: Falcon, TT, Medusa, MagicMac...)\n");
	printf("2 (IDE in Atari-compatibles)\n");
	printf("For further information, see packages SCSIDRV.ZIP and CBHD502.ZIP.\n");

	return (0);
}


LOCAL void
scsidrv_close()
{
	long	rc;

	rc = (long)scsicall->Close;
	{
	register long retvalue __asm__("d0");

	/* BEGIN CSTYLED */
	__asm__ volatile ("
		movl	%1,sp@-;
		jbsr	(%2);
		lea	sp@(4),sp "
	: "=r"(retvalue)			/* outputs */
	: "r"((tHandle) scgp_local->fd),"a"(rc)	/* inputs  */
	: "d0", "d1", "d2", "a0", "a1", "a2"	/* clobbered regs */
	AND_MEMORY);
	/* END CSTYLED */
	}

	num_handle_opened = num_handle_opened - 1;
	if ((num_handle_opened-1) >= 0)
		scgp_local->fd = (int)tab_handle_opened[num_handle_opened-1];
	else
		scgp_local->fd = 0;
}

LOCAL void
atari_free_scsi_all()
{
	int	i;
	int	j;

	for (j = 0; j < MAX_SCG; j++) {
		if (scglocal(scgp_local)->atari_bus[j] != (struct bus_struct *)-1) {
			for (i = 0; i < MAX_TGT; i++) {
				if (scglocal(scgp_local)->atari_bus[j]->dev[i] != (tDevInfo *)-1) {
					free(scglocal(scgp_local)->atari_bus[j]->dev[i]);
				}
			}
			free(scglocal(scgp_local)->atari_bus[j]);
		}
	}
	if (scgp_local->local) {
		free(scgp_local->local);
	}

	scgo_freebuf(scgp_local);
}



LOCAL int
scgo_open(scgp, device)
	SCSI	*scgp;
	char	*device;

{
	/* SCSIDRV present ? */
	BOOL		scsidrv = FALSE;
	BOOL		nopen = FALSE;
	void		*oldstack;
	long		rc_bus;
	int		i;
	int		j;
	tBusInfo	Bus;
	WORD		Inq_bus = cInqFirst;
	long		cookie_value;

	scsicall = NULL;
	scgp_local = scgp;

	if (Getcookie(0x53435349L, &cookie_value) == 0) {
		scsicall = (tpScsiCall)cookie_value;
		printf("SCSIDRV found on your ATARI!\n");
		scsidrv = TRUE;
	} else {
		printf("I can't find SCSIDRV on your ATARI...\n");
		scsidrv = FALSE;
		nopen = FALSE;
	}


	if (scsidrv == TRUE) {
		if (scgp->local == NULL) {
			scgp->local = malloc(sizeof (struct scg_local));
			if (scgp->local == NULL)
				return (0);
			for (j = 0; j < MAX_SCG; j++) {
				scglocal(scgp)->atari_bus[j] = (struct bus_struct *)-1;
			}
		}



	oldstack = (void *)Super(NULL);
	rc_bus = (long)scsicall->InquireSCSI; /* busses available */
	/*
	 * InquireSCSI
	 */
	{
	register long ret_bus_value __asm__("d0") = 0; /* Return of InquireSCSI */
	long ret_bus = 0;

	while (ret_bus == 0) {

	/* BEGIN CSTYLED */
	__asm__ volatile ("
		movl	%1,sp@-;	/* Pointer of bus structure (32-bit long)*/
		movw	%3,sp@-;	/* Inq_bus argument (16-bit WORD) */
		jbsr	(%2);		/* Call to InquireSCSI SCSIDRV function */
		lea	sp@(6),sp	/* Fix the stack */ "
	: "=r"(ret_bus_value)			/* outputs */
	: "r"(&Bus),"a"(rc_bus), "r"(Inq_bus)	/* inputs  */
	: "d0", "d1", "d2", "a0", "a1", "a2"	/* clobbered regs */
	AND_MEMORY);
	/* END CSTYLED */

	ret_bus = ret_bus_value;
	if (ret_bus == 0) {
		scglocal(scgp)->atari_bus[Bus.BusNo] = malloc(sizeof (struct bus_struct));
		memcpy(&scglocal(scgp)->atari_bus[Bus.BusNo]->bus, &Bus, sizeof (tBusInfo));
			for (i = 0; i < MAX_TGT; i++) {
				scglocal(scgp)->atari_bus[Bus.BusNo]->dev[i] = (tDevInfo *)-1;
			}

			{
			long rc_dev;
			WORD Inq_dev = cInqFirst;
			register long ret_dev_value __asm__("d0") = 0;
			long ret_dev = 0;
			tDevInfo Dev;
			rc_dev = (long)scsicall->InquireBus; /* we inquiry for devices in bus Bus.BusNo */
			while (ret_dev == 0) {

			/* BEGIN CSTYLED */
			__asm__ volatile ("
				movl	%2,sp@-;
				movw	%1,sp@-;
				movw	%4,sp@-;
				jbsr	(%3);
				lea	sp@(8),sp "
			: "=r"(ret_dev_value)			/* outputs */
			: "r"(Bus.BusNo),"r"(&Dev),"a"(rc_dev), "r"(Inq_dev)		/* inputs  */
			: "d0", "d1", "d2", "a0", "a1", "a2"	/* clobbered regs */
			AND_MEMORY);
			/* END CSTYLED */

			ret_dev = ret_dev_value;
			if (ret_dev == 0) {
				scglocal(scgp)->atari_bus[Bus.BusNo]->dev[(int)Dev.SCSIId.lo] = malloc(sizeof (tDevInfo));
				memcpy(scglocal(scgp)->atari_bus[Bus.BusNo]->dev[(int)Dev.SCSIId.lo], &Dev, sizeof (tDevInfo));
			}
			Inq_dev = cInqNext;
			}
			}

	}
	Inq_bus = cInqNext;
	}
	}


		Super(oldstack);

		atexit(scsidrv_close);
		atexit(atari_free_scsi_all);

		nopen = TRUE;
	}
	return (nopen);

}



LOCAL int
scgo_close(SCSI * scgp)
{
	scsidrv_close();
	atari_free_scsi_all();
	return (0);
}


LOCAL long
scgo_maxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	/* return (63*1024); */
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
	long		rc;
	unsigned long	MaxLen;
	int		fd = 0;
	tBusInfo	Bus;
	void		*oldstack;

	if (scgp_local->fd > 0) {	/* handle exists ? */
		scsidrv_close();
	}

	if (busno < 0 || busno >= MAX_SCG ||
	    tgt < 0 || tgt >= MAX_TGT ||
	    tlun < 0 || tlun >= MAX_LUN) {
		errno = EINVAL;
		return (-1);
	}

	if (scglocal(scgp)->atari_bus[busno] == (struct bus_struct *)-1) {
		errno = EINVAL;
		return (-1);
	}

	if ((struct tDevInfo *) scglocal(scgp)->atari_bus[busno]->dev[tgt] == (struct tDevInfo *)-1) {
		errno = EINVAL;
		return (-1);
	}


	oldstack = (void *)Super(NULL);
	rc = (long)scsicall->Open;
	{
	register long retvalue __asm__("d0") = 0;

	/* BEGIN CSTYLED */
	__asm__ volatile ("
		movl	%3,sp@-;
		movl	%2,sp@-;
		movw	%1,sp@-;
		jbsr	(%4);
		lea	sp@(10),sp "
	: "=r"(retvalue)			/* outputs */
	: "r"(scglocal(scgp)->atari_bus[busno]->bus.BusNo),
	"r"(&scglocal(scgp)->atari_bus[busno]->dev[tgt]->SCSIId),"r"(&MaxLen),"a"(rc)		/* inputs  */
	: "d0", "d1", "d2", "a0", "a1", "a2"	/* clobbered regs */
	AND_MEMORY);
	/* END CSTYLED */

	fd = (int)retvalue;
	}


	Super(oldstack);

	if (fd > 0) {
		tab_handle_opened[num_handle_opened] = (tHandle)fd;
		num_handle_opened = num_handle_opened + 1;
		return (fd);
	} else {
		return (-1);
	}
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
	/* XXX synchronous reset command - is this wise? */
	errno = EINVAL;
	return (-1);
}


LOCAL int
scgo_send(scgp)
	SCSI	*scgp;
{
	long		rc;
	void		*oldstack;
	tSCSICmd	cmd;
	tBusInfo	Bus;
	tDevInfo	Dev;
	unsigned long	MaxLen;
	long		ret = 0;
	BYTE		reqbuff[18];

	scgp->scmd->ux_errno = 0;

	if (scgp->fd <= 0) {
		scgp->scmd->error = SCG_FATAL;
		return (0);
	}


	oldstack = (void *)Super(NULL);
	cmd.Handle = (tHandle) scgp->fd;
	cmd.Cmd = malloc(scgp->scmd->cdb_len);
	movebytes(&scgp->scmd->cdb, cmd.Cmd, scgp->scmd->cdb_len);
	cmd.CmdLen = /* (UWORD) */ scgp->scmd->cdb_len;
	cmd.Buffer = scgp->scmd->addr;
	cmd.TransferLen = scgp->scmd->size;
	cmd.SenseBuffer = scgp->scmd->u_sense.cmd_sense;
	fillbytes(cmd.SenseBuffer, sizeof (reqbuff), '\0');
	cmd.Timeout = scgp->scmd->timeout * 200;
	cmd.Flags = scgp->scmd->flags & SCG_RECV_DATA;
	scgp->scmd->u_scb.cmd_scb[0] = 0;


	if ((scgp->scmd->flags & SCG_RECV_DATA)) {
		rc = (long)scsicall->In;
	{
	register long retvalue __asm__("d0");

	/* BEGIN CSTYLED */
	__asm__ volatile ("
		movl	%1,sp@-;
		jbsr	(%2);
		lea	sp@(4),sp "
	: "=r"(retvalue)			/* outputs */
	: "r"(&cmd),"a"(rc)			/* inputs  */
	: "d0", "d1", "d2", "a0", "a1", "a2"	/* clobbered regs */
	AND_MEMORY);
	/* END CSTYLED */

	ret = retvalue;
	}
	} else if (scgp->scmd->size > 0) {

	rc = (long)scsicall->Out;
	{
	register long retvalue __asm__("d0");

	/* BEGIN CSTYLED */
	__asm__ volatile ("
		movl	%1,sp@-;
		jbsr	(%2);
		lea	sp@(4),sp "
	: "=r"(retvalue)			/* outputs */
	: "r"(&cmd),"a"(rc)			/* inputs  */
	: "d0", "d1", "d2", "a0", "a1", "a2"	/* clobbered regs */
	AND_MEMORY);
	/* END CSTYLED */

	ret = retvalue;
	}
	} else {

		rc = (long)scsicall->Out;
		{
		register long retvalue __asm__("d0");

		/* BEGIN CSTYLED */
		__asm__ volatile ("
			movl	%1,sp@-;
			jbsr	(%2);
			lea	sp@(4),sp "
		: "=r"(retvalue)			/* outputs */
		: "r"(&cmd),"a"(rc)			/* inputs  */
		: "d0", "d1", "d2", "a0", "a1", "a2"	/* clobbered regs */
		AND_MEMORY);
		/* BEGIN CSTYLED */

		ret = retvalue;
		}
	}

	Super(oldstack);

	scgp->scmd->resid = 0;

	scgp->scmd->sense_count = sizeof (reqbuff);	/* 18 */
	movebytes(cmd.SenseBuffer, &scgp->scmd->u_sense.cmd_sense, sizeof (reqbuff));

	free(cmd.Cmd);

	scgp->scmd->u_scb.cmd_scb[0] = ret;


	switch (ret) {
	case CHECKCONDITION:
	    scgp->scmd->ux_errno = geterrno();
	    scgp->scmd->error = SCG_NO_ERROR;
	    break;
	case TIMEOUTERROR:
	    scgp->scmd->ux_errno = geterrno();
	    scgp->scmd->error = SCG_TIMEOUT;
	    break;
	default:
	    if (ret < 0)
	    {
		scgp->scmd->ux_errno = geterrno();
		scgp->scmd->error = SCG_FATAL;
	    }
	    else
		scgp->scmd->error = SCG_NO_ERROR;
		break;
	}

	return (ret);
}
