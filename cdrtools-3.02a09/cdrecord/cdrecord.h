/* @(#)cdrecord.h	1.205 13/05/28 Copyright 1995-2013 J. Schilling */
/*
 *	Definitions for cdrecord
 *
 *	Copyright (c) 1995-2013 J. Schilling
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

/*
 * Make sure it is there. We need it for off_t.
 */
#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>
#endif

#ifndef	_SCHILY_UTYPES_H
#include <schily/utypes.h>
#endif

/*
 * Defines for command line option flags
 */
#define	F_DUMMY		0x00000001L	/* Do dummy (simulation) writes */
#define	F_TOC		0x00000002L	/* Get TOC */
#define	F_EJECT		0x00000004L	/* Eject disk after doing the work */
#define	F_LOAD		0x00000008L	/* Load disk only */
#define	F_MULTI		0x00000010L	/* Create linkable TOC/multi-session */
#define	F_MSINFO	0x00000020L	/* Get multi-session info */
#define	F_FIX		0x00000040L	/* Fixate disk only */
#define	F_NOFIX		0x00000080L	/* Do not fixate disk */
#define	F_VERSION	0x00000100L	/* Print version info */
#define	F_CHECKDRIVE	0x00000200L	/* Check for driver */
#define	F_INQUIRY	0x00000400L	/* Do inquiry */
#define	F_PRCAP		0x00000800L	/* Print capabilities */
#define	F_SCANBUS	0x00001000L	/* Scan SCSI Bus */
#define	F_RESET		0x00002000L	/* Reset SCSI Bus */
#define	F_BLANK		0x00004000L	/* Blank CD-RW */
#define	F_PRATIP	0x00008000L	/* Print ATIP info */
#define	F_PRDINFO	0x00010000L	/* Print disk info (XXX not yet used)  */
#define	F_IGNSIZE	0x00020000L	/* Ignore disk size */
#define	F_SAO		0x00040000L	/* Session at once */
#define	F_RAW		0x00080000L	/* Raw mode */
#define	F_WRITE		0x00100000L	/* Disk is going to be written */
#define	F_FORCE		0x00200000L	/* Force things (e.g. blank on dead disk)  */
#define	F_WAITI		0x00400000L	/* Wait until data is available on stdin */
#define	F_OVERBURN	0x00800000L	/* Allow oveburning */
#define	F_CLONE		0x01000000L	/* Do clone writing */
#define	F_STDIN		0x02000000L	/* We are using stdin as CD data */
#define	F_IMMED		0x04000000L	/* Try tu use IMMED SCSI flag if possible */
#define	F_DLCK		0x08000000L	/* Load disk and lock door */
#define	F_SETDROPTS	0x10000000L	/* Set driver opts and exit */
#define	F_FORMAT	0x20000000L	/* Format media */
#define	F_ABORT		0x40000000L	/* Send abort sequence to drive */
#ifdef	PROTOTYPES
#define	F_MEDIAINFO	0x80000000UL	/* Print media info */
#else
#define	F_MEDIAINFO	0x80000000L	/* Print media info */
#endif

/*
 * Defines for command extended line option flags2
 */
#define	F2_XX		0x01		/* Not yet		*/

#ifdef	min
#undef	min
#endif
#define	min(a, b)	((a) < (b) ? (a):(b))

#ifdef	max
#undef	max
#endif
#define	max(a, b)	((a) < (b) ? (b):(a))

#undef	roundup
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))

/*
 * NOTICE:	You should not make CDR_BUF_SIZE more than
 *		the buffer size of the CD-Recorder.
 *
 * The Philips CDD 521 is the recorder with the smallest buffer.
 * It only has 256kB of buffer RAM.
 *
 * WARNING:	Philips CDD 521 dies if CDR_BUF_SIZE is to big.
 *		If you like to support the CDD 521 keep the old buffer
 *		size at 63kB.
 */
/*#define	CDR_BUF_SIZE		(126*1024)*/
#define	CDR_BUF_SIZE		(63*1024)
#define	CDR_OLD_BUF_SIZE	(63*1024)
#define	CDR_MAX_BUF_SIZE	(256*1024)

/*
 * Never set MIN_GRACE_TIME < 3 seconds. We need to give
 * the volume management a chance to settle before we
 * start writing.
 */
#ifndef	MIN_GRACE_TIME
#define	MIN_GRACE_TIME	3		/* 3 seconds */
#endif
#ifndef	GRACE_TIME
#define	GRACE_TIME	9		/* 9 seconds */
#endif

/*
 * Some sector sizes used for reading/writing ...
 */
#define	DATA_SEC_SIZE	2048		/* 2048 bytes */
#define	MODE2_SEC_SIZE	2336		/* 2336 bytes */
#define	AUDIO_SEC_SIZE	2352		/* 2352 bytes */
#define	RAW16_SEC_SIZE	(2352+16)	/* 2368 bytes */
#define	RAW96_SEC_SIZE	(2352+96)	/* 2448 bytes */

#define	MAX_TRACK	99	/* Red Book track limit */

#define	PAD_SECS	15	/* NOTE: pad must be less than BUF_SIZE */
#define	PAD_SIZE	(PAD_SECS * DATA_SEC_SIZE)

/*
 * FIFO size must be at least 2x CDR_MAX_BUF_SIZE
 */
#define	DEFAULT_FIFOSIZE (4L*1024L*1024L)

#if	!defined(HAVE_LARGEFILES) && SIZEOF_LLONG > SIZEOF_LONG
typedef	Llong	tsize_t;
#else
typedef	off_t	tsize_t;
#endif

#ifdef	nono
typedef struct tindex {
	int	dummy;		/* Not yet implemented */
} tindex_t;
#endif

typedef struct ofile {
	struct ofile *next;	/* Next open file			*/
	int	f;		/* Open file				*/
	char	*filename;	/* File name				*/
	int	refcnt;		/* open reference count			*/
} ofile_t;

typedef struct track {
	void	*xfp;		/* Open file for this track from xopen()*/
	char	*filename;	/* File name for this track		*/

	tsize_t	itracksize;	/* Size of track bytes (-1 == until EOF)*/
				/* This is in units of isecsize		*/
	tsize_t	tracksize;	/* Size of track bytes (-1 == until EOF)*/
				/* This is in units of secsize		*/

	long	trackstart;	/* Start sector # of this track		*/
	long	tracksecs;	/* Size of this track (sectors)		*/
	long	padsecs;	/* Pad size for this track (sectors)	*/
	long	pregapsize;	/* Pre-gap size for this track (sectors)*/
	long	index0start;	/* Index 0 start within this tr(sectors)*/
	int	isecsize;	/* Read input sector size for this track*/
	int	secsize;	/* Sector size for this track (bytes)	*/
	int	secspt;		/* # of sectors to copy for one transfer*/
	int	pktsize;	/* # of blocks per write packet		*/
	Uchar	dataoff;	/* offset of user data in raw sector	*/
	Uchar	tracks;		/* Number of tracks on this disk	*/
	Uchar	track;		/* Track # as offset in track[] array	*/
	Uchar	trackno;	/* Track # on disk for this track	*/
	Uchar	tracktype;	/* Track type (toc type)		*/
	Uchar	dbtype;		/* Data block type for this track	*/
	int	sectype;	/* Sector type				*/
	int	flags;		/* Flags (see below)			*/
	int	nindex;		/* Number of indices for track		*/
	long	*tindex;	/* Track index descriptor		*/
	char	*isrc;		/* ISRC code for this track / disk MCN	*/
	void	*text;		/* Opaque CD-Text data (txtptr_t *)	*/
} track_t;

#define	track_base(tp)	((tp) - (tp)->track)

/*
 * Defines for tp->flags
 */
