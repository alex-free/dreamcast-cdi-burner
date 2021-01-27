/* @(#)scsi-linux-sg.c	1.98 16/01/11 Copyright 1997-2016 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-linux-sg.c	1.98 16/01/11 Copyright 1997-2016 J. Schilling";
#endif
/*
 *	Interface for Linux generic SCSI implementation (sg).
 *
 *	This is the interface for the broken Linux SCSI generic driver.
 *	This is a hack, that tries to emulate the functionality
 *	of the scg driver.
 *
 *	Design flaws of the sg driver:
 *	-	cannot see if SCSI command could not be send
 *	-	cannot get SCSI status byte
 *	-	cannot get real dma count of tranfer
 *	-	cannot get number of bytes valid in auto sense data
 *	-	to few data in auto sense (CCS/SCSI-2/SCSI-3 needs >= 18)
 *
 *	This code contains support for the sg driver version 2 by
 *		H. Eiﬂfeld & J. Schilling
 *	Although this enhanced version has been announced to Linus and Alan,
 *	there was no reaction at all.
 *
 *	About half a year later there occured a version in the official
 *	Linux that was also called version 2. The interface of this version
 *	looks like a playground - the enhancements from this version are
 *	more or less useless for a portable real-world program.
 *
 *	With Linux 2.4 the official version of the sg driver is called 3.x
 *	and seems to be usable again. The main problem now is the curious
 *	interface that is provided to raise the DMA limit from 32 kB to a
 *	more reasonable value. To do this in a reliable way, a lot of actions
 *	are required.
 *
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 *
 *	Copyright (c) 1997-2016 J. Schilling
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
 * CDDL ß3.6 needs to be replaced by: "You may create a Larger Work by
 * combining Covered Software with other code if all other code is governed by
 * the terms of a license that is OSI approved (see www.opensource.org) and
 * you may distribute the Larger Work as a single product. In such a case,
 * You must make sure the requirements of this License are fulfilled for
 * the Covered Software."
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <linux/version.h>

#ifndef LINUX_VERSION_CODE	/* Very old kernel? */
#	define LINUX_VERSION_CODE 0
#endif

#if LINUX_VERSION_CODE >= 0x01031a /* <linux/scsi.h> introduced in 1.3.26 */
#if LINUX_VERSION_CODE >= 0x020000 /* <scsi/scsi.h> introduced somewhere. */
/* Need to fine tune the ifdef so we get the transition point right. */

#if	defined(HAVE_BROKEN_SCSI_SCSI_H) || \
	defined(HAVE_BROKEN_SRC_SCSI_SCSI_H)
/*
 * Be very careful in case that the Linux Kernel maintainers
 * unexpectedly fix the bugs in the Linux Lernel include files.
 * Only introduce the attempt for a workaround in case the include
 * files are broken anyway.
 */
#define	__KERNEL__
#include <asm/types.h>
#undef	__KERNEL__
#endif
#include <scsi/scsi.h>
#else
#include <linux/scsi.h>
#endif
#else				/* LINUX_VERSION_CODE == 0 Very old kernel? */
#define	__KERNEL__		/* Some Linux Include files are inconsistent */
#include <linux/fs.h>		/* From ancient versions, really needed? */
#undef __KERNEL__
#include "block/blk.h"		/* From ancient versions, really needed? */
#include "scsi/scsi.h"
#endif
#include <schily/stat.h>
#include <schily/device.h>

#if	defined(HAVE_BROKEN_SCSI_SG_H) || \
	defined(HAVE_BROKEN_SRC_SCSI_SG_H)
/*
 * Be very careful in case that the Linux Kernel maintainers
 * unexpectedly fix the bugs in the Linux Lernel include files.
 * Only introduce the attempt for a workaround in case the include
 * files are broken anyway.
 */
#define	__user
#endif
#include "scsi/sg.h"
#if	defined(HAVE_BROKEN_SCSI_SG_H) || \
	defined(HAVE_BROKEN_SRC_SCSI_SG_H)
#undef	__user
#endif

#undef sense			/* conflict in struct cdrom_generic_command */
#include <linux/cdrom.h>

#if	defined(CDROM_PACKET_SIZE) && defined(CDROM_SEND_PACKET)
#define	USE_ATAPI
#endif

/*
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 */
LOCAL	char	_scg_trans_version[] = "scsi-linux-sg.c-1.98";	/* The version for this transport*/

#ifndef	SCSI_IOCTL_GET_BUS_NUMBER
#define	SCSI_IOCTL_GET_BUS_NUMBER 0x5386
#endif

/*
 * XXX There must be a better way than duplicating things from system include
 * XXX files. This is stolen from /usr/src/linux/drivers/scsi/scsi.h
 */
#ifndef	DID_OK
#define	DID_OK		0x00 /* NO error				*/
#define	DID_NO_CONNECT	0x01 /* Couldn't connect before timeout period	*/
#define	DID_BUS_BUSY	0x02 /* BUS stayed busy through time out period	*/
#define	DID_TIME_OUT	0x03 /* TIMED OUT for other reason		*/
#define	DID_BAD_TARGET	0x04 /* BAD target.				*/
#define	DID_ABORT	0x05 /* Told to abort for some other reason	*/
#define	DID_PARITY	0x06 /* Parity error				*/
#define	DID_ERROR	0x07 /* Internal error				*/
#define	DID_RESET	0x08 /* Reset by somebody.			*/
#define	DID_BAD_INTR	0x09 /* Got an interrupt we weren't expecting.	*/
#endif

/*
 *  These indicate the error that occurred, and what is available.
 */
#ifndef	DRIVER_BUSY
#define	DRIVER_BUSY	0x01
#define	DRIVER_SOFT	0x02
#define	DRIVER_MEDIA	0x03
#define	DRIVER_ERROR	0x04

#define	DRIVER_INVALID	0x05
#define	DRIVER_TIMEOUT	0x06
#define	DRIVER_HARD	0x07
#define	DRIVER_SENSE	0x08
#endif

/*
 * XXX Should add extra space in buscookies and scgfiles for a "PP bus"
 * XXX and for two or more "ATAPI busses".
 */
#define	MAX_SCG		(512-MAX_ATA)	/* Max # of SCSI controllers */
#define	MAX_ATA		13		/* Max # of ATA devices */
#define	MAX_TGT		16
#define	MAX_LUN		8

#if	(MAX_SCG+MAX_ATA) >= 1000
error too many SCSI busses
#endif

#ifdef	USE_ATAPI
/*
 * # of virtual buses (schilly_host number)
 */
#define	MAX_ATAPI_HOSTS	MAX_SCG
typedef struct {
	Uchar   typ:4;
	Uchar   bus:4;
	Uchar   host:8;
} ata_buscookies;
#endif

struct scg_local {
	int	scgfile;		/* Used for SG_GET_BUFSIZE ioctl()*/
	short	scgfiles[MAX_SCG + MAX_ATA][MAX_TGT][MAX_LUN];
	short	buscookies[MAX_SCG + MAX_ATA];
	int	pgbus;
	int	pack_id;		/* Should be a random number	*/
	int	drvers;
	short	isold;
	short	flags;
	long	xbufsize;
	char	*xbuf;
	char	*SCSIbuf;
#ifdef	USE_ATAPI
	ata_buscookies	bc[MAX_ATAPI_HOSTS];
#endif
};
#define	scglocal(p)	((struct scg_local *)((p)->local))

/*
 * Flag definitions
 */
#define	LF_ATA		0x01		/* Using /dev/hd* ATA interface	*/

