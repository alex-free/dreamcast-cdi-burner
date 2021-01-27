/* @(#)volume.c	1.25 10/12/19 joerg, Copyright 1997, 1998, 1999, 2000 James Pearson, Copyright 2004-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)volume.c	1.25 10/12/19 joerg, Copyright 1997, 1998, 1999, 2000 James Pearson, Copyright 2004-2010 J. Schilling";
#endif
/*
 *      Copyright (c) 1997, 1998, 1999, 2000 James Pearson
 *	Copyright (c) 2004-2010 J. Schilling
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
 *	volume.c: prepare HFS volume for mkhybrid
 *
 *	James Pearson 17/7/97
 *	modified JCP 29/7/97 to improve allocation sizes to cut
 *	down on wasted space. Now uses the HFS "allocation" size rounded
 *	up to the nearest 2048 bytes. Savings can be significant with
 *	a large volume containing lots of smallish files.
 *
 *	Updated for v1.12 - now uses the built in RELOCATED_DIRECTORY
 *	flag for finding the real directory location JCP 8/1/97
 */

#ifdef APPLE_HFS_HYB

#include "mkisofs.h"
#include <schily/errno.h>
#include <schily/schily.h>

#define	HFS_MIN_SIZE	1600	/* 800k == 1600 HFS blocks */

LOCAL hfsvol  *vol_save = 0;	/* used to "destroy" an HFS volume */

LOCAL	int	AlcSiz		__PR((Ulong));
LOCAL	int	XClpSiz		__PR((Ulong));
LOCAL	int	get_vol_size	__PR((int));
EXPORT	int	write_fork	__PR((hfsfile * hfp, long tot));
EXPORT	int	make_mac_volume	__PR((struct directory *, UInt32_t));
LOCAL	int	copy_to_mac_vol	__PR((hfsvol *, struct directory *));
LOCAL void	set_dir_info	__PR((hfsvol *, struct directory *));

/*
 *	AlcSiz: find allocation size for given volume size
 */
LOCAL int
AlcSiz(vlen)
	Ulong	vlen;
{
	int	lpa,
		drAlBlkSiz;

	/* code extracted from hfs_format() */
	lpa = 1 + vlen / 65536;
	drAlBlkSiz = lpa * HFS_BLOCKSZ;

	/*
	 * now set our "allocation size" to the allocation block rounded up to
	 * the nearest SECTOR_SIZE (2048 bytes)
	 */
	drAlBlkSiz = ROUND_UP(drAlBlkSiz, SECTOR_SIZE);

	return (drAlBlkSiz);
}

/*
 *	XClpSiz: find the default size of the catalog/extent file
 */
LOCAL int
XClpSiz(vlen)
	Ulong	vlen;
{
	int	olpa,
		lpa,
		drNmAlBlks,
		drAlBlkSiz;
	int	vbmsz,
		drXTClpSiz;

	/* code extracted from hfs_format() */

	/* get the lpa from our calculated allocation block size */
	drAlBlkSiz = AlcSiz(vlen);
	lpa = drAlBlkSiz / HFS_BLOCKSZ;

	vbmsz = (vlen / lpa + 4095) / 4096;
	drNmAlBlks = (vlen - 5 - vbmsz) / lpa;
	drXTClpSiz = drNmAlBlks / 128 * drAlBlkSiz;

	/* override the drXTClpSiz size for large volumes */
	if (drXTClpSiz > hce->max_XTCsize) {
		drXTClpSiz = hce->max_XTCsize;
	} else {
		/*
		 * make allowances because we have possibly rounded up the
		 * allocation size get the "original" lpa "
		 */
		olpa = 1 + vlen / 65536;

		/* adjust size upwards */
		drXTClpSiz = ((Ullong)drXTClpSiz * lpa) / olpa;
	}

	/* round up to the nearest allocation size */
	drXTClpSiz = ROUND_UP(drXTClpSiz, drAlBlkSiz);

	return (drXTClpSiz);
}

/*
 *	get_vol_size: get the size of the volume including the extent/catalog
 */