#define	TI_AUDIO	0x00001	/* File is an audio track		*/
#define	TI_PREEMP	0x00002	/* Audio track recorded w/preemphasis	*/
#define	TI_MIX		0x00004	/* This is a mixed mode track		*/
#define	TI_RAW		0x00008	/* Write this track in raw mode		*/
#define	TI_PAD		0x00010	/* Pad data track			*/
#define	TI_SWAB		0x00020	/* Swab audio data			*/
#define	TI_ISOSIZE	0x00040	/* Use iso size for track		*/
#define	TI_NOAUHDR	0x00080	/* Don't look for audio header		*/
#define	TI_FIRST	0x00100	/* This is the first track		*/
#define	TI_LAST		0x00200	/* This is the last track		*/
#define	TI_PACKET	0x00400	/* Fixed- or variable-packet track	*/
#define	TI_NOCLOSE	0x00800	/* Don't close the track after writing	*/
#define	TI_TAO		0x01000	/* This track is written in TAO mode	*/
#define	TI_PREGAP	0x02000	/* Prev. track incl. pregap of this tr. */
#define	TI_SCMS		0x04000	/* Force to follow the SCMS rules	*/
#define	TI_COPY		0x08000	/* Allow digital copy			*/
#define	TI_SHORT_TRACK	0x10000	/* Ignore min 4 second Red Book	std.	*/
#define	TI_RAW16	0x20000	/* This track uses 16 bytes subch.	*/
#define	TI_RAW96R	0x40000	/* This track uses 96 bytes RAW subch.	*/
#define	TI_CLONE	0x80000	/* Special clone treatment needed	*/
#define	TI_TEXT		0x100000 /* This track holds CD-Text information */
#define	TI_NOCD		0x200000 /* We are not writing a CD track	*/
#define	TI_SAO		0x400000 /* This track is written in SAO mode	*/
#define	TI_USEINFO	0x800000 /* Use information from *.inf files	*/
#define	TI_QUADRO	0x1000000 /* Four Channel Audio Data		*/
#define	TI_STDIN	0x2000000 /* Track read is from stdin		*/
#define	TI_HIDDEN	0x4000000 /* Track is hidden / has hidden Tr.	*/


#define	is_audio(tp)	(((tp)->flags & TI_AUDIO) != 0)
#define	is_preemp(tp)	(((tp)->flags & TI_PREEMP) != 0)
#define	is_pad(tp)	(((tp)->flags & TI_PAD) != 0)
#define	is_swab(tp)	(((tp)->flags & TI_SWAB) != 0)
#define	is_first(tp)	(((tp)->flags & TI_FIRST) != 0)
#define	is_last(tp)	(((tp)->flags & TI_LAST) != 0)
#define	is_packet(tp)	(((tp)->flags & TI_PACKET) != 0)
#define	is_noclose(tp)	(((tp)->flags & TI_NOCLOSE) != 0)
#define	is_tao(tp)	(((tp)->flags & TI_TAO) != 0)
#define	is_sao(tp)	(((tp)->flags & TI_SAO) != 0)
#define	is_raw(tp)	(((tp)->flags & TI_RAW) != 0)
#define	is_raw16(tp)	(((tp)->flags & TI_RAW16) != 0)
#define	is_raw96(tp)	(((tp)->flags & (TI_RAW|TI_RAW16)) == TI_RAW)
#define	is_raw96p(tp)	(((tp)->flags & (TI_RAW|TI_RAW16|TI_RAW96R)) == TI_RAW)
#define	is_raw96r(tp)	(((tp)->flags & (TI_RAW|TI_RAW16|TI_RAW96R)) == (TI_RAW|TI_RAW96R))
#define	is_pregap(tp)	(((tp)->flags & TI_PREGAP) != 0)
#define	is_scms(tp)	(((tp)->flags & TI_SCMS) != 0)
#define	is_copy(tp)	(((tp)->flags & TI_COPY) != 0)
#define	is_shorttrk(tp)	(((tp)->flags & TI_SHORT_TRACK) != 0)
#define	is_clone(tp)	(((tp)->flags & TI_CLONE) != 0)
#define	is_text(tp)	(((tp)->flags & TI_TEXT) != 0)
#define	is_quadro(tp)	(((tp)->flags & TI_QUADRO) != 0)
#define	is_hidden(tp)	(((tp)->flags & TI_HIDDEN) != 0)

/*
 * Defines for toc type / track type
 */
#define	TOC_DA		0	/* CD-DA				*/
#define	TOC_ROM		1	/* CD-ROM				*/
#define	TOC_XA1		2	/* CD_ROM XA with first track in mode 1 */
#define	TOC_XA2		3	/* CD_ROM XA with first track in mode 2 */
#define	TOC_CDI		4	/* CDI					*/

#define	TOC_MASK	7	/* Mask needed for toctname[]		*/

/*
 * Additional flags in toc type / trackp->tracktype
 * XXX TOCF_DUMMY istr schon in dp->cdr_cmdflags & F_DUMMY
 * XXX TOCF_MULTI istr schon in dp->cdr_cmdflags & F_MULTI
 */
#define	TOCF_DUMMY	0x10	/* Write in dummy (simulation) mode	*/
#define	TOCF_MULTI	0x20	/* Multisession (Open Next Programarea) */

#define	TOCF_MASK	0xF0	/* All possible flags in tracktype	*/

extern	char	*toc2name[];	/* Convert toc type to name		*/
extern	int	toc2sess[];	/* Convert toc type to session format	*/

/*
 * Defines for sector type
 *
 * Mode is 2 bits
 * Aud  is 1 bit
 *
 * Sector is: aud << 2 | mode
 */
#define	ST_ROM_MODE1	1	/* CD-ROM in mode 1 (vanilla cdrom)	*/
#define	ST_ROM_MODE2	2	/* CD-ROM in mode 2			*/
#define	ST_AUDIO_NOPRE	4	/* CD-DA stereo without preemphasis	*/
#define	ST_AUDIO_PRE	5	/* CD-DA stereo with preemphasis	*/

#define	ST_PREEMPMASK	0x01	/* Mask for preemphasis bit		*/
#define	ST_AUDIOMASK	0x04	/* Mask for audio bit			*/
#define	ST_MODEMASK	0x03	/* Mask for mode bits in sector type	*/
#define	ST_MASK		0x07	/* Mask needed for sectname[]		*/

/*
 * There are 6 different generic basic sector types.
 */
#define	ST_MODE_AUDIO	 0x00	/* Generic Audio mode			*/
#define	ST_MODE_0	 0x10	/* Generic Zero mode			*/
#define	ST_MODE_1	 0x20	/* Generic CD-ROM mode	(ISO/IEC 10149)	*/
#define	ST_MODE_2	 0x30	/* Generic Mode 2	(ISO/IEC 10149)	*/
#define	ST_MODE_2_FORM_1 0x40	/* Generic Mode 2 form 1		*/
#define	ST_MODE_2_FORM_2 0x50	/* Generic Mode 2 form 2		*/
#define	ST_MODE_2_MIXED	 0x60	/* Generic Mode 2 mixed form (1/2)	*/

#define	ST_MODE_MASK	 0x70	/* Mask needed to get generic sectype	*/

#define	ST_MODE_RAW	 0x08	/* Do not touch EDC & subchannels	*/
#define	ST_NOSCRAMBLE	 0x80	/* Do not srcamble sectors 		*/

#define	SECT_AUDIO	(ST_AUDIO_NOPRE  | ST_MODE_AUDIO)
#define	SECT_AUDIO_NOPRE (ST_AUDIO_NOPRE | ST_MODE_AUDIO)
#define	SECT_AUDIO_PRE	(ST_AUDIO_PRE    | ST_MODE_AUDIO)
#define	SECT_MODE_0	(ST_ROM_MODE1    | ST_MODE_0)
#define	SECT_ROM	(ST_ROM_MODE1    | ST_MODE_1)
#define	SECT_MODE_2	(ST_ROM_MODE2    | ST_MODE_2)
#define	SECT_MODE_2_F1	(ST_ROM_MODE2    | ST_MODE_2_FORM_1)
#define	SECT_MODE_2_F2	(ST_ROM_MODE2    | ST_MODE_2_FORM_2)
#define	SECT_MODE_2_MIX	(ST_ROM_MODE2    | ST_MODE_2_MIXED)

extern	char	*st2name[];	/* Convert sector type to name		*/
extern	int	st2mode[];	/* Convert sector type to control nibble*/

/*
 * Control nibble bits:
 *
 * 0	with preemphasis (audio) / incremental (data)
 * 1	digital copy permitted
 * 2	data (not audio) track
 * 3	4 channels (not 2)
 */
#define	TM_PREEM	0x1	/* Audio track with preemphasis	*/
#define	TM_INCREMENTAL	0x1	/* Incremental data track	*/
#define	TM_ALLOW_COPY	0x2	/* Digital copy permitted	*/
#define	TM_DATA		0x4	/* This is a data track		*/
#define	TM_QUADRO	0x8	/* Four channel audio		*/

