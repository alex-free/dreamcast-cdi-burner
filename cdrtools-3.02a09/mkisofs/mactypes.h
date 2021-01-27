/* @(#)mactypes.h	1.3 04/03/01 joerg */

#ifndef	_MACTYPES_H
#define	_MACTYPES_H

/*
 * Various types of HFS files stored on Unix systems
 */
#define	TYPE_NONE	0	/* Unknown file type (ordinary Unix file) */
#define	TYPE_CAP	1	/* AUFS CAP */
#define	TYPE_NETA	2	/* Netatalk */
#define	TYPE_DBL	3	/* AppleDouble */
#define	TYPE_ESH	4	/* Helios EtherShare */
#define	TYPE_FEU	5	/* PC Exchange (Upper case) */
#define	TYPE_FEL	6	/* PC Exchange (Lower case) */
#define	TYPE_SGI	7	/* SGI */
#define	TYPE_MBIN	8	/* MacBinary */
#define	TYPE_SGL	9	/* AppleSingle */
#define	TYPE_DAVE	10	/* DAVE (AppleDouble type) */
#define	TYPE_SFM	11	/* NTFS Services for Macintosh */
#define	TYPE_XDBL	12	/* MacOS X AppleDouble */
#define	TYPE_XHFS	13	/* MacOS X HFS */

/*
 * above encoded in a bit map
 */
#define	DO_NONE		(1 << TYPE_NONE)
#define	DO_CAP		(1 << TYPE_CAP)
#define	DO_NETA		(1 << TYPE_NETA)
#define	DO_DBL		(1 << TYPE_DBL)
#define	DO_ESH		(1 << TYPE_ESH)
#define	DO_FEU		(1 << TYPE_FEU)
#define	DO_FEL		(1 << TYPE_FEL)
#define	DO_SGI		(1 << TYPE_SGI)
#define	DO_MBIN		(1 << TYPE_MBIN)
#define	DO_SGL		(1 << TYPE_SGL)
#define	DO_DAVE		(1 << TYPE_DAVE)
#define	DO_SFM		(1 << TYPE_SFM)
#define	DO_XDBL		(1 << TYPE_XDBL)
#define	DO_XHFS		(1 << TYPE_XHFS)

/*
 * flags describing how the data/rsrc/info files are stored
 * in the whole file name
 */
#define	INSERT		0x0	/* rsrc/info is inserted in name (default) */
#define	PROBE		0x1	/* need to probe file for type */
#define	NOPEND		0x2	/* info data in one file for all files */
#define	APPEND		0x4	/* rsrc/info appended to file name */
#define	NORSRC		0x8	/* rsrc for file may not exist */
#define	NOINFO		0x10	/* info file dosen't exist */

#endif /* _MACTYPES_H */
