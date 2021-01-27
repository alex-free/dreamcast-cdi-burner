/* @(#)scsi-mac-iokit.c	1.17 10/05/24 Copyright 1997,2001-2010 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-mac-iokit.c	1.17 10/05/24 Copyright 1997,2001-2010 J. Schilling";
#endif
/*
 *	Interface to the Darwin IOKit SCSI drivers
 *
 *	Notes: Uses the IOKit/scsi-commands/SCSITaskLib interface
 *
 *	As of October 2001, this interface does not support SCSI parallel bus
 *	(old-fashioned SCSI). It does support ATAPI, Firewire, and USB.
 *
 *	First version done by Constantine Sapuntzakis <csapuntz@Stanford.EDU>
 *
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 *
 *	Copyright (c) 1997,2001-2010 J. Schilling
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
LOCAL	char	_scg_trans_version[] = "scsi-mac-iokit.c-1.17";	/* The version for this transport */

#define	MAX_SCG		16	/* Max # of SCSI controllers */
#define	MAX_TGT		16
#define	MAX_LUN		8

#include <schily/stat.h>
#include <mach/mach.h>
#include <Carbon/Carbon.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOCFPlugIn.h>
/*
 * IOKit/scsi-commands/ (being a symlink at least between Panther and Leopard)
 * did disappear on "Snow Leopard" but we do not know whether IOKit/scsi/
 * exist before. I do not like to create an autoconf test for the file,
 * please report if you have problems on older Mac OS X releases.
 */
/*#include <IOKit/scsi-commands/SCSITaskLib.h>*/
#include <IOKit/scsi/SCSITaskLib.h>
#include <mach/mach_error.h>

struct scg_if {
	MMCDeviceInterface	**mmcDeviceInterface;
	SCSITaskDeviceInterface	**scsiTaskDeviceInterface;
	int			flags;
};

/*
 * Defines for flags
 */
#define	NO_ACCESS	0x01

struct scg_local {
	struct scg_if		scg_if[MAX_SCG][MAX_TGT];
	mach_port_t		masterPort;
};
#define	scglocal(p)	((struct scg_local *)((p)->local))

#define	MAX_DMA_NEXT	(32*1024)
#if 0
#define	MAX_DMA_NEXT	(64*1024)	/* Check if this is not too big */
#endif

LOCAL	int	iokit_open	__PR((SCSI *scgp, BOOL bydev, char *device, int busno, int tgt, int tlun));
LOCAL	void	iokit_warn	__PR((SCSI *scgp));


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
	__scg_help(f, "SCSITaskDeviceInterface", "Apple SCSI",
		"", "Mac Prom device name", "IOCompactDiscServices/0 or IODVDServices/0 or IOBDServices/0",
								FALSE, FALSE);
	return (0);
}

LOCAL char *devnames[] = {
		"IOCompactDiscServices",
		"IODVDServices",
		"IOBDServices",
		0
};
#define	NDEVS	((int)(sizeof (devnames) / sizeof (devnames[0]) - 1))

/*
 * Valid Device names:
 *    IOCompactDiscServices
 *    IODVDServices
 *    IOSCSIPeripheralDeviceNub
 *
 * Also a / and a number can be appended to refer to something
 * more than the first device (e.g. IOCompactDiscServices/5 for the 5th
 * compact disc attached)
 */