/*
 * Adr nibble:
 */
#define	ADR_NONE	0	/* Sub-Q mode info not supplied		*/
#define	ADR_POS		1	/* Sub-Q encodes position data		*/
#define	ADR_MCN		2	/* Sub-Q encodes Media Catalog Number	*/
#define	ADR_ISRC	3	/* Sub-Q encodes ISRC			*/

/*
 * Defines for write type (from SCSI-3/mmc)
 */
#define	WT_PACKET	0x0	/* Packet writing	*/
#define	WT_TAO		0x1	/* Track at once	*/
#define	WT_SAO		0x2	/* Session at once	*/
#define	WT_RAW		0x3	/* Raw			*/
#define	WT_LAYER_JUMP	0x4	/* DVD-R/DL Layer jump	*/
#define	WT_RES_5	0x5	/* Reserved		*/
#define	WT_RES_6	0x6	/* Reserved		*/
#define	WT_RES_7	0x7	/* Reserved		*/
#define	WT_RES_8	0x8	/* Reserved		*/
#define	WT_RES_9	0x9	/* Reserved		*/
#define	WT_RES_A	0xA	/* Reserved		*/
#define	WT_RES_B	0xB	/* Reserved		*/
#define	WT_RES_C	0xC	/* Reserved		*/
#define	WT_RES_D	0xD	/* Reserved		*/
#define	WT_RES_E	0xE	/* Reserved		*/
#define	WT_RES_F	0xF	/* Reserved		*/

/*
 * Data block layout:
 *
 *	-	Sync pattern 12 Bytes:	0x00 0xFF 0xFF ... 0xFF 0xFF 0x00
 *	-	Block header 4  Bytes:	| minute | second | frame | mode |
 *		Mode byte:
 *			Bits 7, 6, 5	Run-in/Run-out/Link
 *			Bits 4, 3, 2	Reserved
 *			Bits 1, 0	Mode
 *	-	Rest of sector see below.
 *
 * Mode 0 Format:
 *	0	12   Bytes Sync header
 *	12	4    Bytes Block header with Data mode == 0
 *	16	2336 Bytes of zero data
 *
 * Mode 1 Format:
 *	0	12   Bytes Sync header
 *	12	4    Bytes Block header with Data mode == 1
 *	16	2048 Bytes of user data
 *	2064	4    Bytes CRC for Bytes 0-2063
 *	2068	8    Bytes Zero fill
 *	2076	172  Bytes P parity symbols
 *	2248	104  Bytes Q parity symbols
 *
 * Mode 2 Format (formless):
 *	0	12   Bytes Sync header
 *	12	4    Bytes Block header with Data mode == 2
 *	16	2336 Bytes of user data
 *
 * Mode 2 form 1 Format:
 *	0	12   Bytes Sync header
 *	12	4    Bytes Block header with Data mode == 2
 *	16	4    Bytes subheader first copy
 *	20	4    Bytes subheader second copy
 *	24	2048 Bytes of user data
 *	2072	4    Bytes CRC for Bytes 16-2071
 *	2076	172  Bytes P parity symbols
 *	2248	104  Bytes Q parity symbols
 *
 * Mode 2 form 2 Format:
 *	0	12   Bytes Sync header
 *	12	4    Bytes Block header with Data mode == 2
 *	16	4    Bytes subheader first copy
 *	20	4    Bytes subheader second copy
 *	24	2324 Bytes of user data
 *	2348	4    Bytes Optional CRC for Bytes 16-2347
 */

/*
 * Mode Byte definitions (the 4th Byte in the Block header)
 */
#define	SH_MODE_DATA	0x00	/* User Data Block	*/
#define	SH_MODE_RI4	0x20	/* Fourth run in Block	*/
#define	SH_MODE_RI3	0x40	/* Third run in Block	*/
#define	SH_MODE_RI2	0x60	/* Second run in Block	*/
#define	SH_MODE_RI1	0x80	/* First run in Block	*/
#define	SH_MODE_LINK	0xA0	/* Link Block		*/
#define	SH_MODE_RO2	0xC0	/* Second run out Block	*/
#define	SH_MODE_RO1	0xE0	/* First run out Block	*/
#define	SH_MODE_M0	0x00	/* Mode 0 Data		*/
#define	SH_MODE_M1	0x01	/* Mode 1 Data		*/
#define	SH_MODE_M2	0x02	/* Mode 2 Data		*/
#define	SH_MODE_MR	0x03	/* Reserved		*/

/*
 * Defines for data block type (from SCSI-3/mmc)
 *
 * Mandatory are only:
 *	DB_ROM_MODE1	(8)	Mode 1     (ISO/IEC 10149)
 *	DB_XA_MODE2	(10)	Mode 2-F1  (CD-ROM XA form 1)
 *	DB_XA_MODE2_MIX	(13)	Mode 2-MIX (CD-ROM XA 1/2+subhdr)
 */
#define	DB_RAW		0	/* 2352 bytes of raw data		  */
#define	DB_RAW_PQ	1	/* 2368 bytes (raw data + P/Q Subchannel) */
#define	DB_RAW_PW	2	/* 2448 bytes (raw data + P-W Subchannel) */
#define	DB_RAW_PW_R	3	/* 2448 bytes (raw data + P-W raw Subchannel)*/
#define	DB_RES_4	4	/* -	Reserved			  */
#define	DB_RES_5	5	/* -	Reserved			  */
#define	DB_RES_6	6	/* -	Reserved			  */
#define	DB_VU_7		7	/* -	Vendor specific			  */
#define	DB_ROM_MODE1	8	/* 2048 bytes Mode 1 (ISO/IEC 10149)	  */
#define	DB_ROM_MODE2	9	/* 2336 bytes Mode 2 (ISO/IEC 10149)	  */
#define	DB_XA_MODE2	10	/* 2048 bytes Mode 2 (CD-ROM XA form 1)   */
#define	DB_XA_MODE2_F1	11	/* 2056 bytes Mode 2 (CD-ROM XA form 1)	  */
#define	DB_XA_MODE2_F2	12	/* 2324 bytes Mode 2 (CD-ROM XA form 2)	  */
#define	DB_XA_MODE2_MIX	13	/* 2332 bytes Mode 2 (CD-ROM XA 1/2+subhdr) */
#define	DB_RES_14	14	/* -	Reserved			  */
#define	DB_VU_15	15	/* -	Vendor specific			  */

extern	char	*db2name[];	/* Convert data block type to name	  */

/*
 * Defines for multi session type (from SCSI-3/mmc)
 */
#define	MS_NONE		0	/* No B0 pointer. Next session not allowed*/
#define	MS_FINAL	1	/* B0 = FF:FF:FF. Next session not allowed*/
#define	MS_RES		2	/* Reserved				  */
#define	MS_MULTI	3	/* B0 = Next PA.  Next session allowed	  */

/*
 * Defines for session format (from SCSI-3/mmc)
 */
#define	SES_DA_ROM	0x00	/* CD-DA or CD-ROM disk			  */
#define	SES_CDI		0x10	/* CD-I disk				  */
#define	SES_XA		0x20	/* CD-ROM XA disk			  */
#define	SES_UNDEF	0xFF	/* Undefined disk type (read disk info)	  */

/*
 * Defines for blanking of CD-RW discs (from SCSI-3/mmc)
 */
#define	BLANK_DISC	0x00	/* Erase the entire disc		  */
#define	BLANK_MINIMAL	0x01	/* Erase the PMA, 1st session TOC, pregap */
#define	BLANK_TRACK	0x02	/* Erase an incomplete track		  */
#define	BLANK_UNRESERVE	0x03	/* Unreserve a track			  */
#define	BLANK_TAIL	0x04	/* Erase the tail of a track		  */
#define	BLANK_UNCLOSE	0x05	/* Unclose the last session		  */
#define	BLANK_SESSION	0x06	/* Erase the last session		  */

/*
 * Useful definitions for audio tracks
 */
#define	msample		(44100 * 2)		/* one 16bit audio sample */
#define	ssample		(msample * 2)		/* one stereo sample	*/
#define	samples(v)	((v) / ssample)		/* # of stereo samples	*/
#define	hsamples(v)	((v) / (ssample/100))	/* 100* # of stereo samples/s*/
#define	fsamples(v)	((v) / (ssample/75))	/* 75* # of stereo samples/s */

