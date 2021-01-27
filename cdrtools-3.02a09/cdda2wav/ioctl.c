/* @(#)ioctl.c	1.39 10/12/19 Copyright 1998,1999,2000 Heiko Eissfeldt, Copyright 2006-2010 J. Schilling */
#include "config.h"
#ifndef lint
static	UConst char sccsid[] =
"@(#)ioctl.c	1.39 10/12/19 Copyright 1998,1999,2000 Heiko Eissfeldt, Copyright 2006-2010 J. Schilling";

#endif
/*
 * Copyright (C) 1999 Heiko Eissfeldt heiko@colossus.escape.de
 * Copyright (c) 2006-2010 J. Schilling
 *
 * Ioctl interface module for cdrom drive access
 *
 * Solaris ATAPI cdrom drives are untested!
 *
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

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
#include <schily/ioctl.h>
#include <schily/stat.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>
#include <schily/device.h>

#include <scg/scsitransp.h>

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

#include "mytype.h"
#include "byteorder.h"
#include "interface.h"
#include "toc.h"
#include "cdda2wav.h"
#include "ioctl.h"
#include "global.h"
#include "exitcodes.h"

#include <schily/utypes.h>
#include <cdrecord.h>

#if defined(HAVE_IOCTL_INTERFACE)
#if  !defined(sun) && !defined(__sun) && \
		!((defined(__FreeBSD__) || defined(__FreeBSD_kernel__)) && \
				(__FreeBSD_version >= 501112))
static struct cdrom_read_audio arg;
#endif

#if ((defined(__FreeBSD__) || defined(__FreeBSD_kernel__)) && \
				__FreeBSD_version >= 400014) || \
					defined(__DragonFly__)
static unsigned sector_size;
#endif

static int err;

static void	EnableCdda_cooked	__PR((SCSI *scgp, int fAudioMode,
							unsigned uSectorsize));
/* ARGSUSED */
static void
EnableCdda_cooked(scgp, fAudioMode, uSectorsize)
	SCSI		*scgp;
	int		fAudioMode;
	unsigned	uSectorsize;
{
#if	((defined(__FreeBSD__) || defined(__FreeBSD_kernel__)) && \
				__FreeBSD_version >= 400014) || \
					defined(__DragonFly__)
	if (scgp && scgp->verbose)
		fprintf(stderr, _("EnableCdda_cooked (CDRIOCSETBLOCKSIZE)...\n"));

	if (fAudioMode) {
		if (ioctl(global.cooked_fd, CDRIOCGETBLOCKSIZE,
							&sector_size) == -1)
			sector_size = CD_FRAMESIZE;
		ioctl(global.cooked_fd, CDRIOCSETBLOCKSIZE, &uSectorsize);
	} else
		ioctl(global.cooked_fd, CDRIOCSETBLOCKSIZE, &sector_size);
#else
#if	defined	CDIOCSETCDDA
	if (scgp && scgp->verbose) {
		fprintf(stderr, _("EnableCdda_cooked (CDIOCSETCDDA)...\n"));
		if (uSectorsize != CD_FRAMESIZE_RAW)
			fprintf(stderr, _("non audio sector size is ignored.\n"));
	}

	ioctl(global.cooked_fd, CDIOCSETCDDA, &fAudioMode);
#else
	fprintf(stderr,
		_("EnableCdda_cooked (CDIOCSETCDDA) is not available...\n"));
#endif
#endif

}


static unsigned ReadToc_cooked __PR((SCSI *x));