LOCAL int
get_vol_size(vblen)
	int	vblen;
{
	int	drXTClpSiz;
	int	drAlBlkSiz;
	int	new_vblen;

	/*
	 * try to estimate a "volume size" based on the code in hfs_format
	 * - we need the size of the catalog/extents and Desktop files included
	 * in the volume, as we add this to the end of the ISO volume
	 */
	drXTClpSiz = XClpSiz(vblen);
	drAlBlkSiz = AlcSiz(vblen);

	/*
	 * catalog file is set at CTC times (default twice) the extents
	 * file size - hence the (ctc_size + 1) below. The Desktop starts of
	 * the same size as the "clump size" == 4 x drAlBlkSiz,
	 * plus a spare drAlBlkSiz for the alternative MDB
	 */
	new_vblen = vblen +
	    ((hce->ctc_size + 1) * drXTClpSiz + 5 * drAlBlkSiz) / HFS_BLOCKSZ;

	return (new_vblen);
}

/*
 *	write_fork: "write" file data to the volume
 *
 *	This is used to update the HFS file internal structures
 *	but no data is actually written (it's trapped deep down in
 *	libhfs).
 */
EXPORT int
write_fork(hfp, tot)
	hfsfile	*hfp;
	long	tot;
{
	char		blk[HFS_BLOCKSZ];
	unsigned short	start;
	long		len;

	len = tot;
	/* we need to know where this fork starts */
	start = hfs_get_drAllocPtr(hfp);

	/* loop through the data a block at a time */
	while (len >= HFS_BLOCKSZ) {
		if (hfs_write(hfp, blk, HFS_BLOCKSZ) < 0)
			return (-1);
		len -= HFS_BLOCKSZ;
	}
	/* write out anything left */
	if (len)
		if (hfs_write(hfp, blk, len) < 0)
			return (-1);

	/*
	 * set the start of the allocation search to be immediately after
	 * this fork
	 */
	hfs_set_drAllocPtr(hfp, start, tot);

	return (0);
}

/*
 *	make_mac_volume: "create" an HFS volume using the ISO data
 *
 *	The HFS volume structures are set up (but no data is written yet).
 *
 *	ISO volumes have a allocation size of 2048 bytes - regardless
 *	of the size of the volume. HFS allocation size is depends on volume
 *	size, so we may have to update the ISO structures to add in any
 *	padding.
 */
EXPORT int
make_mac_volume(dpnt, start_extent)
	struct directory	*dpnt;
	UInt32_t		start_extent;
{
	char	vol_name[HFS_MAX_VLEN + 1];	/* Mac volume name */
	hfsvol	*vol;			/* Mac volume */
	int	vblen;			/* vol length (HFS blocks) */
	int	Csize,
		lastCsize;		/* allocation sizes */
	int	ret = 0;		/* return value */
	int	loop = 1;

	/* umount volume if we have had a previous attempt */
	if (vol_save)
		if (hfs_umount(vol_save, 0, hfs_lock) < 0)
			return (-1);

	/* set the default clump size to the ISO block size */
	Csize = lastCsize = SECTOR_SIZE;

	if (verbose > 1)
		fprintf(stderr, _("Creating HFS Volume info\n"));

	/* name or copy ISO volume name to Mac Volume name */
	strncpy(vol_name, hfs_volume_id ? hfs_volume_id : volume_id,
								HFS_MAX_VLEN);
	vol_name[HFS_MAX_VLEN] = '\0';

	/* get initial size of HFS volume (size of current ISO volume) */
	vblen = (last_extent - session_start) * HFS_BLK_CONV;

	/* make sure volume is at least 800k */
	if (vblen < HFS_MIN_SIZE)
		vblen += insert_padding_file(HFS_MIN_SIZE - vblen);

	/*
	 * add on size of extents/catalog file, but this may mean the
	 * allocation size will change, so loop round until the
	 * allocation size doesn't change
	 */
	while (loop) {
		hce->XTCsize = XClpSiz(vblen);
		vblen = get_vol_size(vblen);
		Csize = AlcSiz(vblen);

		if (Csize == lastCsize) {
			/* allocation size hasn't changed, so carry on */
			loop = 0;
		} else {
			/*
			 * allocation size has changed, so update
			 * ISO volume size
			 */
			if ((vblen = get_adj_size(Csize)) < 0) {
				sprintf(hce->error,
					_("too many files for HFS volume"));
				return (-1);
			}
			vblen +=
				ROUND_UP((start_extent - session_start) *
						HFS_BLK_CONV, Csize);
			lastCsize = Csize;
		}
	}

	/* take off the label/map size */
	vblen -= hce->hfs_map_size;

	hce->hfs_vol_size = vblen;

	/* set the default allocation size for libhfs */
	hce->Csize = Csize;

	/* format and mount the "volume" */
	if (hfs_format(hce, 0, vol_name) < 0) {
		sprintf(hce->error, _("can't HFS format %s"), vol_name);
		return (-1);
	}
	/*
	 * update the ISO structures with new start extents and any
	 * padding required
	 */
	if (Csize != SECTOR_SIZE) {
		last_extent = adj_size(Csize, start_extent,
					hce->hfs_hdr_size + hce->hfs_map_size);
		adj_size_other(dpnt);
	}
	if ((vol = hfs_mount(hce, 0, 0)) == 0) {
		sprintf(hce->error, _("can't HFS mount %s"), vol_name);
		return (-1);
	}
	/* save the volume for possible later use */
	vol_save = vol;

	/*
	 * Recursively "copy" the files to the volume
	 * - we need to know the first allocation block in the volume as
	 * starting blocks of files are relative to this.
	 */
	ret = copy_to_mac_vol(vol, dpnt);
	if (ret < 0)
		return (ret);

	/*
	 * make the Desktop files - I *think* this stops the Mac rebuilding the
	 * desktop when the CD is mounted on a Mac These will be ignored if they
	 * already exist
	 */
	if (create_dt)
		ret = make_desktop(vol,
				(last_extent - session_start) * HFS_BLK_CONV);
	if (ret < 0)
		return (ret);

	/* close the volume */
	if (hfs_flush(vol) < 0)
		return (-1);

	/* unmount and set the start blocks for the catalog/extents files */
	if (hfs_umount(vol, (last_extent - session_start) * HFS_BLK_CONV, hfs_lock) < 0)
		return (-1);

	return (Csize);
}