LOCAL int
scgo_open(scgp, device)
	SCSI	*scgp;
	char	*device;
{
		int	busno	= scg_scsibus(scgp);
		int	tgt	= scg_target(scgp);
		int	tlun	= scg_lun(scgp);
		int	b;
		int	t;
		int	nopen = 0;
		int	ret;

	if (busno >= MAX_SCG || busno >= NDEVS || tgt >= MAX_TGT || tlun >= MAX_LUN) {
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
		if (scgp->local == NULL) {
			if (scgp->errstr)
				js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE, "No memory for scg_local");
			return (0);
		}
		for (b = 0; b < MAX_SCG; b++) {
			for (t = 0; t < MAX_TGT; t++) {
				scglocal(scgp)->scg_if[b][t].mmcDeviceInterface = NULL;
				scglocal(scgp)->scg_if[b][t].scsiTaskDeviceInterface = NULL;
				scglocal(scgp)->scg_if[b][t].flags = 0;
			}
		}
	}

	if ((device != NULL && *device != '\0') || (busno == -2 && tgt == -2))
		goto openbydev;

	if (busno >= 0 && tgt >= 0 && tlun >= 0) {
		ret = iokit_open(scgp, FALSE, devnames[busno], busno, tgt, tlun);
		if (ret == 1)
			scg_settarget(scgp, busno, tgt, tlun);
		else
			scgo_close(scgp);
		if (scgp->fd == -2)
			iokit_warn(scgp);
		return (ret);
	} else {
		int	errsav = 0;
		int	exwarn = 0;

		for (b = 0; b < NDEVS; b++) {
			for (t = 0; t < MAX_TGT; t++) {
				ret = iokit_open(scgp, FALSE, devnames[b], b, t, 0);
				if ((scgp->debug > 0 && ret > 0) || scgp->debug > 2) {
					js_fprintf((FILE *)scgp->errfile,
							"b %d t %d ret %d fd %d\n",
							b, t, ret, scgp->fd);
				}
				if (ret == 1)
					nopen++;
				if (exwarn == 0 && scgp->fd == -2) {
					iokit_warn(scgp);
					exwarn++;
				}
			}
		}
		seterrno(errsav);
	}
openbydev:
	if (nopen == 0) {
		if (device == NULL || device[0] == '\0')
			return (0);

		ret = iokit_open(scgp, TRUE, device, 0, 0, 0);
		if (ret == 1)
			nopen++;
		if (scgp->fd == -2)
			iokit_warn(scgp);
	}
	if (nopen <= 0)
		scgo_close(scgp);
	return (nopen);
}

LOCAL void
iokit_warn(scgp)
	SCSI	*scgp;
{
	js_fprintf((FILE *)scgp->errfile, "\nWarning, 'diskarbitrationd' is running and does not allow us to\n");
	js_fprintf((FILE *)scgp->errfile, "send SCSI commands to the drive.\n");
	js_fprintf((FILE *)scgp->errfile, "To allow us to send SCSI commands, do the following:\n");
	js_fprintf((FILE *)scgp->errfile, "Eject all removable media, then call as root:\n");
	js_fprintf((FILE *)scgp->errfile, "	kill -STOP `(ps -ef | grep diskarbitrationd | awk '{ print $2 }')`\n");
	js_fprintf((FILE *)scgp->errfile, "then re-run the failed command. To continue 'diskarbitrationd' call as root:\n");
	js_fprintf((FILE *)scgp->errfile, "	kill -CONT `(ps -ef | grep diskarbitrationd | awk '{ print $2 }')`\n\n");
}

