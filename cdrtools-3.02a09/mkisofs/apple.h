/* @(#)apple.h	1.7 04/03/02 joerg, Copyright 1997, 1998, 1999, 2000 James Pearson */
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
 * apple.h:	cut down macfile.h from CAP distribution
 */
#ifndef	_APPLE_H
#define	_APPLE_H

#include "mactypes.h"

#ifndef	O_BINARY
#define	O_BINARY 0
#endif /* O_BINARY */

#ifdef	_WIN32_TEST
#undef	UNICODE
#include <windows.h>
#endif /* _WIN32 */

#ifndef	MIN
#define	MIN(a, b) (((a) < (b)) ? (a):(b))
#endif /* MIN */

#define	CT_SIZE		4			/* Size of type/creator */
#define	NUMMAP		512			/* initial number of maps */
#define	BLANK		"    "			/* blank type/creator */
#define	DEFMATCH	"*"			/* default mapping extension */

typedef struct {
	char		*extn;			/* filename extension */
	int		elen;			/* length of extension */
	char		type[CT_SIZE+1];	/* extension type */
	char		creator[CT_SIZE+1];	/* extension creator */
	unsigned short	fdflags;		/* finder flags */
} afpmap;

/* from "data.h" - libhfs routines */
unsigned long d_toutime	__PR((unsigned long));
unsigned long d_dtoutime __PR((long));
long d_getl		__PR((unsigned char *));
short d_getw		__PR((unsigned char *));

/* for libfile routines */
int init_magic		__PR((char *));
char * get_magic_match	__PR((const char *));

typedef unsigned char byte;
typedef unsigned char word[2];
typedef unsigned char dword[4];

#define	INFOLEN 32		/* Finder info is 32 bytes */

typedef struct {
	/* base finder information */
	char fdType[4];			/* File type [4] */
	char fdCreator[4];		/* File creator [8] */
	word fdFlags;			/* Finder flags [10] */
	word fdLocation[2];		/* File's location [14] */
	word fdFldr;			/* File's window [16] */
	/* extended finder information */
	word fdIconID;			/* Icon ID [18] */
	word fdUnused[3];		/* Unused [24] */
	byte fdScript;			/* Script system used [25] */
	byte fdXFlags;			/* Reserved [26] */
	word fdComment;			/* Comment ID [28] */
	dword fdPutAway;		/* Home directory ID [32] */
} Finfo;

typedef struct {
	/* base finder information */
	word frRect[4];			/* Folder's rectangle [8] */
	word frFlags;			/* Finder flags [10] */
	word frLocation[2];		/* Folder's location [14] */
	word frView;			/* Folder's view [16] */
	/* extended finder information */
	word frScroll[2];		/* Folder's scroll position [20] */
	dword frOpenChain;		/* ID's of open folders [24] */
	byte frScript;			/* Script system used [25] */
	byte frXFlags;			/* Reserved [26] */
	word frComment;			/* Comment ID [28] */
	dword frPutAway;		/* Home directory ID [32] */
} Dinfo;

/****** TYPE_CAP ******/

/*
 * taken from the CAP distribution:
 * macfile.h - header file with Macintosh file definitions
 *
 * AppleTalk package for UNIX (4.2 BSD).
 *
 * Copyright (c) 1986, 1987, 1988 by The Trustees of Columbia University in the
 * City of New York.
 *
 * Edit History:
 *
 *  Sept 1987	Created by Charlie
 *
 */


#ifndef	USE_MAC_DATES
#define	USE_MAC_DATES
#endif /* USE_MAC_DATES */

#define	MAXCLEN 199		/* max size of a comment string */
#define	FINFOLEN 32		/* Finder info is 32 bytes */
#define	MAXMACFLEN 31		/* max Mac file name length */

