/* @(#)interface.c	1.83 16/02/14 Copyright 1998-2002,2015 Heiko Eissfeldt, Copyright 2006-2016 J. Schilling */
#include "config.h"
#ifndef lint
static	UConst char sccsid[] =
"@(#)interface.c	1.83 16/02/14 Copyright 1998-2002,2015 Heiko Eissfeldt, Copyright 2006-2016 J. Schilling";

#endif
/*
 * Copyright (C) 1998-2002,2015 Heiko Eissfeldt heiko@colossus.escape.de
 * Copyright (c) 2006-2016      J. Schilling
 *
 * Interface module for cdrom drive access
 *
 * Two interfaces are possible.
 *
 * 1. using 'cooked' ioctls() (Linux only)
 *    : available for atapi, sbpcd and cdu31a drives only.
 *
 * 2. using the generic scsi device (for details see SCSI Prog. HOWTO).
 *    NOTE: a bug/misfeature in the kernel requires blocking signal
 *          SIGINT during SCSI command handling. Once this flaw has
 *          been removed, the sigprocmask SIG_BLOCK and SIG_UNBLOCK calls
 *          should removed, thus saving context switches.
 *
 * For testing purposes I have added a third simulation interface.
 *
 * Version 0.8: used experiences of Jochen Karrer.
 *              SparcLinux port fixes
 *              AlphaLinux port fixes
 *
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
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */
#if 0
#define	SIM_CD
#endif

#include "config.h"
#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/errno.h>
#include <schily/signal.h>
#include <schily/fcntl.h>
#include <schily/assert.h>
#include <schily/nlsdefs.h>
#include <schily/device.h>
#include <schily/ioctl.h>
#include <schily/stat.h>
#include <schily/time.h>
#include <schily/poll.h>
#include <schily/schily.h>


#include "mycdrom.h"
#include "lowlevel.h"
/* some include file locations have changed with newer kernels */
#if defined(__linux__)
# if LINUX_VERSION_CODE > 0x10300 + 97
#  if LINUX_VERSION_CODE < 0x200ff
#   include <linux/sbpcd.h>
#   include <linux/ucdrom.h>
#  endif
#  if !defined(CDROM_SELECT_SPEED)
#   include <linux/ucdrom.h>
#  endif
# endif
#endif

#include <scg/scsireg.h>
#include <scg/scsitransp.h>

#include "mytype.h"
#include "byteorder.h"
#include "interface.h"
#include "cdda2wav.h"
#include "semshm.h"
#include "setuid.h"
#include "ringbuff.h"
#include "toc.h"
#include "global.h"
#include "ioctl.h"
#include "exitcodes.h"
#include "scsi_cmds.h"

#include <schily/utypes.h>
#include <cdrecord.h>
#include "scsi_scan.h"

unsigned interface;

int trackindex_disp = 0;

void	(*EnableCdda)	__PR((SCSI *, int Switch, unsigned uSectorsize));
unsigned (*doReadToc)	__PR((SCSI *scgp));
void	(*ReadTocText)	__PR((SCSI *scgp));
unsigned (*ReadLastAudio) __PR((SCSI *scgp));
int	(*ReadCdRom)	__PR((SCSI *scgp, UINT4 *p, unsigned lSector,
						unsigned SectorBurstVal));
int	(*ReadCdRom_C2)	__PR((SCSI *scgp, UINT4 *p, unsigned lSector,
						unsigned SectorBurstVal));
int	(*ReadCdRomData) __PR((SCSI *scgp, unsigned char *p, unsigned lSector,
						unsigned SectorBurstVal));
int	(*ReadCdRomSub)	__PR((SCSI *scgp, UINT4 *p, unsigned lSector,
						unsigned SectorBurstVal));
subq_chnl *(*ReadSubChannels) __PR((SCSI *scgp, unsigned lSector));
subq_chnl *(*ReadSubQ)	__PR((SCSI *scgp, unsigned char sq_format,
						unsigned char track));
void	(*SelectSpeed)	__PR((SCSI *scgp, unsigned speed));
int	(*Play_at)	__PR((SCSI *scgp, unsigned int from_sector,
						unsigned int sectors));
int	(*StopPlay)	__PR((SCSI *scgp));
void	(*trash_cache)	__PR((UINT4 *p, unsigned lSector,
						unsigned SectorBurstVal));

#if	defined USE_PARANOIA
long	cdda_read	__PR((void *d, void * buffer, long beginsector,
						long sectors));
long	cdda_read_c2	__PR((void *d, void * buffer, long beginsector,
						long sectors));

long
cdda_read(d, buffer, beginsector, sectors)
	void	*d;
	void	*buffer;
	long	beginsector;
	long	sectors;
{
	long	ret = ReadCdRom(d, buffer, beginsector, sectors);
	return (ret);
}

long
cdda_read_c2(d, buffer, beginsector, sectors)
	void	*d;
	void	*buffer;
	long	beginsector;
	long	sectors;
{
	long	ret = ReadCdRom_C2(d, buffer, beginsector, sectors);
	return (ret);
}
#endif

typedef struct string_len {
	char		*str;
	unsigned int	sl;
} mystring;

static mystring drv_is_not_mmc[] = {
	{ "DEC     RRD47   (C) DEC ", 24},
/*	{ "SONY    CD-ROM CDU625    1.0", 28}, */
	{ NULL, 0}	/* must be last entry */
};

