/* @(#)isodump.c	1.52 15/12/08 joerg */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)isodump.c	1.52 15/12/08 joerg";
#endif
/*
 * File isodump.c - dump iso9660 directory information.
 *
 *
 * Written by Eric Youngdale (1993).
 *
 * Copyright 1993 Yggdrasil Computing, Incorporated
 * Copyright (c) 1999-2015 J. Schilling
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

#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/utypes.h>

#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/termios.h>
#include <schily/signal.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

#include "../iso9660.h"
#include "../rock.h"
#include "../scsi.h"
#include "cdrdeflt.h"
#include "../../cdrecord/version.h"

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

#define	infile	in_image
EXPORT FILE	*infile = NULL;
EXPORT BOOL	ignerr = FALSE;
LOCAL off_t	file_addr;
LOCAL int	su_version;
LOCAL int	rr_version;
LOCAL int	use_rock = TRUE;
LOCAL unsigned char buffer[2048];
LOCAL unsigned char search[64];
LOCAL int blocksize;

#define	PAGE	sizeof (buffer)

LOCAL int	isonum_731	__PR((char * p));
LOCAL int	isonum_721	__PR((char * p));
LOCAL int	isonum_723	__PR((char * p));
LOCAL int	isonum_733	__PR((char * p));
LOCAL void	reset_tty	__PR((void));
LOCAL void	set_tty		__PR((void));
LOCAL void	onsusp		__PR((int signo));
LOCAL void	crsr2		__PR((int row, int col));
LOCAL int	parse_rr	__PR((unsigned char * pnt, int len, int cont_flag));
LOCAL void	find_rr		__PR((struct iso_directory_record * idr, Uchar **pntp, int *lenp));
LOCAL int	dump_rr		__PR((struct iso_directory_record * idr));
LOCAL void	showblock	__PR((int flag));
LOCAL int	getbyte		__PR((void));
LOCAL void	usage		__PR((int excode));
EXPORT int	main		__PR((int argc, char *argv[]));

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
isonum_721(p)
	char	*p;
{
	return ((p[0] & 0xff) | ((p[1] & 0xff) << 8));
}

LOCAL int
isonum_723(p)
	char	*p;
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
isonum_733(p)
	char *p;
{
	return (isonum_731(p));
}

#ifdef	USE_V7_TTY
LOCAL	struct sgttyb	savetty;
LOCAL	struct sgttyb	newtty;
#else
#ifdef	USE_TERMIOS
LOCAL	struct termios savetty;
LOCAL	struct termios newtty;
#endif
#endif

LOCAL void
reset_tty()
{
#ifdef USE_V7_TTY
	if (ioctl(STDIN_FILENO, TIOCSETN, &savetty) == -1) {
#else
#ifdef	USE_TERMIOS
#ifdef	TCSANOW
	if (tcsetattr(STDIN_FILENO, TCSANOW, &savetty) == -1) {
#else
	if (ioctl(STDIN_FILENO, TCSETAF, &savetty) == -1) {
#endif
#else	/* USE_TERMIOS */
	if (0) {
#endif	/* USE_TERMIOS */
#endif
		printf(_("Cannot put tty into normal mode\n"));
		exit(1);
	}
}