/* read the table of contents (toc) via the ioctl interface */
static unsigned
ReadToc_cooked(x)
	SCSI	*x;
{
	unsigned	i;
	unsigned	tracks;
	struct cdrom_tochdr	hdr;
	struct cdrom_tocentry	entry[100];
	struct cdrom_tocentry	entryMSF[100];

	if (x && x->verbose) {
		fprintf(stderr, _("ReadToc_cooked (CDROMREADTOCHDR)...\n"));
	}

	/*
	 * get TocHeader to find out how many entries there are
	 */
	err = ioctl(global.cooked_fd, CDROMREADTOCHDR, &hdr);
	if (err != 0) {
		/*
		 * error handling
		 */
		if (err == -1) {
			if (errno == EPERM) {
				fprintf(stderr,
				_("Please run this program setuid root.\n"));
			}
			errmsg(_("Cooked: Error in read TOC.\n"));
			exit(DEVICE_ERROR);
		} else {
			errmsgno(EX_BAD,
				_("Can't get TocHeader (error %d).\n"), err);
			exit(MEDIA_ERROR);
		}
	}
	/*
	 * get all TocEntries
	 */
	for (i = 0; i < hdr.cdth_trk1; i++) {
		entryMSF[i].cdte_track = 1+i;
		entryMSF[i].cdte_format = CDROM_MSF;
		err = ioctl(global.cooked_fd, CDROMREADTOCENTRY, &entryMSF[i]);
		if (err != 0) {
			/*
			 * error handling
			 */
			errmsg(("Can't get TocEntry #%d msf (error %d).\n"),
				i+1, err);
			exit(MEDIA_ERROR);
		}
	}
	entryMSF[i].cdte_track = CDROM_LEADOUT;
	entryMSF[i].cdte_format = CDROM_MSF;
	err = ioctl(global.cooked_fd, CDROMREADTOCENTRY, &entryMSF[i]);
	if (err != 0) {
		/*
		 * error handling
		 */
		errmsg(_("Can't get TocEntry LEADOUT msf (error %d).\n"),
			err);
		exit(MEDIA_ERROR);
	}
	tracks = hdr.cdth_trk1+1;
#ifdef nonono
	for (i = 0; i < tracks; i++) {
		toc[i].bFlags = (entry[i].cdte_adr << 4) | (entry[i].cdte_ctrl & 0x0f);
		toc[i].bTrack = entry[i].cdte_track;
		toc[i].mins = entry[i].cdte_addr.msf.minute;
		toc[i].secs = entry[i].cdte_addr.msf.second;
		toc[i].frms = entry[i].cdte_addr.msf.frame;
	}
#endif
	/*
	 * get all TocEntries now in lba format
	 */
	for (i = 0; i < hdr.cdth_trk1; i++) {
		entry[i].cdte_track = 1+i;
		entry[i].cdte_format = CDROM_LBA;
		err = ioctl(global.cooked_fd, CDROMREADTOCENTRY, &entry[i]);
		if (err != 0) {
			/*
			 * error handling
			 */
			errmsg(_("Can't get TocEntry #%d lba (error %d).\n"),
				i+1, err);
			exit(MEDIA_ERROR);
		}
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
		entry[i].cdte_addr.lba = be32_to_cpu(entry[i].cdte_addr.lba);
#endif
	}
	entry[i].cdte_track = CDROM_LEADOUT;
	entry[i].cdte_format = CDROM_LBA;
	err = ioctl(global.cooked_fd, CDROMREADTOCENTRY, &entry[i]);
	if (err != 0) {
		/*
		 * error handling
		 */
		errmsg(_("Can't get TocEntry LEADOUT lba (error %d).\n"),
			err);
		exit(MEDIA_ERROR);
	}
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
	entry[i].cdte_addr.lba = be32_to_cpu(entry[i].cdte_addr.lba);
#endif

	for (i = 0; i < tracks; i++) {
		toc_entry(i+1,
			(entry[i].cdte_adr << 4) | (entry[i].cdte_ctrl & 0x0f),
			entry[i].cdte_track,
			NULL /* ISRC */,
			entry[i].cdte_addr.lba,
			entryMSF[i].cdte_addr.msf.minute,
			entryMSF[i].cdte_addr.msf.second,
			entryMSF[i].cdte_addr.msf.frame);
	}
	bufferTOC[0] = '\0';
	bufferTOC[1] = '\0';
	return (--tracks);	/* without lead-out */
}

static void	trash_cache_cooked __PR((UINT4 *p, unsigned lSector,
						unsigned SectorBurstVal));

static void
trash_cache_cooked(p, lSector, SectorBurstVal)
	UINT4	*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
	/*
	 * trash the cache
	 */