#define	TEN 10	/* well, it is! */
#define	LCHAR "_"

/*
 *	copy_to_mac_vol: copy all files in a directory to corresponding
 *			 Mac folder.
 *
 *	Files are copied recursively to corresponding folders on the Mac
 *	volume. The caller routine needs to do a hfs_chdir before calling this
 *	routine.
 */
LOCAL int
copy_to_mac_vol(vol, node)
	hfsvol		*vol;
	struct directory *node;
{
	struct directory_entry	*s_entry;	/* ISO directory entry */
	struct directory_entry	*s_entry1;	/* tmp ISO directory entry */
	struct directory	*dpnt;		/* ISO directory */

	hfsfile			*hfp;		/* HFS file */
	hfsdirent		*ent;		/* HFS file entities */
	long			id;		/* current HFS folder */
	long			dext,
				rext;		/* real data/rsrc start blk */
	int			ret;		/* result code */
	int			new_name;	/* HFS file has modified name */

	int			tens;
	int			digits;
	int			i;

	/* store the current HFS directory ID */
	if ((id = hfs_getcwd(vol)) == 0)
		return (-1);

	if (verbose > 1)
		fprintf(stderr, _("HFS scanning %s\n"), node->whole_name);

	/* loop through the ISO directory entries and process files */
	for (s_entry = node->contents; s_entry; s_entry = s_entry->next) {
		/* ignore directory and associated (rsrc) files */
		if (s_entry->isorec.flags[0] & (ISO_DIRECTORY|ISO_ASSOCIATED))
			continue;

		/* ignore any non-Mac type file */
		if (!s_entry->hfs_ent)
			continue;

		/*
		 * ignore if from a previous session
		 * - should be trapped above
		 */
		if (s_entry->starting_block < session_start)
			continue;

#ifdef DEBUG
		fprintf(stderr, " Name = %s", s_entry->whole_name);
		fprintf(stderr, "   Startb =  %d\n", s_entry->starting_block);
#endif	/* DEBUG */

		ent = s_entry->hfs_ent;

		/* create file */
		i = HFS_MAX_FLEN - strlen(ent->name);
		new_name = 0;
		tens = TEN;
		digits = 1;

		while (1) {
			/*
			 * try to open file - if it exists,
			 * then append '_' to the name and try again
			 */
			errno = 0;
			if ((hfs_create(vol, ent->name, ent->u.file.type,
						ent->u.file.creator)) < 0) {
				if (errno != EEXIST) {
					/*
					 * not an "exist" error, or we can't
					 * append as the filename is already
					 * HFS_MAX_FLEN chars
					 */
					sprintf(hce->error,
						_("can't HFS create file %s %s"),
						s_entry->whole_name, ent->name);
					return (-1);
				} else if (i == 0) {
					/*
					 * File name at max HFS length
					 * - make unique name
					 */
					if (!new_name)
						new_name++;

					sprintf(ent->name +
						HFS_MAX_FLEN - digits - 1,
						"%s%d", LCHAR, new_name);
					new_name++;
					if (new_name == tens) {
						tens *= TEN;
						digits++;
					}
				} else {
					/* append '_' to get new name */
					strcat(ent->name, LCHAR);
					i--;
					new_name = 1;
				}
			} else
				break;
		}

		/* warn that we have a new name */
		if (new_name && verbose > 0) {
			fprintf(stderr, _("Using HFS name: %s for %s\n"),
				ent->name,
				s_entry->whole_name);
		}
		/* open file */
		if ((hfp = hfs_open(vol, ent->name)) == 0) {
			sprintf(hce->error, _("can't HFS open %s"),
				s_entry->whole_name);
			return (-1);
		}
		/* if it has a data fork, then "write" it out */
		if (ent->u.file.dsize)
			write_fork(hfp, ent->u.file.dsize);

		/* if it has a resource fork, set the fork and "write" it out */
		if (ent->u.file.rsize) {
			if ((hfs_setfork(hfp, 1)) < 0)
				return (-1);
			write_fork(hfp, ent->u.file.rsize);
		}

		/* make file invisible if ISO9660 hidden */
		if (s_entry->de_flags & HIDDEN_FILE)
			ent->fdflags |= HFS_FNDR_ISINVISIBLE;

		/* update any HFS file attributes */
		if ((hfs_fsetattr(hfp, ent)) < 0) {
			sprintf(hce->error, _("can't HFS set attributes %s"),
				s_entry->whole_name);
			return (-1);
		}
		/*
		 * get the ISO starting block of data fork (may be zero)
		 * and convert to the equivalent HFS block
		 */
		if (ent->u.file.dsize) {
			dext = (s_entry->starting_block - session_start) *
								HFS_BLK_CONV;
		} else {
			dext = 0;
		}

		/*
		 * if the file has a resource fork (associated file),
		 * get it's ISO starting block and convert as above
		 */
		if (s_entry->assoc && ent->u.file.rsize) {
			rext =
			    (s_entry->assoc->starting_block - session_start) *
								HFS_BLK_CONV;
		} else {
			rext = 0;
		}

		/* close the file and update the starting blocks */
		if (hfs_close(hfp, dext, rext) < 0) {
			sprintf(hce->error, _("can't HFS close file %s"),
				s_entry->whole_name);
			return (-1);
		}
	}