LOCAL void
set_tty()
{
#ifdef USE_V7_TTY
	if (ioctl(STDIN_FILENO, TIOCSETN, &newtty) == -1) {
#else
#ifdef	USE_TERMIOS
#ifdef	TCSANOW
	if (tcsetattr(STDIN_FILENO, TCSANOW, &newtty) == -1) {
#else
	if (ioctl(STDIN_FILENO, TCSETAF, &newtty) == -1) {
#endif
#else	/* USE_TERMIOS */
	if (0) {
#endif	/* USE_TERMIOS */
#endif
		printf(_("Cannot put tty into raw mode\n"));
		exit(1);
	}
}

/* Come here when we get a suspend signal from the terminal */

LOCAL void
onsusp(signo)
	int	signo;
{
#ifdef	SIGTTOU
	/* ignore SIGTTOU so we don't get stopped if csh grabs the tty */
	signal(SIGTTOU, SIG_IGN);
#endif
	reset_tty();
	fflush(stdout);
#ifdef	SIGTTOU
	signal(SIGTTOU, SIG_DFL);
	/* Send the TSTP signal to suspend our process group */
	signal(SIGTSTP, SIG_DFL);
/*	sigsetmask(0);*/
	kill(0, SIGTSTP);
	/* Pause for station break */

	/* We're back */
	signal(SIGTSTP, onsusp);
#endif
	set_tty();
}



LOCAL void
crsr2(row, col)
	int	row;
	int	col;
{
	printf("\033[%d;%dH", row, col);
}

LOCAL int
parse_rr(pnt, len, cont_flag)
	unsigned char	*pnt;
	int		len;
	int		cont_flag;
{
	int		slen;
	int		ncount;
	int		extent;
	off_t		cont_extent;
	int		cont_offset;
	int		cont_size;
	int		flag1;
	int		flag2;
	unsigned char	*pnts;
	char		symlinkname[1024];
	char		name[1024];
	int		goof = 0;

/*	printf(" RRlen=%d ", len); */

	symlinkname[0] = 0;

	cont_extent = (off_t)0;
	cont_offset = cont_size = 0;

	ncount = 0;
	flag1 = -1;
	flag2 = 0;
	while (len >= 4) {
		if (ncount)
			printf(",");
		else
			printf("[");
		printf("%c%c", pnt[0], pnt[1]);
		if (pnt[3] != 1 && pnt[3] != 2) {
			printf(_("**BAD RRVERSION (%d) for %c%c\n"), pnt[3], pnt[0], pnt[1]);
			return (0);	/* JS ??? Is this right ??? */
		} else if (pnt[2] < 4) {
			printf(_("**BAD RRLEN (%d) in '%2.2s' field %2.2X %2.2X.\n"), pnt[2], pnt, pnt[0], pnt[1]);
			return (0);		/* JS ??? Is this right ??? */
		} else if (pnt[0] == 'R' && pnt[1] == 'R') {
			printf("=%d", pnt[3]);			/* RR version */
		}
		ncount++;
		if (pnt[0] == 'R' && pnt[1] == 'R') flag1 = pnt[4] & 0xff;
		if (strncmp((char *)pnt, "PX", 2) == 0) flag2 |= RR_FLAG_PX;
		if (strncmp((char *)pnt, "PN", 2) == 0) flag2 |= RR_FLAG_PN;
		if (strncmp((char *)pnt, "SL", 2) == 0) flag2 |= RR_FLAG_SL;
		if (strncmp((char *)pnt, "NM", 2) == 0) {
			slen = pnt[2] - 5;
			pnts = pnt+5;
			if ((pnt[4] & 6) != 0) {
				printf("*");
			}
			memset(name, 0, sizeof (name));
			memcpy(name, pnts, slen);
			printf("=%s", name);
			flag2 |= RR_FLAG_NM;
		}
		if (strncmp((char *)pnt, "CL", 2) == 0) flag2 |= RR_FLAG_CL;
		if (strncmp((char *)pnt, "PL", 2) == 0) flag2 |= RR_FLAG_PL;
		if (strncmp((char *)pnt, "RE", 2) == 0) flag2 |= RR_FLAG_RE;
		if (strncmp((char *)pnt, "TF", 2) == 0) flag2 |= RR_FLAG_TF;

		if (strncmp((char *)pnt, "PX", 2) == 0) {
			extent = isonum_733((char *)pnt+12);	/* Link count */
			printf("=%x", extent);
		}

		if (strncmp((char *)pnt, "CE", 2) == 0) {
			cont_extent = (off_t)isonum_733((char *)pnt+4);
			cont_offset = isonum_733((char *)pnt+12);
			cont_size = isonum_733((char *)pnt+20);
			printf("=[%x,%x,%d]", (int)cont_extent, cont_offset,
								cont_size);
		}

		if (strncmp((char *)pnt, "ER", 2) == 0) {		/* ER */
			int	lid = pnt[4] & 0xFF;			/* Len ID  */
			int	ldes = pnt[5] & 0xFF;			/* Len des */
			int	lsrc = pnt[6] & 0xFF;			/* Len src */
			int	xver = pnt[7] & 0xFF;			/* X vers  */
			flag2 |= RR_FLAG_ER;				/* ER record */

			rr_version = xver;
			printf(_("=[len_id=%d,len_des=%d,len_src=%d,ext_ver=%d,id=\"%.*s\"]"),
				lid, ldes, lsrc, xver, lid, &pnt[8]);

		}
		if (strncmp((char *)pnt, "SP", 2) == 0) {		/* SUSP */
			flag2 |= RR_FLAG_SP;				/* SUSP record */
			su_version = pnt[3] & 0xff;
			printf(_("=[skip=%d]"), pnt[6] & 0xFF);		/* SUSP skip off */

		}
		if (strncmp((char *)pnt, "ST", 2) == 0) {		/* Terminate SUSP */
			break;
		}

		if (strncmp((char *)pnt, "PL", 2) == 0 || strncmp((char *)pnt, "CL", 2) == 0) {
			extent = isonum_733((char *)pnt+4);
			printf("=%x", extent);				/* DIR extent */
		}

		if (strncmp((char *)pnt, "SL", 2) == 0) {
			int	cflag;

			cflag = pnt[4];					/* Component flag */
			pnts = pnt+5;
			slen = pnt[2] - 5;
			while (slen >= 1) {
				switch (pnts[0] & 0xfe) {
				case 0:
					strncat(symlinkname, (char *)(pnts+2), pnts[1]);
					break;
				case 2:
					strcat(symlinkname, ".");
					break;
				case 4:
					strcat(symlinkname, "..");
					break;
				case 8:
					if ((pnts[0] & 1) == 0)
						strcat(symlinkname, "/");
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
				if ((pnts[0] & 1) == 0)
					strcat(symlinkname, "/");

				slen -= (pnts[1] + 2);
				pnts += (pnts[1] + 2);
			}
			if (cflag)
				strcat(symlinkname, "+");
			printf("=%s", symlinkname);
			symlinkname[0] = 0;
		}

		len -= pnt[2];
		pnt += pnt[2];
	}
	if (cont_extent) {
		unsigned char sector[2048];

#ifdef	USE_SCG
		readsecs(cont_extent * blocksize / 2048, sector, ISO_BLOCKS(sizeof (sector)));
#else
		lseek(fileno(infile), cont_extent * blocksize, SEEK_SET);
		read(fileno(infile), sector, sizeof (sector));
#endif
		flag2 |= parse_rr(&sector[cont_offset], cont_size, 1);
	}
	if (ncount)
		printf("]");
	if (!cont_flag && flag1 != -1 && flag1 != (flag2 & 0xFF)) {
		printf(_("Flag %x != %x"), flag1, flag2);
		goof++;
	}
	/*
	 * XXX Check goof?
	 */
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
	int		len;
	unsigned char	*pnt;

	find_rr(idr, &pnt, &len);
	return (parse_rr(pnt, len, 0));
}


LOCAL void
showblock(flag)
	int	flag;
{
	int	i;
	int	j;
	int	line;
	struct iso_directory_record	*idr;

#ifdef	USE_SCG
	readsecs(file_addr / 2048, buffer, ISO_BLOCKS(sizeof (buffer)));
#else
	lseek(fileno(infile), file_addr, SEEK_SET);
	read(fileno(infile), buffer, sizeof (buffer));
#endif
	for (i = 0; i < 60; i++)
		printf("\n");
	fflush(stdout);
	i = line = 0;
	if (flag) {
		while (1 == 1) {
			crsr2(line+3, 1);
			idr = (struct iso_directory_record *) &buffer[i];
			if (idr->length[0] == 0)
				break;
			printf("%3d ", idr->length[0]);
			printf("[%2d] ", idr->volume_sequence_number[0]);
			printf("%5x ", isonum_733(idr->extent));
			printf("%8d ", isonum_733(idr->size));
			printf("%02x/", idr->flags[0]);
			printf((idr->flags[0] & 2) ? "*" : " ");
			if (idr->name_len[0] == 1 && idr->name[0] == 0)
				printf(".             ");
			else if (idr->name_len[0] == 1 && idr->name[0] == 1)
				printf("..            ");
			else {
				for (j = 0; j < (int)idr->name_len[0]; j++) printf("%c", idr->name[j]);
				for (j = 0; j < (14 - (int)idr->name_len[0]); j++) printf(" ");
			}
			if (use_rock)
				dump_rr(idr);
			printf("\n");
			i += buffer[i];
			if (i > 2048 - offsetof(struct iso_directory_record, name[0]))
				break;
			line++;
		}
	}
	printf("\n");
	if (sizeof (file_addr) > sizeof (long)) {
		printf(_(" Zone, zone offset: %14llx %12.12llx  "),
			(Llong)file_addr / blocksize,
			(Llong)file_addr & (Llong)(blocksize - 1));
	} else {
		printf(_(" Zone, zone offset: %6lx %4.4lx  "),
			(long) (file_addr / blocksize),
			(long) file_addr & (blocksize - 1));
	}
	fflush(stdout);
}

LOCAL int
getbyte()
{
	char	c1;

	c1 = buffer[file_addr & (blocksize-1)];
	file_addr++;
	if ((file_addr & (blocksize-1)) == 0)
		showblock(0);
	return (c1);
}

LOCAL void
usage(excode)
	int	excode;
{
	errmsgno(EX_BAD, _("Usage: %s [options] image\n"),
						get_progname());

	error(_("Options:\n"));
	error(_("\t-help, -h	Print this help\n"));
	error(_("\t-version	Print version info and exit\n"));
	error(_("\t-ignore-error Ignore errors\n"));
	error(_("\t-i filename	Filename to read ISO-9660 image from\n"));
	error(_("\tdev=target	SCSI target to use as CD/DVD-Recorder\n"));
	error(_("\nIf neither -i nor dev= are specified, <image> is needed.\n"));
	exit(excode);
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
	char	c;
	int	i;
	struct iso_primary_descriptor	ipd;
	struct iso_directory_record	*idr;

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
		printf(_("isodump %s (%s-%s-%s) Copyright (C) 1993-1999 %s (C) 1999-2015 %s\n"),
					VERSION,
					HOST_CPU, HOST_VENDOR, HOST_OS,
					_("Eric Youngdale"),
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
		fprintf(stderr, _("Cannot open '%s'\n"), filename);
		exit(1);
	}

	file_addr = (off_t) (16 << 11);
#ifdef	USE_SCG
	readsecs(file_addr / 2048, &ipd, ISO_BLOCKS(sizeof (ipd)));
#else
	lseek(fileno(infile), file_addr, SEEK_SET);
	read(fileno(infile), &ipd, sizeof (ipd));
#endif
	idr = (struct iso_directory_record *)ipd.root_directory_record;

	blocksize = isonum_723((char *)ipd.logical_block_size);
	if (blocksize != 512 && blocksize != 1024 && blocksize != 2048) {
		blocksize = 2048;
	}

	file_addr = (off_t)isonum_733(idr->extent);
	file_addr = file_addr * blocksize;

#ifdef	USE_SCG
	readsecs(file_addr / 2048, buffer, ISO_BLOCKS(sizeof (buffer)));
#else
	lseek(fileno(infile), file_addr, SEEK_SET);
	read(fileno(infile), buffer, sizeof (buffer));
#endif
	i = dump_rr((struct iso_directory_record *) buffer);
	if (i == 0 ||
	    (i & (RR_FLAG_SP | RR_FLAG_ER)) == 0 || su_version < 1 || rr_version < 1) {
		use_rock = FALSE;
	}

/* Now setup the keyboard for single character input. */
#ifdef USE_V7_TTY
	if (ioctl(STDIN_FILENO, TIOCGETP, &savetty) == -1) {
#else
#ifdef	USE_TERMIOS
#ifdef	TCSANOW
	if (tcgetattr(STDIN_FILENO, &savetty) == -1) {
#else
	if (ioctl(STDIN_FILENO, TCGETA, &savetty) == -1) {
#endif
#else	/* USE_TERMIOS */
	if (0) {
#endif	/* USE_TERMIOS */
#endif
		printf(_("Stdin must be a tty\n"));
		exit(1);
	}
#ifdef USE_V7_TTY
	newtty = savetty;
	newtty.sg_flags  &= ~(ECHO|CRMOD);
	newtty.sg_flags  |= CBREAK;
#else
#ifdef	USE_TERMIOS
	newtty = savetty;
	newtty.c_lflag   &= ~ICANON;
	newtty.c_lflag   &= ~ECHO;
	newtty.c_cc[VMIN] = 1;
#endif
#endif
	set_tty();
#ifdef	SIGTSTP
	signal(SIGTSTP, onsusp);
#endif
	on_comerr((void(*)__PR((int, void *)))reset_tty, NULL);

	do {
		if (file_addr < 0)
			file_addr = (off_t)0;
		showblock(1);
#ifdef	USE_GETCH
		c = getch();	/* DOS console input */
#else
		read(STDIN_FILENO, &c, 1);
#endif
		if (c == 'a')
			file_addr -= blocksize;
		if (c == 'b')
			file_addr += blocksize;
		if (c == 'g') {
			crsr2(20, 1);
			printf(_("Enter new starting block (in hex):"));
			if (sizeof (file_addr) > sizeof (long)) {
				Llong	ll;
				scanf("%llx", &ll);
				file_addr = (off_t)ll;
			} else {
				long	l;
				scanf("%lx", &l);
				file_addr = (off_t)l;
			}
			file_addr = file_addr * blocksize;
			crsr2(20, 1);
			printf("                                     ");
		}
		if (c == 'f') {
			crsr2(20, 1);
			printf(_("Enter new search string:"));
			fgets((char *)search, sizeof (search), stdin);
			while (search[strlen((char *)search)-1] == '\n')
				search[strlen((char *)search)-1] = 0;
			crsr2(20, 1);
			printf("                                     ");
		}
		if (c == '+') {
			while (1 == 1) {
				int	slen;

				while (1 == 1) {
					c = getbyte();
					if (c == search[0])
						break;
				}
				slen = (int)strlen((char *)search);
				for (i = 1; i < slen; i++) {
					if (search[i] != getbyte())
						break;
				}
				if (i == slen)
					break;
			}
			file_addr &= ~(blocksize-1);
			showblock(1);
		}
		if (c == 'q')
			break;
	} while (1 == 1);
	reset_tty();
	if (infile != NULL)
		fclose(infile);
	return (0);
}