typedef struct {
	byte	finderinfo[INFOLEN];	/* Finder info */
	word	fi_attr;		/* attributes */
#define	FI_MAGIC1 255
	byte	fi_magic1;		/* was: length of comment */
#define	FI_VERSION 0x10			/* version major 1, minor 0 */
					/* if we have more than 8 versions wer're */
					/* doiong something wrong anyway */
	byte	fi_version;		/* version number */
#define	FI_MAGIC 0xda
	byte	fi_magic;		/* magic word check */
	byte	fi_bitmap;		/* bitmap of included info */
#define	FI_BM_SHORTFILENAME 0x1		/* is this included? */
#define	FI_BM_MACINTOSHFILENAME 0x2	/* is this included? */
	byte	fi_shortfilename[12+1];	/* possible short file name */
	byte	fi_macfilename[32+1];	/* possible macintosh file name */
	byte	fi_comln;		/* comment length */
	byte	fi_comnt[MAXCLEN+1];	/* comment string */
#ifdef	USE_MAC_DATES
	byte	fi_datemagic;		/* sanity check */
#define	FI_MDATE 0x01			/* mtime & utime are valid */
#define	FI_CDATE 0x02			/* ctime is valid */
	byte	fi_datevalid;		/* validity flags */
	byte	fi_ctime[4];		/* mac file create time */
	byte	fi_mtime[4];		/* mac file modify time */
	byte	fi_utime[4];		/* (real) time mtime was set */
#endif /* USE_MAC_DATES */
} FileInfo;

/* Atribute flags */
#define	FI_ATTR_SETCLEAR 0x8000 /* set-clear attributes */
#define	FI_ATTR_READONLY 0x20	/* file is read-only */
#define	FI_ATTR_ROPEN 0x10	/* resource fork in use */
#define	FI_ATTR_DOPEN 0x80	/* data fork in use */
#define	FI_ATTR_MUSER 0x2	/* multi-user */
#define	FI_ATTR_INVISIBLE 0x1	/* invisible */

/**** MAC STUFF *****/

/* Flags */
#define	FNDR_fOnDesk 0x1
#define	FNDR_fHasBundle 0x2000
#define	FNDR_fInvisible 0x4000
/* locations */
#define	FNDR_fTrash -3	/* File in Trash */
#define	FNDR_fDesktop -2	/* File on desktop */
#define	FNDR_fDisk 0	/* File in disk window */

/****** TYPE_ESHARE ******/

/*
 *	Information supplied by Jens-Uwe Mager (jum@helios.de)
 */

#define	ES_VERSION 	0x0102
#define	ES_MAGIC 	0x3681093
#define	ES_INFOLEN	32
#define	ES_INFO_SIZE	512

typedef struct {
	dword		magic;
	dword		serno;			/* written only, never read */
	word		version;
	word		attr;			/* invisible... */
	word		openMax;		/* max number of opens */
	word		filler0;
	dword		backupCleared;		/* time backup bit cleared */
	dword		id;			/* dir/file id */
	dword		createTime;		/* unix format */
	dword		backupTime;		/* unix format */
	byte		finderinfo[INFOLEN];	/* Finder info */
} es_FileInfo;

/****** TYPE_USHARE ******/

/*
 * similar to the EtherShare layout, but the finder info stuff is different
 * info provided by: Phil Sylvester <psylvstr@interaccess.com>
 */

typedef struct {
	byte		finderinfo[INFOLEN];	/* Finder info */
	dword		btime;			/* mac file backup time [36]*/
	byte		unknown2[4];		/* ignore [40] */
	dword		ctime;			/* mac file create time [44]*/
	byte		unknown3[8];		/* ignore [52] */
	dword		mtime;			/* mac file modify time [56]*/
	byte		unknown4[456];		/* ignore [512] */
} us_FileInfo;

/****** TYPE_DOUBLE, TYPE_SINGLE ******/

/*
 *	Taken from cvt2cap (c) May 1988, Paul Campbell
 */

typedef struct {
	dword id;
	dword offset;
	dword length;
} a_entry;

typedef struct {
	dword   magic;
	dword   version;
	char    home[16];
	word    nentries;
	a_entry	entries[1];
} a_hdr;

#define	A_HDR_SIZE	26
#define	A_ENTRY_SIZE	sizeof (a_entry)

#define	A_VERSION1	0x00010000
#define	A_VERSION2	0x00020000
#define	APPLE_SINGLE	0x00051600
#define	APPLE_DOUBLE	0x00051607
#define	ID_DATA		1
#define	ID_RESOURCE	2
#define	ID_NAME		3
#define	ID_FILEI	7	/* v1 */
#define	ID_FILEDATESI	8	/* v2 */
#define	ID_FINDER	9

#define	A_DATE		16

/****** TYPE_MACBIN ******/
/*
 *	taken from capit.c by Nigel Perry, np@doc.ic.ac.uk which is adapted
 *	from unmacbin by John M. Sellens, jmsellens@watdragon.uwaterloo.ca
 */