static mystring drv_has_mmc_cdda[] = {
	{ "HITACHI CDR-7930", 16 },
/*	{ "TOSHIBA CD-ROM XM-5402TA3605", 28}, */
	{ NULL, 0}	/* must be last entry */
};

static int	Is_a_Toshiba3401;

int Toshiba3401 __PR((void));

int
Toshiba3401() {
	return (Is_a_Toshiba3401);
}

/* hook */
static void	Dummy	__PR((void));
static void
Dummy()
{
}

static SCSI	*_scgp;

SCSI	*get_scsi_p	__PR((void));

SCSI *
get_scsi_p()
{
	return (_scgp);
}

#if !defined(SIM_CD)

static void	trash_cache_SCSI	__PR((UINT4 *p, unsigned lSector,
						unsigned SectorBurstVal));

static void
trash_cache_SCSI(p, lSector, SectorBurstVal)
	UINT4		*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
	/*
	 * trash the cache
	 */
	ReadCdRom(get_scsi_p(), p,
			find_an_off_sector(lSector, SectorBurstVal),
			min(global.nsectors, 6));
}



static void	Check_interface_for_device __PR((struct stat *statstruct,
						char *pdev_name));
LOCAL	int	OpenCdRom	__PR((char *pdev_name));
LOCAL	void	scg_openerr	__PR((char *errstr));
LOCAL	int	find_drive	__PR((SCSI *scgp, char *dev));

static void	SetupSCSI	__PR((SCSI *scgp));

