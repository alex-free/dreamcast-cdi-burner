/* @(#)hfs.h	1.6 13/04/28 joerg */
/*
 * hfsutils - tools for reading and writing Macintosh HFS volumes
 * Copyright (C) 1996, 1997 Robert Leslie
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <schily/time.h>
#include <schily/standard.h>
#include <schily/libport.h>	/* Define missing prototypes */

#ifdef APPLE_HYB
#include "hybrid.h"

/* don't need device locking for mkhybrid */
#ifndef NODEVLOCKS
#define NODEVLOCKS
#endif /* NODEVLOCKS */

#endif /* APPLE_HYB */

# define HFS_BLOCKSZ	512
# define HFS_MAX_FLEN	31
# define HFS_MAX_VLEN	27

typedef struct _hfsvol_  hfsvol;
typedef struct _hfsfile_ hfsfile;
typedef struct _hfsdir_  hfsdir;

typedef struct {
  char name[HFS_MAX_VLEN + 1];	/* name of volume */
  int flags;			/* volume flags */
  unsigned long totbytes;	/* total bytes on volume */
  unsigned long freebytes;	/* free bytes on volume */
  time_t crdate;		/* volume creation date */
  time_t mddate;		/* last volume modification date */
} hfsvolent;

/* taken from v3.2.6 */
typedef struct {
  char name[HFS_MAX_FLEN + 1];	/* catalog name (MacOS Standard Roman) */
  int flags;			/* bit flags */
  unsigned long cnid;		/* catalog node id (CNID) */
  unsigned long parid;		/* CNID of parent directory */

  time_t crdate;		/* date of creation */
  time_t mddate;		/* date of last modification */
  time_t bkdate;		/* date of last backup */

  short fdflags;		/* Macintosh Finder flags */

  struct {
    signed short v;		/* Finder icon vertical coordinate */
    signed short h;		/* horizontal coordinate */
  } fdlocation;

  union {
    struct {
      unsigned long dsize;	/* size of data fork */
      unsigned long rsize;	/* size of resource fork */

      char type[5];		/* file type code (plus null) */
      char creator[5];		/* file creator code (plus null) */
    } file;

    struct {
      unsigned short valence;	/* number of items in directory */

      struct {
	signed short top;	/* top edge of folder's rectangle */
	signed short left;	/* left edge */
	signed short bottom;	/* bottom edge */
	signed short right;	/* right edge */
      } rect;

      /* mkhybrid extra */
      signed short view;	/* Folder's view */

      struct {
	signed short v;		/* Scoll vertical position */
	signed short h;		/* Scoll horizontal position */
      } frscroll;
    } dir;
  } u;
} hfsdirent;


# define HFS_ISDIR		0x01
# define HFS_ISLOCKED		0x02

# define HFS_CNID_ROOTPAR	1
# define HFS_CNID_ROOTDIR	2
# define HFS_CNID_EXT		3
# define HFS_CNID_CAT		4
# define HFS_CNID_BADALLOC	5

# define HFS_FNDR_ISONDESK		(1 <<  0)
# define HFS_FNDR_COLOR			0x0e
# define HFS_FNDR_COLORRESERVED		(1 <<  4)
# define HFS_FNDR_REQUIRESSWITCHLAUNCH	(1 <<  5)
# define HFS_FNDR_ISSHARED		(1 <<  6)
# define HFS_FNDR_HASNOINITS		(1 <<  7)
# define HFS_FNDR_HASBEENINITED		(1 <<  8)
# define HFS_FNDR_RESERVED		(1 <<  9)
# define HFS_FNDR_HASCUSTOMICON		(1 << 10)
# define HFS_FNDR_ISSTATIONERY		(1 << 11)
# define HFS_FNDR_NAMELOCKED		(1 << 12)
# define HFS_FNDR_HASBUNDLE		(1 << 13)
# define HFS_FNDR_ISINVISIBLE		(1 << 14)
# define HFS_FNDR_ISALIAS		(1 << 15)

extern char *hfs_error;
/*extern unsigned char hfs_charorder[];*/

#ifdef APPLE_HYB
hfsvol *hfs_mount	__PR((hce_mem *, int, int));
#else
hfsvol *hfs_mount	__PR((char *, int, int));
#endif /* APPLE_HYB */

int hfs_flush		__PR((hfsvol *));
void hfs_flushall	__PR((void));
#ifdef APPLE_HYB
int hfs_umount		__PR((hfsvol *, long, long));
#else
int hfs_umount		__PR((hfsvol *));
#endif /* APPLE_HYB */
void hfs_umountall	__PR((void));
hfsvol *hfs_getvol	__PR((char *));
void hfs_setvol		__PR((hfsvol *));

int hfs_vstat		__PR((hfsvol *, hfsvolent *));
#ifdef APPLE_HYB
int hfs_format		__PR((hce_mem *, int, char *));
#else
int hfs_format		__PR((char *, int, char *));
#endif /* APPLE_HYB */

int hfs_chdir		__PR((hfsvol *, char *));
long hfs_getcwd		__PR((hfsvol *));
int hfs_setcwd		__PR((hfsvol *, long));
int hfs_dirinfo		__PR((hfsvol *, long *, char *));

hfsdir *hfs_opendir	__PR((hfsvol *, char *));
int hfs_readdir		__PR((hfsdir *, hfsdirent *));
int hfs_closedir	__PR((hfsdir *));

hfsfile *hfs_open	__PR((hfsvol *, char *));
int hfs_setfork		__PR((hfsfile *, int));
int hfs_getfork		__PR((hfsfile *));
long hfs_read		__PR((hfsfile *, void *, unsigned long));
long hfs_write		__PR((hfsfile *, void *, unsigned long));
int hfs_truncate	__PR((hfsfile *, unsigned long));
long hfs_lseek		__PR((hfsfile *, long, int));
#ifdef APPLE_HYB
int hfs_close		__PR((hfsfile *, long, long));
#else
int hfs_close		__PR((hfsfile *));
#endif /* APPLE_HYB */

int hfs_stat		__PR((hfsvol *, char *, hfsdirent *));
int hfs_fstat		__PR((hfsfile *, hfsdirent *));
int hfs_setattr		__PR((hfsvol *, char *, hfsdirent *));
int hfs_fsetattr	__PR((hfsfile *, hfsdirent *));

int hfs_mkdir		__PR((hfsvol *, char *));
int hfs_rmdir		__PR((hfsvol *, char *));

int hfs_create		__PR((hfsvol *, char *, char *, char *));
int hfs_delete		__PR((hfsvol *, char *));

int hfs_rename		__PR((hfsvol *, char *, char *));

#ifdef APPLE_HYB
unsigned short hfs_get_drAllocPtr	__PR((hfsfile *));
int hfs_set_drAllocPtr			__PR((hfsfile *, unsigned short, int size));
void hfs_vsetbless			__PR((hfsvol *, unsigned long));
#endif /* APPLE_HYB */