#define	minutes(v)	((int)(samples(v) / 60))
#define	seconds(v)	((int)(samples(v) % 60))
#define	hseconds(v)	((int)(hsamples(v) % 100))
#define	frames(v)	((int)(fsamples(v) % 75))

/*
 * sector based macros
 */
#define	Sminutes(s)	((int)((s) / (60*75)))
#define	Sseconds(s)	((int)((s) / 75))
#define	Shseconds(s)	((int)(((s) % 75)*100)/75)
#define	Sframes(s)	((int)((s) % 75))

typedef struct msf {
	char	msf_min;
	char	msf_sec;
	char	msf_frame;
} msf_t;

/*
 * Definitions for read TOC/PMA/ATIP command
 */
#define	FMT_TOC		0
#define	FMT_SINFO	1
#define	FMT_FULLTOC	2
#define	FMT_PMA		3
#define	FMT_ATIP	4
#define	FMT_CDTEXT	5

/*
 * Definitions for read disk information "recording flags"
 * used in UInt16_t "ds_cdrflags".
 */
#define	RF_WRITE	0x0001	/* Disk is going to be written		*/
#define	RF_BLANK	0x0002	/* Disk is going to be erased		*/
#define	RF_PRATIP	0x0004	/* Print ATIP info			*/
#define	RF_LEADIN	0x0008	/* Lead-in has been "manually" written	*/
#define	RF_BURNFREE	0x0010	/* BUFFER underrun free recording	*/
#define	RF_VARIREC	0x0020	/* Plextor VariRec			*/
#define	RF_AUDIOMASTER	0x0040	/* Yamaha AudioMaster			*/
#define	RF_FORCESPEED	0x0080	/* WriteSpeed forced high		*/
#define	RF_DID_STAT	0x0100	/* Already did call cdrstats()		*/
#define	RF_DID_CDRSTAT	0x0200	/* Already did call (*dp->cdr_stats)()	*/
#define	RF_WR_WAIT	0x0400	/* Wait during writing to free bus	*/
#define	RF_SINGLESESS	0x0800	/* Plextor single sess. mode		*/
#define	RF_HIDE_CDR	0x1000	/* Plextor hide CDR features		*/
#define	RF_SPEEDREAD	0x2000	/* Plextor SpeedReed			*/
#define	RF_GIGAREC	0x4000	/* Plextor GigaRec			*/

/*
 * Definitions for read disk information "disk status"
 * used in "ds_diskstat".
 */
#define	DS_EMPTY	0	/* Empty disk				*/
#define	DS_APPENDABLE	1	/* Incomplete disk (appendable)		*/
#define	DS_COMPLETE	2	/* Complete disk (closed/no B0 pointer)	*/
#define	DS_RESERVED	3	/* Reserved				*/

/*
 * Definitions for read disk information "session status"
 * used in "ds_sessstat".
 */
#define	SS_EMPTY	0	/* Empty session			*/
#define	SS_APPENDABLE	1	/* Incomplete session			*/
#define	SS_RESERVED	2	/* Reserved				*/
#define	SS_COMPLETE	3	/* Complete session (needs DS_COMPLETE)	*/

/*
 * Definitions for disk_status write mode
 * used in "ds_wrmode".
 */
#define	WM_NONE		0	/* No write mode selected		*/
#define	WM_BLANK	1	/* Blanking mode			*/
#define	WM_FORMAT	2	/* Formatting				*/
#define	WM_PACKET	4	/* Packet writing			*/
#define	WM_TAO		8	/* Track at Once			*/
#define	WM_SAO		12	/* Session at Once w/ cooked sectors	*/
#define	WM_SAO_RAW16	13	/* Session at Once RAW+16 byte sectors	*/
#define	WM_SAO_RAW96P	14	/* Session at Once RAW+96P byte sectors	*/
#define	WM_SAO_RAW96R	15	/* Session at Once RAW+96R byte sectors	*/
#define	WM_RAW		16	/* RAW with cooked sectors is impossible*/
#define	WM_RAW_RAW16	17	/* RAW with RAW+16 byte sectors		*/
#define	WM_RAW_RAW96P	18	/* RAW with RAW+96P byte sectors	*/
#define	WM_RAW_RAW96R	19	/* RAW with RAW+96R byte sectors	*/

#define	wm_base(wm)	((wm)/4*4) /* The basic write mode for this mode */

/*
 * Definitions for disk_status flags
 * used in UInt16_t "ds_flags".
 */
#define	DSF_DID_V	0x0001	/* Disk id valid			*/
#define	DSF_DBC_V	0x0002	/* Disk bar code valid			*/
#define	DSF_URU		0x0004	/* Disk is for unrestricted use		*/
#define	DSF_ERA		0x0008	/* Disk is erasable			*/
#define	DSF_HIGHSP_ERA	0x0010	/* Disk is high speed erasable		*/
#define	DSF_ULTRASP_ERA	0x0020	/* Disk is ultra speed erasable		*/
#define	DSF_ULTRASPP_ERA 0x0040	/* Disk is ultra speed+ erasable	*/
#define	DSF_NOCD	0x0080	/* Disk is not a CD			*/

#define	DSF_DVD		0x0100	/* Disk is a DVD			*/
#define	DSF_DVD_PLUS_R	0x0200	/* Disk is a DVD+R			*/
#define	DSF_DVD_PLUS_RW	0x0400	/* Disk is a DVD+RW			*/
#define	DSF_NEED_FORMAT	0x0800	/* Disk needs to be formatted		*/
#define	DSF_BD		0x1000	/* Disk is a BD				*/
#define	DSF_BD_RE	0x2000	/* Disk is a BD-RE			*/

/*
 * Definitions for disk_status disk type
 * used in "ds_type".
 */
#define	DST_UNKNOWN	0
#define	DST_CD_ROM	0x08
#define	DST_CD_R	0x09
#define	DST_CD_RW	0x0A
#define	DST_DVD_ROM	0x10
#define	DST_DVD_R	0x11
#define	DST_DVD_RAM	0x12
#define	DST_DVD_RW	0x13
#define	DST_DVD_RW_SEQ	0x14
#define	DST_DVD_R_DL	0x15
#define	DST_DVD_R_DL_LJ	0x16
#define	DST_DVD_PLUS_RW	0x1A
#define	DST_DVD_PLUS_R	0x1B
#define	DST_DVD_PLUS_R_DL 0x2B
#define	DST_BD_ROM	0x40
#define	DST_BD_R_SEQ	0x41
#define	DST_BD_R	0x42
#define	DST_BD_RE	0x43

typedef	struct disk_status	dstat_t;

struct disk_status {
	track_t	*ds_trackp;		/* Pointer to track structure	*/
	UInt32_t ds_diskid;		/* Disk identification		*/
	UInt16_t ds_cdrflags;		/* Recording flags from cdrecord*/
	UInt16_t ds_flags;		/* Disk_status flags		*/
	Uchar	 ds_wrmode;		/* Selected write mode		*/
	Uchar	 ds_type;		/* Abstract disk type		*/

	Uchar	 ds_disktype;		/* Disk type (from TOC/PMA)	*/
	Uchar	 ds_diskstat;		/* Disk status (MMC)		*/
	Uchar	 ds_sessstat;		/* Status of last sesion (MMC)	*/
	Uchar	 ds_trfirst;		/* first track #		*/
	Uchar	 ds_trlast;		/* last track #			*/
	Uchar	 ds_trfirst_ls;		/* first track # in last session*/
	Uchar	 ds_barcode[8];		/* Disk bar code		*/

	Int32_t	 ds_first_leadin;	/* Start of first lead in (ATIP)*/
	Int32_t	 ds_last_leadout;	/* Start of last lead out (ATIP)*/
	Int32_t	 ds_curr_leadin;	/* Start of next lead in	*/
	Int32_t	 ds_curr_leadout;	/* Start of next lead out	*/

	Int32_t	 ds_maxblocks;		/* # of official blocks on disk	*/
	Int32_t	 ds_maxrblocks;		/* # real blocks on disk	*/
	Int32_t	 ds_fwa;		/* first writable addr		*/

	Int32_t	 ds_startsec;		/* Actual start sector		*/
	Int32_t	 ds_endsec;		/* Actual end sector		*/
	Int32_t	 ds_buflow;		/* # of times drive buffer empty*/

	UInt16_t ds_minbuf;		/* Minimum drive bufer fill rt.	*/

