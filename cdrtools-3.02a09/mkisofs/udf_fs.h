/* @(#)udf_fs.h	1.6 13/02/12 Copyright 2001-2013 J. Schilling */
/*
 * udf_fs.h - UDF structure definitions for mkisofs
 *
 * Written by Ben Rudiak-Gould (2001).
 *
 * Copyright 2001-2013 J. Schilling.
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

#ifndef	_UDF_FS_H
#define	_UDF_FS_H

/*
 * Abbreviations:
 *
 * ad		allocation descriptor
 * desc		descriptor
 * ext		extended
 * ident	identifier
 * impl		implementation
 * info		information
 * ptr		pointer
 * seq		sequence
 */

typedef char udf_Uint8;
typedef char udf_dchar;
typedef char udf_dstring;
typedef char udf_byte;
typedef char udf_zerobyte;

/* Is this safe? Are there compilers so perverse as to pad these structs? */
typedef struct udf_Uint16_ {
	char	l;
	char	h;
} udf_Uint16;

typedef struct udf_Uint32_ {
	char	l;
	char	ml;
	char	mh;
	char	h;
} udf_Uint32;
typedef struct udf_Uint64_ {
	udf_Uint32	l;
	udf_Uint32	h;
} udf_Uint64;

typedef struct udf_tag_ {			/* ECMA-167 3/7.2 */
/* 0*/	udf_Uint16	tag_ident;
/* 2*/	udf_Uint16	desc_version;
/* 4*/	udf_Uint8	tag_checksum;
/* 5*/	udf_zerobyte	reserved;
/* 6*/	udf_Uint16	tag_serial_number;
/* 8*/	udf_Uint16	desc_crc;
/*10*/	udf_Uint16	desc_crc_length;
/*12*/	udf_Uint32	tag_location;
/*16*/
} udf_tag;

#define	UDF_TAGID_PRIMARY_VOLUME_DESC		1
#define	UDF_TAGID_ANCHOR_VOLUME_DESC_PTR	2
#define	UDF_TAGID_IMPL_USE_VOLUME_DESC		4
#define	UDF_TAGID_PARTITION_DESC		5
#define	UDF_TAGID_LOGICAL_VOLUME_DESC		6
#define	UDF_TAGID_UNALLOCATED_SPACE_DESC	7
#define	UDF_TAGID_TERMINATING_DESC		8
#define	UDF_TAGID_LOGICAL_VOLUME_INTEGRITY_DESC	9
#define	UDF_TAGID_FILE_SET_DESC			256
#define	UDF_TAGID_FILE_IDENT_DESC		257
#define	UDF_TAGID_FILE_ENTRY			261
#define	UDF_TAGID_EXT_ATTRIBUTE_HEADER_DESC	262
#define	UDF_TAGID_EXT_FILE_ENTRY		266

typedef struct udf_extent_ad_ {			/* ECMA-167 3/7.1 */
/*0*/	udf_Uint32	extent_length;
/*4*/	udf_Uint32	extent_location;
/*8*/
} udf_extent_ad;

typedef struct udf_charspec_ {			/* ECMA-167 1/7.2.1 */
/* 0*/	udf_Uint8	character_set_type;
/* 1*/	udf_byte	character_set_info[63];
/*64*/
} udf_charspec;

typedef struct udf_EntityID_ {			/* ECMA-167 1/7.4 */
/* 0*/	udf_Uint8	flags;
/* 1*/	udf_byte	ident[23];
/*24*/	udf_byte	ident_suffix[8];
/*32*/
} udf_EntityID;

#define	UDF_ENTITYID_FLAG_PROTECTED	2	/* ECMA-167 1/7.4.1 */

typedef struct udf_lb_addr_ {			/* ECMA-167 4/7.1 */
/*0*/	udf_Uint32	logical_block_number;
/*4*/	udf_Uint16	partition_reference_number;
/*6*/
} udf_lb_addr;

typedef struct udf_short_ad_ {			/* ECMA-167 4/14.14.1 */
/*0*/	udf_Uint32	extent_length;
/*4*/	udf_Uint32	extent_position;
/*8*/
} udf_short_ad;

typedef struct udf_long_ad_impl_use_field_ {	/* UDF 2.01 2.3.4.3 */
/*0*/	udf_Uint16	flags;
/*2*/	udf_Uint32	unique_id;
/*6*/
} udf_long_ad_impl_use_field;

