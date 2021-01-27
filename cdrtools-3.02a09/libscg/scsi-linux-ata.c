/* @(#)scsi-linux-ata.c	1.16 13/05/28 Copyright 2002-2013 J. Schilling */
#ifndef lint
static	char ata_sccsid[] =
	"@(#)scsi-linux-ata.c	1.16 13/05/28 Copyright 2002-2013 J. Schilling";
#endif
/*
 *	Interface for Linux generic SCSI implementation (sg).
 *
 *	This inteface is ised with dev=ATAPI:b,t,l
 *
 *	This is the interface for the broken Linux SCSI generic driver.
 *	This is a hack, that tries to emulate the functionality
 *	of the scg driver.
 *
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 *
 *	Copyright (c) 2002-2013 J. Schilling
 *
 *	Thanks to Alexander Kern <alex.kern@gmx.de> for the idea and first
 *	code fragments for supporting the CDROM_SEND_PACKET ioctl() from
 *	the cdrom.c kernel driver. Please note that this interface in priciple
 *	is completely unneeded but the Linux kernel is just a cluster of
 *	code and does not support planned orthogonal interface systems.
 *	For this reason we need CDROM_SEND_PACKET in order to work around a
 *	bug in the linux kernel that prevents to use PCATA drives because
 *	the kernel panics if you try to put ide-scsi on top of the PCATA
 *	driver.
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

#ifdef	USE_ATAPI

LOCAL	char	_scg_atrans_version[] = "scsi-linux-ata.c-1.16";	/* The version for ATAPI transport*/

LOCAL	char *	scgo_aversion	__PR((SCSI *scgp, int what));
LOCAL	int	scgo_ahelp	__PR((SCSI *scgp, FILE *f));
LOCAL	int	scgo_aopen	__PR((SCSI *scgp, char *device));
LOCAL	int	scgo_aclose	__PR((SCSI *scgp));
LOCAL	long	scgo_amaxdma	__PR((SCSI *scgp, long amt));
LOCAL	int	scgo_anumbus	__PR((SCSI *scgp));
LOCAL	BOOL	scgo_ahavebus	__PR((SCSI *scgp, int));
LOCAL	int	scgo_afileno	__PR((SCSI *scgp, int, int, int));
LOCAL	int	scgo_ainitiator_id __PR((SCSI *scgp));
LOCAL	int	scgo_aisatapi	__PR((SCSI *scgp));
LOCAL	int	scgo_areset	__PR((SCSI *scgp, int what));
LOCAL	int	scgo_asend	__PR((SCSI *scgp));

LOCAL scg_ops_t atapi_ops = {
	scgo_asend,
	scgo_aversion,
	scgo_ahelp,
	scgo_aopen,
	scgo_aclose,
	scgo_amaxdma,
	scgo_getbuf,		/* Shared with SG driver */
	scgo_freebuf,		/* Shared with SG driver */
	scgo_anumbus,
	scgo_ahavebus,
	scgo_afileno,
	scgo_ainitiator_id,
	scgo_aisatapi,
	scgo_areset,
};

#define	HOST_EMPTY	0xF
#define	HOST_SCSI	0x0
#define	HOST_IDE	0x1
#define	HOST_USB	0x2
#define	HOST_IEEE1389	0x3
#define	HOST_PARALLEL	0x4
#define	HOST_OTHER	0xE


#define	typlocal(p, atapibus)		scglocal(p)->bc[atapibus].typ
#define	buslocal(p, atapibus)		scglocal(p)->bc[atapibus].bus
#define	hostlocal(p, atapibus)		scglocal(p)->bc[atapibus].host

#define	MAX_DMA_ATA (131072-1)	/* EINVAL (hart) ENOMEM (weich) bei mehr ... */
				/* Bei fehlerhaftem Sense Pointer kommt EFAULT */

LOCAL int scgo_send		__PR((SCSI * scgp));
LOCAL BOOL sg_amapdev		__PR((SCSI * scgp, int f, char *device, int *bus,
					int *target, int *lun));
LOCAL BOOL sg_amapdev_scsi	__PR((SCSI * scgp, int f, int *busp, int *tgtp,
					int *lunp, int *chanp, int *inop));
LOCAL int scgo_aget_first_free_atapibus __PR((SCSI * scgp, int subsystem,
					int host, int bus));
LOCAL int scgo_amerge		__PR((char *path, char *readedlink,
					char *buffer, int buflen));

/*
 * uncomment this when you will get a debug file #define DEBUG
 */
#ifdef DEBUG
#define	LOGFILE "scsi-linux-ata.log"
#define	log(a)	sglog a

LOCAL	void	sglog		__PR((const char *fmt, ...));

#include <schily/varargs.h>

/* VARARGS1 */
#ifdef	PROTOTYPES
LOCAL void
sglog(const char *fmt, ...)
#else
LOCAL void
error(fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;
	FILE	*f	 = fopen(LOGFILE, "a");

	if (f == NULL)
		return;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	js_fprintf(f, "%r", fmt, args);
	va_end(args);
	fclose(f);
}
#else
#define	log(a)
#endif	/* DEBUG */

LOCAL	int	scan_internal __PR((SCSI * scgp, int *fatal));

/*
 * Return version information for the low level SCSI transport code.
 * This has been introduced to make it easier to trace down problems
 * in applications.
 */
