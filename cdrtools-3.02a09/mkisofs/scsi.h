/* @(#)scsi.h	1.4 16/11/14 Copyright 1997-2016 J. Schilling */
/*
 *	Copyright (c) 1997-2016 J. Schilling
 */
/*@@C@@*/

#ifndef	_SCSI_H
#define	_SCSI_H

/*
 * Implemented inside multi.c in case that USE_SCG has not been defined.
 */
extern int	readsecs	__PR((UInt32_t startsecno, void *buffer, int sectorcount));

#ifdef	USE_SCG
extern int	scsidev_open	__PR((char *path));
extern int	scsidev_close	__PR((void));
#endif

#endif	/* _SCSI_H */
