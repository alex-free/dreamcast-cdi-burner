/* @(#)apple.c	1.46 16/10/10 joerg, Copyright 1997, 1998, 1999, 2000 James Pearson, Copyright 2000-2016 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)apple.c	1.46 16/10/10 joerg, Copyright 1997, 1998, 1999, 2000 James Pearson, Copyright 2000-2016 J. Schilling";
#endif
/*
 *      Copyright (c) 1997, 1998, 1999, 2000 James Pearson
 *	Copyright (c) 2000-2016 J. Schilling
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 *	Unix-HFS file interface including maping file extensions to TYPE/CREATOR
 *
 *	Adapted from mkhfs routines for mkhybrid
 *
 *	James Pearson 1/5/97
 *	Bug fix JCP 4/12/97
 *	Updated for 1.12 and added more Unix HFS filetypes. JCP 21/1/98
 *	Tidy up to use Finfo and Dinfo for all formats where
 *		possible JCP 25/4/2000
 *
 *	Things still to de done:
 *
 *		Check file size = finder + rsrc [+ data] is needed
 */

#include "mkisofs.h"
#include <schily/errno.h>
#include <schily/fcntl.h>
#include <schily/utypes.h>
#include <schily/ctype.h>
#include <schily/in.h>
#include <schily/schily.h>

#ifdef APPLE_HYB

#include "apple.h"

/* tidy up mkisofs definition ... */
typedef struct directory_entry dir_ent;

/* routines for getting HFS names and info */
EXPORT	void	del_hfs_info	__PR((struct hfs_info *hfs_info));
EXPORT	void	set_root_info	__PR((char *name));
EXPORT	int	get_hfs_dir	__PR((char *wname, char *dname, dir_ent	*s_entry));
EXPORT	int	get_hfs_info	__PR((char *wname, char	*dname, dir_ent	*s_entry));
EXPORT	int	get_hfs_rname	__PR((char *wname, char *dname, char *rname));
EXPORT	int	hfs_exclude	__PR((char *d_name));
EXPORT	int	hfs_excludepath	__PR((char *path));
EXPORT	void	print_hfs_info	__PR((dir_ent *s_entry));
EXPORT	int	file_is_resource __PR((char *fname, int hfstype));
EXPORT	void	hfs_init	__PR((char *name, Ushort fdflags, Uint hfs_select));
EXPORT	void	delete_rsrc_ent	__PR((dir_ent *s_entry));
EXPORT	void	clean_hfs	__PR((void));
EXPORT	void	perr		__PR((char *a));
#ifndef	HAVE_STRCASECMP
LOCAL	int	strcasecmp	__PR((const char *s1, const char *s2));
#endif
LOCAL	int	get_none_dir	__PR((char *, char *, dir_ent *, int));
LOCAL	int	get_none_info	__PR((char *, char *, dir_ent *, int));
LOCAL	int	get_cap_dir	__PR((char *, char *, dir_ent *, int));
LOCAL	int	get_cap_info	__PR((char *, char *, dir_ent *, int));
LOCAL	int	get_es_dir	__PR((char *, char *, dir_ent *, int));
LOCAL	int	get_es_info	__PR((char *, char *, dir_ent *, int));
LOCAL	int	get_dbl_dir	__PR((char *, char *, dir_ent *, int));
LOCAL	int	get_dbl_info	__PR((char *, char *, dir_ent *, int));
LOCAL	int	get_mb_info	__PR((char *, char *, dir_ent *, int));
LOCAL	int	get_sgl_info	__PR((char *, char *, dir_ent *, int));
LOCAL	int	get_fe_dir	__PR((char *, char *, dir_ent *, int));
LOCAL	int	get_fe_info	__PR((char *, char *, dir_ent *, int));
LOCAL	int	get_sgi_dir	__PR((char *, char *, dir_ent *, int));
LOCAL	int	get_sgi_info	__PR((char *, char *, dir_ent *, int));
LOCAL	int	get_sfm_info	__PR((char *, char *, dir_ent *, int));

#ifdef IS_MACOS_X
LOCAL	int	get_xhfs_dir	__PR((char *, char *, dir_ent *, int));
LOCAL	int	get_xhfs_info	__PR((char *, char *, dir_ent *, int));
#else
#define	get_xhfs_dir	get_none_dir
#define	get_xhfs_info	get_none_info
#endif /* IS_MACOS_X */
#define	OSX_RES_FORK_SUFFIX	"/..namedfork/rsrc"

LOCAL	int	is_pathcomponent	__PR((char *, char *));

LOCAL	void	set_ct		__PR((hfsdirent *, char *, char *));
LOCAL	void	set_Dinfo	__PR((byte *, hfsdirent *));
LOCAL	void	set_Finfo	__PR((byte *, hfsdirent *));
LOCAL	void	cstrncpy	__PR((char *, char *, int));
LOCAL	unsigned char dehex	__PR((char));
LOCAL	unsigned char hex2char	__PR((char *));
LOCAL	void	hstrncpy	__PR((unsigned char *, char *, size_t));
LOCAL	int	read_info_file	__PR((char *, void *, int));

/*LOCAL	unsigned short	calc_mb_crc	__PR((unsigned char *, long, unsigned short));*/
LOCAL	struct hfs_info *get_hfs_fe_info __PR((struct hfs_info *, char *));
LOCAL	struct hfs_info *get_hfs_sgi_info __PR((struct hfs_info *, char *));
LOCAL	struct hfs_info *match_key	__PR((struct hfs_info *, char *));

LOCAL	int	get_hfs_itype	__PR((char *, char *, char *));
LOCAL	void	map_ext		__PR((char *, char **, char **, short *, char *));

LOCAL	afpmap	**map;		/* list of mappings */
LOCAL	afpmap	*defmap;	/* the default mapping */
LOCAL	int	last_ent;	/* previous mapped entry */
LOCAL	int	map_num;	/* number of mappings */
LOCAL	int	mlen;		/* min extension length */
LOCAL	char	tmp[PATH_MAX];	/* tmp working buffer */
LOCAL	int	hfs_num;	/* number of file types */
LOCAL	char	p_buf[PATH_MAX]; /* info working buffer */
LOCAL	FILE	*p_fp = NULL;	/* probe File pointer */
LOCAL	int	p_num = 0;	/* probe bytes read */
LOCAL	unsigned int hselect;	/* type of HFS file selected */

struct hfs_type {	/* Types of various HFS Unix files */
	int	type;	/* type of file */
	int	flags;	/* special flags */
	char	*info;	/* finderinfo name */
	char	*rsrc;	/* resource fork name */
	int	(*get_info) __PR((char *, char *, dir_ent *, int)); /* finderinfo */
								    /*	function */
	int	(*get_dir) __PR((char *, char *, dir_ent *, int));  /* directory */
								    /* name */
								    /* function */
	char	*desc;	/* description */
};

/* Above filled in */
LOCAL struct hfs_type hfs_types[] = {
	{TYPE_NONE, INSERT, "", "", get_none_info, get_none_dir, "None"},
	{TYPE_CAP, INSERT, ".finderinfo/", ".resource/",
				get_cap_info, get_cap_dir, "CAP"},
	{TYPE_NETA, INSERT, ".AppleDouble/", ".AppleDouble/",
				get_dbl_info, get_dbl_dir, "Netatalk"},
	{TYPE_DBL, INSERT, "%", "%", get_dbl_info, get_dbl_dir, "AppleDouble"},
	{TYPE_ESH, INSERT, ".rsrc/", ".rsrc/",
				get_es_info, get_es_dir, "EtherShare/UShare"},
	{TYPE_FEU, NOPEND, "FINDER.DAT", "RESOURCE.FRK/",
				get_fe_info, get_fe_dir, "Exchange"},
	{TYPE_FEL, NOPEND, "finder.dat", "resource.frk/",
				get_fe_info, get_fe_dir, "Exchange"},
	{TYPE_SGI, NOPEND, ".HSancillary", ".HSResource/",
				get_sgi_info, get_sgi_dir, "XINET/SGI"},
	{TYPE_MBIN, PROBE, "", "", get_mb_info, get_none_dir, "MacBinary"},
	{TYPE_SGL, PROBE, "", "", get_sgl_info, get_none_dir, "AppleSingle"},
	{TYPE_DAVE, INSERT, "resource.frk/", "resource.frk/",
				get_dbl_info, get_dbl_dir, "DAVE"},
	{TYPE_SFM, APPEND | NORSRC, ":Afp_AfpInfo", ":Afp_Resource",
				get_sfm_info, get_none_dir, "SFM"},
	{TYPE_XDBL, INSERT, "._", "._", get_dbl_info, get_dbl_dir,
				"MacOS X AppleDouble"},
#ifdef	__needed__
	/*
	 * Causes syslog error since Mac OS X 10.4
	 */
	{TYPE_XHFS, APPEND | NOINFO, "/rsrc", "/rsrc", get_xhfs_info, get_xhfs_dir,
				"MacOS X HFS"}
#endif
	{TYPE_XHFS, APPEND | NOINFO, OSX_RES_FORK_SUFFIX, OSX_RES_FORK_SUFFIX, get_xhfs_info, get_xhfs_dir,
				"MacOS X HFS"},
};

/* used by get_magic_match() return */
LOCAL	char	tmp_type[CT_SIZE + 1];
LOCAL	char	tmp_creator[CT_SIZE + 1];

#ifdef	__used__
/*
 *	An array useful for CRC calculations that use 0x1021 as the "seed"
 *	taken from mcvert.c modified by Jim Van Verth.
 */

LOCAL	unsigned short mb_magic[] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
	0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
	0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
	0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
	0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
	0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
	0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
	0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
	0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
	0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
	0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
	0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
	0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
	0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
	0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
	0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
	0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
	0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
	0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
	0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
	0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
	0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
	0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};

#endif	/* __used__ */

#ifndef	HAVE_STRCASECMP
LOCAL int
strcasecmp(s1, s2)
	const char	*s1;
	const char	*s2;
{
	while (tolower(*s1) == tolower(*s2)) {
		if (*s1 == 0)
			return (0);
		s1++;
		s2++;
	}
	return (tolower(*s1) - tolower(*s2));
}
#endif

/*
 *	set_ct: set CREATOR and TYPE in hfs_ent
 *
 *	CREATOR and TYPE are padded with spaces if not CT_SIZE long
 */

LOCAL void
set_ct(hfs_ent, c, t)
	hfsdirent	*hfs_ent;
	char		*c;
	char		*t;
{
	memset(hfs_ent->u.file.type, ' ', CT_SIZE);
	memset(hfs_ent->u.file.creator, ' ', CT_SIZE);

	strncpy(hfs_ent->u.file.type, t, MIN(CT_SIZE, strlen(t)));
	strncpy(hfs_ent->u.file.creator, c, MIN(CT_SIZE, strlen(c)));

	hfs_ent->u.file.type[CT_SIZE] = '\0';
	hfs_ent->u.file.creator[CT_SIZE] = '\0';
}

/*
 *	cstrncopy: Cap Unix name to HFS name
 *
 *	':' is replaced by '%' and string is terminated with '\0'
 */
LOCAL void
cstrncpy(t, f, c)
	char		*t;
	char		*f;
	int		c;
{
	while (c-- && *f) {
		switch (*f) {
		case ':':
			*t = '%';
			break;
		default:
			*t = *f;
			break;
		}
		t++;
		f++;
	}

	*t = '\0';
}

/*
 * dehex()
 *
 * Given a hexadecimal digit in ASCII, return the integer representation.
 *
 *	Taken from linux/fs/hfs/trans.c by Paul H. Hargrove
 */
#ifdef	PROTOTYPES
LOCAL unsigned char
dehex(char c)
#else
LOCAL unsigned char
dehex(c)
	char	c;

#endif
{
	if ((c >= '0') && (c <= '9')) {
		return (c - '0');
	}
	if ((c >= 'a') && (c <= 'f')) {
		return (c - 'a' + 10);
	}
	if ((c >= 'A') && (c <= 'F')) {
		return (c - 'A' + 10);
	}
/*	return (0xff); */
	return (0);
}