LOCAL char *
scgo_aversion(scgp, what)
	SCSI	*scgp;
	int	what;
{
	if (scgp != (SCSI *)0) {
		switch (what) {

		case SCG_VERSION:
			return (_scg_atrans_version);
		/*
		 * If you changed this source, you are not allowed to
		 * return "schily" for the SCG_AUTHOR request.
		 */
		case SCG_AUTHOR:
			return (_scg_auth_schily);
		case SCG_SCCS_ID:
			return (ata_sccsid);
		}
	}
	return ((char *)0);
}

LOCAL int
scgo_ahelp(scgp, f)
	SCSI	*scgp;
	FILE	*f;
{
	__scg_help(f, "ATA", "ATA Packet specific SCSI transport",
		"ATAPI:", "bus,target,lun", "ATAPI:1,2,0", TRUE, FALSE);
	return (0);
}

LOCAL int
scgo_aopen(scgp, device)
	SCSI	*scgp;
	char	*device;
{
	int	bus = scg_scsibus(scgp);
	int	target = scg_target(scgp);
	int	lun = scg_lun(scgp);

	register int	f;
	register int	b;
	register int	t;
	register int	l;
		int	nopen = 0;

	if (scgp->overbose) {
		error("Warning: dev=ATA: is preferred over dev=ATAPI:.\n");
		error("Warning: Using ATA Packet interface.\n");
	}
	if (scgp->overbose) {
		error("Warning: The related Linux kernel interface code seems to be unmaintained.\n");
		error("Warning: There is absolutely NO DMA, operations thus are slow.\n");
	}

	log(("\n<<<<<<<<<<<<<<<<  LOGGING ON >>>>>>>>>>>>>>>>>\n"));
	if (bus >= MAX_ATAPI_HOSTS || target >= MAX_TGT || lun >= MAX_LUN) {
		errno = EINVAL;
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Illegal value for bus, target or lun '%d,%d,%d'",
				bus, target, lun);

		return (-1);
	}

	if (scgp->local == NULL) {
		scgp->local = malloc(sizeof (struct scg_local));
		if (scgp->local == NULL) {
			return (0);
		}

		scglocal(scgp)->scgfile = -1;
		scglocal(scgp)->pgbus = -2;
		scglocal(scgp)->SCSIbuf = (char *)-1;
		scglocal(scgp)->pack_id = 5;
		scglocal(scgp)->drvers = -1;
		scglocal(scgp)->isold = -1;
		scglocal(scgp)->xbufsize = 0L;
		scglocal(scgp)->xbuf = NULL;


		for (b = 0; b < MAX_ATAPI_HOSTS; b++) {
			typlocal(scgp, b) = HOST_EMPTY;
			for (t = 0; t < MAX_TGT; t++) {
				for (l = 0; l < MAX_LUN; l++)
					scglocal(scgp)->scgfiles[b][t][l] = (short) -1;
			}
		}
	}

	if (device != NULL && strcmp(device, "ATAPI") == 0)
		goto atascan;

	/* if not scanning */
	if ((device != NULL && *device != '\0') || (bus == -2 && target == -2))
		goto openbydev;

atascan:
	if (scan_internal(scgp, &nopen)) {
		if (scgp->errstr)
			js_printf(scgp->errstr, "INFO: scan_internal(...) failed");
		return (-1);
	}
	return (nopen);

openbydev:
	if (device != NULL && strncmp(device, "ATAPI:", 6) == 0)
		device += 6;
	if (scgp->debug > 3) {
		js_fprintf((FILE *) scgp->errfile, "INFO: do scgo_open openbydev");
	}
	if (device != NULL && *device != '\0') {
		int	atapi_bus,
			starget,
			slun;

		f = open(device, O_RDONLY | O_NONBLOCK);

		if (f < 0) {
			if (scgp->errstr)
				js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
					"Cannot open '%s'", device);
			return (0);
		}
		if (sg_amapdev(scgp, f, device, &atapi_bus, &starget, &slun)) {
			scg_settarget(scgp, atapi_bus, starget, slun);
			return (++nopen);
		}
	}
	return (nopen);
}