	/* set folder info and custom icon (if it exists) */
	set_dir_info(vol, node);

	/*
	 * process sub-directories  - have a slight problem here,
	 * if the directory had been relocated, then we need to find the
	 * real directory - we do this by first finding the
	 * real directory_entry, and then finding it's directory info
	 */

	/* following code taken from joliet.c */
	for (s_entry = node->contents; s_entry; s_entry = s_entry->next) {
		if ((s_entry->de_flags & RELOCATED_DIRECTORY) != 0) {
			/*
			 * if the directory has been reloacted, then search the
			 * relocated directory for the real entry
			 */
			for (s_entry1 = reloc_dir->contents; s_entry1;
						s_entry1 = s_entry1->next) {
				if (s_entry1->parent_rec == s_entry)
					break;
			}

			/* have a problem - can't find the real directory */
			if (s_entry1 == NULL) {
				sprintf(hce->error,
					_("can't locate relocated directory %s"),
					s_entry->whole_name);
				return (-1);
			}
		} else
			s_entry1 = s_entry;

		/* now have the correct entry - now find the actual directory */
		if ((s_entry1->isorec.flags[0] & ISO_DIRECTORY) &&
				strcmp(s_entry1->name, ".") != 0 &&
				strcmp(s_entry1->name, "..") != 0) {
			if ((s_entry->de_flags & RELOCATED_DIRECTORY) != 0)
				dpnt = reloc_dir->subdir;
			else
				dpnt = node->subdir;

			while (1) {
				if (dpnt->self == s_entry1)
					break;
				dpnt = dpnt->next;
				if (!dpnt) {
					sprintf(hce->error,
					    _("can't find directory location %s"),
							s_entry1->whole_name);
					return (-1);
				}
			}
			/*
			 * now have the correct directory
			 * - so do the HFS stuff
			 */
			ent = dpnt->hfs_ent;

			/*
			 * if we don't have hfs entries, then this is a "deep"
			 * directory - this will be processed later
			 */
			if (!ent)
				continue;

			/* make sub-folder */
			i = HFS_MAX_FLEN - strlen(ent->name);
			new_name = 0;
			tens = TEN;
			digits = 1;

			while (1) {
				/*
				 * try to create new directory
				 * - if it exists, then append '_' to the name
				 * and try again
				 */
				errno = 0;
				if (hfs_mkdir(vol, ent->name) < 0) {
					if (errno != EEXIST) {
						/*
						 * not an "exist" error,
						 * or we can't append as the
						 * filename is already
						 * HFS_MAX_FLEN chars
						 */
						sprintf(hce->error,
						    _("can't HFS create folder %s"),
							s_entry->whole_name);
						return (-1);
					} else if (i == 0) {
						/*
						 * File name at max HFS length
						 * - make unique name
						 */
						if (!new_name)
							new_name++;

						sprintf(ent->name +
						    HFS_MAX_FLEN - digits - 1,
						    "%s%d", LCHAR, new_name);
						new_name++;
						if (new_name == tens) {
							tens *= TEN;
							digits++;
						}
					} else {
						/* append '_' to get new name */
						strcat(ent->name, LCHAR);
						i--;
						new_name = 1;
					}
				} else
					break;
			}

			/* warn that we have a new name */
			if (new_name && verbose > 0) {
				fprintf(stderr, _("Using HFS name: %s for %s\n"),
							ent->name,
							s_entry->whole_name);
			}
			/* see if we need to "bless" this folder */
			if (hfs_bless &&
			    strcmp(s_entry->whole_name, hfs_bless) == 0) {
				hfs_stat(vol, ent->name, ent);
				hfs_vsetbless(vol, ent->cnid);
				if (verbose > 0) {
					fprintf(stderr, _("Blessing %s (%s)\n"),
							ent->name,
							s_entry->whole_name);
				}
				/* stop any further checks */
				hfs_bless = NULL;
			}
			/* change to sub-folder */
			if (hfs_chdir(vol, ent->name) < 0)
				return (-1);

			/* recursively copy files ... */
			ret = copy_to_mac_vol(vol, dpnt);
			if (ret < 0)
				return (ret);

			/* change back to this folder */
			if (hfs_setcwd(vol, id) < 0)
				return (-1);
		}
	}