LOCAL unsigned char
hex2char(s)
	char		*s;
{
	unsigned char	i1;
	unsigned char	i2;
	unsigned char	o;

	if (strlen(++s) < 2)
		return (0);

	i1 = (unsigned char) s[0];
	i2 = (unsigned char) s[1];

	if (!isxdigit(i1) || !isxdigit(i2))
		return (0);

	o = (dehex(i1) << 4) & 0xf0;
	o |= (dehex(i2) & 0xf);

	return (o);
}


/*
 *	hstrncpy: Unix name to HFS name with special character
 *	translation.
 *
 *	"%xx" or ":xx" is assumed to be a "special" character and
 *	replaced by character code given by the hex characters "xx"
 *
 *	if "xx" is not a hex number, then it is left alone - except
 *	that ":" is replaced by "%"
 *
 */
LOCAL void
hstrncpy(t, f, tlen)
	unsigned char	*t;
	char		*f;
	size_t		tlen;	/* The to-length */
{
	unsigned char	o;
	size_t		slen = strlen(f);

	while (tlen > 0 && *f) {
		size_t	ofl = slen;
		size_t	otl = tlen;

		switch (*f) {
		case ':':
		case '%':
			if ((o = hex2char(f)) == 0) {
				goto def;
			} else {
				*t++ = o;
				tlen--;
				f += 3;
				slen -= 3;
				continue;
			}
		def:
		default:
			conv_charset(t, &tlen, (unsigned char *)f, &slen, in_nls, hfs_onls);
			break;
		}
		t += otl - tlen;
		f += ofl - slen;
	}
	*t = '\0';
}

/*
 *	basename: find just the filename with any directory component
 */
#ifdef	__needed__
/*
 *	not used at the moment ...
 */
LOCAL char
basename(a)
	char	*a;
{
	char	*b;

	if ((b = strchr(a, '/')))
		return (++b);
	else
		return (a);
}
#endif

/*
 *	set_Dinfo: set directory info
 */
LOCAL void
set_Dinfo(ptr, ent)
	byte		*ptr;
	hfsdirent	*ent;
{
	Dinfo	*dinfo = (Dinfo *)ptr;

	/* finder flags */
	ent->fdflags = d_getw((unsigned char *) dinfo->frFlags);

	if (icon_pos) {
		ent->u.dir.rect.top =
		d_getw((unsigned char *) dinfo->frRect[0]);
		ent->u.dir.rect.left =
		d_getw((unsigned char *) dinfo->frRect[1]);
		ent->u.dir.rect.bottom =
		d_getw((unsigned char *) dinfo->frRect[2]);
		ent->u.dir.rect.right =
		d_getw((unsigned char *) dinfo->frRect[3]);

		ent->fdlocation.v =
		d_getw((unsigned char *) dinfo->frLocation[0]);
		ent->fdlocation.h =
		d_getw((unsigned char *) dinfo->frLocation[1]);

		ent->u.dir.view =
		d_getw((unsigned char *) dinfo->frView);

		ent->u.dir.frscroll.v =
		d_getw((unsigned char *) dinfo->frScroll[0]);
		ent->u.dir.frscroll.h =
		d_getw((unsigned char *) dinfo->frScroll[1]);

	} else {
		/*
		 * clear HFS_FNDR_HASBEENINITED to have tidy desktop ??
		 */
		ent->fdflags &= 0xfeff;
	}
}

/*
 *	set_Finfo: set file info
 */
LOCAL void
set_Finfo(ptr, ent)
	byte		*ptr;
	hfsdirent	*ent;
{
	Finfo	*finfo = (Finfo *)ptr;

	/* type and creator from finder info */
	set_ct(ent, finfo->fdCreator, finfo->fdType);

	/* finder flags */
	ent->fdflags = d_getw((unsigned char *) finfo->fdFlags);

	if (icon_pos) {
		ent->fdlocation.v =
		d_getw((unsigned char *) finfo->fdLocation[0]);
		ent->fdlocation.h =
		d_getw((unsigned char *) finfo->fdLocation[1]);
	} else {
		/*
		 * clear HFS_FNDR_HASBEENINITED to have tidy desktop ??
		 */
		ent->fdflags &= 0xfeff;
	}
}

/*
 *	get_none_dir: ordinary Unix directory
 */
LOCAL int
get_none_dir(hname, dname, s_entry, ret)
	char		*hname;
	char		*dname;
	dir_ent		*s_entry;
	int		ret;
{
	/* just copy the given name */
	hstrncpy((unsigned char *) (s_entry->hfs_ent->name),
							dname, HFS_MAX_FLEN);

	return (ret);
}

/*
 *	get_none_info: ordinary Unix file - try to map extension
 */
LOCAL int
get_none_info(hname, dname, s_entry, ret)
	char		*hname;
	char		*dname;
	dir_ent		*s_entry;
	int		ret;
{
	char		*t,
			*c;
	hfsdirent	*hfs_ent = s_entry->hfs_ent;

	map_ext(dname, &t, &c, &s_entry->hfs_ent->fdflags, s_entry->whole_name);

	/* just copy the given name */
	hstrncpy((unsigned char *) (hfs_ent->name), dname, HFS_MAX_FLEN);

	set_ct(hfs_ent, c, t);

	return (ret);
}

/*
 *	read_info_file:	open and read a finderinfo file for an HFS file
 *			or directory
 */
LOCAL int
read_info_file(name, info, len)
	char		*name;	/* finderinfo filename */
	void		*info;	/* info buffer */
	int		len;	/* length of above */
{
	FILE		*fp;
	int		num;

	/* clear out any old finderinfo stuf */
	memset(info, 0, len);

	if ((fp = fopen(name, "rb")) == NULL)
		return (-1);

	/* read and ignore if the file is short - checked later */
	num = fread(info, 1, len, fp);

	fclose(fp);

	return (num);
}

/*
 *	get_cap_dir: get the CAP name for a directory
 */
LOCAL int
get_cap_dir(hname, dname, s_entry, ret)
	char		*hname;		/* whole path */
	char		*dname;		/* this dir name */
	dir_ent		*s_entry;	/* directory entry */
	int		ret;
{
	FileInfo	info;		/* finderinfo struct */
	int		num = -1;	/* bytes read */
	hfsdirent	*hfs_ent = s_entry->hfs_ent;

	num = read_info_file(hname, &info, sizeof (FileInfo));

	/* check finder info is OK */
	if (num > 0 &&
		info.fi_magic1 == FI_MAGIC1 &&
		info.fi_magic == FI_MAGIC &&
		info.fi_bitmap & FI_BM_MACINTOSHFILENAME) {
		/* use the finderinfo name if it exists */
		cstrncpy((char *)(hfs_ent->name),
				(char *)(info.fi_macfilename), HFS_MAX_FLEN);

		set_Dinfo(info.finderinfo, hfs_ent);

		return (ret);
	} else {
		/* otherwise give it it's Unix name */
		hstrncpy((unsigned char *)(s_entry->hfs_ent->name),
							dname, HFS_MAX_FLEN);
		return (TYPE_NONE);
	}
}

/*
 *	get_cap_info:	get CAP finderinfo for a file
 */
LOCAL int
get_cap_info(hname, dname, s_entry, ret)
	char		*hname;		/* whole path */
	char		*dname;		/* this dir name */
	dir_ent		*s_entry;	/* directory entry */
	int		ret;
{
	FileInfo	info;		/* finderinfo struct */
	int		num = -1;	/* bytes read */
	hfsdirent	*hfs_ent = s_entry->hfs_ent;

	num = read_info_file(hname, &info, sizeof (info));

	/* check finder info is OK */
	if (num > 0 &&
		info.fi_magic1 == FI_MAGIC1 &&
		info.fi_magic == FI_MAGIC) {

		if (info.fi_bitmap & FI_BM_MACINTOSHFILENAME) {
			/* use the finderinfo name if it exists */
			cstrncpy((char *)(hfs_ent->name),
				(char *)(info.fi_macfilename), HFS_MAX_FLEN);
		} else {
			/* use Unix name */
			hstrncpy((unsigned char *)(hfs_ent->name), dname,
								HFS_MAX_FLEN);
		}

		set_Finfo(info.finderinfo, hfs_ent);
#ifdef USE_MAC_DATES
		/*
		 * set created/modified dates - these date should have already
		 * been set from the Unix data fork dates. The finderinfo dates
		 * are in Mac format - but we have to convert them back to Unix
		 * for the time being
		 */
		if ((info.fi_datemagic & FI_CDATE)) {
			/* use libhfs routines to get correct byte order */
			hfs_ent->crdate = d_toutime(d_getl(info.fi_ctime));
		}
		if (info.fi_datemagic & FI_MDATE) {
			hfs_ent->mddate = d_toutime(d_getl(info.fi_mtime));
		}
#endif	/* USE_MAC_DATES */
	} else {
		/* failed to open/read finderinfo - so try afpfile mapping */
		if (verbose > 2) {
			fprintf(stderr,
				_("warning: %s doesn't appear to be a %s file\n"),
				s_entry->whole_name, hfs_types[ret].desc);
		}
		ret = get_none_info(hname, dname, s_entry, TYPE_NONE);
	}

	return (ret);
}

/*
 *	get_es_dir:	get EtherShare/UShare finderinfo for a directory
 *
 *	based on code from Jens-Uwe Mager (jum@helios.de) and Phil Sylvester
 *	<psylvstr@interaccess.com>
 */
LOCAL int
get_es_dir(hname, dname, s_entry, ret)
	char		*hname;		/* whole path */
	char		*dname;		/* this dir name */
	dir_ent		*s_entry;	/* directory entry */
	int		ret;
{
	es_FileInfo	*einfo;		/* EtherShare info struct */
	us_FileInfo	*uinfo;		/* UShare info struct */
	char		info[ES_INFO_SIZE];	/* finderinfo buffer */
	int		num = -1;	/* bytes read */
	hfsdirent	*hfs_ent = s_entry->hfs_ent;

	/*
	 * the EtherShare and UShare file layout is the same, but they store
	 * finderinfo differently
	 */
	einfo = (es_FileInfo *) info;
	uinfo = (us_FileInfo *) info;

	num = read_info_file(hname, info, sizeof (info));

	/* check finder info for EtherShare finderinfo */
	if (num >= (int)sizeof (es_FileInfo) &&
		d_getl(einfo->magic) == ES_MAGIC &&
		d_getw(einfo->version) == ES_VERSION) {

		set_Dinfo(einfo->finderinfo, hfs_ent);

	} else if (num >= (int)sizeof (us_FileInfo)) {
		/*
		 * UShare has no magic number, so we assume that this is a valid
		 * info/resource file ...
		 */

		set_Dinfo(uinfo->finderinfo, hfs_ent);

	} else {
		/* failed to open/read finderinfo - so try afpfile mapping */
		if (verbose > 2) {
			fprintf(stderr,
				_("warning: %s doesn't appear to be a %s file\n"),
				s_entry->whole_name, hfs_types[ret].desc);
		}
		ret = get_none_dir(hname, dname, s_entry, TYPE_NONE);
		return (ret);
	}

	/* set name */
	hstrncpy((unsigned char *) (hfs_ent->name), dname, HFS_MAX_FLEN);

	return (ret);
}

/*
 *	get_es_info:	get EtherShare/UShare finderinfo for a file
 *
 *	based on code from Jens-Uwe Mager (jum@helios.de) and Phil Sylvester
 *	<psylvstr@interaccess.com>
 */
