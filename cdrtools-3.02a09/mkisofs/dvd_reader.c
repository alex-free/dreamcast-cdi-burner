/* @(#)dvd_reader.c	1.12 15/12/15 joerg */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)dvd_reader.c	1.12 15/12/15 joerg";
#endif
/*
 * Copyright (C) 2001, 2002 Billy Biggs <vektor@dumbterm.net>,
 *                          Håkan Hjort <d95hjort@dtek.chalmers.se>,
 *                          Olaf Beck <olaf_sc@yahoo.com>
 *			    (I only did the cut down no other contribs)
 *			    Jörg Schilling <schilling@fokus.gmd.de>
 *			    (making the code portable)
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * NOTE: This is a cut down version of libdvdread for mkisofs, due
 * to portability issues with the current libdvdread according to
 * the maintainer of mkisofs.
 * This cut down version only reads from a harddisk file structure
 * and it only implements the functions necessary inorder to make
 * mkisofs produce valid DVD-Video images.
 * DON'T USE THIS LIBRARY IN ANY OTHER PROGRAM GET THE REAL
 * LIBDVDREAD INSTEAD
 */
#ifdef DVD_AUD_VID

#include "mkisofs.h"
#include <schily/fcntl.h>
#include <schily/schily.h>

#include "dvd_reader.h"

struct dvd_file_s {
	/* Basic information. */
	dvd_reader_t	*dvd;

	/* Calculated at open-time, size in blocks. */
	ssize_t		filesize;
};


EXPORT	void		DVDCloseFile	__PR((dvd_file_t *dvd_file));
LOCAL	dvd_file_t	*DVDOpenFilePath __PR((dvd_reader_t *dvd, char *filename));
LOCAL	dvd_file_t	*DVDOpenVOBPath	__PR((dvd_reader_t *dvd, int title, int menu));
EXPORT	dvd_file_t	*DVDOpenFile	__PR((dvd_reader_t *dvd, int titlenum,
						dvd_read_domain_t domain));
LOCAL	dvd_reader_t	*DVDOpenPath	__PR((const char *path_root));
EXPORT	dvd_reader_t	*DVDOpen		__PR((const char *path));
EXPORT	void		DVDClose	__PR((dvd_reader_t *dvd));
EXPORT	ssize_t		DVDFileSize	__PR((dvd_file_t *dvd_file));


/*
 * Free a DVD file
 */
EXPORT void
DVDCloseFile(dvd_file)
	dvd_file_t	*dvd_file;
{
	free(dvd_file);
	dvd_file = 0;
}


/*
 * Stat a IFO or BUP file from a DVD directory tree.
 */
LOCAL dvd_file_t *
DVDOpenFilePath(dvd, filename)
	dvd_reader_t	*dvd;
	char		*filename;
{

	char		full_path[PATH_MAX + 1];
	dvd_file_t	*dvd_file;
	struct stat	fileinfo;

	/* Get the full path of the file. */

	snprintf(full_path, sizeof (full_path),
				"%s/%s", dvd->path_root, filename);


	dvd_file = (dvd_file_t *)e_malloc(sizeof (dvd_file_t));
	if (!dvd_file)
		return (0);
	dvd_file->dvd = dvd;
	dvd_file->filesize = 0;

	if (stat(full_path, &fileinfo) < 0) {
		free(dvd_file);
		return (0);
	}
	dvd_file->filesize = fileinfo.st_size / DVD_VIDEO_LB_LEN;

	return (dvd_file);
}


/*
 * Stat a VOB file from a DVD directory tree.
 */