#if	defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || \
						defined(__DragonFly__)
#if	(defined(__FreeBSD__) || defined(__FreeBSD_kernel__)) && \
						__FreeBSD_version >= 501112
	pread(global.cooked_fd, (void *) &p[0], 3*CD_FRAMESIZE_RAW,
		find_an_off_sector(lSector, SectorBurstVal)*CD_FRAMESIZE_RAW);
#else
	static struct cdrom_read_audio	arg2;

	arg2.address.lba = find_an_off_sector(lSector, SectorBurstVal);
	arg2.addr_format = CDROM_LBA;
	arg2.nframes = 3;
	arg2.buffer = (unsigned char *) &p[0];

	ioctl(global.cooked_fd, CDROMREADAUDIO, &arg2);
#endif
#endif
#if	defined __linux__
	static struct cdrom_read_audio	arg2;

	arg2.addr.lba = find_an_off_sector(lSector, SectorBurstVal);
	arg2.addr_format = CDROM_LBA;
	arg2.nframes = 3;
	arg2.buf = (unsigned char *) &p[0];

	ioctl(global.cooked_fd, CDROMREADAUDIO, &arg2);
#endif
#if	defined __sun || \
		(defined HAVE_SYS_CDIO_H && defined CDROM_DA_NO_SUBCODE)
	struct cdrom_cdda	suncdda;

	suncdda.cdda_addr = lSector;
	suncdda.cdda_length = SectorBurstVal*CD_FRAMESIZE_RAW;
	suncdda.cdda_data = (char *) &p[0];
	suncdda.cdda_subcode = CDROM_DA_NO_SUBCODE;

	ioctl(global.cooked_fd, CDROMCDDA, &suncdda);
#endif
}

static void ReadCdRomData_cooked __PR((SCSI *x, UINT4 *p, unsigned lSector,
						unsigned SectorBurstVal));
/*
 * read 'SectorBurst' adjacent sectors of data sectors
 * to Buffer '*p' beginning at sector 'lSector'
 */
static void
ReadCdRomData_cooked(x, p, lSector, SectorBurstVal)
	SCSI		*x;
	UINT4		*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
	int		retval;

	if (x && x->verbose) {
		fprintf(stderr, _("ReadCdRomData_cooked (lseek & read)...\n"));
	}

	if ((retval = lseek(global.cooked_fd, lSector*CD_FRAMESIZE, SEEK_SET))
				!= (int)lSector*CD_FRAMESIZE) {
		errmsg(_("Cannot seek sector.\n"));
	}
	if ((retval = read(global.cooked_fd, p, SectorBurstVal*CD_FRAMESIZE))
				!= (int)SectorBurstVal*CD_FRAMESIZE) {
		errmsg(_("Cannot read sector.\n"));
	}
}

static int	ReadCdRom_cooked __PR((SCSI *x, UINT4 *p, unsigned lSector,
						unsigned SectorBurstVal));
/*
 * read 'SectorBurst' adjacent sectors of audio sectors
 * to Buffer '*p' beginning at sector 'lSector'
 */
