/* @(#)dump.c	1.41 15/12/07 joerg */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)dump.c	1.41 15/12/07 joerg";
#endif
/*
 * File dump.c - dump a file/device both in hex and in ASCII.
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

#include "../scsi.h"
#include "cdrdeflt.h"
#include "../../cdrecord/version.h"

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
EXPORT	FILE		*infile = NULL;
EXPORT	BOOL		ignerr = FALSE;
LOCAL	off_t		file_addr;
LOCAL	off_t		sec_addr = (off_t)-1;
LOCAL	Uchar		sector[2048];
#define	PAGE	256
LOCAL	Uchar		buffer[PAGE];
LOCAL	Uchar		search[64];

#ifdef	USE_V7_TTY
LOCAL	struct sgttyb	savetty;
LOCAL	struct sgttyb	newtty;
#else
#ifdef	USE_TERMIOS
LOCAL	struct termios	savetty;
LOCAL	struct termios	newtty;
#endif
#endif

LOCAL void	reset_tty	__PR((void));
LOCAL void	set_tty		__PR((void));
LOCAL void	onsusp		__PR((int sig));
LOCAL void	crsr2		__PR((int row, int col));
LOCAL void	readblock	__PR((void));
LOCAL void	showblock	__PR((int flag));
LOCAL int	getbyte		__PR((void));
LOCAL void	usage		__PR((int excode));
EXPORT int	main		__PR((int argc, char *argv[]));

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


/*
 * Come here when we get a suspend signal from the terminal
 */
LOCAL void
onsusp(sig)
	int	sig;
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
	/*    sigsetmask(0);*/
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

LOCAL void
readblock()
{
	off_t	dpos = file_addr - sec_addr;

	if (sec_addr < 0 ||
	    dpos < 0 || (dpos + sizeof (buffer)) > sizeof (sector)) {
		sec_addr = file_addr & ~2047;
#ifdef	USE_SCG
		readsecs(sec_addr/2048, sector, ISO_BLOCKS(sizeof (sector)));
#else
		lseek(fileno(infile), sec_addr, SEEK_SET);
		read(fileno(infile), sector, sizeof (sector));
#endif
		dpos = file_addr - sec_addr;
	}
	movebytes(&sector[dpos], buffer, sizeof (buffer));
}

LOCAL void
showblock(flag)
	int	flag;
{
	unsigned int	k;
	int		i;
	int		j;

	readblock();
	if (flag) {
		for (i = 0; i < 16; i++) {
			crsr2(i+3, 1);
			if (sizeof (file_addr) > sizeof (long)) {
				printf("%16.16llx ", (Llong)file_addr+(i<<4));
			} else {
				printf("%8.8lx ", (long)file_addr+(i<<4));
			}
			for (j = 15; j >= 0; j--) {
				printf("%2.2x", buffer[(i<<4)+j]);
				if (!(j & 0x3))
					printf(" ");
			}
			for (j = 0; j < 16; j++) {
				k = buffer[(i << 4) + j];
				if (k >= ' ' && k < 0x80)
					printf("%c", k);
				else
					printf(".");
			}
		}
	}
	crsr2(20, 1);
	if (sizeof (file_addr) > sizeof (long)) {
		printf(_(" Zone, zone offset: %14llx %12.12llx  "),
			(Llong)file_addr>>11, (Llong)file_addr & 0x7ff);
	} else {
		printf(_(" Zone, zone offset: %6lx %4.4lx  "),
			(long)(file_addr>>11), (long)(file_addr & 0x7ff));
	}
	fflush(stdout);
}

LOCAL int
getbyte()
{
	char	c1;

	c1 = buffer[file_addr & (PAGE-1)];
	file_addr++;
	if ((file_addr & (PAGE-1)) == 0)
		showblock(0);
	return (c1);
}

LOCAL void
usage(excode)
	int	excode;
{
	errmsgno(EX_BAD, _("Usage: %s [options] [image]\n"),
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
	int	j;

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
		printf(_("devdump %s (%s-%s-%s) Copyright (C) 1993-1999 %s (C) 1999-2015 %s\n"),
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

	for (i = 0; i < 30; i++)
		printf("\n");
	file_addr = (off_t)0;

	/*
	 * Now setup the keyboard for single character input.
	 */
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
		if (file_addr < (off_t)0) file_addr = (off_t)0;
		showblock(1);
#ifdef	USE_GETCH
		c = getch();	/* DOS console input */
#else
		read(STDIN_FILENO, &c, 1);
#endif
		if (c == 'a')
			file_addr -= PAGE;
		if (c == 'b')
			file_addr += PAGE;
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
			file_addr = file_addr << 11;
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
				for (j = 1; j < slen; j++) {
					if (search[j] != getbyte())
						break;
				}
				if (j == slen)
					break;
			}
			file_addr &= ~(PAGE-1);
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