	return (0);
}

/*
 *	set_dir_info:	Set directory info for a file - also use a custom
 *			Icon - if it exists.
 *
 *	Sets folder' layout (window layout, view, scroll bars etc)
 *
 *	Set the 'HFS_FNDR_HASCUSTOMICON' bit of the folder flags
 *	if a file called 'Icon\r' exists in the folder
 *
 *	Also makes sure the Icon file is invisible
 *	Don't worry if any of this fails ...
 *
 *	Thanks to Rob Leslie <rob@mars.org> for how to do this.
 */

#define	ICON	"Icon"

LOCAL void
set_dir_info(vol, de)
	hfsvol			*vol;
	struct directory	*de;
{
	hfsdirent	*ent = de->hfs_ent;
	hfsdirent	ent1;
	char		name[HFS_MAX_FLEN + 1];
	unsigned short	flags = 0;

	memset(&ent1, 0, sizeof (hfsdirent));

	sprintf(name, "%s\r", ICON);

	/* get the attributes for the Icon file */
	if (hfs_stat(vol, name, &ent1) == 0) {

		/* make sure it is invisible */
		ent1.fdflags |= HFS_FNDR_ISINVISIBLE;

		/* set the new attributes for the Icon file */
		hfs_setattr(vol, name, &ent1);

		/* flag the folder as having a custom icon */
		flags |= HFS_FNDR_HASCUSTOMICON;
	}

	/* make the current folder invisible if ISO9660 hidden */
	if (de->self->de_flags & HIDDEN_FILE) {
		flags |= HFS_FNDR_ISINVISIBLE;
	}

	/* may not have an hfs_ent for this directory */
	if (ent == NULL) {
		ent = &ent1;
		memset(ent, 0, sizeof (hfsdirent));

		/* get the attributes for the folder */
		if (hfs_stat(vol, ":", ent) < 0)
			return;
	}

	/* set HFS_FNDR_HASCUSTOMICON/HFS_FNDR_ISINVISIBLE if needed */
	ent->fdflags |= flags;

	/* set the new attributes for the folder */
	if (hfs_setattr(vol, ":", ent) < 0) {
		/*
		 * Only needed if we add things after this if statement.
		 */
/*		return;*/
	}
}

#endif	/* APPLE_HFS_HYB */