typedef struct udf_long_ad_ {			/* ECMA-167 4/14.14.2 */
/* 0*/	udf_Uint32	extent_length;
/* 4*/	udf_lb_addr	extent_location;
/*10*/	udf_long_ad_impl_use_field	impl_use;
/*16*/
} udf_long_ad;

typedef struct udf_timestamp_ {			/* TR/71 1.5.4 */
/* 0*/	udf_Uint16	type_and_time_zone;
/* 2*/	udf_Uint16	year;
/* 4*/	udf_Uint8	month;
/* 5*/	udf_Uint8	day;
/* 6*/	udf_Uint8	hour;
/* 7*/	udf_Uint8	minute;
/* 8*/	udf_Uint8	second;
/* 9*/	udf_Uint8	centiseconds;
/*10*/	udf_Uint8	hundreds_of_microseconds;
/*11*/	udf_Uint8	microseconds;
/*12*/
} udf_timestamp;

typedef struct udf_volume_recognition_desc_ {	/* TR/71 2.4.{1,2,3} */
	udf_Uint8	structure_type;
	udf_byte	standard_identifier[5];
	udf_Uint8	structure_version;
	udf_zerobyte	structure_data[2041];
} udf_volume_recognition_desc;

typedef struct udf_anchor_volume_desc_ptr_ {	/* TR/71 2.5.1 */
/*  0*/	udf_tag		desc_tag;
/* 16*/	udf_extent_ad	main_volume_desc_seq_extent;
/* 24*/	udf_extent_ad	reserve_volume_desc_seq_extent;
/* 32*/	udf_zerobyte	reserved[480];
/*512*/
} udf_anchor_volume_desc_ptr;

typedef struct udf_primary_volume_desc_ {	/* TR/71 2.6.1 */
/*  0*/	udf_tag		desc_tag;
/* 16*/	udf_Uint32	volume_desc_seq_number;
/* 20*/	udf_Uint32	primary_volume_desc_number;
/* 24*/	udf_dstring	volume_ident[32];
/* 56*/	udf_Uint16	volume_seq_number;
/* 58*/	udf_Uint16	maximum_volume_seq_number;
/* 60*/	udf_Uint16	interchange_level;
/* 62*/	udf_Uint16	maximum_interchange_level;
/* 64*/	udf_Uint32	character_set_list;
/* 68*/	udf_Uint32	maximum_character_set_list;
/* 72*/	udf_dstring	volume_set_ident[128];
/*200*/	udf_charspec	desc_character_set;
/*264*/	udf_charspec	explanatory_character_set;
/*328*/	udf_extent_ad	volume_abstract;
/*336*/	udf_extent_ad	volume_copyright_notice;
/*344*/	udf_EntityID	application_ident;
/*376*/	udf_timestamp	recording_date_and_time;
/*388*/	udf_EntityID	impl_ident;
/*420*/	udf_byte	impl_use[64];
/*484*/	udf_Uint32	predecessor_volume_desc_seq_location;
/*488*/	udf_Uint16	flags;
/*490*/	udf_zerobyte	reserved[22];
/*512*/
} udf_primary_volume_desc;

typedef struct udf_impl_use_volume_desc_impl_use_field_ {	/* TR/71 2.6.3 */
/*  0*/	udf_charspec	lvi_charset;
/* 64*/	udf_dstring	logical_volume_ident[128];
/*192*/	udf_dstring	lv_info1[36];
/*228*/	udf_dstring	lv_info2[36];
/*264*/	udf_dstring	lv_info3[36];
/*300*/	udf_EntityID	impl_id;
/*332*/	udf_byte	impl_use[128];
/*460*/
} udf_impl_use_volume_desc_impl_use_field;

typedef struct udf_impl_use_volume_desc_ {	/* TR/71 2.6.2 */
/*  0*/	udf_tag		desc_tag;
/* 16*/	udf_Uint32	volume_desc_seq_number;
/* 20*/	udf_EntityID	impl_ident;
/* 52*/	udf_impl_use_volume_desc_impl_use_field	impl_use;
/*512*/
} udf_impl_use_volume_desc;

typedef struct udf_partition_desc_ {		/* TR/71 2.6.4 */
/*  0*/	udf_tag		desc_tag;
/* 16*/	udf_Uint32	volume_desc_seq_number;
/* 20*/	udf_Uint16	partition_flags;
/* 22*/	udf_Uint16	partition_number;
/* 24*/	udf_EntityID	partition_contents;
/* 56*/	udf_byte	partition_contents_use[128];
/*184*/	udf_Uint32	access_type;
/*188*/	udf_Uint32	partition_starting_location;
/*192*/	udf_Uint32	partition_length;
/*196*/	udf_EntityID	impl_ident;
/*228*/	udf_byte	impl_use[128];
/*356*/	udf_zerobyte	reserved[156];
/*512*/
} udf_partition_desc;

