/* @(#)isoinfo.c	1.108 16/08/10 joerg */
#include <schily/mconfig.h>
#ifndef	lint
static	UConst char sccsid[] =
	"@(#)isoinfo.c	1.108 16/08/10 joerg";
#endif
/*
 * File isodump.c - dump iso9660 directory information.
 *
 *
 * Written by Eric Youngdale (1993).
 *
 * Copyright 1993 Yggdrasil Computing, Incorporated
 * Copyright (c) 1999-2016 J. Schilling
 *
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

/*
 * Simple program to dump contents of iso9660 image in more usable format.
 *
 * Usage:
 * To list contents of image (with or without RR):
 *	isoinfo -l [-R] -i imagefile
 * To extract file from image:
 *	isoinfo -i imagefile -x xtractfile > outfile
 * To generate a "find" like list of files:
 *	isoinfo -f -i imagefile
 */

#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>

#include <schily/stdio.h>
#include <schily/utypes.h>
#include <schily/standard.h>
#include <schily/signal.h>
#include <schily/stat.h>
#include <schily/device.h>
#include <schily/time.h>
#include <schily/fcntl.h>
#include <schily/intcvt.h>
#include <schily/nlsdefs.h>
#include <schily/getargs.h>
#include <schily/nlsdefs.h>
#include <schily/ctype.h>
#include <schily/errno.h>
#include <schily/schily.h>

#include "../iso9660.h"
#include "../rock.h"
#include "../scsi.h"
#include "cdrdeflt.h"
#include "../../cdrecord/version.h"

#include <schily/siconv.h>
#include <schily/io.h>				/* for setmode() prototype */
#ifdef	USE_FIND
#include <schily/walk.h>
#include <schily/find.h>
#endif

/*
 * Make sure we have a definition for this.  If not, take a very conservative
 * guess.
 * POSIX requires the max pathname component lenght to be defined in limits.h
 * If variable, it may be undefined. If undefined, there should be
 * a definition for _POSIX_NAME_MAX in limits.h or in unistd.h
 * As _POSIX_NAME_MAX is defined to 14, we cannot use it.
 * XXX Eric's wrong comment:
 * XXX From what I can tell SunOS is the only one with this trouble.
 */
#include <schily/limits.h>

#ifndef	NAME_MAX
#ifdef	FILENAME_MAX
#define	NAME_MAX	FILENAME_MAX
#else
#define	NAME_MAX	256
#endif
#endif

#ifndef	PATH_MAX
#ifdef	FILENAME_MAX
#define	PATH_MAX	FILENAME_MAX
#else
#define	PATH_MAX	1024
#endif
#endif

/*
 * XXX JS: Some structures have odd lengths!
 * Some compilers (e.g. on Sun3/mc68020) padd the structures to even length.
 * For this reason, we cannot use sizeof (struct iso_path_table) or
 * sizeof (struct iso_directory_record) to compute on disk sizes.
 * Instead, we use offsetof(..., name) and add the name size.
 * See iso9660.h
 */
#ifndef	offsetof
#define	offsetof(TYPE, MEMBER)	((size_t) &((TYPE *)0)->MEMBER)
#endif

/*
 * Note: always use these macros to avoid problems.
 *
 * ISO_ROUND_UP(X)	may cause an integer overflow and thus give
 *			incorrect results. So avoid it if possible.
 *
 * ISO_BLOCKS(X)	is overflow safe. Prefer this when ever it is possible.
 */
#define	SECTOR_SIZE	(2048)
#define	ISO_ROUND_UP(X)	(((X) + (SECTOR_SIZE - 1)) & ~(SECTOR_SIZE - 1))
#define	ISO_BLOCKS(X)	(((X) / SECTOR_SIZE) + (((X)%SECTOR_SIZE)?1:0))

#define	GET_UBYTE(a)	a_to_u_byte(a)

#define	infile	in_image
EXPORT FILE	*infile = NULL;
EXPORT BOOL	ignerr = FALSE;
LOCAL int	use_rock = 0;
LOCAL int	use_joliet = 0;
LOCAL int	do_listing = 0;
LOCAL int	do_f = 0;
LOCAL int	do_find = 0;
LOCAL int	find_ac	  = 0;		/* ac past -find option		*/
LOCAL char	*const *find_av = NULL;	/* av past -find option		*/
LOCAL int	find_pac  = 0;		/* ac for first find primary	*/
LOCAL char	*const *find_pav = NULL; /* av for first find primary	*/
LOCAL int	do_sectors = 0;
LOCAL int	do_pathtab = 0;
LOCAL int	do_pvd = 0;
LOCAL BOOL	debug = FALSE;
LOCAL char	*xtract = 0;
LOCAL BOOL	Xtract = FALSE;
LOCAL char	er_id[256];
LOCAL int	su_version = 0;
LOCAL int	rr_version = 0;
LOCAL int	aa_version = 0;
LOCAL int	cl_extent = 0;
LOCAL int	ucs_level = 0;
LOCAL BOOL	iso9660_inodes = FALSE;
LOCAL uid_t	myuid;

#ifdef	USE_FIND
LOCAL findn_t	*find_node;		/* syntaxtree from find_parse()	*/
LOCAL void	*plusp;			/* residual for -exec ...{} +	*/
LOCAL int	find_patlen;		/* len for -find pattern state	*/
LOCAL BOOL	find_print = FALSE;	/* -print or -ls primary found	*/

LOCAL 	int		walkflags = WALK_CHDIR | WALK_PHYS | WALK_NOEXIT;
LOCAL	int		maxdepth = -1;
LOCAL	int		mindepth = -1;
LOCAL	struct WALK	walkstate;
#endif

LOCAL struct stat	fstat_buf;
LOCAL int		found_rr;
LOCAL char		name_buf[256*3];
LOCAL char		xname[8192];
LOCAL unsigned char	date_buf[9];	/* iso_directory_record.date[7] */
/*
 * Use sector_offset != 0 (-N #) if we have an image file
 * of a single session and we need to list the directory contents.
 * This is the session block (sector) number of the start
 * of the session when it would be on disk.
 */
LOCAL unsigned int	sector_offset = 0;

LOCAL unsigned char	buffer[2048];

LOCAL siconvt_t	*unls;

#define	PAGE sizeof (buffer)

#define	ISODCL(from, to) (to - from + 1)

struct todo
{
	struct todo	*next;
	struct todo	*prev;
	char		*name;
	int		extent;
	int		length;
};

LOCAL struct todo	*todo_idr = NULL;
LOCAL struct todo	**todo_pp = &todo_idr;


LOCAL	int	isonum_721	__PR((char * p));
LOCAL	int	isonum_723	__PR((char * p));
LOCAL	int	isonum_731	__PR((char * p));
LOCAL	int	isonum_732	__PR((char * p));
LOCAL	int	isonum_733	__PR((unsigned char * p));
LOCAL	void	printchars	__PR((char *s, int n, BOOL ucs));
LOCAL	char	*sdate		__PR((char *dp));
LOCAL	void	dump_pathtab	__PR((int block, int size));
LOCAL	int	parse_rr	__PR((unsigned char * pnt, int len,
					int cont_flag));
LOCAL	void	find_rr		__PR((struct iso_directory_record * idr,
					Uchar **pntp, int *lenp));
LOCAL	int	dump_rr		__PR((struct iso_directory_record * idr));
LOCAL	BOOL	dump_stat	__PR((char *rootname,
					struct iso_directory_record * idr,
					char *fname,
					int extent));
LOCAL	void	extract		__PR((char *rootname,
					struct iso_directory_record * idr,
					char *fname));
LOCAL	void	extract_file	__PR((int f,
					struct iso_directory_record * idr,
					char *fname));
LOCAL	void	parse_cl_dir	__PR((struct iso_directory_record *idr,
					int extent));
LOCAL	BOOL	parse_de	__PR((struct iso_directory_record *idr));
LOCAL	void	parse_dir	__PR((struct todo *dp,
					char * rootname, int extent, int len));
LOCAL	void	usage		__PR((int excode));
EXPORT	int	main		__PR((int argc, char *argv[]));
LOCAL	void	list_vd		__PR((struct iso_primary_descriptor *vp, BOOL ucs));
LOCAL	void	list_locales	__PR((void));
LOCAL	void	printf_bootinfo	__PR((FILE *f, int bootcat_offset));
LOCAL	char	*arch_name	__PR((int val));
LOCAL	char	*boot_name	__PR((int val));
LOCAL	char	*bootmedia_name	__PR((int val));
LOCAL	int	time_cvt	__PR((unsigned char *dp, int len));
LOCAL	time_t	iso9660_time	__PR((unsigned char *date, int *hsecp,
					BOOL longfmt));
#ifdef	USE_FIND
LOCAL	int	getfind		__PR((char *arg, long *valp,
					int *pac, char *const **pav));
LOCAL	BOOL	find_stat	__PR((char *rootname,
					struct iso_directory_record * idr,
					char *fname,
					int extent));
#endif



LOCAL int
isonum_721(p)
	char	*p;
{
	return ((p[0] & 0xff)
		| ((p[1] & 0xff) << 8));
}

LOCAL int
isonum_723(p)
	char * p;
{
#if 0
	if (p[0] != p[3] || p[1] != p[2]) {
		fprintf(stderr, "invalid format 7.2.3 number\n");
		exit(1);
	}
#endif
	return (isonum_721(p));
}

LOCAL int
isonum_731(p)
	char	*p;
{
	return ((p[0] & 0xff)
		| ((p[1] & 0xff) << 8)
		| ((p[2] & 0xff) << 16)
		| ((p[3] & 0xff) << 24));
}

