/* @(#)scsi-sun.c	1.90 13/05/14 Copyright 1988,1995,2000-2013 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-sun.c	1.90 13/05/14 Copyright 1988,1995,2000-2013 J. Schilling";
#endif
/*
 *	SCSI user level command transport routines for
 *	the SCSI general driver 'scg'.
 *
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 *
 *	Copyright (c) 1988,1995,2000-2013 J. Schilling
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

#include <scg/scgio.h>

#include <schily/libport.h>	/* Needed for gethostid() */
#ifdef	HAVE_SUN_DKIO_H
#	include <sun/dkio.h>

#	define	dk_cinfo	dk_conf
#	define	dki_slave	dkc_slave
#	define	DKIO_GETCINFO	DKIOCGCONF
#endif
#ifdef	HAVE_SYS_DKIO_H
#	include <sys/dkio.h>

#	define	DKIO_GETCINFO	DKIOCINFO
#endif

#define	TARGET(slave)	((slave) >> 3)
#define	LUN(slave)	((slave) & 07)

/*
 * Tht USCSI ioctl() is not usable on SunOS 4.x
 */
#ifdef	__SVR4
/*#define	VOLMGT_DEBUG*/
#include <volmgt.h>
#include <schily/stat.h>
#	define	USE_USCSI
#endif

LOCAL	char	_scg_trans_version[] = "scg-1.90";	/* The version for /dev/scg	*/
#ifdef	USE_USCSI
LOCAL	char	_scg_utrans_version[] = "uscsi-1.90";	/* The version for USCSI	*/

LOCAL	int	scgo_uhelp	__PR((SCSI *scgp, FILE *f));
LOCAL	int	scgo_uopen	__PR((SCSI *scgp, char *device));
LOCAL	int	scgo_volopen	__PR((SCSI *scgp, char *devname));
LOCAL	int	scgo_openmedia	__PR((SCSI *scgp, char *mname));
LOCAL	int	scgo_uclose	__PR((SCSI *scgp));
LOCAL	int	scgo_ucinfo	__PR((int f, struct dk_cinfo *cp, int debug));
LOCAL	int	scgo_ugettlun	__PR((int f, int *tgtp, int *lunp));
LOCAL	long	scgo_umaxdma	__PR((SCSI *scgp, long amt));
LOCAL	int	scgo_openide	__PR((void));
LOCAL	int	scgo_unumbus	__PR((SCSI *scgp));
LOCAL	BOOL	scgo_uhavebus	__PR((SCSI *scgp, int));
LOCAL	int	scgo_ufileno	__PR((SCSI *scgp, int, int, int));
LOCAL	int	scgo_uinitiator_id __PR((SCSI *scgp));
LOCAL	int	scgo_uisatapi	__PR((SCSI *scgp));
LOCAL	int	scgo_ureset	__PR((SCSI *scgp, int what));
LOCAL	int	scgo_usend	__PR((SCSI *scgp));

LOCAL	int	have_volmgt = -1;

LOCAL scg_ops_t sun_uscsi_ops = {
	scgo_usend,
	scgo_version,		/* Shared with SCG driver */
	scgo_uhelp,
	scgo_uopen,
	scgo_uclose,
	scgo_umaxdma,
	scgo_getbuf,		/* Shared with SCG driver */
	scgo_freebuf,		/* Shared with SCG driver */
	scgo_unumbus,
	scgo_uhavebus,
	scgo_ufileno,
	scgo_uinitiator_id,
	scgo_uisatapi,
	scgo_ureset,
};
#endif

/*
 * Need to move this into an scg driver ioctl.
 */
/*#define	MAX_DMA_SUN4M	(1024*1024)*/
#define	MAX_DMA_SUN4M	(124*1024)	/* Currently max working size */
/*#define	MAX_DMA_SUN4C	(126*1024)*/
#define	MAX_DMA_SUN4C	(124*1024)	/* Currently max working size */
#define	MAX_DMA_SUN3	(63*1024)
#define	MAX_DMA_SUN386	(56*1024)
#define	MAX_DMA_OTHER	(32*1024)

#define	ARCH_MASK	0xF0
#define	ARCH_SUN2	0x00
#define	ARCH_SUN3	0x10
#define	ARCH_SUN4	0x20
#define	ARCH_SUN386	0x30
#define	ARCH_SUN3X	0x40
#define	ARCH_SUN4C	0x50
#define	ARCH_SUN4E	0x60
#define	ARCH_SUN4M	0x70
#define	ARCH_SUNX	0x80

