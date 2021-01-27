/* @(#)ifo_types.h	1.2 04/03/02 joerg */

#ifndef	_IFO_TYPES_H
#define	_IFO_TYPES_H
/*
 * Copyright (C) 2001, 2002 Billy Biggs <vektor@dumbterm.net>,
 *			    Håkan Hjort <d95hjort@dtek.chalmers.se>,
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

#include "dvd_reader.h"


typedef struct {
	UInt32_t	title_set_sector; 	/* sector */
} title_info_t;


typedef struct {
	UInt16_t	nr_of_srpts;
	title_info_t *	title;			/* array of title info */
} tt_srpt_t;

typedef struct {
	UInt32_t	vmg_last_sector;	/*sector */
	UInt32_t	vmgi_last_sector;	/* sector */
	UInt16_t	vmg_nr_of_title_sets;
	UInt32_t	vmgm_vobs;		/* sector */
	UInt32_t	tt_srpt;		/* sector */
} vmgi_mat_t;



typedef struct {
	UInt32_t	vts_last_sector;	/* sector */
	UInt32_t	vtsi_last_sector;	/* sector */
	UInt32_t	vtsm_vobs;		/* sector */
	UInt32_t	vtstt_vobs;		/* sector */
} vtsi_mat_t;


typedef struct {
	/* VMGI */
	vmgi_mat_t *	vmgi_mat;
	tt_srpt_t  *	tt_srpt;

	/* VTSI */
	vtsi_mat_t *	vtsi_mat;
} ifo_handle_t;


#endif	/* _IFO_TYPES_H */
