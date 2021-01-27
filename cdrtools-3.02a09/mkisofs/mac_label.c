/* @(#)mac_label.c	1.21 16/10/10 joerg, Copyright 1997-2000 James Pearson, Copyright 2004-2016 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)mac_label.c	1.21 16/10/10 joerg, Copyright 1997-2000 James Pearson, Copyright 2004-2016 J. Schilling";
#endif
/*
 *      Copyright (c) 1997, 1998, 1999, 2000 James Pearson
 *	Copyright (c) 2004-2016 J. Schilling
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
 *	mac_label.c: generate Mactintosh partition maps and label
 *
 *	Taken from "mkisofs 1.05 PLUS" by Andy Polyakov <appro@fy.chalmers.se>
 *	(see http://fy.chalmers.se/~appro/mkisofs_plus.html for details)
 *
 *	The format of the HFS driver file:
 *
 *	HFS CD Label Block				512 bytes
 *	Driver Partition Map (for 2048 byte blocks)	512 bytes
 *	Driver Partition Map (for 512 byte blocks)	512 bytes
 *	Empty						512 bytes
 *	Driver Partition				N x 2048 bytes
 *	HFS Partition Boot Block			1024 bytes
 *
 *	File of the above format can be extracted from a CD using
 *	apple_driver.c
 *
 *	James Pearson 16/5/98
 */

/* PREP_BOOT Troy Benjegerdes 2/4/99 */

#include "mkisofs.h"
#include "mac_label.h"
#include "apple.h"
#include <schily/schily.h>

#ifdef PREP_BOOT
void	gen_prepboot_label	__PR((unsigned char *ml));

#endif	/* PREP_BOOT */
#ifdef	APPLE_HYB
int	gen_mac_label		__PR((defer * mac_boot));
int	autostart		__PR((void));
#endif	/* APPLE_HYB */

#ifdef PREP_BOOT
void
gen_prepboot_label(ml)
	unsigned char	*ml;
{
	struct directory_entry *de;
	int		i = 0;
	int		block;
	int		size;
	MacLabel	*mac_label = (MacLabel *) ml;

	if (verbose > 1) {
		fprintf(stderr, _("Creating %d PReP boot partition(s)\n"),
						use_prep_boot + use_chrp_boot);
	}
	mac_label->fdiskMagic[0] = fdiskMagic0;
	mac_label->fdiskMagic[1] = fdiskMagic1;

	if (use_chrp_boot) {
		fprintf(stderr, _("CHRP boot partition 1\n"));

		mac_label->image[i].boot = 0x80;

		mac_label->image[i].CHSstart[0] = 0xff;
		mac_label->image[i].CHSstart[1] = 0xff;
		mac_label->image[i].CHSstart[2] = 0xff;

		mac_label->image[i].type = chrpPartType;	/* 0x96 */

		mac_label->image[i].CHSend[0] = 0xff;
		mac_label->image[i].CHSend[1] = 0xff;
		mac_label->image[i].CHSend[2] = 0xff;

		mac_label->image[i].startSect[0] = 0;
		mac_label->image[i].startSect[1] = 0;
		mac_label->image[i].startSect[2] = 0;
		mac_label->image[i].startSect[3] = 0;

		size = (last_extent - session_start) * 2048 / 512;
		mac_label->image[i].size[0] = size & 0xff;
		mac_label->image[i].size[1] = (size >> 8) & 0xff;
		mac_label->image[i].size[2] = (size >> 16) & 0xff;
		mac_label->image[i].size[3] = (size >> 24) & 0xff;

		i++;
	}

	for (; i < use_prep_boot + use_chrp_boot; i++) {
		de = search_tree_file(root, prep_boot_image[i - use_chrp_boot]);
		if (!de) {
			ex_boot_enoent(_("image"),
				prep_boot_image[i - use_chrp_boot]);
			/* NOTREACHED */
		}
		/* get size and block in 512-byte blocks */
		block = get_733(de->isorec.extent) * 2048 / 512;
		size = get_733(de->isorec.size) / 512 + 1;
		fprintf(stderr, _("PReP boot partition %d is \"%s\"\n"),
			i + 1, prep_boot_image[i - use_chrp_boot]);

		mac_label->image[i].boot = 0x80;

		mac_label->image[i].CHSstart[0] = 0xff;
		mac_label->image[i].CHSstart[1] = 0xff;
		mac_label->image[i].CHSstart[2] = 0xff;

		mac_label->image[i].type = prepPartType;	/* 0x41 */

		mac_label->image[i].CHSend[0] = 0xff;
		mac_label->image[i].CHSend[1] = 0xff;
		mac_label->image[i].CHSend[2] = 0xff;

		/* deal with  endianess */
		mac_label->image[i].startSect[0] = block & 0xff;
		mac_label->image[i].startSect[1] = (block >> 8) & 0xff;
		mac_label->image[i].startSect[2] = (block >> 16) & 0xff;
		mac_label->image[i].startSect[3] = (block >> 24) & 0xff;

		mac_label->image[i].size[0] = size & 0xff;
		mac_label->image[i].size[1] = (size >> 8) & 0xff;
		mac_label->image[i].size[2] = (size >> 16) & 0xff;
		mac_label->image[i].size[3] = (size >> 24) & 0xff;
	}
	for (; i < 4; i++) {
		mac_label->image[i].CHSstart[0] = 0xff;
		mac_label->image[i].CHSstart[1] = 0xff;
		mac_label->image[i].CHSstart[2] = 0xff;

		mac_label->image[i].CHSend[0] = 0xff;
		mac_label->image[i].CHSend[1] = 0xff;
		mac_label->image[i].CHSend[2] = 0xff;
	}
}

