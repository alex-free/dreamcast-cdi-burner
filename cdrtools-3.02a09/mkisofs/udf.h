/* @(#)udf.h	1.2 04/03/01 Copyright 2001-2004 J. Schilling */
/*
 *	UDF external definitions for mkisofs
 *
 *	Copyright (c) 2001-2004 J. Schilling
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

#ifndef	_UDF_H
#define	_UDF_H

extern struct output_fragment udf_vol_recognition_area_frag;
extern struct output_fragment udf_main_seq_frag;
extern struct output_fragment udf_main_seq_copy_frag;
extern struct output_fragment udf_integ_seq_frag;
extern struct output_fragment udf_anchor_vol_desc_frag;
extern struct output_fragment udf_file_set_desc_frag;
extern struct output_fragment udf_dirtree_frag;
extern struct output_fragment udf_file_entries_frag;
extern struct output_fragment udf_end_anchor_vol_desc_frag;

extern struct output_fragment udf_pad_to_sector_32_frag;
extern struct output_fragment udf_pad_to_sector_256_frag;
extern struct output_fragment udf_padend_avdp_frag;

int assign_dvd_weights __PR((char *name, struct directory *this_dir, int val));

#endif	/* _UDF_H */