LOCAL int
scan_internal(scgp, nopen)
	SCSI	*scgp;
	int	*nopen;
{
	int	i,
		f;
	int	atapi_bus,
		target,
		lun;
	char	device[128];
	/*
	 * try always with devfs
	 * unfortunatelly the solution with test of existing
	 * of '/dev/.devfsd' don't work, because it root.root 700
	 * and i don't like run suid root
	 */
	BOOL	DEVFS = TRUE;

	if (DEVFS) {
		for (i = 0; ; i++) {
			sprintf(device, "/dev/cdroms/cdrom%i", i);
			if ((f = open(device, O_RDONLY | O_NONBLOCK)) < 0) {
				if (errno != ENOENT && errno != ENXIO && errno != ENODEV && errno != EACCES) {
					if (scgp->debug > 4) {
						js_fprintf((FILE *) scgp->errfile,
						"try open(%s) return %i, errno %i, cancel\n", device, f, errno);
					}
					return (-2);
				} else if (errno == ENOENT || errno == ENODEV) {
					if (scgp->debug > 4) {
						js_fprintf((FILE *) scgp->errfile,
						"try open(%s) return %i, errno %i\n", device, f, errno);
					}
					if (0 == i) {
						DEVFS = FALSE;
						if (scgp->debug > 4) {
							js_fprintf((FILE *) scgp->errfile,
							"DEVFS not detected, continuing with old dev\n");
						}
					}
					break;
				}
				if (scgp->debug > 4) {
					if (errno == EACCES) {
						js_fprintf((FILE *) scgp->errfile,
						"errno (EACCESS), you don't have the needed rights for %s\n",
						device);
					}
					js_fprintf((FILE *) scgp->errfile,
					"try open(%s) return %i, errno %i, trying next cdrom\n",
					device, f, errno);
				}
			} else {
				if (scgp->debug > 4) {
					js_fprintf((FILE *) scgp->errfile,
					"try open(%s) return %i errno %i calling sg_mapdev(...)\n",
					device, f, errno);
				}
				if (sg_amapdev(scgp, f, device, &atapi_bus, &target, &lun)) {
					(++(*nopen));
				} else {
					close(f);
				}
			}
		}
	}
	if (!DEVFS) {
		/* for /dev/sr0 - /dev/sr? */
		for (i = 0; ; i++) {
			sprintf(device, "/dev/sr%i", i);
			if ((f = open(device, O_RDONLY | O_NONBLOCK)) < 0) {
				if (errno != ENOENT && errno != ENXIO && errno != ENODEV && errno != EACCES) {
					if (scgp->debug > 4) {
						js_fprintf((FILE *) scgp->errfile,
						"try open(%s) return %i, errno %i, cancel\n",
						device, f, errno);
					}
					return (-2);
				} else if (errno == ENOENT || errno == ENODEV) {
					break;
				}
			} else {
				if (sg_amapdev(scgp, f, device, &atapi_bus, &target, &lun)) {
					(++(*nopen));
				} else {
					close(f);
				}
			}
		}

		/* for /dev/hda - /dev/hdz */
		for (i = 'a'; i <= 'z'; i++) {
			sprintf(device, "/dev/hd%c", i);
			if ((f = open(device, O_RDONLY | O_NONBLOCK)) < 0) {
				if (errno != ENOENT && errno != ENXIO && errno != EACCES) {
					if (scgp->debug > 4) {
						js_fprintf((FILE *) scgp->errfile,
						"try open(%s) return %i, errno %i, cancel\n",
						device, f, errno);
					}
					return (-2);
				} else if (errno == ENOENT || errno == ENODEV) {
					break;
				}
			} else {
				/* ugly hack, make better, when you can. Alex */
				if (0 > ioctl(f, CDROM_DRIVE_STATUS, CDSL_CURRENT)) {
					if (scgp->debug > 4) {
						js_fprintf((FILE *) scgp->errfile,
						"%s is not a cdrom, skipping\n",
						device);
					}
					close(f);
				} else if (sg_amapdev(scgp, f, device, &atapi_bus, &target, &lun)) {
					(++(*nopen));
				} else {
					close(f);
				}
			}
		}
	}
	return (0);
}

LOCAL int
scgo_aclose(scgp)
	SCSI	*scgp;
{
	register int	f;
	register int	h;
	register int	t;
	register int	l;

	if (scgp->local == NULL)
		return (-1);

	for (h = 0; h < MAX_ATAPI_HOSTS; h++) {
		typlocal(scgp, h) = (HOST_EMPTY);
		for (t = 0; t < MAX_TGT; t++) {
			for (l = 0; l < MAX_LUN; l++) {
				f = scglocal(scgp)->scgfiles[h][t][l];
				if (f >= 0)
					close(f);
				scglocal(scgp)->scgfiles[h][t][l] = (short) -1;
			}
		}
	}

	if (scglocal(scgp)->xbuf != NULL) {
		free(scglocal(scgp)->xbuf);
		scglocal(scgp)->xbufsize = 0L;
		scglocal(scgp)->xbuf = NULL;
	}
	log(("<<<<<<<<<<<<<<<<  LOGGING OFF >>>>>>>>>>>>>>>>>\n\n"));
	return (0);
}

LOCAL int
scgo_aget_first_free_atapibus(scgp, subsystem, host, bus)
	SCSI	*scgp;
	int	subsystem;
	int	host;
	int	bus;
{
	int	first_free_atapi_bus;

	for (first_free_atapi_bus = 0;
			first_free_atapi_bus < MAX_ATAPI_HOSTS;
						first_free_atapi_bus++) {

		if (typlocal(scgp, first_free_atapi_bus) == HOST_EMPTY ||
		    (typlocal(scgp, first_free_atapi_bus) == subsystem &&
		    hostlocal(scgp, first_free_atapi_bus) == host &&
		    buslocal(scgp, first_free_atapi_bus) == bus))
			break;
	}

	if (first_free_atapi_bus >= MAX_ATAPI_HOSTS) {
		errmsgno(EX_BAD, "ERROR: in scgo_get_first_free_atapibus(...). Too many CDROMs, more than %i",
			MAX_ATAPI_HOSTS);
		errmsgno(EX_BAD, "Increase MAX_ATAPI_HOSTS in scsi-linux-ata.c and recompile!");
		return (-1);
	}
	return (first_free_atapi_bus);
}