static void
SetupSCSI(scgp)
	SCSI	*scgp;
{
	unsigned char	*p;
	int		err;

	if (interface != GENERIC_SCSI) {
		/*
		 * unfortunately we have the wrong interface and are
		 * not able to change on the fly
		 */
		errmsgno(EX_BAD,
		_("The generic SCSI interface and devices are required\n"));
		exit(SYNTAX_ERROR);
	}

	/*
	 * do a test unit ready to 'init' the device.
	 */
	seterrno(0);
	TestForMedium(scgp);
	err = geterrno();
	if (err == EPERM || err == EACCES) {
		scg_openerr("");
		/* NOTREACHED */
	}

	/*
	 * check for the correct type of unit.
	 */
	p = ScsiInquiry(scgp);

#undef	TYPE_ROM
#define	TYPE_ROM	5
#undef	TYPE_WORM
#define	TYPE_WORM 	4
	if (p == NULL) {
		errmsgno(EX_BAD, _("Inquiry command failed. Aborting...\n"));
		exit(DEVICE_ERROR);
	}

	if ((*p != TYPE_ROM && *p != TYPE_WORM)) {
		errmsgno(EX_BAD,
		_("This is neither a scsi cdrom nor a worm device.\n"));
		exit(SYNTAX_ERROR);
	}

	if (global.quiet == 0) {
		fprintf(outfp,
		_("Type: %s, Vendor '%8.8s' Model '%16.16s' Revision '%4.4s' "),
			*p == TYPE_ROM ? "ROM" : "WORM",
			p+8,
			p+16,
			p+32);
	}
	/*
	 * generic Sony type defaults
	 */
	density = 0x0;
	accepts_fua_bit = -1;
	EnableCdda = (void (*) __PR((SCSI *, int, unsigned)))Dummy;
	ReadCdRom = ReadCdda12;
	ReadCdRomSub = ReadCddaSubSony;
	ReadCdRom_C2 = ReadCddaNoFallback_C2;
	ReadCdRomData = (int (*) __PR((SCSI *,
					unsigned char *,
					unsigned, unsigned))) ReadStandardData;
	ReadLastAudio = ReadFirstSessionTOCSony;
	SelectSpeed = SpeedSelectSCSISony;
	Play_at = Play_atSCSI;
	StopPlay = StopPlaySCSI;
	trash_cache = trash_cache_SCSI;
	ReadTocText = ReadTocTextSCSIMMC;
	doReadToc = ReadTocSCSI;
	ReadSubQ = ReadSubQSCSI;
	ReadSubChannels = (subq_chnl * (*) __PR((SCSI *, unsigned)))NULL;

	/*
	 * check for brands and adjust special peculiaritites
	 */

	/*
	 * If your drive is not treated correctly, you can adjust some things
	 * here:
	 * global.in_lendian: should be to 1, if the CDROM drive or CD-Writer
	 *	  delivers the samples in the native byteorder of the audio cd
	 *	  (LSB first).
	 *	  HP CD-Writers need it set to 0.
	 * NOTE: If you get correct wav files when using sox with the '-x'
	 *   option, the endianess is wrong. You can use the -C option to
	 *   specify the value of global.in_lendian.
	 */

	{
		int	mmc_code;

		scgp->silent++;
		allow_atapi(scgp, 1);
		if (*p == TYPE_ROM) {
			mmc_code = heiko_mmc(scgp);
		} else {
			mmc_code = 0;
		}
		scgp->silent--;

		/*
		 * Exceptions for drives that report incorrect MMC capability
		 */
		if (mmc_code != 0) {
			/*
			 * these drives are NOT capable of MMC commands
			 */
			mystring *pp = drv_is_not_mmc;
			while (pp->str != NULL) {
				if (strncmp(pp->str, (char *)p+8, pp->sl) == 0) {
					mmc_code = 0;
					break;
				}
				pp++;
			}
		}
		{
			/*
			 * these drives flag themselves as non-MMC, but offer
			 * CDDA reading only with a MMC method.
			 */
			mystring *pp = drv_has_mmc_cdda;
			while (pp->str != NULL) {
				if (strncmp(pp->str, (char *)p+8, pp->sl) == 0) {
					mmc_code = 1;
					break;
				}
				pp++;
			}
		}

		switch (mmc_code) {
		case 2:	/* SCSI-3 cdrom drive with accurate audio stream */
			/* FALLTHROUGH */
		case 1:	/* SCSI-3 cdrom drive with no accurate audio stream */
			/* FALLTHROUGH */
lost_toshibas:
			global.in_lendian = 1;
			if (mmc_code == 2)
				global.overlap = 0;
			else
				global.overlap = 1;
			ReadCdRom = ReadCddaFallbackMMC;
			ReadCdRom_C2 = ReadCddaFallbackMMC_C2;
			ReadCdRomSub = ReadCddaSubSony;
			ReadLastAudio = ReadFirstSessionTOCMMC;
			SelectSpeed = SpeedSelectSCSIMMC;
			ReadTocText = ReadTocTextSCSIMMC;
			doReadToc = ReadTocMMC;
			ReadSubChannels = ReadSubChannelsFallbackMMC;
			if (!memcmp(p+8, "SONY    CD-RW  CRX100E  1.0", 27))
				ReadTocText = (void (*) __PR((SCSI *)))NULL;
			if (!global.quiet)
				fprintf(outfp, "MMC+CDDA\n");
			break;
		case -1: /* "MMC drive does not support cdda reading, sorry\n." */
			doReadToc = ReadTocMMC;
			if (!global.quiet)
				fprintf(outfp, "MMC-CDDA\n");
			/* FALLTHROUGH */
		case 0:	/* non SCSI-3 cdrom drive */
			if (!global.quiet) fprintf(outfp, _("no MMC\n"));
				ReadLastAudio = (unsigned (*) __PR((SCSI *)))NULL;
			if (!memcmp(p+8, "TOSHIBA", 7) ||
			    !memcmp(p+8, "IBM", 3) ||
			    !memcmp(p+8, "DEC", 3)) {
				/*
				 * Older Toshiba ATAPI drives don't identify
				 * themselves as MMC.
				 * The last digit of the model number is
				 * '2' for ATAPI drives.
				 * These are treated as MMC.
				 */
				if (!memcmp(p+15, " CD-ROM XM-", 11) &&
				    p[29] == '2') {
					goto lost_toshibas;
				}
				density = 0x82;
				EnableCdda = EnableCddaModeSelect;
				ReadSubChannels = ReadStandardSub;
				ReadCdRom = ReadStandard;
				SelectSpeed = SpeedSelectSCSIToshiba;
				if (!memcmp(p+15, " CD-ROM XM-3401", 15)) {
					Is_a_Toshiba3401 = 1;
				}
				global.in_lendian = 1;
			} else if (!memcmp(p+8, "IMS", 3) ||
				    !memcmp(p+8, "KODAK", 5) ||
				    !memcmp(p+8, "RICOH", 5) ||
				    !memcmp(p+8, "HP", 2) ||
				    !memcmp(p+8, "PHILIPS", 7) ||
				    !memcmp(p+8, "PLASMON", 7) ||
				    !memcmp(p+8, "GRUNDIG CDR100IPW", 17) ||
				    !memcmp(p+8, "MITSUMI CD-R ", 13)) {
				EnableCdda = EnableCddaModeSelect;
				ReadCdRom = ReadStandard;
				SelectSpeed = SpeedSelectSCSIPhilipsCDD2600;

				/*
				 * treat all of these as bigendian
				 */
				global.in_lendian = 0;

				/*
				 * no overlap reading for cd-writers
				 */
				global.overlap = 0;
			} else if (!memcmp(p+8, "NRC", 3)) {
				SelectSpeed = (void (*) __PR((SCSI *, unsigned)))NULL;
			} else if (!memcmp(p+8, "YAMAHA", 6)) {
				EnableCdda = EnableCddaModeSelect;
				SelectSpeed = SpeedSelectSCSIYamaha;

				/*
				 * no overlap reading for cd-writers
				 */
				global.overlap = 0;
				global.in_lendian = 1;
			} else if (!memcmp(p+8, "PLEXTOR", 7)) {
				global.in_lendian = 1;
				global.overlap = 0;
				ReadLastAudio = ReadFirstSessionTOCSony;
				ReadTocText = ReadTocTextSCSIMMC;
				doReadToc = ReadTocSony;
				ReadSubChannels = ReadSubChannelsSony;
			} else if (!memcmp(p+8, "SONY", 4)) {
				global.in_lendian = 1;
				if (!memcmp(p+16, "CD-ROM CDU55E", 13)) {
					ReadCdRom = ReadCddaMMC12;
				}
				ReadLastAudio = ReadFirstSessionTOCSony;
				ReadTocText = ReadTocTextSCSIMMC;
				doReadToc = ReadTocSony;
				ReadSubChannels = ReadSubChannelsSony;
			} else if (!memcmp(p+8, "NEC", 3)) {
				ReadCdRom = ReadCdda10;
				ReadTocText = (void (*) __PR((SCSI *)))NULL;
				SelectSpeed = SpeedSelectSCSINEC;
				global.in_lendian = 1;
				/*
				 * I assume all versions of the 502 require
				 * this?
				 * no overlap reading for NEC CD-ROM 502!
				 */
				if (!memcmp(p+29, "5022.0r", 3))
					global.overlap = 0;
			} else if (!memcmp(p+8, "MATSHITA", 8)) {
				ReadCdRom = ReadCdda12Matsushita;
				global.in_lendian = 1;
			}
		} /* switch (get_mmc) */
	}

	/*
	 * look if caddy is loaded
	 */
	if (interface == GENERIC_SCSI) {
		scgp->silent++;
		while (!wait_unit_ready(scgp, 60)) {
			int	c;

			fprintf(outfp,
			_("load cdrom please and press enter"));
			fflush(outfp);
			while ((c = getchar()) != '\n') {
				if (c == EOF)
					break;
			}
			if (c == EOF)
				exit(DEVICE_ERROR);
		}
		scgp->silent--;
	}
}

