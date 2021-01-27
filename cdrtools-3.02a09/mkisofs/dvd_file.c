/* @(#)dvd_file.c	1.15 15/12/15 joerg */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)dvd_file.c	1.15 15/12/15 joerg";
#endif
/*
 * DVD_AUD_VID code
 *  Copyright (c) 2002 Olaf Beck - olaf_sc@yahoo.com
 *  Copyright (c) 2002-2012 Jörg Schilling <schilling@fokus.gmd.de>
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; see the file COPYING.  If not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifdef DVD_AUD_VID

#include "mkisofs.h"
#include <schily/schily.h>
#include "dvd_reader.h"
#include "dvd_file.h"
#include "ifo_read.h"

LOCAL	void	bsort		__PR((int sector[], int title[], int size));
LOCAL	void	uniq		__PR((int sector[], int title[],
					int title_sets_array[],
					int sector_sets_array[], int titles));
LOCAL	void	DVDFreeFileSetArrays __PR((int *sector, int *title, int *title_sets_array,
					int *sector_sets_array));
EXPORT	void	DVDFreeFileSet	__PR((title_set_info_t *title_set_info));
EXPORT	title_set_info_t *DVDGetFileSet __PR((char *dvd));
EXPORT	int	DVDGetFilePad	__PR((title_set_info_t *title_set_info, char *name));


LOCAL void
bsort(sector, title, size)
	int	sector[];
	int	title[];
	int	size;
{
	int	temp_title;
	int	temp_sector;
	int	i;
	int	j;

	for (i = 0; i < size; i++) {
		for (j = 0; j < size; j++) {
			if (sector[i] < sector[j]) {
				temp_sector = sector[i];
				temp_title = title[i];
				sector[i] = sector[j];
				title[i] = title[j];
				sector[j] = temp_sector;
				title[j] = temp_title;
			}
		}
	}
}


LOCAL void
uniq(sector, title, title_sets_array, sector_sets_array, titles)
	int	sector[];
	int	title[];
	int	title_sets_array[];
	int	sector_sets_array[];
	int	titles;
{
	int	i;
	int	j;


	for (i = 0, j = 0; j < titles; ) {
		if (sector[j] != sector[j+1]) {
			title_sets_array[i]  = title[j];
			sector_sets_array[i] = sector[j];
#ifdef DEBUG
			fprintf(stderr, _("Sector offset is %d\n"), sector_sets_array[i]);
#endif
			i++;
			j++;
		} else {
			do {
				if (j < titles)
					j++;

			} while (sector[j] == sector[j+1]);

		}
	}

}

LOCAL void
DVDFreeFileSetArrays(sector, title, title_sets_array, sector_sets_array)
	int	*sector;
	int	*title;
	int	*title_sets_array;
	int	*sector_sets_array;
{
	free(sector);
	free(title);
	free(title_sets_array);
	free(sector_sets_array);
}

EXPORT void
DVDFreeFileSet(title_set_info)
	title_set_info_t *title_set_info;
{
	free(title_set_info->title_set);
	free(title_set_info);
}

EXPORT title_set_info_t *
DVDGetFileSet(dvd)
	char	*dvd;
{
	/*
	 * TODO  Fix close of files if
	 *	we error out
	 *	We also assume that all
	 *	DVD files are of valid
	 *	size i.e. file%2048 == 0
	 */

	/* title interation */
	int		title_sets;
	int		titles;
	int		counter;
	int		i;

	/* DVD file structures */
	dvd_reader_t	*_dvd = NULL;

	ifo_handle_t	*vmg_ifo = NULL;
	ifo_handle_t	*vts_ifo = NULL;

	dvd_file_t	*vmg_vob_file = NULL;
	dvd_file_t	*vmg_ifo_file = NULL;

	dvd_file_t	*vts_ifo_file = NULL;
	dvd_file_t	*vts_menu_file = NULL;
	dvd_file_t	*vts_title_file = NULL;

	/* The sizes it self of each file */
	int		ifo;
	int		bup;
	int		menu_vob;
	int		title_vob;

	/* Arrays keeping the title - filset relationship */
	int		*sector;
	int		*title;
	int		*title_sets_array;
	int		*sector_sets_array;

	/* DVD Video files */
	struct stat	fileinfo;
	char		temppoint[PATH_MAX + 1];

	/* The Title Set Info struct*/
	title_set_info_t *title_set_info;

	/* Temporary mount point - to be used later */
	char		mountpoint[PATH_MAX + 1];

	strncpy(mountpoint, dvd, sizeof (mountpoint));
	mountpoint[sizeof (mountpoint)-1] = '\0';


	_dvd = DVDOpen(dvd);
	if (!_dvd) {
		errmsgno(EX_BAD, _("Can't open device '%s'.\n"), dvd);
		return (0);
	}
	vmg_ifo = ifoOpen(_dvd, 0);
	if (!vmg_ifo) {
		errmsgno(EX_BAD, _("Can't open VMG info for '%s'.\n"), dvd);
		/* Close the DVD */
		DVDClose(_dvd);
		return (0);
	}

	/* Check mount point */

	snprintf(temppoint, sizeof (temppoint),
				"%s/VIDEO_TS/VIDEO_TS.IFO", mountpoint);


	if (stat(temppoint, &fileinfo) < 0) {
		/* If we can't stat the file, give up */
		errmsg(_("Can't stat '%s'.\n"), temppoint);
		return (0);
	}



	title_sets = vmg_ifo->vmgi_mat->vmg_nr_of_title_sets;
	titles = vmg_ifo->tt_srpt->nr_of_srpts;

	sector = e_malloc(titles * sizeof (int));
	memset(sector, 0, titles * sizeof (int));
	title = e_malloc(titles * sizeof (int));
	title_sets_array = e_malloc(title_sets * sizeof (int));
	sector_sets_array = e_malloc(title_sets * sizeof (int));
	title_set_info = (title_set_info_t *)e_malloc(sizeof (title_set_info_t));
	title_set_info->title_set = (title_set_t *)e_malloc((title_sets + 1) *
							sizeof (title_set_t));

	title_set_info->num_titles = title_sets;


	/* Fill and sort the arrays for titles*/

	if (titles >= 1) {
		for (counter = 0; counter < titles; counter++) {
			sector[counter] = vmg_ifo->tt_srpt->title[counter].title_set_sector;
			title[counter]  = counter + 1;
		}
	}

	/* Yes, we should probably do a better sort than B - but what the heck*/
	bsort(sector, title, titles);


	/*
	 * Since title sets and titles are not the same we will need to sort
	 * out "bogus" titles
	 */

	uniq(sector, title, title_sets_array, sector_sets_array, titles);


	/* Open VIDEO_TS.VOB is present */

	vmg_vob_file = DVDOpenFile(_dvd, 0, DVD_READ_MENU_VOBS);

	/* Check VIDEO_TS title set */

	vmg_ifo_file = DVDOpenFile(_dvd, 0, DVD_READ_INFO_FILE);

	if ((vmg_vob_file == 0) && vmg_ifo->vmgi_mat->vmg_last_sector + 1
			< 2 * DVDFileSize(vmg_ifo_file)) {
		errmsgno(EX_BAD, _("IFO is not of correct size aborting.\n"));
		DVDFreeFileSetArrays(sector, title, title_sets_array,
					sector_sets_array);
		DVDFreeFileSet(title_set_info);
		return (0);
	} else if ((vmg_vob_file != 0) && (vmg_ifo->vmgi_mat->vmg_last_sector
		    + 1  < 2 * DVDFileSize(vmg_ifo_file) +
		    DVDFileSize(vmg_vob_file))) {
		errmsgno(EX_BAD, _("Either VIDEO_TS.IFO or VIDEO_TS.VOB is not of correct size.\n"));
		DVDFreeFileSetArrays(sector, title, title_sets_array,
					sector_sets_array);
		DVDFreeFileSet(title_set_info);
		return (0);
	}

	/* Find the actuall right size of VIDEO_TS.IFO */
	if (vmg_vob_file == 0) {
		if (vmg_ifo->vmgi_mat->vmg_last_sector + 1 > 2
				*  DVDFileSize(vmg_ifo_file)) {
			ifo = vmg_ifo->vmgi_mat->vmg_last_sector
				- DVDFileSize(vmg_ifo_file) + 1;
		} else {
			ifo = vmg_ifo->vmgi_mat->vmgi_last_sector + 1;
		}
	} else {
		if (vmg_ifo->vmgi_mat->vmgi_last_sector + 1
				< vmg_ifo->vmgi_mat->vmgm_vobs) {
			ifo = vmg_ifo->vmgi_mat->vmgm_vobs;
		} else {
			ifo = vmg_ifo->vmgi_mat->vmgi_last_sector + 1;
		}
	}

	title_set_info->title_set[0].size_ifo = ifo * 2048;
	title_set_info->title_set[0].realsize_ifo = fileinfo.st_size;
	title_set_info->title_set[0].pad_ifo = ifo - DVDFileSize(vmg_ifo_file);

	/* Find the actuall right size of VIDEO_TS.VOB */
	if (vmg_vob_file != 0) {
		if (ifo + DVDFileSize(vmg_ifo_file) +
		    DVDFileSize(vmg_vob_file) - 1 <
		    vmg_ifo->vmgi_mat->vmg_last_sector) {
				menu_vob = vmg_ifo->vmgi_mat->vmg_last_sector -
						ifo - DVDFileSize(vmg_ifo_file) + 1;
		} else {
			menu_vob = vmg_ifo->vmgi_mat->vmg_last_sector
			- ifo - DVDFileSize(vmg_ifo_file) + 1;
		}

		snprintf(temppoint, sizeof (temppoint),
				"%s/VIDEO_TS/VIDEO_TS.VOB", mountpoint);
		if (stat(temppoint, &fileinfo) < 0) {
			errmsg(_("calc: Can't stat '%s'.\n"), temppoint);
			DVDFreeFileSetArrays(sector, title, title_sets_array,
						sector_sets_array);
			DVDFreeFileSet(title_set_info);
			return (0);
		}

		title_set_info->title_set[0].realsize_menu = fileinfo.st_size;
		title_set_info->title_set[0].pad_menu = menu_vob -
						DVDFileSize(vmg_vob_file);
		title_set_info->title_set[0].size_menu = menu_vob * 2048;
		DVDCloseFile(vmg_vob_file);
	} else {
		title_set_info->title_set[0].size_menu = 0;
		title_set_info->title_set[0].realsize_menu = 0;
		title_set_info->title_set[0].pad_menu = 0;
		menu_vob = 0;
	}


	/* Finding the actuall right size of VIDEO_TS.BUP */
	if (title_sets >= 1) {
		bup = sector_sets_array[0] - menu_vob - ifo;
	} else {
		/* Just in case we burn a DVD-Video without any title_sets */
		bup = vmg_ifo->vmgi_mat->vmg_last_sector + 1 - menu_vob - ifo;
	}

	/* Never trust the BUP file - use a copy of the IFO */
	snprintf(temppoint, sizeof (temppoint),
				"%s/VIDEO_TS/VIDEO_TS.IFO", mountpoint);

	if (stat(temppoint, &fileinfo) < 0) {
		errmsg(_("calc: Can't stat '%s'.\n"), temppoint);
		DVDFreeFileSetArrays(sector, title, title_sets_array,
					sector_sets_array);
		DVDFreeFileSet(title_set_info);
		return (0);
	}

	title_set_info->title_set[0].realsize_bup = fileinfo.st_size;
	title_set_info->title_set[0].size_bup = bup * 2048;
	title_set_info->title_set[0].pad_bup = bup - DVDFileSize(vmg_ifo_file);

	/* Take care of the titles which we don't have in VMG */

	title_set_info->title_set[0].number_of_vob_files = 0;
	title_set_info->title_set[0].realsize_vob[0] = 0;
	title_set_info->title_set[0].pad_title = 0;

	DVDCloseFile(vmg_ifo_file);

	if (title_sets >= 1) {
		for (counter = 0; counter < title_sets; counter++) {

			vts_ifo = ifoOpen(_dvd, counter + 1);

			if (!vts_ifo) {
				errmsgno(EX_BAD, _("Can't open VTS info.\n"));
				DVDFreeFileSetArrays(sector, title,
					title_sets_array, sector_sets_array);
				DVDFreeFileSet(title_set_info);
				return (0);
			}

			snprintf(temppoint, sizeof (temppoint),
				"%s/VIDEO_TS/VTS_%02i_0.IFO",
				mountpoint, counter + 1);

			if (stat(temppoint, &fileinfo) < 0) {
				errmsg(_("calc: Can't stat '%s'.\n"), temppoint);
				DVDFreeFileSetArrays(sector, title,
					title_sets_array, sector_sets_array);
				DVDFreeFileSet(title_set_info);
				return (0);
			}


			/* Test if VTS_XX_0.VOB is present */

			vts_menu_file = DVDOpenFile(_dvd, counter + 1,
					DVD_READ_MENU_VOBS);

			/* Test if VTS_XX_X.VOB are present */

			vts_title_file = DVDOpenFile(_dvd, counter + 1,
						DVD_READ_TITLE_VOBS);

			/* Check VIDEO_TS.IFO */

			vts_ifo_file = DVDOpenFile(_dvd, counter + 1,
							DVD_READ_INFO_FILE);

			/*
			 * Checking that title will fit in the
			 * space given by the ifo file
			 */


			if (vts_ifo->vtsi_mat->vts_last_sector + 1
				< 2 * DVDFileSize(vts_ifo_file)) {
				errmsgno(EX_BAD, _("IFO is not of correct size aborting.\n"));
				DVDFreeFileSetArrays(sector, title,
					title_sets_array, sector_sets_array);
				DVDFreeFileSet(title_set_info);
				return (0);
			} else if ((vts_title_file != 0) &&
				    (vts_menu_file != 0) &&
				    (vts_ifo->vtsi_mat->vts_last_sector + 1
				    < 2 * DVDFileSize(vts_ifo_file) +
				    DVDFileSize(vts_title_file) +
				    DVDFileSize(vts_menu_file))) {
				errmsgno(EX_BAD, _("Either VIDEO_TS.IFO or VIDEO_TS.VOB is not of correct size.\n"));
				DVDFreeFileSetArrays(sector, title,
					title_sets_array, sector_sets_array);
				DVDFreeFileSet(title_set_info);
				return (0);
			} else if ((vts_title_file != 0) &&
				    (vts_menu_file == 0) &&
				    (vts_ifo->vtsi_mat->vts_last_sector + 1
				    < 2 * DVDFileSize(vts_ifo_file) +
				    DVDFileSize(vts_title_file))) {
				errmsgno(EX_BAD, _("Either VIDEO_TS.IFO or VIDEO_TS.VOB is not of correct size.\n"));
				DVDFreeFileSetArrays(sector, title,
					title_sets_array, sector_sets_array);
				DVDFreeFileSet(title_set_info);
				return (0);
			} else if ((vts_menu_file != 0) &&
				    (vts_title_file == 0) &&
				    (vts_ifo->vtsi_mat->vts_last_sector + 1
				    < 2 * DVDFileSize(vts_ifo_file) +
				    DVDFileSize(vts_menu_file))) {
				errmsgno(EX_BAD, _("Either VIDEO_TS.IFO or VIDEO_TS.VOB is not of correct size.\n"));
				DVDFreeFileSetArrays(sector, title,
					title_sets_array, sector_sets_array);
				DVDFreeFileSet(title_set_info);
				return (0);
			}


			/* Find the actuall right size of VTS_XX_0.IFO */
			if ((vts_title_file == 0) && (vts_menu_file == 0)) {
				if (vts_ifo->vtsi_mat->vts_last_sector + 1 >
				    2 * DVDFileSize(vts_ifo_file)) {
					ifo = vts_ifo->vtsi_mat->vts_last_sector
						- DVDFileSize(vts_ifo_file) + 1;
				} else {
					ifo = vts_ifo->vtsi_mat->vts_last_sector
						- DVDFileSize(vts_ifo_file) + 1;
				}
			} else if (vts_title_file == 0) {
				if (vts_ifo->vtsi_mat->vtsi_last_sector + 1 <
				    vts_ifo->vtsi_mat->vtstt_vobs) {
					ifo = vmg_ifo->vtsi_mat->vtstt_vobs;
				} else {
					ifo = vmg_ifo->vtsi_mat->vtstt_vobs;
				}
			} else {
				if (vts_ifo->vtsi_mat->vtsi_last_sector + 1 <
				    vts_ifo->vtsi_mat->vtsm_vobs) {
					ifo = vts_ifo->vtsi_mat->vtsm_vobs;
				} else {
					ifo = vts_ifo->vtsi_mat->vtsi_last_sector + 1;
				}
			}
			title_set_info->title_set[counter + 1].size_ifo =
						ifo * 2048;
			title_set_info->title_set[counter + 1].realsize_ifo =
						fileinfo.st_size;
			title_set_info->title_set[counter + 1].pad_ifo =
						ifo - DVDFileSize(vts_ifo_file);


			/* Find the actuall right size of VTS_XX_0.VOB */
			if (vts_menu_file != 0) {
				if (vts_ifo->vtsi_mat->vtsm_vobs == 0)  {
					/*
					 * Apparently start sector 0 means that
					 * VTS_XX_0.VOB is empty after all...
					 */
					menu_vob = 0;
					if (DVDFileSize(vts_menu_file) != 0) {
						/*
						 * Paranoia: we most likely never
						 * come here...
						 */
						errmsgno(EX_BAD,
							_("%s/VIDEO_TS/VTS_%02i_0.IFO appears to be corrupted.\n"),
							mountpoint, counter+1);
						return (0);
					}
				} else if ((vts_title_file != 0) &&
					(vts_ifo->vtsi_mat->vtstt_vobs -
					vts_ifo->vtsi_mat->vtsm_vobs >
						DVDFileSize(vts_menu_file))) {
					menu_vob = vts_ifo->vtsi_mat->vtstt_vobs -
							vts_ifo->vtsi_mat->vtsm_vobs;
				} else if ((vts_title_file == 0) &&
					    (vts_ifo->vtsi_mat->vtsm_vobs +
					    DVDFileSize(vts_menu_file) +
					    DVDFileSize(vts_ifo_file) - 1 <
					    vts_ifo->vtsi_mat->vts_last_sector)) {
					menu_vob = vts_ifo->vtsi_mat->vts_last_sector
						- DVDFileSize(vts_ifo_file)
						- vts_ifo->vtsi_mat->vtsm_vobs + 1;
				} else {
					menu_vob = vts_ifo->vtsi_mat->vtstt_vobs -
							vts_ifo->vtsi_mat->vtsm_vobs;
				}

				snprintf(temppoint, sizeof (temppoint),
					"%s/VIDEO_TS/VTS_%02i_0.VOB", mountpoint, counter + 1);

				if (stat(temppoint, &fileinfo)  < 0) {
					errmsg(_("calc: Can't stat '%s'.\n"), temppoint);
					DVDFreeFileSetArrays(sector, title,
						title_sets_array, sector_sets_array);
					DVDFreeFileSet(title_set_info);
					return (0);
				}

				title_set_info->title_set[counter + 1].realsize_menu = fileinfo.st_size;
				title_set_info->title_set[counter + 1].size_menu = menu_vob * 2048;
				title_set_info->title_set[counter + 1].pad_menu = menu_vob - DVDFileSize(vts_menu_file);

			} else {
				title_set_info->title_set[counter + 1].size_menu = 0;
				title_set_info->title_set[counter + 1].realsize_menu = 0;
				title_set_info->title_set[counter + 1].pad_menu = 0;
				menu_vob = 0;
			}


			/* Find the actuall total size of VTS_XX_[1 to 9].VOB */

			if (vts_title_file != 0) {
				if (ifo + menu_vob + DVDFileSize(vts_ifo_file) -
				    1 < vts_ifo->vtsi_mat->vts_last_sector) {
				    title_vob = vts_ifo->vtsi_mat->vts_last_sector
						+ 1 - ifo - menu_vob -
						DVDFileSize(vts_ifo_file);
				} else {
					title_vob = vts_ifo->vtsi_mat->vts_last_sector +
						1 - ifo - menu_vob -
						DVDFileSize(vts_ifo_file);
				}
				/*
				 * Find out how many vob files
				 * and the size of them
				 */
				for (i = 0; i < 9; ++i) {
					snprintf(temppoint, sizeof (temppoint),
						"%s/VIDEO_TS/VTS_%02i_%i.VOB",
						mountpoint, counter + 1, i + 1);
					if (stat(temppoint, &fileinfo) < 0) {
						break;
					}
					title_set_info->title_set[counter + 1].realsize_vob[i] = fileinfo.st_size;
				}
				title_set_info->title_set[counter + 1].number_of_vob_files = i;
				title_set_info->title_set[counter + 1].size_title = title_vob * 2048;
				title_set_info->title_set[counter + 1].pad_title = title_vob - DVDFileSize(vts_title_file);
			} else {
				title_set_info->title_set[counter + 1].number_of_vob_files = 0;
				title_set_info->title_set[counter + 1].realsize_vob[0] = 0;
				title_set_info->title_set[counter + 1].size_title = 0;
				title_set_info->title_set[counter + 1].pad_title = 0;
				title_vob = 0;

			}


			/* Find the actuall total size of VTS_XX_0.BUP */
			if (title_sets - 1 > counter) {
				bup = sector_sets_array[counter+1]
					- sector_sets_array[counter]
					- title_vob - menu_vob - ifo;
			} else {
				bup = vts_ifo->vtsi_mat->vts_last_sector + 1
					- title_vob - menu_vob - ifo;
			}

			/* Never trust the BUP use a copy of the IFO */
			snprintf(temppoint, sizeof (temppoint),
				"%s/VIDEO_TS/VTS_%02i_0.IFO",
				mountpoint, counter + 1);

			if (stat(temppoint, &fileinfo) < 0) {
				errmsg(_("calc: Can't stat '%s'\n"), temppoint);
				DVDFreeFileSetArrays(sector, title,
					title_sets_array, sector_sets_array);
				DVDFreeFileSet(title_set_info);
				return (0);
			}

			title_set_info->title_set[counter + 1].size_bup =
						bup * 2048;
			title_set_info->title_set[counter + 1].realsize_bup =
						fileinfo.st_size;
			title_set_info->title_set[counter + 1].pad_bup =
						bup - DVDFileSize(vts_ifo_file);


			/* Closing files */

			if (vts_menu_file != 0) {
				DVDCloseFile(vts_menu_file);
			}

			if (vts_title_file != 0) {
				DVDCloseFile(vts_title_file);
			}


			if (vts_ifo_file != 0) {
				DVDCloseFile(vts_ifo_file);
			}

			ifoClose(vts_ifo);

		}

	}

	DVDFreeFileSetArrays(sector, title, title_sets_array, sector_sets_array);

	/* Close the VMG ifo file we got all the info we need */
	ifoClose(vmg_ifo);


	/* Close the DVD */
	DVDClose(_dvd);

	/* Return the actuall info*/
	return (title_set_info);


}

