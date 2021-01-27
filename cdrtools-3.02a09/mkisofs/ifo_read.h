/* @(#)ifo_read.h	1.2 04/03/02 joerg */

#ifndef	_IFO_READ_H
#define	_IFO_READ_H

/*
 * Copyright (C) 2000, 2001, 2002 Björn Englund <d4bjorn@dtek.chalmers.se>,
 *				  Håkan Hjort <d95hjort@dtek.chalmers.se
 *				  Olaf Beck <olaf_sc@yahoo.com>
 *				  (I only did the cut down no other contribs)
 *				  Jörg Schilling <schilling@fokus.gmd.de>
 *				  (making the code portable)
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



#include "ifo_types.h"
#include "dvd_reader.h"

#ifdef __cplusplus
extern "C" {
#endif


/*
 * handle = ifoOpen(dvd, title);
 *
 * Opens an IFO and reads a tiny fraction of the data for the IFO file
 * corresponding to the given title set. If title 0 is given, the video
 * manager IFO file is read.
 * Returns a handle to a tiny parsed fraction of a IFO strcuture
 */
extern	ifo_handle_t *ifoOpen	__PR((dvd_reader_t *, int));


/*
 * ifoClose(ifofile);
 * Cleans up the IFO information. This will free all data allocated.
 */
extern	void ifoClose		__PR((ifo_handle_t *));

#ifdef __cplusplus
};
#endif
#endif /* _IFO_READ_H */