/*
 * We are using a "real" /dev/scg?
 */
#define	scsi_xsend(scgp)	ioctl((scgp)->fd, SCGIO_CMD, (scgp)->scmd)
#define	MAX_SCG		16	/* Max # of SCSI controllers */
#define	MAX_TGT		16
#define	MAX_LUN		8

struct scg_local {
	union {
		int	SCG_files[MAX_SCG];
#ifdef	USE_USCSI
		short	scg_files[MAX_SCG][MAX_TGT][MAX_LUN];
#endif
	} u;
};
#define	scglocal(p)	((struct scg_local *)((p)->local))
#define	scgfiles(p)	(scglocal(p)->u.SCG_files)

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
#ifdef	USE_USCSI
			if (scgp->ops == &sun_uscsi_ops)
				return (_scg_utrans_version);
#endif
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
	__scg_help(f, "scg", "Generic transport independent SCSI",
		"", "bus,target,lun", "1,2,0", TRUE, FALSE);
#ifdef	USE_USCSI
	scgo_uhelp(scgp, f);
#endif
	return (0);
}

LOCAL int
scgo_open(scgp, device)
	SCSI	*scgp;
	char	*device;
{
		int	busno	= scg_scsibus(scgp);
		int	tgt	= scg_target(scgp);
/*		int	tlun	= scg_lun(scgp);*/
	register int	f;
	register int	i;
	register int	nopen = 0;
	char		devname[32];

	if (busno >= MAX_SCG) {
		errno = EINVAL;
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Illegal value for busno '%d'", busno);
		return (-1);
	}

	if ((device != NULL && *device != '\0') || (busno == -2 && tgt == -2)) {
#ifdef	USE_USCSI
		scgp->ops = &sun_uscsi_ops;
		return (SCGO_OPEN(scgp, device));
#else
		errno = EINVAL;
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Open by 'devname' not supported on this OS");
		return (-1);
#endif
	}

	if (scgp->local == NULL) {
		scgp->local = malloc(sizeof (struct scg_local));
		if (scgp->local == NULL) {
			if (scgp->errstr)
				js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE, "No memory for scg_local");
			return (0);
		}

		for (i = 0; i < MAX_SCG; i++) {
			scgfiles(scgp)[i] = -1;
		}
	}


	for (i = 0; i < MAX_SCG; i++) {
		/*
		 * Skip unneeded devices if not in SCSI Bus scan open mode
		 */
		if (busno >= 0 && busno != i)
			continue;
		js_snprintf(devname, sizeof (devname), "/dev/scg%d", i);
		f = open(devname, O_RDWR);
		if (f < 0) {
			if (errno != ENOENT && errno != ENXIO) {
				if (scgp->errstr)
					js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
						"Cannot open '%s'", devname);
				return (-1);
			}
		} else {
			nopen++;
		}
		scgfiles(scgp)[i] = f;
	}
#ifdef	USE_USCSI
	if (nopen <= 0) {
		if (scgp->local != NULL) {
			free(scgp->local);
			scgp->local = NULL;
		}
		scgp->ops = &sun_uscsi_ops;
		return (SCGO_OPEN(scgp, device));
	}
#endif
	return (nopen);
}

LOCAL int
scgo_close(scgp)
	SCSI	*scgp;
{
	register int	i;

	if (scgp->local == NULL)
		return (-1);

	for (i = 0; i < MAX_SCG; i++) {
		if (scgfiles(scgp)[i] >= 0)
			close(scgfiles(scgp)[i]);
		scgfiles(scgp)[i] = -1;
	}
	return (0);
}

LOCAL long
scgo_maxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	long	maxdma = MAX_DMA_OTHER;
#if	!defined(__i386) && !defined(i386) && !defined(__amd64) && !defined(__x86_64)
	int	cpu_type;
#endif

#if	defined(__i386) || defined(i386) || defined(__amd64) || defined(__x86_64)
	maxdma = MAX_DMA_SUN386;
#else
	cpu_type = gethostid() >> 24;

	switch (cpu_type & ARCH_MASK) {

	case ARCH_SUN4C:
	case ARCH_SUN4E:
		maxdma = MAX_DMA_SUN4C;
		break;

	case ARCH_SUN4M:
	case ARCH_SUNX:
		maxdma = MAX_DMA_SUN4M;
		break;

	default:
		maxdma = MAX_DMA_SUN3;
	}
