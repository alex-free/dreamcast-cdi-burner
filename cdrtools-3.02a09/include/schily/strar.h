/* @(#)strar.h	1.4 17/02/15 Copyright 2001-2017 J. Schilling */
/*
 *	Defitions for the stream archive interfaces.
 *
 *	A stream archive is based on the method used for
 *	POSIX tar extended headers
 *
 *	Copyright (c) 2001-2017 J. Schilling
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

#ifndef	_SCHILY_STRAR_H
#define	_SCHILY_STRAR_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_STAT_H
#include <schily/stat.h>
#endif
#ifndef _SCHILY_STDIO_H
#include <schily/stdio.h>
#endif
#ifndef _SCHILY_UTYPES_H
#include <schily/utypes.h>
#endif

typedef	struct	{
	FILE	*f_fp;		/* FILE * f. Archiv			  */
	const char *f_fpname;	/* Archive name				  */
	FILE	*f_list;	/* FILE * f. Listing Output		  */
	const char *f_listname;	/* List output name			  */
	Ulong	f_cmdflags;	/* Command specific flags		  */

	char	*f_name;	/* Zeiger auf den langen Dateinamen	  */
	Ulong	f_namelen;	/* Länge des Dateinamens		  */
	char	*f_lname;	/* Zeiger auf den langen Linknamen	  */
	Ulong	f_lnamelen;	/* Länge des Linknamens			  */

	char	*f_uname;	/* User name oder NULL Pointer		  */
	Ulong	f_umaxlen;	/* Maximale Länge des Usernamens	  */
	char	*f_gname;	/* Group name oder NULL Pointer		  */
	Ulong	f_gmaxlen;	/* Maximale Länge des Gruppennamens	  */

	dev_t	f_dev;		/* Geraet auf dem sich d. Datei befindet  */
	ino_t	f_ino;		/* Dateinummer				  */
	nlink_t	f_nlink;	/* Anzahl der Links			  */

	mode_t	f_mode;		/* Zugriffsrechte			  */

	uid_t	f_uid;		/* Benutzernummer			  */
	gid_t	f_gid;		/* Benutzergruppe			  */

	Ullong	f_llsize;	/* Dateigroesze wenn off_t zu kein	  */
	off_t	f_size;		/* Dateigroesze				  */
	off_t	f_rsize;	/* Dateigroesze auf Band		  */

	Ulong	f_flags;	/* Bearbeitungshinweise			  */
	Ulong	f_xflags;	/* Flags für x-header			  */
	Ulong	f_xftype;	/* Header Dateityp (neu generell)	  */
	Ulong	f_rxftype;	/* Echter Dateityp (neu generell)	  */

#ifdef	NEW_RDEV
	dev_t	f_rdev;		/* Major/Minor bei Geraeten		  */
	major_t	f_rdevmaj;	/* Major bei Geraeten			  */
	minor_t	f_rdevmin;	/* Minor bei Geraeten			  */
#else
	Ulong	f_rdev;		/* Major/Minor bei Geraeten		  */
	Ulong	f_rdevmaj;	/* Major bei Geraeten			  */
	Ulong	f_rdevmin;	/* Minor bei Geraeten			  */
#endif

	time_t	f_atime;	/* Zeit d. letzten Zugriffs		  */
	long	f_ansec;	/* nsec Teil "				  */
	time_t	f_mtime;	/* Zeit d. letzten Aenderung		  */
	long	f_mnsec;	/* nsec Teil "				  */
	time_t	f_ctime;	/* Zeit d. letzten Statusaend.		  */
	long	f_cnsec;	/* nsec Teil "				  */
	long	f_status;	/* File send status			  */
} FINFO;

typedef	FINFO	strar;

/*
 * Used with f_cmdflags
 */
#define	CMD_VERBOSE	0xFF	/* Allow verbose levels from 0..255	  */
#define	CMD_CREATE	0x100
#define	CMD_XTRACT	0x200
#define	CMD_LIST	0x400
#define	CMD_CTIME	0x800

/*
 * Used with f_flags
 */