#define	UDF_PARTITION_FLAG_ALLOCATED	1	/* ECMA-167 3/10.5.3 */
#define	UDF_ACCESSTYPE_READONLY		1	/* ECMA-167 3/10.5.7 */

typedef struct udf_type_1_partition_map_ {	/* TR/71 2.6.8 */
/*0*/	udf_Uint8	partition_map_type;
/*1*/	udf_Uint8	partition_map_length;
/*2*/	udf_Uint16	volume_seq_number;
/*4*/	udf_Uint16	partition_number;
/*6*/
} udf_type_1_partition_map;

#define	UDF_PARTITION_MAP_TYPE_1	1

typedef struct udf_logical_volume_desc_ {	/* TR/71 2.6.7 */
/*  0*/	udf_tag		desc_tag;
/* 16*/	udf_Uint32	volume_desc_seq_number;
/* 20*/	udf_charspec	desc_character_set;
/* 84*/	udf_dstring	logical_volume_ident[128];
/*212*/	udf_Uint32	logical_block_size;
/*216*/	udf_EntityID	domain_ident;
/*248*/	udf_long_ad	logical_volume_contents_use;
/*264*/	udf_Uint32	map_table_length;
/*268*/	udf_Uint32	number_of_partition_maps;
/*272*/	udf_EntityID	impl_ident;
/*304*/	udf_byte	impl_use[128];
/*432*/	udf_extent_ad	integrity_seq_extent;
/*440*/	udf_type_1_partition_map	partition_map[1];
/*446*/
} udf_logical_volume_desc;

typedef struct udf_unallocated_space_desc_ {	/* TR/71 2.6.9 */
/* 0*/	udf_tag		desc_tag;
/*16*/	udf_Uint32	volume_desc_seq_number;
/*20*/	udf_Uint32	number_of_allocation_descs;
/*24*/	/*udf_extent_ad	allocation_descs[0];*/
} udf_unallocated_space_desc;

typedef struct udf_terminating_desc_ {		/* TR/71 2.6.10 */
/*  0*/	udf_tag		desc_tag;
/* 16*/	udf_zerobyte	reserved[496];
/*512*/
} udf_terminating_desc;

typedef struct udf_logical_volume_integrity_desc_impl_use_field_ {	/* TR/71 2.7.3 */
/* 0*/	udf_EntityID	impl_id;
/*32*/	udf_Uint32	number_of_files;
/*36*/	udf_Uint32	number_of_directories;
/*40*/	udf_Uint16	minimum_udf_read_revision;
/*42*/	udf_Uint16	minimum_udf_write_revision;
/*44*/	udf_Uint16	maximum_udf_write_revision;
/*46*/	/*udf_byte	impl_use[0];*/
} udf_logical_volume_integrity_desc_impl_use_field;

typedef struct udf_logical_volume_integrity_desc_contents_use_field_ {	/* TR/71 2.7.2 */
	udf_Uint64	unique_id;
	udf_zerobyte	reserved[24];
} udf_logical_volume_integrity_desc_contents_use_field;

typedef struct udf_logical_volume_integrity_desc_ {	/* TR/71 2.7.1 */
/* 0*/	udf_tag		desc_tag;
/*16*/	udf_timestamp	recording_date;
/*28*/	udf_Uint32	integrity_type;
/*32*/	udf_extent_ad	next_integrity_extent;
/*40*/	udf_logical_volume_integrity_desc_contents_use_field	logical_volume_contents_use;
/*72*/	udf_Uint32	number_of_partitions;
/*76*/	udf_Uint32	length_of_impl_use;
/*80*/	udf_Uint32	free_space_table;
/*84*/	udf_Uint32	size_table;
/*88*/	udf_logical_volume_integrity_desc_impl_use_field	impl_use;
} udf_logical_volume_integrity_desc;

#define	UDF_INTEGRITY_TYPE_CLOSE	1	/* ECMA-167 3/10.10.3 */