LOCAL int
get_es_info(hname, dname, s_entry, ret)
	char		*hname;		/* whole path */
	char		*dname;		/* this dir name */
	dir_ent		*s_entry;	/* directory entry */
	int		ret;
{
	es_FileInfo	*einfo;		/* EtherShare info struct */
	us_FileInfo	*uinfo;		/* UShare info struct */
	char		info[ES_INFO_SIZE];	/* finderinfo buffer */
	int		num = -1;	/* bytes read */
	hfsdirent	*hfs_ent = s_entry->hfs_ent;
	dir_ent		*s_entry1;

	/*
	 * the EtherShare and UShare file layout is the same, but they store
	 * finderinfo differently
	 */
	einfo = (es_FileInfo *) info;
	uinfo = (us_FileInfo *) info;

	num = read_info_file(hname, info, sizeof (info));

	/* check finder info for EtherShare finderinfo */
	if (num >= (int)sizeof (es_FileInfo) &&
		d_getl(einfo->magic) == ES_MAGIC &&
		d_getw(einfo->version) == ES_VERSION) {

		set_Finfo(einfo->finderinfo, hfs_ent);

		/*
		 * set create date - modified date set from the Unix
		 * data fork date
		 */

		hfs_ent->crdate = d_getl(einfo->createTime);

	} else if (num >= (int)sizeof (us_FileInfo)) {
		/*
		 * UShare has no magic number, so we assume that this is a valid
		 * info/resource file ...
		 */

		set_Finfo(uinfo->finderinfo, hfs_ent);

		/* set create and modified date - if they exist */
		if (d_getl(uinfo->ctime))
			hfs_ent->crdate =
				d_getl(uinfo->ctime);

		if (d_getl(uinfo->mtime))
			hfs_ent->mddate =
				d_getl(uinfo->mtime);
	} else {
		/* failed to open/read finderinfo - so try afpfile mapping */
		if (verbose > 2) {
			fprintf(stderr,
				_("warning: %s doesn't appear to be a %s file\n"),
				s_entry->whole_name, hfs_types[ret].desc);
		}
		ret = get_none_info(hname, dname, s_entry, TYPE_NONE);
		return (ret);
	}

	/* this should exist ... */
	if ((s_entry1 = s_entry->assoc) == NULL)
		perr("TYPE_ESH error - shouldn't happen!");

	/* set name */
	hstrncpy((unsigned char *) (hfs_ent->name), dname, HFS_MAX_FLEN);

	/* real rsrc file starts ES_INFO_SIZE bytes into the file */
	if (s_entry1->size <= ES_INFO_SIZE) {
		s_entry1->size = 0;
		hfs_ent->u.file.rsize = 0;
	} else {
		s_entry1->size -= ES_INFO_SIZE;
		hfs_ent->u.file.rsize = s_entry1->size;
		s_entry1->hfs_off = ES_INFO_SIZE;
	}

	set_733((char *)s_entry1->isorec.size, s_entry1->size);

	return (ret);
}

/*
 * calc_crc() --
 *   Compute the MacBinary II-style CRC for the data pointed to by p, with the
 *   crc seeded to seed.
 *
 *   Modified by Jim Van Verth to use the magic array for efficiency.
 */
#ifdef	__used__
#ifdef	PROTOTYPES
LOCAL unsigned short
calc_mb_crc(unsigned char *p, long len, unsigned short seed)
#else
LOCAL unsigned short
calc_mb_crc(p, len, seed)
	unsigned char	*p;
	long		len;
	unsigned short	seed;

#endif
{
	unsigned short	hold;	/* crc computed so far */
	long		i;	/* index into data */

	hold = seed;	/* start with seed */
	for (i = 0; i < len; i++, p++) {
		hold ^= (*p << 8);
		hold = (hold << 8) ^ mb_magic[(unsigned char) (hold >> 8)];
	}

	return (hold);
} /* calc_mb_crc() */

#endif	/* __used__ */

LOCAL int
get_mb_info(hname, dname, s_entry, ret)
	char		*hname;		/* whole path */
	char		*dname;		/* this dir name */
	dir_ent		*s_entry;	/* directory entry */
	int		ret;
{
	mb_info		*info;		/* finderinfo struct */
	char		*c;
	char		*t;
	hfsdirent	*hfs_ent;
	dir_ent		*s_entry1;
	int		i;

#ifdef TEST_CODE
	unsigned short	crc_file,
			crc_calc;

#endif

	info = (mb_info *) p_buf;

	/*
	 * routine called twice for each file - first to check that it is a
	 * valid MacBinary file, second to fill in the HFS info. p_buf holds
	 * the required raw data and it *should* remain the same between the
	 * two calls
	 */
	if (s_entry == 0) {
		/*
		 * test that the CRC is OK - not set for MacBinary I files (and
		 * incorrect in some MacBinary II files!). If this fails, then
		 * perform some other checks
		 */

#ifdef TEST_CODE
		/* leave this out for the time being ... */
		if (p_num >= MB_SIZE && info->version == 0 && info->zero1 == 0) {
			crc_calc = calc_mb_crc((unsigned char *) info, 124, 0);
			crc_file = d_getw(info->crc);
#ifdef DEBUG
			fprintf(stderr, _("%s: file %d, calc %d\n"), hname,
							crc_file, crc_calc);
#endif	/* DEBUG */
			if (crc_file == crc_calc)
				return (ret);
		}
#endif	/* TEST_CODE */

		/*
		 * check some of the fields for a valid MacBinary file not
		 * zero1 and zero2 SHOULD be zero - but some files incorrect
		 */

/*	    if (p_num < MB_SIZE || info->nlen > 63 || info->zero2 || */
		if (p_num < MB_SIZE || info->zero1 ||
			info->zero2 || info->nlen > 63 ||
			info->version || info->nlen == 0 || *info->name == 0)
			return (TYPE_NONE);

		/* check that the filename is OKish */
		for (i = 0; i < (int)info->nlen; i++)
			if (info->name[i] == 0)
				return (TYPE_NONE);

		/* check CREATOR and TYPE are valid */
		for (i = 0; i < 4; i++)
			if (info->type[i] == 0 || info->auth[i] == 0)
				return (TYPE_NONE);
	} else {
		/* we have a vaild MacBinary file, so fill in the bits */

		/* this should exist ... */
		if ((s_entry1 = s_entry->assoc) == NULL)
			perr("TYPE_MBIN error - shouldn't happen!");

		hfs_ent = s_entry->hfs_ent;

		/* type and creator from finder info */
		t = (char *)(info->type);
		c = (char *)(info->auth);

		set_ct(hfs_ent, c, t);

		/* finder flags */
		hfs_ent->fdflags = ((info->flags << 8) & 0xff00) | info->flags2;

		if (icon_pos) {
			hfs_ent->fdlocation.v =
				d_getw((unsigned char *)info->icon_vert);
			hfs_ent->fdlocation.h =
				d_getw((unsigned char *)info->icon_horiz);
		} else {
			/*
			 * clear HFS_FNDR_HASBEENINITED to have tidy desktop ??
			 */
			hfs_ent->fdflags &= 0xfeff;
		}

		/*
		 * set created/modified dates - these date should have already
		 * been set from the Unix data fork dates. The finderinfo dates
		 * are in Mac format - but we have to convert them back to Unix
		 * for the time being
		 */
		hfs_ent->crdate = d_toutime(d_getl(info->cdate));
		hfs_ent->mddate = d_toutime(d_getl(info->mdate));

		/* set name */
		hstrncpy((unsigned char *)(hfs_ent->name),
			(char *)(info->name), MIN(HFS_MAX_FLEN, info->nlen));

		/* set correct fork sizes */
		hfs_ent->u.file.dsize = d_getl(info->dflen);
		hfs_ent->u.file.rsize = d_getl(info->rflen);

		/* update directory entries for data fork */
		s_entry->size = hfs_ent->u.file.dsize;
		s_entry->hfs_off = MB_SIZE;
		set_733((char *)s_entry->isorec.size, s_entry->size);

		/*
		 * real rsrc file starts after data fork (must be a multiple of
		 * MB_SIZE)
		 */
		s_entry1->size = hfs_ent->u.file.rsize;
		s_entry1->hfs_off = MB_SIZE + ROUND_UP(hfs_ent->u.file.dsize, MB_SIZE);
		set_733((char *)s_entry1->isorec.size, s_entry1->size);
	}

	return (ret);
}

/*
 *	get_dbl_dir:	get Apple double finderinfo for a directory
 *
 *	Based on code from cvt2cap.c (c) May 1988, Paul Campbell
 */
LOCAL int
get_dbl_dir(hname, dname, s_entry, ret)
	char		*hname;		/* whole path */
	char		*dname;		/* this dir name */
	dir_ent		*s_entry;	/* directory entry */
	int		ret;
{
	FileInfo	info;		/* finderinfo struct */
	a_hdr		*hp;
	a_entry		*ep;
	int		num = -1;	/* bytes read */
	int		nentries;
	FILE		*fp;
	hfsdirent	*hfs_ent = s_entry->hfs_ent;
	char		name[64];
	int		i;
	int		fail = 0;
	int		len = 0;

	hp = (a_hdr *) p_buf;
	memset(hp, 0, A_HDR_SIZE);

	memset(name, 0, sizeof (name));

	/* open and read the info/rsrc file (it's the same file) */
	if ((fp = fopen(hname, "rb")) != NULL)
		num = fread(hp, 1, A_HDR_SIZE, fp);

	/*
	 * check finder info is OK - some Netatalk files don't have magic
	 * or version set - ignore if it's a netatalk file
	 */
	if (num == A_HDR_SIZE && ((ret == TYPE_NETA) ||
			(d_getl(hp->magic) == APPLE_DOUBLE &&
				(d_getl(hp->version) == A_VERSION1 ||
					d_getl(hp->version) == A_VERSION2)))) {

		/* read TOC of the AppleDouble file */
		nentries = (int)d_getw(hp->nentries);
		if (fread(hp->entries, A_ENTRY_SIZE, nentries, fp) < 1) {
			fail = 1;
			nentries = 0;
		}
		/* extract what is needed */
		for (i = 0, ep = hp->entries; i < nentries; i++, ep++) {
			switch ((int)d_getl(ep->id)) {
			case ID_FINDER:
				/* get the finder info */
				fseek(fp, (off_t)d_getl(ep->offset), SEEK_SET);
				if (fread(&info, d_getl(ep->length), 1, fp) < 1) {
					fail = 1;
				}
				break;
			case ID_NAME:
				/* get Mac file name */
				fseek(fp, (off_t)d_getl(ep->offset), SEEK_SET);
				if (fread(name, d_getl(ep->length), 1, fp) < 1)
					*name = '\0';
				len = d_getl(ep->length);
				break;
			default:
				break;
			}
		}

		fclose(fp);
		fp = NULL;

		/* skip this if we had a problem */
		if (!fail) {

			set_Dinfo(info.finderinfo, hfs_ent);

			/* use stored name if it exists */
			if (*name) {
				/*
				 * In some cases the name is stored in the
				 * Pascal string format - first char is the
				 * length, the rest is the actual string.
				 * The following *should* be OK
				 */
				if (len == 32 && (int)name[0] < 32) {
					cstrncpy(hfs_ent->name, &name[1],
						MIN(name[0], HFS_MAX_FLEN));
				} else {
					cstrncpy(hfs_ent->name, name,
							HFS_MAX_FLEN);
				}
			} else {
				hstrncpy((unsigned char *)(hfs_ent->name),
							dname, HFS_MAX_FLEN);
			}
		}
	} else {
		/* failed to open/read finderinfo */
		fail = 1;
	}
	if (fp)
		fclose(fp);

	if (fail) {
		/* problem with the file - try mapping/magic */
		if (verbose > 2) {
			fprintf(stderr,
				_("warning: %s doesn't appear to be a %s file\n"),
				s_entry->whole_name, hfs_types[ret].desc);
		}
		ret = get_none_dir(hname, dname, s_entry, TYPE_NONE);
	}
	return (ret);
}