LOCAL int
iokit_open(scgp, bydev, device, busno, tgt, tlun)
	SCSI	*scgp;
	BOOL	bydev;
	char	*device;
	int	busno;
	int	tgt;
	int	tlun;
{
	mach_port_t masterPort = 0;
	io_iterator_t scsiObjectIterator = 0;
	IOReturn ioReturnValue = kIOReturnSuccess;
	CFMutableDictionaryRef dict = NULL;
	io_object_t scsiDevice = 0;
	HRESULT plugInResult;
	IOCFPlugInInterface **plugInInterface = NULL;
	MMCDeviceInterface **mmcDeviceInterface = NULL;
	SCSITaskDeviceInterface **scsiTaskDeviceInterface = NULL;
	SInt32 score = 0;
	int err = -1;
	char *realdevice = device;
	char *tmp;
	int idx;

	if (scgp->local == NULL)
		return (err);

	if (bydev) {
		realdevice = tmp = strdup(device);
		if (realdevice == NULL)
			return (err);
		tmp = strchr(tmp, '/');
		if (tmp != NULL) {
			*tmp++ = '\0';
			tgt = atoi(tmp);
		}
	}

	/*
	 * Get master port handle
	 * The master port handle is deallocated in scgo_close()
	 */
	masterPort = scglocal(scgp)->masterPort;
	if (!masterPort) {
		ioReturnValue = IOMasterPort(bootstrap_port, &masterPort);

		if (ioReturnValue != kIOReturnSuccess) {
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				    "Couldn't get a master IOKit port. Error %d",
				    ioReturnValue);
			if (bydev)
				free(realdevice);
			return (err);
		}
		scglocal(scgp)->masterPort = masterPort;
	}

	/*
	 * Get Service dict for "IOCompactDiscServices" or "IODVDServices"
	 * or "IODBDServices"
	 */
	dict = IOServiceMatching(realdevice);
	if (dict == NULL) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "Couldn't create dictionary for searching '%s'.",
			    realdevice);
		goto out;
	}

	ioReturnValue = IOServiceGetMatchingServices(masterPort, dict,
						    &scsiObjectIterator);
	dict = NULL;

	if (scsiObjectIterator == 0 ||
	    (ioReturnValue != kIOReturnSuccess)) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "No matching device %s/%d found.",
					realdevice, tgt);
		goto out;
	}

	for (idx = 0; (scsiDevice = IOIteratorNext(scsiObjectIterator)) != 0; idx++) {
		if (idx == tgt)
			break;
		IOObjectRelease(scsiDevice);
		scsiDevice = 0;
		idx++;
	}

	if (scsiDevice == 0) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "No matching device %s/%d found. Iterator failed.",
					realdevice, tgt);
		goto out;
	}

	ioReturnValue = IOCreatePlugInInterfaceForService(scsiDevice,
			kIOMMCDeviceUserClientTypeID,
			kIOCFPlugInInterfaceID,
			&plugInInterface, &score);
	if (ioReturnValue != kIOReturnSuccess) {
		goto try_generic;
	}

	plugInResult = (*plugInInterface)->QueryInterface(plugInInterface,
				CFUUIDGetUUIDBytes(kIOMMCDeviceInterfaceID),
				(LPVOID)&mmcDeviceInterface);

	if (plugInResult != KERN_SUCCESS) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "Unable to get MMC Interface: 0x%lX",
			    (long)plugInResult);

		goto out;
	}

	scsiTaskDeviceInterface =
		(*mmcDeviceInterface)->GetSCSITaskDeviceInterface(mmcDeviceInterface);

	if (scsiTaskDeviceInterface == NULL) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "Failed to get taskDeviceInterface");
		goto out;
	}

	goto init;

try_generic:
	ioReturnValue = IOCreatePlugInInterfaceForService(scsiDevice,
					kIOSCSITaskDeviceUserClientTypeID,
					kIOCFPlugInInterfaceID,
					&plugInInterface, &score);
	if (ioReturnValue != kIOReturnSuccess) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "Unable to get plugin Interface: %x",
			    ioReturnValue);
		goto out;
	}

	plugInResult = (*plugInInterface)->QueryInterface(plugInInterface,
			    CFUUIDGetUUIDBytes(kIOSCSITaskDeviceInterfaceID),
					(LPVOID)&scsiTaskDeviceInterface);

	if (plugInResult != KERN_SUCCESS) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "Unable to get generic Interface: 0x%lX",
			    (long)plugInResult);

		goto out;
	}