typedef struct udf_file_set_desc_ {		/* TR/71 3.3.1 */
/* 0*/	udf_tag		desc_tag;
/*16*/	udf_timestamp	recording_date_and_time;
/*28*/	udf_Uint16	interchange_level;
/*30*/	udf_Uint16	maximum_interchange_level;
/*32*/	udf_Uint32	character_set_list;
/*36*/	udf_Uint32	maximum_character_set_list;
/*40*/	udf_Uint32	file_set_number;
/*44*/	udf_Uint32	file_set_desc_number;
/*48*/	udf_charspec	logical_volume_ident_character_set;
/*112*/	udf_dstring	logical_volume_ident[128];
/*240*/	udf_charspec	file_set_character_set;
/*304*/	udf_dstring	file_set_ident[32];
/*336*/	udf_dstring	copyright_file_ident[32];
/*368*/	udf_dstring	abstract_file_ident[32];
/*400*/	udf_long_ad	root_directory_icb;
/*416*/	udf_EntityID	domain_ident;
/*448*/	udf_long_ad	next_extent;
/*464*/	udf_zerobyte	reserved[48];
/*512*/
} udf_file_set_desc;

typedef struct udf_file_ident_desc_ {		/* TR/71 3.4.1 */
/* 0*/	udf_tag		desc_tag;
/*16*/	udf_Uint16	file_version_number;
/*18*/	udf_Uint8	file_characteristics;
/*19*/	udf_Uint8	length_of_file_ident;
/*20*/	udf_long_ad	icb;
/*36*/	udf_Uint16	length_of_impl_use;
/*38*/	/*udf_EntityID	impl_use;*/
/*38*/	udf_dchar	file_ident[1];
	/*udf_zerobyte	padding[0/1/2/3];*/
} udf_file_ident_desc;

#define	UDF_FILE_CHARACTERISTIC_HIDDEN		1	/* ECMA-167 4/14.4.3 */
#define	UDF_FILE_CHARACTERISTIC_DIRECTORY	2
#define	UDF_FILE_CHARACTERISTIC_DELETED		4
#define	UDF_FILE_CHARACTERISTIC_PARENT		8

typedef struct udf_icbtag_ {			/* TR/71 3.5.2 */
/* 0*/	udf_Uint32	prior_recorded_number_of_direct_entries;
/* 4*/	udf_Uint16	strategy_type;
/* 6*/	udf_Uint16	strategy_parameter;
/* 8*/	udf_Uint16	maximum_number_of_entries;
/*10*/	udf_zerobyte	reserved;
/*11*/	udf_Uint8	file_type;
/*12*/	udf_lb_addr	parent_icb_location;
/*18*/	udf_Uint16	flags;
/*20*/
} udf_icbtag;

/*
 * File types
 */
#define	UDF_ICBTAG_FILETYPE_UNSPEC	0
#define	UDF_ICBTAG_FILETYPE_UNALL_SPACE	1
#define	UDF_ICBTAG_FILETYPE_PART_INTEG	2
#define	UDF_ICBTAG_FILETYPE_INDIRECT	3
#define	UDF_ICBTAG_FILETYPE_DIRECTORY	4	/* ECMA-167 4/14.6.6 */
#define	UDF_ICBTAG_FILETYPE_BYTESEQ	5	/* FILE */
#define	UDF_ICBTAG_FILETYPE_BLOCK_DEV	6
#define	UDF_ICBTAG_FILETYPE_CHAR_DEV	7
#define	UDF_ICBTAG_FILETYPE_EA		8	/* Extended attributes */
#define	UDF_ICBTAG_FILETYPE_FIFO	9
#define	UDF_ICBTAG_FILETYPE_C_ISSOCK	10
#define	UDF_ICBTAG_FILETYPE_T_ENTRY	11	/* Terminal entry */
#define	UDF_ICBTAG_FILETYPE_SYMLINK	12
#define	UDF_ICBTAG_FILETYPE_STREAMDIR	13
					/* 14..247	Reserved */
					/* 248..255	Subject to agreement */

#define	UDF_ICBTAG_FLAG_MASK_AD_TYPE	7	/* TR/71 3.5.3 */
#define	UDF_ICBTAG_FLAG_SHORT_AD	0
#define	UDF_ICBTAG_FLAG_DIRECTORY_SORT	8
#define	UDF_ICBTAG_FLAG_NONRELOCATABLE	16
#define	UDF_ICBTAG_FLAG_ARCHIVE		32
#define	UDF_ICBTAG_FLAG_SETUID		64
#define	UDF_ICBTAG_FLAG_SETGID		128
#define	UDF_ICBTAG_FLAG_STICKY		256
#define	UDF_ICBTAG_FLAG_CONTIGUOUS	512
#define	UDF_ICBTAG_FLAG_SYSTEM		1024
#define	UDF_ICBTAG_FLAG_TRANSFORMED	2048
#define	UDF_ICBTAG_FLAG_MULTI_VERSIONS	4096
#define	UDF_ICBTAG_FLAG_STREAM	8192