#ifdef	SG_BIG_BUFF
#define	MAX_DMA_LINUX	SG_BIG_BUFF	/* Defined in include/scsi/sg.h	*/
#else
#define	MAX_DMA_LINUX	(4*1024)	/* Old Linux versions		*/
#endif

#ifndef	SG_MAX_SENSE
#	define	SG_MAX_SENSE	16	/* Too small for CCS / SCSI-2	*/
#endif					/* But cannot be changed	*/

#if	!defined(__i386) && !defined(i386) && !defined(mc68000)
#define	MISALIGN
#endif
/*#define	MISALIGN*/
/*#undef	SG_GET_BUFSIZE*/

#if	defined(USE_PG) && !defined(USE_PG_ONLY)
#include "scsi-linux-pg.c"
#endif
#ifdef	USE_ATAPI
#include "scsi-linux-ata.c"
#endif

/*
 * Work around some broken Linux distributions with defective include files.
 */
#if !defined(HZ) && !defined(USER_HZ)
#define	USER_HZ	100
#endif

#ifdef	MISALIGN
LOCAL	int	sg_getint	__PR((int *ip));
#endif
LOCAL	int	scgo_send	__PR((SCSI *scgp));
#ifdef	SG_IO
LOCAL	int	sg_rwsend	__PR((SCSI *scgp));
#endif
LOCAL	void	sg_clearnblock	__PR((int f));
LOCAL	BOOL	sg_setup	__PR((SCSI *scgp, int f, int busno, int tgt, int tlun, int ataidx));
LOCAL	void	sg_initdev	__PR((SCSI *scgp, int f));
LOCAL	int	sg_mapbus	__PR((SCSI *scgp, int busno, int ino));
LOCAL	BOOL	sg_mapdev	__PR((SCSI *scgp, int f, int *busp, int *tgtp, int *lunp,
							int *chanp, int *inop, int ataidx));
#if defined(SG_SET_RESERVED_SIZE) && defined(SG_GET_RESERVED_SIZE)
LOCAL	long	sg_raisedma	__PR((SCSI *scgp, long newmax));
LOCAL	void	sg_checkdma	__PR((int f, int *valp));
#endif
LOCAL	void	sg_settimeout	__PR((int f, int timeout));

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
#ifdef	USE_PG
		/*
		 * If we only have a Parallel port or only opened a handle
		 * for PP, just return PP version.
		 */
		if (scglocal(scgp)->pgbus == 0 ||
		    (scg_scsibus(scgp) >= 0 &&
		    scg_scsibus(scgp) == scglocal(scgp)->pgbus))
			return (pg_version(scgp, what));
#endif
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
		case SCG_KVERSION:
			{
				static	char kv[16];
				int	n;

				if (scglocal(scgp)->drvers >= 0) {
					n = scglocal(scgp)->drvers;
					js_snprintf(kv, sizeof (kv),
					"%d.%d.%d",
					n/10000, (n%10000)/100, n%100);

					return (kv);
				}
			}
		}
	}
	return ((char *)0);
}

LOCAL int
scgo_help(scgp, f)
	SCSI	*scgp;
	FILE	*f;
{
	__scg_help(f, "sg", "Generic transport independent SCSI",
		"", "bus,target,lun", "1,2,0", TRUE, FALSE);
#ifdef	USE_PG
	pg_help(scgp, f);
#endif
#ifdef	USE_ATAPI
	scgo_ahelp(scgp, f);
#endif
	__scg_help(f, "ATA", "ATA Packet specific SCSI transport using sg interface",
		"ATA:", "bus,target,lun", "1,2,0", TRUE, FALSE);
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
	register int	i;
	register int	b;
	register int	t;
	register int	l;
	register int	nopen = 0;
		int	xopen = 0;
	char		devname[64];
		BOOL	use_ata = FALSE;

	if ((busno >= MAX_SCG && busno < 1000) || busno >= (1000+MAX_ATA) ||
	    tgt >= MAX_TGT || tlun >= MAX_LUN) {
		errno = EINVAL;
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Illegal value for busno, target or lun '%d,%d,%d'",
				busno, tgt, tlun);
		return (-1);
	}
	if (busno >= 1000)
		busno = busno - 1000 + MAX_SCG;

	if (device != NULL && *device != '\0') {
#ifdef	USE_ATAPI
		if (strncmp(device, "ATAPI", 5) == 0) {
			scgp->ops = &atapi_ops;
			return (SCGO_OPEN(scgp, device));
		}
#endif
		if (strcmp(device, "ATA") == 0) {
			/*
			 * Sending generic SCSI commands via /dev/hd* is a
			 * really bad idea when there also is a generic
			 * SCSI driver interface - it breaks the protocol
			 * layering model. A better idea would be to
			 * have a SCSI host bus adapter driver that sends
			 * the SCSI commands via the ATA hardware. This way,
			 * the layering model would be honored.
			 *
			 * People like Jens Axboe should finally fix the DMA
			 * bugs in the ide-scsi hostadaptor emulation module
			 * from Linux instead of publishing childish patches
			 * to the comment above.
			 */
			use_ata = TRUE;
			device = NULL;
			if (scgp->overbose) {
				/*
				 * I strongly encourage people who believe that
				 * they need to patch this message away to read
				 * the messages in the Solaris USCSI libscg
				 * layer instead of wetting their tissues while
				 * being unwilling to look besides their
				 * own belly button.
				 */
				js_fprintf((FILE *)scgp->errfile,
				"Warning: Using badly designed ATAPI via /dev/hd* interface.\n");
			}
		}
	}

	if (scgp->local == NULL) {
		scgp->local = malloc(sizeof (struct scg_local));
		if (scgp->local == NULL)
			return (0);

		scglocal(scgp)->scgfile = -1;
		scglocal(scgp)->pgbus = -2;
		scglocal(scgp)->SCSIbuf = (char *)-1;
		scglocal(scgp)->pack_id = 5;
		scglocal(scgp)->drvers = -1;
		scglocal(scgp)->isold = -1;
		scglocal(scgp)->flags = 0;
		if (use_ata)
			scglocal(scgp)->flags |= LF_ATA;
		scglocal(scgp)->xbufsize = 0L;
		scglocal(scgp)->xbuf = NULL;

		for (b = 0; b < MAX_SCG+MAX_ATA; b++) {
			scglocal(scgp)->buscookies[b] = (short)-1;
			for (t = 0; t < MAX_TGT; t++) {
				for (l = 0; l < MAX_LUN; l++)
					scglocal(scgp)->scgfiles[b][t][l] = (short)-1;
			}
		}
	}

	if (use_ata)
		goto scanopen;

	if ((device != NULL && *device != '\0') || (busno == -2 && tgt == -2))
		goto openbydev;

scanopen:
	/*
	 * Note that it makes no sense to scan less than all /dev/hd* devices
	 * as even /dev/hda may be a device that talks SCSI (e.g. a ATAPI
	 * notebook disk or a CD/DVD writer). The CD/DVD writer case may
	 * look silly but there may be users that did boot from a SCSI hdd
	 * and connected 4 CD/DVD writers to both IDE cables in the PC.
	 */
#if LINUX_VERSION_CODE <= 0x020600
	if (use_ata)