LOCAL int
isonum_732(p)
	char	*p;
{
	return ((p[3] & 0xff)
		| ((p[2] & 0xff) << 8)
		| ((p[1] & 0xff) << 16)
		| ((p[0] & 0xff) << 24));
}

LOCAL int
isonum_733(p)
	unsigned char	*p;
{
	return (isonum_731((char *)p));
}

LOCAL void
printchars(s, n, ucs)
	char	*s;
	int	n;
	BOOL	ucs;
{
	int	i;
	char	*p;
	int	c;

	for (; n > 0 && (*s || (ucs && s[1])); n--) {
		if (ucs) {
			c = *s++ & 0xFF;
			c *= 256;
			c += *s++ & 0xFF;
			n--;
		} else {
			c = *s++;
		}
		if (c == ' ') {
			int	c2;
			p = s;
			i = n;
			while (--i >= 0) {
				if (ucs) {
					c2 = *p++ & 0xFF;
					c2 *= 256;
					c2 += *p++ & 0xFF;
					i--;
				} else {
					c2 = *p++;
				}
				if (c2 != ' ')
					break;
			}
			if (i <= 0)
				break;
		}
		putchar(c);
	}
}

/*
 * Print date info from PVD
 */
LOCAL char *
sdate(dp)
	char	*dp;
{
	static	char	d[30];

	js_sprintf(d, "%4.4s %2.2s %2.2s %2.2s:%2.2s:%2.2s.%2.2s",
			&dp[0],		/* Year */
			&dp[4],		/* Month */
			&dp[6],		/* Monthday */
			&dp[8],		/* Hour */
			&dp[10],	/* Minute */
			&dp[12],	/* Seconds */
			&dp[14]);	/* Hunreds of a Seconds */

	/*
	 * dp[16] contains minute offset from Greenwich
	 * Positive values are to the east of Greenwich.
	 */
	return (d);
}

LOCAL void
dump_pathtab(block, size)
	int	block;
	int	size;
{
	unsigned char	*buf;
	int		offset;
	int		idx;
	int		extent;
	int		pindex;
	int		j;
	int		len;
	int		jlen;
	char		namebuf[256*3];
	unsigned char	uc;


	printf(_("Path table starts at block %d, size %d\n"), block, size);

	buf = (unsigned char *) malloc(ISO_ROUND_UP(size));

#ifdef	USE_SCG
	readsecs(block - sector_offset, buf, ISO_BLOCKS(size));
#else
	lseek(fileno(infile), ((off_t)(block - sector_offset)) << 11, SEEK_SET);
	read(fileno(infile), buf, size);
#endif

	offset = 0;
	idx = 1;
	while (offset < size) {
		len    = buf[offset];
		extent = isonum_731((char *)buf + offset + 2);
		pindex  = isonum_721((char *)buf + offset + 6);
		switch (ucs_level) {
		case 3:
		case 2:
		case 1:
			jlen = len/2;
			namebuf[0] = '\0';
#ifdef	USE_ICONV
#ifdef	HAVE_ICONV_CONST
#define	__IC_CONST	const
#else
#define	__IC_CONST
#endif
			if (use_iconv(unls)) {
				int	u;
				char	*to = namebuf;

				for (j = 0, u = 0; j < jlen; j++) {
					char	*ibuf = (char *)&buf[offset + 8 + j*2];
					size_t	isize = 2;		/* UCS-2 character size */
					size_t	osize = 4;

					if (iconv(unls->sic_uni2cd, (__IC_CONST char **)&ibuf, &isize,
							(char **)&to, &osize) == -1) {
						int	err = geterrno();

						if ((err == EINVAL || err == EILSEQ) &&
						    osize == 4) {
							*to = '_';
							u += 1;
							to++;
						}
					} else {
						u += 4 - osize;
						to = &namebuf[u];
					}
				}
				j = u;
			} else
#endif
			for (j = 0; j < jlen; j++) {
				UInt16_t	unichar;

				unichar = (buf[offset + 8 + j*2] & 0xFF) * 256 +
					    (buf[offset + 8 + j*2+1] & 0xFF);

				if (unls)
					uc = sic_uni2c(unls, unichar);	/* Get the backconverted char */
				else
					uc = unichar > 255 ? '_' : unichar;

				namebuf[j] = uc ? uc : '_';
			}
			namebuf[j] = '\0';
			printf("%4d: %4d %x %s\n",
				idx, pindex, extent, namebuf);
			break;
		case 0:
			printf("%4d: %4d %x %.*s\n",
				idx, pindex, extent, len, buf + offset + 8);
		}

		idx++;
		offset += 8 + len;
		if (offset & 1)
			offset++;
	}

	free(buf);
}

LOCAL int
parse_rr(pnt, len, cont_flag)
	unsigned char	*pnt;
	int		len;
	int		cont_flag;
{
	int slen;
	int xlen;
	int ncount;
	int pl_extent;
	int cont_extent, cont_offset, cont_size;
	int flag1, flag2;
	unsigned char *pnts;
	char symlinkname[1024];
	int goof = 0;

	symlinkname[0] = 0;

	cl_extent = cont_extent = cont_offset = cont_size = 0;

	ncount = 0;
	flag1 = -1;
	flag2 = 0;
	while (len >= 4) {
		if (pnt[3] != 1 && pnt[3] != 2) {
			printf(_("**BAD RRVERSION (%d) in '%2.2s' field %2.2X %2.2X.\n"), pnt[3], pnt, pnt[0], pnt[1]);
			return (0);		/* JS ??? Is this right ??? */
		}
		if (pnt[2] < 4) {
			printf(_("**BAD RRLEN (%d) in '%2.2s' field %2.2X %2.2X.\n"), pnt[2], pnt, pnt[0], pnt[1]);
			return (0);		/* JS ??? Is this right ??? */
		}

		ncount++;
		if (pnt[0] == 'R' && pnt[1] == 'R') flag1 = pnt[4] & 0xff;
		if (strncmp((char *)pnt, "PX", 2) == 0) flag2 |= RR_FLAG_PX;	/* POSIX attributes */
		if (strncmp((char *)pnt, "PN", 2) == 0) flag2 |= RR_FLAG_PN;	/* POSIX device number */
		if (strncmp((char *)pnt, "SL", 2) == 0) flag2 |= RR_FLAG_SL;	/* Symlink */
		if (strncmp((char *)pnt, "NM", 2) == 0) flag2 |= RR_FLAG_NM;	/* Alternate Name */
		if (strncmp((char *)pnt, "CL", 2) == 0) flag2 |= RR_FLAG_CL;	/* Child link */
		if (strncmp((char *)pnt, "PL", 2) == 0) flag2 |= RR_FLAG_PL;	/* Parent link */
		if (strncmp((char *)pnt, "RE", 2) == 0) flag2 |= RR_FLAG_RE;	/* Relocated Direcotry */
		if (strncmp((char *)pnt, "TF", 2) == 0) {
			BOOL	longfmt;
			int	size = 7;
			int	hsec;
			unsigned char *p = &pnt[5];

			flag2 |= RR_FLAG_TF;					/* Time stamp */
			longfmt = (pnt[4] & 0x80) != 0;
			if (longfmt)
				size = 17;
			if (pnt[4] & 0x01) {
				p += size;
			}
			if (pnt[4] & 0x02) {
				fstat_buf.st_mtime = iso9660_time(p,
							&hsec, longfmt);
				hsec *= 10000000;
				stat_set_mnsecs(&fstat_buf, hsec);
				p += size;
			}
			if (pnt[4] & 0x04) {
				fstat_buf.st_atime = iso9660_time(p,
							&hsec, longfmt);
				hsec *= 10000000;
				stat_set_ansecs(&fstat_buf, hsec);
				p += size;
			}
			if (pnt[4] & 0x08) {
				fstat_buf.st_ctime = iso9660_time(p,
							&hsec, longfmt);
				hsec *= 10000000;
				stat_set_cnsecs(&fstat_buf, hsec);
				p += size;
			}
		}
		if (strncmp((char *)pnt, "SF", 2) == 0) flag2 |= RR_FLAG_SF;	/* Sparse File */

		if (strncmp((char *)pnt, "SP", 2) == 0) {
			flag2 |= RR_FLAG_SP;					/* SUSP record */
			su_version = pnt[3] & 0xff;
		}
		if (strncmp((char *)pnt, "AA", 2) == 0) {			/* Neither SUSP nor RR */
			flag2 |= RR_FLAG_AA;					/* Apple Signature record */
			aa_version = pnt[3] & 0xff;
		}
		if (strncmp((char *)pnt, "ER", 2) == 0) {
			flag2 |= RR_FLAG_ER;					/* ER record */
			rr_version = pnt[7] & 0xff;				/* Ext Version */
			strlcpy(er_id, (char *)&pnt[8], (pnt[4] & 0xFF) + 1);
		}

		if (strncmp((char *)pnt, "PX", 2) == 0) {			/* POSIX attributes */
			fstat_buf.st_mode = isonum_733(pnt+4);
			fstat_buf.st_nlink = isonum_733(pnt+12);
			fstat_buf.st_uid = isonum_733(pnt+20);
			fstat_buf.st_gid = isonum_733(pnt+28);
			if ((pnt[2] & 0xFF) >= 44)				/* Check for RR 1.12 */
				fstat_buf.st_ino = (UInt32_t)isonum_733(pnt+36);
		}

		if (strncmp((char *)pnt, "PN", 2) == 0) {			/* POSIX device number */
			dev_t	devmajor = isonum_733(pnt+4);
			dev_t	devminor = isonum_733(pnt+12);

			fstat_buf.st_rdev = makedev(devmajor, devminor);
		}

		if (strncmp((char *)pnt, "NM", 2) == 0) {		/* Alternate Name */
			int	l = strlen(name_buf);

			if (!found_rr)
				l = 0;
			strncpy(&name_buf[l], (char *)(pnt+5), pnt[2] - 5);
			name_buf[l + pnt[2] - 5] = 0;
			found_rr = 1;
		}

		if (strncmp((char *)pnt, "CE", 2) == 0) {		/* Continuation Area */
			cont_extent = isonum_733(pnt+4);
			cont_offset = isonum_733(pnt+12);
			cont_size = isonum_733(pnt+20);
		}

		if (strncmp((char *)pnt, "ST", 2) == 0) {		/* Terminate SUSP */
			break;
		}

		if (strncmp((char *)pnt, "CL", 2) == 0) {
			cl_extent = isonum_733(pnt+4);			/* Child link location */
		}
		if (strncmp((char *)pnt, "PL", 2) == 0) {
			pl_extent = isonum_733(pnt+4);			/* Parent link location */
		}

		if (strncmp((char *)pnt, "SL", 2) == 0) {		/* Symlink */
			int	cflag;

			cflag = pnt[4];
			pnts = pnt+5;
			slen = pnt[2] - 5;
			while (slen >= 1) {
				switch (pnts[0] & 0xfe) {
				case 0:
					strncat(symlinkname, (char *)(pnts+2), pnts[1]);
					symlinkname[pnts[1]] = 0;
					break;
				case 2:
					strcat(symlinkname, ".");
					break;
				case 4:
					strcat(symlinkname, "..");
					break;
				case 8:
					symlinkname[0] = '\0';
					break;
				case 16:
					strcat(symlinkname, "/mnt");
					printf(_("Warning - mount point requested"));
					break;
				case 32:
					strcat(symlinkname, "kafka");
					printf(_("Warning - host_name requested"));
					break;
				default:
					printf(_("Reserved bit setting in symlink"));
					goof++;
					break;
				}
				if ((pnts[0] & 0xfe) && pnts[1] != 0) {
					printf(_("Incorrect length in symlink component"));
				}
				if (pnts[0] == 8)
					xname[0] = '\0';
				if (xname[0] == 0)
					strcpy(xname, "-> ");
				strcat(xname, symlinkname);
				symlinkname[0] = 0;
				xlen = strlen(xname);
				if ((pnts[0] & 1) == 0)
					strcat(xname, "/");

				slen -= (pnts[1] + 2);
				pnts += (pnts[1] + 2);
			}
			symlinkname[0] = 0;
		}

		len -= pnt[2];
		pnt += pnt[2];
	}
	if (cont_extent) {
		unsigned char	sector[2048];

#ifdef	USE_SCG
		readsecs(cont_extent - sector_offset, sector, ISO_BLOCKS(sizeof (sector)));
#else
		lseek(fileno(infile), ((off_t)(cont_extent - sector_offset)) << 11, SEEK_SET);
		read(fileno(infile), sector, sizeof (sector));
#endif
		flag2 |= parse_rr(&sector[cont_offset], cont_size, 1);
	}
	/*
	 * for symbolic links, strip out the last '/'
	 */
	if (xname[0] != 0 && xname[strlen(xname)-1] == '/') {
		if (strlen(xname) > 4)
			xname[strlen(xname)-1] = '\0';
	}
	return (flag2);
}