#endif	/* PREP_BOOT */

#ifdef	APPLE_HYB
int
gen_mac_label(mac_boot)
	defer		*mac_boot;
{
	FILE		*fp;
	MacLabel	*mac_label;
	MacPart		*mac_part;
	char		*buffer = (char *)hce->hfs_map;
	int		block_size;
	int		have_hfs_boot = 0;
	char		tmp[SECTOR_SIZE];
	struct stat	stat_buf;
	mac_partition_table mpm[2];
	int		mpc = 0;
	int		i;

	/* If we have a boot file, then open and check it */
	if (mac_boot->name) {
		if (stat(mac_boot->name, &stat_buf) < 0) {
			sprintf(hce->error, _("unable to stat HFS boot file %s"),
								mac_boot->name);
			return (-1);
		}
		if ((fp = fopen(mac_boot->name, "rb")) == NULL) {
			sprintf(hce->error, _("unable to open HFS boot file %s"),
								mac_boot->name);
			return (-1);
		}
		if (fread(tmp, 1, SECTOR_SIZE, fp) != SECTOR_SIZE) {
			sprintf(hce->error, _("unable to read HFS boot file %s"),
								mac_boot->name);
			fclose(fp);
			return (-1);
		}
		/* check we have a bootable partition */
		mac_part = (MacPart *)(tmp + HFS_BLOCKSZ);

		if (!(IS_MAC_PART(mac_part) &&
		    strncmp((char *)mac_part->pmPartType, pmPartType_2, 12) == 0)) {
			sprintf(hce->error, _("%s is not a HFS boot file"),
								mac_boot->name);
			fclose(fp);
			return (-1);
		}
		/* check we have a boot block as well - last 2 blocks of file */

		if (fseek(fp, (off_t)-2 * HFS_BLOCKSZ, SEEK_END) != 0) {
			sprintf(hce->error, _("unable to seek HFS boot file %s"),
								mac_boot->name);
			fclose(fp);
			return (-1);
		}
		/* overwrite (empty) boot block for our HFS volume */
		if (fread(hce->hfs_hdr, 2, HFS_BLOCKSZ, fp) != HFS_BLOCKSZ) {
			sprintf(hce->error, _("unable to read HFS boot block %s"),
								mac_boot->name);
			fclose(fp);
			return (-1);
		}
		fclose(fp);

		/* check boot block is valid */
		if (d_getw((unsigned char *)hce->hfs_hdr) != HFS_BB_SIGWORD) {
			sprintf(hce->error,
				_("%s does not contain a valid boot block"),
								mac_boot->name);
			return (-1);
		}
		/*
		 * collect info about boot file for later user
		 * - skip over the bootfile header
		 */
		mac_boot->size = stat_buf.st_size - SECTOR_SIZE - 2*HFS_BLOCKSZ;
		mac_boot->off = SECTOR_SIZE;
		mac_boot->pad = 0;

		/*
		 * get size in SECTOR_SIZE blocks
		 * - shouldn't need to round up
		 */
		mpm[mpc].size = ISO_BLOCKS(mac_boot->size);

		mpm[mpc].ntype = PM2;
		mpm[mpc].type = (char *)mac_part->pmPartType;
		mpm[mpc].start = mac_boot->extent = last_extent;
		mpm[mpc].name = 0;

		/* flag that we have a boot file */
		have_hfs_boot++;

		/* add boot file size to the total size */
		last_extent += mpm[mpc].size;
		hfs_extra += mpm[mpc].size;

		mpc++;
	}
	/* set info about our hybrid volume */
	mpm[mpc].ntype = PM4;
	mpm[mpc].type = pmPartType_4;
	mpm[mpc].start = hce->hfs_map_size / HFS_BLK_CONV;
	mpm[mpc].size = last_extent - mpm[mpc].start -
			ISO_BLOCKS(mac_boot->size);
	mpm[mpc].name = volume_id;

	mpc++;

	if (verbose > 1)
		fprintf(stderr, _("Creating HFS Label %s %s\n"), mac_boot->name ?
			_("with boot file") : "",
			mac_boot->name ? mac_boot->name : "");

	/* for a bootable CD, block size is SECTOR_SIZE */
	block_size = have_hfs_boot ? SECTOR_SIZE : HFS_BLOCKSZ;

	/* create the CD label */
	mac_label = (MacLabel *)buffer;
	mac_label->sbSig[0] = 'E';
	mac_label->sbSig[1] = 'R';
	set_722((char *)mac_label->sbBlkSize, block_size);
	set_732((char *)mac_label->sbBlkCount,
				last_extent * (SECTOR_SIZE / block_size));
	set_722((char *)mac_label->sbDevType, 1);
	set_722((char *)mac_label->sbDevId, 1);

	/* create the partition map entry */
	mac_part = (MacPart *)(buffer + block_size);
	mac_part->pmSig[0] = 'P';
	mac_part->pmSig[1] = 'M';
	set_732((char *)mac_part->pmMapBlkCnt, mpc + 1);
	set_732((char *)mac_part->pmPyPartStart, 1);
	set_732((char *)mac_part->pmPartBlkCnt, mpc + 1);
	strncpy((char *)mac_part->pmPartName, "Apple",
						sizeof (mac_part->pmPartName));
	strncpy((char *)mac_part->pmPartType, "Apple_partition_map",
						sizeof (mac_part->pmPartType));
	set_732((char *)mac_part->pmLgDataStart, 0);
	set_732((char *)mac_part->pmDataCnt, mpc + 1);
	set_732((char *)mac_part->pmPartStatus, PM_STAT_DEFAULT);

	/* create partition map entries for our partitions */
	for (i = 0; i < mpc; i++) {
		mac_part = (MacPart *)(buffer + (i + 2) * block_size);
		if (mpm[i].ntype == PM2) {
			/* get driver label and patch it */
			memcpy((char *)mac_label, tmp, HFS_BLOCKSZ);
			set_732((char *)mac_label->sbBlkCount,
				last_extent * (SECTOR_SIZE / block_size));
			set_732((char *)mac_label->ddBlock,
				(mpm[i].start) * (SECTOR_SIZE / block_size));
			memcpy((char *)mac_part, tmp + HFS_BLOCKSZ,
								HFS_BLOCKSZ);
			set_732((char *)mac_part->pmMapBlkCnt, mpc + 1);
			set_732((char *)mac_part->pmPyPartStart,
				(mpm[i].start) * (SECTOR_SIZE / block_size));
		} else {
			mac_part->pmSig[0] = 'P';
			mac_part->pmSig[1] = 'M';
			set_732((char *)mac_part->pmMapBlkCnt, mpc + 1);
			set_732((char *)mac_part->pmPyPartStart,
				mpm[i].start * (SECTOR_SIZE / HFS_BLOCKSZ));
			set_732((char *)mac_part->pmPartBlkCnt,
				mpm[i].size * (SECTOR_SIZE / HFS_BLOCKSZ));
			strncpy((char *)mac_part->pmPartName, mpm[i].name,
				sizeof (mac_part->pmPartName));
			strncpy((char *)mac_part->pmPartType, mpm[i].type,
				sizeof (mac_part->pmPartType));
			set_732((char *)mac_part->pmLgDataStart, 0);
			set_732((char *)mac_part->pmDataCnt,
				mpm[i].size * (SECTOR_SIZE / HFS_BLOCKSZ));
			set_732((char *)mac_part->pmPartStatus,
				PM_STAT_DEFAULT);
		}
	}

	if (have_hfs_boot) {	/* generate 512 partition table as well */
		mac_part = (MacPart *) (buffer + HFS_BLOCKSZ);
		if (mpc < 3) {	/* don't have to interleave with 2048 table */
			mac_part->pmSig[0] = 'P';
			mac_part->pmSig[1] = 'M';
			set_732((char *)mac_part->pmMapBlkCnt, mpc + 1);
			set_732((char *)mac_part->pmPyPartStart, 1);
			set_732((char *)mac_part->pmPartBlkCnt, mpc + 1);
			strncpy((char *)mac_part->pmPartName, "Apple",
					sizeof (mac_part->pmPartName));
			strncpy((char *)mac_part->pmPartType,
					"Apple_partition_map",
					sizeof (mac_part->pmPartType));
			set_732((char *)mac_part->pmLgDataStart, 0);
			set_732((char *)mac_part->pmDataCnt, mpc + 1);
			set_732((char *)mac_part->pmPartStatus,
							PM_STAT_DEFAULT);
			mac_part++;	/* +HFS_BLOCKSZ */
		}
		for (i = 0; i < mpc; i++, mac_part++) {
			if (mac_part == (MacPart *)(buffer + SECTOR_SIZE))
				mac_part++;	/* jump over 2048 partition */
						/* entry */
			if (mpm[i].ntype == PM2) {
				memcpy((char *)mac_part, tmp + HFS_BLOCKSZ * 2,
							HFS_BLOCKSZ);
				if (!IS_MAC_PART(mac_part)) {
					mac_part--;
					continue;
				}
				set_732((char *)mac_part->pmMapBlkCnt, mpc+1);
				set_732((char *)mac_part->pmPyPartStart,
				    mpm[i].start * (SECTOR_SIZE / HFS_BLOCKSZ));
			} else {
				mac_part->pmSig[0] = 'P';
				mac_part->pmSig[1] = 'M';
				set_732((char *)mac_part->pmMapBlkCnt, mpc+1);
				set_732((char *)mac_part->pmPyPartStart,
				    mpm[i].start * (SECTOR_SIZE / HFS_BLOCKSZ));
				set_732((char *)mac_part->pmPartBlkCnt,
				    mpm[i].size * (SECTOR_SIZE / HFS_BLOCKSZ));
				strncpy((char *)mac_part->pmPartName,
				    mpm[i].name, sizeof (mac_part->pmPartName));
				strncpy((char *)mac_part->pmPartType,
				    mpm[i].type, sizeof (mac_part->pmPartType));
				set_732((char *)mac_part->pmLgDataStart, 0);
				set_732((char *)mac_part->pmDataCnt,
				    mpm[i].size * (SECTOR_SIZE / HFS_BLOCKSZ));
				set_732((char *)mac_part->pmPartStatus,
							PM_STAT_DEFAULT);
			}
		}
	}
	return (0);
}