#endif

#ifndef	__SVR4
	/*
	 * SunOS 4.x allows esp hardware on VME boards and thus
	 * limits DMA on esp to 64k-1
	 */
	if (maxdma > MAX_DMA_SUN3)
		maxdma = MAX_DMA_SUN3;
#endif
	return (maxdma);
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
	if (scgp->local == NULL)
		return (FALSE);

	return (busno < 0 || busno >= MAX_SCG) ? FALSE : (scgfiles(scgp)[busno] >= 0);
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

	return ((busno < 0 || busno >= MAX_SCG) ? -1 : scgfiles(scgp)[busno]);
}

LOCAL int
scgo_initiator_id(scgp)
	SCSI	*scgp;
{
	int		id = -1;
#ifdef	DKIO_GETCINFO
	struct dk_cinfo	conf;
#endif

#ifdef	DKIO_GETCINFO
	if (scgp->fd < 0)
		return (id);
	if (ioctl(scgp->fd, DKIO_GETCINFO, &conf) < 0)
		return (id);
	if (TARGET(conf.dki_slave) != -1)
		id = TARGET(conf.dki_slave);
#endif
	return (id);
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
	return (ioctl(scgp->fd, SCGIORESET, 0));
}

LOCAL void *
scgo_getbuf(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	scgp->bufbase = (void *)valloc((size_t)amt);
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
scgo_send(scgp)
	SCSI	*scgp;
{
	scgp->scmd->target = scg_target(scgp);
	return (ioctl(scgp->fd, SCGIO_CMD, scgp->scmd));
}

/*--------------------------------------------------------------------------*/
/*
 *	This is Sun USCSI interface code ...
 */
#ifdef	USE_USCSI
#include <sys/scsi/impl/uscsi.h>

/*
 * Bit Mask definitions, for use accessing the status as a byte.
 */
#define	STATUS_MASK			0x3E
#define	STATUS_GOOD			0x00
#define	STATUS_CHECK			0x02

#define	STATUS_RESERVATION_CONFLICT	0x18
#define	STATUS_TERMINATED		0x22

#ifdef	nonono
#define	STATUS_MASK			0x3E
#define	STATUS_GOOD			0x00
#define	STATUS_CHECK			0x02

#define	STATUS_MET			0x04
#define	STATUS_BUSY			0x08
#define	STATUS_INTERMEDIATE		0x10
#define	STATUS_SCSI2			0x20
#define	STATUS_INTERMEDIATE_MET		0x14
#define	STATUS_RESERVATION_CONFLICT	0x18
#define	STATUS_TERMINATED		0x22
#define	STATUS_QFULL			0x28
#define	STATUS_ACA_ACTIVE		0x30
#endif

LOCAL int
scgo_uhelp(scgp, f)
	SCSI	*scgp;
	FILE	*f;
{
	__scg_help(f, "USCSI", "SCSI transport for targets known by Solaris drivers",
		"USCSI:", "bus,target,lun", "USCSI:1,2,0", TRUE, TRUE);
	return (0);
}

LOCAL int
scgo_uopen(scgp, device)
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
	char		devname[32];

	if (have_volmgt < 0)
		have_volmgt = volmgt_running();

	if (scgp->overbose) {
		js_fprintf((FILE *)scgp->errfile,
				"Warning: Using USCSI interface.\n");
	}
#ifndef	SEEK_HOLE
	/*
	 * SEEK_HOLE first appears in Solaris 11 Build 14, volmgt supports
	 * medialess drives since Build 21. Using SEEK_HOLD as indicator
	 * seems to be the best guess.
	 */
	if (scgp->overbose > 0 && have_volmgt) {
		js_fprintf((FILE *)scgp->errfile,
		"Warning: Volume management is running, medialess managed drives are invisible.\n");
	}
#endif

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
		if (scgp->local == NULL) {
			if (scgp->errstr)
				js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE, "No memory for scg_local");
			return (0);
		}

		for (b = 0; b < MAX_SCG; b++) {
			for (t = 0; t < MAX_TGT; t++) {
				for (l = 0; l < MAX_LUN; l++)
					scglocal(scgp)->u.scg_files[b][t][l] = (short)-1;
			}
		}
	}

	if (device != NULL && strcmp(device, "USCSI") == 0)
		goto uscsiscan;

	if ((device != NULL && *device != '\0') || (busno == -2 && tgt == -2))
		goto openbydev;