LOCAL void
find_rr(idr, pntp, lenp)
	struct iso_directory_record *idr;
	Uchar	**pntp;
	int	*lenp;
{
	struct iso_xa_dir_record *xadp;
	int len;
	unsigned char * pnt;

	len = idr->length[0] & 0xff;
	len -= offsetof(struct iso_directory_record, name[0]);
	len -= idr->name_len[0];

	pnt = (unsigned char *) idr;
	pnt += offsetof(struct iso_directory_record, name[0]);
	pnt += idr->name_len[0];
	if ((idr->name_len[0] & 1) == 0) {
		pnt++;
		len--;
	}
	if (len >= 14) {
		xadp = (struct iso_xa_dir_record *)pnt;

		if (xadp->signature[0] == 'X' && xadp->signature[1] == 'A' &&
		    xadp->reserved[0] == '\0') {
			len -= 14;
			pnt += 14;
		}
	}
	*pntp = pnt;
	*lenp = len;
}

LOCAL int
dump_rr(idr)
	struct iso_directory_record *idr;
{
	int len;
	unsigned char * pnt;

	find_rr(idr, &pnt, &len);
	return (parse_rr(pnt, len, 0));
}

LOCAL char		*months[12] = {"Jan", "Feb", "Mar", "Apr",
				"May", "Jun", "Jul",
				"Aug", "Sep", "Oct", "Nov", "Dec"};

/*
 * Return TRUE if this file was "selected" either based on find rules
 * or for all files if there was no find rule.
 */
LOCAL BOOL
dump_stat(rootname, idr, fname, extent)
	char	*rootname;
	struct iso_directory_record *idr;
	char	*fname;
	int	extent;
{
	int	i;
	int	off = 0;
	char	outline[100];

	if (do_find) {
		if (!find_stat(rootname, idr, fname, extent))
			return (FALSE);
		if (!do_listing || find_print)
			return (TRUE);
	} else if (!do_listing)
		return (TRUE);

	memset(outline, ' ', sizeof (outline));

	if (fstat_buf.st_ino != 0)
		off = js_sprintf(outline, "%10llu ", (ULlong)fstat_buf.st_ino);
	if (off < 0)
		off = 0;

	if (S_ISREG(fstat_buf.st_mode))
		outline[off] = '-';
	else if (S_ISDIR(fstat_buf.st_mode))
		outline[off] = 'd';
	else if (S_ISLNK(fstat_buf.st_mode))
		outline[off] = 'l';
	else if (S_ISCHR(fstat_buf.st_mode))
		outline[off] = 'c';
	else if (S_ISBLK(fstat_buf.st_mode))
		outline[off] = 'b';
	else if (S_ISFIFO(fstat_buf.st_mode))
		outline[off] = 'f';
	else if (S_ISSOCK(fstat_buf.st_mode))
		outline[off] = 's';
	else
		outline[off] = '?';

	memset(outline+off+1, '-', 9);
	if (fstat_buf.st_mode & S_IRUSR)
		outline[off+1] = 'r';
	if (fstat_buf.st_mode & S_IWUSR)
		outline[off+2] = 'w';
	if (fstat_buf.st_mode & S_IXUSR)
		outline[off+3] = 'x';

	if (fstat_buf.st_mode & S_IRGRP)
		outline[off+4] = 'r';
	if (fstat_buf.st_mode & S_IWGRP)
		outline[off+5] = 'w';
	if (fstat_buf.st_mode & S_IXGRP)
		outline[off+6] = 'x';

	if (fstat_buf.st_mode & S_IROTH)
		outline[off+7] = 'r';
	if (fstat_buf.st_mode & S_IWOTH)
		outline[off+8] = 'w';
	if (fstat_buf.st_mode & S_IXOTH)
		outline[off+9] = 'x';

	off += 10;
	off += js_sprintf(outline+off, " %3ld", (long)fstat_buf.st_nlink);
	off += js_sprintf(outline+off, " %4ld", (unsigned long)fstat_buf.st_uid);
	off += js_sprintf(outline+off, " %4ld", (unsigned long)fstat_buf.st_gid);

	if (do_sectors == 0) {
		off += js_sprintf(outline+off, " %10lld", (Llong)fstat_buf.st_size);
	} else {
		off += js_sprintf(outline+off, " %10lld", (Llong)((fstat_buf.st_size+PAGE-1)/PAGE));
	}

	if (date_buf[1] >= 1 && date_buf[1] <= 12) {
		memcpy(outline+off+1, months[date_buf[1]-1], 3);
		off += 4;
	} else {
		/*
		 * Some broken ISO-9660 formatters write illegal month numbers.
		 */
		off += 1 + js_sprintf(outline+off+1, "???");
	}

	off += js_sprintf(outline+off, " %2d", date_buf[2]);
	off += js_sprintf(outline+off, " %4d", date_buf[0]+1900);

	off += js_sprintf(outline+off, " [%7d", extent);	/* XXX up to 20 GB */
	off += js_sprintf(outline+off, " %02X]", idr->flags[0] & 0xFF);

	for (i = 0; i < off; i++) {
		if (outline[i] == 0) outline[i] = ' ';
	}
	outline[off] = 0;
	printf("%s %s %s\n", outline, name_buf, xname);
	return (TRUE);
}