LOCAL int
scgo_amerge(path, readedlink, buffer, buflen)
	char	*path;
	char	*readedlink;
	char	*buffer;
	int	buflen;
{
	char	*aa;

#define	TOKEN_ARRAY		20
#define	LAST_CHAR(x)		(x)[strlen((x))-1]
#define	ONE_CHAR_BACK(x)	(x)[strlen((x))-1] = '\0'
	char	*ppa[TOKEN_ARRAY];
	char	*pa;

	int	i;
	int	len;
	char	seps[] = "/";
	char	*last_slash;

	if (!path || !readedlink || !buffer)
		return (-EINVAL);

	if ('/' == readedlink[0]) {
		aa = (char *) malloc(strlen(readedlink) + 1);
		if (!aa)
			return (-ENOMEM);

		strcpy(aa, readedlink);
	} else {
		aa = (char *) malloc(strlen(path) + strlen(readedlink) + 1);
		if (!aa)
			return (-ENOMEM);

		strcpy(aa, path);
		if (LAST_CHAR(aa) == '/') {
			ONE_CHAR_BACK(aa);
		}
		last_slash = strrchr(aa, '/');
		if (last_slash == NULL)
			strcpy(aa, "/");
		else
			*(++last_slash) = '\0';
		strcat(aa, readedlink);
	}
	memset(ppa, 0x00, sizeof (ppa));

	for (i = 0, pa = strtok(aa, seps);
		i < TOKEN_ARRAY && pa != NULL;
		++i, pa = strtok(NULL, seps)) {
		ppa[i] = pa;
	}

	if (i == TOKEN_ARRAY) {
		free(aa);
		return (-ENOMEM);
	}
	for (i = 0; i < TOKEN_ARRAY && ppa[i]; i++) {
		if (strcmp(ppa[i], "..") == 0) {
			ppa[i] = NULL;
			if (i > 1)
				ppa[i - 1] = NULL;
		}
	}

	/* dry run */
	len = 0;
	for (i = 0; i < TOKEN_ARRAY; i++) {
		if (ppa[i]) {
			len += 1;
			len += strlen(ppa[i]);
		}
	}
	if (0 == len)
		len = 1;

	if (len + 1 <= buflen) {
		strcpy(buffer, "");
		for (i = 0; i < TOKEN_ARRAY; i++) {
			if (ppa[i]) {
				strcat(buffer, "/");
				strcat(buffer, ppa[i]);
			}
		}

		if (strlen(buffer) == 0)
			strcpy(buffer, "/");
	}
	free(aa);

	return (len + 1);
}

/*
 *	/dev/cdroms/cdrom0	first CD-ROM
 *	/dev/cdroms/cdrom1	second CD-ROM
 *
 *
 *	SCSI Devices
 *
 *	To uniquely identify any SCSI device requires the following information:
 *
 *	controller	(host adapter)
 *	bus		(SCSI channel)
 *	target		(SCSI ID)
 *	unit		(Logical Unit Number)
 *
 *	All SCSI devices are placed under /dev/scsi (assuming devfs is mounted on /dev).
 *	Hence, a SCSI device with the following parameters:
 *		c=1,b=2,t=3,u=4 would appear as:
 *
 *		/dev/scsi/host1/bus2/target3/lun4	device directory
 *
 *	Inside this directory, a number of device entries may be created,
 *	depending on which SCSI device-type drivers were installed.
 *
 *	See the section on the disc naming scheme to see what entries
 *	the SCSI disc driver creates.
 *
 *	See the section on the tape naming scheme to see what entries
 *	the SCSI tape driver creates.
 *
 *	The SCSI CD-ROM driver creates:  cd
 *	The SCSI generic driver creates: generic
 *
 *	IDE Devices
 *
 *	To uniquely identify any IDE device requires the following information:
 *
 *	controller
 *	bus		(0/1 aka. primary/secondary)
 *	target		(0/1 aka. master/slave)
 *	unit
 *
 *	All IDE devices are placed under /dev/ide, and uses a similar
 *	naming scheme to the SCSI subsystem.
 *
 *
 *	Example /dev/cdroms/cdrom0 ->  /dev/scsi/host1/bus2/target3/lun4/cd
 *	Example /dev/cdroms/cdrom1 ->  /dev/ide/host1/bus0/target1/lun4/cd
 *
 */