uscsiscan:
	if (busno >= 0 && tgt >= 0 && tlun >= 0) {

		if (busno >= MAX_SCG || tgt >= MAX_TGT || tlun >= MAX_LUN)
			return (-1);

		js_snprintf(devname, sizeof (devname), "/dev/rdsk/c%dt%dd%ds2",
			busno, tgt, tlun);
		f = open(devname, O_RDONLY | O_NDELAY);
		if (f < 0 && geterrno() == EBUSY)
			f = scgo_volopen(scgp, devname);
		if (f < 0) {
			js_snprintf(scgp->errstr,
				    SCSI_ERRSTR_SIZE,
				"Cannot open '%s'", devname);
			return (0);
		}
		scglocal(scgp)->u.scg_files[busno][tgt][tlun] = f;
		return (1);
	} else {
		int	errsav = 0;

		for (b = 0; b < MAX_SCG; b++) {
			for (t = 0; t < MAX_TGT; t++) {
				for (l = 0; l < MAX_LUN; l++) {
					js_snprintf(devname, sizeof (devname),
						"/dev/rdsk/c%dt%dd%ds2",
						b, t, l);
					f = open(devname, O_RDONLY | O_NDELAY);
					if (f < 0 && geterrno() == EBUSY) {
						f = scgo_volopen(scgp, devname);
						/*
						 * Hack to mark inaccessible
						 * drives with fd == -2
						 */
						if (f < 0 &&
						    scglocal(scgp)->u.scg_files[b][t][l] < 0)
							scglocal(scgp)->u.scg_files[b][t][l] = f;
					}
					if (f < 0 && errno != ENOENT &&
						    errno != ENXIO &&
						    errno != ENODEV) {
						errsav = geterrno();
						if (scgp->errstr)
							js_snprintf(scgp->errstr,
							    SCSI_ERRSTR_SIZE,
							    "Cannot open '%s'", devname);
					}
					if (f < 0 && l == 0)
						break;
					if (f >= 0) {
						nopen ++;
						if (scglocal(scgp)->u.scg_files[b][t][l] == -1)
							scglocal(scgp)->u.scg_files[b][t][l] = f;
						else
							close(f);
					}
				}
			}
		}
		seterrno(errsav);
	}
openbydev:
	if (nopen == 0) {
		int	target;
		int	lun;

		if (device != NULL && strncmp(device, "USCSI:", 6) == 0)
			device += 6;
		if (device == NULL || device[0] == '\0')
			return (0);

		f = open(device, O_RDONLY | O_NDELAY);
		if (f < 0)
			f = scgo_volopen(scgp, device);
		if (f < 0) {
			js_snprintf(scgp->errstr,
				    SCSI_ERRSTR_SIZE,
				"Cannot open '%s'", device);
			return (0);
		}

		if (busno < 0)
			busno = 0;	/* Use Fake number if not specified */

		if (scgo_ugettlun(f, &target, &lun) >= 0) {
			if (tgt >= 0 && tlun >= 0) {
				if (tgt != target || tlun != lun) {
					close(f);
					return (0);
				}
			}
			tgt = target;
			tlun = lun;
		} else {
			if (tgt < 0 || tlun < 0) {
				close(f);
				return (0);
			}
		}

		if (scglocal(scgp)->u.scg_files[busno][tgt][tlun] == -1)
			scglocal(scgp)->u.scg_files[busno][tgt][tlun] = f;
		scg_settarget(scgp, busno, tgt, tlun);

		return (++nopen);
	}
	return (nopen);
}