typedef struct udf_ext_attribute_header_desc_ {	/* TR/71 3.6.1 */
/* 0*/	udf_tag		desc_tag;
/*16*/	udf_Uint32	impl_attributes_location;
/*20*/	udf_Uint32	application_attributes_location;
/*24*/
} udf_ext_attribute_header_desc;

typedef struct udf_ext_attribute_common_ {	/* TR/71 3.6.{2,3} */
/* 0*/	udf_Uint32	attribute_type;
/* 4*/	udf_Uint8	attribute_subtype;
/* 5*/	udf_zerobyte	reserved[3];
/* 8*/	udf_Uint32	attribute_length;
/*12*/	udf_Uint32	impl_use_length;
/*16*/	udf_EntityID	impl_ident;
/*48*/	udf_Uint16	header_checksum;
/*50*/
} udf_ext_attribute_common;

typedef struct udf_ext_attribute_dev_spec_ {	/* ECMA-167 4/14.10.7 */
/* 0*/	udf_Uint32	attribute_type;		/* = 12 */
/* 4*/	udf_Uint8	attribute_subtype;	/* = 1 */
/* 5*/	udf_zerobyte	reserved[3];
/* 8*/	udf_Uint32	attribute_length;	/* = 24 */
/*12*/	udf_Uint32	impl_use_length;	/* = 0 */
/*16*/	udf_Uint32	dev_major;		/* major(st_rdev) */
/*20*/	udf_Uint32	dev_minor;		/* minor(st_rdev) */
#ifdef	__needed__
/*24*/	udf_Uint8	impl_use[0];
#endif
/*24*/
} udf_ext_attribute_dev_spec;

typedef struct udf_ext_attribute_free_ea_space_ {	/* TR/71 3.6.{2,3} */
/* 0*/	udf_Uint32	attribute_type;		/* = 2048 */
/* 4*/	udf_Uint8	attribute_subtype;	/* = 1 */
/* 5*/	udf_zerobyte	reserved[3];
/* 8*/	udf_Uint32	attribute_length;	/* = 52 */
/*12*/	udf_Uint32	impl_use_length;	/* = 4 */
/*16*/	udf_EntityID	impl_ident;		/* "*UDF FreeEASpace" */
/*48*/	udf_Uint16	header_checksum;
/*50*/	udf_Uint16	free_ea_space;		/* = 0 */
/*52*/
} udf_ext_attribute_free_ea_space;

typedef struct udf_ext_attribute_dvd_cgms_info_ {	/* TR/71 3.6.{2,4} */
/* 0*/	udf_Uint32	attribute_type;		/* = 2048 */
/* 4*/	udf_Uint8	attribute_subtype;	/* = 1 */
/* 5*/	udf_zerobyte	reserved[3];
/* 8*/	udf_Uint32	attribute_length;	/* = 56 */
/*12*/	udf_Uint32	impl_use_length;	/* = 8 */
/*16*/	udf_EntityID	impl_ident;		/* "*UDF DVD CGMS Info" */
/*48*/	udf_Uint16	header_checksum;
/*50*/	udf_byte	cgms_info;
/*51*/	udf_Uint8	data_structure_type;
/*52*/	udf_byte	protection_system_info[4];
/*56*/
} udf_ext_attribute_dvd_cgms_info;

#define	UDF_CGMSINFO_NO_COPIES			48	/* TR/71 3.6.4 */
#define	UDF_CGMSINFO_ONE_GENERATION		32
#define	UDF_CGMSINFO_UNLIMITED_COPIES		0
#define	UDF_CGMSINFO_FLAG_COPYRIGHTED_MATERIAL	128


/* start mac finder info defs */
typedef struct udf_point_ {
	udf_Uint16	v;
	udf_Uint16	h;
} udf_point;

typedef struct udf_rect_ {
	udf_Uint16	top;
	udf_Uint16	left;
	udf_Uint16	bottom;
	udf_Uint16	right;
} udf_rect;

typedef struct udf_dinfo_ {
	udf_rect	frrect;
	udf_Uint16	frflags;
	udf_point	frlocation;
	udf_Uint16	frview;
} udf_dinfo;

typedef struct udf_dxinfo_ {
	udf_point	frscroll;
	udf_Uint32	fropenchain;
	udf_Uint8	frscript;
	udf_Uint8	frxflags;
	udf_Uint16	frcomment;
	udf_Uint32	frputaway;
} udf_dxinfo;

