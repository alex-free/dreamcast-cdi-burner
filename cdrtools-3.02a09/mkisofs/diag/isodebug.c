/* @(#)isodebug.c	1.30 15/11/29 Copyright 1996-2015 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)isodebug.c	1.30 15/11/29 Copyright 1996-2015 J. Schilling";
#endif
/*
 *	Copyright (c) 1996-2015 J. Schilling
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

#include <schily/stdio.h>
#include <schily/types.h>
#include <schily/stat.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/intcvt.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

#include "../scsi.h"
#include "cdrdeflt.h"
#include "../../cdrecord/version.h"

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

#define	GET_UBYTE(a)	a_to_u_byte(a)
#define	GET_SBYTE(a)	a_to_byte(a)
#define	GET_SHORT(a)	a_to_u_2_byte(&((unsigned char *) (a))[0])
#define	GET_BSHORT(a)	a_to_u_2_byte(&((unsigned char *) (a))[2])
#define	GET_INT(a)	a_to_4_byte(&((unsigned char *) (a))[0])
#define	GET_LINT(a)	la_to_4_byte(&((unsigned char *) (a))[0])
#define	GET_BINT(a)	a_to_4_byte(&((unsigned char *) (a))[4])

#define	infile	in_image
EXPORT	FILE		*infile = NULL;
EXPORT	BOOL		ignerr = FALSE;
LOCAL	int		vol_desc_sum;

LOCAL void	usage		__PR((int excode));
LOCAL char	*isodinfo	__PR((FILE *f));
EXPORT int	main		__PR((int argc, char *argv[]));

LOCAL void
usage(excode)
	int	excode;
{
	errmsgno(EX_BAD, _("Usage: %s [options] image\n"),
						get_progname());

	error(_("Options:\n"));
	error(_("\t-help,-h	Print this help\n"));
	error(_("\t-version	Print version info and exit\n"));
	error(_("\t-ignore-error Ignore errors\n"));
	error(_("\t-i filename	Filename to read ISO-9660 image from\n"));
	error(_("\tdev=target	SCSI target to use as CD/DVD-Recorder\n"));
	error(_("\nIf neither -i nor dev= are speficied, <image> is needed.\n"));
	exit(excode);
}

LOCAL char *
isodinfo(f)
	FILE	*f;
{
static	struct iso9660_voldesc		vd;
	struct iso9660_pr_voldesc	*vp;
#ifndef	USE_SCG
	struct stat			sb;
	mode_t				mode;
#endif
	BOOL				found = FALSE;
	off_t				sec_off = 16L;
	int				i;

#ifndef	USE_SCG
	/*
	 * First check if a bad guy tries to call isosize()
	 * with an unappropriate file descriptor.
	 * return -1 in this case.
	 */
	if (isatty(fileno(f)))
		return (NULL);
	if (fstat(fileno(f), &sb) < 0)
		return (NULL);
	mode = sb.st_mode & S_IFMT;
	if (!S_ISREG(mode) && !S_ISBLK(mode) && !S_ISCHR(mode))
		return (NULL);
#endif

	vp = (struct iso9660_pr_voldesc *) &vd;

	do {
#ifdef	USE_SCG
		readsecs(sec_off, &vd, 1);
#else
		if (lseek(fileno(f), (off_t)(sec_off * 2048L), SEEK_SET) == -1)
			return (NULL);
		read(fileno(f), &vd, sizeof (vd));
#endif
		sec_off++;

		if (GET_UBYTE(vd.vd_type) == VD_PRIMARY) {
			int	j;
			int	s;
			Uchar	*cp;
			/*
			 * Compute checksum used as a fingerprint in case we
			 * include correct inode/link-count information in the
			 * current image.
			 */
			for (j = 0, s = 0, cp = (Uchar *)&vd;
						j < 2048; j++) {
				s += cp[j] & 0xFF;
			}
			vol_desc_sum = s;

			found = TRUE;
/*			break;*/
		}

	} while (GET_UBYTE(vd.vd_type) != VD_TERM);

	if (GET_UBYTE(vd.vd_type) != VD_TERM)
		return (NULL);

#ifdef	USE_SCG
	readsecs(sec_off, &vd, 1);
#else
	if (lseek(fileno(f), (off_t)(sec_off * 2048L), SEEK_SET) == -1)
		return (NULL);
	read(fileno(f), &vd, sizeof (vd));