LOCAL int
scgo_volopen(scgp, devname)
	SCSI	*scgp;
	char	*devname;
{
	int	oerr = geterrno();
	int	f = -1;
	char	*name   = NULL;	/* Volume symbolic device name		*/
	char	*symdev = NULL;	/* /dev/... name based on "name" 	*/
	char	*mname  = NULL;	/* Volume media name based on "name"	*/

	if (!have_volmgt)
		return (-1);

#ifdef	VOLMGT_DEBUG
	scgp->debug++;
#endif
	if (scgp->debug > 0) {
		js_fprintf((FILE *)scgp->errfile,
			"scgo_volopen(%s)\n", devname);
	}

	/*
	 * We come here because trying to open "devname" did not work.
	 * First try to translate between a symbolic name and a /dev/...
	 * based device name. Then translate back to a symbolic name.
	 */
	symdev = volmgt_symdev(devname);
	if (symdev)
		name = volmgt_symname(symdev);
	if (scgp->debug > 0) {
		js_fprintf((FILE *)scgp->errfile,
			"volmgt_symdev(%s)=%s -> %s\n", devname, symdev, name);
	}

	/*
	 * If "devname" is not a symbolic device name, then it must be
	 * a /dev/... based device name. Try to translate it into a
	 * symbolic name. Then translate back to a /dev/... name.
	 */
	if (name == NULL) {
		name = volmgt_symname(devname);
		if (name)
			symdev = volmgt_symdev(name);
	}
	if (scgp->debug > 0) {
		js_fprintf((FILE *)scgp->errfile,
			"volmgt_symdev(%s)=%s -> %s\n", devname, symdev, name);
	}

	/*
	 * If we have been able to translate to a symbolic device name,
	 * translate this name into a volume management media name that
	 * may be used for opening.
	 */
	if (name)
		mname = media_findname(name);
	if (scgp->debug > 0) {
		js_fprintf((FILE *)scgp->errfile,
			"symdev %s name %s mname %s\n", symdev, name, mname);
	}

	/*
	 * Das scheint nur mit dem normierten /dev/rdsk/ *s2 Namen zu gehen.
	 */
	if (scgp->debug > 0) {
		js_fprintf((FILE *)scgp->errfile,
			"volmgt_inuse(%s) %d\n", symdev, volmgt_inuse(symdev));
	}
	if (mname)
		f = scgo_openmedia(scgp, mname);
	else if (name)
		f = -2;	/* Mark inaccessible drives with fd == -2 */

	/*
	 * Besonderen Fehlertext oder fprintf/errfile bei non-scanbus Open und
	 * wenn errrno == EBUSY && kein Mapping?
	 */
	if (name)
		free(name);
	if (symdev)
		free(symdev);
	if (mname)
		free(mname);
	seterrno(oerr);
#ifdef	VOLMGT_DEBUG
	scgp->debug--;
#endif
	return (f);
}

LOCAL int
scgo_openmedia(scgp, mname)
	SCSI	*scgp;
	char	*mname;
{
	int	f = -1;
	char	*device = NULL;
	struct	stat sb;

	if (mname == NULL)
		return (-1);

	/*
	 * Check whether the media name refers to a directory.
	 * In this case, the medium is partitioned and we need to
	 * check all partitions.
	 */
	if (stat(mname, &sb) >= 0) {
		if (S_ISDIR(sb.st_mode)) {
			char    name[128];
			int	i;

			/*
			 * First check partition '2', the whole disk.
			 */
			js_snprintf(name, sizeof (name), "%s/s2", mname);
			f = open(name, O_RDONLY | O_NDELAY);
			if (f >= 0)
				return (f);
			/*
			 * Now try all other partitions.
			 */
			for (i = 0; i < 16; i++) {
				if (i == 2)
					continue;
				js_snprintf(name, sizeof (name),
							"%s/s%d", mname, i);
				if (stat(name, &sb) >= 0)
					break;
			}
			if (i < 16) {
				device = mname;
			}
		} else {
			device = mname;
		}
	}
	if (device)
		f = open(device, O_RDONLY | O_NDELAY);
	return (f);
}

LOCAL int
scgo_uclose(scgp)
	SCSI	*scgp;
{
	register int	f;
	register int	b;
	register int	t;
	register int	l;

	if (scgp->local == NULL)
		return (-1);

	for (b = 0; b < MAX_SCG; b++) {
		for (t = 0; t < MAX_TGT; t++) {
			for (l = 0; l < MAX_LUN; l++) {
				f = scglocal(scgp)->u.scg_files[b][t][l];
				if (f >= 0)
					close(f);
				scglocal(scgp)->u.scg_files[b][t][l] = (short)-1;
			}
		}
	}
	return (0);
}