typedef struct udf_finfo_ {
	udf_Uint32	fdtype;
	udf_Uint32	fdcreator;
	udf_Uint16	fdflags;
	udf_point	fdlocation;
	udf_Uint16	fdfldr;
} udf_finfo;

typedef struct udf_fxinfo_ {
	udf_Uint16	fdiconid;
	udf_Uint8	unused[6];
	udf_Uint8	fdscript;
	udf_Uint8	fdxflags;
	udf_Uint16	fdcomment;
	udf_Uint32	fdputaway;
} udf_fxinfo;

typedef struct udf_mac_file_finderinfo_ {
/* 0*/	udf_Uint16	headerchecksum;
/* 2*/	udf_Uint16	reserved;
/* 4*/	udf_Uint32	parentdirid;
/* 8*/	udf_finfo	fileinfo;
/*24*/	udf_fxinfo	fileextinfo;
/*40*/	udf_Uint32	resourcedatalength;
/*44*/	udf_Uint32	resourcealloclength;
/*48*/
} udf_mac_file_finderinfo;

typedef struct udf_mac_dir_volumeinfo_ {
/* 0*/	udf_Uint16	headerchecksum;
/* 2*/	udf_timestamp	moddate;
/* 14*/	udf_timestamp	budate;
/* 26*/	udf_Uint32	volfinderinfo[8];	/* ?? */
/* 58*/ udf_Uint8	unknown[2];	/* ?? */
/* 60 */
} udf_mac_dir_volumeinfo;


typedef struct udf_mac_dir_finderinfo_ {
/* 0*/	udf_Uint16	headerchecksum;
/* 2*/	udf_Uint16	reserved;
/* 4*/	udf_Uint32	parentdirid;
/* 8*/	udf_dinfo	dirinfo;
/*24*/	udf_dxinfo	dirextinfo;
/*40*/
} udf_mac_dir_finderinfo;

typedef struct udf_ext_attribute_file_macfinderinfo_ {
/* 0*/	udf_Uint32	attribute_type;		/* = 2048 */
/* 4*/	udf_Uint8	attribute_subtype;	/* = 1 */
/* 5*/	udf_zerobyte	reserved[3];
/* 8*/	udf_Uint32	attribute_length;	/* = 48 + 48 */
/*12*/	udf_Uint32	impl_use_length;	/* = 48 */
/*16*/	udf_EntityID	impl_ident;		/* "*UDF Mac FinderInfo" */
/*48*/	udf_mac_file_finderinfo	finderinfo;
/*96*/
} udf_ext_attribute_file_macfinderinfo;

typedef struct udf_ext_attribute_dir_macvolinfo_ {
/* 0*/	udf_Uint32	attribute_type;		/* = 2048 */
/* 4*/	udf_Uint8	attribute_subtype;	/* = 1 */
/* 5*/	udf_zerobyte	reserved[3];
/* 8*/	udf_Uint32	attribute_length;	/* = 48 + 60 */
/*12*/	udf_Uint32	impl_use_length;	/* = 60 */
/*16*/	udf_EntityID	impl_ident;		/* "*UDF Mac VolumeInfo" */

/*48*/	udf_mac_dir_volumeinfo	volumeinfo;
/*96*/
} udf_ext_attribute_dir_macvolinfo;

typedef struct udf_ext_attribute_dir_macfinderinfo_ {
/* 0*/	udf_Uint32	attribute_type;		/* = 2048 */
/* 4*/	udf_Uint8	attribute_subtype;	/* = 1 */
/* 5*/	udf_zerobyte	reserved[3];
/* 8*/	udf_Uint32	attribute_length;	/* = 48 + 40 */
/*12*/	udf_Uint32	impl_use_length;	/* = 40 */
/*16*/	udf_EntityID	impl_ident;		/* "*UDF Mac FinderInfo" */
/*48*/	udf_mac_dir_finderinfo	finderinfo;
/*96*/
} udf_ext_attribute_dir_macfinderinfo;

#define	EXTATTR_IMP_USE 2048
/* end mac finder info defs */