#endif
	for (i = 0; i <= 25; i++) {
		js_snprintf(devname, sizeof (devname), "/dev/hd%c", i+'a');
					/* O_NONBLOCK is dangerous */
		f = open(devname, O_RDWR | O_NONBLOCK);
		if (f < 0) {
			/*
			 * Set up error string but let us clear it later
			 * if at least one open succeeded.
			 */
			if (scgp->errstr)
				js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
							"Cannot open '/dev/hd*'");
#ifdef	EROFS
			if (errno == EROFS)	/* should we rather open RO? */
				continue;
#endif
			if (errno != ENOENT && errno != ENXIO && errno != ENODEV) {
				if (scgp->errstr)
					js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
							"Cannot open '%s'", devname);
				return (0);
			}
		} else {
			int	iparm;

			if (ioctl(f, SG_GET_TIMEOUT, &iparm) < 0) {
				if (scgp->errstr)
					js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
							"SCSI unsupported with '/dev/hd*'");
				close(f);
				continue;
			}
			sg_clearnblock(f);	/* Be very proper about this */
			if (sg_setup(scgp, f, busno, tgt, tlun, i))
				return (++nopen);
			if (busno < 0 && tgt < 0 && tlun < 0)
				nopen++;
		}
	}
	if (nopen > 0 && scgp->errstr)
		scgp->errstr[0] = '\0';

	xopen = nopen;
	if (!use_ata) for (i = 0; i < 32; i++) {
		js_snprintf(devname, sizeof (devname), "/dev/sg%d", i);
					/* O_NONBLOCK is dangerous */
		f = open(devname, O_RDWR | O_NONBLOCK);
		if (f < 0) {
			/*
			 * Set up error string but let us clear it later
			 * if at least one open succeeded.
			 */
			if (scgp->errstr)
				js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
							"Cannot open '/dev/sg*'");
			if (errno != ENOENT && errno != ENXIO && errno != ENODEV) {
				if (scgp->errstr)
					js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
							"Cannot open '%s'", devname);
				return (0);
			}
		} else {
			sg_clearnblock(f);	/* Be very proper about this */
			if (sg_setup(scgp, f, busno, tgt, tlun, -1))
				return (++nopen);
			if (busno < 0 && tgt < 0 && tlun < 0)
				nopen++;
		}
	}
	if (nopen > 0 && scgp->errstr)
		scgp->errstr[0] = '\0';

	if (!use_ata && nopen == xopen) for (i = 0; i <= 25; i++) {
		js_snprintf(devname, sizeof (devname), "/dev/sg%c", i+'a');
					/* O_NONBLOCK is dangerous */
		f = open(devname, O_RDWR | O_NONBLOCK);
		if (f < 0) {
			/*
			 * Set up error string but let us clear it later
			 * if at least one open succeeded.
			 */
			if (scgp->errstr)
				js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
							"Cannot open '/dev/sg*'");
			if (errno != ENOENT && errno != ENXIO && errno != ENODEV) {
				if (scgp->errstr)
					js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
							"Cannot open '%s'", devname);
				return (0);
			}
		} else {
			sg_clearnblock(f);	/* Be very proper about this */
			if (sg_setup(scgp, f, busno, tgt, tlun, -1))
				return (++nopen);
			if (busno < 0 && tgt < 0 && tlun < 0)
				nopen++;
		}
	}
	if (nopen > 0 && scgp->errstr)
		scgp->errstr[0] = '\0';

openbydev:
	if (device != NULL && *device != '\0') {
		b = -1;
		if (strlen(device) == 8 && strncmp(device, "/dev/hd", 7) == 0) {
			b = device[7] - 'a';
			if (b < 0 || b > 25)
				b = -1;
		}
		if (scgp->overbose) {
			/*
			 * Before you patch this away, are you sure that you
			 * know what you are going to to?
			 *
			 * Note that this is a warning that helps users from
			 * cdda2wav, mkisofs and other programs (that
			 * distinguish SCSI addresses from file names) from
			 * getting unexpected results.
			 */
			js_fprintf((FILE *)scgp->errfile,
			"Warning: Open by 'devname' is unintentional and not supported.\n");
		}
					/* O_NONBLOCK is dangerous */
		f = open(device, O_RDWR | O_NONBLOCK);
/*		if (f < 0 && errno == ENOENT)*/
/*			goto openpg;*/

		if (f < 0) {
			/*
			 * The pg driver has the same rules to decide whether
			 * to use openbydev. If we cannot open the device, it
			 * makes no sense to try the /dev/pg* driver.
			 */
			if (scgp->errstr)
				js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
					"Cannot open '%s'",
					device);
			return (0);
		}

		sg_clearnblock(f);		/* Be very proper about this */
		if (!sg_mapdev(scgp, f, &busno, &tgt, &tlun, 0, 0, b)) {
			close(f);
			/*
			 * If sg_mapdev() failes, this may be /dev/pg* device.
			 */
			goto openpg;
		}

#ifdef	OOO
		if (scg_scsibus(scgp) < 0)
			scg_scsibus(scgp) = busno;
		if (scg_target(scgp) < 0)
			scg_target(scgp) = tgt;
		if (scg_lun(scgp) < 0)
			scg_lun(scgp) = tlun;
#endif

		scg_settarget(scgp, busno, tgt, tlun);
		if (sg_setup(scgp, f, busno, tgt, tlun, b))
			return (++nopen);
	}
openpg:
#ifdef	USE_PG
	nopen += pg_open(scgp, device);
#endif
	if (scgp->debug > 0) for (b = 0; b < MAX_SCG+MAX_ATA; b++) {
		js_fprintf((FILE *)scgp->errfile,
			"Bus: %d cookie: %X\n",
			b, scglocal(scgp)->buscookies[b]);
		for (t = 0; t < MAX_TGT; t++) {
			for (l = 0; l < MAX_LUN; l++) {
				if (scglocal(scgp)->scgfiles[b][t][l] != (short)-1) {
					js_fprintf((FILE *)scgp->errfile,
						"file (%d,%d,%d): %d\n",
						b, t, l, scglocal(scgp)->scgfiles[b][t][l]);
				}
			}
		}
	}
	return (nopen);
}

LOCAL int
scgo_close(scgp)
	SCSI	*scgp;
{
	register int	f;
	register int	b;
	register int	t;
	register int	l;

	if (scgp->local == NULL)
		return (-1);

	for (b = 0; b < MAX_SCG+MAX_ATA; b++) {
		if (b == scglocal(scgp)->pgbus)
			continue;
		scglocal(scgp)->buscookies[b] = (short)-1;
		for (t = 0; t < MAX_TGT; t++) {
			for (l = 0; l < MAX_LUN; l++) {
				f = scglocal(scgp)->scgfiles[b][t][l];
				if (f >= 0)
					close(f);
				scglocal(scgp)->scgfiles[b][t][l] = (short)-1;
			}
		}
	}
	if (scglocal(scgp)->xbuf != NULL) {
		free(scglocal(scgp)->xbuf);
		scglocal(scgp)->xbufsize = 0L;
		scglocal(scgp)->xbuf = NULL;
	}
#ifdef	USE_PG
	pg_close(scgp);
#endif
	return (0);
}

/*
 * The Linux kernel becomes more and more unmaintainable.
 * Every year, a new incompatible SCSI transport interface is added.
 * Each of them has it's own contradictory constraints.
 * While you cannot have O_NONBLOCK set during operation, at least one
 * of the drivers requires O_NONBLOCK to be set during open().
 * This is used to clear O_NONBLOCK immediately after open() succeeded.
 */
LOCAL void
sg_clearnblock(f)
	int	f;
{
	int	n;

	n = fcntl(f, F_GETFL);
	n &= ~O_NONBLOCK;
	fcntl(f, F_SETFL, n);
}