/********************** General setup *******************************/

/*
 * As the name implies, interfaces and devices are checked.  We also
 * adjust nsectors, overlap, and interface for the first time here.
 * Any unnecessary privileges (setuid, setgid) are also dropped here.
 */
static void
Check_interface_for_device(statstruct, pdev_name)
	struct stat	*statstruct;
	char		*pdev_name;
{

#if !defined(STAT_MACROS_BROKEN) || (STAT_MACROS_BROKEN != 1)
	if (!S_ISCHR(statstruct->st_mode) &&
	    !S_ISBLK(statstruct->st_mode)) {
		errmsgno(EX_BAD, _("%s is not a device.\n"),
			pdev_name);
		exit(SYNTAX_ERROR);
	}
#endif

#if defined(HAVE_ST_RDEV) && (HAVE_ST_RDEV == 1)
	switch (major(statstruct->st_rdev)) {
#if defined(__linux__)
	case SCSI_GENERIC_MAJOR:	/* generic */
#else
	default:			/* ??? what is the proper value here */
#endif
#if !defined(STAT_MACROS_BROKEN) || (STAT_MACROS_BROKEN != 1)
#if defined(__linux__)
		if (!S_ISCHR(statstruct->st_mode)) {
			errmsgno(EX_BAD, _("%s is not a char device.\n"),
				pdev_name);
			exit(SYNTAX_ERROR);
		}

		if (interface != GENERIC_SCSI) {
			fprintf(outfp,
			_("wrong interface (cooked_ioctl) for this device (%s)\nset to generic_scsi\n"),
				pdev_name);
			interface = GENERIC_SCSI;
		}
#endif
#else
	default:			/* ??? what is the proper value here */
#endif
		break;

#if defined(__linux__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
#if defined(__linux__)
	case SCSI_CDROM_MAJOR:		/* scsi cd */
	default:			/* for example ATAPI cds */
#else
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
#if __FreeBSD_version >= 600021
	case 0:				/* majors abandoned */
		/* FALLTHROUGH */
#endif
#if __FreeBSD_version >= 501113
	case 4:				/* GEOM */
		/* FALLTHROUGH */
#endif
	case 117:			/* pre-GEOM atapi cd */
		if (!S_ISCHR(statstruct->st_mode)) {
			errmsgno(EX_BAD, _("%s is not a char device.\n"),
				pdev_name);
			exit(SYNTAX_ERROR);
		}
		if (interface != COOKED_IOCTL) {
			fprintf(outfp,
_("cdrom device (%s) is not of type generic SCSI. \
Setting interface to cooked_ioctl.\n"), pdev_name);
			interface = COOKED_IOCTL;
		}
		break;
	case 19:			/* first atapi cd */
#endif
#endif
		if (!S_ISBLK(statstruct->st_mode)) {
			errmsgno(EX_BAD, _("%s is not a block device.\n"),
				pdev_name);
			exit(SYNTAX_ERROR);
		}
#if defined(__linux__)
#if LINUX_VERSION_CODE >= 0x20600
		/*
		 * In Linux kernel 2.6 it is better to use the SCSI interface
		 * with the device.
		 */
		break;
#endif
#endif
		if (interface != COOKED_IOCTL) {
			fprintf(outfp,
_("cdrom device (%s) is not of type generic SCSI. \
Setting interface to cooked_ioctl.\n"), pdev_name);
			interface = COOKED_IOCTL;
		}

		if (interface == COOKED_IOCTL) {
			fprintf(outfp,
			_("\nW: The cooked_ioctl interface is functionally very limited!!\n"));
#if	defined(__linux__)
			fprintf(outfp,
			_("\nW: For good sampling quality simply use the generic SCSI interface!\n"
				"For example dev=1,0,0\n"));
#endif
		}

		break;
#endif
	}
#endif
	if (global.overlap >= global.nsectors)
		global.overlap = global.nsectors-1;
}

/*
 * open the cdrom device
 */