/*
 *	autostart: make the HFS CD use the QuickTime 2.0 Autostart feature.
 *
 *	based on information from Eric Eisenhart <eric@sonic.net> and
 *	http://developer.apple.com/qa/qtpc/qtpc12.html and
 *	http://developer.apple.com/dev/techsupport/develop/issue26/macqa.html
 *
 *	The name of the AutoStart file is stored in the area allocated for
 *	the Clipboard name. This area begins 106 bytes into the sector of
 *	block 0, with the first four bytes at that offset containing the
 *	hex value 0x006A7068. This value indicates that an AutoStart
 *	filename follows. After this 4-byte tag, 12 bytes remain, starting
 *	at offset 110. In these 12 bytes, the name of the AutoStart file is
 *	stored as a Pascal string, giving you up to 11 characters to identify
 *	the file. The file must reside in the root directory of the HFS
 *	volume or partition.
 */

int
autostart()
{
	int	len;
	int	i;

	if ((len = strlen(autoname)) > 11)
		return (-1);

	hce->hfs_hdr[106] = 0x00;
	hce->hfs_hdr[107] = 0x6A;
	hce->hfs_hdr[108] = 0x70;
	hce->hfs_hdr[109] = 0x68;
	hce->hfs_hdr[110] = len;

	for (i = 0; i < len; i++)
		hce->hfs_hdr[111 + i] = autoname[i];

	return (0);
}
#endif	/* APPLE_HYB */