LOCAL BOOL
sg_setup(scgp, f, busno, tgt, tlun, ataidx)
	SCSI	*scgp;
	int	f;
	int	busno;
	int	tgt;
	int	tlun;
	int	ataidx;
{
	int	n;
	int	Chan;
	int	Ino;
	int	Bus;
	int	Target;
	int	Lun;
	BOOL	onetarget = FALSE;

#ifdef	SG_GET_VERSION_NUM
	if (scglocal(scgp)->drvers < 0) {
		scglocal(scgp)->drvers = 0;
		if (ioctl(f, SG_GET_VERSION_NUM, &n) >= 0) {
			scglocal(scgp)->drvers = n;
			if (scgp->overbose) {
				js_fprintf((FILE *)scgp->errfile,
					"Linux sg driver version: %d.%d.%d\n",
					n/10000, (n%10000)/100, n%100);
			}
		}
	}
#endif
	if (scg_scsibus(scgp) >= 0 && scg_target(scgp) >= 0 && scg_lun(scgp) >= 0)
		onetarget = TRUE;

	sg_mapdev(scgp, f, &Bus, &Target, &Lun, &Chan, &Ino, ataidx);

	/*
	 * For old kernels try to make the best guess.
	 */
	Ino |= Chan << 8;
	n = sg_mapbus(scgp, Bus, Ino);
	if (Bus == -1) {
		Bus = n;
		if (scgp->debug > 0) {
			js_fprintf((FILE *)scgp->errfile,
				"SCSI Bus: %d (mapped from %d)\n", Bus, Ino);
		}
	}

	if (Bus < 0 || Bus >= (MAX_SCG+MAX_ATA) ||
					Target < 0 || Target >= MAX_TGT ||
						Lun < 0 || Lun >= MAX_LUN) {
		return (FALSE);
	}

	if (scglocal(scgp)->scgfiles[Bus][Target][Lun] == (short)-1)
		scglocal(scgp)->scgfiles[Bus][Target][Lun] = (short)f;

	if (onetarget) {
		if (Bus == busno && Target == tgt && Lun == tlun) {
			sg_initdev(scgp, f);
			scglocal(scgp)->scgfile = f;	/* remember file for ioctl's */
			return (TRUE);
		} else {
			scglocal(scgp)->scgfiles[Bus][Target][Lun] = (short)-1;
			close(f);
		}
	} else {
		/*
		 * SCSI bus scanning may cause other generic SCSI activities to
		 * fail because we set the default timeout and clear command
		 * queues (in case of the old sg driver interface).
		 */
		sg_initdev(scgp, f);
		if (scglocal(scgp)->scgfile < 0)
			scglocal(scgp)->scgfile = f;	/* remember file for ioctl's */
	}
	return (FALSE);
}

LOCAL void
sg_initdev(scgp, f)
	SCSI	*scgp;
	int	f;
{
	struct sg_rep {
		struct sg_header	hd;
		unsigned char		rbuf[100];
	} sg_rep;
	int	n;
	int	i;
	struct stat sb;

	sg_settimeout(f, scgp->deftimeout);

	/*
	 * If it's a block device, don't read.... pre Linux-2.4 /dev/sg*
	 * definitely is a character device and we only need to clear the
	 * queue for old /dev/sg* versions. If somebody ever implements
	 * raw disk access for Linux, this test may fail.
	 */
	if (fstat(f, &sb) >= 0 && S_ISBLK(sb.st_mode))
		return;

	/* Eat any unwanted garbage from prior use of this device */

	n = fcntl(f, F_GETFL);	/* Be very proper about this */
	fcntl(f, F_SETFL, n|O_NONBLOCK);

	fillbytes((caddr_t)&sg_rep, sizeof (struct sg_header), '\0');
	sg_rep.hd.reply_len = sizeof (struct sg_header);

	/*
	 * This is really ugly.
	 * We come here if 'f' is related to a raw device. If Linux
	 * will ever have raw devices for /dev/hd* we may get problems.
	 * As long as there is no clean way to find out whether the
	 * filedescriptor 'f' is related to an old /dev/sg* or to
	 * /dev/hd*, we must assume that we found an old /dev/sg* and
	 * clean it up. Unfortunately, reading from /dev/hd* will
	 * Access the medium.
	 */
	for (i = 0; i < 1000; i++) {	/* Read at least 32k from /dev/sg* */
		int	ret;

		ret = read(f, &sg_rep, sizeof (sg_rep));
		if (ret > 0)
			continue;
		if (ret == 0 || errno == EAGAIN || errno == EIO)
			break;
		if (ret < 0 && i > 10)	/* Stop on repeated unknown error */
			break;
	}
	fcntl(f, F_SETFL, n);
}

LOCAL int
sg_mapbus(scgp, busno, ino)
	SCSI	*scgp;
	int	busno;
	int	ino;
{
	register int	i;

	if (busno >= 0 && busno < (MAX_SCG+MAX_ATA)) {
		/*
		 * SCSI_IOCTL_GET_BUS_NUMBER worked.
		 * Now we have the problem that Linux does not properly number
		 * SCSI busses. The Bus number that Linux creates really is
		 * the controller (card) number. I case of multi SCSI bus
		 * cards we are lost.
		 */
		if (scglocal(scgp)->buscookies[busno] == (short)-1) {
			scglocal(scgp)->buscookies[busno] = ino;
			return (busno);
		}
		if (scglocal(scgp)->buscookies[busno] != (short)ino)
			errmsgno(EX_BAD, "Warning Linux Bus mapping botch.\n");
		return (busno);

	} else for (i = 0; i < (MAX_SCG+MAX_ATA); i++) {
		if (scglocal(scgp)->buscookies[i] == (short)-1) {
			scglocal(scgp)->buscookies[i] = ino;
			return (i);
		}

		if (scglocal(scgp)->buscookies[i] == ino)
			return (i);
	}
	return (0);
}

LOCAL BOOL
sg_mapdev(scgp, f, busp, tgtp, lunp, chanp, inop, ataidx)
	SCSI	*scgp;
	int	f;
	int	*busp;
	int	*tgtp;
	int	*lunp;
	int	*chanp;
	int	*inop;
	int	ataidx;
{
	struct	sg_id {
		long	l1; /* target | lun << 8 | channel << 16 | low_ino << 24 */
		long	l2; /* Unique id */
	} sg_id;
	int	Chan;
	int	Ino;
	int	Bus;
	int	Target;
	int	Lun;

	if (ataidx >= 0) {
		/*
		 * The badly designed /dev/hd* interface maps everything
		 * to 0,0,0 so we need to do the mapping ourselves.
		 */
		*busp = (ataidx / 2) + MAX_SCG;
		*tgtp = ataidx % 2;
		*lunp = 0;
		if (chanp)
			*chanp = 0;
		if (inop)
			*inop = 0;
		return (TRUE);
	}
	if (ioctl(f, SCSI_IOCTL_GET_IDLUN, &sg_id))
		return (FALSE);
	if (scgp->debug > 0) {
		js_fprintf((FILE *)scgp->errfile,
			"l1: 0x%lX l2: 0x%lX\n", sg_id.l1, sg_id.l2);
	}
	if (ioctl(f, SCSI_IOCTL_GET_BUS_NUMBER, &Bus) < 0) {
		Bus = -1;
	}

	Target	= sg_id.l1 & 0xFF;
	Lun	= (sg_id.l1 >> 8) & 0xFF;
	Chan	= (sg_id.l1 >> 16) & 0xFF;
	Ino	= (sg_id.l1 >> 24) & 0xFF;
	if (scgp->debug > 0) {
		js_fprintf((FILE *)scgp->errfile,
			"Bus: %d Target: %d Lun: %d Chan: %d Ino: %d\n",
			Bus, Target, Lun, Chan, Ino);
	}
	*busp = Bus;
	*tgtp = Target;
	*lunp = Lun;
	if (chanp)
		*chanp = Chan;
	if (inop)
		*inop = Ino;
	return (TRUE);
}