LOCAL void
extract(rootname, idr, fname)
	char				*rootname;
	struct iso_directory_record	*idr;
	char				*fname;
{
	int		f;
	struct timespec	times[2];
static	BOOL		isfirst = TRUE;

	if ((idr->flags[0] & 2) != 0 &&
	    (idr->name_len[0] == 1 &&
	    (idr->name[0] == 0 || idr->name[0] == 1))) {
		/*
		 * Catch all "." and ".." entries here.
		 */
		if (idr->name[0] == 1)	/* Skip "/.." */
			return;
		if (rootname[0] == '/' && rootname[1] == '\0')
			fname = "/.";
	}
	if (*fname == '/')
		fname++;
	makedirs(fname, S_IRUSR|S_IWUSR|S_IXUSR|
			S_IRGRP|S_IWGRP|S_IXGRP|
			S_IROTH|S_IWOTH|S_IXOTH, TRUE);

	switch (fstat_buf.st_mode & S_IFMT) {

#ifndef	HAVE_MKNOD
	err:
#endif
	default:
			errmsgno(EX_BAD,
				"Unsupported file type %0lo for '%s'.\n",
				(unsigned long)fstat_buf.st_mode & S_IFMT,
				fname);
			return;


	case S_IFDIR:
#ifdef	__MINGW32__
#define	mkdir(n, m)	mkdir(n)
#endif
			if (mkdir(fname, fstat_buf.st_mode) < 0 &&
			    geterrno() != EEXIST)
				errmsg("Cannot make directory '%s'.\n", fname);
			goto setmode;

#ifdef	S_IFBLK
	case S_IFBLK:
#ifdef	HAVE_MKNOD
			if (mknod(fname, fstat_buf.st_mode, fstat_buf.st_rdev) < 0)
				errmsg("Cannot make block device '%s'.\n", fname);
			goto setmode;
#else
			goto err;
#endif
#endif
#ifdef	S_IFCHR
	case S_IFCHR:
#ifdef	HAVE_MKNOD
			if (mknod(fname, fstat_buf.st_mode, fstat_buf.st_rdev) < 0)
				errmsg("Cannot make character device '%s'.\n", fname);
			goto setmode;
#else
			goto err;
#endif
#endif
#ifdef	S_IFIFO
	case S_IFIFO:
#ifdef	HAVE_MKFIFO
			if (mkfifo(fname, fstat_buf.st_mode) < 0)
				errmsg("Cannot make fifo '%s'.\n", fname);
			goto setmode;
#else
			goto err;
#endif
#endif
#ifdef	S_IFSOCK
	case S_IFSOCK:
#ifdef	HAVE_MKNOD
			if (mknod(fname, fstat_buf.st_mode, 0) < 0)
				errmsg("Cannot make socket '%s'.\n", fname);
			goto setmode;
#else
			goto err;
#endif
#endif
#ifdef	S_IFLNK
	case S_IFLNK:
			if (symlink(&xname[3], fname) < 0)
				errmsg("Cannot make symlink '%s'.\n", fname);
			goto setmode;
#endif

	case S_IFREG:	break;
	}

	makedirs(fname, S_IRUSR|S_IWUSR|S_IXUSR|
			S_IRGRP|S_IWGRP|S_IXGRP|
			S_IROTH|S_IWOTH|S_IXOTH, TRUE);
	if (isfirst)
		f = open(fname, O_WRONLY|O_CREAT|O_TRUNC, 0600);
	else
		f = open(fname, O_WRONLY|O_CREAT, 0600);
	if (f < 0) {
		errmsg("Cannot create '%s'.\n", fname);
		return;
	}
	lseek(f, 0, SEEK_END);
	extract_file(f, idr, fname);
	if ((idr->flags[0] & ISO_MULTIEXTENT) == 0) {
#ifdef	HAVE_FCHOWN
		fchown(f, fstat_buf.st_uid, fstat_buf.st_gid);
#else
		fchownat(AT_FDCWD, fname, fstat_buf.st_uid, fstat_buf.st_gid, 0);
#endif
#ifdef	HAVE_FCHMOD
		fchmod(f, fstat_buf.st_mode);
#else
		fchmodat(AT_FDCWD, fname, fstat_buf.st_mode, 0);
#endif
		times[0].tv_sec = fstat_buf.st_atime;
		times[0].tv_nsec = stat_ansecs(&fstat_buf);
		times[1].tv_sec = fstat_buf.st_mtime;
		times[1].tv_nsec = stat_mnsecs(&fstat_buf);
#if defined(HAVE_FUTIMESAT) || defined(HAVE_FUTIMES) || defined(HAVE_FUTIMENS)
		futimens(f, times);
#else
		utimensat(AT_FDCWD, fname, times, 0);
#endif
		isfirst = TRUE;		/* Next call is for a new file */
	} else {
		isfirst = FALSE;	/* Next call is a continuation */
	}
	close(f);
	return;
setmode:
	fchownat(AT_FDCWD, fname, fstat_buf.st_uid, fstat_buf.st_gid, AT_SYMLINK_NOFOLLOW);
	fchmodat(AT_FDCWD, fname, fstat_buf.st_mode, AT_SYMLINK_NOFOLLOW);
	times[0].tv_sec = fstat_buf.st_atime;
	times[0].tv_nsec = stat_ansecs(&fstat_buf);
	times[1].tv_sec = fstat_buf.st_mtime;
	times[1].tv_nsec = stat_mnsecs(&fstat_buf);
	utimensat(AT_FDCWD, fname, times, AT_SYMLINK_NOFOLLOW);
}

LOCAL void
extract_file(f, idr, fname)
	int				f;
	struct iso_directory_record	*idr;
	char				*fname;
{
	int		extent, len, tlen;
	unsigned char	buff[20480];

	if (f == STDOUT_FILENO)
		setmode(fileno(stdout), O_BINARY);

	extent = isonum_733((unsigned char *)idr->extent);
	len = isonum_733((unsigned char *)idr->size);

	while (len > 0) {
		tlen = (len > sizeof (buff) ? sizeof (buff) : len);
#ifdef	USE_SCG
		readsecs(extent - sector_offset, buff, ISO_BLOCKS(tlen));
#else
		lseek(fileno(infile), ((off_t)(extent - sector_offset)) << 11, SEEK_SET);
		read(fileno(infile), buff, tlen);
#endif
		len -= tlen;
		extent += ISO_BLOCKS(tlen);
		if (write(f, buff, tlen) != tlen)
			errmsg("Write error on '%s'.\n", fname);
	}
}


LOCAL void
parse_cl_dir(idr, extent)
	struct iso_directory_record	*idr;
	int				extent;
{
	char				cl_name_buf[256*3];

	strlcpy(cl_name_buf, name_buf, sizeof (cl_name_buf));
#ifdef	USE_SCG
	readsecs(extent - sector_offset, idr, 1);
#else
	lseek(fileno(infile), ((off_t)(extent - sector_offset)) << 11, SEEK_SET);
	read(fileno(infile), idr, 2048);
#endif

	if (parse_de(idr) && use_rock)
		dump_rr(idr);
	strlcpy(name_buf, cl_name_buf, sizeof (name_buf));
}

LOCAL BOOL
parse_de(idr)
	struct iso_directory_record	*idr;
{
	unsigned char	uc;

	if (idr->length[0] == 0)
		return (FALSE);
	memset(&fstat_buf, 0, sizeof (fstat_buf));
	found_rr = 0;
	name_buf[0] = xname[0] = 0;
	fstat_buf.st_size = (off_t)(unsigned)isonum_733((unsigned char *)idr->size);
	if (idr->flags[0] & 2)
		fstat_buf.st_mode |= S_IFDIR;
	else
		fstat_buf.st_mode |= S_IFREG;
	if (idr->name_len[0] == 1 && idr->name[0] == 0)
		strcpy(name_buf, ".");
	else if (idr->name_len[0] == 1 && idr->name[0] == 1)
		strcpy(name_buf, "..");
	else {
		switch (ucs_level) {
		case 3:
		case 2:
		case 1:
			/*
			 * Unicode name.  Convert as best we can.
			 */
			{
			int	j;
				name_buf[0] = '\0';
#ifdef	USE_ICONV
			if (use_iconv(unls)) {
				int	u;
				char	*to = name_buf;

				for (j = 0, u = 0; j < (int)idr->name_len[0] / 2; j++) {
					char	*ibuf = (char *)&idr->name[j*2];
					size_t	isize = 2;		/* UCS-2 character size */
					size_t	osize = 4;

					if (iconv(unls->sic_uni2cd, (__IC_CONST char **)&ibuf, &isize,
							(char **)&to, &osize) == -1) {
						int	err = geterrno();

						if ((err == EINVAL || err == EILSEQ) &&
						    osize == 4) {
							*to = '_';
							u += 1;
							to++;
						}
					} else {
						u += 4 - osize;
						to = &name_buf[u];
					}
				}
				j = u;
			} else
#endif
			for (j = 0; j < (int)idr->name_len[0] / 2; j++) {
				UInt16_t	unichar;

				unichar = (idr->name[j*2] & 0xFF) * 256 +
					    (idr->name[j*2+1] & 0xFF);

				/*
				 * Get the backconverted char
				 */
				if (unls)
					uc = sic_uni2c(unls, unichar);
				else
					uc = unichar > 255 ? '_' : unichar;

				name_buf[j] = uc ? uc : '_';
			}
			name_buf[j] = '\0';
			}
			break;
		case 0:
			/*
			 * Normal non-Unicode name.
			 */
			strncpy(name_buf, idr->name, idr->name_len[0]);
			name_buf[idr->name_len[0]] = 0;
			break;
		default:
			/*
			 * Don't know how to do these yet.  Maybe they are the same
			 * as one of the above.
			 */
			exit(1);
		}
	}
	memcpy(date_buf, idr->date, sizeof (idr->date));
	/*
	 * Always first set up time stamps and file modes from
	 * ISO-9660. This is used as a fallback in case that
	 * there is no related Rock Ridge based data.
	 */
	fstat_buf.st_atime =
	fstat_buf.st_mtime =
	fstat_buf.st_ctime = iso9660_time(date_buf, NULL, FALSE);
	fstat_buf.st_mode |= S_IRUSR|S_IXUSR |
		    S_IRGRP|S_IXGRP |
		    S_IROTH|S_IXOTH;
	fstat_buf.st_nlink = 1;
	fstat_buf.st_ino = 0;
	fstat_buf.st_uid = 0;
	fstat_buf.st_gid = 0;
	if (iso9660_inodes) {
		fstat_buf.st_ino = (unsigned long)
		    isonum_733((unsigned char *)idr->extent);
	}
	return (TRUE);
}

