/* @(#)iso9660.h	1.5 04/03/02 Copyright 1996, 2004 J. Schilling */
/*
 *	Copyright (c) 1996, 2004 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#define	_delta(from, to)	((to) - (from) + 1)

#define	VD_BOOT		0
#define	VD_PRIMARY	1
#define	VD_SUPPLEMENT	2
#define	VD_PARTITION	3
#define	VD_TERM		255

#define	VD_ID		"CD001"

struct	iso9660_voldesc {
	char	vd_type		[_delta(1, 1)];
	char	vd_id		[_delta(2, 6)];
	char	vd_version	[_delta(7, 7)];
	char	vd_fill		[_delta(8, 2048)];
};

struct	iso9660_boot_voldesc {
	char	vd_type		[_delta(1, 1)];
	char	vd_id		[_delta(2, 6)];
	char	vd_version	[_delta(7, 7)];
	char	vd_bootsys	[_delta(8, 39)];
	char	vd_bootid	[_delta(40, 71)];
	char	vd_bootcode	[_delta(72, 2048)];
};

struct	iso9660_pr_voldesc {
	char	vd_type			[_delta(1,	1)];
	char	vd_id			[_delta(2,	6)];
	char	vd_version		[_delta(7,	7)];
	char	vd_unused1		[_delta(8,	8)];
	char	vd_system_id		[_delta(9,	40)];
	char	vd_volume_id		[_delta(41,	72)];
	char	vd_unused2		[_delta(73,	80)];
	char	vd_volume_space_size	[_delta(81,	88)];
	char	vd_unused3		[_delta(89,	120)];
	char	vd_volume_set_size	[_delta(121,	124)];
	char	vd_volume_seq_number	[_delta(125,	128)];
	char	vd_lbsize		[_delta(129,	132)];
	char	vd_path_table_size	[_delta(133,	140)];
	char	vd_pos_path_table_l	[_delta(141,	144)];
	char	vd_opt_pos_path_table_l	[_delta(145,	148)];
	char	vd_pos_path_table_m	[_delta(149,	152)];
	char	vd_opt_pos_path_table_m	[_delta(153,	156)];
	char	vd_root_dir		[_delta(157,	190)];
	char	vd_volume_set_id	[_delta(191,	318)];
	char	vd_publisher_id		[_delta(319,	446)];
	char	vd_data_preparer_id	[_delta(447,	574)];
	char	vd_application_id	[_delta(575,	702)];
	char	vd_copyr_file_id	[_delta(703,	739)];
	char	vd_abstr_file_id	[_delta(740,	776)];
	char	vd_bibl_file_id		[_delta(777,	813)];
	char	vd_create_time		[_delta(814,	830)];
	char	vd_mod_time		[_delta(831,	847)];
	char	vd_expiry_time		[_delta(848,	864)];
	char	vd_effective_time	[_delta(865,	881)];
	char	vd_file_struct_vers	[_delta(882,	882)];
	char	vd_reserved1		[_delta(883,	883)];
	char	vd_application_use	[_delta(884,	1395)];
	char	vd_fill			[_delta(1396,	2048)];
};

struct	iso9660_dir {
	char	dr_len			[_delta(1,	1)];
	char	dr_eattr_len		[_delta(2,	2)];
	char	dr_eattr_pos		[_delta(3,	10)];
	char	dr_data_len		[_delta(11,	18)];
	char	dr_recording_time	[_delta(19,	25)];
	char	dr_file_flags		[_delta(26,	26)];
	char	dr_file_unit_size	[_delta(27,	27)];
	char	dr_interleave_gap	[_delta(28,	28)];
	char	dr_volume_seq_number	[_delta(29,	32)];
	char	dr_file_name_len	[_delta(33,	33)];
	char	dr_file_name		[_delta(34,	34)];
};

struct	iso9660_dtime {
	unsigned char	dt_year;
	unsigned char	dt_month;
	unsigned char	dt_day;
	unsigned char	dt_hour;
	unsigned char	dt_minute;
	unsigned char	dt_second;
		char	dt_gmtoff;
};

struct	iso9660_ltime {
	char	lt_year			[_delta(1,	4)];
	char	lt_month		[_delta(5,	6)];
	char	lt_day			[_delta(7,	8)];
	char	lt_hour			[_delta(9,	10)];
	char	lt_minute		[_delta(11,	12)];
	char	lt_second		[_delta(13,	14)];
	char	lt_hsecond		[_delta(15,	16)];
	char	lt_gmtoff		[_delta(17,	17)];
};

struct iso9660_path_table {
	char	pt_di_len		[_delta(1,	1)];
	char	pt_eattr_len		[_delta(2,	2)];
	char	pt_eattr_pos		[_delta(3,	6)];
	char	pt_di_parent		[_delta(7,	8)];
	char	pt_name			[_delta(9,	9)];
};

struct iso9660_eattr {
	char	ea_owner		[_delta(1,	4)];
	char	ea_group		[_delta(5,	8)];
	char	ea_perm			[_delta(9,	10)];
	char	ea_ctime		[_delta(11,	27)];
	char	ea_mtime		[_delta(28,	44)];
	char	ea_extime		[_delta(45,	61)];
	char	ea_eftime		[_delta(62,	78)];
	char	ea_record_format	[_delta(79,	79)];
	char	ea_record_attr		[_delta(80,	80)];
	char	ea_record_len		[_delta(81,	84)];
	char	ea_system_id		[_delta(85,	116)];
	char	ea_system_use		[_delta(117,	180)];
	char	ea_version		[_delta(181,	181)];
	char	ea_esc_seq_len		[_delta(182,	182)];
	char	ea_reserved1		[_delta(183,	246)];
	char	ea_appl_use_len		[_delta(247,	250)];
	char	ea_appl_use		[_delta(251,	251)];	/* actually more */
/*	char	ea_esc_seq		[_delta(xxx,	xxx)];	*/

};

#define	PERM_MB_ONE	0xAAAA

#define	PERM_RSYS	0x0001
#define	PERM_XSYS	0x0004

#define	PERM_RUSR	0x0010
#define	PERM_XUSR	0x0040

#define	PERM_RGRP	0x0100
#define	PERM_XGRP	0x0400

#define	PERM_ROTH	0x1000
#define	PERM_XOTH	0x4000


#define	GET_UBYTE(a)	a_to_u_byte(a)
#define	GET_SBYTE(a)	a_to_byte(a)
#define	GET_SHORT(a)	a_to_u_2_byte(&((unsigned char *) (a))[0])
#define	GET_BSHORT(a)	a_to_u_2_byte(&((unsigned char *) (a))[2])
#define	GET_INT(a)	a_to_4_byte(&((unsigned char *) (a))[0])
#define	GET_LINT(a)	la_to_4_byte(&((unsigned char *) (a))[0])
#define	GET_BINT(a)	a_to_4_byte(&((unsigned char *) (a))[4])