	UInt16_t ds_at_min_speed;	/* The minimal ATIP write speed	*/
	UInt16_t ds_at_max_speed;	/* The maximal ATIP write speed	*/
	UInt16_t ds_dr_cur_rspeed;	/* The drive's cur read speed	*/
	UInt16_t ds_dr_max_rspeed;	/* The drive's max read speed	*/
	UInt16_t ds_dr_cur_wspeed;	/* The drive's cur write speed	*/
	UInt16_t ds_dr_max_wspeed;	/* The drive's max write speed	*/
	UInt16_t ds_wspeed;		/* The selected/drive wr. speed */

	Int32_t	 ds_layer_break;	/* Start of layer break		*/
};

/*
 * First approach of a CDR device abstraction layer.
 * This interface will change as long as I did not find the
 * optimum that fits for all devices.
 *
 * Called with pointer to whole track array:
 *	cdr_send_cue()
 *	cdr_write_leadin()
 *	cdr_open_session()
 *	cdr_fixate()
 *
 * Called with (track_t *) 0 or pointer to current track:
 *	cdr_next_wr_address()
 *
 * Called with pointer to current track:
 *	cdr_open_track()
 *	cdr_close_track()
 *
 * Calling sequence:
 *	cdr_identify()					May modify driver
 *	Here, the cdr_t will be allocated and
 *	copied to a new writable area.
 *	cdr_attach()					Get drive properties
 *	cdr_buffer_cap()
 *	cdr_getdisktype()				GET ATIP
 *	cdr_init()					set TAO for -msinfo
 *	cdr_check_session				XXX ????
 *	cdr_opt1()					set early options
 *	cdr_set_speed_dummy(scgp, dp, &speed)
 *	<---	Grace time processing goes here
 *	{ do_opc(); cdr_blank() }
 *	cdr_opt2()					set late options
 *	cdr_open_session()				set up params (no wrt.)
 *	do_opc()
 *	cdr_write_leadin()				start writing
 *	LOOP {
 *		cdr_open_track()
 *		cdr_next_wr_address()			only TAO / Packet
 *		write_track_data()
 *		cdr_close_track()
 *	}
 *	write_leadout()				XXX should go -> driver!
 *	cdr_fixate()
 *	cdr_stats()
 */
/*--------------------------------------------------------------------------*/
typedef	struct cdr_cmd	cdr_t;

#ifdef	_SCG_SCSITRANSP_H
struct cdr_cmd {
	int	cdr_dev;			/* Numerical device type */
	UInt32_t cdr_cmdflags;			/* Command line options */
	UInt32_t cdr_cmdflags2;			/* More cmdline options */
	UInt32_t cdr_flags;			/* Drive related flags	*/
	UInt32_t cdr_flags2;			/* More Drive flags	*/
	UInt8_t	 cdr_cdrw_support;		/* CD-RW write media types */
	UInt8_t	 cdr_wrmodedef;			/* Default write mode	*/
	UInt16_t cdr_speeddef;			/* Default write speed	*/
	UInt16_t cdr_speedmax;			/* Max. write speed	*/

	char	*cdr_drname;			/* Driver ID string	*/
	char	*cdr_drtext;			/* Driver ID text	*/
	struct cd_mode_page_2A *cdr_cdcap;
	dstat_t	*cdr_dstat;
#ifdef	_SCG_SCSIREG_H
	cdr_t	*(*cdr_identify)	__PR((SCSI *scgp, cdr_t *, struct scsi_inquiry *));	/* identify drive */
#else
	cdr_t	*(*cdr_identify)	__PR((SCSI *scgp, cdr_t *, void *));		/* identify drive */
#endif
	int	(*cdr_attach)		__PR((SCSI *scgp, cdr_t *));			/* init error decoding etc*/
	int	(*cdr_init)		__PR((SCSI *scgp, cdr_t *));			/* init drive to useful deflts */
	int	(*cdr_getdisktype)	__PR((SCSI *scgp, cdr_t *));			/* get disk type */
	int	(*cdr_prdiskstatus)	__PR((SCSI *scgp, cdr_t *));			/* print disk status */
	int	(*cdr_load)		__PR((SCSI *scgp, cdr_t *));			/* load disk */
	int	(*cdr_unload)		__PR((SCSI *scgp, cdr_t *));			/* unload disk */
	int	(*cdr_buffer_cap)	__PR((SCSI *scgp, long *sizep, long *freep));	/* read buffer capacity */
	int	(*cdr_check_recovery)	__PR((SCSI *scgp, cdr_t *));			/* check if recover is needed */
	int	(*cdr_recover)		__PR((SCSI *scgp, cdr_t *, int track));		/* do recover */
	int	(*cdr_set_speed_dummy)	__PR((SCSI *scgp, cdr_t *, int *speedp));	/* set recording speed & dummy write */
	int	(*cdr_set_secsize)	__PR((SCSI *scgp, int secsize));		/* set sector size */
	int	(*cdr_next_wr_address)	__PR((SCSI *scgp, track_t *trackp, long *ap));	/* get next writable addr. */
	int	(*cdr_reserve_track)	__PR((SCSI *scgp, Ulong len));			/* reserve track for future use */
	int	(*cdr_write_trackdata)	__PR((SCSI *scgp, caddr_t buf, long daddr, long bytecnt, int seccnt, BOOL islast));
	int	(*cdr_gen_cue)		__PR((track_t *trackp, void *cuep, BOOL needgap));		/* generate cue sheet */
	int	(*cdr_send_cue)		__PR((SCSI *scgp, cdr_t *, track_t *trackp));	/* send cue sheet */
	int	(*cdr_write_leadin)	__PR((SCSI *scgp, cdr_t *, track_t *trackp));	/* write leadin */
	int	(*cdr_open_track)	__PR((SCSI *scgp, cdr_t *, track_t *trackp));	/* open new track */
	int	(*cdr_close_track)	__PR((SCSI *scgp, cdr_t *, track_t *trackp));	/* close written track */
	int	(*cdr_open_session)	__PR((SCSI *scgp, cdr_t *, track_t *trackp));	/* open new session */
	int	(*cdr_close_session)	__PR((SCSI *scgp, cdr_t *));			/* really needed ??? */
	int	(*cdr_abort_session)	__PR((SCSI *scgp, cdr_t *));			/* abort current write */
	int	(*cdr_session_offset)	__PR((SCSI *scgp, long *soff));			/* read session offset*/
	int	(*cdr_fixate)		__PR((SCSI *scgp, cdr_t *, track_t *trackp));	/* write toc on disk */
	int	(*cdr_stats)		__PR((SCSI *scgp, cdr_t *));			/* final statistics printing*/
	int	(*cdr_blank)		__PR((SCSI *scgp, cdr_t *, long addr, int blanktype));	/* blank something */
	int	(*cdr_format)		__PR((SCSI *scgp, cdr_t *, int fmtflags));	/* format media */
	int	(*cdr_opc)		__PR((SCSI *scgp, caddr_t bp, int cnt, int doopc));	/* Do OPC */
	int	(*cdr_opt1)		__PR((SCSI *scgp, cdr_t *));			/* do early option processing*/
	int	(*cdr_opt2)		__PR((SCSI *scgp, cdr_t *));			/* do late option processing */
};
#endif

/*
 * Definitions for cdr_flags
 */