static int
OpenCdRom(pdev_name)
	char	*pdev_name;
{
	int		retval = 0;
	struct stat	fstatstruct;
#ifdef HAVE_IOCTL_INTERFACE
	struct stat	statstruct;
	int	have_named_device = 0;
#endif

	interface = GENERIC_SCSI;

	/*
	 * The device (given by pdevname) can be:
	 * a. an SCSI device specified with a /dev/xxx name,
	 * b. an SCSI device specified with bus,target,lun numbers,
	 * c. a non-SCSI device such as ATAPI or proprietary CDROM devices.
	 */
#ifdef HAVE_IOCTL_INTERFACE
	have_named_device = FALSE;
	if (pdev_name) {
		have_named_device = strchr(pdev_name, ':') == NULL &&
					memcmp(pdev_name, "/dev/", 5) == 0;
	} else {
		interface = GENERIC_SCSI;	/* Paranoia for "coverity" */
	}

	if (have_named_device) {
		if (stat(pdev_name, &statstruct)) {
			errmsg(_("Cannot stat device %s.\n"), pdev_name);
			exit(STAT_ERROR);
		} else {
			Check_interface_for_device(&statstruct, pdev_name);
		}
	}
#endif

	if (interface == GENERIC_SCSI) {
		SCSI	*scgp;
		char	errstr[80];

		needroot(0);
		needgroup(0);
		if (global.issetuid || global.uid != 0)
			priv_on();
		/*
		 * Call scg_remote() to force loading the remote SCSI transport
		 * library code that is located in librscg instead of the dummy
		 * remote routines that are located inside libscg.
		 */
		scg_remote();
		if (pdev_name != NULL &&
		    ((strncmp(pdev_name, "HELP", 4) == 0) ||
		    (strncmp(pdev_name, "help", 4) == 0))) {
			scg_help(stderr);
			exit(NO_ERROR);
		}

		/*
		 * device name, debug, verboseopen
		 */
		_scgp = scg_open(pdev_name, errstr, sizeof (errstr), 0, 0);

		if (_scgp == NULL) {
			scg_openerr(errstr);
			/* NOTREACHED */
		}
		scgp = _scgp;
		scg_settimeout(scgp, 300);
		scg_settimeout(scgp, 60);
		if (global.dev_opts) {
			int	i = scg_opts(scgp, global.dev_opts);
			if (i <= 0)
				exit(i < 0 ? EX_BAD : 0);
		}
		scgp->silent = global.scsi_silent;
		scgp->verbose = global.scsi_verbose;
		scgp->debug = global.scsi_debug;
		scgp->kdebug = global.scsi_kdebug;

		global.bufsize = scg_bufsize(scgp, global.bufsize);
		if (global.nsectors >
		    (unsigned)global.bufsize/CD_FRAMESIZE_RAW) {
			global.nsectors = global.bufsize/CD_FRAMESIZE_RAW;
		}
#ifdef	USE_PARANOIA
		if (global.paranoia_parms.enable_c2_check && global.nsectors >
		    (unsigned)global.bufsize/CD_FRAMESIZE_RAWER) {
			global.nsectors = global.bufsize/CD_FRAMESIZE_RAWER;
		}
#endif
		if (global.overlap >= global.nsectors)
			global.overlap = global.nsectors-1;

		/*
		 * Newer versions of Linux seem to introduce an incompatible
		 * change and require root privileges or limit RLIMIT_MEMLOCK
		 * infinity in order to get a SCSI buffer in case we did call
		 * mlockall(MCL_FUTURE).
		 */
		init_scsibuf(scgp, global.bufsize);
		if (global.issetuid || global.uid != 0)
			priv_off();
		dontneedgroup();
		dontneedroot();

		if (global.scanbus) {
			int	i = select_target(scgp, outfp);

			if (i < 0) {
				scg_openerr("");
				/* NOTREACHED */
			}
			exit(0);
		}
		if (scg_scsibus(scgp) < 0 &&
				scg_target(scgp) < 0 && scg_lun(scgp) < 0) {
			int	i = find_drive(scgp, pdev_name);

			if (i < 0) {
				scg_openerr("");
				/* NOTREACHED */
			}
		}
	} else {
		needgroup(0);
		retval = open(pdev_name,
#ifdef	linux
					O_NONBLOCK |
#endif
					O_RDONLY);
		dontneedgroup();

		if (retval < 0) {
			errmsg(_("Cannot open '%s'.\n"), pdev_name);
			exit(DEVICEOPEN_ERROR);
		}

		/*
		 * Do final security checks here
		 */
		if (fstat(retval, &fstatstruct)) {
			errmsg(_("Could not fstat %s (fd %d).\n"),
				pdev_name, retval);
			exit(STAT_ERROR);
		}
		Check_interface_for_device(&fstatstruct, pdev_name);

#if defined HAVE_IOCTL_INTERFACE
/* Watch for race conditions */
		if (have_named_device &&
		    (fstatstruct.st_dev != statstruct.st_dev ||
		    fstatstruct.st_ino != statstruct.st_ino)) {
			errmsgno(EX_BAD,
			_("Race condition attempted in OpenCdRom.  Exiting now.\n"));
			exit(RACE_ERROR);
		}
#endif
		/*
		 * The program structure looks like a desaster :-(
		 * We do this more than once as it is impossible to understand
		 * where the right place would be to do this....
		 */
		if (_scgp != NULL) {
			_scgp->verbose = global.scsi_verbose;
		}
	}
	return (retval);
}