LOCAL BOOL
sg_amapdev(scgp, f, device, atapibus, target, lun)
	SCSI	*scgp;
	int	f;
	char	*device;
	int	*atapibus;
	int	*target;
	int	*lun;
{
	struct host {
		char	host[4];
		char	host_no;
	};
	struct bus {
		char	bus[3];
		char	bus_no;
	};
	struct target {
		char	target[6];
		char	target_no;
	};
	struct lun {
		char	lun[3];
		char	lun_no;
	};

	int	h,
		b,
		t,
		l;

#define	TOKEN_DEV		"dev"
#define	TOKEN_SUBSYSTEM_SCSI	"scsi"
#define	TOKEN_SUBSYSTEM_IDE	"ide"
#define	TOKEN_HOST		"host"
#define	TOKEN_BUS		"bus"
#define	TOKEN_TARGET		"target"
#define	TOKEN_LUN		"lun"
#define	TOKEN_CD		"cd"

#define	ID_TOKEN_DEV		0
#define	ID_TOKEN_SUBSYSTEM	1
#define	ID_TOKEN_HOST		2
#define	ID_TOKEN_BUS		3
#define	ID_TOKEN_TARGET		4
#define	ID_TOKEN_LUN		5
#define	ID_TOKEN_CD		6
#define	ID_TOKEN_LAST		ID_TOKEN_CD
#define	ID_TOKEN_MAX		ID_TOKEN_LAST + 2
#define	CHARTOINT(x)		(abs(atoi(&x)))

	char		*token[ID_TOKEN_MAX],
			*seps = "/";
	int		i,
			result;
	struct stat	buf;

#ifndef MAX_PATH
#define	MAX_PATH 260
#endif
#define	LOCAL_MAX_PATH MAX_PATH
	char		tmp[LOCAL_MAX_PATH],
			tmp1[LOCAL_MAX_PATH];
	int		first_free_atapi_bus;
	int		subsystem = HOST_EMPTY;

	/* old DEV */
	typedef struct {
		char		prefix[2];
		char		device;
	} old_dev;
	/* strtok need char* instead of const char* */
	result = stat(device, &buf);
	if (result || !S_ISBLK(buf.st_mode))
		return (FALSE);

	result = lstat(device, &buf);
	if (!result && S_ISLNK(buf.st_mode)) {
		result = readlink(device, tmp, LOCAL_MAX_PATH);
		if (result > 0 && result < LOCAL_MAX_PATH) {
			tmp[result] = '\0';

			result = scgo_amerge(device, tmp, tmp1, LOCAL_MAX_PATH);
			if (result > 0 && result < LOCAL_MAX_PATH) {
				tmp1[result] = '\0';
				strcpy(tmp, tmp1);
			} else {
				errmsgno(EX_BAD,
				"ERROR: with link merging! base %s link %s, result of merging %i\n",
					device, tmp, result);
				return (FALSE);
			}
		} else {
			errmsgno(EX_BAD,
			"ERROR: with link reading! link %s, result of readlink %i\n",
				device, result);
			return (FALSE);
		}
	} else {
		strncpy(tmp, device, sizeof (tmp));
	}
	if (scgp->debug > 3) {
		js_fprintf((FILE *) scgp->errfile, "INFO: %s -> %s\n", device, tmp);
	}
	memset(token, 0x00, sizeof (token));
	i = 0;
	token[i] = strtok(tmp, seps);
	while (token[i] != NULL && (++i) && i < ID_TOKEN_MAX) {
		token[i] = strtok(NULL, seps);
	}

	if (i == ID_TOKEN_MAX ||
		!(token[ID_TOKEN_DEV]) ||
		strcmp(token[ID_TOKEN_DEV], TOKEN_DEV)) {

		errmsgno(EX_BAD, "ERROR: unknow format\n");
		errmsgno(EX_BAD, "EXAMPLE: /dev/scsi/host1/bus2/target3/lun4/cd\n");
		errmsgno(EX_BAD, "EXAMPLE: /dev/ide/host0/bus0/target1/lun0/cd\n");
		errmsgno(EX_BAD, "EXAMPLE: /dev/hda or /dev/sr0\n");
		return (FALSE);
	}
	if (!(strcmp(token[ID_TOKEN_SUBSYSTEM], TOKEN_SUBSYSTEM_SCSI)) ||
	    !(strcmp(token[ID_TOKEN_SUBSYSTEM], TOKEN_SUBSYSTEM_IDE))) {
		h = CHARTOINT(((struct host *) token[ID_TOKEN_HOST])->host_no);
		b = CHARTOINT(((struct bus *) token[ID_TOKEN_BUS])->bus_no);
		t = CHARTOINT(((struct target *) token[ID_TOKEN_TARGET])->target_no);
		l = CHARTOINT(((struct lun *) token[ID_TOKEN_LUN])->lun_no);
#ifdef PARANOID
		if (strncmp(token[ID_TOKEN_HOST], TOKEN_HOST, strlen(TOKEN_HOST))) {
			log(("ERROR: invalid host specified\n"));
			return (FALSE);
		}
		if (strncmp(token[ID_TOKEN_BUS], TOKEN_BUS, strlen(TOKEN_BUS))) {
			log(("ERROR: invalid bus specified\n"));
			return (FALSE);
		}
		if (strncmp(token[ID_TOKEN_TARGET], TOKEN_TARGET, strlen(TOKEN_TARGET))) {
			log(("ERROR: invalid target specified\n"));
			return (FALSE);
		}
		if (strncmp(token[ID_TOKEN_LUN], TOKEN_LUN, strlen(TOKEN_LUN))) {
			log(("ERROR: invalid lun specified\n"));
			return (FALSE);
		}
		if (!(strcmp(token[ID_TOKEN_SUBSYSTEM], TOKEN_SUBSYSTEM_IDE))) {
			if (b > 1 || t > 1) {
				log(("ERROR: invalid bus or target for IDE specified\n"));
				return (FALSE);
			}
		}
#endif	/* PARANOID */

		if (!(strcmp(token[ID_TOKEN_SUBSYSTEM], TOKEN_SUBSYSTEM_IDE))) {
			subsystem = HOST_IDE;
		} else if (!(strcmp(token[ID_TOKEN_SUBSYSTEM], TOKEN_SUBSYSTEM_SCSI))) {
			subsystem = HOST_SCSI;
		} else {
			subsystem = HOST_OTHER;
		}
	} else if (!token[ID_TOKEN_HOST] &&
		strlen(token[ID_TOKEN_SUBSYSTEM]) == sizeof (old_dev)) {
		char	j;

		old_dev	*pDev = (old_dev *) token[ID_TOKEN_SUBSYSTEM];

		if (strncmp(pDev->prefix, "hd", 2) == 0) {
			j = pDev->device - ('a');

			subsystem = HOST_IDE;
			h = j / 4;
			b = (j % 4) / 2;
			t = (j % 4) % 2;
			l = 0;
		} else if (strncmp(pDev->prefix, "sr", 2) == 0) {
#ifdef	nonono
			if (pDev->device >= '0' && pDev->device <= '9')
				j = pDev->device - ('0');
			else
				j = pDev->device - ('a');


			h = j / 4;
			b = (j % 4) / 2;
			t = (j % 4) % 2;
			l = 0;
#endif	/* nonono */
			/* other solution, with ioctl */
			int	Chan	= -1,
				Ino	= -1,
				Bus	= -1,
				Target	= -1,
				Lun	= -1;

			subsystem = HOST_SCSI;
			sg_amapdev_scsi(scgp, f, &Bus, &Target, &Lun, &Chan, &Ino);

			/* For old kernels try to make the best guess. */
#ifdef	nonono
				int	n;
				Ino |= Chan << 8;
				n = sg_mapbus(scgp, Bus, Ino);
				if (Bus == -1) {
					Bus = n;
					if (scgp->debug > 0) {
						js_fprintf((FILE *)scgp->errfile,
							"SCSI Bus: %d (mapped from %d)\n",
							Bus, Ino);
					}
				}
/*				It is me too high ;-()*/
#endif	/* nonono */
			h = Ino;
			b = Chan;
			t = Target;
			l = Lun;
		} else {
			errmsgno(EX_BAD, "ERROR: unknow subsystem (%s) in (%s)\n",
				token[ID_TOKEN_SUBSYSTEM], device);
			return (FALSE);
		}
	} else {
		errmsgno(EX_BAD, "ERROR: unknow subsystem (%s) in (%s)\n",
			token[ID_TOKEN_SUBSYSTEM], device);
		return (FALSE);
	}

	if (scgp->verbose)
		js_printf(scgp->errstr, "INFO: subsystem %s: h %i, b %i, t %i, l %i",
			token[ID_TOKEN_SUBSYSTEM], h, b, t, l);

	first_free_atapi_bus = scgo_aget_first_free_atapibus(scgp, subsystem, h, b);
	if (-1 == first_free_atapi_bus) {
		return (FALSE);
	}
	if (scglocal(scgp)->scgfiles[first_free_atapi_bus][t][l] != (-1)) {
		errmsgno(EX_BAD, "ERROR: this cdrom is already mapped %s(%d,%d,%d)\n",
			device, first_free_atapi_bus, t, l);
		return (FALSE);
	} else {
		scglocal(scgp)->scgfiles[first_free_atapi_bus][t][l] = f;
		typlocal(scgp, first_free_atapi_bus) = subsystem;
		hostlocal(scgp, first_free_atapi_bus) = h;
		buslocal(scgp, first_free_atapi_bus) = b;
		*atapibus = first_free_atapi_bus;
		*target = t;
		*lun = l;

		if (scgp->debug > 1) {
			js_fprintf((FILE *) scgp->errfile,
				"INFO: /dev/%s, (host%d/bus%d/target%d/lun%d) will be mapped on the atapi bus No %d (%d,%d,%d)\n",
				token[ID_TOKEN_SUBSYSTEM], h, b, t, l,
				first_free_atapi_bus, first_free_atapi_bus, t, l);
		}
	}
	return (TRUE);
}