#define	CDR_TAO		0x01		/* Drive supports Track at once	*/
#define	CDR_SAO		0x02		/* Drive supports Sess at once	*/
#define	CDR_PACKET	0x04		/* Drive supports packet writing*/
#define	CDR_RAW		0x08		/* Drive supports raw writing	*/
#define	CDR_RAW16	0x10		/* Drive supports RAW raw16	*/
#define	CDR_RAW96P	0x20		/* Drive supports RAW raw96 pak	*/
#define	CDR_RAW96R	0x40		/* Drive supports RAW raw96 raw	*/
#ifdef	__needed__
#define	CDR_SRAW16	0x100		/* Drive supports SAO raw16	*/
#endif
#define	CDR_SRAW96P	0x200		/* Drive supports SAO raw96 pak	*/
#define	CDR_SRAW96R	0x400		/* Drive supports SAO raw96 raw */
#define	CDR_LAYER_JUMP	0x800		/* Drive s. DVD-R/DL Layer jump	*/
#define	CDR_SWABAUDIO	0x1000		/* Drive swabs audio data	*/
#define	CDR_ISREADER	0x2000		/* Drive is s CD-ROM reader	*/
#define	CDR_TRAYLOAD	0x4000		/* Drive loads CD with tray	*/
#define	CDR_CADDYLOAD	0x8000		/* Drive loads CD with caddy	*/
#define	CDR_NO_LOLIMIT	0x10000		/* Drive ignores lead-out limit	*/
#define	CDR_DVD		0x20000		/* Drive is a DVD drive		*/
#define	CDR_SIMUL	0x40000		/* Drive is simulated		*/
#define	CDR_BURNFREE	0x80000		/* Drive sup. BUFund. free rec.	*/
#define	CDR_VARIREC	0x100000	/* Drive sup. VariRec	 Plex.	*/
#define	CDR_AUDIOMASTER	0x200000	/* Drive sup. AudioMaster Yamah.*/
#define	CDR_FORCESPEED	0x400000	/* Drive sup. WriteSpeed ctl.	*/
#define	CDR_DISKTATTOO	0x800000	/* Drive sup. Yamaha DiskT@2	*/
#define	CDR_SINGLESESS	0x1000000	/* Drive sup. single sess. mode */
#define	CDR_HIDE_CDR	0x2000000	/* Drive sup. hide CDR features	*/
#define	CDR_SPEEDREAD	0x4000000	/* Drive sup. SpeedReed		*/
#define	CDR_GIGAREC	0x8000000	/* Drive sup. GigaRec	 Plex.	*/
#define	CDR_MMC		0x10000000	/* Drive is MMC compliant	*/
#define	CDR_MMC2	0x20000000	/* Drive is MMC-2 compliant	*/
#define	CDR_MMC3	0x40000000	/* Drive is MMC-3 compliant	*/
#ifdef	PROTOTYPES
#define	CDR_ALLOC	0x80000000UL	/* structure is allocated	*/
#else
#define	CDR_ALLOC	0x80000000	/* structure is allocated	*/
#endif

/*
 * Definitions for cdr_flags
 */
#define	CDR2_NOCD	0x01		/* Drive not operating on a CD	*/
#define	CDR2_BD		0x02		/* Drive is a BluRay drive	*/

/*
 * Definitions for cdr_cdrw_support
 */
#define	CDR_CDRW_NONE	0x00		/* CD-RW writing not supported */
#define	CDR_CDRW_MULTI	0x01		/* CD-RW multi speed supported */
#define	CDR_CDRW_HIGH	0x02		/* CD-RW high speed supported */
#define	CDR_CDRW_ULTRA	0x04		/* CD-RW ultra high speed supported */
#define	CDR_CDRW_ULTRAP	0x08		/* CD-RW ultra high speed+ supported */
#define	CDR_CDRW_ALL	0xFF		/* All bits set: unknown - support all */

/*
 * cdrecord.c
 */
extern	int	read_buf	__PR((int f, char *bp, int size));
extern	int	fill_buf	__PR((int f, track_t *trackp, long secno, char *bp, int size));
extern	int	get_buf		__PR((int f, track_t *trackp, long secno, char **bpp, int size));
#ifdef	_SCG_SCSITRANSP_H
extern	int	write_secs	__PR((SCSI *scgp, cdr_t *dp, char *bp, long startsec, int bytespt, int secspt, BOOL islast));
extern	int	write_track_data __PR((SCSI *scgp, cdr_t *, track_t *));
extern	int	pad_track	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp,
					long startsec, Llong amt,
					BOOL dolast, Llong *bytesp));
extern	void	load_media	__PR((SCSI *scgp, cdr_t *, BOOL));
extern	void	unload_media	__PR((SCSI *scgp, cdr_t *, UInt32_t));
extern	void	reload_media	__PR((SCSI *scgp, cdr_t *));
#endif
extern	void	raisepri	__PR((int));
extern	int	getnum		__PR((char *arg, long *valp));

/*
 * cd_misc.c
 */
extern	int	from_bcd	__PR((int b));
extern	int	to_bcd		__PR((int i));
extern	long	msf_to_lba	__PR((int m, int s, int f, BOOL force_positive));
extern	BOOL	lba_to_msf	__PR((long lba, msf_t *mp));
extern	void	sec_to_msf	__PR((long sec, msf_t *mp));
extern	void	print_min_atip	__PR((long li, long lo));
extern	BOOL	is_cdspeed	__PR((int speed));

/*
 * fifo.c
 */
extern	long	init_fifo	__PR((long));
extern	BOOL	init_faio	__PR((track_t *track, int));
extern	BOOL	await_faio	__PR((void));
extern	void	kill_faio	__PR((void));
extern	int	wait_faio	__PR((void));
extern	int	faio_read_buf	__PR((int f, char *bp, int size));
extern	int	faio_get_buf	__PR((int f, char **bpp, int size));
extern	void	fifo_stats	__PR((void));
extern	int	fifo_percent	__PR((BOOL addone));

/*
 * wm_session.c
 */
#ifdef	_SCG_SCSITRANSP_H
extern	int	write_session_data __PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
#endif

/*
 * wm_track.c
 */
#ifdef	_SCG_SCSITRANSP_H
/*extern	int	write_track_data __PR((SCSI *scgp, cdr_t *dp, track_t *trackp));*/
#endif

/*
 * wm_packet.c
 */
#ifdef	_SCG_SCSITRANSP_H
extern	int	write_packet_data __PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
#endif

/*
 * modes.c
 */
#ifdef	_SCG_SCSITRANSP_H
extern	BOOL	get_mode_params	__PR((SCSI *scgp, int page, char *pagename,
					Uchar *modep, Uchar *cmodep,
					Uchar *dmodep, Uchar *smodep,
					int *lenp));
extern	BOOL	set_mode_params	__PR((SCSI *scgp, char *pagename, Uchar *modep,
					int len, int save, int secsize));
#endif

/*
 * misc.c
 */
#ifdef	timerclear
extern	void	timevaldiff	__PR((struct timeval *start, struct timeval *stop));
extern	void	prtimediff	__PR((const char *fmt,
					struct timeval *start,
					struct timeval *stop));
#endif

/*
 * getnum.c
 */
extern	int	getnum		__PR((char *arg, long *valp));
extern	int	getllnum	__PR((char *arg, Llong *lvalp));

/*
 * scsi_cdr.c
 */
#ifdef	_SCG_SCSITRANSP_H
extern	BOOL	unit_ready	__PR((SCSI *scgp));
extern	BOOL	wait_unit_ready	__PR((SCSI *scgp, int secs));
extern	BOOL	scsi_in_progress __PR((SCSI *scgp));
extern	BOOL	cdr_underrun	__PR((SCSI *scgp));
extern	int	test_unit_ready	__PR((SCSI *scgp));
extern	int	rezero_unit	__PR((SCSI *scgp));
extern	int	request_sense	__PR((SCSI *scgp));
extern	int	request_sense_b	__PR((SCSI *scgp, caddr_t bp, int cnt));
extern	int	inquiry		__PR((SCSI *scgp, caddr_t, int));
extern	int	read_capacity	__PR((SCSI *scgp));
#ifdef	EOF	/* stdio.h has been included */
extern	void	print_capacity	__PR((SCSI *scgp, FILE *f));
#endif
extern	int	scsi_load_unload __PR((SCSI *scgp, int));
extern	int	scsi_prevent_removal __PR((SCSI *scgp, int));
extern	int	scsi_start_stop_unit __PR((SCSI *scgp, int, int, BOOL immed));

#define	ROTCTL_CLV	0	/* CLV or PCAV	*/
#define	ROTCTL_CAV	1	/* True CAV	*/

extern	int	scsi_set_speed	__PR((SCSI *scgp, int readspeed, int writespeed, int rotctl));
extern	int	scsi_get_speed	__PR((SCSI *scgp, int *readspeedp, int *writespeedp));
extern	int	qic02		__PR((SCSI *scgp, int));
extern	int	write_xscsi	__PR((SCSI *scgp, caddr_t, long, long, int));
extern	int	write_xg0	__PR((SCSI *scgp, caddr_t, long, long, int));
extern	int	write_xg1	__PR((SCSI *scgp, caddr_t, long, long, int));
extern	int	write_xg5	__PR((SCSI *scgp, caddr_t, long, long, int));
extern	int	seek_scsi	__PR((SCSI *scgp, long addr));
extern	int	seek_g0		__PR((SCSI *scgp, long addr));
extern	int	seek_g1		__PR((SCSI *scgp, long addr));
extern	int	scsi_flush_cache __PR((SCSI *scgp, BOOL immed));
extern	int	read_buffer	__PR((SCSI *scgp, caddr_t bp, int cnt, int mode));
extern	int	write_buffer	__PR((SCSI *scgp, char *buffer, long length, int mode, int bufferid, long offset));
extern	int	read_subchannel	__PR((SCSI *scgp, caddr_t bp, int track,
					int cnt, int msf, int subq, int fmt));
