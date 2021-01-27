/* @(#)dvd_reader.h	1.3 06/09/13 joerg */

#ifndef	_DVD_READER_H
#define	_DVD_READER_H

/*
 * Copyright (C) 2001, 2002 Billy Biggs <vektor@dumbterm.net>,
 *			    Håkan Hjort <d95hjort@dtek.chalmers.se
 *			    Olaf Beck <olaf_sc@yahoo.com>
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


#include <schily/unistd.h>	/* Make sure <sys/types.h> is included */

/*
 * Maximum length of filenames for UDF.
 */
#define	MAX_UDF_FILE_NAME_LEN 2048

/*
 * The length of one Logical Block of a DVD Video.
 */
#define	DVD_VIDEO_LB_LEN 2048

#ifdef __cplusplus
extern "C" {
#endif


struct dvd_reader_s {
	/* Information required for a directory path drive. */
	char	*path_root;
};


typedef struct dvd_reader_s	dvd_reader_t;
typedef struct dvd_file_s	dvd_file_t;


/*
 * dvd = DVDOpen(path);
 * Opens a directory name of a DVD-Video structure on HD.
 * Returns zero if it fails.
 * The path should be like this
 * "path/VIDEO_TS/VTS_01_1.VOB"
 */


extern	dvd_reader_t *DVDOpen	__PR((const char *));


/*
 * DVDClose(dvd);
 *
 * Closes and cleans up the DVD reader object.  You must close all open files
 * before calling this function.
 */


extern	void DVDClose		__PR((dvd_reader_t *));

/*
 * INFO_FILE       : VIDEO_TS.IFO     (manager)
 *                   VTS_XX_0.IFO     (title)
 *
 * INFO_BACKUP_FILE: VIDEO_TS.BUP     (manager)
 *                   VTS_XX_0.BUP     (title)
 *
 * MENU_VOBS       : VIDEO_TS.VOB     (manager)
 *                   VTS_XX_0.VOB     (title)
 *
 * TITLE_VOBS      : VTS_XX_[1-9].VOB (title)
 *                   All files in the title set are opened and
 *                   read as a single file.
 */
typedef enum {
	DVD_READ_INFO_FILE,
	DVD_READ_INFO_BACKUP_FILE,
	DVD_READ_MENU_VOBS,
	DVD_READ_TITLE_VOBS
} dvd_read_domain_t;

/*
 * dvd_file = DVDOpenFile(dvd, titlenum, domain);
 *
 * Opens a file on the DVD given the title number and domain.  If the title
 * number is 0, the video manager information is opened
 * (VIDEO_TS.[IFO,BUP,VOB]).  Returns a file structure which may be used for
 * reads, or 0 if the file was not found.
 */
extern	dvd_file_t * DVDOpenFile	__PR((dvd_reader_t *, int, dvd_read_domain_t));

/*
 * DVDCloseFile(dvd_file);
 *
 * Closes a file and frees the associated structure.
 */
extern	void DVDCloseFile		__PR((dvd_file_t *));


/*
 * blocks = DVDFileSize(dvd_file);
 *
 * Returns the file size in blocks.
 */
extern	ssize_t DVDFileSize		__PR((dvd_file_t *));


#ifdef __cplusplus
};
#endif
#endif /* _DVD_READER_H */
