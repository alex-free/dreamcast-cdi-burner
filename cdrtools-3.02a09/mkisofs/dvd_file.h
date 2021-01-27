/* @(#)dvd_file.h	1.1 02/07/21 Copyright 2002 J. Schilling */
/*
 *	Defnitions for users of dvd_file.c
 *
 *	Copyright (c) 2002 J. Schilling
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
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef	_DVD_FILE_H
#define	_DVD_FILE_H

extern	void		DVDFreeFileSet		__PR((title_set_info_t * title_set_info));
extern	title_set_info_t *DVDGetFileSet		__PR((char * dvd));
extern	int		DVDGetFilePad		__PR((title_set_info_t * title_set_info, char * name));

#endif	/* _DVD_FILE_H */