LOCAL dvd_file_t *
DVDOpenVOBPath(dvd, title, menu)
	dvd_reader_t	*dvd;
	int		title;
	int		menu;
{

	char		filename[PATH_MAX + 1];
	struct stat	fileinfo;
	dvd_file_t	*dvd_file;
	int		i;

	dvd_file = (dvd_file_t *)e_malloc(sizeof (dvd_file_t));
	if (!dvd_file)
		return (0);
	dvd_file->dvd = dvd;
	dvd_file->filesize = 0;

	if (menu) {
		if (title == 0) {
			snprintf(filename, sizeof (filename),
				"%s/VIDEO_TS/VIDEO_TS.VOB", dvd->path_root);
		} else {
			snprintf(filename, sizeof (filename),
				"%s/VIDEO_TS/VTS_%02i_0.VOB", dvd->path_root, title);
		}
		if (stat(filename, &fileinfo) < 0) {
			free(dvd_file);
			return (0);
		}
		dvd_file->filesize = fileinfo.st_size / DVD_VIDEO_LB_LEN;
	} else {
		for (i = 0; i < 9; ++i) {

			snprintf(filename, sizeof (filename),
				"%s/VIDEO_TS/VTS_%02i_%i.VOB", dvd->path_root, title, i + 1);
			if (stat(filename, &fileinfo) < 0) {
					break;
			}

			dvd_file->filesize += fileinfo.st_size / DVD_VIDEO_LB_LEN;
		}
	}

	return (dvd_file);
}

/*
 * Stat a DVD file from a DVD directory tree
 */
EXPORT dvd_file_t *
#ifdef	PROTOTYPES
DVDOpenFile(dvd_reader_t *dvd, int titlenum, dvd_read_domain_t domain)
#else
DVDOpenFile(dvd, titlenum, domain)
	dvd_reader_t	*dvd;
	int		titlenum;
	dvd_read_domain_t domain;
#endif
{
	char		filename[MAX_UDF_FILE_NAME_LEN];

	switch (domain) {

	case DVD_READ_INFO_FILE:
		if (titlenum == 0) {
			snprintf(filename, sizeof (filename),
					"/VIDEO_TS/VIDEO_TS.IFO");
		} else {
			snprintf(filename, sizeof (filename),
					"/VIDEO_TS/VTS_%02i_0.IFO", titlenum);
		}
		break;

	case DVD_READ_INFO_BACKUP_FILE:
		if (titlenum == 0) {
			snprintf(filename, sizeof (filename),
					"/VIDEO_TS/VIDEO_TS.BUP");
		} else {
			snprintf(filename, sizeof (filename),
					"/VIDEO_TS/VTS_%02i_0.BUP", titlenum);
		}
		break;

	case DVD_READ_MENU_VOBS:
		return (DVDOpenVOBPath(dvd, titlenum, 1));

	case DVD_READ_TITLE_VOBS:
		if (titlenum == 0)
			return (0);
		return (DVDOpenVOBPath(dvd, titlenum, 0));

	default:
		errmsgno(EX_BAD, _("Invalid domain for file open.\n"));
		return (0);
	}
	return (DVDOpenFilePath(dvd, filename));
}



/*
 * Stat a DVD directory structure
 */
LOCAL dvd_reader_t *
DVDOpenPath(path_root)
	const char	*path_root;
{
	dvd_reader_t	*dvd;

	dvd = (dvd_reader_t *)e_malloc(sizeof (dvd_reader_t));
	if (!dvd)
		return (0);
	dvd->path_root = e_strdup(path_root);

	return (dvd);
}


/*
 * Stat a DVD structure - this one only works with directory structures
 */
EXPORT dvd_reader_t *
DVDOpen(path)
	const char	*path;
{
	struct stat	fileinfo;
	int		ret;

	if (!path)
		return (0);

	ret = stat(path, &fileinfo);
	if (ret < 0) {
	/* If we can't stat the file, give up */
		errmsg(_("Can't stat '%s'.\n"), path);
		return (0);
	}


	if (S_ISDIR(fileinfo.st_mode)) {
		return (DVDOpenPath(path));
	}

	/* If it's none of the above, screw it. */
	errmsgno(EX_BAD, _("Could not open '%s'.\n"), path);
	return (0);
}

/*
 * Free a DVD structure - this one will only close a directory tree
 */
EXPORT void
DVDClose(dvd)
	dvd_reader_t	*dvd;
{
	if (dvd) {
		if (dvd->path_root) free(dvd->path_root);
		free(dvd);
		dvd = 0;
	}
}



/*
 * Return the size of a DVD file
 */
EXPORT ssize_t
DVDFileSize(dvd_file)
	dvd_file_t	*dvd_file;
{
	return (dvd_file->filesize);
}

#endif /* DVD_AUD_VID */