static int
ReadCdRom_cooked(x, p, lSector, SectorBurstVal)
	SCSI		*x;
	UINT4		*p;
	unsigned	lSector;
	unsigned	SectorBurstVal;
{
	int	retry_count = 0;
static int	nothing_read = 1;

/* read 2352 bytes audio data */
#if	defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || \
						defined(__DragonFly__)
#if	(defined(__FreeBSD__) || defined(__FreeBSD_kernel__)) && \
						__FreeBSD_version >= 501112
	if (x && x->verbose) {
		fprintf(stderr, _("ReadCdRom_cooked (pread)...\n"));
	}

	do {
		err = 0;
		if (pread(global.cooked_fd, (void *) &p[0],
				SectorBurstVal*CD_FRAMESIZE_RAW,
				lSector*CD_FRAMESIZE_RAW) == -1) {
			err = -1;
		}
#else
	arg.address.lba = lSector;
	arg.addr_format = CDROM_LBA;
	arg.nframes = SectorBurstVal;
	arg.buffer = (unsigned char *) &p[0];

	if (x && x->verbose) {
		fprintf(stderr, _("ReadCdRom_cooked (CDROMREADAUDIO)...\n"));
	}

	do {
		err = ioctl(global.cooked_fd, CDROMREADAUDIO, &arg);
#endif
#endif
#if	defined __linux__
	arg.addr.lba = lSector;
	arg.addr_format = CDROM_LBA;
	arg.nframes = SectorBurstVal;
	arg.buf = (unsigned char *) &p[0];

	if (x && x->verbose) {
		fprintf(stderr, _("ReadCdRom_cooked (CDROMREADAUDIO)...\n"));
	}

	do {
		err = ioctl(global.cooked_fd, CDROMREADAUDIO, &arg);
#endif
#if	defined __sun || \
		(defined HAVE_SYS_CDIO_H && defined CDROM_DA_NO_SUBCODE)
	struct cdrom_cdda	suncdda;

	suncdda.cdda_addr = lSector;
	suncdda.cdda_length = SectorBurstVal*CD_FRAMESIZE_RAW;
	suncdda.cdda_data = (char *) &p[0];
	suncdda.cdda_subcode = CDROM_DA_NO_SUBCODE;

	if (x && x->verbose) {
		fprintf(stderr, _("ReadCdRom_cooked (CDROMCDDA)...\n"));
	}

	do {
		err = ioctl(global.cooked_fd, CDROMCDDA, &suncdda);
#endif
		retry_count++;

		if (err) {
			trash_cache_cooked(p, lSector, SectorBurstVal);
		}

	} while ((err) && (retry_count < 30));

	if (err != 0) {
		if (x->silent == 0) {
			/*
			 * error handling
			 */
			if (err == -1) {
				if (nothing_read &&
				    (errno == EINVAL || errno == EIO)) {
					fprintf(stderr,
						_("Sorry, this driver and/or drive does not support cdda reading.\n"));
				}
				errmsg(_("Cooked: Error read cdda sector %u + %u, buffer %p + %x\n"),
					lSector, SectorBurstVal, p,
					global.shmsize);
			} else {
				errmsgno(EX_BAD,
					_("Can't read frame #%u (error %d).\n"),
					lSector, err);
			}
		}
		return (SectorBurstVal - 1);
	} else {
		nothing_read = 0;
	}

	return (SectorBurstVal);
}

static int	StopPlay_cooked	__PR((SCSI *x));
static int
StopPlay_cooked(x)
	SCSI	*x;
{
	if (x && x->verbose) {
		fprintf(stderr, _("StopPlay_cooked (CDROMSTOP)...\n"));
	}

	return (ioctl(global.cooked_fd, CDROMSTOP, 0) ? 0 : -1);
}

static int	Play_at_cooked __PR((SCSI *x, unsigned int from_sector,
							unsigned int sectors));
static int
Play_at_cooked(x, from_sector, sectors)
	SCSI	*x;
	unsigned int	from_sector;
	unsigned int	sectors;
{
	struct cdrom_msf	cmsf;
	int			retval;

	if (x && x->verbose) {
		fprintf(stderr,
		_("Play_at_cooked (CDROMSTART & CDROMPLAYMSF)... (%u-%u)"),
			from_sector, from_sector+sectors-1);
		fprintf(stderr, "\n");
	}

	cmsf.cdmsf_min0 = (from_sector + 150) / (60*75);
	cmsf.cdmsf_sec0 = ((from_sector + 150) / 75) % 60;
	cmsf.cdmsf_frame0 = (from_sector + 150) % 75;
	cmsf.cdmsf_min1 = (from_sector + 150 + sectors) / (60*75);
	cmsf.cdmsf_sec1 = ((from_sector + 150 + sectors) / 75) % 60;
	cmsf.cdmsf_frame1 = (from_sector + 150 + sectors) % 75;

#if	0
	/*
	 * makes index scanning under FreeBSD too slow
	 */
	if ((retval = ioctl(global.cooked_fd, CDROMSTART, 0)) != 0) {
		errmsg(_("Cannot do ioctl(%d, CDROMSTART, 0).\n"),
			global.cooked_fd);
	}
#endif
	if ((retval = ioctl(global.cooked_fd, CDROMPLAYMSF, &cmsf)) != 0) {
		errmsg(_("Cannot do ioctl(%d, CDROMPLAYMSF, ...).\n"),
			global.cooked_fd);
	}
	return (retval);
}

#if	defined	PROTOTYPES
static subq_chnl	*ReadSubQ_cooked __PR((SCSI *x,
						unsigned char sq_format,
						unsigned char track))
#else
/*
 * request sub-q-channel information. This function may cause confusion
 * for a drive, when called in the sampling process.
 */
static subq_chnl *
ReadSubQ_cooked(x, sq_format, track)
	SCSI	*x;
	unsigned char	sq_format;
	unsigned char	track;
#endif
{
	struct cdrom_subchnl		sub_ch;

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
	struct cd_sub_channel_info	sub_ch_info;

	if (x && x->verbose) {
		fprintf(stderr,
		_("ReadSubQ_cooked (CDROM_GET_MCN or CDROMSUBCHNL)...\n"));
	}

	sub_ch.address_format = CD_MSF_FORMAT;
	sub_ch.track = track;
	sub_ch.data_len = sizeof (struct cd_sub_channel_info);
	sub_ch.data = &sub_ch_info;

	switch (sq_format) {

	case GET_CATALOGNUMBER:
		sub_ch.data_format = CD_MEDIA_CATALOG;
#else
	if (x && x->verbose) {
		fprintf(stderr,
		_("ReadSubQ_cooked (CDROM_GET_MCN or CDROMSUBCHNL)...\n"));
	}

	switch (sq_format) {

	case GET_CATALOGNUMBER:
#endif
#if	defined CDROM_GET_MCN
		if (!(err = ioctl(global.cooked_fd, CDROM_GET_MCN,
					(struct cdrom_mcn *) SubQbuffer))) {
			subq_chnl *SQp = (subq_chnl *) SubQbuffer;
			subq_catalog *SQPp = (subq_catalog *) &SQp->data;

			memmove(SQPp->media_catalog_number, SQp,
					sizeof (SQPp->media_catalog_number));
			SQPp->zero = 0;
			SQPp->mc_valid = 0x80;
			break;
	} else
#endif
	{
		return (NULL);
	}

	case GET_POSITIONDATA:
#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__DragonFly__)
		sub_ch.data_format = CD_CURRENT_POSITION;
#endif
#if defined(__linux__)
		sub_ch.cdsc_format = CDROM_MSF;
#endif
		if (!(err = ioctl(global.cooked_fd, CDROMSUBCHNL, &sub_ch))) {
			/*
			 * copy to SubQbuffer
			 */
			subq_chnl *SQp = (subq_chnl *) (SubQbuffer);
			subq_position *SQPp = (subq_position *) SQp->data;
			SQp->audio_status 	= sub_ch.cdsc_audiostatus;
			SQp->format 		= sub_ch.cdsc_format;
			SQp->control_adr	= (sub_ch.cdsc_adr << 4) |
						    (sub_ch.cdsc_ctrl & 0x0f);
			SQp->track 		= sub_ch.cdsc_trk;
			SQp->index 		= sub_ch.cdsc_ind;
			SQPp->abs_min 	= sub_ch.cdsc_absaddr.msf.minute;
			SQPp->abs_sec 	= sub_ch.cdsc_absaddr.msf.second;
			SQPp->abs_frame 	= sub_ch.cdsc_absaddr.msf.frame;
			SQPp->trel_min 	= sub_ch.cdsc_reladdr.msf.minute;
			SQPp->trel_sec 	= sub_ch.cdsc_reladdr.msf.second;
			SQPp->trel_frame 	= sub_ch.cdsc_reladdr.msf.frame;
		} else {
			if (err == -1) {
				if (errno == EPERM)
					fprintf(stderr,
					_("Please run this program setuid root.\n"));
				errmsg(_("Cooked: Error in read subq.\n"));
				exit(DEVICE_ERROR);
			} else {
				errmsgno(EX_BAD,
				_("Can't read sub q channel (error %d).\n"),
					err);
				exit(DEVICE_ERROR);
			}
		}
		break;

	default:
		return (NULL);
	} /* switch */

	return ((subq_chnl *)(SubQbuffer));
}

/*
 * Speed control
 */
static void	SpeedSelect_cooked	__PR((SCSI *x, unsigned speed));
/* ARGSUSED */
static void
SpeedSelect_cooked(x, speed)
	SCSI		*x;
	unsigned	speed;
{
	if (x && x->verbose) {
		fprintf(stderr,
			_("SpeedSelect_cooked (CDROM_SELECT_SPEED)...\n"));
	}

#ifdef CDROM_SELECT_SPEED
	/*
	 * CAUTION!!!!! Non standard ioctl parameter types here!!!!
	 */
	if ((err = ioctl(global.cooked_fd, CDROM_SELECT_SPEED, speed))) {
		if (err == -1) {
			if (errno == EPERM) {
				fprintf(stderr,
				_("Please run this program setuid root.\n"));
			}
			errmsg(_("Cooked: Error in speed select.\n"));
			/* exit(err); */
		} else {
			errmsgno(EX_BAD,
				_("Can't set speed %d (error %d).\n"),
				speed, err);
			exit(DEVICE_ERROR);
		}
	}
#endif
}

/*
 * set function pointers to use the ioctl routines
 */
void
SetupCookedIoctl(pdev_name)
	char	*pdev_name;
{
#if (HAVE_ST_RDEV == 1)
	struct stat	statstruct;

	if (fstat(global.cooked_fd, &statstruct)) {
		errmsg(_("Cannot stat cd %d (%s).\n"),
				global.cooked_fd, pdev_name);
		exit(STAT_ERROR);
	}
#if	defined __linux__
	switch (major(statstruct.st_rdev)) {

	case CDU31A_CDROM_MAJOR:	/* sony cdu-31a/33a */
		global.nsectors = 13;
		if (global.nsectors >= 14) {
			global.overlap = 10;
		}
		break;

	case MATSUSHITA_CDROM_MAJOR:	/* sbpcd 1 */
	case MATSUSHITA_CDROM2_MAJOR:	/* sbpcd 2 */
	case MATSUSHITA_CDROM3_MAJOR:	/* sbpcd 3 */
	case MATSUSHITA_CDROM4_MAJOR:	/* sbpcd 4 */
		/*
		 * some are more compatible than others
		 */
		global.nsectors = 13;
		break;

	default:
		global.nsectors = 8;
		break;
	}

	err = ioctl(global.cooked_fd, CDROMAUDIOBUFSIZ, global.nsectors);

	switch (major(statstruct.st_rdev)) {

	case MATSUSHITA_CDROM_MAJOR:	/* sbpcd 1 */
	case MATSUSHITA_CDROM2_MAJOR:	/* sbpcd 2 */
	case MATSUSHITA_CDROM3_MAJOR:	/* sbpcd 3 */
	case MATSUSHITA_CDROM4_MAJOR:	/* sbpcd 4 */

		if (err == -1) {
			errmsg(_("Error doing ioctl(CDROMAUDIOBUFSIZ).\n"));
		}
	}
#endif
#endif
	EnableCdda = EnableCdda_cooked;
	ReadCdRom = ReadCdRom_cooked;
	ReadCdRomData = (int (*) __PR((SCSI *,
				unsigned char *,
				unsigned, unsigned))) ReadCdRomData_cooked;
	doReadToc = ReadToc_cooked;
	ReadTocText = (void (*) __PR((SCSI *)))NULL;
	ReadSubQ = ReadSubQ_cooked;
	ReadSubChannels = (subq_chnl * (*) __PR((SCSI *, unsigned)))NULL;
	SelectSpeed = SpeedSelect_cooked;
	Play_at = Play_at_cooked;
	StopPlay = StopPlay_cooked;
	trash_cache = trash_cache_cooked;
	ReadLastAudio = (unsigned (*) __PR((SCSI *)))NULL;
}
#endif