LOCAL BOOL
sg_amapdev_scsi(scgp, f, busp, tgtp, lunp, chanp, inop)
	SCSI	*scgp;
	int	f;
	int	*busp;
	int	*tgtp;
	int	*lunp;
	int	*chanp;
	int	*inop;
{
	struct sg_id {
		long	l1;	/* target | lun << 8 | channel << 16 | low_ino << 24 */
		long	l2;	/* Unique id */
	} sg_id;
	int	Chan;
	int	Ino;
	int	Bus;
	int	Target;
	int	Lun;

	if (ioctl(f, SCSI_IOCTL_GET_IDLUN, &sg_id))
		return (FALSE);

	if (scgp->debug > 0) {
		js_fprintf((FILE *) scgp->errfile,
			"INFO: l1: 0x%lX l2: 0x%lX\n", sg_id.l1, sg_id.l2);
	}
	if (ioctl(f, SCSI_IOCTL_GET_BUS_NUMBER, &Bus) < 0) {
		Bus = -1;
	}
	Target = sg_id.l1 & 0xFF;
	Lun = (sg_id.l1 >> 8) & 0xFF;
	Chan = (sg_id.l1 >> 16) & 0xFF;
	Ino = (sg_id.l1 >> 24) & 0xFF;
	if (scgp->debug > 0) {
		js_fprintf((FILE *) scgp->errfile,
			"INFO: Bus: %d Target: %d Lun: %d Chan: %d Ino: %d\n",
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

LOCAL long
scgo_amaxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	/*
	 * EINVAL (hart) ENOMEM (weich) bei mehr ...
	 * Bei fehlerhaftem Sense Pointer kommt EFAULT
	 */
	return (MAX_DMA_ATA);
}

LOCAL int
scgo_anumbus(scgp)
	SCSI	*scgp;
{
	return (MAX_ATAPI_HOSTS);
}

LOCAL BOOL
scgo_ahavebus(scgp, busno)
	SCSI	*scgp;
	int	busno;
{
	register int	t;
	register int	l;

	if (busno < 0 || busno >= MAX_ATAPI_HOSTS)
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
scgo_afileno(scgp, busno, tgt, tlun)
	SCSI	*scgp;
	int	busno;
	int	tgt;
	int	tlun;
{
	if (busno < 0 || busno >= MAX_ATAPI_HOSTS ||
		tgt < 0 || tgt >= MAX_TGT ||
		tlun < 0 || tlun >= MAX_LUN)
		return (-1);

	if (scgp->local == NULL)
		return (-1);

	return ((int) scglocal(scgp)->scgfiles[busno][tgt][tlun]);
}

LOCAL int
scgo_ainitiator_id(scgp)
	SCSI	*scgp;
{
	js_printf(scgp->errstr, "NOT IMPELEMENTED: scgo_initiator_id");
	return (-1);
}

LOCAL int
scgo_aisatapi(scgp)
	SCSI	*scgp;
{
	int atapibus = scgp->addr.scsibus;
	int typ = typlocal(scgp, atapibus);
	if (typ == HOST_EMPTY)
		return (-1);
	if (typ != HOST_SCSI)
		return (1);
	else
		return (0);
}

LOCAL int
scgo_areset(scgp, what)
	SCSI	*scgp;
	int	what;
{
	if (what == SCG_RESET_NOP)
		return (0);

	if (what == SCG_RESET_TGT || what == SCG_RESET_BUS)
		return (ioctl(what, CDROMRESET));

	return (-1);
}

LOCAL int
scgo_asend(scgp)
	SCSI	*scgp;
{
	struct scg_cmd	*sp = scgp->scmd;
	int		ret,
			i;
	struct cdrom_generic_command sg_cgc;
	struct request_sense sense_cgc;
static	uid_t		cureuid = 0;	/* XXX Hack until we have uid management */

#ifdef DEBUG
	char		tmp_send[340],
			tmp_read[340],
			tmp_sense[340],
			tmp1[30];
	int		j;
	char		*p;
#endif

	if (scgp->fd < 0) {
		sp->error = SCG_FATAL;
		sp->ux_errno = EIO;
		return (0);
	}
	if (sp->cdb_len > CDROM_PACKET_SIZE) {
		sp->error = SCG_FATAL;
		sp->ux_errno = EIO;
		return (0);
	}
	/* initialize */
	fillbytes((caddr_t) & sg_cgc, sizeof (sg_cgc), '\0');
	fillbytes((caddr_t) & sense_cgc, sizeof (sense_cgc), '\0');

	if (sp->flags & SCG_RECV_DATA) {
		sg_cgc.data_direction = CGC_DATA_READ;
	} else if (sp->size > 0) {
		sg_cgc.data_direction = CGC_DATA_WRITE;
	} else {
		sg_cgc.data_direction = CGC_DATA_NONE;
	}
#if LINUX_VERSION_CODE >= 0x020403
	if (sp->flags & SCG_SILENT) {
		sg_cgc.quiet = 1;
	}
#endif
	for (i = 0; i < sp->cdb_len; i++) {
		sg_cgc.cmd[i] = sp->cdb.cmd_cdb[i];
	}

	sg_cgc.buflen = sp->size;
	sg_cgc.buffer = (void *)sp->addr; /* Workaround silly type in sg_cgc */

	if (sp->sense_len > sizeof (sense_cgc))
		sense_cgc.add_sense_len = sizeof (sense_cgc) - 8;
	else
		sense_cgc.add_sense_len = sp->sense_len - 8;

	sg_cgc.sense = &sense_cgc;
#if LINUX_VERSION_CODE >= 0x020403
	sg_cgc.timeout = sp->timeout * 1000;
#endif
#ifdef DEBUG
	strcpy(tmp_send, "send cmd:\n");
	for (j = 0; j < sp->cdb_len; j++) {
		sprintf(tmp1, " %02X", sp->cdb.cmd_cdb[j]);
		strcat(tmp_send, tmp1);
	}
	strcat(tmp_send, "\n");

	if (sg_cgc.data_direction == CGC_DATA_WRITE) {
		int	z;

		sprintf(tmp1, "data_write: %i bytes\n", sp->size);
		strcat(tmp_send, tmp1);
		for (j = 0, z = 1; j < 80 && j < sp->size; j++, z++) {
			if (z > 16) {
				z = 1;
				strcat(tmp_send, "\n");
			}
			sprintf(tmp1, " %02X", (unsigned char) (sp->addr[j]));
			strcat(tmp_send, tmp1);
		}
		strcat(tmp_send, "\n");

		if (sp->size > 80) {
			strcat(tmp_send, "...\n");
		}
	}
#endif	/* DEBUG */

	if (cureuid != 0)
		seteuid(0);
again:
	errno = 0;
	if ((ret = ioctl(scgp->fd, CDROM_SEND_PACKET, &sg_cgc)) < 0)
		sp->ux_errno = geterrno();
	if (ret < 0 && geterrno() == EPERM) {	/* XXX Hack until we have uid management */
		cureuid = geteuid();
		if (seteuid(0) >= 0)
			goto again;
	}
	if (cureuid != 0)
		seteuid(cureuid);

	if (ret < 0 && scgp->debug > 4) {
		js_fprintf((FILE *) scgp->errfile,
			"ioctl(CDROM_SEND_PACKET) ret: %d\n", ret);
	}
	/*
	 * copy scsi data back
	 */
	if (sp->flags & SCG_RECV_DATA && ((void *) sp->addr != (void *) sg_cgc.buffer)) {
		memcpy(sp->addr, sg_cgc.buffer, (sp->size < sg_cgc.buflen) ? sp->size : sg_cgc.buflen);
		if (sg_cgc.buflen > sp->size)
			sp->resid = sg_cgc.buflen - sp->size;
	}
	sp->error = SCG_NO_ERROR;
#ifdef DEBUG
	if (ret < 0) {
		switch (sp->ux_errno) {
		case ENOTTY:
			p = "ENOTTY";
			break;
		case EINVAL:
			p = "EINVAL";
			break;
		case ENXIO:
			p = "ENXIO";
			break;
		case EPERM:
			p = "EPERM";
			break;
		case EACCES:
			p = "EACCES";
			break;
		case EIO:
			p = "EIO";
			break;
		case ENOMEDIUM:
			p = "ENOMEDIUM";
			break;
		case EDRIVE_CANT_DO_THIS:
			p = "EDRIVE_CANT_DO_THIS";
			break;
		default:
			p = "UNKNOW";
		};
		log(("%s", tmp_send));
		log(("ERROR: returns %i errno %i(%s)\n", ret, sp->ux_errno, p));
	}
#endif	/* DEBUG */
	if (ret < 0) {
		/*
		 * Check if SCSI command could not be send at all.
		 * Linux usually returns EINVAL for an unknown ioctl.
		 * In case somebody from the Linux kernel team learns that the
		 * corect errno would be ENOTTY, we check for this errno too.
		 */
		if (sp->ux_errno == EINVAL) {
			/*
			 * Try to work around broken Linux kernel design...
			 * If SCSI Sense Key is 0x05 (Illegal request), Linux
			 * returns a useless EINVAL making it close to
			 * impossible distinct from "Illegal ioctl()" or
			 * "Invalid parameter".
			 */
			if ((((Uchar *)sg_cgc.sense)[0] != 0) ||
			    (((Uchar *)sg_cgc.sense)[2] != 0))
				sp->ux_errno = EIO;

		} else if ((sp->ux_errno == ENOTTY || sp->ux_errno == EINVAL)) {
			/*
			 * May be "Illegal ioctl()".
			 */
			return (-1);
		}
		if (sp->ux_errno == ENXIO ||
		    sp->ux_errno == EPERM ||
		    sp->ux_errno == EACCES) {
			return (-1);
		}
	} else if (ret == 0) {
#ifdef DEBUG
		if (sg_cgc.data_direction == CGC_DATA_READ) {
			int	z;

			sprintf(tmp_read, "data_read: %i bytes\n", sp->size);
			for (j = 0, z = 1; j < 80 && j < sp->size; j++, z++) {
				if (z > 16) {
					z = 1;
					strcat(tmp_read, "\n");
				}
				sprintf(tmp1, " %02X", (unsigned char) (sp->addr[j]));
				strcat(tmp_read, tmp1);
			}
			strcat(tmp_read, "\n");
			if (sp->size > 80) {
				strcat(tmp_read, "...\n");
			}
		}
#endif	/* DEBUG */
	}
	/*
	 * copy sense back
	 */
	if (ret < 0 && sg_cgc.sense->error_code) {
		sp->sense_count = sense_cgc.add_sense_len + 8;
#ifdef DEBUG
		sprintf(tmp_sense, "sense_data: length %i\n", sp->sense_count);
		for (j = 0; j < sp->sense_count; j++) {
			sprintf(tmp1, " %02X", (((unsigned char *) (&sense_cgc))[j]));
			strcat(tmp_sense, tmp1);
		}
		log(("%s\n", tmp_sense));

		sprintf(tmp_sense, "sense_data: error code 0x%02X, sense key 0x%02X,"
			" additional length %i, ASC 0x%02X, ASCQ 0x%02X\n",
			sg_cgc.sense->error_code, sg_cgc.sense->sense_key,
			sg_cgc.sense->add_sense_len, sg_cgc.sense->asc,
			sg_cgc.sense->ascq);

		log(("%s\n", tmp_sense));
#endif	/* DEBUG */
		memcpy(sp->u_sense.cmd_sense, /* (caddr_t) */ &sense_cgc, SCG_MAX_SENSE);
		sp->u_scb.cmd_scb[0] = ST_CHK_COND;

		switch (sg_cgc.sense->sense_key) {
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

	sp->resid = 0;
	return (0);
}
#endif	/* USE_ATAPI */