typedef struct udf_macvolume_filed_entry_ {		/* TR/71 3.5.1 */
/* 0*/	udf_tag		desc_tag;
/*16*/	udf_icbtag	icb_tag;
/*36*/	udf_Uint32	uid;
/*40*/	udf_Uint32	gid;
/*44*/	udf_Uint32	permissions;
/*48*/	udf_Uint16	file_link_count;
/*50*/	udf_Uint8	record_format;
/*51*/	udf_Uint8	record_display_attributes;
/*52*/	udf_Uint32	record_length;
/*56*/	udf_Uint64	info_length;
/*64*/	udf_Uint64	logical_blocks_recorded;
/*72*/	udf_timestamp	access_time;
/*84*/	udf_timestamp	modification_time;
/*96*/	udf_timestamp	attribute_time;
/*108*/	udf_Uint32	checkpoint;
/*112*/	udf_long_ad	ext_attribute_icb;
/*128*/	udf_EntityID	impl_ident;
/*160*/	udf_Uint64	unique_id;
/*168*/	udf_Uint32	length_of_ext_attributes;
/*172*/	udf_Uint32	length_of_allocation_descs;
udf_ext_attribute_header_desc			ext_attribute_header;
udf_ext_attribute_free_ea_space			ext_attribute_free_ea_space;
udf_ext_attribute_dvd_cgms_info			ext_attribute_dvd_cgms_info;
udf_ext_attribute_dir_macvolinfo		ext_attribute_macvolumeinfo;
udf_ext_attribute_dir_macfinderinfo		ext_attribute_macfinderinfo;
udf_short_ad							allocation_desc;
} udf_macvolume_filed_entry;


typedef struct udf_file_entry_ {		/* TR/71 3.5.1 */
/* 0*/	udf_tag		desc_tag;
/*16*/	udf_icbtag	icb_tag;
/*36*/	udf_Uint32	uid;
/*40*/	udf_Uint32	gid;
/*44*/	udf_Uint32	permissions;
/*48*/	udf_Uint16	file_link_count;
/*50*/	udf_Uint8	record_format;
/*51*/	udf_Uint8	record_display_attributes;
/*52*/	udf_Uint32	record_length;
/*56*/	udf_Uint64	info_length;
/*64*/	udf_Uint64	logical_blocks_recorded;
/*72*/	udf_timestamp	access_time;
/*84*/	udf_timestamp	modification_time;
/*96*/	udf_timestamp	attribute_time;
/*108*/	udf_Uint32	checkpoint;
/*112*/	udf_long_ad	ext_attribute_icb;
/*128*/	udf_EntityID	impl_ident;
/*160*/	udf_Uint64	unique_id;
/*168*/	udf_Uint32	length_of_ext_attributes;
/*172*/	udf_Uint32	length_of_allocation_descs;
#if 0
/*176*/	udf_ext_attribute_header_desc	ext_attribute_header;
/*200*/	udf_ext_attribute_free_ea_space	ext_attribute_free_ea_space;
/*252*/	udf_ext_attribute_dvd_cgms_info	ext_attribute_dvd_cgms_info;
/*308*/	udf_short_ad	allocation_desc;
/*316*/
#else
	udf_ext_attribute_header_desc	ext_attribute_header;
	udf_ext_attribute_dev_spec	ext_attribute_dev_spec;
	udf_ext_attribute_free_ea_space	ext_attribute_free_ea_space;
	udf_ext_attribute_dvd_cgms_info	ext_attribute_dvd_cgms_info;
	udf_ext_attribute_file_macfinderinfo	ext_attribute_macfinderinfo;
	udf_short_ad	allocation_desc;
#endif
} udf_file_entry;

typedef struct udf_attr_file_entry_ {		/* TR/71 3.5.1 */
/* 0*/	udf_tag		desc_tag;
/*16*/	udf_icbtag	icb_tag;
/*36*/	udf_Uint32	uid;
/*40*/	udf_Uint32	gid;
/*44*/	udf_Uint32	permissions;
/*48*/	udf_Uint16	file_link_count;
/*50*/	udf_Uint8	record_format;
/*51*/	udf_Uint8	record_display_attributes;
/*52*/	udf_Uint32	record_length;
/*56*/	udf_Uint64	info_length;
/*64*/	udf_Uint64	logical_blocks_recorded;
/*72*/	udf_timestamp	access_time;
/*84*/	udf_timestamp	modification_time;
/*96*/	udf_timestamp	attribute_time;
/*108*/	udf_Uint32	checkpoint;
/*112*/	udf_long_ad	ext_attribute_icb;
/*128*/	udf_EntityID	impl_ident;
/*160*/	udf_Uint64	unique_id;
/*168*/	udf_Uint32	length_of_ext_attributes;
/*172*/	udf_Uint32	length_of_allocation_descs;
#if 0
/*176*/	udf_ext_attribute_header_desc	ext_attribute_header;
/*200*/	udf_ext_attribute_free_ea_space	ext_attribute_free_ea_space;
/*252*/	udf_ext_attribute_dvd_cgms_info	ext_attribute_dvd_cgms_info;
/*308*/	udf_short_ad	allocation_desc;
/*316*/
#else
	udf_short_ad	allocation_desc;
#endif
} udf_attr_file_entry;