/*
 *	Depending on the version, AppleDouble/Single stores dates
 *	relative to 1st Jan 1904 (v1) or 1st Jan 2000 (v2)
 *
 *	The d_toutime() function uses 1st Jan 1904 to convert to
 *	Unix time (1st Jan 1970).
 *
 *	The d_dtoutime() function uses 1st Jan 2000 to convert to
 *	Unix time (1st Jan 1970).
 *
 *	However, NetaTalk files seem to do their own thing - older
 *	Netatalk files don't have a magic number of version and
 *	store dates in ID=7 (don't know how). Newer Netatalk files
 *	claim to be version 1, but store dates in ID=7 as if they
 *	were version 2 files.
 */

/*
 *	get_dbl_info:	get Apple double finderinfo for a file
 *
 *	Based on code from cvt2cap.c (c) May 1988, Paul Campbell
 */
LOCAL int
get_dbl_info(hname, dname, s_entry, ret)
	char		*hname;		/* whole path */
	char		*dname;		/* this dir name */
	dir_ent		*s_entry;	/* directory entry */
	int		ret;
{
	FileInfo	info;		/* finderinfo struct */
	a_hdr		*hp;
	a_entry		*ep;
	int		num = -1;	/* bytes read */
	int		nentries;
	FILE		*fp;
	hfsdirent	*hfs_ent = s_entry->hfs_ent;
	dir_ent		*s_entry1;
	char		name[64];
	int		i;
	int		fail = 0;
	int		len = 0;
	unsigned char	dates[A_DATE];
	int		ver = 0, dlen;

	hp = (a_hdr *) p_buf;
	memset(hp, 0, A_HDR_SIZE);

	memset(name, 0, sizeof (name));
	memset(dates, 0, sizeof (dates));

	/* get the rsrc file info - should exist ... */
	if ((s_entry1 = s_entry->assoc) == NULL)
		perr("TYPE_DBL error - shouldn't happen!");

	/* open and read the info/rsrc file (it's the same file) */
	if ((fp = fopen(hname, "rb")) != NULL)
		num = fread(hp, 1, A_HDR_SIZE, fp);

	/*
	 * check finder info is OK - some Netatalk files don't have magic
	 * or version set - ignore if it's a netatalk file
	 */

	ver = d_getl(hp->version);
	if (num == A_HDR_SIZE && ((ret == TYPE_NETA) ||
			(d_getl(hp->magic) == APPLE_DOUBLE &&
				(ver == A_VERSION1 || ver == A_VERSION2)))) {

		/* read TOC of the AppleDouble file */
		nentries = (int)d_getw(hp->nentries);
		if (fread(hp->entries, A_ENTRY_SIZE, nentries, fp) < 1) {
			fail = 1;
			nentries = 0;
		}
		/* extract what is needed */
		for (i = 0, ep = hp->entries; i < nentries; i++, ep++) {
			switch ((int)d_getl(ep->id)) {
			case ID_FINDER:
				/* get the finder info */
				fseek(fp, (off_t)d_getl(ep->offset), SEEK_SET);
				if (fread(&info, d_getl(ep->length), 1, fp) < 1) {
					fail = 1;
				}
				break;
			case ID_RESOURCE:
				/* set the offset and correct rsrc fork size */
				s_entry1->size = d_getl(ep->length);
				hfs_ent->u.file.rsize = s_entry1->size;
				/* offset to start of real rsrc fork */
				s_entry1->hfs_off = d_getl(ep->offset);
				set_733((char *)s_entry1->isorec.size,
								s_entry1->size);
				break;
			case ID_NAME:
				/* get Mac file name */
				fseek(fp, (off_t)d_getl(ep->offset), SEEK_SET);
				if (fread(name, d_getl(ep->length), 1, fp) < 1)
					*name = '\0';
				len = d_getl(ep->length);
				break;
			case ID_FILEI:
				/* Workround for NetaTalk files ... */
				if (ret == TYPE_NETA && ver == A_VERSION1)
					ver = A_VERSION2;
				/* fall through */
			case ID_FILEDATESI:
				/* get file info */
				fseek(fp, d_getl(ep->offset), 0);
				dlen = MIN(d_getl(ep->length), A_DATE);
				if (fread(dates, dlen, 1, fp) < 1) {
					fail = 1;
				} else {
					/* get the correct Unix time */
					switch (ver) {

					case (A_VERSION1):
						hfs_ent->crdate =
						d_toutime(d_getl(dates));
						hfs_ent->mddate =
						d_toutime(d_getl(dates+4));
						break;
					case (A_VERSION2):
						hfs_ent->crdate =
						d_dtoutime(d_getl(dates));
						hfs_ent->mddate =
						d_dtoutime(d_getl(dates+4));
						break;
					default:
						/* Use Unix dates */
						break;
					}
				}
				break;
			default:
				break;
			}
		}

		fclose(fp);
		fp = NULL;

		/* skip this if we had a problem */
		if (!fail) {
			set_Finfo(info.finderinfo, hfs_ent);

			/* use stored name if it exists */
			if (*name) {
				/*
				 * In some cases the name is stored in the
				 * Pascal string format - first char is the
				 * length, the rest is the actual string.
				 * The following *should* be OK
				 */
				if (len == 32 && (int)name[0] < 32) {
					cstrncpy(hfs_ent->name, &name[1],
						MIN(name[0], HFS_MAX_FLEN));
				} else {
					cstrncpy(hfs_ent->name, name,
							HFS_MAX_FLEN);
				}
			} else {
				hstrncpy((unsigned char *)(hfs_ent->name),
							dname, HFS_MAX_FLEN);
			}
		}
	} else {
		/* failed to open/read finderinfo */
		fail = 1;
	}
	if (fp)
		fclose(fp);

	if (fail) {
		/* problem with the file - try mapping/magic */
		if (verbose > 2) {
			fprintf(stderr,
				_("warning: %s doesn't appear to be a %s file\n"),
				s_entry->whole_name, hfs_types[ret].desc);
		}
		ret = get_none_info(hname, dname, s_entry, TYPE_NONE);
	}
	return (ret);
}

/*
 *	get_sgl_info:	get Apple single finderinfo for a file
 *
 *	Based on code from cvt2cap.c (c) May 1988, Paul Campbell
 */
LOCAL int
get_sgl_info(hname, dname, s_entry, ret)
	char		*hname;		/* whole path */
	char		*dname;		/* this dir name */
	dir_ent		*s_entry;	/* directory entry */
	int		ret;
{
	FileInfo	*info = 0;	/* finderinfo struct */
	a_hdr		*hp;
static	a_entry		*entries;
	a_entry		*ep;
	int		nentries;
	hfsdirent	*hfs_ent;
	dir_ent		*s_entry1;
	char		name[64];
	int		i;
	int		len = 0;
	unsigned char	*dates;
	int		ver = 0;

	/*
	 * routine called twice for each file
	 * - first to check that it is a valid
	 * AppleSingle file, second to fill in the HFS info.
	 * p_buf holds the required
	 * raw data and it *should* remain the same between the two calls
	 */
	hp = (a_hdr *) p_buf;

	if (s_entry == 0) {
		if (p_num < A_HDR_SIZE ||
			d_getl(hp->magic) != APPLE_SINGLE ||
			(d_getl(hp->version) != A_VERSION1 &&
				d_getl(hp->version) != A_VERSION2))
			return (TYPE_NONE);

		/* check we have TOC for the AppleSingle file */
		nentries = (int)d_getw(hp->nentries);
		if (p_num < (int)(A_HDR_SIZE + nentries * A_ENTRY_SIZE))
			return (TYPE_NONE);

		/* save the TOC */
		entries = (a_entry *) e_malloc(nentries * A_ENTRY_SIZE);

		memcpy(entries, (p_buf + A_HDR_SIZE), nentries * A_ENTRY_SIZE);
	} else {
		/* have a vaild AppleSingle File */
		memset(name, 0, sizeof (name));

		/* get the rsrc file info - should exist ... */
		if ((s_entry1 = s_entry->assoc) == NULL)
			perr("TYPE_SGL error - shouldn't happen!");

		hfs_ent = s_entry->hfs_ent;

		nentries = (int)d_getw(hp->nentries);
		ver = d_getl(hp->version);

		/* extract what is needed */
		for (i = 0, ep = entries; i < nentries; i++, ep++) {
			switch ((int)d_getl(ep->id)) {
			case ID_FINDER:
				/* get the finder info */
				info = (FileInfo *)(p_buf + d_getl(ep->offset));
				break;
			case ID_DATA:
				/* set the offset and correct data fork size */
				hfs_ent->u.file.dsize = s_entry->size =
							d_getl(ep->length);
				/* offset to start of real data fork */
				s_entry->hfs_off = d_getl(ep->offset);
				set_733((char *)s_entry->isorec.size,
								s_entry->size);
				break;
			case ID_RESOURCE:
				/* set the offset and correct rsrc fork size */
				hfs_ent->u.file.rsize = s_entry1->size =
							d_getl(ep->length);
				/* offset to start of real rsrc fork */
				s_entry1->hfs_off = d_getl(ep->offset);
				set_733((char *)s_entry1->isorec.size,
								s_entry1->size);
				break;
			case ID_NAME:
				/* get Mac file name */
				strncpy(name, (p_buf + d_getl(ep->offset)),
					d_getl(ep->length));
				len = d_getl(ep->length);
				break;
			case ID_FILEI:
				/* get file info - ignore at the moment*/
				break;
			case ID_FILEDATESI:
				/* get file info */
				dates = (unsigned char *)p_buf + d_getl(ep->offset);
				/* get the correct Unix time */
				if (ver == A_VERSION1) {
					hfs_ent->crdate =
						d_toutime(d_getl(dates));
					hfs_ent->mddate =
						d_toutime(d_getl(dates+4));
				} else {
					hfs_ent->crdate =
						d_dtoutime(d_getl(dates));
					hfs_ent->mddate =
						d_dtoutime(d_getl(dates+4));
				}
				break;
			default:
				break;
			}
		}

		free(entries);

		if (info == NULL) {
			/*
			 * failed to open/read finderinfo
			 * - so try afpfile mapping
			 */
			if (verbose > 2) {
				fprintf(stderr,
				_("warning: %s doesn't appear to be a %s file\n"),
					s_entry->whole_name,
					hfs_types[ret].desc);
			}
			ret = get_none_info(hname, dname, s_entry, TYPE_NONE);
			return (ret);
		}

		set_Finfo(info->finderinfo, hfs_ent);

		/* use stored name if it exists */
		if (*name) {
			/*
			 * In some cases the name is stored in the Pascal string
			 * format - first char is the length, the rest is the
			 * actual string. The following *should* be OK
			 */
			if (len == 32 && (int)name[0] < 32) {
				cstrncpy(hfs_ent->name, &name[1], MIN(name[0],
								HFS_MAX_FLEN));
			} else {
				cstrncpy(hfs_ent->name, name, HFS_MAX_FLEN);
			}
		} else {
			hstrncpy((unsigned char *)(hfs_ent->name), dname,
								HFS_MAX_FLEN);
		}
	}

	return (ret);
}

/*
 *	get_hfs_fe_info: read in the whole finderinfo for a PC Exchange
 *		directory - saves on reading this many times for each file.
 *
 *	Based of information provided by Mark Weinstein <mrwesq@earthlink.net>
 *
 *	Note: the FINDER.DAT file layout depends on the FAT cluster size
 *	therefore, files should only be read directly from the FAT media
 *
 *	Only tested with PC Exchange v2.1 - don't know if it will work
 *	with v2.2 and above.
 */
