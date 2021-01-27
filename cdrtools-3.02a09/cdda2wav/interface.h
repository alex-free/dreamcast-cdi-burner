/* @(#)interface.h	1.27 13/12/26 Copyright 1998-2001 Heiko Eissfeldt, Copyright 2005-2013 J. Schilling */

/*
 * Copyright (C) by Heiko Eissfeldt
 * Copyright (c) 2005-2013 J. Schilling
 *
 * header file interface.h for cdda2wav
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

#ifndef	DEF_BUFSIZE
#define	DEF_BUFSIZE		(3*1024*1024)	/* Max def. SCSI Buf size 3M */
#endif

#ifndef	CD_FRAMESIZE
#define	CD_FRAMESIZE		2048
#endif

#ifndef	CD_FRAMESIZE_RAW
#define	CD_FRAMESIZE_RAW	2352
#endif

#define	CD_FRAMESAMPLES		(CD_FRAMESIZE_RAW / 4)

/*
 * NOTE: windows.h defines interface as an alias for struct, this
 *	 is used by COM/OLE2, I guess it is class on C++
 *	 We man need to #undef 'interface'
 */
#undef interface

extern unsigned interface;

extern int trackindex_disp;
#ifndef	NSECTORS
#define	NSECTORS		75
#endif

/* interface types */
#define	GENERIC_SCSI		0
#define	COOKED_IOCTL		1

/* constants for sub-q-channel info */
#define	GET_ALL			0
#define	GET_POSITIONDATA	1
#define	GET_CATALOGNUMBER	2
#define	GET_TRACK_ISRC		3

typedef struct subq_chnl {
    unsigned char reserved;
    unsigned char audio_status;
    unsigned short subq_length;
    unsigned char format;
    unsigned char control_adr;
    unsigned char track;
    unsigned char index;
    unsigned char data[40];	/* this has subq_all, subq_position,	  */
				/* subq_catalog or subq_track_isrc format */
} subq_chnl;

typedef struct subq_all {
    unsigned char abs_min;
    unsigned char abs_sec;
    unsigned char abs_frame;
    unsigned char abs_reserved;
    unsigned char trel_min;
    unsigned char trel_sec;
    unsigned char trel_frame;
    unsigned char trel_reserved;
    unsigned char mc_valid;	/* MSB */
    unsigned char media_catalog_number[13];
    unsigned char zero;
    unsigned char aframe;
    unsigned char tc_valid;	/* MSB */
    unsigned char track_ISRC[15];
} subq_all;

typedef struct subq_position {
    unsigned char abs_reserved;
    unsigned char abs_min;
    unsigned char abs_sec;
    unsigned char abs_frame;
    unsigned char trel_reserved;
    unsigned char trel_min;
    unsigned char trel_sec;
    unsigned char trel_frame;
} subq_position;

typedef struct subq_catalog {
    unsigned char mc_valid;	/* MSB */
    unsigned char media_catalog_number[13];
    unsigned char zero;
    unsigned char aframe;
} subq_catalog;

typedef struct subq_track_isrc {
    unsigned char tc_valid;	/* MSB */
    unsigned char track_isrc[15];
} subq_track_isrc;

#if	!defined	NO_SCSI_STUFF

/* cdrom access function pointer */
extern	void	(*EnableCdda)	__PR((SCSI *scgp, int Switch,
						unsigned uSectorsize));
extern	unsigned (*doReadToc)	__PR((SCSI *scgp));
extern	void	(*ReadTocText)	__PR((SCSI *scgp));
extern	unsigned (*ReadLastAudio) __PR((SCSI *scgp));
extern	int	(*ReadCdRom)	__PR((SCSI *scgp, UINT4 *p, unsigned lSector,
						unsigned SectorBurstVal));
extern	int	(*ReadCdRom_C2)	__PR((SCSI *scgp, UINT4 *p, unsigned lSector,
						unsigned SectorBurstVal));
extern	int	(*ReadCdRomSub)	__PR((SCSI *scgp, UINT4 *p, unsigned lSector,
						unsigned SectorBurstVal));
extern	int	(*ReadCdRomData) __PR((SCSI *scgp, unsigned char *p,
						unsigned lSector,
						unsigned SectorBurstVal));
extern	subq_chnl *(*ReadSubQ)	__PR((SCSI *scgp, unsigned char sq_format,
						unsigned char track));
extern	subq_chnl *(*ReadSubChannels) __PR((SCSI *scgp, unsigned lSector));
extern	void	(*SelectSpeed)	__PR((SCSI *scgp, unsigned speed));
extern	int	(*Play_at)	__PR((SCSI *scgp, unsigned from_sector,
						unsigned sectors));
extern	int	(*StopPlay)	__PR((SCSI *scgp));
extern	void	(*trash_cache)	__PR((UINT4 *p, unsigned lSector,
						unsigned SectorBurstVal));

extern	SCSI	*get_scsi_p	__PR((void));
#endif

extern	unsigned char	*bufferTOC;
extern	int		bufTOCsize;
extern	subq_chnl	*SubQbuffer;


extern	void	SetupInterface	__PR((void));
extern	int	Toshiba3401	__PR((void));

extern int	poll_in			__PR((void));

/*
 * The callback interface for libparanoia to the CD-ROM interface
 *
 * cdda_read() is in interface.c the other functions are in toc.c
 */
extern long	cdda_disc_firstsector	__PR((void *d));		/* -> long sector */
extern long	cdda_disc_lastsector	__PR((void *d));		/* -> long sector */
extern long	cdda_read		__PR((void *d, void *buffer, long beginsector, long sectors));	/* -> long sectors */
extern long	cdda_read_c2		__PR((void *d, void *buffer, long beginsector, long sectors));	/* -> long sectors */
extern int	cdda_sector_gettrack	__PR((void *d, long sector));	/* -> int trackno */
extern int	cdda_track_audiop	__PR((void *d, int track));	/* -> int Is audiotrack */
extern long	cdda_track_firstsector	__PR((void *d, int track));	/* -> long sector */
extern long	cdda_track_lastsector	__PR((void *d, int track));	/* -> long sector */
extern int	cdda_tracks		__PR((void *d));		/* -> int tracks */