typedef struct udf_filed_entry_ {		/* TR/71 3.5.1 */
/* 0*/	udf_tag		desc_tag;
/*16*/	udf_icbtag	icb_tag;
/*36*/	udf_Uint32	uid;
/*40*/	udf_Uint32	gid;
/*44*/	udf_Uint32	permissions;
/*48*/	udf_Uint16	file_link_count;
/*50*/	udf_Uint8	record_format;
/*51*/	udf_Uint8	record_display_attributes;
/*52*/	udf_Uint32	record_length;
/*56*/	udf_Uint64	info_length;
/*64*/	udf_Uint64	logical_blocks_recorded;
/*72*/	udf_timestamp	access_time;
/*84*/	udf_timestamp	modification_time;
/*96*/	udf_timestamp	attribute_time;
/*108*/	udf_Uint32	checkpoint;
/*112*/	udf_long_ad	ext_attribute_icb;
/*128*/	udf_EntityID	impl_ident;
/*160*/	udf_Uint64	unique_id;
/*168*/	udf_Uint32	length_of_ext_attributes;
/*172*/	udf_Uint32	length_of_allocation_descs;
#if 0
/*176*/	udf_ext_attribute_header_desc	ext_attribute_header;
/*200*/	udf_ext_attribute_free_ea_space	ext_attribute_free_ea_space;
/*252*/	udf_ext_attribute_dvd_cgms_info	ext_attribute_dvd_cgms_info;
/*308*/	udf_short_ad	allocation_desc;
/*316*/
#else
udf_ext_attribute_header_desc			ext_attribute_header;
udf_ext_attribute_free_ea_space			ext_attribute_free_ea_space;
udf_ext_attribute_dvd_cgms_info			ext_attribute_dvd_cgms_info;
udf_ext_attribute_dir_macfinderinfo		ext_attribute_macfinderinfo;
udf_short_ad							allocation_desc;
#endif
} udf_filed_entry;


typedef struct udf_ext_file_entry_ {		/* ECMA 167/3 4/50 */
/* 0*/	udf_tag		desc_tag;	/* 266 */
/*16*/	udf_icbtag	icb_tag;
/*36*/	udf_Uint32	uid;
/*40*/	udf_Uint32	gid;
/*44*/	udf_Uint32	permissions;
/*48*/	udf_Uint16	file_link_count;
/*50*/	udf_Uint8	record_format;
/*51*/	udf_Uint8	record_display_attributes;
/*52*/	udf_Uint32	record_length;
/*56*/	udf_Uint64	info_length;
/* */		udf_Uint64		object_size;
/*64+8*/	udf_Uint64		logical_blocks_recorded;
/*72*/		udf_timestamp	access_time;
/*84*/		udf_timestamp	modification_time;
/* */		udf_timestamp	creation_time;
/*96+12*/	udf_timestamp	attribute_time;
/*108*/		udf_Uint32		checkpoint;
/* */		udf_Uint32		reserved;
/*112+4*/	udf_long_ad		ext_attribute_icb;
/* */		udf_long_ad		stream_dir_icb;
/*128+16*/	udf_EntityID	impl_ident;
/*160*/		udf_Uint64		unique_id;
/*168*/		udf_Uint32		length_of_ext_attributes;
/*172*/		udf_Uint32		length_of_allocation_descs;
			udf_short_ad	allocation_desc;
} udf_ext_file_entry;


/*
 * (U,G,O) = (owner, group, other)
 * (X,R) = (execute, read)
 *
 * There are Write, Change Attribute and Delete permissions also,
 * but it is not permitted to set them on DVD Read-Only media.
 */
#define	UDF_FILEENTRY_PERMISSION_OX	1	/* TR/71 3.5.4 */
#define	UDF_FILEENTRY_PERMISSION_OW	2
#define	UDF_FILEENTRY_PERMISSION_OR	4
#define	UDF_FILEENTRY_PERMISSION_GX	32
#define	UDF_FILEENTRY_PERMISSION_GW	64
#define	UDF_FILEENTRY_PERMISSION_GR	128
#define	UDF_FILEENTRY_PERMISSION_UX	1024
#define	UDF_FILEENTRY_PERMISSION_UW	2048
#define	UDF_FILEENTRY_PERMISSION_UR	4096


#endif	/* _UDF_FS_H */