#if defined(SG_SET_RESERVED_SIZE) && defined(SG_GET_RESERVED_SIZE)
/*
 * The way Linux does DMA resouce management is a bit curious.
 * It totally deviates from all other OS and forces long ugly code.
 * If we are opening all drivers for a SCSI bus scan operation, we need
 * to set the limit for all open devices.
 * This may use up all kernel memory ... so do the job carefully.
 *
 * A big problem is that SG_SET_RESERVED_SIZE does not return any hint
 * on whether the request did fail. The only way to find if it worked
 * is to use SG_GET_RESERVED_SIZE to read back the current values.
 */
LOCAL long
sg_raisedma(scgp, newmax)
	SCSI	*scgp;
	long	newmax;
{
	register int	b;
	register int	t;
	register int	l;
	register int	f;
		int	val;
		int	old;

	/*
	 * First try to raise the DMA limit to a moderate value that
	 * most likely does not use up all kernel memory.
	 */
	val = 126*1024;

	if (val > MAX_DMA_LINUX) {
		for (b = 0; b < (MAX_SCG+MAX_ATA); b++) {
			for (t = 0; t < MAX_TGT; t++) {
				for (l = 0; l < MAX_LUN; l++) {
					if ((f = SCGO_FILENO(scgp, b, t, l)) < 0)
						continue;
					old = 0;
					if (ioctl(f, SG_GET_RESERVED_SIZE, &old) < 0)
						continue;
					if (val > old)
						ioctl(f, SG_SET_RESERVED_SIZE, &val);
				}
			}
		}
	}

	/*
	 * Now to raise the DMA limit to what we really need.
	 */
	if (newmax > val) {
		val = newmax;
		for (b = 0; b < (MAX_SCG+MAX_ATA); b++) {
			for (t = 0; t < MAX_TGT; t++) {
				for (l = 0; l < MAX_LUN; l++) {
					if ((f = SCGO_FILENO(scgp, b, t, l)) < 0)
						continue;
					old = 0;
					if (ioctl(f, SG_GET_RESERVED_SIZE, &old) < 0)
						continue;
					if (val > old)
						ioctl(f, SG_SET_RESERVED_SIZE, &val);
				}
			}
		}
	}

	/*
	 * To make sure we did not fail (the ioctl does not report errors)
	 * we need to check the DMA limits. We return the smallest value.
	 */
	for (b = 0; b < (MAX_SCG+MAX_ATA); b++) {
		for (t = 0; t < MAX_TGT; t++) {
			for (l = 0; l < MAX_LUN; l++) {
/*
 * XXX Remove the SG_GET_MAX_TRANSFER_LENGTH after the
 * XXX SG_GET_RESERVED_SIZE call returns the real max DMA.
 */
#ifdef	SG_GET_MAX_TRANSFER_LENGTH
				int	v2;
#endif
				if ((f = SCGO_FILENO(scgp, b, t, l)) < 0)
					continue;
				if (ioctl(f, SG_GET_RESERVED_SIZE, &val) < 0)
					continue;
#ifdef	SG_GET_MAX_TRANSFER_LENGTH
				if (ioctl(f, SG_GET_MAX_TRANSFER_LENGTH, &v2) >= 0) {
					if (v2 < val)
						val = v2;
				}
#else
				sg_checkdma(f, &val);	/* This does not work */
#endif
				if (scgp->debug > 0) {
					js_fprintf((FILE *)scgp->errfile,
						"Target (%d,%d,%d): DMA max %d old max: %ld\n",
						b, t, l, val, newmax);
				}
				if (val < newmax)
					newmax = val;
			}
		}
	}
	return ((long)newmax);
}

/*
 * This is extremely ugly code. It would be nice if Linux could
 * provide an ioctl to retrieve the max DMA size...
 * XXX Remove this function completely at the same time when
 * XXX SG_GET_MAX_TRANSFER_LENGTHis removed.
 */
LOCAL void
sg_checkdma(f, valp)
	int	f;
	int	*valp;
{
#ifdef	SG_GET_PACK_ID
	struct stat	sb;
	int		val;
	char		nbuf[80];
	int		fx;

	if (ioctl(f, SG_GET_PACK_ID, &val) < 0)	/* Only works with /dev/sg* */
		return;
	val = -1;
	if (fstat(f, &sb) >= 0)			/* Try to get instance # */
		val = minor(sb.st_rdev);
	if (val < 0)
		return;

	js_snprintf(nbuf, sizeof (nbuf),
	    "/sys/class/scsi_generic/sg%d/device/block/queue/max_hw_sectors_kb",
	    val);
	fx = open(nbuf, O_RDONLY | O_NDELAY);
	if (fx >= 0) {
		if (read(fx, nbuf, sizeof (nbuf)) > 0) {
			val = -1;
			astoi(nbuf, &val);
			if (val > 0) {
				val *= 1024;
				if (*valp > val)
					*valp = val;
			}
		}
		close(fx);
	}
#endif	/* SG_GET_PACK_ID */
}
#endif

LOCAL long
scgo_maxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	long maxdma = MAX_DMA_LINUX;

#if defined(SG_SET_RESERVED_SIZE) && defined(SG_GET_RESERVED_SIZE)
	/*
	 * Use the curious new kernel interface found on Linux >= 2.2.10
	 * This interface first appeared in 2.2.6 but it was not working.
	 */
	if (scglocal(scgp)->drvers >= 20134)
		maxdma = sg_raisedma(scgp, amt);
#endif
#ifdef	SG_GET_BUFSIZE
	/*
	 * We assume that all /dev/sg instances use the same
	 * maximum buffer size.
	 */
	maxdma = ioctl(scglocal(scgp)->scgfile, SG_GET_BUFSIZE, 0);
#endif
	if (maxdma < 0) {
#ifdef	USE_PG
		/*
		 * If we only have a Parallel port, just return PP maxdma.
		 */
		if (scglocal(scgp)->pgbus == 0)
			return (pg_maxdma(scgp, amt));
#endif
		if (scglocal(scgp)->scgfile >= 0)
			maxdma = MAX_DMA_LINUX;
	}
#ifdef	USE_PG
	if (scg_scsibus(scgp) == scglocal(scgp)->pgbus)
		return (pg_maxdma(scgp, amt));
	if ((scg_scsibus(scgp) < 0) && (pg_maxdma(scgp, amt) < maxdma))
		return (pg_maxdma(scgp, amt));
#endif
	return (maxdma);
}

LOCAL void *
scgo_getbuf(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	char	*ret;

	if (scgp->debug > 0) {
		js_fprintf((FILE *)scgp->errfile,
				"scgo_getbuf: %ld bytes\n", amt);
	}
	/*
	 * For performance reason, we allocate pagesize()
	 * bytes before the SCSI buffer to avoid
	 * copying the whole buffer contents when
	 * setting up the /dev/sg data structures.
	 */
	ret = valloc((size_t)(amt+getpagesize()));
	if (ret == NULL)
		return (ret);
	scgp->bufbase = ret;
	ret += getpagesize();
	scglocal(scgp)->SCSIbuf = ret;
	return ((void *)ret);
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
	return (1000+MAX_ATA);
	return (MAX_SCG+MAX_ATA);
}