LOCAL void
parse_dir(dp, rootname, extent, len)
	struct todo	*dp;
	char	*rootname;
	int	extent;			/* Directory extent */
	int	len;			/* Directory size   */
{
	struct todo 	*td;
	int		i;
	struct iso_directory_record * idr;
	struct iso_directory_record	didr;
	struct stat			dstat;
	unsigned char	cl_buffer[2048];
	unsigned char	flags = 0;
	Llong		size = 0;
	int		sextent = 0;
	int	rlen;
	int	blen;
	int	rr_flags = 0;
static	char	*n = 0;
static	int	nlen = 0;


	if (do_listing && (!do_find || !find_print))
		printf(_("\nDirectory listing of %s\n"), rootname);

	rlen = strlen(rootname);

	while (len > 0) {
#ifdef	USE_SCG
		readsecs(extent - sector_offset, buffer, ISO_BLOCKS(sizeof (buffer)));
#else
		lseek(fileno(infile), ((off_t)(extent - sector_offset)) << 11, SEEK_SET);
		read(fileno(infile), buffer, sizeof (buffer));
#endif
		len -= sizeof (buffer);
		extent++;
		i = 0;
		while (1 == 1) {
			idr = (struct iso_directory_record *) &buffer[i];
			if (idr->length[0] == 0)
				break;
			parse_de(idr);
			if (use_rock) {
				rr_flags = dump_rr(idr);

				if (rr_flags & RR_FLAG_CL) {
					/*
					 * Need to reparse the child link
					 * but note that we parse "CL/."
					 * so we get no usable file name.
					 */
					idr = (struct iso_directory_record *) cl_buffer;
					parse_cl_dir(idr, cl_extent);
				} else if (rr_flags & RR_FLAG_RE)
					goto cont;	/* skip rr_moved */
			}
			if (Xtract &&
			    (idr->flags[0] & 2) != 0 &&
			    idr->name_len[0] == 1 &&
			    idr->name[0] == 0) {
				/*
				 * The '.' entry.
				 */
				didr = *idr;
				dstat = fstat_buf;
			}
			blen = strlen(name_buf);

			blen = rlen + blen + 1;
			if (nlen < blen) {
				n = ___realloc(n, blen, _("find_stat name"));
				nlen = blen;
			}
			strcatl(n, rootname, name_buf, (char *)0);
			if (name_buf[0] == '.' && name_buf[1] == '\0')
				n[rlen] = '\0';

			if ((idr->flags[0] & 2) != 0 &&
			    ((rr_flags & RR_FLAG_CL) ||
			    (idr->name_len[0] != 1 ||
			    (idr->name[0] != 0 && idr->name[0] != 1)))) {
				/*
				 * This is a plain directory (neither "xxx/."
				 * nor "xxx/..").
				 * Add this directory to the todo list.
				 */
				int		dir_loop = 0;
				int		nextent;
				struct todo 	*tp = dp;

				nextent = isonum_733((unsigned char *)idr->extent);
				while (tp) {
					if (tp->extent == nextent) {
						dir_loop = 1;
						break;
					}
					tp = tp->prev;
				}
				if (dir_loop == 0) {
					td = (struct todo *) malloc(sizeof (*td));
					if (td == NULL)
						comerr(_("No memory.\n"));
					td->next = NULL;
					td->prev = dp;
					td->extent = isonum_733((unsigned char *)idr->extent);
					td->length = isonum_733((unsigned char *)idr->size);
					td->name = (char *) malloc(strlen(rootname)
								+ strlen(name_buf) + 2);
					if (td->name == NULL)
						comerr(_("No memory.\n"));
					strcpy(td->name, rootname);
					strcat(td->name, name_buf);
					strcat(td->name, "/");

					*todo_pp = td;
					todo_pp = &td->next;
				}
			} else {
				if (xtract && strcmp(xtract, n) == 0) {
					extract_file(STDOUT_FILENO, idr, "stdout");
				}
			}
			if (do_f &&
			    (idr->name_len[0] != 1 ||
			    (idr->name[0] != 0 && idr->name[0] != 1))) {
				printf("%s\n", n);
			}
			if (do_listing || Xtract || do_find) {
				/*
				 * In case if a multi-extent file, remember the
				 * start extent number.
				 */
				if ((idr->flags[0] & ISO_MULTIEXTENT) && size == 0)
					sextent = isonum_733((unsigned char *)idr->extent);

				if (debug ||
				    ((idr->flags[0] & ISO_MULTIEXTENT) == 0 && size == 0)) {
					if (dump_stat(rootname, idr, n,
							isonum_733((unsigned char *)idr->extent))) {
						if (Xtract) {
							if ((idr->flags[0] & 2) != 0 &&
							    idr->name_len[0] != 1 &&
							    idr->name[0] != 1) {
								char *p = n;
								if (*p == '/')
									p++;
								makedirs(p,
									S_IRUSR|S_IWUSR|S_IXUSR|
									S_IRGRP|S_IWGRP|S_IXGRP|
									S_IROTH|S_IWOTH|S_IXOTH,
									FALSE);
							} else {
								if (myuid != 0 &&
								    S_ISDIR(fstat_buf.st_mode)) {
									fstat_buf.st_mode |= S_IWUSR;
								}
								extract(rootname, idr, n);
							}
						}
					}
				} else if (Xtract && find_stat(rootname, idr, n, sextent)) {
					/*
					 * Extract all multi extent files here...
					 */
					extract(rootname, idr, n);
				}
				size += fstat_buf.st_size;
				if ((flags & ISO_MULTIEXTENT) &&
				    (idr->flags[0] & ISO_MULTIEXTENT) == 0) {
					fstat_buf.st_size = size;
					if (!debug)
						idr->flags[0] |= ISO_MULTIEXTENT;
					dump_stat(rootname, idr, n, sextent);
					if (!debug)
						idr->flags[0] &= ~ISO_MULTIEXTENT;
				}
				flags = idr->flags[0];
				if ((idr->flags[0] & ISO_MULTIEXTENT) == 0)
					size = 0;
			}
		cont:
			i += buffer[i];
			if (i > 2048 - offsetof(struct iso_directory_record, name[0])) break;
		}
	}
	if (Xtract) {
		char *nm = strrchr(rootname, '/');

		if (nm != rootname)
			nm++;
		if (find_stat(rootname, &didr, nm, 0)) {
			fstat_buf = dstat;
			extract(rootname, &didr, rootname);
		}
	}
}

LOCAL void
usage(excode)
	int	excode;
{
#ifdef	USE_FIND
	errmsgno(EX_BAD, _("Usage: %s [options] -i filename [-find [[find expr.]]\n"), get_progname());
#else
	errmsgno(EX_BAD, _("Usage: %s [options] -i filename\n"), get_progname());
#endif

	error(_("Options:\n"));
	error(_("\t-help,-h	Print this help\n"));
	error(_("\t-version	Print version info and exit\n"));
	error(_("\t-debug		Print additional debug info\n"));
	error(_("\t-ignore-error Ignore errors\n"));
	error(_("\t-d		Print information from the primary volume descriptor\n"));
	error(_("\t-f		Generate output similar to 'find .  -print'\n"));
#ifdef	USE_FIND
	error(_("\t-find [find expr.] Option separator: Use find command line to the right\n"));
#endif
	error(_("\t-J		Print information from Joliet extensions\n"));
	error(_("\t-j charset	Use charset to display Joliet file names\n"));
	error(_("\t-l		Generate output similar to 'ls -lR'\n"));
	error(_("\t-p		Print Path Table\n"));
	error(_("\t-R		Print information from Rock Ridge extensions\n"));
	error(_("\t-s		Print file size infos in multiples of sector size (%ld bytes).\n"), (long)PAGE);
	error(_("\t-N sector	Sector number where ISO image should start on CD\n"));
	error(_("\t-T sector	Sector number where actual session starts on CD\n"));
	error(_("\t-i filename	Filename to read ISO-9660 image from\n"));
	error(_("\tdev=target	SCSI target to use as CD/DVD-Recorder\n"));
	error(_("\t-X		Extract all matching files to the filesystem\n"));
	error(_("\t-x pathname	Extract specified file to stdout\n"));
	exit(excode);
}