LOCAL struct hfs_info *
get_hfs_fe_info(hfs_info, name)
	struct hfs_info	*hfs_info;
	char		*name;
{
	FILE		*fp;
	int		fe_num,
			fe_pad;
	fe_info		info;
	int		c = 0;
	struct hfs_info	*hfs_info1 = NULL;
	char		keyname[12];
	char		*s,
			*e,
			*k;
	int		i;

	/*
	 * no longer attempt to find out FAT cluster
	 * - rely on command line parameter
	 */
	if (afe_size <= 0)
		return (NULL);

	fe_num = afe_size / FE_SIZE;
	fe_pad = afe_size % FE_SIZE;

	if ((fp = fopen(name, "rb")) == NULL)
		return (NULL);

	while (fread(&info, 1, FE_SIZE, fp) != 0) {

		/* the Mac name may be NULL - so ignore this entry */
		if (info.nlen != 0) {

			hfs_info1 =
			(struct hfs_info *)e_malloc(sizeof (struct hfs_info));
			/* add this entry to the list */
			hfs_info1->next = hfs_info;
			hfs_info = hfs_info1;

			/*
			 * get the bits we need
			 * - ignore [cm]time for the moment
			 */
			cstrncpy(hfs_info->name, (char *)(info.name),
					info.nlen);

			memcpy(hfs_info->finderinfo, info.finderinfo, INFOLEN);

			s = (char *)(info.sname);
			e = (char *)(info.ext);
			k = keyname;

			/*
			 * short (Unix) name is stored in PC format,
			 * so needs to be mangled a bit
			 */

			/* name part */
			for (i = 0; i < 8; i++, s++, k++) {
				if (*s == ' ')
					break;
				else
					*k = *s;
			}

			/* extension - if it exists */
			if (strncmp((const char *)(info.ext), "   ", 3)) {
				*k = '.';
				k++;
				for (i = 0; i < 3; i++, e++, k++) {
					if (*e == ' ')
						break;
					else
						*k = *e;
				}
			}
			*k = '\0';

			hfs_info1->keyname = e_strdup(keyname);
		}
		/*
		 * each record is FE_SIZE long, and there are FE_NUM
		 * per each "cluster size", so we may need to skip the padding
		 */
		if (++c == fe_num) {
			c = 0;
			fseek(fp, (off_t)fe_pad, SEEK_CUR);
		}
	}
	fclose(fp);

	return (hfs_info);
}

/*
 *	get_hfs_sgi_info: read in the whole finderinfo for a SGI (XINET)
 *		directory - saves on reading this many times for each
 *		file.
 */
LOCAL struct hfs_info *
get_hfs_sgi_info(hfs_info, name)
	struct hfs_info	*hfs_info;
	char		*name;
{
	FILE		*fp;
	sgi_info	info;
	struct hfs_info	*hfs_info1 = NULL;

	if ((fp = fopen(name, "rb")) == NULL)
		return (NULL);

	while (fread(&info, 1, SGI_SIZE, fp) != 0) {

		hfs_info1 = (struct hfs_info *)e_malloc(sizeof (struct hfs_info));
		/* add this entry to the list */
		hfs_info1->next = hfs_info;
		hfs_info = hfs_info1;

		/* get the bits we need - ignore [cm]time for the moment */
		cstrncpy(hfs_info->name, (char *)info.name, HFS_MAX_FLEN);

		memcpy(hfs_info->finderinfo, info.finderinfo, INFOLEN);

		/* use the HFS name as the key */
		hfs_info1->keyname = hfs_info->name;

	}
	fclose(fp);

	return (hfs_info);
}

/*
 *	del_hfs_info: delete the info list and recover memory
 */
EXPORT void
del_hfs_info(hfs_info)
	struct hfs_info	*hfs_info;
{
	struct hfs_info	*hfs_info1;

	while (hfs_info) {
		hfs_info1 = hfs_info;
		hfs_info = hfs_info->next;

		/* key may be the same as the HFS name - so don't free it */
		*hfs_info1->name = '\0';
		if (*hfs_info1->keyname)
			free(hfs_info1->keyname);
		free(hfs_info1);
	}
}

/*
 *	match_key: find the correct hfs_ent using the Unix filename
 *		as the key
 */
LOCAL struct hfs_info *
match_key(hfs_info, key)
	struct hfs_info	*hfs_info;
	char		*key;

{
	while (hfs_info) {
		if (strcasecmp(key, hfs_info->keyname) == 0)
			return (hfs_info);
		hfs_info = hfs_info->next;
	}

	return (NULL);
}

/*
 *	get_fe_dir: get PC Exchange directory name
 *
 *	base on probing with od ...
 */
LOCAL int
get_fe_dir(hname, dname, s_entry, ret)
	char		*hname;		/* whole path */
	char		*dname;		/* this dir name */
	dir_ent		*s_entry;	/* directory entry */
	int		ret;
{
	struct hfs_info	*hfs_info;
	hfsdirent	*hfs_ent = s_entry->hfs_ent;

	/* cached finderinfo stored with parent directory */
	hfs_info = s_entry->filedir->hfs_info;

	/* if we have no cache, then make one and store it */
	if (hfs_info == NULL) {
		if ((hfs_info = get_hfs_fe_info(hfs_info, hname)) == NULL)
			ret = TYPE_NONE;
		else
			s_entry->filedir->hfs_info = hfs_info;
	}
	if (ret != TYPE_NONE) {
		/* see if we can find the details of this file */
		if ((hfs_info = match_key(hfs_info, dname)) != NULL) {
			strcpy(hfs_ent->name, hfs_info->name);

			set_Dinfo(hfs_info->finderinfo, hfs_ent);

			return (ret);
		}
	}
	/* can't find the entry, so use the Unix name */
	hstrncpy((unsigned char *)(hfs_ent->name), dname, HFS_MAX_FLEN);

	return (TYPE_NONE);
}

/*
 *	get_fe_info: get PC Exchange file details.
 *
 *	base on probing with od and details from Mark Weinstein
 *	<mrwesq@earthlink.net>
 */
LOCAL int
get_fe_info(hname, dname, s_entry, ret)
	char	*hname;		/* whole path */
	char	*dname;		/* this dir name */
	dir_ent	*s_entry;	/* directory entry */
	int	ret;
{
	struct hfs_info	*hfs_info;
	hfsdirent	*hfs_ent = s_entry->hfs_ent;

	/* cached finderinfo stored with parent directory */
	hfs_info = s_entry->filedir->hfs_info;

	/* if we have no cache, then make one and store it */
	if (hfs_info == NULL) {
		if ((hfs_info = get_hfs_fe_info(hfs_info, hname)) == NULL)
			ret = TYPE_NONE;
		else
			s_entry->filedir->hfs_info = hfs_info;
	}
	if (ret != TYPE_NONE) {
		char	*dn = dname;

#ifdef _WIN32_TEST
		/*
		 * may have a problem here - v2.2 has long filenames,
		 * but we need to key on the short filename,
		 * so we need do go a bit of win32 stuff
		 * ...
		 */
		char	sname[1024];
		char	lname[1024];

		cygwin32_conv_to_full_win32_path(s_entry->whole_name, lname);

		if (GetShortPathName(lname, sname, sizeof (sname))) {
			if (dn = strrchr(sname, '\\'))
				dn++;
			else
				dn = sname;
		}
#endif	/* _WIN32 */

		/* see if we can find the details of this file */
		if ((hfs_info = match_key(hfs_info, dn)) != NULL) {

			strcpy(hfs_ent->name, hfs_info->name);

			set_Finfo(hfs_info->finderinfo, hfs_ent);

			return (ret);
		}
	}
	/* no entry found - use extension mapping */
	if (verbose > 2) {
		fprintf(stderr, _("warning: %s doesn't appear to be a %s file\n"),
			s_entry->whole_name, hfs_types[ret].desc);
	}
	ret = get_none_info(hname, dname, s_entry, TYPE_NONE);

	return (TYPE_NONE);
}

/*
 *	get_sgi_dir: get SGI (XINET) HFS directory name
 *
 *	base on probing with od ...
 */
LOCAL int
get_sgi_dir(hname, dname, s_entry, ret)
	char	*hname;		/* whole path */
	char	*dname;		/* this dir name */
	dir_ent	*s_entry;	/* directory entry */
	int	ret;
{
	struct hfs_info	*hfs_info;
	hfsdirent	*hfs_ent = s_entry->hfs_ent;

	/* cached finderinfo stored with parent directory */
	hfs_info = s_entry->filedir->hfs_info;

	/* if we haven't got a cache, then make one */
	if (hfs_info == NULL) {
		if ((hfs_info = get_hfs_sgi_info(hfs_info, hname)) == NULL)
			ret = TYPE_NONE;
		else
			s_entry->filedir->hfs_info = hfs_info;
	}
	/* find the matching entry in the cache */
	if (ret != TYPE_NONE) {
		/* key is (hopefully) the real Mac name */
		cstrncpy(tmp, dname, strlen(dname));
		if ((hfs_info = match_key(hfs_info, tmp)) != NULL) {
			strcpy(hfs_ent->name, hfs_info->name);

			set_Dinfo(hfs_info->finderinfo, hfs_ent);

			return (ret);
		}
	}
	/* no entry found - use Unix name */
	hstrncpy((unsigned char *)(hfs_ent->name), dname, HFS_MAX_FLEN);

	return (TYPE_NONE);
}

/*
 *	get_sgi_info: get SGI (XINET) HFS finder info
 *
 *	base on probing with od ...
 */
LOCAL int
get_sgi_info(hname, dname, s_entry, ret)
	char	*hname;		/* whole path */
	char	*dname;		/* this dir name */
	dir_ent	*s_entry;	/* directory entry */
	int	ret;
{
	struct hfs_info	*hfs_info;
	hfsdirent	*hfs_ent = s_entry->hfs_ent;

	/* cached finderinfo stored with parent directory */
	hfs_info = s_entry->filedir->hfs_info;

	/* if we haven't got a cache, then make one */
	if (hfs_info == NULL) {
		if ((hfs_info = get_hfs_sgi_info(hfs_info, hname)) == NULL)
			ret = TYPE_NONE;
		else
			s_entry->filedir->hfs_info = hfs_info;
	}
	if (ret != TYPE_NONE) {
		/*
		 * tmp is the same as hname here, but we don't need hname
		 * anymore in this function  ...  see if we can find the
		 * details of this file using the Unix name as the key
		 */
		cstrncpy(tmp, dname, strlen(dname));
		if ((hfs_info = match_key(hfs_info, tmp)) != NULL) {

			strcpy(hfs_ent->name, hfs_info->name);

			set_Finfo(hfs_info->finderinfo, hfs_ent);

			return (ret);
		}
	}
	/* no entry found, so try file extension */
	if (verbose > 2) {
		fprintf(stderr, _("warning: %s doesn't appear to be a %s file\n"),
			s_entry->whole_name, hfs_types[ret].desc);
	}
	ret = get_none_info(hname, dname, s_entry, TYPE_NONE);

	return (TYPE_NONE);
}

/*
 *	get_sfm_info:	get SFM finderinfo for a file
 */

LOCAL	byte	sfm_magic[4] = {0x41, 0x46, 0x50, 0x00};
LOCAL	byte	sfm_version[4] = {0x00, 0x00, 0x01, 0x00};

LOCAL int
get_sfm_info(hname, dname, s_entry, ret)
	char	*hname;		/* whole path */
	char	*dname;		/* this dir name */
	dir_ent	*s_entry;	/* directory entry */
	int	ret;
{
	sfm_info	info;	/* finderinfo struct */
	int		num = -1; /* bytes read */
	hfsdirent	*hfs_ent = s_entry->hfs_ent;

	num = read_info_file(hname, &info, sizeof (info));

	/* check finder info is OK */
	if (num == sizeof (info) &&
		!memcmp((char *)info.afpi_Signature, (char *)sfm_magic, 4) &&
		!memcmp((char *)info.afpi_Version, (char *)sfm_version, 4)) {
		/* use Unix name */
		hstrncpy((unsigned char *)(hfs_ent->name), dname, HFS_MAX_FLEN);

		set_Finfo(info.finderinfo, hfs_ent);

	} else {
		/* failed to open/read finderinfo - so try afpfile mapping */
		if (verbose > 2) {
			fprintf(stderr,
				_("warning: %s doesn't appear to be a %s file\n"),
				s_entry->whole_name, hfs_types[ret].desc);
		}
		ret = get_none_info(hname, dname, s_entry, TYPE_NONE);
	}

	return (ret);
}

