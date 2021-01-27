/* @(#)cdda_paranoia.h	1.29 13/12/22 J. Schilling from cdparanoia-III-alpha9.8 */
/*
 * CopyPolicy: GNU Lesser General Public License v2.1 applies
 * Copyright (C) 1997-2001,2008 by Monty (xiphmont@mit.edu)
 * Copyright (C) 2002-2013 by J. Schilling
 */

#ifndef	_CDROM_PARANOIA_H
#define	_CDROM_PARANOIA_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef _SCHILY_UTYPES_H
#include <schily/utypes.h>
#endif

#define	CD_FRAMESIZE_RAW		2352
#define	CD_FRAMEWORDS			(CD_FRAMESIZE_RAW/2)
#define	CD_C2PTR_RAW			(CD_FRAMESIZE_RAW/8)
#define	CD_C2SIZE_RAW			(CD_FRAMESIZE_RAW + CD_C2PTR_RAW)

#define	CD_READAHEAD			150	/* # of sectors to read for a block */

/*
 * Second parameter of the callback function
 */
#define	PARANOIA_CB_READ		 0	/* Read off adjust ??? */
#define	PARANOIA_CB_VERIFY		 1	/* Verifying jitter */
#define	PARANOIA_CB_FIXUP_EDGE		 2	/* Fixed jitter at edge */
#define	PARANOIA_CB_FIXUP_ATOM		 3	/* Fixed jitter in atomic read */
#define	PARANOIA_CB_SCRATCH		 4	/* Unsupported */
#define	PARANOIA_CB_REPAIR		 5	/* Unsupported */
#define	PARANOIA_CB_SKIP		 6	/* Skip exhausted retry */
#define	PARANOIA_CB_DRIFT		 7	/* Drift detected */
#define	PARANOIA_CB_BACKOFF		 8	/* Unsupported */
#define	PARANOIA_CB_OVERLAP		 9	/* Dyn Overlap adjust */
#define	PARANOIA_CB_FIXUP_DROPPED	10	/* Fixed dropped bytes */
#define	PARANOIA_CB_FIXUP_DUPED		11	/* Fixed duplicate bytes */
#define	PARANOIA_CB_READERR		12	/* Hard read error */
#define	PARANOIA_CB_CACHEERR		13	/* Cache seek positional error */
#define	PARANOIA_CB_SECS		14	/* # of sectors with last read */
#define	PARANOIA_CB_C2ERR		15	/* # of reads with C2 errors */
#define	PARANOIA_CB_C2BYTES		16	/* # of bytes with C2 errors */
#define	PARANOIA_CB_C2SECS		17	/* # of sectors with C2 errors */
#define	PARANOIA_CB_C2MAXERRS		18	/* Max. # of C2 errors per sector */

/*
 * Cdparanoia modes to be set with paranoia_modeset()
 */
#define	PARANOIA_MODE_FULL		 0xFF	/* Everything except C2CHECK */
#define	PARANOIA_MODE_DISABLE		 0

#define	PARANOIA_MODE_VERIFY		 1	/* Verify data integrity in overlap area */
#define	PARANOIA_MODE_FRAGMENT		 2	/* unsupported */
#define	PARANOIA_MODE_OVERLAP		 4	/* Perform overlapped reads */
#define	PARANOIA_MODE_SCRATCH		 8	/* unsupported */
#define	PARANOIA_MODE_REPAIR		16	/* unsupported */
#define	PARANOIA_MODE_NEVERSKIP		32	/* Do not skip failed reads (retry maxretries) */
#define	PARANOIA_MODE_C2CHECK		256	/* Check C2 error pointer */


#ifndef	CDP_COMPILE
typedef	void    cdrom_paranoia;
#endif

/*
 * The interface from libcdparanoia to the high level caller
 *
 * paranoia_init() is a new function that was not present in the original
 * implementation from Monty which works only on Linux and only with gcc.
 * It is needed to make libparanoia reentrant and useful on systems with
 * a limited linker (e.g. Mac OS X).
 *
 * The first parameter "void *d" is a pointer to the I/O handle for
 * the callback functions. This typically is "SCSI *" when using raw
 * SCSI based I/O.
 *
 * The callbacks do the following:
 *	d_read			read raw audio sectors, return # of sectors read
 *	d_disc_firstsector	return first audio sector on disk
 *	d_disc_lastsector	return last audio sector on disk
 *	d_tracks		return # of audio tracks on disk
 *	d_track_firstsector	return first audio sector for track
 *	d_track_lastsector	return last audio sector for track
 *	d_sector_gettrack	return track number for a sector number
 *	d_track_audiop		return whether track is a data track
 */
extern cdrom_paranoia *paranoia_init	__PR((void * d, int nsectors,
			long	(*d_read)	__PR((void *d, void *buffer,
							long beginsector,
							long sectors)),
			long	(*d_disc_firstsector)	__PR((void *d)),
			long	(*d_disc_lastsector)	__PR((void *d)),
			int	(*d_tracks)		__PR((void *d)),
			long	(*d_track_firstsector) __PR((void *d, int track)),
			long	(*d_track_lastsector)  __PR((void *d, int track)),
			int 	(*d_sector_gettrack) __PR((void *d, long sector)),
			int 	(*d_track_audiop) __PR((void *d, int track))));

/*
 * paranoia_dynoverlapset	set dynamic setcot overlap range
 * paranoia_modeset		set paranoia modes from bit values above
 * paranoia_modeget		get paranoia modes from bit values above
 * paranoia_set_readahead	set # of sectors to be read on a bulk
 * paranoia_get_readahead	get # of sectors to be read on a bulk
 * paranoia_seek		seek to new sector for paranoia operations
 * paranoia_read		read a single audio sector from seek pointer
 * paranoia_read_limited	like paranoia_read() but with retry counter
 * paranoia_free		free allocated data from last operation
 * paranoia_overlapset		set static overlap (disables dynamic overlap)
 *
 * The callback function is called to update statistics information.
 */
extern void	paranoia_dynoverlapset	__PR((cdrom_paranoia * p,
							int minoverlap,
							int maxoverlap));
extern void	paranoia_modeset	__PR((cdrom_paranoia * p, int mode));
extern int	paranoia_modeget	__PR((cdrom_paranoia * p));
extern void	paranoia_set_readahead	__PR((cdrom_paranoia * p,
							int readahead));
extern int	paranoia_get_readahead	__PR((cdrom_paranoia * p));
extern long	paranoia_seek		__PR((cdrom_paranoia * p,
							long seek,
							int mode));
extern Int16_t	*paranoia_read		__PR((cdrom_paranoia * p,
						void (*callback) (long, int)));
extern Int16_t	*paranoia_read_limited	__PR((cdrom_paranoia * p,
						void (*callback) (long, int),
						int maxretries));
extern void	paranoia_free		__PR((cdrom_paranoia * p));
extern void	paranoia_overlapset	__PR((cdrom_paranoia * p,
						long overlap));

#ifndef	HAVE_MEMMOVE
#ifndef _SCHILY_SCHILY_H
#include <schily/schily.h>
#endif
#define	memmove(dst, src, size)		movebytes((src), (dst), (size))
#endif

#endif	/* _CDROM_PARANOIA_H */
