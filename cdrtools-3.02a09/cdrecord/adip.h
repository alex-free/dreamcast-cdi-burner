/* @(#)adip.h	1.2 06/09/13 Copyright 2004 J. Schilling */

#ifndef	ADIP_H
#define	ADIP_H

#include <schily/utypes.h>

typedef struct adip {
	Uchar	cat_vers;		/*  0	*/
	Uchar	disk_size;		/*  1	*/
	Uchar	disk_struct;		/*  2	*/
	Uchar	density;		/*  3	*/
	Uchar	data_zone_alloc[12];	/*  4	*/
	Uchar	mbz_16;			/* 16	*/
	Uchar	res_17[2];		/* 17	*/
	Uchar	man_id[8];		/* 19	*/
	Uchar	media_id[3];		/* 27	*/
	Uchar	prod_revision;		/* 30	*/
	Uchar	adip_numbytes;		/* 31	*/
	Uchar	ref_speed;		/* 32	*/
	Uchar	max_speed;		/* 33	*/
	Uchar	wavelength;		/* 34	*/
	Uchar	norm_write_power;	/* 35	*/
	Uchar	max_read_power_ref;	/* 36	*/
	Uchar	pind_ref;		/* 37	*/
	Uchar	beta_ref;		/* 38	*/
	Uchar	max_read_power_max;	/* 39	*/
	Uchar	pind_max;		/* 40	*/
	Uchar	beta_max;		/* 41	*/
	Uchar	pulse[14];		/* 42	*/
	Uchar	res_56[192];		/* 56	*/
	Uchar	res_controldat[8];	/* 248	*/
} adip_t;



#endif	/* ADIP_H */