LOCAL BOOL
scgo_havebus(scgp, busno)
	SCSI	*scgp;
	int	busno;
{
	register int	t;
	register int	l;

	if (busno >= 1000) {
		if (busno >= (1000+MAX_ATA))
			return (FALSE);
		busno = busno - 1000 + MAX_SCG;
	} else if (busno < 0 || busno >= MAX_SCG)
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
	if (busno >= 1000)
		busno = busno - 1000 + MAX_SCG;

	if (busno < 0 || busno >= (MAX_SCG+MAX_ATA) ||
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
#ifdef	USE_PG
	if (scg_scsibus(scgp) == scglocal(scgp)->pgbus)
		return (pg_initiator_id(scgp));
#endif
	return (-1);
}

LOCAL int
scgo_isatapi(scgp)
	SCSI	*scgp;
{
#ifdef	USE_PG
	if (scg_scsibus(scgp) == scglocal(scgp)->pgbus)
		return (pg_isatapi(scgp));
#endif

	/*
	 * The /dev/hd* interface always returns TRUE for SG_EMULATED_HOST.
	 * So this is completely useless.
	 */
	if (scglocal(scgp)->flags & LF_ATA)
		return (-1);

#ifdef	SG_EMULATED_HOST
	{
	int	emulated = FALSE;

	/*
	 * XXX Should we use this at all?
	 * XXX The badly designed /dev/hd* interface always
	 * XXX returns TRUE, even when used with e.g. /dev/sr0.
	 */
	if (ioctl(scgp->fd, SG_EMULATED_HOST, &emulated) >= 0)
		return (emulated != 0);
	}
#endif
	return (-1);
}

LOCAL int
scgo_reset(scgp, what)
	SCSI	*scgp;
	int	what;
{
#ifdef	SG_SCSI_RESET
	int	f = scgp->fd;
	int	func = -1;
#endif
#ifdef	USE_PG
	if (scg_scsibus(scgp) == scglocal(scgp)->pgbus)
		return (pg_reset(scgp, what));
#endif
	/*
	 * Do we have a SCSI reset in the Linux sg driver?
	 */
#ifdef	SG_SCSI_RESET
	/*
	 * Newer Linux sg driver seem to finally implement it...
	 */
#ifdef	SG_SCSI_RESET_NOTHING
	func = SG_SCSI_RESET_NOTHING;
	if (ioctl(f, SG_SCSI_RESET, &func) >= 0) {
		if (what == SCG_RESET_NOP)
			return (0);
#ifdef	SG_SCSI_RESET_DEVICE
		if (what == SCG_RESET_TGT) {
			func = SG_SCSI_RESET_DEVICE;
			if (ioctl(f, SG_SCSI_RESET, &func) >= 0)
				return (0);
		}
#endif
#ifdef	SG_SCSI_RESET_BUS
		if (what == SCG_RESET_BUS) {
			func = SG_SCSI_RESET_BUS;
			if (ioctl(f, SG_SCSI_RESET, &func) >= 0)
				return (0);
		}
#endif
	}
#endif
#endif
	return (-1);
}

LOCAL void
sg_settimeout(f, tmo)
	int	f;
	int	tmo;
{
#ifdef	USER_HZ
	tmo *= USER_HZ;
	if (tmo)
		tmo += USER_HZ/2;
#else
	tmo *= HZ;
	if (tmo)
		tmo += HZ/2;
#endif

	if (ioctl(f, SG_SET_TIMEOUT, &tmo) < 0)
		comerr("Cannot set SG_SET_TIMEOUT.\n");
}

/*
 * Get misaligned int.
 * Needed for all recent processors (sparc/ppc/alpha)
 * because the /dev/sg design forces us to do misaligned
 * reads of integers.
 */
#ifdef	MISALIGN
LOCAL int
sg_getint(ip)
	int	*ip;
{
		int	ret;
	register char	*cp = (char *)ip;
	register char	*tp = (char *)&ret;
	register int	i;

	for (i = sizeof (int); --i >= 0; )
		*tp++ = *cp++;

	return (ret);
}
#define	GETINT(a)	sg_getint(&(a))
#else
#define	GETINT(a)	(a)
#endif

#ifdef	SG_IO
LOCAL int
scgo_send(scgp)
	SCSI		*scgp;
{
	struct scg_cmd	*sp = scgp->scmd;
	int		ret;
	sg_io_hdr_t	sg_io;
	struct timeval	to;
static	uid_t		cureuid = 0;	/* XXX Hack until we have uid management */

	if (scgp->fd < 0) {
		sp->error = SCG_FATAL;
		sp->ux_errno = EIO;
		return (0);
	}
	if (scglocal(scgp)->isold > 0) {
		return (sg_rwsend(scgp));
	}
	fillbytes((caddr_t)&sg_io, sizeof (sg_io), '\0');

	sg_io.interface_id = 'S';

	if (sp->flags & SCG_RECV_DATA) {
		sg_io.dxfer_direction = SG_DXFER_FROM_DEV;
	} else if (sp->size > 0) {
		sg_io.dxfer_direction = SG_DXFER_TO_DEV;
	} else {
		sg_io.dxfer_direction = SG_DXFER_NONE;
	}
	sg_io.cmd_len = sp->cdb_len;
	if (sp->sense_len > SG_MAX_SENSE)
		sg_io.mx_sb_len = SG_MAX_SENSE;
	else
		sg_io.mx_sb_len = sp->sense_len;
	sg_io.dxfer_len = sp->size;
	sg_io.dxferp = sp->addr;
	sg_io.cmdp = sp->cdb.cmd_cdb;
	sg_io.sbp = sp->u_sense.cmd_sense;
	sg_io.timeout = sp->timeout*1000;
	sg_io.flags |= SG_FLAG_DIRECT_IO;

	if (cureuid != 0)
		seteuid(0);
again:
	errno = 0;
	ret = ioctl(scgp->fd, SG_IO, &sg_io);
	if (ret < 0 && geterrno() == EPERM) {	/* XXX Hack until we have uid management */
		cureuid = geteuid();
		if (seteuid(0) >= 0)
			goto again;
	}
	if (cureuid != 0)
		seteuid(cureuid);

	if (scgp->debug > 0) {
		js_fprintf((FILE *)scgp->errfile,
				"ioctl ret: %d\n", ret);
	}

	if (ret < 0) {
		sp->ux_errno = geterrno();
		/*
		 * Check if SCSI command could not be send at all.
		 * Linux usually returns EINVAL for an unknown ioctl.
		 * In case somebody from the Linux kernel team learns that the
		 * corect errno would be ENOTTY, we check for this errno too.
		 */
		if ((sp->ux_errno == ENOTTY || sp->ux_errno == EINVAL) &&
		    scglocal(scgp)->isold < 0) {
			scglocal(scgp)->isold = 1;
			return (sg_rwsend(scgp));
		}
		if (sp->ux_errno == ENXIO || sp->ux_errno == EPERM ||
		    sp->ux_errno == EINVAL || sp->ux_errno == EACCES) {
			return (-1);
		}
	}

	sp->u_scb.cmd_scb[0] = sg_io.status;
	sp->sense_count = sg_io.sb_len_wr;

	if (scgp->debug > 0) {
		js_fprintf((FILE *)scgp->errfile,
				"host_status: %02X driver_status: %02X\n",
				sg_io.host_status, sg_io.driver_status);
	}

	switch (sg_io.host_status) {

	case DID_OK:
			/*
			 * If there is no DMA overrun and there is a
			 * SCSI Status byte != 0 then the SCSI cdb transport
			 * was OK and sp->error must be SCG_NO_ERROR.
			 */
			if ((sg_io.driver_status & DRIVER_SENSE) != 0) {
				if (sp->ux_errno == 0)
					sp->ux_errno = EIO;

				if (sp->u_sense.cmd_sense[0] != 0 &&
				    sp->u_scb.cmd_scb[0] == 0) {
					/*
					 * The Linux SCSI system up to 2.4.xx
					 * trashes the status byte in the
					 * kernel. This is true at least for
					 * ide-scsi emulation. Until this gets
					 * fixed, we need this hack.
					 */
					sp->u_scb.cmd_scb[0] = ST_CHK_COND;
					if (sp->sense_count == 0)
						sp->sense_count = SG_MAX_SENSE;

					if ((sp->u_sense.cmd_sense[2] == 0) &&
					    (sp->u_sense.cmd_sense[12] == 0) &&
					    (sp->u_sense.cmd_sense[13] == 0)) {
						/*
						 * The Linux SCSI system will
						 * send a request sense for
						 * even a dma underrun error.
						 * Clear CHECK CONDITION state
						 * in case of No Sense.
						 */
						sp->u_scb.cmd_scb[0] = 0;
						sp->u_sense.cmd_sense[0] = 0;
						sp->sense_count = 0;
					}
				}
			}
			break;

	case DID_NO_CONNECT:	/* Arbitration won, retry NO_CONNECT? */
			sp->error = SCG_RETRYABLE;
			break;
	case DID_BAD_TARGET:
			sp->error = SCG_FATAL;
			break;

	case DID_TIME_OUT:
		__scg_times(scgp);

		if (sp->timeout > 1 && scgp->cmdstop->tv_sec == 0) {
			sp->u_scb.cmd_scb[0] = 0;
			sp->error = SCG_FATAL;	/* a selection timeout */
		} else {
			sp->error = SCG_TIMEOUT;
		}
		break;

	default:
		to.tv_sec = sp->timeout;
		to.tv_usec = 500000;
		__scg_times(scgp);

		if (scgp->cmdstop->tv_sec > to.tv_sec ||
		    (scgp->cmdstop->tv_sec == to.tv_sec &&
			scgp->cmdstop->tv_usec > to.tv_usec)) {

			sp->ux_errno = 0;
			sp->error = SCG_TIMEOUT;	/* a timeout */
		} else {
			sp->error = SCG_RETRYABLE;
			if (!scgp->silent) {
				js_fprintf((FILE *)scgp->errfile,
					"Unknown sg_io.host_status %X\n",
						sg_io.host_status);
			}
		}
		break;
	}
	if (sp->error && sp->ux_errno == 0)
		sp->ux_errno = EIO;

	sp->resid = sg_io.resid;
	return (0);
}
#else
#	define	sg_rwsend	scgo_send
#endif

LOCAL int
sg_rwsend(scgp)
	SCSI		*scgp;
{
	int		f = scgp->fd;
	struct scg_cmd	*sp = scgp->scmd;
	struct sg_rq	*sgp;
	struct sg_rq	*sgp2;
	int	i;
	int	pack_len;
	int	reply_len;
	int	amt = sp->cdb_len;
	struct sg_rq {
		struct sg_header	hd;
		unsigned char		buf[MAX_DMA_LINUX+SCG_MAX_CMD];
	} sg_rq;
#ifdef	SG_GET_BUFSIZE		/* We may use a 'sg' version 2 driver	*/
	char	driver_byte;
	char	host_byte;
	char	msg_byte;
	char	status_byte;
#endif

	if (f < 0) {
		sp->error = SCG_FATAL;
		sp->ux_errno = EIO;
		return (0);
	}
#ifdef	USE_PG
	if (scg_scsibus(scgp) == scglocal(scgp)->pgbus)
		return (pg_send(scgp));
#endif
	if (sp->timeout != scgp->deftimeout)
		sg_settimeout(f, sp->timeout);


	sgp2 = sgp = &sg_rq;
	if (sp->addr == scglocal(scgp)->SCSIbuf) {
		sgp = (struct sg_rq *)
			(scglocal(scgp)->SCSIbuf - (sizeof (struct sg_header) + amt));
		sgp2 = (struct sg_rq *)
			(scglocal(scgp)->SCSIbuf - (sizeof (struct sg_header)));
	} else {
		if (scgp->debug > 0) {
			js_fprintf((FILE *)scgp->errfile,
				"DMA addr: 0x%8.8lX size: %d - using copy buffer\n",
				(long)sp->addr, sp->size);
		}
		if (sp->size > (int)(sizeof (sg_rq.buf) - SCG_MAX_CMD)) {

			if (scglocal(scgp)->xbuf == NULL) {
				scglocal(scgp)->xbufsize = scgp->maxbuf;
				scglocal(scgp)->xbuf =
					malloc(scglocal(scgp)->xbufsize +
						SCG_MAX_CMD +
						sizeof (struct sg_header));
				if (scgp->debug > 0) {
					js_fprintf((FILE *)scgp->errfile,
						"Allocated DMA copy buffer, addr: 0x%8.8lX size: %ld\n",
						(long)scglocal(scgp)->xbuf,
						scgp->maxbuf);
				}
			}
			if (scglocal(scgp)->xbuf == NULL ||
				sp->size > scglocal(scgp)->xbufsize) {
				errno = ENOMEM;
				return (-1);
			}
			sgp2 = sgp = (struct sg_rq *)scglocal(scgp)->xbuf;
		}
	}

	/*
	 * This is done to avoid misaligned access of sgp->some_int
	 */
	pack_len = sizeof (struct sg_header) + amt;
	reply_len = sizeof (struct sg_header);
	if (sp->flags & SCG_RECV_DATA) {
		reply_len += sp->size;
	} else {
		pack_len += sp->size;
	}

#ifdef	MISALIGN
	/*
	 * sgp->some_int may be misaligned if (sp->addr == SCSIbuf)
	 * This is no problem on Intel porocessors, however
	 * all other processors don't like it.
	 * sizeof (struct sg_header) + amt is usually not a multiple of
	 * sizeof (int). For this reason, we fill in the values into sg_rq
	 * which is always corectly aligned and then copy it to the real
	 * location if this location differs from sg_rq.
	 * Never read/write directly to sgp->some_int !!!!!
	 */
	fillbytes((caddr_t)&sg_rq, sizeof (struct sg_header), '\0');

	sg_rq.hd.pack_len = pack_len;
	sg_rq.hd.reply_len = reply_len;
	sg_rq.hd.pack_id = scglocal(scgp)->pack_id++;
/*	sg_rq.hd.result = 0;	not needed because of fillbytes() */

	if ((caddr_t)&sg_rq != (caddr_t)sgp)
		movebytes((caddr_t)&sg_rq, (caddr_t)sgp, sizeof (struct sg_header));
#else
	fillbytes((caddr_t)sgp, sizeof (struct sg_header), '\0');

	sgp->hd.pack_len = pack_len;
	sgp->hd.reply_len = reply_len;
	sgp->hd.pack_id = scglocal(scgp)->pack_id++;
/*	sgp->hd.result = 0;	not needed because of fillbytes() */
#endif
	if (amt == 12)
		sgp->hd.twelve_byte = 1;


	for (i = 0; i < amt; i++) {
		sgp->buf[i] = sp->cdb.cmd_cdb[i];
	}
	if (!(sp->flags & SCG_RECV_DATA)) {
		if ((void *)sp->addr != (void *)&sgp->buf[amt])
			movebytes(sp->addr, &sgp->buf[amt], sp->size);
		amt += sp->size;
	}
#ifdef	SG_GET_BUFSIZE
	sgp->hd.want_new  = 1;			/* Order new behaviour	*/
	sgp->hd.cdb_len	  = sp->cdb_len;	/* Set CDB length	*/
	if (sp->sense_len > SG_MAX_SENSE)
		sgp->hd.sense_len = SG_MAX_SENSE;
	else
		sgp->hd.sense_len = sp->sense_len;
#endif
	i = sizeof (struct sg_header) + amt;
	if ((amt = write(f, sgp, i)) < 0) {			/* write */
		sg_settimeout(f, scgp->deftimeout);
		return (-1);
	} else if (amt != i) {
		errmsg("scgo_send(%s) wrote %d bytes (expected %d).\n",
						scgp->cmdname, amt, i);
	}

	if (sp->addr == scglocal(scgp)->SCSIbuf) {
		movebytes(sgp, sgp2, sizeof (struct sg_header));
		sgp = sgp2;
	}
	sgp->hd.sense_buffer[0] = 0;
	if ((amt = read(f, sgp, reply_len)) < 0) {		/* read */
		sg_settimeout(f, scgp->deftimeout);
		return (-1);
	}

	if (sp->flags & SCG_RECV_DATA && ((void *)sgp->buf != (void *)sp->addr)) {
		movebytes(sgp->buf, sp->addr, sp->size);
	}
	sp->ux_errno = GETINT(sgp->hd.result);		/* Unaligned read */
	sp->error = SCG_NO_ERROR;

#ifdef	SG_GET_BUFSIZE
	if (sgp->hd.grant_new) {
		sp->sense_count = sgp->hd.sense_len;
		pack_len    = GETINT(sgp->hd.sg_cmd_status);	/* Unaligned read */
		driver_byte = (pack_len  >> 24) & 0xFF;
		host_byte   = (pack_len  >> 16) & 0xFF;
		msg_byte    = (pack_len  >> 8) & 0xFF;
		status_byte =  pack_len  & 0xFF;

		switch (host_byte) {

		case DID_OK:
				if ((driver_byte & DRIVER_SENSE ||
				    sgp->hd.sense_buffer[0] != 0) &&
				    status_byte == 0) {
					/*
					 * The Linux SCSI system up to 2.4.xx
					 * trashes the status byte in the
					 * kernel. This is true at least for
					 * ide-scsi emulation. Until this gets
					 * fixed, we need this hack.
					 */
					status_byte = ST_CHK_COND;
					if (sgp->hd.sense_len == 0)
						sgp->hd.sense_len = SG_MAX_SENSE;

					if ((sp->u_sense.cmd_sense[2] == 0) &&
					    (sp->u_sense.cmd_sense[12] == 0) &&
					    (sp->u_sense.cmd_sense[13] == 0)) {
						/*
						 * The Linux SCSI system will
						 * send a request sense for
						 * even a dma underrun error.
						 * Clear CHECK CONDITION state
						 * in case of No Sense.
						 */
						sp->u_scb.cmd_scb[0] = 0;
						sp->u_sense.cmd_sense[0] = 0;
						sp->sense_count = 0;
					}
				}
				break;

		case DID_NO_CONNECT:	/* Arbitration won, retry NO_CONNECT? */
				sp->error = SCG_RETRYABLE;
				break;

		case DID_BAD_TARGET:
				sp->error = SCG_FATAL;
				break;

		case DID_TIME_OUT:
				sp->error = SCG_TIMEOUT;
				break;

		default:
				sp->error = SCG_RETRYABLE;

				if ((driver_byte & DRIVER_SENSE ||
				    sgp->hd.sense_buffer[0] != 0) &&
				    status_byte == 0) {
					status_byte = ST_CHK_COND;
					sp->error = SCG_NO_ERROR;
				}
				if (status_byte != 0 && sgp->hd.sense_len == 0) {
					sgp->hd.sense_len = SG_MAX_SENSE;
					sp->error = SCG_NO_ERROR;
				}
				break;

		}
		if ((host_byte != DID_OK || status_byte != 0) && sp->ux_errno == 0)
			sp->ux_errno = EIO;
		sp->u_scb.cmd_scb[0] = status_byte;
		if (status_byte & ST_CHK_COND) {
			sp->sense_count = sgp->hd.sense_len;
			movebytes(sgp->hd.sense_buffer, sp->u_sense.cmd_sense, sp->sense_count);
		}
	} else
#endif
	{
		if (GETINT(sgp->hd.result) == EBUSY) {	/* Unaligned read */
			struct timeval to;

			to.tv_sec = sp->timeout;
			to.tv_usec = 500000;
			__scg_times(scgp);

			if (sp->timeout > 1 && scgp->cmdstop->tv_sec == 0) {
				sp->u_scb.cmd_scb[0] = 0;
				sp->ux_errno = EIO;
				sp->error = SCG_FATAL;	/* a selection timeout */
			} else if (scgp->cmdstop->tv_sec < to.tv_sec ||
			    (scgp->cmdstop->tv_sec == to.tv_sec &&
				scgp->cmdstop->tv_usec < to.tv_usec)) {

				sp->ux_errno = EIO;
				sp->error = SCG_TIMEOUT;	/* a timeout */
			} else {
				sp->error = SCG_RETRYABLE;	/* may be BUS_BUSY */
			}
		}

		if (sp->flags & SCG_RECV_DATA)
			sp->resid = (sp->size + sizeof (struct sg_header)) - amt;
		else
			sp->resid = 0;	/* sg version1 cannot return DMA resid count */

		if (sgp->hd.sense_buffer[0] != 0) {
			sp->scb.chk = 1;
			sp->sense_count = SG_MAX_SENSE;
			movebytes(sgp->hd.sense_buffer, sp->u_sense.cmd_sense, sp->sense_count);
			if (sp->ux_errno == 0)
				sp->ux_errno = EIO;
		}
	}

	if (scgp->verbose > 0 && scgp->debug > 0) {
#ifdef	SG_GET_BUFSIZE
		js_fprintf((FILE *)scgp->errfile,
"status: 0x%08X pack_len: %d, reply_len: %d pack_id: %d result: %d wn: %d gn: %d cdb_len: %d sense_len: %d sense[0]: %02X\n",
				GETINT(sgp->hd.sg_cmd_status),
				GETINT(sgp->hd.pack_len),
				GETINT(sgp->hd.reply_len),
				GETINT(sgp->hd.pack_id),
				GETINT(sgp->hd.result),
				sgp->hd.want_new,
				sgp->hd.grant_new,
				sgp->hd.cdb_len,
				sgp->hd.sense_len,
				sgp->hd.sense_buffer[0]);
#else
		js_fprintf((FILE *)scgp->errfile,
				"pack_len: %d, reply_len: %d pack_id: %d result: %d sense[0]: %02X\n",
				GETINT(sgp->hd.pack_len),
				GETINT(sgp->hd.reply_len),
				GETINT(sgp->hd.pack_id),
				GETINT(sgp->hd.result),
				sgp->hd.sense_buffer[0]);
#endif
#ifdef	DEBUG
		js_fprintf((FILE *)scgp->errfile, "sense: ");
		for (i = 0; i < 16; i++)
			js_fprintf((FILE *)scgp->errfile, "%02X ", sgp->hd.sense_buffer[i]);
		js_fprintf((FILE *)scgp->errfile, "\n");
#endif
	}

	if (sp->timeout != scgp->deftimeout)
		sg_settimeout(f, scgp->deftimeout);
	return (0);
}
