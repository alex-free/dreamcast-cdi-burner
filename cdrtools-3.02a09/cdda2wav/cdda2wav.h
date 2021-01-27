/* @(#)cdda2wav.h	1.9 15/10/19 Copyright 1998-2000,2015 Heiko Eissfeldt, Copyright 2004-2006 J. Schilling */
/*
 * Copyright (C) 1998-2000,2015  Heiko Eissfeldt
 * Copyright (c) 2004-2006       J. Schilling
 *
 * prototypes from cdda2wav.c
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
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#define	max(a, b)	((a) > (b) ? (a) : (b))
#define	min(a, b)	((a) < (b) ? (a) : (b))


/* verbose levels */
#define	SHOW_TOC		1
#define	SHOW_SUMMARY		2
#define	SHOW_INDICES		4
#define	SHOW_MCN		8
#define	SHOW_ISRC		16
#define	SHOW_STARTPOSITIONS	32
#define	SHOW_TITLES		64
#define	SHOW_JUSTAUDIOTRACKS	128
#define	SHOW_MAX		255

/* Endianess */
#define	GUESS			(-2)
#define	NONE			(-1)
#define	LITTLE			0
#define	BIG			1

extern	void		FatalError	__PR((int err, const char *szMessage, ...));
extern	void		AnalyzeQchannel	__PR((unsigned frame));
extern	long		SamplesNeeded	__PR((long amount, long undersampling));
extern	unsigned int	get_current_track_writing __PR((void));

#if defined(sun) && !defined(SVR4)
#define	atexit(f)	on_exit(f, 0)
#endif

#ifndef	_LINUX_CDROM_H
#define	_LINUX_CDROM_H

/*
 * some fix numbers
 */
#define	CD_MINS		74	/* max. minutes per CD, not really a limit */
#define	CD_SECS		60	/* seconds per minute */
#define	CD_FRAMES	75	/* frames per second */

#define	CD_SYNC_SIZE	12	/* 12 sync bytes per raw data frame, not transfered by the drive */
#define	CD_HEAD_SIZE	4	/* header (address) bytes per raw data frame */
#define	CD_SUBHEAD_SIZE	8	/* subheader bytes per raw XA data frame */
#define	CD_XA_HEAD	(CD_HEAD_SIZE+CD_SUBHEAD_SIZE) /* "before data" part of raw XA frame */
#define	CD_XA_SYNC_HEAD	(CD_SYNC_SIZE+CD_XA_HEAD) /* sync bytes + header of XA frame */

#define	CD_FRAMESIZE	2048	/* bytes per frame, "cooked" mode */
#define	CD_FRAMESIZE_RAW 2352	/* bytes per frame, "raw" mode */
/* most drives don't deliver everything: */
#define	CD_FRAMESIZE_RAW1 (CD_FRAMESIZE_RAW-CD_SYNC_SIZE) /* 2340 */
#define	CD_FRAMESIZE_RAW0 (CD_FRAMESIZE_RAW-CD_SYNC_SIZE-CD_HEAD_SIZE) /* 2336 */
/* Optics drive also has a 'read all' mode: */
#define	CD_FRAMESIZE_RAWER 2646 /* bytes per frame */

#define	CD_EDC_SIZE	4	/* bytes EDC per most raw data frame types */
#define	CD_ZERO_SIZE	8	/* bytes zero per yellow book mode 1 frame */
#define	CD_ECC_SIZE	276	/* bytes ECC per most raw data frame types */
#define	CD_XA_TAIL	(CD_EDC_SIZE+CD_ECC_SIZE) /* "after data" part of raw XA frame */

#define	CD_FRAMESIZE_SUB 96	/* subchannel data "frame" size */
#define	CD_MSF_OFFSET	150	/* MSF numbering offset of first frame */

#define	CD_CHUNK_SIZE	24	/* lowest-level "data bytes piece" */
#define	CD_NUM_OF_CHUNKS 98 	/* chunks per frame */

#define	CD_FRAMESIZE_XA CD_FRAMESIZE_RAW1 /* obsolete name */
#define	CD_BLOCK_OFFSET    CD_MSF_OFFSET /* obsolete name */

/*
 * the raw frame layout:
 *
 * - audio (red):                  | audio_sample_bytes |
 *                                 |        2352        |
 *
 * - data (yellow, mode1):         | sync - head - data - EDC - zero - ECC |
 *                                 |  12  -   4  - 2048 -  4  -   8  - 276 |
 *
 * - data (yellow, mode2):         | sync - head - data |
 *                                 |  12  -   4  - 2336 |
 *
 * - XA data (green, mode2 form1): | sync - head - sub - data - EDC - ECC |
 *                                 |  12  -   4  -  8  - 2048 -  4  - 276 |
 *
 * - XA data (green, mode2 form2): | sync - head - sub - data - EDC |
 *                                 |  12  -   4  -  8  - 2324 -  4  |
 */


/*
 * CD-ROM address types (cdrom_tocentry.cdte_format)
 */
#if	!defined CDROM_LBA
#define	CDROM_LBA 0x01 /* "logical block": first frame is #0 */
#define	CDROM_MSF 0x02 /* "minute-second-frame": binary, not bcd here! */
#endif
/*
 * bit to tell whether track is data or audio (cdrom_tocentry.cdte_ctrl)
 */
#define	CDROM_DATA_TRACK	0x04

/*
 * The leadout track is always 0xAA, regardless of # of tracks on disc
 */
#define	CDROM_LEADOUT	0xAA

/*
 * audio states (from SCSI-2, but seen with other drives, too)
 */
#define	CDROM_AUDIO_INVALID	0x00	/* audio status not supported */
#define	CDROM_AUDIO_PLAY	0x11	/* audio play operation in progress */
#define	CDROM_AUDIO_PAUSED	0x12	/* audio play operation paused */
#define	CDROM_AUDIO_COMPLETED	0x13	/* audio play successfully completed */
#define	CDROM_AUDIO_ERROR	0x14	/* audio play stopped due to error */
#define	CDROM_AUDIO_NO_STATUS	0x15	/* no current audio status to return */

#ifdef FIVETWELVE
#define	CDROM_MODE1_SIZE	512
#else
#define	CDROM_MODE1_SIZE	2048
#endif

#define	CDROM_MODE2_SIZE	2336

#endif	/* _LINUX_CDROM_H */
