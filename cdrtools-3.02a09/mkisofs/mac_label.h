/* @(#)mac_label.h	1.3 04/03/01 joerg, Copyright 1997, 1998, 1999, 2000 James Pearson */
/*
 *      Copyright (c) 1997, 1998, 1999, 2000 James Pearson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 *	mac_label.h: defines Macintosh partition maps and label
 *
 *	Taken from "mkisofs 1.05 PLUS" by Andy Polyakov <appro@fy.chalmers.se>
 *	(see http://fy.chalmers.se/~appro/mkisofs_plus.html for details)
 *
 *	Much of this is already defined in the libhfs code, but to keep
 *	things simple we stick with these.
 */

#ifndef	_MAC_LABEL_H
#define	_MAC_LABEL_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef PREP_BOOT
struct fdiskPartition {
#define	prepPartType 0x41
#define	chrpPartType 0x96
	unsigned char	boot;		/* bootable flag; not used */
	unsigned char	CHSstart[3];	/* start CHS; not used */
	unsigned char	type;		/* Partition type, = 0x41 (PReP bootable) */
	unsigned char	CHSend[3];	/* end CHS; not used */
	unsigned char	startSect[4];	/* start sector (really start of boot image on CD */
	unsigned char	size[4];	/* size of partition (or boot image ;) ) */
};
typedef struct fdiskPartition fdiskPartition;
#endif

/* Driver Descriptor Map */
#define	sbSigMagic	"ER"
struct MacLabel {
	unsigned char	sbSig[2];	/* unique value for SCSI block 0 */
	unsigned char	sbBlkSize[2];	/* block size of device */
	unsigned char	sbBlkCount[4];	/* number of blocks on device */
	unsigned char	sbDevType[2];	/* device type */
	unsigned char	sbDevId[2];	/* device id */
	unsigned char	sbData[4];	/* not used */
	unsigned char	sbDrvrCount[2];	/* driver descriptor count */
	unsigned char	ddBlock[4];	/* 1st driver's starting block */
	unsigned char	ddSize[2];	/* size of 1st driver (512-byte blks) */
	unsigned char	ddType[2];	/* system type (1 for Mac+) */
#ifndef PREP_BOOT
	unsigned char	ddPad[486];	/* ARRAY[0..242] OF INTEGER; not used */
#else
#define	fdiskMagic0 0x55
#define	fdiskMagic1 0xAA
	unsigned char   pad1[420];	/* not used */
	fdiskPartition	image[4];	/* heh heh heh, we can have up to 4 */
					/* different boot images */
	unsigned char	fdiskMagic[2];	/* PReP uses fdisk partition map */
#endif /* PREP_BOOT */
};
typedef struct MacLabel MacLabel;

#define	IS_MAC_LABEL(d)		(((MacLabel*)(d))->sbSig[0] == 'E' && \
				((MacLabel*)(d))->sbSig[1] == 'R')

/* Partition Map Entry */
#define	pmSigMagic	"PM"

#define	pmPartType_1	"Apple_partition_map"
#define	pmPartName_11	"Apple"

#define	pmPartType_2	"Apple_Driver"
#define	pmPartType_21	"Apple_Driver43"

#define	pmPartType_3	"Apple_UNIX_SVR2"
#define	pmPartName_31	"A/UX Root"
#define	pmPartName_32	"A/UX Usr"
#define	pmPartName_33	"Random A/UX fs"
#define	pmPartName_34	"Swap"

#define	pmPartType_4	"Apple_HFS"
#define	pmPartName_41	"MacOS"

#define	pmPartType_5	"Apple_Free"
#define	pmPartName_51	"Extra"

#define	PM2	2
#define	PM4	4

struct MacPart {
	unsigned char	pmSig[2];	/* unique value for map entry blk */
	unsigned char	pmSigPad[2];	/* currently unused */
	unsigned char	pmMapBlkCnt[4];	/* # of blks in partition map */
	unsigned char	pmPyPartStart[4]; /* physical start blk of partition */
	unsigned char	pmPartBlkCnt[4];  /* # of blks in this partition */
	unsigned char	pmPartName[32];	  /* ASCII partition name */
	unsigned char	pmPartType[32];	  /* ASCII partition type */
	unsigned char	pmLgDataStart[4]; /* log. # of partition's 1st data blk */
	unsigned char	pmDataCnt[4];	  /* # of blks in partition's data area */
	unsigned char	pmPartStatus[4];  /* bit field for partition status */
	unsigned char	pmLgBootStart[4]; /* log. blk of partition's boot code */
	unsigned char	pmBootSize[4];	/* number of bytes in boot code */
	unsigned char	pmBootAddr[4];	/* memory load address of boot code */
	unsigned char	pmBootAddr2[4];	/* currently unused */
	unsigned char	pmBootEntry[4];	/* entry point of boot code */
	unsigned char	pmBootEntry2[4]; /* currently unused */
	unsigned char	pmBootCksum[4];	 /* checksum of boot code */
	unsigned char	pmProcessor[16]; /* ASCII for the processor type */
	unsigned char	pmPad[376];	 /* ARRAY[0..187] OF INTEGER; not used */
};
typedef struct MacPart MacPart;

#define	IS_MAC_PART(d)		(((MacPart*)(d))->pmSig[0] == 'P' && \
				((MacPart*)(d))->pmSig[1] == 'M')

#define	PM_STAT_VALID		0x01	/* Set if a valid partition map entry */
#define	PM_STAT_ALLOC		0x02	/* Set if partition is already allocated; clear if available */
#define	PM_STAT_INUSE		0x04	/* Set if partition is in use; may be cleared after a system reset */
#define	PM_STAT_BOOTABLE	0x08	/* Set if partition contains valid boot information */
#define	PM_STAT_READABLE	0x10	/* Set if partition allows reading */
#define	PM_STAT_WRITABLE	0x20	/* Set if partition allows writing */
#define	PM_STAT_BOOT_PIC	0x40	/* Set if boot code is position-independent */
#define	PM_STAT_UNUSED		0x80	/* Unused */
#define	PM_STAT_DEFAULT		PM_STAT_VALID|PM_STAT_ALLOC|PM_STAT_READABLE|PM_STAT_WRITABLE

typedef struct {
	char *name;			/* Partition name */
	char *type;			/* Partition type */
	int   ntype;			/* Partition type (numeric) */
	int   start;			/* start extent (SECTOR_SIZE blocks) */
	int   size;			/* extents (SECTOR_SIZE blocks) */
} mac_partition_table;

/* from libhfs */
#define	HFS_BB_SIGWORD		0x4c4b

typedef struct deferred_write defer;

#ifdef __cplusplus
}
#endif

#endif /* _MAC_LABEL_H */