#endif
	sec_off++;

	if (strncmp((char *)&vd, "MKI ", 4) == 0)
		return ((char *)&vd);

	for (i = 0; i < 16; i++) {
#ifdef	USE_SCG
		readsecs(sec_off, &vd, 1);
#else
		if (lseek(fileno(f), (off_t)(sec_off * 2048L), SEEK_SET) == -1)
			return (NULL);
		read(fileno(f), &vd, sizeof (vd));
#endif
		sec_off++;

		if (strncmp((char *)&vd, "MKI ", 4) == 0)
			break;
	}

	return ((char *)&vd);
}

EXPORT int
main(argc, argv)
	int	argc;
	char	*argv[];
{
	int	cac;
	char	* const *cav;
	char	*opts = "help,h,version,ignore-error,i*,dev*";
	BOOL	help = FALSE;
	BOOL	prvers = FALSE;
	char	*filename = NULL;
	char	*sdevname = NULL;
#if	defined(USE_NLS)
	char	*dir;
#endif
	char	*p;
	char	*eol;

	save_args(argc, argv);

#if	defined(USE_NLS)
	setlocale(LC_ALL, "");
#if !defined(TEXT_DOMAIN)	/* Should be defined by cc -D */
#define	TEXT_DOMAIN "isoinfo"	/* Use this only if it weren't */
#endif
	dir = searchfileinpath("share/locale", F_OK,
					SIP_ANY_FILE|SIP_NO_PATH, NULL);
	if (dir)
		(void) bindtextdomain(TEXT_DOMAIN, dir);
	else
#if defined(PROTOTYPES) && defined(INS_BASE)
	(void) bindtextdomain(TEXT_DOMAIN, INS_BASE "/share/locale");
#else
	(void) bindtextdomain(TEXT_DOMAIN, "/usr/share/locale");
#endif
	(void) textdomain(TEXT_DOMAIN);
#endif

	cac = argc - 1;
	cav = argv + 1;
	if (getallargs(&cac, &cav, opts, &help, &help, &prvers, &ignerr,
			&filename, &sdevname) < 0) {
		errmsgno(EX_BAD, _("Bad Option: '%s'\n"), cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (prvers) {
		printf(_("isodebug %s (%s-%s-%s) Copyright (C) 1996-2015 %s\n"),
					VERSION,
					HOST_CPU, HOST_VENDOR, HOST_OS,
					_("Joerg Schilling"));
		exit(0);
	}
	cac = argc - 1;
	cav = argv + 1;
	if (filename == NULL && sdevname == NULL) {
		if (getfiles(&cac, &cav, opts) != 0) {
			filename = cav[0];
			cac--, cav++;
		}
	}
	if (getfiles(&cac, &cav, opts) != 0) {
		errmsgno(EX_BAD, _("Bad Argument: '%s'\n"), cav[0]);
		usage(EX_BAD);
	}
	if (filename != NULL && sdevname != NULL) {
		errmsgno(EX_BAD, _("Only one of -i or dev= allowed\n"));
		usage(EX_BAD);
	}
#ifdef	USE_SCG
	if (filename == NULL && sdevname == NULL)
		cdr_defaults(&sdevname, NULL, NULL, NULL, NULL);
#endif
	if (filename == NULL && sdevname == NULL) {
		errmsgno(EX_BAD, _("ISO-9660 image not specified\n"));
		usage(EX_BAD);
	}

	if (filename != NULL)
		infile = fopen(filename, "rb");
	else
		filename = sdevname;

	if (infile != NULL) {
		/* EMPTY */;
#ifdef	USE_SCG
	} else if (scsidev_open(filename) < 0) {
#else
	} else {
#endif
		comerr(_("Cannot open '%s'\n"), filename);
	}

	p = isodinfo(infile);
	if (p == NULL) {
		printf(_("No ISO-9660 image debug info.\n"));
	} else if (strncmp(p, "MKI ", 4) == 0) {
		int	sum;

		sum  = p[2045] & 0xFF;
		sum *= 256;
		sum += p[2046] & 0xFF;
		sum *= 256;
		sum += p[2047] & 0xFF;
		p[2045] = '\0';
		if (sum == vol_desc_sum)
			printf(_("ISO-9660 image includes checksum signature for correct inode numbers.\n"));

		eol = strchr(p, '\n');
		if (eol)
			*eol = '\0';
		printf(_("ISO-9660 image created at %s\n"), &p[4]);
		if (eol) {
			printf(_("\nCmdline: '%s'\n"), &eol[1]);
		}
	}
	return (0);
}