EXPORT int
DVDGetFilePad(title_set_info, name)
	title_set_info_t *title_set_info;
	char	*name;
{
	char	title_a[3];
	char	vob_a[2];
	int	title;
	int	vob;

	title_a[0] = title_a[1] = title_a[2] = '\0';
	vob_a[0] = vob_a[1] = '\0';

	if (name[0] != 'V') {
		return (0);
	}
	if (memcmp(name, "VIDEO_TS", 8) == 0) {
		if (strstr(name, ".IFO") != 0) {
			return (title_set_info->title_set[0].pad_ifo);
		} else if (strstr(name, ".VOB") != 0) {
			return (title_set_info->title_set[0].pad_menu);
		} else if (strstr(name, ".BUP") != 0) {
			return (title_set_info->title_set[0].pad_bup);
		} else {
			return (0);
		}
	} else if (memcmp(name, "VTS_", 4) == 0) {
		title_a[0] = name[4];
		title_a[1] = name[5];
		title_a[2] = '\0';
		vob_a[0] = name[7];
		vob_a[1] = '\0';
		title = atoi(title_a);
		vob = atoi(vob_a);
		if (title > title_set_info->num_titles) {
			return (0);
		} else {
			if (strstr(name, ".IFO") != 0) {
				return (title_set_info->title_set[title].pad_ifo);
			} else if (strstr(name, ".BUP") != 0) {
				return (title_set_info->title_set[title].pad_bup);
			} else if (vob == 0) {
				return (title_set_info->title_set[title].pad_menu);
			} else if (vob == title_set_info->title_set[title].number_of_vob_files) {
				return (title_set_info->title_set[title].pad_title);
			} else {
				return (0);
			}
		}
	} else {
		return (0);
	}
}

#endif /*DVD_AUD_VID*/