#ifdef IS_MACOS_X
/*
 *	get_xhfs_dir:	get MacOS X HFS finderinfo for a directory
 *
 *	Code ideas from 'hfstar' by Marcel Weiher marcel@metaobject.com
 *	and another GNU hfstar by Torres Vedras paulotex@yahoo.com
 *
 *	Here we are dealing with actual HFS files - not some encoding
 *	we have to use a system call to get the finderinfo
 *
 *	The file name here is the pseudo name for the resource fork
 *
 *	Notes from HELIOS: we will not use the pseudo name here,
 *	otherwise we will get the info for the resource file
 *	instead of info for the data file.
 */
LOCAL int
get_xhfs_dir(hname, dname, s_entry, ret)
	char		*hname;		/* whole path */
	char		*dname;		/* this dir name */
	dir_ent		*s_entry;	/* directory entry */
	int		ret;
{
	int		err;
	hfsdirent	*hfs_ent = s_entry->hfs_ent;
	attrinfo	ainfo;
	struct attrlist attrs;
	int		i;

	memset(&attrs, 0, sizeof (attrs));
	memset(&ainfo, 0, sizeof (ainfo));

	/* set flags we need to get info from getattrlist() */
	attrs.bitmapcount = ATTR_BIT_MAP_COUNT;
	attrs.commonattr  = ATTR_CMN_CRTIME | ATTR_CMN_MODTIME |
				ATTR_CMN_FNDRINFO;
	attrs.commonattr  |= ATTR_CMN_OBJID;		/* Helios add */

	/* get the info */
	err = getattrlist(hname, &attrs, &ainfo, sizeof (ainfo), 0);

	if (err == 0) {
		/*
		 * If the Finfo is blank then we assume it's not a
		 * 'true' HFS directory ...
		 */
		err = 1;
		for (i = 0; i < sizeof (ainfo.info); i++) {
			if (ainfo.info[i] != 0) {
				err = 0;
				break;
			}
		}
		err = 0;	/* HELIOS: don't do any afpfile mapping */
	}

	/* check finder info is OK */
	if (err == 0) {

		hstrncpy((unsigned char *) (s_entry->hfs_ent->name),
							dname, HFS_MAX_FLEN);

		set_Dinfo(ainfo.info, hfs_ent);

		return (ret);
	} else {
		/* otherwise give it it's Unix name */
		hstrncpy((unsigned char *) (s_entry->hfs_ent->name),
							dname, HFS_MAX_FLEN);
		return (TYPE_NONE);
	}
}

/*
 *	get_xhfs_info:	get MacOS X HFS finderinfo for a file
 *
 *	Code ideas from 'hfstar' by Marcel Weiher marcel@metaobject.com,
 *	another GNU hfstar by Torres Vedras paulotex@yahoo.com and
 *	hfspax by Howard Oakley howard@quercus.demon.co.uk
 *
 *	Here we are dealing with actual HFS files - not some encoding
 *	we have to use a system call to get the finderinfo
 *
 *	The file name here is the pseudo name for the resource fork
 *
 *	Notes from HELIOS: we will not use the pseudo name here,
 *	otherwise we will get the info for the resource file
 *	instead of info for the data file.
 */
LOCAL int
get_xhfs_info(hname, dname, s_entry, ret)
	char		*hname;		/* whole path */
	char		*dname;		/* this dir name */
	dir_ent		*s_entry;	/* directory entry */
	int		ret;
{
	int		err;
	hfsdirent	*hfs_ent = s_entry->hfs_ent;
	attrinfo	ainfo;
	struct attrlist attrs;
	int		i;
	char	tmphname[2048];	/* XXX is this sufficient with -find? */

	strlcpy(tmphname, hname, sizeof (tmphname));
	/*
	 * delete the /..namedfork/rsrc
	 */
	tmphname[strlen(tmphname) - strlen(OSX_RES_FORK_SUFFIX)] = 0;

	memset(&attrs, 0, sizeof (attrs));
	memset(&ainfo, 0, sizeof (ainfo));

	/* set flags we need to get info from getattrlist() */
	attrs.bitmapcount = ATTR_BIT_MAP_COUNT;
	attrs.commonattr  = ATTR_CMN_CRTIME | ATTR_CMN_MODTIME |
				ATTR_CMN_FNDRINFO;
	attrs.commonattr  |= ATTR_CMN_OBJID;		/* Helios add */
	attrs.fileattr = ATTR_FILE_RSRCLENGTH;

	/* get the info */
	err = getattrlist(tmphname, &attrs, &ainfo, sizeof (ainfo), 0);

	/* check finder info is OK */
	if (err == 0) {

		/*
		 * If the Finfo is blank and the resource file is empty,
		 * then we assume it's not a 'true' HFS file ...
		 * There will be not associated file if the resource fork
		 * is empty
		 */

		if (s_entry->assoc == NULL) {
			err = 1;
			for (i = 0; i < sizeof (ainfo.info); i++) {
				if (ainfo.info[i] != 0) {
					err = 0;
					break;
				}
			}
		}

		err = 0;	/* HELIOS: don't do any afpfile mapping */

		if (err == 0) {

			/* use Unix name */
			hstrncpy((unsigned char *) (hfs_ent->name), dname,
						HFS_MAX_FLEN);

			set_Finfo(ainfo.info, hfs_ent);

			/*
			 * dates have already been set - but we will
			 * set them here as well from the HFS info
			 * shouldn't need to check for byte order, as
			 * the source is HFS ... but we will just in case
			 */
			hfs_ent->crdate = d_getl((byte *)&ainfo.ctime.tv_sec);

			hfs_ent->mddate = d_getl((byte *)&ainfo.mtime.tv_sec);
		}

	}

	if (err) {
		/* not a 'true' HFS file - so try afpfile mapping */
#if 0
		/*
		 * don't print a warning as we will get lots on HFS
		 * file systems ...
		 */
		if (verbose > 2) {
			fprintf(stderr,
				_("warning: %s doesn't appear to be a %s file\n"),
				s_entry->whole_name, hfs_types[ret].desc);
		}
#endif
		ret = get_none_info(tmphname, dname, s_entry, TYPE_NONE);
	}

	return (ret);
}
#endif /* IS_MACOS_X */

/*
 *	get_hfs_itype: get the type of HFS info for a file
 */
LOCAL int
get_hfs_itype(wname, dname, htmp)
	char	*wname;
	char	*dname;
	char	*htmp;
{
	int	wlen,
		i;
	int	no_type = TYPE_NONE;

	wlen = strlen(wname) - strlen(dname);

	/* search through the known types looking for matches */
	for (i = 1; i < hfs_num; i++) {
		/* skip the ones that we don't care about */
		if ((hfs_types[i].flags & PROBE) ||
				*(hfs_types[i].info) == TYPE_NONE) {
			continue;
		}

		strcpy(htmp, wname);

		/*
		 * special case - if the info file doesn't exist
		 * for a requested type, then remember the type -
		 * we don't return here, as we _may_ find another type
		 * so we save the type here in case - we will have
		 * problems if more than one of this type ever exists ...
		 */
		if (hfs_types[i].flags & NOINFO) {
			no_type = i;
		} else {

			/* append or insert finderinfo filename part */
			if (hfs_types[i].flags & APPEND)
				strcat(htmp, hfs_types[i].info);
			else
				sprintf(htmp + wlen, "%s%s", hfs_types[i].info,
					(hfs_types[i].flags & NOPEND) ? "" : dname);

			/* hack time ... Netatalk is a special case ... */
			if (i == TYPE_NETA) {
				strcpy(htmp, wname);
				strcat(htmp, "/.AppleDouble/.Parent");
			}

			if (!access(htmp, R_OK))
				return (hfs_types[i].type);
		}
	}

	return (no_type);
}

/*
 *	set_root_info: set the root folder hfs_ent from given file
 */
EXPORT void
set_root_info(name)
	char	*name;
{
	dir_ent		*s_entry;
	hfsdirent	*hfs_ent;
	int		i;

	s_entry = root->self;

	hfs_ent = (hfsdirent *) e_malloc(sizeof (hfsdirent));
	memset(hfs_ent, 0, sizeof (hfsdirent));

	/* make sure root has a valid hfs_ent */
	s_entry->hfs_ent = root->hfs_ent = hfs_ent;

	/* search for correct type of root info data */
	for (i = 1; i < hfs_num; i++) {
		if ((hfs_types[i].flags & PROBE) ||
				(hfs_types[i].get_info == get_none_info))
			continue;

		if ((*(hfs_types[i].get_dir))(name, "", s_entry, i) == i)
			return;
	}
}


/*
 *	get_hfs_dir: set the HFS directory name
 */
EXPORT int
get_hfs_dir(wname, dname, s_entry)
	char	*wname;
	char	*dname;
	dir_ent	*s_entry;
{
	int	type;

	/* get the HFS file type from the info file (if it exists) */
	type = get_hfs_itype(wname, dname, tmp);

	/* try to get the required info */
	type = (*(hfs_types[type].get_dir)) (tmp, dname, s_entry, type);

	return (type);
}

/*
 *	get_hfs_info: set the HFS info for a file
 */
EXPORT int
get_hfs_info(wname, dname, s_entry)
	char	*wname;
	char	*dname;
	dir_ent	*s_entry;
{
	int	type,
		wlen,
		i;

	wlen = strlen(wname) - strlen(dname);

	/* we may already know the type of Unix/HFS file - so process */
	if (s_entry->hfs_type != TYPE_NONE) {

		type = s_entry->hfs_type;

		strcpy(tmp, wname);

		/* append or insert finderinfo filename part */
		if (hfs_types[type].flags & APPEND)
			strcat(tmp, hfs_types[type].info);
		else
			sprintf(tmp + wlen, "%s%s", hfs_types[type].info,
				(hfs_types[type].flags & NOPEND) ? "" : dname);

		type = (*(hfs_types[type].get_info))(tmp, dname, s_entry, type);

		/* if everything is as expected, then return */
		if (s_entry->hfs_type == type)
			return (type);
	}
	/* we don't know what type we have so, find out */
	for (i = 1; i < hfs_num; i++) {
		if ((hfs_types[i].flags & PROBE) ||
				*(hfs_types[i].info) == TYPE_NONE) {
			continue;
		}

		strcpy(tmp, wname);

		/* append or insert finderinfo filename part */
		if (hfs_types[i].flags & APPEND) {
			strcat(tmp, hfs_types[i].info);
		} else {
			sprintf(tmp + wlen, "%s%s", hfs_types[i].info,
				(hfs_types[i].flags & NOPEND) ? "" : dname);
		}

		/* if the file exists - and not a type we've already tried */
		if (!access(tmp, R_OK) && i != s_entry->hfs_type) {
			type = (*(hfs_types[i].get_info))(tmp, dname,
							s_entry, i);
			s_entry->hfs_type = type;
			return (type);
		}
	}

	/* nothing found, so just a Unix file */
	type = (*(hfs_types[TYPE_NONE].get_info))(wname, dname,
							s_entry, TYPE_NONE);

	return (type);
}

/*
 *	get_hfs_rname: set the name of the Unix rsrc file for a file
 *
 *	For the time being we ignore the 'NOINFO' flag - the only case
 *	at the moment is for MacOS X HFS files - for files the resource
 *	fork exists - so testing the "filename/rsrc" pseudo file as
 *	the 'info' filename is OK ...
 */