extern	int	read_toc	__PR((SCSI *scgp, caddr_t, int, int, int, int));
extern	int	read_toc_philips __PR((SCSI *scgp, caddr_t, int, int, int, int));
extern	int	read_header	__PR((SCSI *scgp, caddr_t, long, int, int));
extern	int	read_disk_info	__PR((SCSI *scgp, caddr_t, int));

#define	TI_TYPE_LBA	0	/* Address is LBA */
#define	TI_TYPE_TRACK	1	/* Address: 0 -> TOC, xx -> Track xx, 0xFF -> Inv Track */
#define	TI_TYPE_SESS	2	/* Address is session # */
extern	int	read_track_info	__PR((SCSI *scgp, caddr_t, int type, int addr, int cnt));
extern	int	get_trackinfo	__PR((SCSI *scgp, caddr_t, int type, int addr, int cnt));
extern	int	read_rzone_info	__PR((SCSI *scgp, caddr_t bp, int cnt));
extern	int	reserve_tr_rzone __PR((SCSI *scgp, long size));
extern	int	read_dvd_structure __PR((SCSI *scgp, caddr_t bp, int cnt, int mt, int addr, int layer, int fmt));
extern	int	send_dvd_structure __PR((SCSI *scgp, caddr_t bp, int cnt, int fmt));
extern	int	send_opc	__PR((SCSI *scgp, caddr_t, int cnt, int doopc));

#define	CL_TYPE_STOP_DEICE	0	/* Stop De-icing a DVD+RW Media */
#define	CL_TYPE_TRACK		1	/* Close Track # */
#define	CL_TYPE_SESSION		2	/* Close Session/Border / Stop backgrnd. format */
#define	CL_TYPE_INTER_BORDER	3	/* Close intermediate Border */
extern	int	scsi_close_tr_session __PR((SCSI *scgp, int type, int track, BOOL immed));
extern	int	read_master_cue	__PR((SCSI *scgp, caddr_t bp, int sheet, int cnt));
extern	int	send_cue_sheet	__PR((SCSI *scgp, caddr_t bp, long size));
extern	int	read_buff_cap	__PR((SCSI *scgp, long *, long *));
extern	int	scsi_blank	__PR((SCSI *scgp, long addr, int blanktype, BOOL immed));
extern	BOOL	allow_atapi	__PR((SCSI *scgp, BOOL new));
extern	int	mode_select	__PR((SCSI *scgp, Uchar *, int, int, int));
extern	int	mode_sense	__PR((SCSI *scgp, Uchar *dp, int cnt, int page, int pcf));
extern	int	mode_select_sg0	__PR((SCSI *scgp, Uchar *, int, int, int));
extern	int	mode_sense_sg0	__PR((SCSI *scgp, Uchar *dp, int cnt, int page, int pcf));
extern	int	mode_select_g0	__PR((SCSI *scgp, Uchar *, int, int, int));
extern	int	mode_select_g1	__PR((SCSI *scgp, Uchar *, int, int, int));
extern	int	mode_sense_g0	__PR((SCSI *scgp, Uchar *dp, int cnt, int page, int pcf));
extern	int	mode_sense_g1	__PR((SCSI *scgp, Uchar *dp, int cnt, int page, int pcf));
extern	int	read_tochdr	__PR((SCSI *scgp, cdr_t *, int *, int *));
extern	int	read_cdtext	__PR((SCSI *scgp));
extern	int	read_trackinfo	__PR((SCSI *scgp, int, long *, struct msf *, int *, int *, int *));
extern	int	read_B0		__PR((SCSI *scgp, BOOL isbcd, long *b0p, long *lop));
extern	int	read_session_offset __PR((SCSI *scgp, long *));
extern	int	read_session_offset_philips __PR((SCSI *scgp, long *));
extern	int	sense_secsize	__PR((SCSI *scgp, int current));
extern	int	select_secsize	__PR((SCSI *scgp, int));
extern	BOOL	is_cddrive	__PR((SCSI *scgp));
extern	BOOL	is_unknown_dev	__PR((SCSI *scgp));
extern	int	read_scsi	__PR((SCSI *scgp, caddr_t, long, int));
extern	int	read_g0		__PR((SCSI *scgp, caddr_t, long, int));
extern	int	read_g1		__PR((SCSI *scgp, caddr_t, long, int));
extern	BOOL	getdev		__PR((SCSI *scgp, BOOL));
#ifdef	EOF	/* stdio.h has been included */
extern	void	printinq	__PR((SCSI *scgp, FILE *f));
#endif
extern	void	printdev	__PR((SCSI *scgp));
extern	BOOL	do_inquiry	__PR((SCSI *scgp, BOOL));
extern	BOOL	recovery_needed	__PR((SCSI *scgp, cdr_t *));
extern	int	scsi_load	__PR((SCSI *scgp, cdr_t *));
extern	int	scsi_unload	__PR((SCSI *scgp, cdr_t *));
extern	int	scsi_cdr_write	__PR((SCSI *scgp, caddr_t bp, long sectaddr, long size, int blocks, BOOL islast));
extern	struct cd_mode_page_2A * mmc_cap __PR((SCSI *scgp, Uchar *modep));
extern	void	mmc_getval	__PR((struct cd_mode_page_2A *mp,
					BOOL *cdrrp, BOOL *cdwrp,
					BOOL *cdrrwp, BOOL *cdwrwp,
					BOOL *dvdp, BOOL *dvdwp));
extern	BOOL	is_mmc		__PR((SCSI *scgp, BOOL *cdwp, BOOL *dvdwp));
extern	BOOL	mmc_check	__PR((SCSI *scgp, BOOL *cdrrp, BOOL *cdwrp,
					BOOL *cdrrwp, BOOL *cdwrwp,
					BOOL *dvdp, BOOL *dvdwp));
extern	void	print_capabilities	__PR((SCSI *scgp));

extern	int	verify			__PR((SCSI *scgp, long start, int count, long *bad_block));
#endif

/*
 * scsi_cdr.c
 */
#ifdef	_SCG_SCSITRANSP_H
extern	void	print_capabilities_mmc4	__PR((SCSI *scgp));
#endif

/*
 * scsi_mmc.c
 */

/*
 * Definitions for the return value of get_mediatype()
 */
#define	MT_NONE		0	/* Unknown or unsupported	*/
#define	MT_CD		1	/* CD type media		*/
#define	MT_DVD		2	/* DVD type media		*/
#define	MT_BD		3	/* Blu Ray type media		*/
#define	MT_HDDVD	4	/* HD-DVD type media		*/

#ifdef	_SCG_SCSITRANSP_H
extern	int	get_configuration	__PR((SCSI *scgp, caddr_t bp, int cnt, int st_feature, int rt));
extern	int	get_curprofile		__PR((SCSI *scgp));
extern	int	has_profile		__PR((SCSI *scgp, int profile));
extern	int	print_profiles		__PR((SCSI *scgp));
extern	int	get_proflist		__PR((SCSI *scgp, BOOL *wp, BOOL *cdp, BOOL *dvdp,
							BOOL *dvdplusp, BOOL *ddcdp));
extern	int	get_wproflist		__PR((SCSI *scgp, BOOL *cdp, BOOL *dvdp,
							BOOL *dvdplusp, BOOL *ddcdp));