LOCAL int
scgo_ucinfo(f, cp, debug)
	int		f;
	struct dk_cinfo *cp;
	int		debug;
{
	fillbytes(cp, sizeof (*cp), '\0');

	if (ioctl(f, DKIOCINFO, cp) < 0)
		return (-1);

	if (debug <= 0)
		return (0);

	js_fprintf(stderr, "cname:		'%s'\n", cp->dki_cname);
	js_fprintf(stderr, "ctype:		0x%04hX %hd\n", cp->dki_ctype, cp->dki_ctype);
	js_fprintf(stderr, "cflags:		0x%04hX\n", cp->dki_flags);
	js_fprintf(stderr, "cnum:		%hd\n", cp->dki_cnum);
#ifdef	__EVER__
	js_fprintf(stderr, "addr:		%d\n", cp->dki_addr);
	js_fprintf(stderr, "space:		%d\n", cp->dki_space);
	js_fprintf(stderr, "prio:		%d\n", cp->dki_prio);
	js_fprintf(stderr, "vec:		%d\n", cp->dki_vec);
#endif
	js_fprintf(stderr, "dname:		'%s'\n", cp->dki_dname);
	js_fprintf(stderr, "unit:		%d\n", cp->dki_unit);
	js_fprintf(stderr, "slave:		%d %04o Tgt: %d Lun: %d\n",
				cp->dki_slave, cp->dki_slave,
				TARGET(cp->dki_slave), LUN(cp->dki_slave));
	js_fprintf(stderr, "partition:	%hd\n", cp->dki_partition);
	js_fprintf(stderr, "maxtransfer:	%d (%d)\n",
				cp->dki_maxtransfer,
				cp->dki_maxtransfer * DEV_BSIZE);
	return (0);
}

LOCAL int
scgo_ugettlun(f, tgtp, lunp)
	int	f;
	int	*tgtp;
	int	*lunp;
{
	struct dk_cinfo ci;

	if (scgo_ucinfo(f, &ci, 0) < 0)
		return (-1);
	if (tgtp)
		*tgtp = TARGET(ci.dki_slave);
	if (lunp)
		*lunp = LUN(ci.dki_slave);
	return (0);
}

LOCAL long
scgo_umaxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	register int	b;
	register int	t;
	register int	l;
	long		maxdma = -1L;
	int		f;
	struct dk_cinfo ci;
	BOOL		found_ide = FALSE;

	if (scgp->local == NULL)
		return (-1L);

	for (b = 0; b < MAX_SCG; b++) {
		for (t = 0; t < MAX_TGT; t++) {
			for (l = 0; l < MAX_LUN; l++) {
				if ((f = scglocal(scgp)->u.scg_files[b][t][l]) < 0)
					continue;
				if (scgo_ucinfo(f, &ci, scgp->debug) < 0)
					continue;
				if (maxdma < 0)
					maxdma = (long)(ci.dki_maxtransfer * DEV_BSIZE);
				if (maxdma > (long)(ci.dki_maxtransfer * DEV_BSIZE))
					maxdma = (long)(ci.dki_maxtransfer * DEV_BSIZE);
				if (streql(ci.dki_cname, "ide"))
					found_ide = TRUE;
			}
		}
	}

#if	defined(__i386) || defined(i386) || defined(__amd64) || defined(__x86_64)
	/*
	 * At least on Solaris 9 x86, DKIOCINFO returns a wrong value
	 * for dki_maxtransfer if the target is an ATAPI drive.
	 * Without DMA, it seems to work if we use 256 kB DMA size for ATAPI,
	 * but if we allow DMA, only 68 kB will work (for more we get a silent
	 * DMA residual count == DMA transfer count).
	 * For this reason, we try to figure out the correct value for 'ide'
	 * by retrieving the (correct) value from a ide hard disk.
	 */
	if (found_ide) {
		if ((f = scgo_openide()) >= 0) {
#ifdef	sould_we
			long omaxdma = maxdma;
#endif

			if (scgo_ucinfo(f, &ci, scgp->debug) >= 0) {
				if (maxdma < 0)
					maxdma = (long)(ci.dki_maxtransfer * DEV_BSIZE);
				if (maxdma > (long)(ci.dki_maxtransfer * DEV_BSIZE))
					maxdma = (long)(ci.dki_maxtransfer * DEV_BSIZE);
			}
			close(f);
#ifdef	sould_we
			/*
			 * The kernel returns 56 kB but we tested that 68 kB works.
			 */
			if (omaxdma > maxdma && maxdma == (112 * DEV_BSIZE))
				maxdma = 136 * DEV_BSIZE;
#endif
		} else {
			/*
			 * No IDE disk on this system?
			 */
			if (maxdma == (512 * DEV_BSIZE))
				maxdma = MAX_DMA_SUN386;
		}
	}
#endif
	/*
	 * The Sun tape driver does not support to retrieve the max DMA count.
	 * Use the knwoledge about default DMA sizes in this case.
	 */
	if (maxdma < 0)
		maxdma = scgo_maxdma(scgp, amt);

	return (maxdma);
}