EXPORT int
main(argc, argv)
	int	argc;
	char	*argv[];
{
	int	cac;
	char	* const *cav;
	int	c;
	int	ret = 0;
	char	*filename = NULL;
	char	*sdevname = NULL;
#if	defined(USE_NLS)
	char	*dir;
#endif
	/*
	 * Use toc_offset != 0 (-T #) if we have a complete multi-session
	 * disc that we want/need to play with.
	 * Here we specify the offset where we want to
	 * start searching for the TOC.
	 */
	int	toc_offset = 0;
	int	extent;
	struct todo * td;
	struct iso_primary_descriptor ipd;
	struct iso_primary_descriptor jpd;
	struct eltorito_boot_descriptor bpd;
	struct iso_directory_record * idr;
	char	*charset = NULL;
	char	*opts = "help,h,version,debug,ignore-error,d,p,i*,dev*,J,R,l,x*,X,find~,f,s,N#l,T#l,j*";
	BOOL	help = FALSE;
	BOOL	prvers = FALSE;
	BOOL	found_eltorito = FALSE;
	int	bootcat_offset = 0;
	int	voldesc_sum = 0;
	char	*cp;


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
	if (getallargs(&cac, &cav, opts,
				&help, &help, &prvers, &debug, &ignerr,
				&do_pvd, &do_pathtab,
				&filename, &sdevname,
				&use_joliet, &use_rock,
				&do_listing,
				&xtract, &Xtract,
				getfind, NULL,
				&do_f, &do_sectors,
				&sector_offset, &toc_offset,
				&charset) < 0) {
		errmsgno(EX_BAD, _("Bad Option: '%s'\n"), cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (prvers) {
		printf(_("isoinfo %s (%s-%s-%s) Copyright (C) 1993-1999 %s (C) 1999-2016 %s\n"),
					VERSION,
					HOST_CPU, HOST_VENDOR, HOST_OS,
					_("Eric Youngdale"),
					_("Joerg Schilling"));
		exit(0);
	}
	myuid = getuid();
#ifdef	USE_FIND
	if (do_find) {
		finda_t	fa;

		cac = find_ac;
		cav = find_av;
		find_firstprim(&cac, &cav);
		find_pac = cac;
		find_pav = cav;

		if (cac > 0) {
			find_argsinit(&fa);
			fa.walkflags = walkflags;
			fa.Argc = cac;
			fa.Argv = (char **)cav;
			find_node = find_parse(&fa);
			if (fa.primtype == FIND_ERRARG)
				comexit(fa.error);
			if (fa.primtype != FIND_ENDARGS)
				comerrno(EX_BAD, _("Incomplete expression.\n"));
			plusp = fa.plusp;
			find_patlen = fa.patlen;
			walkflags = fa.walkflags;
			maxdepth = fa.maxdepth;
			mindepth = fa.mindepth;

			if (find_node && xtract) {
				if (find_pname(find_node, "-exec") ||
				    find_pname(find_node, "-exec+") ||
				    find_pname(find_node, "-ok"))
					comerrno(EX_BAD,
					"Cannot -exec with '-o -'.\n");
			}
			if (find_node && find_hasprint(find_node))
				find_print = TRUE;
		}
		if (find_ac > find_pac) {
			errmsgno(EX_BAD, _("Unsupported pathspec for -find.\n"));
			usage(EX_BAD);
		}
#ifdef	__not_yet__
		if (find_ac <= 0 || find_ac == find_pac) {
			errmsgno(EX_BAD, _("Missing pathspec for -find.\n"));
			usage(EX_BAD);
		}
#endif

		walkinitstate(&walkstate);
		if (find_patlen > 0) {
			walkstate.patstate = ___malloc(sizeof (int) * find_patlen,
						_("space for pattern state"));
		}

		find_timeinit(time(0));
		walkstate.walkflags	= walkflags;
		walkstate.maxdepth	= maxdepth;
		walkstate.mindepth	= mindepth;
		walkstate.lname		= NULL;
		walkstate.tree		= find_node;
		walkstate.err		= 0;
		walkstate.pflags	= 0;
	}
#endif
	cac = argc - 1;
	cav = argv + 1;
	if (!do_find && getfiles(&cac, &cav, opts) != 0) {
		errmsgno(EX_BAD, _("Bad Argument: '%s'\n"), cav[0]);
		usage(EX_BAD);
	}

#if	defined(USE_NLS) && defined(HAVE_NL_LANGINFO) && defined(CODESET)
	/*
	 * If the locale has not been set up, nl_langinfo() returns the
	 * name of the default codeset. This should be either "646",
	 * "ISO-646", "ASCII", or something similar. Unfortunately, the
	 * POSIX standard does not include a list of valid locale names,
	 * so ne need to find all values in use.
	 *
	 * Observed:
	 * Solaris 	"646"
	 * Linux	"ANSI_X3.4-1968"	strange value from Linux...
	 */
	if (charset == NULL) {
		char	*codeset = nl_langinfo(CODESET);
		Uchar	*p;

		if (codeset != NULL)
			codeset = strdup(codeset);
		if (codeset == NULL)			/* Should not happen */
			goto setcharset;
		if (*codeset == '\0')			/* Invalid locale    */
			goto setcharset;

		for (p = (Uchar *)codeset; *p != '\0'; p++) {
			if (islower(*p))
				*p = toupper(*p);
		}
		p = (Uchar *)strstr(codeset, "ISO");
		if (p != NULL) {
			if (*p == '_' || *p == '-')
				p++;
			codeset = (char *)p;
		}
		if (strcmp("646", codeset) != 0 &&
		    strcmp("ASCII", codeset) != 0 &&
		    strcmp("US-ASCII", codeset) != 0 &&
		    strcmp("US_ASCII", codeset) != 0 &&
		    strcmp("USASCII", codeset) != 0 &&
		    strcmp("ANSI_X3.4-1968", codeset) != 0)
			charset = nl_langinfo(CODESET);

		if (codeset != NULL)
			free(codeset);

#define	verbose	1
		if (verbose > 0 && charset != NULL) {
			error(_("Setting input-charset to '%s' from locale.\n"),
				charset);
		}
	}
setcharset:
	/*
	 * Allow to switch off locale with -input-charset "".
	 */
	if (charset != NULL && *charset == '\0')
		charset = NULL;
#endif

	if (charset == NULL) {
#if	(defined(__CYGWIN32__) || defined(__CYGWIN__) || defined(__DJGPP__) || defined(__MINGW32__)) && !defined(IS_CYGWIN_1)
		unls = sic_open("cp437");
#else
		unls = sic_open("default");
#endif
	} else {
		unls = sic_open(charset);
	}
	if (unls == NULL) {	/* Unknown charset specified */
		fprintf(stderr, _("Unknown charset: %s\nKnown charsets are:\n"),
							charset);
		sic_list(stdout); /* List all known charset names */

		list_locales();
		exit(EX_BAD);
		exit(1);
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
		fprintf(stderr, _("ISO-9660 image not specified\n"));
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
		fprintf(stderr, _("Unable to open %s\n"), filename);
		exit(1);
	}

	/*
	 * Absolute sector offset, so don't subtract sector_offset here.
	 */
#ifdef	USE_SCG
	readsecs(16 + toc_offset, &ipd, ISO_BLOCKS(sizeof (ipd)));
#else
	lseek(fileno(infile), ((off_t)(16 + toc_offset)) <<11, SEEK_SET);
	read(fileno(infile), &ipd, sizeof (ipd));
#endif
	idr = (struct iso_directory_record *)ipd.root_directory_record;
	for (c = 0, cp = (char *)&ipd; c < 2048; c++)
		voldesc_sum += *cp++ & 0xFF;
	if (GET_UBYTE(ipd.type) == ISO_VD_PRIMARY) {
		c = 17;
		do {
#ifdef	USE_SCG
			readsecs(c + toc_offset, &jpd, ISO_BLOCKS(sizeof (jpd)));
#else
			lseek(fileno(infile), ((off_t)(c + toc_offset)) <<11, SEEK_SET);
			read(fileno(infile), &jpd, sizeof (jpd));
#endif
		} while (++c < 32 && GET_UBYTE(jpd.type) != ISO_VD_END);
		if (GET_UBYTE(jpd.type) == ISO_VD_END) do {
#ifdef	USE_SCG
			readsecs(c + toc_offset, &jpd, ISO_BLOCKS(sizeof (jpd)));
#else
			lseek(fileno(infile), ((off_t)(c + toc_offset)) <<11, SEEK_SET);
			read(fileno(infile), &jpd, sizeof (jpd));
#endif
			cp = (char *)&jpd;
			if (strncmp(cp, "MKI ", 4) == 0) {
				int	sum;

				sum  = cp[2045] & 0xFF;
				sum *= 256;
				sum += cp[2046] & 0xFF;
				sum *= 256;
				sum += cp[2047] & 0xFF;
				if (sum == voldesc_sum)
					iso9660_inodes = TRUE;
				break;
			}
		} while (++c < 48);
	}

	/*
	 * Read '.' entry for the root directory.
	 */
	extent = isonum_733((unsigned char *)idr->extent);
#ifdef	USE_SCG
	readsecs(extent - sector_offset, buffer, ISO_BLOCKS(sizeof (buffer)));
#else
	lseek(fileno(infile), ((off_t)(extent - sector_offset)) <<11, SEEK_SET);
	read(fileno(infile), buffer, sizeof (buffer));
#endif
	c = dump_rr((struct iso_directory_record *) buffer);
	if (c == 0 ||
	    (c & (RR_FLAG_SP | RR_FLAG_ER)) == 0 || su_version < 1 || rr_version < 1) {
		if (!debug)
			use_rock = FALSE;
	}
	if (do_pvd) {
		/*
		 * High sierra:
		 *
		 *	DESC TYPE	== 1 (VD_SFS)	offset 8	len 1
		 *	STR ID		== "CDROM"	offset 9	len 5
		 *	STD_VER		== 1		offset 14	len 1
		 */
		if ((((char *)&ipd)[8] == 1) &&
		    (strncmp(&((char *)&ipd)[9], "CDROM", 5) == 0) &&
		    (((char *)&ipd)[14] == 1)) {
			printf(_("CD-ROM is in High Sierra format\n"));
			exit(0);
		}
		/*
		 * ISO 9660:
		 *
		 *	DESC TYPE	== 1 (VD_PVD)	offset 0	len 1
		 *	STR ID		== "CD001"	offset 1	len 5
		 *	STD_VER		== 1		offset 6	len 1
		 */
		if ((ipd.type[0] != ISO_VD_PRIMARY) ||
		    (strncmp(ipd.id, ISO_STANDARD_ID, sizeof (ipd.id)) != 0) ||
		    (ipd.version[0] != 1)) {
			printf(_("CD-ROM is NOT in ISO 9660 format\n"));
			exit(1);
		}

		printf(_("CD-ROM is in ISO 9660 format\n"));
		if (!use_joliet)
			list_vd(&ipd, FALSE);
		{
			int	block = 16;
			movebytes(&ipd, &jpd, sizeof (ipd));
			while ((Uchar)jpd.type[0] != ISO_VD_END) {

				if (debug && (Uchar) jpd.type[0] == ISO_VD_SUPPLEMENTARY)
					error(_("Joliet escape sequence 0: '%c' 1: '%c' 2: '%c' 3: '%c'\n"),
						jpd.escape_sequences[0],
						jpd.escape_sequences[1],
						jpd.escape_sequences[2],
						jpd.escape_sequences[3]);
					/*
					 * If Joliet UCS escape sequence found, we may be wrong
					 */
					if (jpd.escape_sequences[0] == '%' &&
					    jpd.escape_sequences[1] == '/' &&
					    (jpd.escape_sequences[3] == '\0' ||
					    jpd.escape_sequences[3] == ' ') &&
					    (jpd.escape_sequences[2] == '@' ||
					    jpd.escape_sequences[2] == 'C' ||
					    jpd.escape_sequences[2] == 'E')) {

						if (jpd.version[0] == 1)
							goto nextblock;
				}
				if (jpd.type[0] == 0) {
					movebytes(&jpd, &bpd, sizeof (bpd));
					if (strncmp(bpd.system_id, EL_TORITO_ID, sizeof (EL_TORITO_ID)) == 0) {
						bootcat_offset = (Uchar)bpd.bootcat_ptr[0] +
								(Uchar)bpd.bootcat_ptr[1] * 256 +
								(Uchar)bpd.bootcat_ptr[2] * 65536 +
								(Uchar)bpd.bootcat_ptr[3] * 16777216;
						found_eltorito = TRUE;
						printf(_("El Torito VD version %d found, boot catalog is in sector %d\n"),
							bpd.version[0],
							bootcat_offset);
					}
				}
				if (jpd.version[0] == 2) {
					printf(_("CD-ROM uses ISO 9660:1999 relaxed format\n"));
					break;
				}

			nextblock:
				block++;
#ifdef	USE_SCG
				readsecs(block + toc_offset, &jpd, ISO_BLOCKS(sizeof (jpd)));
#else
				lseek(fileno(infile), ((off_t)(block + toc_offset)) <<11, SEEK_SET);
				read(fileno(infile), &jpd, sizeof (jpd));
#endif
			}
		}
	}
	/*
	 * ISO 9660:
	 *
	 *	DESC TYPE	== 1 (VD_PVD)	offset 0	len 1
	 *	STR ID		== "CD001"	offset 1	len 5
	 *	STD_VER		== 1		offset 6	len 1
	 */
	if ((ipd.type[0] != ISO_VD_PRIMARY) ||
	    (strncmp(ipd.id, ISO_STANDARD_ID, sizeof (ipd.id)) != 0) ||
	    (ipd.version[0] != 1)) {
		printf(_("CD-ROM is NOT in ISO 9660 format\n"));
		exit(1);
	}

	if (use_joliet || do_pvd) {
		int block = 16;
		movebytes(&ipd, &jpd, sizeof (ipd));
		while ((unsigned char) jpd.type[0] != ISO_VD_END) {
			if (debug && (unsigned char) jpd.type[0] == ISO_VD_SUPPLEMENTARY)
				error(_("Joliet escape sequence 0: '%c' 1: '%c' 2: '%c' 3: '%c'\n"),
					jpd.escape_sequences[0],
					jpd.escape_sequences[1],
					jpd.escape_sequences[2],
					jpd.escape_sequences[3]);
			/*
			 * Find the UCS escape sequence.
			 */
			if (jpd.escape_sequences[0] == '%' &&
			    jpd.escape_sequences[1] == '/' &&
			    (jpd.escape_sequences[3] == '\0' ||
			    jpd.escape_sequences[3] == ' ') &&
			    (jpd.escape_sequences[2] == '@' ||
			    jpd.escape_sequences[2] == 'C' ||
			    jpd.escape_sequences[2] == 'E')) {
				break;
			}

			block++;
#ifdef	USE_SCG
			readsecs(block + toc_offset, &jpd, ISO_BLOCKS(sizeof (jpd)));
#else
			lseek(fileno(infile),
				((off_t)(block + toc_offset)) <<11, SEEK_SET);
			read(fileno(infile), &jpd, sizeof (jpd));
#endif
		}

		if (use_joliet && ((unsigned char) jpd.type[0] == ISO_VD_END)) {
			fprintf(stderr, _("Unable to find Joliet SVD\n"));
			exit(1);
		}

		switch (jpd.escape_sequences[2]) {
		case '@':
			ucs_level = 1;
			break;
		case 'C':
			ucs_level = 2;
			break;
		case 'E':
			ucs_level = 3;
			break;
		}

		if (ucs_level > 3) {
			fprintf(stderr,
				_("Don't know what ucs_level == %d means\n"),
				ucs_level);
			exit(1);
		}
		if (jpd.escape_sequences[3] == ' ')
			errmsgno(EX_BAD,
			_("Warning: Joliet escape sequence uses illegal space at offset 3\n"));
	}

	if (do_pvd) {
		if (ucs_level > 0) {
			if (use_joliet) {
				printf(_("\nJoliet with UCS level %d found,"), ucs_level);
				printf(_(" Joliet volume descriptor:\n"));
				list_vd(&jpd, TRUE);
			} else {
				printf(_("\nJoliet with UCS level %d found."), ucs_level);
			}
		} else {
			printf(_("NO Joliet present\n"));
		}

		if (c != 0) {
#ifdef	RR_DEBUG
			printf("RR %X %d\n", c, c);
#endif
			if (c & RR_FLAG_SP) {
				printf(
				_("\nSUSP signatures version %d found\n"),
				su_version);
				if (c & RR_FLAG_ER) {
					if (rr_version < 1) {
						printf(
						_("No valid Rock Ridge signature found\n"));
					} else {
						printf(
						_("Rock Ridge signatures version %d found\n"),
						rr_version);
						printf(
						_("Rock Ridge id '%s'\n"),
						er_id);
					}
				}
			} else {
				printf(
				_("\nBad Rock Ridge signatures found (SU record missing)\n"));
			}
			/*
			 * This is currently a no op!
			 * We need to check the first plain file instead of
			 * the '.' entry in the root directory.
			 */
			if (c & RR_FLAG_AA) {
				printf(_("\nApple signatures version %d found\n"),
								aa_version);
			}
		} else {
			printf(_("\nNo SUSP/Rock Ridge present\n"));
		}
		if (found_eltorito)
			printf_bootinfo(infile, bootcat_offset);
		exit(0);
	}

	if (use_joliet)
		idr = (struct iso_directory_record *)jpd.root_directory_record;

	if (do_pathtab) {
		if (use_joliet) {
			dump_pathtab(isonum_731(jpd.type_l_path_table),
			isonum_733((unsigned char *)jpd.path_table_size));
		} else {
			dump_pathtab(isonum_731(ipd.type_l_path_table),
			isonum_733((unsigned char *)ipd.path_table_size));
		}
	}

	parse_dir(todo_idr, "/", isonum_733((unsigned char *)idr->extent),
				isonum_733((unsigned char *)idr->size));
	td = todo_idr;
	while (td) {
		parse_dir(td, td->name, td->extent, td->length);
		free(td->name);
		td = td->next;
	}

	/*
	 * Execute all unflushed '-exec .... {} +' expressions.
	 */
	if (do_find) {
		find_plusflush(plusp, &walkstate);
#ifdef	__use_find_free__
		find_free(Tree, &fa);
#endif
		if (walkstate.patstate != NULL)
			free(walkstate.patstate);
		ret = walkstate.err;
	}

	if (infile != NULL)
		fclose(infile);
	return (ret);
}

LOCAL void
list_vd(vp, ucs)
	struct iso_primary_descriptor	*vp;
	BOOL				ucs;
{
	struct iso_directory_record	*idr = (struct iso_directory_record *)
						vp->root_directory_record;

	printf(_("System id: "));
	printchars(vp->system_id, 32, ucs);
	putchar('\n');
	printf(_("Volume id: "));
	printchars(vp->volume_id, 32, ucs);
	putchar('\n');

	printf(_("Volume set id: "));
	printchars(vp->volume_set_id, 128, ucs);
	putchar('\n');
	printf(_("Publisher id: "));
	printchars(vp->publisher_id, 128, ucs);
	putchar('\n');
	printf(_("Data preparer id: "));
	printchars(vp->preparer_id, 128, ucs);
	putchar('\n');
	printf(_("Application id: "));
	printchars(vp->application_id, 128, ucs);
	putchar('\n');

	printf(_("Copyright File id: "));
	printchars(vp->copyright_file_id, 37, ucs);
	putchar('\n');
	printf(_("Abstract File id: "));
	printchars(vp->abstract_file_id, 37, ucs);
	putchar('\n');
	printf(_("Bibliographic File id: "));
	printchars(vp->bibliographic_file_id, 37, ucs);
	putchar('\n');

	printf(_("Volume set size is: %d\n"), isonum_723(vp->volume_set_size));
	printf(_("Volume set sequence number is: %d\n"), isonum_723(vp->volume_sequence_number));
	printf(_("Logical block size is: %d\n"), isonum_723(vp->logical_block_size));
	printf(_("Volume size is: %d\n"), isonum_733((unsigned char *)vp->volume_space_size));
	if (debug) {
		int	dextent;
		int	dlen;

		dextent = isonum_733((unsigned char *)idr->extent);
		dlen = isonum_733((unsigned char *)idr->size);
		printf(_("Root directory extent:  %d size: %d\n"),
			dextent, dlen);
		printf(_("Path table size is:     %d\n"),
			isonum_733((unsigned char *)vp->path_table_size));
		printf(_("L Path table start:     %d\n"),
			isonum_731(vp->type_l_path_table));
		printf(_("L Path opt table start: %d\n"),
			isonum_731(vp->opt_type_l_path_table));
		printf(_("M Path table start:     %d\n"),
			isonum_732(vp->type_m_path_table));
		printf(_("M Path opt table start: %d\n"),
			isonum_732(vp->opt_type_m_path_table));
		printf(_("Creation Date:     %s\n"),
			sdate(vp->creation_date));
		printf(_("Modification Date: %s\n"),
			sdate(vp->modification_date));
		printf(_("Expiration Date:   %s\n"),
			sdate(vp->expiration_date));
		printf(_("Effective Date:    %s\n"),
			sdate(vp->effective_date));
		printf(_("File structure version: %d\n"),
			vp->file_structure_version[0]);
	}
}

LOCAL void
list_locales()
{
	int	n;

	n = sic_list(stdout);
	if (n <= 0) {
		errmsgno(EX_BAD, "'%s/lib/siconv/' %s.\n",
			INS_BASE, n < 0 ? _("missing"):_("incomplete"));
		if (n == 0) {
			errmsgno(EX_BAD,
			_("Check '%s/lib/siconv/' for missing translation tables.\n"),
			INS_BASE);
		}
	}
#ifdef	USE_ICONV
	if (n > 0) {
		errmsgno(EX_BAD,
		_("'iconv -l' lists more available names.\n"));
	}
#endif
}

#include <schily/intcvt.h>

LOCAL void
printf_bootinfo(f, bootcat_offset)
	FILE	*f;
	int	bootcat_offset;
{
	int					s = 0;
	int					i;
	Uchar					*p;
	struct eltorito_validation_entry	*evp;
	struct eltorito_defaultboot_entry	*ebe;

#ifdef	USE_SCG
	readsecs(bootcat_offset, buffer, ISO_BLOCKS(sizeof (buffer)));
#else
	lseek(fileno(f), ((off_t)bootcat_offset) <<11, SEEK_SET);
	read(fileno(f), buffer, sizeof (buffer));
#endif

	evp = (struct eltorito_validation_entry *)buffer;
	ebe = (struct eltorito_defaultboot_entry *)&buffer[32];


	p = (Uchar *)evp;
	for (i = 0; i < 32; i += 2) {
		s += p[i] & 0xFF;
		s += (p[i+1] & 0xFF) * 256;
	}
	s = s % 65536;

	printf(_("Eltorito validation header:\n"));
	printf(_("    Hid %d\n"), (Uchar)evp->headerid[0]);
	printf(_("    Arch %d (%s)\n"), (Uchar)evp->arch[0], arch_name((Uchar)evp->arch[0]));
	printf(_("    ID '%.23s'\n"), evp->id);
	printf(_("    Cksum %2.2X %2.2X %s\n"), (Uchar)evp->cksum[0], (Uchar)evp->cksum[1],
					s == 0 ? _("OK"):_("BAD"));
	printf(_("    Key %X %X\n"), (Uchar)evp->key1[0], (Uchar)evp->key2[0]);

	printf(_("    Eltorito defaultboot header:\n"));
	printf(_("        Bootid %X (%s)\n"), (Uchar)ebe->boot_id[0], boot_name((Uchar)ebe->boot_id[0]));
	printf(_("        Boot media %X (%s)\n"), (Uchar)ebe->boot_media[0], bootmedia_name((Uchar)ebe->boot_media[0]));
	printf(_("        Load segment %X\n"), la_to_2_byte(ebe->loadseg));
	printf(_("        Sys type %X\n"), (Uchar)ebe->sys_type[0]);
	printf(_("        Nsect %X\n"), la_to_2_byte(ebe->nsect));
	printf(_("        Bootoff %lX %ld\n"), la_to_4_byte(ebe->bootoff), la_to_4_byte(ebe->bootoff));

}

LOCAL char *
arch_name(val)
	int	val;
{
	switch (val) {

	case EL_TORITO_ARCH_x86:
		return ("x86");
	case EL_TORITO_ARCH_PPC:
		return ("PPC");
	case EL_TORITO_ARCH_MAC:
		return ("MAC");
	default:
		return ("Unknown Arch");
	}
}

LOCAL char *
boot_name(val)
	int	val;
{
	switch (val) {

	case EL_TORITO_BOOTABLE:
		return ("bootable");
	case EL_TORITO_NOT_BOOTABLE:
		return ("not bootable");
	default:
		return ("Illegal");
	}
}

LOCAL char *
bootmedia_name(val)
	int	val;
{
	switch (val) {

	case EL_TORITO_MEDIA_NOEMUL:
		return ("No Emulation Boot");
	case EL_TORITO_MEDIA_12FLOP:
		return ("1200 Floppy");
	case EL_TORITO_MEDIA_144FLOP:
		return ("1.44MB Floppy");
	case EL_TORITO_MEDIA_288FLOP:
		return ("2.88MB Floppy");
	case EL_TORITO_MEDIA_HD:
		return ("Hard Disk Emulation");
	default:
		return ("Illegal Bootmedia");
	}
}

LOCAL int
time_cvt(dp, len)
	unsigned char	*dp;
	int		len;
{
	int	ret;

	for (ret = 0; --len >= 0; ) {
		ret *= 10;
		ret += *dp++ - '0';
	}
	return (ret);
}

LOCAL time_t
iso9660_time(date, hsecp, longfmt)
	unsigned char	*date;
	int		*hsecp;
	BOOL		longfmt;
{
	time_t	t;
	int	y;
	int	m;
	int	d;
	int	days;
	int	hour;
	int	min;
	int	sec;
	int	hsec;
	int	gmtoff;

	if (longfmt) {
		y = time_cvt(&date[0], 4);	/* Year 0..9999 */
		m = time_cvt(&date[4], 2);
		d = time_cvt(&date[6], 2);
		hour = time_cvt(&date[8], 2);
		min = time_cvt(&date[10], 2);
		sec = time_cvt(&date[12], 2);
		hsec = time_cvt(&date[14], 2);
		gmtoff = ((char *)date)[16];
	} else {
		y = date[0] + 1900;		/* Year 1900..2155 */
		m = date[1];
		d = date[2];
		hour = date[3];
		min = date[4];
		sec = date[5];
		hsec = 0;
		gmtoff = ((char *)date)[6];
	}
	if (hsecp)
		*hsecp = hsec;
	/*
	 * The original algorithm did win a Fortan contest in early times.
	 * It computes days relative to September 19th 1989.
	 * days = 367*(y-1980)-7*(y+(m+9)/12)/4-3*((y+(m-9)/7)/100+1)/4+275*m/9+d-100;
	 * The updated algorithm was modified to use Jan 1st 1970 as base
	 * and was taken from FreeBSD.
	 */
	days = 367*(y-1960)-7*(y+(m+9)/12)/4-3*((y+(m+9)/12-1)/100+1)/4+275*m/9+d-239;
	t = days;
	t = ((((t * 24) + hour) * 60 + min) * 60) + sec;
	if (-48 <= gmtoff && gmtoff <= 52)
		t -= gmtoff * 15 * 60;
	return (t);
}

#ifdef	USE_FIND
/* ARGSUSED */
LOCAL int
getfind(arg, valp, pac, pav)
	char	*arg;
	long	*valp;	/* Not used until we introduce a ptr to opt struct */
	int	*pac;
	char	*const	**pav;
{
	do_find = TRUE;
	find_ac = *pac;
	find_av = *pav;
	find_ac--, find_av++;
	return (NOARGS);
}

/*
 * Called from dump_stat()
 */
LOCAL BOOL
find_stat(rootname, idr, fname, extent)
	char	*rootname;
	struct iso_directory_record *idr;
	char	*fname;
	int	extent;
{
	BOOL	ret;
	int	rlen;

	if (name_buf[0] == '.' && name_buf[1] == '.' && name_buf[2] == '\0')
		if (find_node)
			return (FALSE);

	if (find_node == NULL)	/* No find(1) rules */
		return (TRUE);	/* so pass everything */

	rlen = strlen(rootname);

#ifdef	HAVE_ST_BLKSIZE
	fstat_buf.st_blksize = 0;
#endif
#ifdef	HAVE_ST_BLOCKS
	fstat_buf.st_blocks = (fstat_buf.st_size+1023) / DEV_BSIZE;
#endif
	walkstate.lname = &xname[3];
	walkstate.pflags = PF_ACL|PF_XATTR;
#ifdef	XXX
	if (info->f_xflags & (XF_ACL_ACCESS|XF_ACL_DEFAULT))
		walkstate.pflags |= PF_HAS_ACL;
	if (info->f_xflags & XF_XATTR)
		walkstate.pflags |= PF_HAS_XATTR;
#endif

	ret = find_expr(fname, &fname[rlen], &fstat_buf, &walkstate, find_node);
	if (!ret)
		return (ret);
	return (ret);
}
#endif	/* USE_FIND */