LOCAL void
scg_openerr(errstr)
	char	*errstr;
{
	int	err = geterrno();

	errmsgno(err, _("%s%sCannot open or use SCSI driver.\n"),
			errstr, errstr[0]?". ":"");
	errmsgno(EX_BAD,
	_("For possible targets try 'cdda2wav -scanbus'.%s\n"),
			geteuid() ?
				_(" Make sure you are root."):"");

	if (global.issetuid || global.uid != 0)
		priv_off();
	dontneedgroup();
	dontneedroot();
#if defined(sun) || defined(__sun)
	fprintf(stderr,
	_("On SunOS/Solaris make sure you have Joerg Schillings scg SCSI driver installed.\n"));
#endif
#if defined(__linux__)
	fprintf(stderr,
	_("Use the script scan_scsi.linux to find out more.\n"));
#endif
	fprintf(stderr,
	_("Probably you did not define your SCSI device.\n"));
	fprintf(stderr,
	_("Set the CDDA_DEVICE environment variable or use the -D option.\n"));
	fprintf(stderr,
	_("You can also define the default device in the Makefile.\n"));
	fprintf(stderr,
	_("For possible transport specifiers try 'cdda2wav dev=help'.\n"));
	exit(SYNTAX_ERROR);
}

LOCAL int
find_drive(scgp, dev)
	SCSI	*scgp;
	char	*dev;
{
	int	ntarget;

	fprintf(outfp, _("No target specified, trying to find one...\n"));
	ntarget = find_target(scgp, INQ_ROMD, -1);
	if (ntarget < 0)
		return (ntarget);
	if (ntarget == 1) {
		/*
		 * Simple case, exactly one CD-ROM found.
		 */
		find_target(scgp, INQ_ROMD, 1);
	} else if (ntarget <= 0 &&
			(ntarget = find_target(scgp, INQ_WORM, -1)) == 1) {
		/*
		 * Exactly one CD-ROM acting as WORM found.
		 */
		find_target(scgp, INQ_WORM, 1);
	} else if (ntarget <= 0) {
		/*
		 * No single CD-ROM or WORM found.
		 */
		errmsgno(EX_BAD, _("No CD/DVD/BD-Recorder target found.\n"));
		errmsgno(EX_BAD,
		_("Your platform may not allow to scan for SCSI devices.\n"));
		comerrno(EX_BAD,
		_("Call 'cdda2wav dev=help' or ask your sysadmin for possible targets.\n"));
	} else {
		errmsgno(EX_BAD, _("Too many CD/DVD/BD-Recorder targets found.\n"));
		select_target(scgp, outfp);
		comerrno(EX_BAD,
		_("Select a target from the list above and use 'cdda2wav dev=%s%sb,t,l'.\n"),
			dev?dev:"", dev?(dev[strlen(dev)-1] == ':'?"":":"):"");
	}
	fprintf(outfp, _("Using dev=%s%s%d,%d,%d.\n"),
			dev?dev:"", dev?(dev[strlen(dev)-1] == ':'?"":":"):"",
			scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp));
	return (ntarget);
}


#endif /* SIM_CD */

/******************* Simulation interface *****************/
#if	defined SIM_CD
#include "toc.h"
static unsigned long	sim_pos = 0;

/*
 * read 'SectorBurst' adjacent sectors of audio sectors
 * to Buffer '*p' beginning at sector 'lSector'
 */
static int	ReadCdRom_sim	__PR((SCSI *x, UINT4 *p, unsigned lSector,
						unsigned SectorBurstVal));
static int
ReadCdRom_sim(x, p, lSector, SectorBurstVal)
	SCSI		*x;
	UINT4		*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
	unsigned int	loop = 0;
	Int16_t		*q = (Int16_t *) p;
	int		joffset = 0;

	if (lSector > g_toc[cdtracks].dwStartSector ||
	    lSector + SectorBurstVal > g_toc[cdtracks].dwStartSector + 1) {
		fprintf(stderr,
		_("Read request out of bounds: %u - %u (%d - %d allowed)\n"),
			lSector, lSector + SectorBurstVal,
			0, g_toc[cdtracks].dwStartSector);
	}
#if 0
	/*
	 * jitter with a probability of jprob
	 */
	if (random() <= jprob) {
		/*
		 * jitter up to jmax samples
		 */
		joffset = random();
	}
#endif

#ifdef DEBUG_SHM
	fprintf(stderr, ", last_b = %p\n", *last_buffer);
#endif
	for (loop = lSector*CD_FRAMESAMPLES + joffset;
	    loop < (lSector+SectorBurstVal)*CD_FRAMESAMPLES + joffset;
	    loop++) {
		*q++ = loop;
		*q++ = ~loop;
	}
#ifdef DEBUG_SHM
	fprintf(stderr,
		"sim wrote from %p upto %p - 4 (%d), last_b = %p\n",
		p, q, SectorBurstVal*CD_FRAMESAMPLES, *last_buffer);
#endif
	sim_pos = (lSector+SectorBurstVal)*CD_FRAMESAMPLES + joffset;
	return (SectorBurstVal);
}

static int	Play_at_sim	__PR((SCSI *x, unsigned int from_sector,
							unsigned int sectors));