init:
	ioReturnValue =
		(*scsiTaskDeviceInterface)->ObtainExclusiveAccess(scsiTaskDeviceInterface);

	if (ioReturnValue != kIOReturnSuccess) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "Unable to get exclusive access to device");
		scglocal(scgp)->scg_if[busno][tgt].flags |= NO_ACCESS;
		scg_settarget(scgp, busno, tgt, tlun);
		goto out;
	}

	if (mmcDeviceInterface) {
		(*mmcDeviceInterface)->AddRef(mmcDeviceInterface);
	}
	(*scsiTaskDeviceInterface)->AddRef(scsiTaskDeviceInterface);
	scglocal(scgp)->masterPort = masterPort;
	scglocal(scgp)->scg_if[busno][tgt].mmcDeviceInterface = mmcDeviceInterface;
	scglocal(scgp)->scg_if[busno][tgt].scsiTaskDeviceInterface = scsiTaskDeviceInterface;
	scg_settarget(scgp, busno, tgt, tlun);
	err = 1;

out:
	if (scsiTaskDeviceInterface != NULL) {
		(*scsiTaskDeviceInterface)->Release(scsiTaskDeviceInterface);
	}

	if (plugInInterface != NULL) {
		(*plugInInterface)->Release(plugInInterface);
	}

	if (scsiDevice != 0) {
		IOObjectRelease(scsiDevice);
	}

	if (scsiObjectIterator != 0) {
		IOObjectRelease(scsiObjectIterator);
	}

	if (dict != NULL) {
		CFRelease(dict);
	}

	if (bydev) {
		free(realdevice);
	}
	return (err);
}

LOCAL int
scgo_close(scgp)
	SCSI	*scgp;
{
	int			b;
	int			t;
	SCSITaskDeviceInterface	**sc;
	MMCDeviceInterface	**mmc;

	if (scgp->local == NULL)
		return (-1);

	for (b = 0; b < MAX_SCG; b++) {
		for (t = 0; t < MAX_TGT; t++) {
			sc = scglocal(scgp)->scg_if[b][t].scsiTaskDeviceInterface;
			if (sc) {
				(*sc)->ReleaseExclusiveAccess(sc);
				(*sc)->Release(sc);
			}
			scglocal(scgp)->scg_if[b][t].scsiTaskDeviceInterface = NULL;
			mmc = scglocal(scgp)->scg_if[b][t].mmcDeviceInterface;
			if (mmc != NULL)
				(*mmc)->Release(mmc);
			scglocal(scgp)->scg_if[b][t].mmcDeviceInterface = NULL;
		}
	}

	mach_port_deallocate(mach_task_self(), scglocal(scgp)->masterPort);

	free(scgp->local);
	scgp->local = NULL;

	return (0);
}

LOCAL long
scgo_maxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	long maxdma = MAX_DMA_NEXT;
#ifdef	SGIOCMAXDMA
	int  m;

	if (ioctl(scglocal(scgp)->scgfile, SGIOCMAXDMA, &m) >= 0) {
		maxdma = m;
		if (scgp->debug > 0) {
			js_fprintf((FILE *)scgp->errfile,
				"maxdma: %d\n", maxdma);
		}
	}
#endif
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

	if (scgp->local == NULL || busno < 0 || busno >= MAX_SCG)
		return (FALSE);

	for (t = 0; t < MAX_TGT; t++) {
		if (scglocal(scgp)->scg_if[busno][t].scsiTaskDeviceInterface != NULL)
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
	if (scglocal(scgp) == NULL)
		return (-1);

	if (scglocal(scgp)->scg_if[busno][tgt].flags & NO_ACCESS)
		return (-2);
	if (scglocal(scgp)->scg_if[busno][tgt].scsiTaskDeviceInterface != NULL)
		return (0);
	return (-1);
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
	if (what == SCG_RESET_NOP)
		return (0);
	if (what != SCG_RESET_BUS) {
		errno = EINVAL;
		return (-1);
	}

	errno = 0;
	return (-1);
}