#define	MB_NAMELEN 63		/* maximum legal Mac file name length */
#define	MB_SIZE 128

/*
 * Format of a bin file:
 * A bin file is composed of 128 byte blocks.  The first block is the
 * info_header (see below).  Then comes the data fork, null padded to fill the
 * last block.  Then comes the resource fork, padded to fill the last block.  A
 * proposal to follow with the text of the Get Info box has not been implemented,
 * to the best of my knowledge.  Version, zero1 and zero2 are what the receiving
 * program looks at to determine if a MacBinary transfer is being initiated.
 */
typedef struct {   		/* info file header (128 bytes). Unfortunately, these */
				/* longs don't align to word boundaries */
	byte version;		/* there is only a version 0 at this time */
	byte nlen;		/* Length of filename. */
	byte name[MB_NAMELEN];	/* Filename */
	byte type[4];		/* File type. */
	byte auth[4];		/* File creator. */
	byte flags;		/* file flags: LkIvBnSyBzByChIt */
	byte zero1;		/* Locked, Invisible,Bundle, System */
				/* Bozo, Busy, Changed, Init */
	byte icon_vert[2];	/* Vertical icon position within window */
	byte icon_horiz[2];	/* Horizontal icon postion in window */
	byte window_id[2];	/* Window or folder ID. */
	byte protect;		/* = 1 for protected file, 0 otherwise */
	byte zero2;
	byte dflen[4];		/* Data Fork length (bytes) - most sig.  */
	byte rflen[4];		/* Resource Fork length	byte first */
	byte cdate[4];		/* File's creation date. */
	byte mdate[4];		/* File's "last modified" date. */
	byte ilen[2];		/* GetInfo message length */
	byte flags2;		/* Finder flags, bits 0-7 */
	byte unused[14];
	byte packlen[4];	/* length of total files when unpacked */
	byte headlen[2];	/* length of secondary header */
	byte uploadvers;	/* Version of MacBinary II that the uploading program is written for */
	byte readvers;		/* Minimum MacBinary II version needed to read this file */
	byte crc[2];		/* CRC of the previous 124 bytes */
	byte padding[2];	/* two trailing unused bytes */
} mb_info;

/****** TYPE_FE ******/

/* Information provided by Mark Weinstein <mrwesq@earthlink.net> */

typedef struct {
	byte	nlen;
	byte	name[31];
	byte	finderinfo[INFOLEN];	/* Finder info */
	byte	cdate[4];
	byte	mdate[4];
	byte	bdate[4];
	byte	fileid[4];
	byte	sname[8];
	byte	ext[3];
	byte	pad;
} fe_info;

#define	FE_SIZE 92

/****** TYPE_SGI ******/

typedef struct {
	byte    unknown1[8];
	byte	finderinfo[INFOLEN];	/* Finder info */
	byte    unknown2[214];
	byte    name[32];
	byte    unknown3[14];
} sgi_info;

#define	SGI_SIZE 300

/****** TYPE_SFM ******/

/*
 * Information provided by Lou Rieger <lrieger@meridiancg.com> taken from
 * an email from Eddie Bowers <eddieb@microsoft.com>
 */

typedef struct {
	byte	afpi_Signature[4];	/* Must be 0x00504641 */
	byte	afpi_Version[4];	/* Must be 0x00010000 */
	byte	afpi_Reserved1[4];
	byte	afpi_BackupTime[4];	/* Backup time for the file/dir */
	byte	finderinfo[INFOLEN];	/* Finder info */
	byte	afpi_ProDosInfo[6];	/* ProDos Info */
	byte	afpi_Reserved2[6];
} sfm_info;

#define	SFM_MAGIC	0x00504641
#define	SFM_VERSION	0x00010000

/****** TYPE_DHFS ******/

#ifdef IS_MACOS_X

/*
 *	Code ideas from 'hfstar' by Marcel Weiher marcel@metaobject.com,
 *	another GNU hfstar by Torres Vedras paulotex@yahoo.com and
 *	hfspax by Howard Oakley howard@quercus.demon.co.uk
 */

#include <sys/attr.h>

typedef struct {
	unsigned long	info_length;
	struct timespec	ctime;
	struct timespec	mtime;
	byte		info[32];
} attrinfo;

#endif /* IS_MACOS_X */

#endif /* _APPLE_H */