static int
Play_at_sim(x, from_sector, sectors)
	SCSI		*x;
	unsigned int	from_sector;
	unsigned int	sectors;
{
	sim_pos = from_sector*CD_FRAMESAMPLES;
	return (0);
}

static unsigned	sim_indices;


/*
 * read the table of contents (toc) via the ioctl interface
 */
static unsigned	ReadToc_sim	__PR((SCSI *x, TOC *toc));
static unsigned
ReadToc_sim(x, toc)
	SCSI	*x;
	TOC	*toc;
{
	unsigned int	scenario;
	int		scen[12][3] = {
		{ 1,   1, 500	},
		{ 1,   2, 500		},
		{ 1,  99, 150*99	},
		{ 2,   1, 500		},
		{ 2,   2, 500		},
		{ 2,  99, 150*99	},
		{ 2,   1, 500		},
		{ 5,   2, 500		},
		{ 5,  99, 150*99	},
		{ 99,  1, 1000		},
		{ 99,  2, 1000		},
		{ 99, 99, 150*99	},
	};
	unsigned int	i;
	unsigned	trcks;
#if 0
	fprintf(stderr, "select one of the following TOCs\n"
		"0 :  1 track  with  1 index\n"
		"1 :  1 track  with  2 indices\n"
		"2 :  1 track  with 99 indices\n"
		"3 :  2 tracks with  1 index each\n"
		"4 :  2 tracks with  2 indices each\n"
		"5 :  2 tracks with 99 indices each\n"
		"6 :  2 tracks (data and audio) with  1 index each\n"
		"7 :  5 tracks with  2 indices each\n"
		"8 :  5 tracks with 99 indices each\n"
		"9 : 99 tracks with  1 index each\n"
		"10: 99 tracks with  2 indices each\n"
		"11: 99 tracks with 99 indices each\n");

	do {
		scanf("%u", &scenario);
	} while (scenario > sizeof (scen)/2/sizeof (int));
#else
	scenario = 6;
#endif
	/*
	 * build table of contents
	 */
#if 0
	trcks = scen[scenario][0] + 1;
	sim_indices = scen[scenario][1];

	for (i = 0; i < trcks; i++) {
		toc[i].bFlags = (scenario == 6 && i == 0) ? 0x40 : 0xb1;
		toc[i].bTrack = i + 1;
		toc[i].dwStartSector = i * scen[scenario][2];
		toc[i].mins = (toc[i].dwStartSector+150) / (60*75);
		toc[i].secs = (toc[i].dwStartSector+150 / 75) % (60);
		toc[i].frms = (toc[i].dwStartSector+150) % (75);
	}
	toc[i].bTrack = 0xaa;
	toc[i].dwStartSector = i * scen[scenario][2];
	toc[i].mins = (toc[i].dwStartSector+150) / (60*75);
	toc[i].secs = (toc[i].dwStartSector+150 / 75) % (60);
	toc[i].frms = (toc[i].dwStartSector+150) % (75);
#else
	{
		int	starts[15] = { 23625, 30115, 39050, 51777, 67507,
				88612, 112962, 116840, 143387, 162662,
				173990, 186427, 188077, 209757, 257120};

		trcks = 14 + 1;
		sim_indices = 1;

		for (i = 0; i < trcks; i++) {
			toc[i].bFlags = 0x0;
			toc[i].bTrack = i + 1;
			toc[i].dwStartSector = starts[i];
			toc[i].mins = (starts[i]+150) / (60*75);
			toc[i].secs = (starts[i]+150 / 75) % (60);
			toc[i].frms = (starts[i]+150) % (75);
		}
		toc[i].bTrack = 0xaa;
		toc[i].dwStartSector = starts[i];
		toc[i].mins = (starts[i]) / (60*75);
		toc[i].secs = (starts[i] / 75) % (60);
		toc[i].frms = (starts[i]) % (75);
	}
#endif
	return (--trcks);	/* without lead-out */
}


static subq_chnl	*ReadSubQ_sim	__PR((SCSI *scgp,
						unsigned char sq_format,
						unsigned char track));
/*
 * request sub-q-channel information. This function may cause confusion
 * for a drive, when called in the sampling process.
 */
static subq_chnl *
ReadSubQ_sim(scgp, sq_format, track)
	SCSI		*scgp;
	unsigned char	sq_format;
	unsigned char	track;
{
	subq_chnl	*SQp = (subq_chnl *) (SubQbuffer);
	subq_position	*SQPp = (subq_position *) &SQp->data;
	unsigned long	sim_pos1;
	unsigned long	sim_pos2;

	if (sq_format != GET_POSITIONDATA)
		return (NULL);			/* not supported by sim */

	/*
	 * simulate CDROMSUBCHNL ioctl
	 *
	 * copy to SubQbuffer
	 */
	SQp->audio_status 	= 0;
	SQp->format		= 0xff;
	SQp->control_adr	= 0xff;
	sim_pos1		= sim_pos/CD_FRAMESAMPLES;
	sim_pos2		= sim_pos1 % 150;
	SQp->track 		= (sim_pos1 / 5000) + 1;
	SQp->index 		= ((sim_pos1 / 150) % sim_indices) + 1;
	sim_pos1		+= 150;
	SQPp->abs_min		= sim_pos1 / (75*60);
	SQPp->abs_sec		= (sim_pos1 / 75) % 60;
	SQPp->abs_frame		= sim_pos1 % 75;
	SQPp->trel_min		= sim_pos2 / (75*60);
	SQPp->trel_sec		= (sim_pos2 / 75) % 60;
	SQPp->trel_frameb	= sim_pos2 % 75;

	return ((subq_chnl *)(SubQbuffer));
}