#define	F_BAD_SIZE	0x1000	/* Bad size data detected		  */
#define	F_BAD_META	0x2000	/* Bad meta data detected		  */
#define	F_BAD_UID	0x4000	/* Bad uid value detected		  */
#define	F_BAD_GID	0x8000	/* Bad gid value detected		  */

/*
 * Used with f_xflags
 */
#define	XF_ATIME	0x0001	/* Zeit d. letzten Zugriffs		  */
#define	XF_CTIME	0x0002	/* Zeit d. letzten Statusaend.		  */
#define	XF_MTIME	0x0004	/* Zeit d. letzten Aenderung		  */
#define	XF_COMMENT	0x0008	/* Beliebiger Kommentar			  */
#define	XF_UID		0x0010	/* Benutzernummer			  */
#define	XF_UNAME	0x0020	/* Langer Benutzername			  */
#define	XF_GID		0x0040	/* Benutzergruppe			  */
#define	XF_GNAME	0x0080	/* Langer Benutzergruppenname		  */
#define	XF_PATH		0x0100	/* Langer Name				  */
#define	XF_LINKPATH	0x0200	/* Langer Link Name			  */
				/* Dateigröße auf Band (f_rsize)	  */
#define	XF_SIZE		0x0400	/* Dateigröße wenn > 8 GB		  */
#define	XF_CHARSET	0x0800	/* Zeichensatz für Dateiinhalte		  */

#define	XF_DEVMAJOR	0x1000	/* Major bei Geräten			  */
#define	XF_DEVMINOR	0x2000	/* Major bei Geräten			  */

#define	XF_FFLAGS	0x10000	/* File flags				  */
				/* Echte Dateigröße (f_size)		  */
#define	XF_REALSIZE	0x20000	/* Dateigröße wenn > 8 GB		  */
#define	XF_STATUS	0x40000	/* File send status			  */
#define	XF_EOF		0x80000	/* Logical EOF in archive		  */
#define	XF_DEV		0x100000 /* Device FS is on			  */
#define	XF_INO		0x200000 /* Inode number for file		  */
#define	XF_NLINK	0x400000 /* Link count				  */
#define	XF_MODE		0x800000 /* File mode				  */
#define	XF_FILETYPE	0x1000000 /* File type				  */

#define	XF_BASE_FILEMETA (XF_FILETYPE | XF_MODE)
#define	XF_ALL_FILEMETA	(XF_FILETYPE | XF_MODE | \
			XF_ATIME | XF_MTIME | XF_CTIME | \
			XF_UID | XF_GID | XF_UNAME | XF_GNAME | \
			XF_DEV | XF_INO | XF_NLINK | XF_DEVMAJOR | XF_DEVMINOR)

/*
 * All Extended header tags that are covered by POSIX.1-2001
 */
#define	XF_POSIX	(XF_ATIME|XF_CTIME|XF_MTIME|XF_COMMENT|\
			XF_UID|XF_UNAME|XF_GID|XF_GNAME|\
			XF_PATH|XF_LINKPATH|XF_SIZE|XF_CHARSET)


/*
 * Open modes
 */
#define	OM_READ		1
#define	OM_WRITE	2
#define	OM_ARFD		4

extern	int	strar_open	__PR((strar *s, const char *name, int arfd,
					int mode));
extern	int	strar_close	__PR((strar *s));
extern	void	strar_init	__PR((strar *s));
extern	void	strar_reset	__PR((strar *s));
extern	void	strar_archtype	__PR((strar *s));
extern	void	strar_eof	__PR((strar *s));
extern	int	strar_send	__PR((strar *s, const char *name));
extern	int	strar_st_send	__PR((strar *s, struct stat *sp));
extern	void	strar_list_file	__PR((strar *s));
extern	void	strar_vprint	__PR((strar *s));
extern	int	strar_receive	__PR((strar *s, int (*func)(strar *)));

extern	int	strar_hparse	__PR((strar *s));
extern	int	strar_get	__PR((strar *s));
extern	int	strar_skip	__PR((strar *s));

extern	int	strar_setnowarn	__PR((int val));
extern	void	strar_xbreset	__PR((void));


#endif	/* _SCHILY_STRAR_H */