extern	int	get_mediatype		__PR((SCSI *scgp));
#endif	/* _SCG_SCSITRANSP_H */
extern	int	get_singlespeed		__PR((int mt));
extern	float	get_secsps		__PR((int mt));
extern	char	*get_mclassname		__PR((int mt));
extern	int	get_blf			__PR((int mt));
#ifdef	_SCG_SCSITRANSP_H
extern	int	print_features		__PR((SCSI *scgp));
extern	int	check_writemodes_mmc	__PR((SCSI *scgp, cdr_t *dp));
extern	int	scsi_get_perf_maxspeed	__PR((SCSI *scgp, Ulong *readp, Ulong *writep, Ulong *endp));
extern	int	scsi_get_perf_curspeed	__PR((SCSI *scgp, Ulong *readp, Ulong *writep, Ulong *endp));
extern	int	speed_select_mdvd	__PR((SCSI *scgp, int readspeed, int writespeed));
extern	void	print_format_capacities	__PR((SCSI *scgp));
extern	int	get_format_capacities	__PR((SCSI *scgp, caddr_t bp, int cnt));
extern	int	read_format_capacities	__PR((SCSI *scgp, caddr_t bp, int cnt));
#endif	/* _SCG_SCSITRANSP_H */
#ifdef _SCSIMMC_H
extern	void	przone			__PR((struct rzone_info *rp));
#endif	/* _SCSIMMC_H */
#ifdef	_SCG_SCSITRANSP_H
#ifdef	_SCSIMMC_H
extern	int	get_diskinfo		__PR((SCSI *scgp, struct disk_info *dip, int cnt));
extern	char	*get_ses_type		__PR((int ses_type));
extern	void	print_diskinfo		__PR((struct disk_info *dip, BOOL is_cd));
#endif
extern	int	prdiskstatus		__PR((SCSI *scgp, cdr_t *dp, BOOL is_cd));
extern	int	sessstatus		__PR((SCSI *scgp, BOOL is_cd, long *offp, long *nwap));
extern	void	print_performance_mmc	__PR((SCSI *scgp));
#endif	/* _SCG_SCSITRANSP_H */

/*
 * scsi_mmc4.c
 */
#ifdef	_SCG_SCSITRANSP_H
extern	int	get_supported_cdrw_media_types	__PR((SCSI *scgp));
#endif

/*
 * cdr_drv.c
 */
#ifdef	_SCG_SCSITRANSP_H
#ifdef	_SCG_SCSIREG_H
extern	cdr_t	*drive_identify		__PR((SCSI *scgp, cdr_t *, struct scsi_inquiry *ip));
#else
extern	cdr_t	*drive_identify		__PR((SCSI *scgp, cdr_t *, void *ip));
#endif
extern	int	drive_attach		__PR((SCSI *scgp, cdr_t *));
#endif
extern	int	attach_unknown		__PR((void));
#ifdef	_SCG_SCSITRANSP_H
extern	int	blank_dummy		__PR((SCSI *scgp, cdr_t *, long addr, int blanktype));
extern	int	blank_simul		__PR((SCSI *scgp, cdr_t *, long addr, int blanktype));
EXPORT	int	format_dummy		__PR((SCSI *scgp, cdr_t *, int fmtflags));
extern	int	drive_getdisktype	__PR((SCSI *scgp, cdr_t *dp));
extern	int	cmd_ill			__PR((SCSI *scgp));
extern	int	cmd_dummy		__PR((SCSI *scgp, cdr_t *));
extern	int	no_sendcue		__PR((SCSI *scgp, cdr_t *, track_t *trackp));
extern	int	no_diskstatus		__PR((SCSI *scgp, cdr_t *));
extern	int	buf_dummy		__PR((SCSI *scgp, long *sp, long *fp));
#endif
extern	BOOL	set_cdrcmds		__PR((char *name, cdr_t **dpp));
#ifdef	_SCG_SCSITRANSP_H
extern	cdr_t	*get_cdrcmds		__PR((SCSI *scgp));
#endif


/*
 * drv_mmc.c
 */
#ifdef	_SCG_SCSITRANSP_H
extern	void	mmc_opthelp		__PR((SCSI *scgp, cdr_t *dp, int excode));
#endif
extern	char	*hasdrvopt		__PR((char *optstr, char *optname));
extern	char	*hasdrvoptx		__PR((char *optstr, char *optname, int flag));
#ifdef	_SCG_SCSITRANSP_H
extern struct ricoh_mode_page_30 * get_justlink_ricoh	__PR((SCSI *scgp, Uchar *mode));
#endif

/*
 * isosize.c
 */
extern	Llong	isosize		__PR((int f));
extern	Llong	bisosize	__PR((char *bp, int len));

/*
 * audiosize.c
 */
extern	BOOL	is_auname	__PR((const char *name));
extern	off_t	ausize		__PR((int f));
extern	BOOL	is_wavname	__PR((const char *name));
extern	off_t	wavsize		__PR((int f));

/*
 * auinfo.c
 */
extern	BOOL	auinfosize	__PR((char *name, track_t *trackp));
extern	BOOL	auinfhidden	__PR((char *name, int track, track_t *trackp));
extern	void	auinfo		__PR((char *name, int track, track_t *trackp));
#ifdef CDTEXT_H
extern	textptr_t *gettextptr	__PR((int track, track_t *trackp));
#endif
extern	void	setmcn		__PR((char *mcn, track_t *trackp));
extern	void	setisrc		__PR((char *isrc, track_t *trackp));
extern	void	setindex	__PR((char *tindex, track_t *trackp));

/*
 * diskid.c
 */
extern	void	pr_manufacturer		__PR((msf_t *mp, BOOL rw, BOOL audio));
extern	int	manufacturer_id		__PR((msf_t *mp));
extern	long	disk_rcap		__PR((msf_t *mp, long maxblock, BOOL rw, BOOL audio));

/*--------------------------------------------------------------------------*/
/* Test only								    */
/*--------------------------------------------------------------------------*/
#ifdef _SCSIMMC_H
/*extern	int	do_cue		__PR((track_t *trackp, struct mmc_cue **cuep));*/
#else
/*extern	int	do_cue		__PR((track_t *trackp, void *cuep));*/
#endif

/*
 * subchan.c
 */
extern	int	do_leadin	__PR((track_t *trackp));
#ifdef	_SCG_SCSITRANSP_H
extern	int	write_leadin	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp, int leadinstart));
extern	int	write_leadout	__PR((SCSI *scgp, cdr_t *dp, track_t *trackp));
#endif
extern	void	fillsubch	__PR((track_t *trackp, Uchar *sp, int secno, int nsecs));
extern	void	filltpoint	__PR((Uchar *sub, int ctrl_adr, int point, msf_t *mp));
extern	void	fillttime	__PR((Uchar *sub, msf_t *mp));
extern	void	qpto96		__PR((Uchar *sub, Uchar *subq, int dop));
extern	void	addrw		__PR((Uchar *sub, Uchar	*subrwptr));
extern	void	qwto16		__PR((Uchar *subq, Uchar *subptr));
extern	void	subrecodesecs	__PR((track_t *trackp, Uchar *bp, int address, int nsecs));

/*
 * sector.c
 */
extern	int	encspeed	__PR((BOOL be_verbose));
extern	void	encsectors	__PR((track_t *trackp, Uchar *bp, int address, int nsecs));
extern	void	scrsectors	__PR((track_t *trackp, Uchar *bp, int address, int nsecs));
extern	void	encodesector	__PR((Uchar *sp, int sectype, int address));
extern	void	fillsector	__PR((Uchar *sp, int sectype, int address));

/*
 * clone.c
 */
extern	void	clone_toc	__PR((track_t *trackp));
extern	void	clone_tracktype	__PR((track_t *trackp));

/*
 * cdtext.c
 */
extern	BOOL	checktextfile	__PR((char *fname));
extern	void	packtext	__PR((int tracks, track_t *trackp));
#ifdef	_SCG_SCSITRANSP_H
extern	int	write_cdtext	__PR((SCSI *scgp, cdr_t *dp, long startsec));
#endif

/*
 * cue.c
 */
extern	int	parsecue	__PR((char *cuefname, track_t trackp[]));
#ifdef	EOF	/* stdio.h has been included */
extern	void	fparsecue	__PR((FILE *f, track_t trackp[]));
#endif

/*
 * vendor.c
 */

/*
 * priv.c
 */
#ifdef	CDDA2WAV
extern	void	priv_init	__PR((void));
extern	void	priv_on		__PR((void));
extern	void	priv_off	__PR((void));
#endif
#if	defined(CDRECORD) || defined(READCD)
extern	void	priv_drop	__PR((void));
extern	BOOL	priv_from_priv	__PR((void));
#endif
extern	BOOL	priv_eff_priv	__PR((int pname));
#ifdef	HAVE_SOLARIS_PPRIV
extern	void	do_pfexec	__PR((int ac, char *av[], ...));
#endif