#if	defined(__i386) || defined(i386) || defined(__amd64) || defined(__x86_64)
LOCAL int
scgo_openide()
{
	char	buf[20];
	int	b;
	int	t;
	int	f = -1;

	for (b = 0; b < 5; b++) {
		for (t = 0; t < 2; t++) {
			js_snprintf(buf, sizeof (buf),
				"/dev/rdsk/c%dd%dp0", b, t);
			if ((f = open(buf, O_RDONLY | O_NDELAY)) >= 0)
				goto out;
		}
	}
out:
	return (f);
}
#endif

LOCAL int
scgo_unumbus(scgp)
	SCSI	*scgp;
{
	return (MAX_SCG);
}

LOCAL BOOL
scgo_uhavebus(scgp, busno)
	SCSI	*scgp;
	int	busno;
{
	register int	t;
	register int	l;

	if (scgp->local == NULL || busno < 0 || busno >= MAX_SCG)
		return (FALSE);

	for (t = 0; t < MAX_TGT; t++) {
		for (l = 0; l < MAX_LUN; l++)
			if (scglocal(scgp)->u.scg_files[busno][t][l] >= 0)
				return (TRUE);
	}
	return (FALSE);
}

LOCAL int
scgo_ufileno(scgp, busno, tgt, tlun)
	SCSI	*scgp;
	int	busno;
	int	tgt;
	int	tlun;
{
	if (scgp->local == NULL ||
	    busno < 0 || busno >= MAX_SCG ||
	    tgt < 0 || tgt >= MAX_TGT ||
	    tlun < 0 || tlun >= MAX_LUN)
		return (-1);

	return ((int)scglocal(scgp)->u.scg_files[busno][tgt][tlun]);
}

LOCAL int
scgo_uinitiator_id(scgp)
	SCSI	*scgp;
{
	return (-1);
}

LOCAL int
scgo_uisatapi(scgp)
	SCSI	*scgp;
{
	char		devname[32];
	char		symlinkname[MAXPATHLEN];
	int		len;
	struct dk_cinfo ci;

	if (ioctl(scgp->fd, DKIOCINFO, &ci) < 0)
		return (-1);

	js_snprintf(devname, sizeof (devname), "/dev/rdsk/c%dt%dd%ds2",
		scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp));

	symlinkname[0] = '\0';
	len = readlink(devname, symlinkname, sizeof (symlinkname));
	if (len > 0)
		symlinkname[len] = '\0';

	if (len >= 0 && strstr(symlinkname, "ide") != NULL)
		return (TRUE);
	else
		return (FALSE);
}

LOCAL int
scgo_ureset(scgp, what)
	SCSI	*scgp;
	int	what;
{
	struct uscsi_cmd req;

	if (what == SCG_RESET_NOP)
		return (0);

	fillbytes(&req, sizeof (req), '\0');

	if (what == SCG_RESET_TGT) {
		req.uscsi_flags = USCSI_RESET | USCSI_SILENT;	/* reset target */
	} else if (what != SCG_RESET_BUS) {
		req.uscsi_flags = USCSI_RESET_ALL | USCSI_SILENT; /* reset bus */
	} else {
		errno = EINVAL;
		return (-1);
	}

	return (ioctl(scgp->fd, USCSICMD, &req));
}