EXPORT int
get_hfs_rname(wname, dname, rname)
	char	*wname;
	char	*dname;
	char	*rname;
{
	int	wlen,
		type,
		i;
	int	p_fd = -1;

	wlen = strlen(wname) - strlen(dname);

	/* try to find what sort of Unix HFS file type we have */
	for (i = 1; i < hfs_num; i++) {
		/* skip if don't want to probe the files - (default) */
		if (hfs_types[i].flags & PROBE)
			continue;

		strcpy(rname, wname);

		/* if we have a different info file, the find out it's type */
		if (*(hfs_types[i].rsrc) && *(hfs_types[i].info)) {
			/* first test the Info file */

			/* append or insert finderinfo filename part */
			if (hfs_types[i].flags & APPEND) {
				strcat(rname, hfs_types[i].info);
			} else {
				sprintf(rname + wlen, "%s%s", hfs_types[i].info,
					(hfs_types[i].flags & NOPEND) ?
								"" : dname);
			}

			/* if it exists, then check the Rsrc file */
			if (!access(rname, R_OK)) {
				if (hfs_types[i].flags & APPEND) {
					sprintf(rname + wlen, "%s%s", dname,
						hfs_types[i].rsrc);
				} else {
					sprintf(rname + wlen, "%s%s",
						hfs_types[i].rsrc, dname);
				}

				/*
				 * for some types, a rsrc fork may not exist,
				 * so just return the current type
				 * in these cases
				 */
				if (hfs_types[i].flags & NORSRC ||
							!access(rname, R_OK))
					return (hfs_types[i].type);
			}
		} else {
			/*
			 * if we are probing,
			 * then have a look at the contents to find type
			 */
			if (p_fd < 0) {
				/* open file, if not already open */
				if ((p_fd = open(wname,
						O_RDONLY | O_BINARY)) < 0) {
					/* can't open it, then give up */
					return (TYPE_NONE);
				} else {
					if ((p_num = read(p_fd, p_buf,
							sizeof (p_buf))) <= 0) {
						/*
						 * can't read, or zero length
						 * - give up
						 */
						close(p_fd);
						return (TYPE_NONE);
					}
					/* get file pointer file */
					p_fp = fdopen(p_fd, "rb");
					if (p_fp == NULL) {
						close(p_fd);
						return (TYPE_NONE);
					}
				}
			}
			/*
			 * call routine to do the work
			 * - use the given dname as this
			 * is the name we may use on the CD
			 */
			type = (*(hfs_types[i].get_info)) (rname, dname, 0, i);
			if (type != 0) {
				fclose(p_fp);
				return (type);
			}
			if (p_fp) {
				/*
				 * close file
				 * - just use contents of buffer next time
				 */
				fclose(p_fp);
				p_fp = NULL;
			}
		}
	}

	return (0);
}

/*
 *	hfs_exclude: file/directory names that hold finder/resource
 *		     information that we want to exclude from the tree.
 *		     These files/directories are processed later ...
 */
EXPORT int
hfs_exclude(d_name)
	char	*d_name;
{
	/* we don't exclude "." and ".." */
	if (strcmp(d_name, ".") == 0)
		return (0);
	if (strcmp(d_name, "..") == 0)
		return (0);

	/* do not add the following to our list of dir entries */
	if (DO_CAP & hselect) {
		/* CAP */
		if (strcmp(d_name, ".finderinfo") == 0)
			return (1);
		if (strcmp(d_name, ".resource") == 0)
			return (1);
		if (strcmp(d_name, ".ADeskTop") == 0)
			return (1);
		if (strcmp(d_name, ".IDeskTop") == 0)
			return (1);
		if (strcmp(d_name, "Network Trash Folder") == 0)
			return (1);
		/*
		 * special case when HFS volume is mounted using Linux's hfs_fs
		 * Brad Midgley <brad@pht.com>
		 */
		if (strcmp(d_name, ".rootinfo") == 0)
			return (1);
	}
	if (DO_ESH & hselect) {
		/* Helios EtherShare files */
		if (strcmp(d_name, ".rsrc") == 0)
			return (1);
		if (strcmp(d_name, ".Desktop") == 0)
			return (1);
		if (strcmp(d_name, ".DeskServer") == 0)
			return (1);
		if (strcmp(d_name, ".Label") == 0)
			return (1);
	}
	if (DO_DBL & hselect) {
	/* Apple Double */
		/*
		 * special case when HFS volume is mounted using Linux's hfs_fs
		 */
		if (strcmp(d_name, "%RootInfo") == 0)
			return (1);
		/*
		 * have to be careful here - a filename starting with '%'
		 * may be vaild if the next two letters are a hex character -
		 * unfortunately '%' 'digit' 'digit' may be a valid resource
		 * file name ...
		 */
		if (*d_name == '%')
			if (hex2char(d_name) == 0)
				return (1);
	}
	if (DO_NETA & hselect) {
		if (strcmp(d_name, ".AppleDouble") == 0)
			return (1);
		if (strcmp(d_name, ".AppleDesktop") == 0)
			return (1);
	}
	if ((DO_FEU & hselect) || (DO_FEL & hselect)) {
		/* PC Exchange */
		if (strcmp(d_name, "RESOURCE.FRK") == 0)
			return (1);
		if (strcmp(d_name, "FINDER.DAT") == 0)
			return (1);
		if (strcmp(d_name, "DESKTOP") == 0)
			return (1);
		if (strcmp(d_name, "FILEID.DAT") == 0)
			return (1);
		if (strcmp(d_name, "resource.frk") == 0)
			return (1);
		if (strcmp(d_name, "finder.dat") == 0)
			return (1);
		if (strcmp(d_name, "desktop") == 0)
			return (1);
		if (strcmp(d_name, "fileid.dat") == 0)
			return (1);
	}
	if (DO_SGI & hselect) {
		/* SGI */
		if (strcmp(d_name, ".HSResource") == 0)
			return (1);
		if (strcmp(d_name, ".HSancillary") == 0)
			return (1);
	}
	if (DO_DAVE & hselect) {
		/* DAVE */
		if (strcmp(d_name, "resource.frk") == 0)
			return (1);
		if (strcmp(d_name, "DesktopFolderDB") == 0)
			return (1);
	}
#ifndef _WIN32
	/*
	 * NTFS streams are not "seen" as files,
	 * so WinNT will not see these files -
	 * so ignore - used for testing under Unix
	 */
	if (DO_SFM & hselect) {
		/* SFM */
		char	*dn = strrchr(d_name, ':');

		if (dn) {
			if (strcmp(dn, ":Afp_Resource") == 0)
				return (1);
			if (strcmp(dn, ":Comments") == 0)
				return (1);
			if (strcmp(dn, ":Afp_AfpInfo") == 0)
				return (1);
		}
	}
#endif	/* _WIN32 */

	if (DO_XDBL & hselect) {
		/* XDB */
		if (strncmp(d_name, "._", 2) == 0)
			return (1);
	}

	return (0);
}

/*
 *	is_pathcomponent: Check if <compare> is a path component of
 *		<path>. Return 1 if yes and 0 otherwise.
 */
LOCAL int
is_pathcomponent(path, compare)
	char	*path;
	char	*compare;
{
	char	*p, *q;
	char	*r = path;

	while ((p = strstr(r, compare)) != NULL) {
		q = p + strlen(compare);
		if ((*q == 0 || *q == '/') && (p == r || *(p - 1) == '/'))
			return (1);
		r = q;
	}
	return (0);
}

/*
 *	hfs_excludepath: file/directory names that hold finder/resource
 *		     information that we want to exclude from the tree.
 *		     These files/directories are processed later ...
 */
EXPORT int
hfs_excludepath(path)
	char	*path;
{
	/* do not add the following to our list of dir entries */
	if (DO_CAP & hselect) {
		/* CAP */
		if (is_pathcomponent(path, ".finderinfo"))
			return (1);
		if (is_pathcomponent(path, ".resource"))
			return (1);
		if (is_pathcomponent(path, ".ADeskTop"))
			return (1);
		if (is_pathcomponent(path, ".IDeskTop"))
			return (1);
		if (is_pathcomponent(path, "Network Trash Folder"))
			return (1);
		/*
		 * special case when HFS volume is mounted using Linux's hfs_fs
		 * Brad Midgley <brad@pht.com>
		 */
		if (is_pathcomponent(path, ".rootinfo"))
			return (1);
	}
	if ((DO_ESH & hselect)) {
		/* Helios EtherShare files */
		if (is_pathcomponent(path, ".rsrc"))
			return (1);
		if (is_pathcomponent(path, ".Desktop"))
			return (1);
		if (is_pathcomponent(path, ".DeskServer"))
			return (1);
		if (is_pathcomponent(path, ".Label"))
			return (1);
	}
	if (DO_DBL & hselect) {
	/* Apple Double */
		/*
		 * special case when HFS volume is mounted using Linux's hfs_fs
		 */
		if (is_pathcomponent(path, "%RootInfo"))
			return (1);
		/*
		 * have to be careful here - a filename starting with '%'
		 * may be vaild if the next two letters are a hex character -
		 * unfortunately '%' 'digit' 'digit' may be a valid resource
		 * file name ...
		 */
		/* todo!! */
		if (*path == '%')
			if (hex2char(path) == 0)
				return (1);
	}
	if (DO_NETA & hselect) {
		if (is_pathcomponent(path, ".AppleDouble"))
			return (1);
		if (is_pathcomponent(path, ".AppleDesktop"))
			return (1);
	}
	if ((DO_FEU & hselect) || (DO_FEL & hselect)) {
		/* PC Exchange */
		if (is_pathcomponent(path, "RESOURCE.FRK"))
			return (1);
		if (is_pathcomponent(path, "FINDER.DAT"))
			return (1);
		if (is_pathcomponent(path, "DESKTOP"))
			return (1);
		if (is_pathcomponent(path, "FILEID.DAT"))
			return (1);
		if (is_pathcomponent(path, "resource.frk"))
			return (1);
		if (is_pathcomponent(path, "finder.dat"))
			return (1);
		if (is_pathcomponent(path, "desktop"))
			return (1);
		if (is_pathcomponent(path, "fileid.dat"))
			return (1);
	}
	if (DO_SGI & hselect) {
		/* SGI */
		if (is_pathcomponent(path, ".HSResource"))
			return (1);
		if (is_pathcomponent(path, ".HSancillary"))
			return (1);
	}
	if (DO_DAVE & hselect) {
		/* DAVE */
		if (is_pathcomponent(path, "resource.frk"))
			return (1);
		if (is_pathcomponent(path, "DesktopFolderDB"))
			return (1);
	}
#ifndef _WIN32
	/*
	 * NTFS streams are not "seen" as files,
	 * so WinNT will not see these files -
	 * so ignore - used for testing under Unix
	 */
	/* todo!! */
	if (DO_SFM & hselect) {
		/* SFM */
		if (is_pathcomponent(path, ":Afp_Resource"))
			return (1);
		if (is_pathcomponent(path, ":Comments"))
			return (1);
		if (is_pathcomponent(path, ":Afp_AfpInfo"))
			return (1);
	}
#endif	/* _WIN32 */

	if (DO_XDBL & hselect) {
		char	*p;
		char	*r = path;
		char	*compare = "._";
		/* XDB */
		while ((p = strstr(r, compare)) != NULL) {
			if (p == r) {
				if (*(p + strlen(compare)) != 0) {
					return (1);
				}
			} else if (*(p - 1) == '/' && *(p + strlen(compare)) != 0) {
				return (1);
			}
			r += strlen(compare);
		}
	}

	return (0);
}


/*
 *	print_hfs_info: print info about the HFS files.
 *
 */
EXPORT void
print_hfs_info(s_entry)
	dir_ent	*s_entry;
{
	fprintf(stderr, _("Name: %s\n"), s_entry->whole_name);
	fprintf(stderr, _("\tFile type: %s\n"), hfs_types[s_entry->hfs_type].desc);
	fprintf(stderr, _("\tHFS Name: %s\n"), s_entry->hfs_ent->name);
	fprintf(stderr, _("\tISO Name: %s\n"), s_entry->isorec.name);
	fprintf(stderr, _("\tCREATOR: '%s'\n"), s_entry->hfs_ent->u.file.creator);
	fprintf(stderr, _("\tTYPE:	'%s'\n"), s_entry->hfs_ent->u.file.type);
	fprintf(stderr, _("\tFlags:	 %d\n"), s_entry->hfs_ent->fdflags);
	fprintf(stderr, _("\tISO-Size: %ld\n"), (long)get_733(s_entry->isorec.size));
	fprintf(stderr, _("\tSize:     %llu\n"), (Llong)s_entry->size);
	fprintf(stderr, _("\tExtent:	 %ld\n"), (long)get_733(s_entry->isorec.extent));
	if (s_entry->assoc) {
		fprintf(stderr, _("\tResource Name: %s\n"), s_entry->assoc->whole_name);
		fprintf(stderr, _("\t\tISO-Size:	%ld\n"), (long)get_733(s_entry->assoc->isorec.size));
		fprintf(stderr, _("\t\tSize:     %llu\n"), (Llong)s_entry->assoc->size);
		fprintf(stderr, _("\t\tExtent:	%ld\n"), (long)get_733(s_entry->assoc->isorec.extent));
	}
}