LOCAL int
scgo_send(scgp)
	SCSI		*scgp;
{
	struct scg_cmd		*sp = scgp->scmd;
	SCSITaskDeviceInterface	**sc = NULL;
	SCSITaskInterface	**cmd = NULL;
#if	defined(__LP64__)			/* Ugly differences for LP64 */
	IOAddressRange		iov;
#else
	IOVirtualRange		iov;
#endif
	SCSI_Sense_Data		senseData;
	SCSITaskStatus		status;
	UInt64			bytesTransferred;
	IOReturn		ioReturnValue;
	int			ret = 0;

	if (scgp->local == NULL) {
		sp->error = SCG_FATAL;
		return (0);
	}
	sc = scglocal(scgp)->scg_if[scg_scsibus(scgp)][scg_target(scgp)].scsiTaskDeviceInterface;
	if (sc == NULL) {
		sp->error = SCG_FATAL;
		return (0);
	}

	cmd = (*sc)->CreateSCSITask(sc);
	if (cmd == NULL) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "Failed to create SCSI task");
		ret = -1;

		sp->error = SCG_FATAL;
		sp->ux_errno = EIO;
		goto out;
	}


#if	defined(__LP64__)			/* Ugly differences for LP64 */
	iov.address = (mach_vm_address_t) sp->addr;
#else
	iov.address = (IOVirtualAddress) sp->addr;
#endif
	iov.length = sp->size;

	ioReturnValue = (*cmd)->SetCommandDescriptorBlock(cmd,
						sp->cdb.cmd_cdb, sp->cdb_len);

	if (ioReturnValue != kIOReturnSuccess) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "SetCommandDescriptorBlock failed with status %x",
			    ioReturnValue);
		ret = -1;
		goto out;
	}

	ioReturnValue = (*cmd)->SetScatterGatherEntries(cmd, &iov, 1, sp->size,
				(sp->flags & SCG_RECV_DATA) ?
				kSCSIDataTransfer_FromTargetToInitiator :
				kSCSIDataTransfer_FromInitiatorToTarget);
	if (ioReturnValue != kIOReturnSuccess) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "SetScatterGatherEntries failed with status %x",
			    ioReturnValue);
		ret = -1;
		goto out;
	}

	ioReturnValue = (*cmd)->SetTimeoutDuration(cmd, sp->timeout * 1000);
	if (ioReturnValue != kIOReturnSuccess) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "SetTimeoutDuration failed with status %x",
			    ioReturnValue);
		ret = -1;
		goto out;
	}

	memset(&senseData, 0, sizeof (senseData));

	seterrno(0);
	ioReturnValue = (*cmd)->ExecuteTaskSync(cmd,
				&senseData, &status, &bytesTransferred);

	sp->resid = sp->size - bytesTransferred;
	sp->error = SCG_NO_ERROR;
	sp->ux_errno = geterrno();

	if (ioReturnValue != kIOReturnSuccess) {
		js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			    "Command execution failed with status %x",
			    ioReturnValue);
		sp->error = SCG_RETRYABLE;
		ret = -1;
		goto out;
	}

	memset(&sp->scb, 0, sizeof (sp->scb));
	memset(&sp->u_sense.cmd_sense, 0, sizeof (sp->u_sense.cmd_sense));
	if (senseData.VALID_RESPONSE_CODE != 0 || status == 0x02) {
		/*
		 * There is no sense length - we need to asume that
		 * we always get 18 bytes.
		 */
		sp->sense_count = kSenseDefaultSize;
		memmove(&sp->u_sense.cmd_sense, &senseData, kSenseDefaultSize);
		if (sp->ux_errno == 0)
			sp->ux_errno = EIO;
	}

	sp->u_scb.cmd_scb[0] = status;

	/* ??? */
	if (status == kSCSITaskStatus_No_Status) {
		sp->error = SCG_RETRYABLE;
		ret = -1;
		goto out;
	}
	/*
	 * XXX Is it possible to have other senseful SCSI transport error codes?
	 */

out:
	if (cmd != NULL) {
		(*cmd)->Release(cmd);
	}

	return (ret);
}
