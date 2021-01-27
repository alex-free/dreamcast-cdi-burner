/* @(#)hybrid.h	1.3 02/10/04 joerg */
/*
**	hybrid.h: extra info needed by libhfs and mkisofs
**
**	James Pearson 15/9/97
*/

#ifndef _HYBRID_H

/*
 *	The following three variables can be overridden at run time
 *	by using the -hfs-parms option i.e. to use the following defaults
 *	you could use: -hfs-parms CTC=2,CTC_LOOP=4,MAX_XTCSIZE=4194304
 *	i.e. to change just MAX_XTCSIZE to 2Mb use:
 *	-hfs-parms MAX_XTCSIZE=2097152
 */

#define	CTC	2		/* factor to increase initial Catalog file
				   size to prevent the file growing */
#define CTC_LOOP 4		/* number of attemps before we give up
				   trying to create the volume */

#define MAX_XTCSIZE 4*1024*1024 /* the maximum size of the Catalog file -
				   as the size of the Catalog file is linked
				   to the size of the volume, multi-gigabyte
				   volumes create very large Catalog and
				   Extent files - 4Mb is the size calculated
				   for a 1Gb volume - which is probably
				   overkill, but seems to be OK */

#define HCE_ERROR -9999		/* dummy errno value for Catalog file
				   size problems */

#define HFS_MAP_SIZE	16	/* size of HFS partition maps (8Kb) */

typedef struct {
  int hfs_ce_size;		/* extents/catalog size in HFS blks */
  int hfs_hdr_size;		/* vol header size in HFS blks */
  int hfs_dt_size;		/* Desktop file size in HFS blks */
  int hfs_tot_size;		/* extents/catalog/dt size in HFS blks */
  int hfs_map_size;		/* size of partition maps in HFS blks */
  unsigned long hfs_vol_size;	/* size of volume in HFS blks */
  unsigned char *hfs_ce;	/* mem copy of extents/catalog files */
  unsigned char *hfs_hdr;	/* mem copy of vol header */
  unsigned char *hfs_alt_mdb;	/* location of alternate MDB */
  unsigned char *hfs_map;	/* location of partiton_maps */
  int Csize;			/* size of allocation unit (bytes) */
  int XTCsize;			/* def size of catalog/extents files (bytes) */
  int max_XTCsize;		/* max size of catalog/extents files (bytes) */
  int ctc_size;			/* factor to increase Catalog file size */
  char *error;			/* HFS error message */
} hce_mem;

#define _HYBRID_H
#endif /* _HYBRID_H */