static void	SelectSpeed_sim	__PR((SCSI *x, unsigned sp));
/* ARGSUSED */
static void
SelectSpeed_sim(x, sp)
	SCSI		*x;
	unsigned	sp;
{
}

static void trash_cache_sim __PR((UINT4 *p, unsigned lSector,
						unsigned SectorBurstVal));

/* ARGSUSED */
static void
trash_cache_sim(p, lSector, SectorBurstVal)
	UINT4		*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
}

static void	SetupSimCd	__PR((void));

static void
SetupSimCd()
{
	EnableCdda = (void (*) __PR((SCSI *, int, unsigned)))Dummy;
	ReadCdRom = ReadCdRom_sim;
	ReadCdRomData = (int (*) __PR((SCSI *,
					unsigned char *,
					unsigned, unsigned)))ReadCdRom_sim;
	doReadToc = ReadToc_sim;
	ReadTocText = (void (*) __PR((SCSI *)))NULL;
	ReadSubQ = ReadSubQ_sim;
	ReadSubChannels = (subq_chnl * (*) __PR((SCSI *, unsigned)))NULL;
	ReadLastAudio = (unsigned (*) __PR((SCSI *)))NULL;
	SelectSpeed = SelectSpeed_sim;
	Play_at = Play_at_sim;
	StopPlay = (int (*) __PR((SCSI *)))Dummy;
	trash_cache = trash_cache_sim;
}

#endif /* def SIM_CD */

/* perform initialization depending on the interface used. */
void
SetupInterface()
{
#if	defined SIM_CD
	fprintf(stderr, "SIMULATION MODE !!!!!!!!!!!\n");
#else
	/*
	 * ensure interface is setup correctly
	 */
	global.cooked_fd = OpenCdRom(global.dev_name);
#endif

#ifdef  _SC_PAGESIZE
	global.pagesize = sysconf(_SC_PAGESIZE);
#else
	global.pagesize = getpagesize();
#endif

	/*
	 * request one sector for table of contents
	 */
	bufTOCsize = CD_FRAMESIZE_RAW + 96;	/* Sufficient space for 222 TOC entries */
	bufferTOC = malloc(bufTOCsize);		/* assumes sufficient aligned addresses */
	/*
	 * SubQchannel buffer
	 */
	SubQbuffer = malloc(48);		/* assumes sufficient aligned addresses */
	if (!bufferTOC || !SubQbuffer) {
		errmsg(_("Too low on memory. Giving up.\n"));
		exit(NOMEM_ERROR);
	}

#if	defined SIM_CD
	scgp = malloc(sizeof (* scgp));
	if (scgp == NULL) {
		FatalError(geterrno(), _("No memory for SCSI structure.\n"));
	}
	scgp->silent = 0;
	SetupSimCd();
#else
	/*
	 * if drive is of type scsi, get vendor name
	 */
	if (interface == GENERIC_SCSI) {
		unsigned	sector_size;
		SCSI		*scgp = _scgp;

		SetupSCSI(scgp);
		sector_size = get_orig_sectorsize(scgp, &orgmode4, &orgmode10,
								&orgmode11);
		if (!SCSI_emulated_ATAPI_on(scgp)) {
			if (sector_size != 2048 &&
			    set_sectorsize(scgp, 2048)) {
				fprintf(stderr,
				_("Could not change sector size from %u to 2048\n"),
					sector_size);
			}
		}

		/*
		 * get cache setting
		 *
		 * set cache to zero
		 */
	} else {
#if defined(HAVE_IOCTL_INTERFACE)
		_scgp = malloc(sizeof (* _scgp));
		if (_scgp == NULL) {
			FatalError(geterrno(),
				_("No memory for SCSI structure.\n"));
		}
		_scgp->silent = 0;
		SetupCookedIoctl(global.dev_name);
#else
		FatalError(EX_BAD,
		_("Sorry, there is no known method to access the device.\n"));
#endif
	}
#endif	/* if def SIM_CD */
	/*
	 * The structure looks like a desaster :-(
	 * We do this more than once as it is impossible to understand where
	 * the right place would be to do this....
	 */
	if (_scgp != NULL) {
		_scgp->verbose = global.scsi_verbose;
	}
}

EXPORT int
poll_in()
{
#ifdef	HAVE_POLL
	struct pollfd	pfd[1];

	pfd[0].fd = fileno(stdin);
	pfd[0].events = POLLIN;
	pfd[0].revents = 0;
	return (poll(pfd, 1, 0));
#else
#ifdef	HAVE_SELECT
	struct timeval	tv;
	fd_set		rd;

	FD_ZERO(&rd);
	FD_SET(fileno(stdin), &rd);

	tv.tv_sec = 0;
	tv.tv_usec = 0;
	return (select(1, &rd, NULL, NULL, &tv));
#else
	comerrno(EX_BAD, _("Poll/Select not available.\n"));
#endif
#endif
}