/* test if passed file is a resource file */
EXPORT int
file_is_resource(fname, hfstype)
	char	*fname;
	int	hfstype;
{
	char	compare[2048];

	switch (hfstype) {
	case TYPE_NONE:
	case TYPE_MBIN:
	case TYPE_SGL:
		break;
	case TYPE_XHFS:
		strlcpy(compare, hfs_types[hfstype].rsrc, sizeof (compare));
		if (strlen(fname) > strlen(compare)) {
			if (strcmp(&fname[strlen(fname) - strlen(compare)], compare) == 0) {
				return (1);
			}
		}
		break;
	case TYPE_DAVE:
	case TYPE_SGI:
	case TYPE_FEL:
	case TYPE_FEU:
	case TYPE_ESH:
	case TYPE_NETA:
	case TYPE_CAP:
		strcpy(compare, "/");
		strcat(compare, hfs_types[hfstype].rsrc);
		if (strstr(fname, compare) != NULL) {
			return (1);
		}
		break;
	case TYPE_XDBL:
	case TYPE_SFM:
	case TYPE_DBL:
		strcpy(compare, "/");
		strcat(compare, hfs_types[hfstype].rsrc);
		if (strstr(fname, compare) != NULL) {
			return (1);
		}
		break;
	default:
		break;
	}
	return (0);
}

/*
 *	hfs_init: sets up the mapping list from the afpfile as well
 *		 the default mapping (with or without) an afpfile
 */
#ifdef	PROTOTYPES
EXPORT void
hfs_init(char *name, Ushort fdflags, Uint hfs_select)
#else
EXPORT void
hfs_init(name, fdflags, hfs_select)
	char	*name;		/* afpfile name */
	Ushort	fdflags;	/* default finder flags */
	Uint	hfs_select;	/* select certain mac files */

#endif
{
	FILE	*fp;		/* File pointer */
	int	count = NUMMAP;	/* max number of entries */
	char	buf[PATH_MAX];	/* working buffer */
	afpmap	*amap;		/* mapping entry */
	char	*c,
		*t,
		*e;
	int	i;

	/* setup number of Unix/HFS filetype - we may wish to not bother */
	if (hfs_select) {
		hfs_num = sizeof (hfs_types) / sizeof (struct hfs_type);

		/*
		 * code below needs to be tidied up
		 * - most can be made redundant
		 */
		for (i = 0; i < hfs_num; i++)
			hfs_types[i].flags &= ~1;	/* 0xfffffffe */

		for (i = 1; i < hfs_num; i++)
			if (!((1 << i) & hfs_select))
				hfs_types[i].flags |= PROBE;

		hselect = hfs_select;
	} else
		hfs_num = hselect = 0;

#ifdef DEBUG
	for (i = 0; i < hfs_num; i++)
		fprintf(stderr, "type = %d flags = %d\n",
					i, hfs_types[i].flags);
#endif	/* DEBUG */

	/* min length set to max to start with */
	mlen = PATH_MAX;

	/* initialise magic file */
	if (magic_file && init_magic(magic_file) != 0)
		comerr("Unable to open magic file '%s'.\n", magic_file);

	/* set defaults */
	map_num = last_ent = 0;

	/* allocate memory for the default entry */
	defmap = (afpmap *) e_malloc(sizeof (afpmap));

	/* set default values */
	defmap->extn = DEFMATCH;

	/* make sure creator and type are 4 chars long */
	strcpy(defmap->type, BLANK);
	strcpy(defmap->creator, BLANK);

	e = deftype;
	t = defmap->type;

	while (*e && (e - deftype) < CT_SIZE)
		*t++ = *e++;

	e = defcreator;
	c = defmap->creator;

	while (*e && (e - defcreator) < CT_SIZE)
		*c++ = *e++;

	/* length is not important here */
	defmap->elen = 0;

	/* no flags */
	defmap->fdflags = fdflags;

	/* no afpfile - no mappings */
	if (*name == '\0') {
		map = NULL;
		return;
	}
	if ((fp = fopen(name, "r")) == NULL)
		comerr("Unable to open mapping file '%s'.\n", name);

	map = (afpmap **) e_malloc(NUMMAP * sizeof (afpmap *));

	/* read afpfile line by line */
	while (fgets(buf, PATH_MAX, fp) != NULL) {
		/* ignore any comment lines */
		c = tmp;
		*c = '\0';
		if (sscanf(buf, "%1s", c) == EOF || *c == '#')
			continue;

		/* increase list size if needed */
		if (map_num == count) {
			count += NUMMAP;
			map = (afpmap **)realloc(map, count * sizeof (afpmap *));
			if (map == NULL)
				perr("not enough memory for mapping file");
		}
		/* allocate memory for this entry */
		amap = (afpmap *) e_malloc(sizeof (afpmap));

		t = amap->type;
		c = amap->creator;

		/* extract the info */
		if (sscanf(buf, "%s%*s%*1s%c%c%c%c%*1s%*1s%c%c%c%c%*1s",
				tmp, c, c + 1, c + 2, c + 3,
				t, t + 1, t + 2, t + 3) != 9) {
			fprintf(stderr,
				_("error scanning afpfile %s - continuing\n"), name);
			free(amap);
			continue;
		}
		/* copy the extension found */
		amap->extn = e_strdup(tmp);

		/* set end-of-string */
		*(t + 4) = *(c + 4) = '\0';

		/* find the length of the extension */
		amap->elen = strlen(amap->extn);

		/* set flags */
		amap->fdflags = fdflags;

		/* see if we have the default creator/type */
		if (strcmp(amap->extn, DEFMATCH) == 0) {
			/* get rid of the old default */
			free(defmap);
			/* make this the default */
			defmap = amap;
			continue;
		}
		/* update the smallest extension length */
		mlen = MIN(mlen, amap->elen);

		/* add entry to the list */
		map[map_num++] = amap;

	}
	fclose(fp);

	/* free up some memory */
	if (map_num != count) {
		map = (afpmap **) realloc(map, map_num * sizeof (afpmap *));
		if (map == NULL)
			perr("not enough memory for mapping file");
	}
}

/*
 *	map_ext: map a files extension with the list to get type/creator
 */
LOCAL void
map_ext(name, type, creator, fdflags, whole_name)
	char	*name;		/* filename */
	char	**type;		/* set type */
	char	**creator;	/* set creator */
	short	*fdflags;	/* set finder flags */
	char	*whole_name;
{
	int	i;		/* loop counter */
	int	len;		/* filename length */
	afpmap	*amap;		/* mapping entry */
	char	*ret;

	/* we don't take fdflags from the map or magic file */
	*fdflags = defmap->fdflags;

	/*
	 * if we have a magic file and we want to search it first,
	 * then try to get a match
	 */
	if (magic_file && hfs_last == MAP_LAST) {
		ret = get_magic_match(whole_name);

		if (ret) {
			if (sscanf(ret, "%4s%4s", tmp_creator, tmp_type) == 2) {
				*type = tmp_type;
				*creator = tmp_creator;
				return;
			}
		}
	}
	len = strlen(name);

	/* have an afpfile and filename if long enough */
	if (map && len >= mlen) {
		/*
		 * search through the list - we start where we left off
		 * last time in case this file is of the same type as the
		 * last one
		 */
		for (i = 0; i < map_num; i++) {
			amap = map[last_ent];

			/* compare the end of the filename */
/*			if (strcmp((name+len - amap->elen), amap->extn) == 0) { */
			if (strcasecmp((name+len - amap->elen), amap->extn) == 0) {
				/* set the required info */
				*type = amap->type;
				*creator = amap->creator;
				*fdflags = amap->fdflags;
				return;
			}
			/*
			 * move on to the next entry - wrapping round
			 * if neccessary
			 */
			last_ent++;
			last_ent %= map_num;
		}
	}
	/*
	 * if no matches are found, file name too short, or no afpfile,
	 * then take defaults
	 */
	*type = defmap->type;
	*creator = defmap->creator;

	/*
	 * if we have a magic file and we haven't searched yet,
	 * then try to get a match
	 */
	if (magic_file && hfs_last == MAG_LAST) {
		ret = get_magic_match(whole_name);

		if (ret) {
			if (sscanf(ret, "%4s%4s", tmp_creator, tmp_type) == 2) {
				*type = tmp_type;
				*creator = tmp_creator;
			}
		}
	}
}

EXPORT void
delete_rsrc_ent(s_entry)
	dir_ent	*s_entry;
{
	dir_ent	*s_entry1 = s_entry->next;

	if (s_entry1 == NULL)
		return;

	s_entry->next = s_entry1->next;
	s_entry->assoc = NULL;

	free(s_entry1->name);
	free(s_entry1->whole_name);

	free(s_entry1);
}

EXPORT void
clean_hfs()
{
	if (map)
		free(map);

	if (defmap)
		free(defmap);

	if (magic_file)
		clean_magic();
}

/*
 * We are in hope that errno is set up by libhfs_iso if there
 * is no system error code.
 */
EXPORT void
perr(a)
	char	*a;
{
	if (a)
		comerr("%s\n", _(a));
	else
		comerr(_("<no error message given>\n"));
}
#endif	/* APPLE_HYB */


#ifndef APPLE_HFS_HYB

/*
 * Convert 2 bytes in big-endian format into local host format
 */
EXPORT short
d_getw(p)
	Uchar	*p;
{
	return ((short)((p[0] << 8) | p[1]));
}

/*
 * Convert 4 bytes in big-endian format into local host format
 */
EXPORT long
d_getl(p)
	Uchar	*p;
{
	return ((long)((p[0] << 24) | (p[1] << 16) | (p[2] <<  8) | p[3]));
}

/*
 *	Apple v1 strores dates beginnign with 1st Jan 1904
 *	Apple v2 strores dates beginnign with 1st Jan 2000
 */
#define	V2TDIFF 946684800L	/* 30 years (1970 .. 2000)	*/
#define	V1TDIFF	2082844800L	/* 66 years (1904 .. 1970)	*/
#define	TZNONE	0x0F0F0F0F	/* no valid time		*/

LOCAL unsigned long tzdiff = TZNONE;

/*
 * Calculate the timezone difference between local time and UTC
 */
LOCAL void
inittzdiff()
{
	time_t		now;
	struct tm	tm;
	struct tm	*lmp;
	struct tm	*gmp;

	time(&now);
	lmp = localtime(&now);
	gmp = gmtime(&now);

	tzdiff = 0;
	if (lmp && gmp) {
		tm = *gmp;
		tm.tm_isdst = lmp->tm_isdst;

		tzdiff = now - mktime(&tm);
	}
}

/*
 * Convert Macintosh time to UNIX time
 */
EXPORT unsigned long
d_toutime(secs)
	unsigned long	secs;
{
	time_t utime = secs;

	if (tzdiff == TZNONE)
		inittzdiff();

	return (utime - V1TDIFF - tzdiff);
}

/*
 * Convert Apple Double v2 time to UNIX time
 */
EXPORT unsigned long
d_dtoutime(secs)
	long		secs;
{
	time_t utime = secs;

	if (tzdiff == TZNONE)
		inittzdiff();

	return (utime + V2TDIFF - tzdiff);
}
#endif	/* !APPLE_HFS_HYB */