LOCAL int
scgo_usend(scgp)
	SCSI		*scgp;
{
	struct scg_cmd	*sp = scgp->scmd;
	struct uscsi_cmd req;
	int		ret;
static	uid_t		cureuid = 0;	/* XXX Hack until we have uid management */

	if (scgp->fd < 0) {
		sp->error = SCG_FATAL;
		return (0);
	}

	fillbytes(&req, sizeof (req), '\0');

	req.uscsi_flags = USCSI_SILENT | USCSI_DIAGNOSE | USCSI_RQENABLE;

	if (sp->flags & SCG_RECV_DATA) {
		req.uscsi_flags |= USCSI_READ;
	} else if (sp->size > 0) {
		req.uscsi_flags |= USCSI_WRITE;
	}
	req.uscsi_buflen	= sp->size;
	req.uscsi_bufaddr	= sp->addr;
	req.uscsi_timeout	= sp->timeout;
	req.uscsi_cdblen	= sp->cdb_len;
	req.uscsi_rqbuf		= (caddr_t) sp->u_sense.cmd_sense;
	req.uscsi_rqlen		= sp->sense_len;
	req.uscsi_cdb		= (caddr_t) &sp->cdb;

	if (cureuid != 0)
		seteuid(0);
again:
	errno = 0;
	ret = ioctl(scgp->fd, USCSICMD, &req);

	if (ret < 0 && geterrno() == EPERM) {	/* XXX Hack until we have uid management */
		cureuid = geteuid();
		if (seteuid(0) >= 0)
			goto again;
	}
	if (cureuid != 0)
		seteuid(cureuid);

	if (scgp->debug > 0) {
		js_fprintf((FILE *)scgp->errfile, "ret: %d errno: %d (%s)\n", ret, errno, errmsgstr(errno));
		js_fprintf((FILE *)scgp->errfile, "uscsi_flags:     0x%x\n", req.uscsi_flags);
		js_fprintf((FILE *)scgp->errfile, "uscsi_status:    0x%x\n", req.uscsi_status);
		js_fprintf((FILE *)scgp->errfile, "uscsi_timeout:   %d\n", req.uscsi_timeout);
		js_fprintf((FILE *)scgp->errfile, "uscsi_bufaddr:   0x%lx\n", (long)req.uscsi_bufaddr);
								/*
								 * Cast auf int OK solange sp->size
								 * auch ein int bleibt.
								 */
		js_fprintf((FILE *)scgp->errfile, "uscsi_buflen:    %d\n", (int)req.uscsi_buflen);
		js_fprintf((FILE *)scgp->errfile, "uscsi_resid:     %d\n", (int)req.uscsi_resid);
		js_fprintf((FILE *)scgp->errfile, "uscsi_rqlen:     %d\n", req.uscsi_rqlen);
		js_fprintf((FILE *)scgp->errfile, "uscsi_rqstatus:  0x%x\n", req.uscsi_rqstatus);
		js_fprintf((FILE *)scgp->errfile, "uscsi_rqresid:   %d\n", req.uscsi_rqresid);
		js_fprintf((FILE *)scgp->errfile, "uscsi_rqbuf ptr: 0x%lx\n", (long)req.uscsi_rqbuf);
		js_fprintf((FILE *)scgp->errfile, "uscsi_rqbuf:     ");
		if (req.uscsi_rqbuf != NULL && req.uscsi_rqlen > req.uscsi_rqresid) {
			int	i;
			int	len = req.uscsi_rqlen - req.uscsi_rqresid;

			for (i = 0; i < len; i++) {
				js_fprintf((FILE *)scgp->errfile, "0x%02X ", ((char *)req.uscsi_rqbuf)[i]);
			}
			js_fprintf((FILE *)scgp->errfile, "\n");
		} else {
			js_fprintf((FILE *)scgp->errfile, "<data not available>\n");
		}
	}
	if (ret < 0) {
		sp->ux_errno = geterrno();
		/*
		 * Check if SCSI command cound not be send at all.
		 */
		if (sp->ux_errno == ENOTTY && scgo_uisatapi(scgp) == TRUE) {
			if (scgp->debug > 0) {
				js_fprintf((FILE *)scgp->errfile,
					"ENOTTY atapi: %d\n", scgo_uisatapi(scgp));
			}
			sp->error = SCG_FATAL;
			return (0);
		}
		if (errno == ENXIO) {
			sp->error = SCG_FATAL;
			return (0);
		}
		if (errno == ENOTTY || errno == EINVAL || errno == EACCES || errno == EPERM) {
			return (-1);
		}
	} else {
		sp->ux_errno = 0;
	}
	ret			= 0;
	sp->sense_count		= req.uscsi_rqlen - req.uscsi_rqresid;
	sp->resid		= req.uscsi_resid;
	sp->u_scb.cmd_scb[0]	= req.uscsi_status;

	if ((req.uscsi_status & STATUS_MASK) == STATUS_GOOD) {
		sp->error = SCG_NO_ERROR;
		return (0);
	}
	if (req.uscsi_rqstatus == 0 &&
	    ((req.uscsi_status & STATUS_MASK) == STATUS_CHECK)) {
		sp->error = SCG_NO_ERROR;
		return (0);
	}
	if (req.uscsi_status & (STATUS_TERMINATED |
	    STATUS_RESERVATION_CONFLICT)) {
		sp->error = SCG_FATAL;
	}
	if (req.uscsi_status != 0) {
		/*
		 * This is most likely wrong. There seems to be no way
		 * to produce SCG_RETRYABLE with USCSI.
		 */
		sp->error = SCG_RETRYABLE;
	}

	return (ret);
}
#endif	/* USE_USCSI */
