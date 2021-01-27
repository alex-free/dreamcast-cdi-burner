/* @(#)cdrecord.c	1.415 16/01/21 Copyright 1995-2016 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)cdrecord.c	1.415 16/01/21 Copyright 1995-2016 J. Schilling";
#endif
/*
 *	Record data on a CD/CVD-Recorder
 *
 *	Copyright (c) 1995-2016 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

/*#define	TR_DEBUG*/
#include <schily/mconfig.h>
#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/fcntl.h>
#include <schily/errno.h>
#include <schily/time.h>
#include <schily/resource.h>	/* for rlimit */
#include <schily/stat.h>
#include <schily/unistd.h>
#include <schily/mman.h>
#include <schily/string.h>
#include <schily/utypes.h>
#include <schily/intcvt.h>
#include <schily/signal.h>
#include <schily/schily.h>
#include <schily/getargs.h>
#include <schily/nlsdefs.h>
#include <schily/priv.h>

#include "xio.h"

#include <scg/scsireg.h>	/* XXX wegen SC_NOT_READY */
#include <scg/scsitransp.h>
#include <scg/scgcmd.h>		/* XXX fuer read_buffer */
#include "scsi_scan.h"

#include "auheader.h"
#include "cdrecord.h"
#include "cdrdeflt.h"
#include "movesect.h"


#ifdef	VMS
#include <vms_init.h>
#endif

#include "version.h"

char	cdr_version[] = VERSION;

#if defined(_POSIX_PRIORITY_SCHEDULING) && _POSIX_PRIORITY_SCHEDULING -0 >= 0 && \
    defined(HAVE_SCHED_SETSCHEDULER)
#ifdef  HAVE_SYS_PRIOCNTL_H	/* The preferred SYSvR4 schduler */
#else
#define	USE_POSIX_PRIORITY_SCHEDULING
#endif
#endif

/*
 * Map toc/track types into names.
 */
char	*toc2name[] = {
		"CD-DA",
		"CD-ROM",
		"CD-ROM XA mode 1",
		"CD-ROM XA mode 2",
		"CD-I",
		"Illegal toc type 5",
		"Illegal toc type 6",
		"Illegal toc type 7",
};

/*
 * Map sector types into names.
 */
char	*st2name[] = {
		"Illegal sector type 0",
		"CD-ROM mode 1",
		"CD-ROM mode 2",
		"Illegal sector type 3",
		"CD-DA without preemphasis",
		"CD-DA with preemphasis",
		"Illegal sector type 6",
		"Illegal sector type 7",
};

/*
 * Map data block types into names.
 */
char	*db2name[] = {
		"Raw (audio)",
		"Raw (audio) with P/Q sub channel",
		"Raw (audio) with P/W packed sub channel",
		"Raw (audio) with P/W raw sub channel",
		"Reserved mode 4",
		"Reserved mode 5",
		"Reserved mode 6",
		"Vendor unique mode 7",
		"CD-ROM mode 1",
		"CD-ROM mode 2",
		"CD-ROM XA mode 2 form 1",
		"CD-ROM XA mode 2 form 1 (with subheader)",
		"CD-ROM XA mode 2 form 2",
		"CD-ROM XA mode 2 form 1/2/mix",
		"Reserved mode 14",
		"Vendor unique mode 15",
};

/*
 * Map write modes into names.
 */
LOCAL	char	wm_none[] = "unknown";
LOCAL	char	wm_ill[]  = "illegal";

char	*wm2name[] = {
		wm_none,
		"BLANK",
		"FORMAT",
		wm_ill,
		"PACKET",
		wm_ill,
		wm_ill,
		wm_ill,
		"TAO",
		wm_ill,
		wm_ill,
		wm_ill,
		"SAO",
		"SAO/RAW16",	/* Most liklely not needed */
		"SAO/RAW96P",
		"SAO/RAW96R",
		"RAW",
		"RAW/RAW16",
		"RAW/RAW96P",
		"RAW/RAW96R",
};

	int	debug;		/* print debug messages		debug=#,-d  */
LOCAL	int	kdebug;		/* print kernel debug messages	kdebug#,kd# */
LOCAL	int	scsi_verbose;	/* SCSI verbose flag		-Verbose,-V */
LOCAL	int	silent;		/* SCSI silent flag		-silent,-s  */
	int	lverbose;	/* local verbose flag		-verbose,-v */
	int	xdebug;		/* extended debug flag		-x,xd=#	    */

char	*buf;			/* The transfer buffer */
long	bufsize = -1;		/* The size of the transfer buffer */

LOCAL	int	gracetime = GRACE_TIME;
LOCAL	int	raw_speed = -1;
LOCAL	int	dma_speed = -1;
LOCAL	int	dminbuf = -1;	/* XXX Hack for now drive min buf fill */
EXPORT	BOOL	isgui;
LOCAL	int	didintr;
EXPORT	char	*driveropts;
LOCAL	char	*cuefilename;
LOCAL	uid_t	oeuid = (uid_t)-1;
LOCAL	uid_t	ouid = (uid_t)-1;
LOCAL	BOOL	issetuid = FALSE;
LOCAL	char	*isobuf;	/* Buffer for doing -isosize on stdin	  */
LOCAL	int	isobsize;	/* The amount of remaining data in isobuf */
LOCAL	int	isoboff;	/* The current "read" offset in isobuf	  */

struct timeval	starttime;
struct timeval	wstarttime;
struct timeval	stoptime;
struct timeval	fixtime;

static	long	fs = -1L;	/* fifo (ring buffer) size */

EXPORT	int 	main		__PR((int ac, char **av));
LOCAL	void	scg_openerr	__PR((char *errstr));
LOCAL	int	find_drive	__PR((SCSI *scgp, char *dev, int flags));
LOCAL	int	gracewait	__PR((cdr_t *dp, BOOL *didgracep));
LOCAL	void	cdrstats	__PR((cdr_t *dp));
LOCAL	void	susage		__PR((int));
LOCAL	void	usage		__PR((int));
LOCAL	void	blusage		__PR((int));
LOCAL	void	intr		__PR((int sig));
LOCAL	void	catchsig	__PR((int sig));
LOCAL	int	scsi_cb		__PR((void *arg));
LOCAL	void	intfifo		__PR((int sig));
LOCAL	void	exscsi		__PR((int excode, void *arg));
LOCAL	void	excdr		__PR((int excode, void *arg));
EXPORT	int	read_buf	__PR((int f, char *bp, int size));
EXPORT	int	fill_buf	__PR((int f, track_t *trackp, long secno,
							char *bp, int size));
EXPORT	int	get_buf		__PR((int f, track_t *trackp, long secno,
							char **bpp, int size));
EXPORT	int	write_secs	__PR((SCSI *scgp, cdr_t *dp, char *bp,
						long startsec, int bytespt,
						int secspt, BOOL islast));
EXPORT	int	write_track_data __PR((SCSI *scgp, cdr_t *, track_t *));
EXPORT	int	pad_track	__PR((SCSI *scgp, cdr_t *dp,
					track_t *trackp,
					long startsec, Llong amt,
					BOOL dolast, Llong *bytesp));
EXPORT	int	write_buf	__PR((SCSI *scgp, cdr_t *dp,
					track_t *trackp,
					char *bp, long startsec, Llong amt,
					int secsize,
					BOOL dolast, Llong *bytesp));
LOCAL	void	printdata	__PR((int, track_t *));
LOCAL	void	printaudio	__PR((int, track_t *));
LOCAL	void	checkfile	__PR((int, track_t *));
LOCAL	int	checkfiles	__PR((int, track_t *));
LOCAL	void	setleadinout	__PR((int, track_t *));
LOCAL	void	setpregaps	__PR((int, track_t *));
LOCAL	long	checktsize	__PR((int, track_t *));
LOCAL	void	opentracks	__PR((track_t *));
LOCAL	int	cvt_hidden	__PR((track_t *));
LOCAL	void	checksize	__PR((track_t *));
LOCAL	BOOL	checkdsize	__PR((SCSI *scgp, cdr_t *dp,
					long tsize, UInt32_t flags));
LOCAL	void	raise_fdlim	__PR((void));
LOCAL	void	raise_memlock	__PR((void));
LOCAL	int	getfilecount	__PR((int ac, char *const *av, const char *fmt));
LOCAL	void	gargs		__PR((int, char **, int *, track_t *,
					char **, char **,
					int *, cdr_t **,
					int *, UInt32_t *, int *));
LOCAL	int	default_wr_mode	__PR((int tracks, track_t *trackp,
					UInt32_t *flagsp, int *wmp, int flags));
LOCAL	void	etracks		__PR((char *opt));
LOCAL	void	set_trsizes	__PR((cdr_t *, int, track_t *));
EXPORT	void	load_media	__PR((SCSI *scgp, cdr_t *, BOOL));
EXPORT	void	unload_media	__PR((SCSI *scgp, cdr_t *, UInt32_t));
EXPORT	void	reload_media	__PR((SCSI *scgp, cdr_t *));
EXPORT	void	set_secsize	__PR((SCSI *scgp, int secsize));
LOCAL	int	_read_buffer	__PR((SCSI *scgp, int));
LOCAL	int	get_dmaspeed	__PR((SCSI *scgp, cdr_t *));
LOCAL	BOOL	do_opc		__PR((SCSI *scgp, cdr_t *, UInt32_t));
LOCAL	void	check_recovery	__PR((SCSI *scgp, cdr_t *, UInt32_t));
	void	audioread	__PR((SCSI *scgp, cdr_t *, int));
LOCAL	void	print_msinfo	__PR((SCSI *scgp, cdr_t *));
LOCAL	void	print_toc	__PR((SCSI *scgp, cdr_t *));
LOCAL	void	print_track	__PR((int, long, struct msf *, int, int, int));
#if !defined(HAVE_SYS_PRIOCNTL_H)
LOCAL	int	rt_raisepri	__PR((int));
#endif
EXPORT	void	raisepri	__PR((int));
LOCAL	void	wait_input	__PR((void));
LOCAL	void	checkgui	__PR((void));
LOCAL	int	getbltype	__PR((char *optstr, long *typep));
LOCAL	void	print_drflags	__PR((cdr_t *dp));
LOCAL	void	print_wrmodes	__PR((cdr_t *dp));
LOCAL	BOOL	check_wrmode	__PR((cdr_t *dp, UInt32_t wmode, int tflags));
LOCAL	void	set_wrmode	__PR((cdr_t *dp, UInt32_t wmode, int tflags));
LOCAL	void	linuxcheck	__PR((void));
LOCAL	void	priv_warn	__PR((const char *what, const char *msg));
#ifdef	TR_DEBUG
EXPORT	void	prtrack		__PR((track_t *trackp));
#endif

struct exargs {
	SCSI		*scgp;		/* The open SCSI * pointer	    */
	cdr_t		*dp;		/* The pointer to the device driver */
	int		old_secsize;
	UInt32_t	flags;
	int		exflags;
} exargs;

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	char	*dev = NULL;
#if	defined(USE_NLS)
	char	*dir;
#endif
	int	timeout = 40;	/* Set default timeout to 40s CW-7502 is slow*/
	int	speed = -1;
	UInt32_t flags = 0L;
	int	blanktype = 0;
	int	i;
	int	tracks = 0;
	int	trackno;
	long	tsize;
	track_t	track[MAX_TRACK+2];	/* Max tracks + track 0 + track AA */
	cdr_t	*dp = (cdr_t *)0;
	long	startsec = 0L;
	int	errs = 0;
	SCSI	*scgp = NULL;
	char	*scgopts = NULL;
	char	errstr[80];
	BOOL	gracedone = FALSE;

#ifdef __EMX__
	/* This gives wildcard expansion with Non-Posix shells with EMX */
	_wildcard(&ac, &av);
#endif
#ifdef	HAVE_SOLARIS_PPRIV
	/*
	 * Try to gain additional privs on Solaris
	 */
	do_pfexec(ac, av,
		PRIV_FILE_DAC_READ,	/* to open /dev/ nodes for USCSICMD */
		PRIV_SYS_DEVICES,	/* to issue USCSICMD ioctl */
		PRIV_PROC_LOCK_MEMORY,	/* to grant realtime writes to CD-R */
		PRIV_PROC_PRIOCNTL,	/* to grant realtime writes to CD-R */
		PRIV_NET_PRIVADDR,	/* to access remote writer via RSCSI */
		NULL);
	/*
	 * Starting from here, we potentially have more privileges.
	 */
#endif
	save_args(ac, av);
	/*
	 * At this point, we should have the needed privileges, either because:
	 *
	 *	1)	We have been called by a privileged user (eg. root)
	 *	2)	This is a suid-root process
	 *	3)	This is a process that did call pfexec to gain privs
	 *	4)	This is a process that has been called via pfexec
	 *	5)	This is a process that gained privs via fcaps
	 *
	 * Case (1) is the only case where whe should not give up privileges
	 * because people would not expect it and because there will be no
	 * privilege escalation in this process.
	 */
	oeuid = geteuid();		/* Remember saved set uid	*/
	ouid = getuid();		/* Remember uid			*/
#ifdef	HAVE_ISSETUGID
	issetuid = issetugid();
#else
	issetuid = oeuid != ouid;
#endif

#if	defined(USE_NLS)
	(void) setlocale(LC_ALL, "");
#if !defined(TEXT_DOMAIN)	/* Should be defined by cc -D */
#define	TEXT_DOMAIN "cdrecord"	/* Use this only if it weren't */
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


	fillbytes(track, sizeof (track), '\0');
	for (i = 0; i < MAX_TRACK+2; i++)
		track[i].track = track[i].trackno = i;
	track[0].tracktype = TOC_MASK;
	raise_fdlim();
	gargs(ac, av, &tracks, track, &dev, &scgopts, &timeout, &dp, &speed,
						&flags, &blanktype);
	if ((track[0].tracktype & TOC_MASK) == TOC_MASK)
		comerrno(EX_BAD, _("Internal error: Bad TOC type.\n"));

	/*
	 * Attention: Additional restrictions to the CDDL are in place
	 * regarding this file. Please read both the files CDDL.Schily
	 * and cdrecord/LIMITATIONS for details.
	 *
	 * The Author does encourage you to not make changes not in
	 * consent with him. Since this is free software, you are of
	 * course free to make any modifications you seem fit. However,
	 * if you choose to do this, additional restrictions
	 * apply. Again, see cdrecord/LIMITATIONS for details.
	 *
	 * In order to comply with the restriction, the author suggests
	 * the following measures:
	 *
	 *	-	Clearly state that the current version is an
	 *		inofficial (modified) version and thus may have bugs
	 *		that are not present in the original.
	 *
	 *	-	Print your support e-mail address and tell people that
	 *		you will do complete support for this version of
	 *		cdrecord.
	 *
	 *		Or clearly state that there is absolutely no support
	 *		for the modified version you did create.
	 *
	 *	-	Tell the users not to ask the original author for
	 *		help.
	 *
	 * This limitation definitely also applies when you use any other
	 * cdrecord release together with libscg-0.6 or later, or when you
	 * use any amount of code from cdrecord-1.11a17 or later.
	 * In fact, it applies to any version of cdrecord.
	 *
	 * I am sorry for the inconvenience but I am forced to do this because
	 * some people create inofficial branches. These branches create
	 * problems but the initiators do not give support and thus cause the
	 * development of the official cdrecord versions to slow down because
	 * I am loaded with unneeded work.
	 *
	 * Please note that this is a memorandum on how I interpret the OSI
	 * rules at http://www.opensource.org/docs/definition.php
	 * If you use/modify/redistribute cdrecord, you need to accept it
	 * this way.
	 *
	 *
	 * The above statement is void if there has been neither a new version
	 * of cdrecord nor a new version of star from the original author
	 * within more then a year.
	 */

	/*
	 * Begin restricted code for quality assurance.
	 */

	/*
	 * Ugly, but Linux incude files violate POSIX and #define printf,
	 * so we cannot include the #ifdef inside the printf() arg list.
	 */
	i = set_cdrcmds("mmc_dvd", (cdr_t **)NULL);

#	define	PRODVD_TITLE	i?"-ProDVD":""
#	define	PROBD_TITLE	"-ProBD"
#ifdef	CLONE_WRITE
#	define	CLONE_TITLE	"-Clone"
#else
#	define	CLONE_TITLE	""
#endif
	if ((flags & F_MSINFO) == 0 || lverbose || flags & F_VERSION) {
		printf(_("Cdrecord%s%s%s %s (%s-%s-%s) Copyright (C) 1995-2016 %s\n"),
								PRODVD_TITLE,
								PROBD_TITLE,
								CLONE_TITLE,
								cdr_version,
								HOST_CPU, HOST_VENDOR, HOST_OS,
								_("Joerg Schilling"));

#if	defined(SOURCE_MODIFIED) || !defined(IS_SCHILY_XCONFIG)
#define	INSERT_YOUR_EMAIL_ADDRESS_HERE
#define	NO_SUPPORT	0
		printf(_("NOTE: this version of cdrecord is an inofficial (modified) release of cdrecord\n"));
		printf(_("      and thus may have bugs that are not present in the original version.\n"));
#if	NO_SUPPORT
		printf(_("      The author of the modifications decided not to provide a support e-mail\n"));
		printf(_("      address so there is absolutely no support for this version.\n"));
#else
		printf(_("      Please send bug reports and support requests to <%s>.\n"), INSERT_YOUR_EMAIL_ADDRESS_HERE);
#endif
		printf(_("      The original author should not be bothered with problems of this version.\n"));
		printf("\n");
#endif
#if	!defined(IS_SCHILY_XCONFIG)
		printf(_("\nWarning: This version of cdrecord has not been configured via the standard\n"));
		printf(_("autoconfiguration method of the Schily makefile system. There is a high risk\n"));
		printf(_("that the code is not configured correctly and for this reason will not behave\n"));
		printf(_("as expected.\n"));
#endif
	}

	if (flags & F_VERSION)
		exit(0);
	/*
	 * End restricted code for quality assurance.
	 */
	checkgui();

	if (debug || lverbose) {
		printf(_("TOC Type: %d = %s\n"),
			track[0].tracktype & TOC_MASK,
			toc2name[track[0].tracktype & TOC_MASK]);
	}

	if ((flags & (F_MSINFO|F_TOC|F_PRATIP|F_FIX|F_VERSION|F_CHECKDRIVE|F_INQUIRY|F_SCANBUS|F_RESET)) == 0) {
		/*
		 * Try to lock us im memory (will only work for root)
		 * but you need access to root anyway to send SCSI commands.
		 * We need to be root to open /dev/scg? or similar devices
		 * on other OS variants and we need to be root to be able
		 * to send SCSI commands at least on AIX and
		 * Solaris (USCSI only) regardless of the permissions for
		 * opening files
		 *
		 * XXX The folowing test used to be
		 * XXX #if defined(HAVE_MLOCKALL) || defined(_POSIX_MEMLOCK)
		 * XXX but the definition for _POSIX_MEMLOCK did change during
		 * XXX the last 8 years and the autoconf test is better for
		 * XXX the static case. sysconf() only makes sense if we like
		 * XXX to check dynamically.
		 */
		raise_memlock();
#if defined(HAVE_MLOCKALL)
		/*
		 * XXX mlockall() needs root privilleges.
		 */
		if (mlockall(MCL_CURRENT|MCL_FUTURE) < 0) {
			errmsg(_("WARNING: Cannot do mlockall(2).\n"));
			errmsgno(EX_BAD, _("WARNING: This causes a high risk for buffer underruns.\n"));
		}
#endif

		/*
		 * XXX raisepri() needs root privilleges.
		 */
		raisepri(0); /* max priority */
		/*
		 * XXX shmctl(id, SHM_LOCK, 0) needs root privilleges.
		 * XXX So if we use SysV shared memory, wee need to be root.
		 *
		 * Note that not being able to set up a FIFO bombs us
		 * back to the DOS ages. Trying to run cdrecord without
		 * root privillegs is extremely silly, it breaks most
		 * of the advanced features. We need to be at least installed
		 * suid root or called by RBACs pfexec.
		 */
		fs = init_fifo(fs); /* Attach shared memory (still one process) */
	}

	if ((flags & F_WAITI) != 0)
		wait_input();


	/*
	 * Call scg_remote() to force loading the remote SCSI transport library
	 * code that is located in librscg instead of the dummy remote routines
	 * that are located inside libscg.
	 */
	scg_remote();
	if (dev != NULL &&
	    ((strncmp(dev, "HELP", 4) == 0) ||
	    (strncmp(dev, "help", 4) == 0))) {
		scg_help(stderr);
		exit(0);
	}
	/*
	 * The following scg_open() call needs more privileges, so we check for
	 * sufficient privileges here.
	 * The check has been introduced as some Linux distributions miss the
	 * skills to perceive the necessity for the needed privileges. So we
	 * warn which features are impaired by actually missing privileges.
	 */
	if (!priv_eff_priv(SCHILY_PRIV_FILE_DAC_READ))
		priv_warn("file read", "You will not be able to open all needed devices.");
#ifndef	__SUNOS5
	/*
	 * Due to a design bug in the Solaris USCSI ioctl, we don't need
	 * PRIV_FILE_DAC_WRITE to send SCSI commands and most installations
	 * probably don't grant PRIV_FILE_DAC_WRITE. Once we need /dev/scg*,
	 * we would need to test for PRIV_FILE_DAC_WRITE also.
	 */
	if (!priv_eff_priv(SCHILY_PRIV_FILE_DAC_WRITE))
		priv_warn("file write", "You will not be able to open all needed devices.");
#endif
	if (!priv_eff_priv(SCHILY_PRIV_SYS_DEVICES))
		priv_warn("device",
		    "You may not be able to send all needed SCSI commands, this my cause various unexplainable problems.");
	if (!priv_eff_priv(SCHILY_PRIV_PROC_LOCK_MEMORY))
		priv_warn("memlock", "You may get buffer underruns.");
	if (!priv_eff_priv(SCHILY_PRIV_PROC_PRIOCNTL))
		priv_warn("priocntl", "You may get buffer underruns.");
	if (!priv_eff_priv(SCHILY_PRIV_NET_PRIVADDR))
		priv_warn("network", "You will not be able to do remote SCSI.");

	/*
	 * XXX scg_open() needs root privilleges.
	 */
	if ((scgp = scg_open(dev, errstr, sizeof (errstr),
				debug, (flags & F_MSINFO) == 0 || lverbose)) == (SCSI *)0) {
			scg_openerr(errstr);
			/* NOTREACHED */
	}

	/*
	 * Drop privs we do not need anymore.
	 * We no longer need:
	 *	file_dac_read,proc_lock_memory,proc_priocntl,net_privaddr
	 * We still need:
	 *	sys_devices
	 *
	 * If this is a suid-root process or if the real uid of
	 * this process is not root, we may have gained privileges
	 * from suid-root or pfexec and need to manage privileges in
	 * order to prevent privilege escalations for the user.
	 */
	if (issetuid || ouid != 0)
		priv_drop();
	/*
	 * This is only for OS that do not support fine grained privs.
	 *
	 * XXX Below this point we do not need root privilleges anymore.
	 */
	if (geteuid() != getuid()) {	/* AIX does not like to do this */
					/* If we are not root		*/
#ifdef	HAVE_SETREUID
		if (setreuid(-1, getuid()) < 0)
#else
#ifdef	HAVE_SETEUID
		if (seteuid(getuid()) < 0)
#else
		if (setuid(getuid()) < 0)
#endif
#endif
			comerr(_("Panic cannot set back effective uid.\n"));
	}
	/*
	 * WARNING: We now are no more able to do any privilleged operation
	 * unless we have been called by root.
	 *
	 * XXX It may be that we later get problems in init_faio() because
	 * XXX this function calls raisepri() to lower the priority slightly.
	 */
	scg_settimeout(scgp, timeout);
	if (scgopts) {
		i = scg_opts(scgp, scgopts);
		if (i <= 0)
			exit(i < 0 ? EX_BAD : 0);
	}
	scgp->flags |= SCGF_PERM_PRINT;
	scgp->verbose = scsi_verbose;
	scgp->silent = silent;
	scgp->debug = debug;
	scgp->kdebug = kdebug;
	scgp->cap->c_bsize = DATA_SEC_SIZE;


	if ((flags & F_MSINFO) == 0 || lverbose) {
		char	*vers;
		char	*auth;

		/*
		 * Warning: If you modify this section of code, you must
		 * change the name of the program.
		 */
		vers = scg_version(0, SCG_VERSION);
		auth = scg_version(0, SCG_AUTHOR);
		printf(_("Using libscg version '%s-%s'.\n"), auth, vers);
		if (auth == 0 || strcmp("schily", auth) != 0) {
			errmsgno(EX_BAD,
			_("Warning: using inofficial version of libscg (%s-%s '%s').\n"),
				auth, vers, scg_version(0, SCG_SCCS_ID));
		}

		vers = scg_version(scgp, SCG_VERSION);
		auth = scg_version(scgp, SCG_AUTHOR);
		if (lverbose > 1)
			error(_("Using libscg transport code version '%s-%s'\n"), auth, vers);
		if (auth == 0 || strcmp("schily", auth) != 0) {
			errmsgno(EX_BAD,
			_("Warning: using inofficial libscg transport code version (%s-%s '%s').\n"),
				auth, vers, scg_version(scgp, SCG_SCCS_ID));
		}

		vers = scg_version(scgp, SCG_RVERSION);
		auth = scg_version(scgp, SCG_RAUTHOR);
		if (lverbose > 1 && vers && auth)
			error(_("Using remote transport code version '%s-%s'\n"), auth, vers);
		if (auth != 0 && strcmp("schily", auth) != 0) {
			errmsgno(EX_BAD,
			_("Warning: using inofficial remote transport code version (%s-%s '%s').\n"),
				auth, vers, scg_version(scgp, SCG_RSCCS_ID));
		}
	}
	if (lverbose && driveropts)
		printf(_("Driveropts: '%s'\n"), driveropts);

/*	bufsize = scg_bufsize(scgp, CDR_BUF_SIZE);*/
	bufsize = scg_bufsize(scgp, bufsize);
	if (lverbose || debug)
		error(_("SCSI buffer size: %ld\n"), bufsize);
	if ((buf = scg_getbuf(scgp, bufsize)) == NULL)
		comerr(_("Cannot get SCSI I/O buffer.\n"));

	if ((flags & F_SCANBUS) != 0) {
		i = select_target(scgp, stdout);
		if (i < 0) {
			scg_openerr("");
			/* NOTREACHED */
		}
		exit(0);
	}
	if (scg_scsibus(scgp) < 0 &&
				scg_target(scgp) < 0 && scg_lun(scgp) < 0) {
		i = find_drive(scgp, dev, flags);
		if (i < 0) {
			scg_openerr("");
			/* NOTREACHED */
		}
	}
	if ((flags & F_RESET) != 0) {
		if (scg_reset(scgp, SCG_RESET_NOP) < 0)
			comerr(_("Cannot reset (OS does not implement reset).\n"));
		if (scg_reset(scgp, SCG_RESET_TGT) >= 0)
			exit(0);
		if (scg_reset(scgp, SCG_RESET_BUS) < 0)
			comerr(_("Cannot reset target.\n"));
		exit(0);
	}
	/*
	 * First try to check which type of SCSI device we
	 * have.
	 */
	if (debug || lverbose)
		printf("atapi: %d\n", scg_isatapi(scgp));
	seterrno(0);
	scgp->silent++;
	i = test_unit_ready(scgp);	/* eat up unit attention */
	scgp->silent--;
	if (i < 0) {
		i = geterrno();
		if (i == EPERM || i == EACCES) {
			scg_openerr("");
			/* NOTREACHED */
		}
	}
	if (!do_inquiry(scgp, (flags & F_MSINFO) == 0 || lverbose)) {
		errmsgno(EX_BAD, _("Cannot do inquiry for CD/DVD/BD-Recorder.\n"));
		if (unit_ready(scgp))
			errmsgno(EX_BAD, _("The unit seems to be hung and needs power cycling.\n"));
		exit(EX_BAD);
	}

	if ((flags & F_PRCAP) != 0) {
		print_capabilities(scgp);
		print_performance_mmc(scgp);
		print_capabilities_mmc4(scgp);
		if (get_curprofile(scgp) >= 0) {
			printf(_("\nSupported profiles according to MMC-4 feature list:\n"));
			print_profiles(scgp);
			printf(_("\nSupported features according to MMC-4 feature list:\n"));
			print_features(scgp);
		}
		exit(0);
	}
	if ((flags & F_INQUIRY) != 0)
		exit(0);

	/*
	 * Here we start to use drive specific commands and not generic SCSI
	 * anymore. We first get the drive specific driver via get_cdrcmds().
	 */

	if (dp == (cdr_t *)NULL) {	/* No driver= option specified	*/
		dp = get_cdrcmds(scgp);	/* Calls dp->cdr_identify()	*/
	} else if (!is_unknown_dev(scgp) && dp != get_cdrcmds(scgp)) {
		errmsgno(EX_BAD, _("WARNING: Trying to use other driver on known device.\n"));
	}

	if (!is_cddrive(scgp))
		comerrno(EX_BAD, _("Sorry, no CD/DVD/BD-Drive found on this target.\n"));
	if (dp == (cdr_t *)0)
		comerrno(EX_BAD, _("Sorry, no supported CD/DVD/BD-Recorder found on this target.\n"));
	/*
	 * The driver is known, set up data structures...
	 */
	{
		cdr_t	*ndp;
		dstat_t	*dsp;

		ndp = malloc(sizeof (cdr_t));
		dsp = malloc(sizeof (dstat_t));
		if (ndp == NULL || dsp == NULL)
			comerr(_("Cannot allocate memory for driver structure.\n"));
		movebytes(dp, ndp, sizeof (cdr_t));
		dp = ndp;
		dp->cdr_flags |= CDR_ALLOC;
		dp->cdr_cmdflags = flags;

		fillbytes(dsp, sizeof (*dsp), '\0');
		dsp->ds_trackp		= track;
		dsp->ds_minbuf		= 0xFFFF;
		dsp->ds_layer_break	= -1;
		dp->cdr_dstat		= dsp;
	}

	/*
	 * Reduce buffer size for older non-MMC drives.
	 * The Philips CDD-521 is known not to work with a DMA size > 63 kB.
	 */
	if (!is_mmc(scgp, NULL, NULL) && bufsize > CDR_OLD_BUF_SIZE)
		bufsize = CDR_OLD_BUF_SIZE;

	if ((flags & (F_MSINFO|F_TOC|F_LOAD|F_DLCK|F_EJECT)) == 0 ||
	    tracks > 0 ||
	    cuefilename != NULL) {


		if ((dp->cdr_flags & CDR_ISREADER) != 0) {
			errmsgno(EX_BAD,
			_("Sorry, no CD/DVD/BD-Recorder or unsupported CD/DVD/BD-Recorder found on this target.\n"));
			comexit(EX_BAD);
		}

	}

	/*
	 * Set up data structures for current drive state.
	 */
	if ((*dp->cdr_attach)(scgp, dp) != 0)
		comerrno(EX_BAD, _("Cannot attach driver for CD/DVD/BD-Recorder.\n"));

	if (lverbose > 1) {
		printf(_("Drive current speed: %d\n"), dp->cdr_dstat->ds_dr_cur_wspeed);
		printf(_("Drive default speed: %d\n"), dp->cdr_speeddef);
		printf(_("Drive max speed    : %d\n"), dp->cdr_speedmax);
	}
	if (speed > (int)dp->cdr_speedmax && (flags & F_FORCE) == 0)
		speed = dp->cdr_speedmax;
	if (speed < 0)
		speed = dp->cdr_speeddef;

	if (lverbose > 1) {
		printf(_("Selected speed     : %d\n"), speed);
	}
	dp->cdr_dstat->ds_wspeed = speed; /* XXX Remove 'speed' in future */

	exargs.scgp	   = scgp;
	exargs.dp	   = dp;
	exargs.old_secsize = -1;
	exargs.flags	   = flags;

	if ((flags & F_MSINFO) == 0 || lverbose) {
		printf(_("Using %s (%s).\n"), dp->cdr_drtext, dp->cdr_drname);
		print_drflags(dp);
		print_wrmodes(dp);
	}
	scgp->silent++;
	if ((debug || lverbose)) {
		long	physbufsize = -1;

		tsize = -1;
		(*dp->cdr_buffer_cap)(scgp, &tsize, (long *)0);

		fillbytes(buf, 4, '\0');
		if (read_buffer(scgp, buf, 4, 0) >= 0 &&
		    scg_getresid(scgp) == 0) {
			physbufsize = a_to_u_3_byte(&buf[1]);
		}
		if (tsize > 0) {
			printf(_("Drive buf size : %lu = %lu KB\n"),
						tsize, tsize >> 10);
		}
		if (physbufsize > 0 && physbufsize != tsize) {
			printf(_("Drive pbuf size: %lu = %lu KB\n"),
					physbufsize, physbufsize >> 10);
		}
	}
	scgp->silent--;

	dma_speed = get_dmaspeed(scgp, dp);
	if (dma_speed <= 0)
		errmsgno(EX_BAD, _("Warning: The DMA speed test has been skipped.\n"));
	if ((debug || lverbose) && dma_speed > 0) {
		/*
		 * We do not yet know what medium type is in...
		 */
		printf(_("Drive DMA Speed: %d kB/s %dx CD %dx DVD %dx BD\n"),
			dma_speed, dma_speed/176, dma_speed/1385,
			dma_speed/4495);
	}
	if ((tracks > 0 || cuefilename != NULL) && (debug || lverbose))
		printf(_("FIFO size      : %lu = %lu KB\n"), fs, fs >> 10);

#ifdef	HAVE_LIB_EDC_ECC
	if ((flags & F_RAW) != 0 && (dp->cdr_dstat->ds_flags & DSF_NOCD) == 0)
		raw_speed = encspeed(debug || lverbose);
#endif

	if ((flags & F_CHECKDRIVE) != 0)
		exit(0);

	if (dp->cdr_flags2 & CDR2_NOCD) {
		for (i = 0; i < MAX_TRACK+2; i++)
			track[i].flags |= TI_NOCD;

		if (flags & F_MULTI)  {
			if ((dp->cdr_flags & CDR_PACKET) == 0) {
				comerrno(EX_BAD,
				_("Drive does not support packet writing (needed for -multi).\n"));
			}
/*			flags &= F_TAO;*/
			flags |= F_SAO;
			dp->cdr_cmdflags = flags;
			for (i = 0; i < MAX_TRACK+2; i++) {
				track[i].flags &= ~TI_TAO;
				track[i].flags |= TI_SAO;
			}
		}
	}

	if ((flags & F_ABORT) != 0) {
		/*
		 * flush cache is not supported by CD-ROMs avoid prob with -toc
		 */
		scgp->silent++;
		scsi_flush_cache(scgp, FALSE);
		(*dp->cdr_abort_session)(scgp, dp);
		scgp->silent--;
		exit(0);
	}

	if (tracks == 0 && cuefilename == NULL &&
	    (flags & (F_FIX|F_BLANK)) == 0 && (flags & F_EJECT) != 0) {
		/*
		 * Do not check if the unit is ready here to allow to open
		 * an empty unit too.
		 */
		unload_media(scgp, dp, flags);
		exit(0);
	}
	flush();

	if (cuefilename) {
		parsecue(cuefilename, track);
		tracks = track[0].tracks;
	} else {
		opentracks(track);
		tracks = track[0].tracks;
	}

	if (tracks > 1)
		sleep(2);	/* Let the user watch the inquiry messages */

	if (tracks > 0 && !check_wrmode(dp, flags, track[1].flags))
		comerrno(EX_BAD, _("Illegal write mode for this drive.\n"));

	if ((track[0].flags & TI_TEXT) == 0 &&	/* CD-Text not yet processed */
	    (track[MAX_TRACK+1].flags & TI_TEXT) != 0) {
		/*
		 * CD-Text from textfile= or from CUE CDTEXTFILE will win
		 * over CD-Text from *.inf files and over CD-Text from
		 * CUE SONGWRITER, ...
		 */
		packtext(tracks, track);
		track[0].flags |= TI_TEXT;
	}
#ifdef	CLONE_WRITE
	if (flags & F_CLONE) {
		clone_toc(track);
		clone_tracktype(track);
	}
#endif
	setleadinout(tracks, track);
	set_trsizes(dp, tracks, track);
	setpregaps(tracks, track);
	checkfiles(tracks, track);
	tsize = checktsize(tracks, track);

	/*
	 * Make wm2name[wrmode] work.
	 * This must be done after the track flags have been set up
	 * by the functions above.
	 */
	if (tracks == 0 && (flags & F_BLANK) != 0)
		dp->cdr_dstat->ds_wrmode = WM_BLANK;
	else if (tracks == 0 && (flags & F_FORMAT) != 0)
		dp->cdr_dstat->ds_wrmode = WM_FORMAT;
	else
		set_wrmode(dp, flags, track[1].flags);

	/*
	 * Debug only
	 */
	{
		void	*cp = NULL;

		(*dp->cdr_gen_cue)(track, &cp, FALSE);
		if (cp)
			free(cp);
	}

	/*
	 * Create Lead-in data. Only needed in RAW mode.
	 */
	do_leadin(track);


	/*
	 * Install exit handler before we change the drive status.
	 */
	on_comerr(exscsi, &exargs);

	if ((flags & F_FORCE) == 0)
		load_media(scgp, dp, TRUE);

	if ((flags & (F_LOAD|F_DLCK)) != 0) {
		if ((flags & F_DLCK) == 0) {
			scgp->silent++;		/* silently		*/
			scsi_prevent_removal(
				scgp, 0);	/* allow manual open	*/
			scgp->silent--;		/* if load failed...	*/
		}
		exit(0);			/* we did not change status */
	}
	exargs.old_secsize = sense_secsize(scgp, 1);
	if (exargs.old_secsize < 0)
		exargs.old_secsize = sense_secsize(scgp, 0);
	if (debug)
		printf(_("Current Secsize: %d\n"), exargs.old_secsize);
	scgp->silent++;
	if (read_capacity(scgp) < 0) {
		if (exargs.old_secsize > 0)
			scgp->cap->c_bsize = exargs.old_secsize;
	}
	scgp->silent--;
	if (exargs.old_secsize < 0)
		exargs.old_secsize = scgp->cap->c_bsize;
	if (exargs.old_secsize != scgp->cap->c_bsize)
		errmsgno(EX_BAD, _("Warning: blockdesc secsize %d differs from cap secsize %d.\n"),
				exargs.old_secsize, scgp->cap->c_bsize);

	if (lverbose)
		printf(_("Current Secsize: %d\n"), exargs.old_secsize);

	if (exargs.old_secsize > 0 && exargs.old_secsize != DATA_SEC_SIZE) {
		/*
		 * Some drives (e.g. Plextor) don't like to write correctly
		 * in SAO mode if the sector size is set to 512 bytes.
		 * In addition, cdrecord -msinfo will not work properly
		 * if the sector size is not 2048 bytes.
		 */
		set_secsize(scgp, DATA_SEC_SIZE);
	}

	/*
	 * Is this the right place to do this ?
	 */
	check_recovery(scgp, dp, flags);

/*audioread(dp, flags);*/
/*unload_media(scgp, dp, flags);*/
/*return 0;*/
	if (flags & F_WRITE)
		dp->cdr_dstat->ds_cdrflags |= RF_WRITE;
	if (flags & F_BLANK)
		dp->cdr_dstat->ds_cdrflags |= RF_BLANK;
	if (flags & F_PRATIP || lverbose > 0) {
		dp->cdr_dstat->ds_cdrflags |= RF_PRATIP;
	}
	if (flags & F_IMMED || dminbuf > 0) {
		if (dminbuf <= 0)
			dminbuf = 50;
		if (lverbose <= 0)	/* XXX Hack needed for now */
			lverbose++;
		dp->cdr_dstat->ds_cdrflags |= RF_WR_WAIT;
	}
	if ((*dp->cdr_getdisktype)(scgp, dp) < 0) {
		errmsgno(EX_BAD, _("Cannot get disk type.\n"));
		if ((flags & F_FORCE) == 0)
			comexit(EX_BAD);
	}
	if (flags & F_PRATIP) {
		comexit(0);
	}
	if (cuefilename != 0 && (dp->cdr_dstat->ds_flags & DSF_NOCD) != 0)
		comerrno(EX_BAD, _("Wrong media, the cuefile= option only works with CDs.\n"));
	/*
	 * The next actions should depend on the disk type.
	 */
	if (dma_speed > 0) {
		if ((dp->cdr_dstat->ds_flags & DSF_BD) != 0)
			dma_speed /= 4495;
		else
		if ((dp->cdr_dstat->ds_flags & DSF_DVD) == 0)
			dma_speed /= 176;
		else
			dma_speed /= 1385;
	}

	/* BEGIN CSTYLED */
	/*
	 * Init drive to default modes:
	 *
	 * We set TAO unconditionally to make checkdsize() work
	 * currectly in SAO mode too.
	 *
	 * At least MMC drives will not return the next writable
	 * address we expect when the drive's write mode is set
	 * to SAO. We need this address for mkisofs and thus
	 * it must be the first user accessible sector and not the
	 * first sector of the pregap.
	 *
	 * XXX The ACER drive:
	 * XXX Vendor_info    : 'ATAPI   '
	 * XXX Identifikation : 'CD-R/RW 8X4X32  '
	 * XXX Revision       : '5.EW'
	 * XXX Will not return from -dummy to non-dummy without
	 * XXX opening the tray.
	 */
	/* END CSTYLED */
	scgp->silent++;
	if ((*dp->cdr_init)(scgp, dp) < 0)
		comerrno(EX_BAD, _("Cannot init drive.\n"));
	scgp->silent--;

	if (flags & F_SETDROPTS) {
		/*
		 * Note that the set speed function also contains
		 * drive option processing for speed related drive options.
		 */
		if ((*dp->cdr_opt1)(scgp, dp) < 0) {
			errmsgno(EX_BAD, _("Cannot set up 1st set of driver options.\n"));
		}
		if ((*dp->cdr_set_speed_dummy)(scgp, dp, &speed) < 0) {
			errmsgno(EX_BAD, _("Cannot set speed/dummy.\n"));
		}
		dp->cdr_dstat->ds_wspeed = speed; /* XXX Remove 'speed' in future */
		if ((*dp->cdr_opt2)(scgp, dp) < 0) {
			errmsgno(EX_BAD, _("Cannot set up 2nd set of driver options.\n"));
		}
		comexit(0);
	}
	/*
	 * XXX If dp->cdr_opt1() ever affects the result for
	 * XXX the multi session info we would need to move it here.
	 */
	if (flags & F_MEDIAINFO) {
		(*dp->cdr_prdiskstatus)(scgp, dp);
		comexit(0);
	}
	if (flags & F_MSINFO) {
		print_msinfo(scgp, dp);
		comexit(0);
	}
	if (flags & F_TOC) {
		print_toc(scgp, dp);
		comexit(0);
	}
	if ((flags & F_FORMAT) || (dp->cdr_dstat->ds_flags & DSF_NEED_FORMAT)) {
		int	omode = dp->cdr_dstat->ds_wrmode;

		dp->cdr_dstat->ds_wrmode = WM_FORMAT;
		if (lverbose) {
			printf(_("Format was %sneeded.\n"),
			(dp->cdr_dstat->ds_flags & DSF_NEED_FORMAT) ? "" : _("not "));
		}
		/*
		 * XXX Sollte hier (*dp->cdr_set_speed_dummy)() hin?
		 */
		if (gracewait(dp, &gracedone) < 0) {
			/*
			 * In case kill() did not work ;-)
			 */
			errs++;
			goto restore_it;
		}

		wait_unit_ready(scgp, 120);
		if (gettimeofday(&starttime, (struct timezone *)0) < 0)
			errmsg(_("Cannot get start time.\n"));

		if ((*dp->cdr_format)(scgp, dp, 0) < 0) {
			errmsgno(EX_BAD, _("Cannot format medium.\n"));
			comexit(EX_BAD);
		}

		if (gettimeofday(&fixtime, (struct timezone *)0) < 0)
			errmsg(_("Cannot get format time.\n"));
		if (lverbose)
			prtimediff(_("Formatting time: "), &starttime, &fixtime);

		if (!wait_unit_ready(scgp, 240) || tracks == 0) {
			comexit(0);
		}
		dp->cdr_dstat->ds_wrmode = omode;

		if (tracks > 0) {
			int	cdrflags = dp->cdr_dstat->ds_cdrflags;

			dp->cdr_dstat->ds_cdrflags &= ~RF_PRATIP;
			if ((*dp->cdr_getdisktype)(scgp, dp) < 0) {
				errmsgno(EX_BAD, _("Cannot get disk type.\n"));
				if ((flags & F_FORCE) == 0)
					comexit(EX_BAD);
			}
			dp->cdr_dstat->ds_cdrflags = cdrflags;
		}
	}

#ifdef	XXX
	if ((*dp->cdr_check_session)() < 0) {
		comexit(EX_BAD);
	}
#endif
	{
		Int32_t omb = dp->cdr_dstat->ds_maxblocks;

		if ((*dp->cdr_opt1)(scgp, dp) < 0) {
			errmsgno(EX_BAD, _("Cannot set up 1st set of driver options.\n"));
		}
		if (tsize > 0 && omb != dp->cdr_dstat->ds_maxblocks) {
			printf(_("Disk size changed by user options.\n"));
			printf(_("Checking disk capacity according to new values.\n"));
		}
	}
	if (tsize == 0) {
		if (tracks > 0) {
			errmsgno(EX_BAD,
			_("WARNING: Total disk size unknown. Data may not fit on disk.\n"));
		}
	} else if (tracks > 0) {
		/*
		 * XXX How do we let the user check the remaining
		 * XXX disk size witout starting the write process?
		 */
		if (((flags & F_BLANK) == 0) &&
		    !checkdsize(scgp, dp, tsize, flags))
			comexit(EX_BAD);
	}
	if (tracks > 0 && fs > 0L) {
#if defined(USE_POSIX_PRIORITY_SCHEDULING) && defined(HAVE_SETREUID)
		/*
		 * Hack to work around the POSIX design bug in real time
		 * priority handling: we need to be root even to lower
		 * our priority.
		 * Note that we need to find a more general way that works
		 * even on OS that do not support setreuid() which is *BSD
		 * and SUSv3 only.
		 */
		if (oeuid != getuid()) {
			if (setreuid(-1, oeuid) < 0)
				errmsg(_("Could set back effective uid.\n"));
		}
#endif
		/*
		 * Hack to support DVD+R/DL and the firmware problems
		 * for BD-R found in the Lite-ON BD B LH-2B1S/AL09
		 */
		if (get_mediatype(scgp) >= MT_DVD) {
			int	bls = get_blf(get_mediatype(scgp));
			long	nbs;

			bls *= 2048;			/* Count in bytes */
			nbs = bufsize / bls * bls;
			if (nbs == 0) {
				for (nbs = bls; nbs > 2048; nbs /= 2)
					if (nbs <= bufsize)
						break;
			}
			if (nbs != bufsize) {
				if (lverbose) {
					printf(
					_("Reducing transfer size from %ld to %ld bytes.\n"),
									bufsize, nbs);
				}
				bufsize = nbs;
				set_trsizes(dp, tracks, track);
			}
		}
		/*
		 * fork() here to start the extra process needed for
		 * improved buffering.
		 */
		if (!init_faio(track, bufsize))
			fs = 0L;
		else
			on_comerr(excdr, &exargs); /* Will be called first */

#if defined(USE_POSIX_PRIORITY_SCHEDULING) && defined(HAVE_SETREUID)
		/*
		 * XXX Below this point we never need root privilleges anymore.
		 */
		if (geteuid() != getuid()) {	/* AIX does not like to do this */
						/* If we are not root		*/
			if (setreuid(-1, getuid()) < 0)
				comerr(_("Panic cannot set back effective uid.\n"));
		}
#endif
	}
	if ((*dp->cdr_set_speed_dummy)(scgp, dp, &speed) < 0) {
		errmsgno(EX_BAD, _("Cannot set speed/dummy.\n"));
		if ((flags & F_FORCE) == 0)
			comexit(EX_BAD);
	}
	dp->cdr_dstat->ds_wspeed = speed; /* XXX Remove 'speed' in future */
	if ((flags & F_WRITE) != 0 && raw_speed >= 0) {
		int	max_raw = (flags & F_FORCE) != 0 ? raw_speed:raw_speed/2;

		if (getenv("CDR_FORCERAWSPEED")) {
			errmsgno(EX_BAD,
			_("WARNING: 'CDR_FORCERAWSPEED=' is set, buffer underruns may occur.\n"));
			max_raw = raw_speed;
		}

		for (i = 1; i <= MAX_TRACK; i++) {
			/*
			 * Check for Clone tracks
			 */
			if ((track[i].sectype & ST_MODE_RAW) != 0)
				continue;
			/*
			 * Check for non-data tracks
			 */
			if ((track[i].sectype & ST_MODE_MASK) == ST_MODE_AUDIO)
				continue;

			if (speed > max_raw) {
				errmsgno(EX_BAD,
				_("Processor too slow. Cannot write RAW data at speed %d.\n"),
				speed);
				comerrno(EX_BAD, _("Max RAW data speed on this processor is %d.\n"),
				max_raw);
			}
			break;
		}
	}
	if (tracks > 0 && (flags & F_WRITE) != 0 && dma_speed > 0) {
		int	max_dma = (flags & F_FORCE) != 0 ? dma_speed:(dma_speed+1)*4/5;
		char	*p = NULL;

		if ((p = getenv("CDR_FORCESPEED")) != NULL) {
			if ((dp->cdr_dstat->ds_cdrflags & RF_BURNFREE) == 0) {
				errmsgno(EX_BAD,
				_("WARNING: 'CDR_FORCESPEED=' is set.\n"));
				errmsgno(EX_BAD,
				_("WARNING: Use 'driveropts=burnfree' to avoid buffer underuns.\n"));
			}
			max_dma = dma_speed;
		}

		if (speed > max_dma) {
			errmsgno(EX_BAD,
			_("DMA speed too slow (OK for %dx). Cannot write at speed %dx.\n"),
					max_dma, speed);
			if ((dp->cdr_dstat->ds_cdrflags & RF_BURNFREE) == 0) {
				errmsgno(EX_BAD, _("Max DMA data speed is %d.\n"), max_dma);
				if (p == NULL || !streql(p, "any"))
					comerrno(EX_BAD, _("Try to use 'driveropts=burnfree'.\n"));
			}
		}
	}
	if ((flags & (F_WRITE|F_BLANK)) != 0 &&
				(dp->cdr_dstat->ds_flags & DSF_ERA) != 0) {
		if (xdebug) {
			printf(_("Current speed %d, medium low speed: %d medium high speed: %d\n"),
				speed,
				dp->cdr_dstat->ds_at_min_speed,
				dp->cdr_dstat->ds_at_max_speed);
		}
		if (dp->cdr_dstat->ds_at_max_speed > 0 &&
				speed <= 8 &&
				speed > (int)dp->cdr_dstat->ds_at_max_speed) {
			/*
			 * Be careful here: 10x media may be written faster.
			 * The current code will work as long as there is no
			 * writer that can only write faster than 8x
			 */
			if ((flags & F_FORCE) == 0) {
				errmsgno(EX_BAD,
				_("Write speed %d of medium not sufficient for this writer.\n"),
					dp->cdr_dstat->ds_at_max_speed);
				comerrno(EX_BAD,
				_("You may have used an ultra low speed medium on a high speed writer.\n"));
			}
		}

		if ((dp->cdr_dstat->ds_flags & DSF_ULTRASPP_ERA) != 0 &&
		    (speed < 16 || (dp->cdr_cdrw_support & CDR_CDRW_ULTRAP) == 0)) {
			if ((dp->cdr_cdrw_support & CDR_CDRW_ULTRAP) == 0) {
				comerrno(EX_BAD,
				/* CSTYLED */
				_("Trying to use ultra high speed+ medium on a writer which is not\ncompatible with ultra high speed+ media.\n"));
			} else if ((flags & F_FORCE) == 0) {
				comerrno(EX_BAD,
				_("Probably trying to use ultra high speed+ medium on improper writer.\n"));
			}
		} else if ((dp->cdr_dstat->ds_flags & DSF_ULTRASP_ERA) != 0 &&
		    (speed < 10 || (dp->cdr_cdrw_support & CDR_CDRW_ULTRA) == 0)) {
			if ((dp->cdr_cdrw_support & CDR_CDRW_ULTRA) == 0) {
				comerrno(EX_BAD,
				/* CSTYLED */
				_("Trying to use ultra high speed medium on a writer which is not\ncompatible with ultra high speed media.\n"));
			} else if ((flags & F_FORCE) == 0) {
				comerrno(EX_BAD,
				_("Probably trying to use ultra high speed medium on improper writer.\n"));
			}
		}
		if (dp->cdr_dstat->ds_at_min_speed >= 4 &&
				dp->cdr_dstat->ds_at_max_speed > 4 &&
				dp->cdr_dstat->ds_dr_max_wspeed <= 4) {
			if ((flags & F_FORCE) == 0) {
				comerrno(EX_BAD,
				_("Trying to use high speed medium on low speed writer.\n"));
			}
		}
		if ((int)dp->cdr_dstat->ds_at_min_speed > speed) {
			if ((flags & F_FORCE) == 0) {
				errmsgno(EX_BAD,
				_("Write speed %d of writer not sufficient for this medium.\n"),
					speed);
				errmsgno(EX_BAD,
				_("You did use a %s speed medium on an improper writer or\n"),
				dp->cdr_dstat->ds_flags & DSF_ULTRASP_ERA ?
				"ultra high": "high");
				comerrno(EX_BAD,
				_("you used a speed=# option with a speed too low for this medium.\n"));
			}
		}
	}
	if ((flags & (F_BLANK|F_FORCE)) == (F_BLANK|F_FORCE)) {
		/*
		 * This is a first "blind" blanking attempt.
		 */
		printf(_("Waiting for drive to calm down.\n"));
		wait_unit_ready(scgp, 120);
		if (gracewait(dp, &gracedone) < 0) {
			/*
			 * In case kill() did not work ;-)
			 */
			errs++;
			goto restore_it;
		}
		scsi_blank(scgp, 0L, blanktype, FALSE);
	}

	/*
	 * Last chance to quit!
	 */
	if (gracewait(dp, &gracedone) < 0) {
		/*
		 * In case kill() did not work ;-)
		 */
		errs++;
		goto restore_it;
	}
	if (tracks > 0 && fs > 0L) {
		/*
		 * Wait for the read-buffer to become full.
		 * This should be take no extra time if the input is a file.
		 * If the input is a pipe (e.g. mkisofs) this can take a
		 * while. If mkisofs dumps core before it starts writing,
		 * we abort before the writing process started.
		 */
		if (!await_faio()) {
			comerrno(EX_BAD, _("Input buffer error, aborting.\n"));
		}
	}
	wait_unit_ready(scgp, 120);

	starttime.tv_sec = 0;
	wstarttime.tv_sec = 0;
	stoptime.tv_sec = 0;
	fixtime.tv_sec = 0;
	if (gettimeofday(&starttime, (struct timezone *)0) < 0)
		errmsg(_("Cannot get start time.\n"));

	/*
	 * Blank the media if we were requested to do so
	 */
	if (flags & F_BLANK) {
		/*
		 * Do not abort if OPC failes. Just give it a chance
		 * for better laser power calibration than without OPC.
		 *
		 * Ricoh drives return with a vendor unique sense code.
		 * This is most likely because they refuse to do OPC
		 * on a non blank media.
		 */
		scgp->silent++;
		do_opc(scgp, dp, flags);
		scgp->silent--;
		wait_unit_ready(scgp, 120);
		if (gettimeofday(&starttime, (struct timezone *)0) < 0)
			errmsg(_("Cannot get start time.\n"));

		if ((*dp->cdr_blank)(scgp, dp, 0L, blanktype) < 0) {
			errmsgno(EX_BAD, _("Cannot blank disk, aborting.\n"));
			if (blanktype != BLANK_DISC) {
				errmsgno(EX_BAD, _("Some drives do not support all blank types.\n"));
				errmsgno(EX_BAD, _("Try again with cdrecord blank=all.\n"));
			}
			comexit(EX_BAD);
		}
		if (gettimeofday(&fixtime, (struct timezone *)0) < 0)
			errmsg(_("Cannot get blank time.\n"));
		if (lverbose)
			prtimediff(_("Blanking time: "), &starttime, &fixtime);

		/*
		 * XXX Erst blank und dann format?
		 * XXX Wenn ja, dann hier (flags & F_FORMAT) testen
		 */
		if (!wait_unit_ready(scgp, 240) || tracks == 0) {
			comexit(0);
		}
		if (tracks > 0) {
			int	cdrflags = dp->cdr_dstat->ds_cdrflags;

			dp->cdr_dstat->ds_cdrflags &= ~RF_PRATIP;
			if ((*dp->cdr_getdisktype)(scgp, dp) < 0) {
				errmsgno(EX_BAD, _("Cannot get disk type.\n"));
				if ((flags & F_FORCE) == 0)
					comexit(EX_BAD);
			}
			if (!checkdsize(scgp, dp, tsize, flags))
				comexit(EX_BAD);

			dp->cdr_dstat->ds_cdrflags = cdrflags;
		}
		/*
		 * Reset start time so we will not see blanking time and
		 * writing time counted together.
		 */
		if (gettimeofday(&starttime, (struct timezone *)0) < 0)
			errmsg(_("Cannot get start time.\n"));
	}
	if (tracks == 0 && (flags & F_FIX) == 0)
		comerrno(EX_BAD, _("No tracks found.\n"));

	/*
	 * Get the number of last recorded track by reading the CD TOC.
	 * As we start with track[0] (Lead In) and track[1] is the first
	 * recordable tack in track[i], we get the right track numbers.
	 * This will not allow to write DVDs with more than 100 sessions.
	 */
	scgp->silent++;
	if (read_tochdr(scgp, dp, NULL, &trackno) < 0) {
		trackno = 0;
	}
	scgp->silent--;
	if ((tracks + trackno) > MAX_TRACK) {
		/*
		 * XXX How many tracks are allowed on a DVD -> 1024
		 */
		comerrno(EX_BAD, _("Too many tracks for this disk, last track number is %d.\n"),
				tracks + trackno);
	}

	for (i = 0; i <= tracks+1; i++) {	/* Lead-in ... Lead-out */
		track[i].trackno = i + trackno;	/* Set up real track #	*/
		/*
		 * As long as we implement virtual tracks for DVDs/BDs
		 * and do not allow to write more than track at once,
		 * we simply set all "trackno" entries to the next trackno.
		 */
		if ((dp->cdr_flags2 & CDR2_NOCD) && i > 0)
			track[i].trackno = trackno + 1;
	}

	if ((*dp->cdr_opt2)(scgp, dp) < 0) {
		errmsgno(EX_BAD, _("Cannot set up 2nd set of driver options.\n"));
	}

	/*
	 * Now we actually start writing to the CD/DVD/BD.
	 * XXX Check total size of the tracks and remaining size of disk.
	 */
	if ((*dp->cdr_open_session)(scgp, dp, track) < 0) {
		comerrno(EX_BAD, _("Cannot open new session.\n"));
	}
	if (!do_opc(scgp, dp, flags))
		comexit(EX_BAD);

	/*
	 * As long as open_session() will do nothing but
	 * set up parameters, we may leave fix_it here.
	 * I case we have to add an open_session() for a drive
	 * that wants to do something that modifies the disk
	 * We have to think about a new solution.
	 */
	if (flags & F_FIX)
		goto fix_it;

	/*
	 * This call may modify trackp[i].trackstart for all tracks.
	 */
	if ((*dp->cdr_write_leadin)(scgp, dp, track) < 0)
		comerrno(EX_BAD, _("Could not write Lead-in.\n"));

	if (lverbose && (dp->cdr_dstat->ds_cdrflags & RF_LEADIN) != 0) {

		if (gettimeofday(&fixtime, (struct timezone *)0) < 0)
			errmsg(_("Cannot get lead-in write time.\n"));
		prtimediff(_("Lead-in write time: "), &starttime, &fixtime);
	}

	if (gettimeofday(&wstarttime, (struct timezone *)0) < 0)
		errmsg(_("Cannot get start time.\n"));
	for (i = 1; i <= tracks; i++) {
		startsec = 0L;

		if ((*dp->cdr_open_track)(scgp, dp, &track[i]) < 0) {
			errmsgno(EX_BAD, _("Cannot open next track.\n"));
			/*
			 * XXX We should try to avoid to fixate unwritten media
			 * XXX e.g. when opening track 1 fails, but then we
			 * XXX would need to keep track of whether the media
			 * XXX was written yet. This could be done in *dp.
			 */
			errs++;
			break;
		}

		/*
		 * Do not change the next writable address in DAO & RAW mode.
		 * XXX We should check whether this is still OK.
		 * Definitely get next writable address it in case of multi session.
		 */
		if ((flags & (F_SAO|F_RAW)) == 0 ||
		    ((dp->cdr_dstat->ds_flags & DSF_DVD) &&
		    (flags & F_MULTI) != 0)) {
			if ((*dp->cdr_next_wr_address)(scgp, &track[i], &startsec) < 0) {
				errmsgno(EX_BAD, _("Cannot get next writable address.\n"));
				errs++;
				break;
			}
			track[i].trackstart = startsec;
		}
		if (debug || lverbose) {
			if (track[i].trackno != track[i-1].trackno)
				printf(_("Starting new track at sector: %ld\n"),
						track[i].trackstart);
			else
				printf(_("Continuing track at sector: %ld\n"),
						track[i].trackstart);
			flush();
		}
		if (write_track_data(scgp, dp, &track[i]) < 0) {
			if (cdr_underrun(scgp)) {
				errmsgno(EX_BAD,
				_("The current problem looks like a buffer underrun.\n"));
				if ((dp->cdr_dstat->ds_cdrflags & RF_BURNFREE) == 0)
					errmsgno(EX_BAD,
				_("Try to use 'driveropts=burnfree'.\n"));
				else {
					errmsgno(EX_BAD,
				_("It looks like 'driveropts=burnfree' does not work for this drive.\n"));
					errmsgno(EX_BAD, _("Please report.\n"));
				}

				errmsgno(EX_BAD,
				_("Make sure that you are root, enable DMA and check your HW/OS set up.\n"));
			} else {
				errmsgno(EX_BAD, _("A write error occured.\n"));
				errmsgno(EX_BAD, _("Please properly read the error message above.\n"));
			}
			errs++;
			sleep(5);
			unit_ready(scgp);
			(*dp->cdr_close_track)(scgp, dp, &track[i]);
			break;
		}
		if ((*dp->cdr_close_track)(scgp, dp, &track[i]) < 0) {
			/*
			 * Check for "Dummy blocks added" message first.
			 */
			if (scg_sense_key(scgp) != SC_ILLEGAL_REQUEST ||
					scg_sense_code(scgp) != 0xB5) {
				errmsgno(EX_BAD, _("Cannot close track.\n"));
				errs++;
				break;
			}
		}
	}
fix_it:
	if (gettimeofday(&stoptime, (struct timezone *)0) < 0)
		errmsg(_("Cannot get stop time.\n"));
	cdrstats(dp);

	if (flags & F_RAW) {
		if (lverbose) {
			printf(_("Writing Leadout...\n"));
			flush();
		}
		write_leadout(scgp, dp, track);
	}
	if ((flags & F_NOFIX) == 0) {
		if (lverbose) {
			printf(_("Fixating...\n"));
			flush();
		}
		if ((*dp->cdr_fixate)(scgp, dp, track) < 0) {
			/*
			 * Ignore fixating errors in dummy mode.
			 */
			if ((flags & F_DUMMY) == 0) {
				errmsgno(EX_BAD, _("Cannot fixate disk.\n"));
				errs++;
			}
		}
		if (gettimeofday(&fixtime, (struct timezone *)0) < 0)
			errmsg(_("Cannot get fix time.\n"));
		if (lverbose)
			prtimediff(_("Fixating time: "), &stoptime, &fixtime);
	}
	if ((dp->cdr_dstat->ds_cdrflags & RF_DID_CDRSTAT) == 0) {
		dp->cdr_dstat->ds_cdrflags |= RF_DID_CDRSTAT;
		(*dp->cdr_stats)(scgp, dp);
	}
	if ((flags & (F_RAW|F_EJECT)) == F_RAW) {
		/*
		 * Most drives seem to forget to reread the TOC from disk
		 * if they are in RAW mode.
		 */
		scgp->silent++;
		if (read_tochdr(scgp, dp, NULL, NULL) < 0) {
			scgp->silent--;
			if ((flags & F_DUMMY) == 0)
				reload_media(scgp, dp);
		} else {
			scgp->silent--;
		}
	}

restore_it:
	/*
	 * Try to restore the old sector size and stop FIFO.
	 */
	comexit(errs?-2:0);
	return (0);
}

LOCAL void
scg_openerr(errstr)
	char	*errstr;
{
	errmsg(_("%s%sCannot open or use SCSI driver.\n"), errstr, errstr[0]?". ":"");
	errmsgno(EX_BAD, _("For possible targets try 'cdrecord -scanbus'.%s\n"),
				geteuid() ? _(" Make sure you are root."):"");
	errmsgno(EX_BAD, _("For possible transport specifiers try 'cdrecord dev=help'.\n"));
	exit(EX_BAD);
}

LOCAL int
find_drive(scgp, dev, flags)
	SCSI	*scgp;
	char	*dev;
	int	flags;
{
	int	ntarget;

	if ((flags & F_MSINFO) == 0)
		error(_("No target specified, trying to find one...\n"));
	ntarget = find_target(scgp, INQ_ROMD, -1);
	if (ntarget < 0)
		return (ntarget);
	if (ntarget == 1) {
		/*
		 * Simple case, exactly one CD-ROM found.
		 */
		find_target(scgp, INQ_ROMD, 1);
	} else if (ntarget <= 0 && (ntarget = find_target(scgp, INQ_WORM, -1)) == 1) {
		/*
		 * Exactly one CD-ROM acting as WORM found.
		 */
		find_target(scgp, INQ_WORM, 1);
	} else if (ntarget <= 0) {
		/*
		 * No single CD-ROM or WORM found.
		 */
		errmsgno(EX_BAD, _("No CD/DVD/BD-Recorder target found.\n"));
		errmsgno(EX_BAD, _("Your platform may not allow to scan for SCSI devices.\n"));
		comerrno(EX_BAD, _("Call 'cdrecord dev=help' or ask your sysadmin for possible targets.\n"));
	} else {
		errmsgno(EX_BAD, _("Too many CD/DVD/BD-Recorder targets found.\n"));
		select_target(scgp, stdout);
		comerrno(EX_BAD, _("Select a target from the list above and use 'cdrecord dev=%s%sb,t,l'.\n"),
			dev?dev:"", dev?(dev[strlen(dev)-1] == ':'?"":":"):"");
	}
	if ((flags & F_MSINFO) == 0)
		error(_("Using dev=%s%s%d,%d,%d.\n"),
			dev?dev:"", dev?(dev[strlen(dev)-1] == ':'?"":":"):"",
			scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp));

	return (ntarget);
}

LOCAL int
gracewait(dp, didgracep)
	cdr_t	*dp;
	BOOL	*didgracep;
{
	int	i;
	BOOL	didgrace = FALSE;

	if (didgracep)
		didgrace = *didgracep;

	if (gracetime < MIN_GRACE_TIME)
		gracetime = MIN_GRACE_TIME;
	if (gracetime > 999)
		gracetime = 999;

	printf(_("Starting to write CD/DVD/BD at speed %d in %s%s %s mode for %s session.\n"),
		(int)dp->cdr_dstat->ds_wspeed,
		(dp->cdr_cmdflags & F_DUMMY) ? _("dummy") : _("real"),
		(dp->cdr_cmdflags & F_FORCE) ? _(" force") : "",
		wm2name[dp->cdr_dstat->ds_wrmode],
		(dp->cdr_cmdflags & F_MULTI) ? _("multi") : _("single"));
	if (didgrace) {
		printf(_("No chance to quit anymore."));
		goto grace_done;
	}
	printf(_("Last chance to quit, starting %s write in %d seconds."),
		(dp->cdr_cmdflags & F_DUMMY)?_("dummy"):_("real"), gracetime);
	flush();
#ifdef	SIGINT
	signal(SIGINT, intr);
#endif
#ifdef	SIGHUP
	signal(SIGHUP, intr);
#endif
#ifdef	SIGTERM
	signal(SIGTERM, intr);
#endif
	/*
	 * Note to people who like to change this: I am geting patch requests
	 * for an option to reduce the grace_time two times a year. I am not
	 * willing to change things with respect to grace_time because it would
	 * not reduce the needed time in a significant amount and because it
	 * would break other things.
	 */

	for (i = gracetime; --i >= 0; ) {
		sleep(1);
		if (didintr) {
			printf("\n");
#ifdef	SIGINT
			excdr(SIGINT, &exargs);
			signal(SIGINT, SIG_DFL);
#ifdef	HAVE_KILL
			kill(getpid(), SIGINT);
#endif
#endif
			/*
			 * In case kill() did not work ;-)
			 */
			if (didgracep)
				*didgracep = FALSE;
			return (-1);
		}
		printf(_("\b\b\b\b\b\b\b\b\b\b\b\b\b%4d seconds."), i);
		flush();
	}
grace_done:
	printf(_(" Operation starts."));
	flush();
#ifdef	SIGINT
	signal(SIGINT, SIG_DFL);
#endif
#ifdef	SIGHUP
	signal(SIGHUP, SIG_DFL);
#endif
#ifdef	SIGTERM
	signal(SIGTERM, SIG_DFL);
#endif
#ifdef	SIGINT
	signal(SIGINT, intfifo);
#endif
#ifdef	SIGHUP
	signal(SIGHUP, intfifo);
#endif
#ifdef	SIGTERM
	signal(SIGTERM, intfifo);
#endif
	printf("\n");

	if (didgracep)
		*didgracep = TRUE;
	return (0);
}

LOCAL void
cdrstats(dp)
	cdr_t	*dp;
{
	float	secsps = 75.0;
	int	nsecs;
	float	fspeed;
	struct timeval	tcur;
	struct timeval	tlast;
	BOOL	nostop = FALSE;

	if (starttime.tv_sec == 0)
		return;

	if (stoptime.tv_sec == 0) {
		gettimeofday(&stoptime, (struct timezone *)0);
		nostop = TRUE;
	}

	if ((dp->cdr_dstat->ds_cdrflags & RF_DID_STAT) != 0)
		return;
	dp->cdr_dstat->ds_cdrflags |= RF_DID_STAT;

	if (lverbose == 0)
		return;

	if (dp->cdr_cmdflags & F_FIX)
		return;

	if ((dp->cdr_cmdflags & (F_WRITE|F_BLANK)) == F_BLANK)
		return;

	tlast = wstarttime;
	tcur = stoptime;

	prtimediff(_("Writing  time: "), &starttime, &stoptime);

	nsecs = dp->cdr_dstat->ds_endsec - dp->cdr_dstat->ds_startsec;

	if (dp->cdr_dstat->ds_flags & DSF_DVD)
		secsps = 676.27;
	if (dp->cdr_dstat->ds_flags & DSF_BD)
		secsps = 2195.07;

	tlast.tv_sec = tcur.tv_sec - tlast.tv_sec;
	tlast.tv_usec = tcur.tv_usec - tlast.tv_usec;
	while (tlast.tv_usec < 0) {
		tlast.tv_usec += 1000000;
		tlast.tv_sec -= 1;
	}
	if (!nostop && nsecs != 0 && dp->cdr_dstat->ds_endsec > 0) {
		/*
		 * May not be known (e.g. cdrecord -)
		 *
		 * XXX if we later allow this code to know how much has
		 * XXX actually been written, then we may remove the
		 * XXX dependance from nostop & nsecs != 0
		 */
		fspeed = (nsecs / secsps) /
			(tlast.tv_sec * 1.0 + tlast.tv_usec * 0.000001);
		if (fspeed > 999.0)
			fspeed = 999.0;
		printf(_("Average write speed %5.1fx.\n"), fspeed);
	}

	if (dp->cdr_dstat->ds_minbuf <= 100) {
		printf(_("Min drive buffer fill was %u%%\n"),
			(unsigned int)dp->cdr_dstat->ds_minbuf);
	}
	if (dp->cdr_dstat->ds_buflow > 0) {
		printf(_("Total of %ld possible drive buffer underruns predicted.\n"),
			(long)dp->cdr_dstat->ds_buflow);
	}
}

/*
 * Short usage
 */
LOCAL void
susage(ret)
	int	ret;
{
	error(_("Usage: %s [options] track1...trackn\n"), get_progname());
	error(_("\nUse\t%s -help\n"), get_progname());
	error(_("to get a list of valid options.\n"));
	error(_("\nUse\t%s blank=help\n"), get_progname());
	error(_("to get a list of valid blanking options.\n"));
	error(_("\nUse\t%s dev=b,t,l driveropts=help -checkdrive\n"), get_progname());
	error(_("to get a list of drive specific options.\n"));
	error(_("\nUse\t%s dev=help\n"), get_progname());
	error(_("to get a list of possible SCSI transport specifiers.\n"));
	exit(ret);
	/* NOTREACHED */
}

LOCAL void
usage(excode)
	int excode;
{
	error(_("Usage: %s [options] track1...trackn\n"), get_progname());
	error(_("Options:\n"));
	error(_("\t-version	print version information and exit\n"));
	error(_("\tdev=target	SCSI target to use as CD/DVD/BD-Recorder\n"));
	error(_("\tscgopts=spec	SCSI options for libscg\n"));
	error(_("\tgracetime=#	set the grace time before starting to write to #.\n"));
	error(_("\ttimeout=#	set the default SCSI command timeout to #.\n"));
	error(_("\tdebug=#,-d	Set to # or increment misc debug level\n"));
	error(_("\tkdebug=#,kd=#	do Kernel debugging\n"));
	error(_("\t-verbose,-v	increment general verbose level by one\n"));
	error(_("\t-Verbose,-V	increment SCSI command transport verbose level by one\n"));
	error(_("\t-silent,-s	do not print status of failed SCSI commands\n"));
	error(_("\tdriver=name	user supplied driver name, use with extreme care\n"));
	error(_("\tdriveropts=opt	a comma separated list of driver specific options\n"));
	error(_("\t-setdropts	set driver specific options and exit\n"));
	error(_("\t-checkdrive	check if a driver for the drive is present\n"));
	error(_("\t-prcap		print drive capabilities for MMC compliant drives\n"));
	error(_("\t-inq		do an inquiry for the drive and exit\n"));
	error(_("\t-scanbus	scan the SCSI bus and exit\n"));
	error(_("\t-reset		reset the SCSI bus with the cdrecorder (if possible)\n"));
	error(_("\t-abort		send an abort sequence to the drive (may help if hung)\n"));
	error(_("\t-overburn	allow to write more than the official size of a medium\n"));
	error(_("\t-ignsize	ignore the known size of a medium (may cause problems)\n"));
	error(_("\t-useinfo	use *.inf files to overwrite audio options.\n"));
	error(_("\tspeed=#		set speed of drive\n"));
	error(_("\tblank=type	blank a CD-RW disc (see blank=help)\n"));
	error(_("\t-format		format a CD-RW/DVD-RW/DVD+RW disc\n"));
#ifdef	FIFO
	error(_("\tfs=#		Set fifo size to # (0 to disable, default is %ld MB)\n"),
							DEFAULT_FIFOSIZE/(1024L*1024L));
#endif
	error(_("\tts=#		set maximum transfer size for a single SCSI command\n"));
	error(_("\t-load		load the disk and exit (works only with tray loader)\n"));
	error(_("\t-lock		load and lock the disk and exit (works only with tray loader)\n"));
	error(_("\t-eject		eject the disk after doing the work\n"));
	error(_("\t-dummy		do everything with laser turned off\n"));
	error(_("\t-minfo		retrieve and print media information/status\n"));
	error(_("\t-media-info	retrieve and print media information/status\n"));
	error(_("\t-msinfo		retrieve multi-session info for mkisofs >= 1.10\n"));
	error(_("\t-toc		retrieve and print TOC/PMA data\n"));
	error(_("\t-atip		retrieve and print ATIP data\n"));
	error(_("\t-multi		generate a TOC that allows multi session\n"));
	error(_("\t		In this case default track type is CD-ROM XA mode 2 form 1 - 2048 bytes\n"));
	error(_("\t-fix		fixate a corrupt or unfixated disk (generate a TOC)\n"));
	error(_("\t-nofix		do not fixate disk after writing tracks\n"));
	error(_("\t-waiti		wait until input is available before opening SCSI\n"));
	error(_("\t-immed		Try to use the SCSI IMMED flag with certain long lasting commands\n"));
	error(_("\t-force		force to continue on some errors to allow blanking bad disks\n"));
	error(_("\t-tao		Write disk in TAO mode. This option will be replaced in the future.\n"));
	error(_("\t-dao		Write disk in SAO mode. This option will be replaced in the future.\n"));
	error(_("\t-sao		Write disk in SAO mode. This option will be replaced in the future.\n"));
	error(_("\t-raw		Write disk in RAW mode. This option will be replaced in the future.\n"));
	error(_("\t-raw96r		Write disk in RAW/RAW96R mode. This option will be replaced in the future.\n"));
	error(_("\t-raw96p		Write disk in RAW/RAW96P mode. This option will be replaced in the future.\n"));
	error(_("\t-raw16		Write disk in RAW/RAW16 mode. This option will be replaced in the future.\n"));
#ifdef	CLONE_WRITE
	error(_("\t-clone		Write disk in clone write mode.\n"));
#endif
	error(_("\ttsize=#		Length of valid data in next track\n"));
	error(_("\tpadsize=#	Amount of padding for next track\n"));
	error(_("\tpregap=#	Amount of pre-gap sectors before next track\n"));
	error(_("\tdefpregap=#	Amount of pre-gap sectors for all but track #1\n"));
	error(_("\tmcn=text	Set the media catalog number for this CD to 'text'\n"));
	error(_("\tisrc=text	Set the ISRC number for the next track to 'text'\n"));
	error(_("\tindex=list	Set the index list for the next track to 'list'\n"));
	error(_("\t-text		Write CD-Text from information from *.inf or *.cue files\n"));
	error(_("\ttextfile=name	Set the file with CD-Text data to 'name'\n"));
	error(_("\tcuefile=name	Set the file with CDRWIN CUE data to 'name'\n"));

	error(_("\t-audio		Subsequent tracks are CD-DA audio tracks\n"));
	error(_("\t-data		Subsequent tracks are CD-ROM data mode 1 - 2048 bytes (default)\n"));
	error(_("\t-mode2		Subsequent tracks are CD-ROM data mode 2 - 2336 bytes\n"));
	error(_("\t-xa		Subsequent tracks are CD-ROM XA mode 2 form 1 - 2048 bytes\n"));
	error(_("\t-xa1		Subsequent tracks are CD-ROM XA mode 2 form 1 - 2056 bytes\n"));
	error(_("\t-xa2		Subsequent tracks are CD-ROM XA mode 2 form 2 - 2324 bytes\n"));
	error(_("\t-xamix		Subsequent tracks are CD-ROM XA mode 2 form 1/2 - 2332 bytes\n"));
	error(_("\t-cdi		Subsequent tracks are CDI tracks\n"));
	error(_("\t-isosize	Use iso9660 file system size for next data track\n"));
									/* micro-s */
	error(_("\t-preemp		Audio tracks are mastered with 50/15 us preemphasis\n"));
	error(_("\t-nopreemp	Audio tracks are mastered with no preemphasis (default)\n"));
	error(_("\t-copy		Audio tracks have unlimited copy permission\n"));
	error(_("\t-nocopy		Audio tracks may only be copied once for personal use (default)\n"));
	error(_("\t-scms		Audio tracks will not have any copy permission at all\n"));
	error(_("\t-pad		Pad data tracks with %d zeroed sectors\n"), PAD_SECS);
	error(_("\t		Pad audio tracks to a multiple of %d bytes\n"), AUDIO_SEC_SIZE);
	error(_("\t-nopad		Do not pad data tracks (default)\n"));
	error(_("\t-shorttrack	Subsequent tracks may be non Red Book < 4 seconds if in SAO or RAW mode\n"));
	error(_("\t-noshorttrack	Subsequent tracks must be >= 4 seconds\n"));
	error(_("\t-swab		Audio data source is byte-swapped (little-endian/Intel)\n"));
	error(_("The type of the first track is used for the toc type.\n"));
	error(_("Currently only form 1 tracks are supported.\n"));
	exit(excode);
}

LOCAL void
blusage(ret)
	int	ret;
{
	error(_("Blanking options:\n"));
	error(_("\tall\t\tblank the entire disk\n"));
	error(_("\tdisc\t\tblank the entire disk\n"));
	error(_("\tdisk\t\tblank the entire disk\n"));
	error(_("\tfast\t\tminimally blank the entire disk (PMA, TOC, pregap)\n"));
	error(_("\tminimal\t\tminimally blank the entire disk (PMA, TOC, pregap)\n"));
	error(_("\ttrack\t\tblank a track\n"));
	error(_("\tunreserve\tunreserve a track\n"));
	error(_("\ttrtail\t\tblank a track tail\n"));
	error(_("\tunclose\t\tunclose last session\n"));
	error(_("\tsession\t\tblank last session\n"));

	exit(ret);
	/* NOTREACHED */
}

/* ARGSUSED */
LOCAL void
intr(sig)
	int	sig;
{
	sig = 0;	/* Fake usage for gcc */

#ifdef SIGINT
	signal(SIGINT, intr);
#endif
	didintr++;
}

LOCAL void
catchsig(sig)
	int	sig;
{
#ifdef	HAVE_SIGNAL
	signal(sig, catchsig);
#endif
}

LOCAL int
scsi_cb(arg)
	void	*arg;
{
	comexit(EX_BAD);
	/* NOTREACHED */
	return (0);	/* Keep lint happy */
}

LOCAL void
intfifo(sig)
	int	sig;
{
	errmsgno(EX_BAD, _("Caught interrupt.\n"));
	if (exargs.scgp) {
		SCSI	*scgp = exargs.scgp;

		if (scgp->running) {
			if (scgp->cb_fun != NULL) {
				comerrno(EX_BAD, _("Second interrupt. Doing hard abort.\n"));
				/* NOTREACHED */
			}
			scgp->cb_fun = scsi_cb;
			scgp->cb_arg = &exargs;
			return;
		}
	}
	comexit(sig);
}

/*
 * Restore SCSI drive status.
 */
/* ARGSUSED */
LOCAL void
exscsi(excode, arg)
	int	excode;
	void	*arg;
{
	struct exargs	*exp = (struct exargs *)arg;

	/*
	 * Try to restore the old sector size.
	 */
	if (exp != NULL && exp->exflags == 0) {
		if (exp->scgp == NULL) {	/* Closed before */
			return;
		}
		if (exp->scgp->running) {
			return;
		}
		/*
		 * flush cache is not supported by CD-ROMs avoid prob with -toc
		 */
		exp->scgp->silent++;
		scsi_flush_cache(exp->scgp, FALSE);
		(*exp->dp->cdr_abort_session)(exp->scgp, exp->dp);
		exp->scgp->silent--;
		set_secsize(exp->scgp, exp->old_secsize);
		unload_media(exp->scgp, exp->dp, exp->flags);

		exp->exflags++;	/* Make sure that it only get called once */
	}
}

/*
 * excdr() is installed last with on_comerr() and thus will be called first.
 * We call exscsi() from here to control the order of calls.
 */
LOCAL void
excdr(excode, arg)
	int	excode;
	void	*arg;
{
	struct exargs	*exp = (struct exargs *)arg;

	exscsi(excode, arg);	/* Restore/reset drive status */

	cdrstats(exp->dp);
	if ((exp->dp->cdr_dstat->ds_cdrflags & RF_DID_CDRSTAT) == 0) {
		exp->dp->cdr_dstat->ds_cdrflags |= RF_DID_CDRSTAT;
		if (exp->scgp && exp->scgp->running == 0)
			(*exp->dp->cdr_stats)(exp->scgp, exp->dp);
	}

	/*
	 * We no longer need SCSI, close it.
	 */
#ifdef	SCG_CLOSE_DEBUG
	error("scg_close(%p)\n", exp->scgp);
#endif
	if (exp->scgp)
		scg_close(exp->scgp);
	exp->scgp = NULL;

#ifdef	FIFO
	kill_faio();
	wait_faio();
	if (debug || lverbose)
		fifo_stats();
#endif
}

EXPORT int
read_buf(f, bp, size)
	int	f;
	char	*bp;
	int	size;
{
	char	*p = bp;
	int	amount = 0;
	int	n;

	if (isobsize > 0) {
		if (size <= isobsize) {
			movebytes(&isobuf[isoboff], bp, size);
			isoboff += size;
			isobsize -= size;
			return (size);
		} else {
			amount = isobsize;
			movebytes(&isobuf[isoboff], bp, isobsize);
			isoboff += isobsize;
			isobsize = 0;
		}
	}
	do {
		do {
			n = read(f, p, size-amount);
		} while (n < 0 && (geterrno() == EAGAIN || geterrno() == EINTR));
		if (n < 0)
			return (n);
		amount += n;
		p += n;

	} while (amount < size && n > 0);
	return (amount);
}

EXPORT int
fill_buf(f, trackp, secno, bp, size)
	int	f;
	track_t	*trackp;
	long	secno;
	char	*bp;
	int	size;
{
	int	amount = 0;
	int	nsecs;
	int	rsize;
	int	rmod;
	int	readoffset = 0;

	nsecs = size / trackp->secsize;
	if (nsecs < trackp->secspt) {
		/*
		 * Clear buffer to prepare for last transfer.
		 * Make sure that a partial sector ends with NULs
		 */
		fillbytes(bp, trackp->secspt * trackp->secsize, '\0');
	}

	if (!is_raw(trackp)) {
		amount = read_buf(f, bp, size);
		if (amount != size) {
			if (amount < 0)
				return (amount);
			/*
			 * We got less than expected, clear rest of buf.
			 */
			fillbytes(&bp[amount], size-amount, '\0');
		}
		if (is_swab(trackp))
			swabbytes(bp, amount);
		return (amount);
	}

	rsize = nsecs * trackp->isecsize;
	rmod  = size % trackp->secsize;
	if (rmod > 0) {
		rsize += rmod;
		nsecs++;
	}

	readoffset = trackp->dataoff;
	amount = read_buf(f, bp + readoffset, rsize);
	if (is_swab(trackp))
		swabbytes(bp + readoffset, amount);

	if (trackp->isecsize == 2448 && trackp->secsize == 2368)
		subrecodesecs(trackp, (Uchar *)bp, secno, nsecs);

	scatter_secs(trackp, bp + readoffset, nsecs);

	if (amount != rsize) {
		if (amount < 0)
			return (amount);
		/*
		 * We got less than expected, clear rest of buf.
		 */
		fillbytes(&bp[amount], rsize-amount, '\0');
		nsecs = amount / trackp->isecsize;
		rmod  = amount % trackp->isecsize;
		amount = nsecs * trackp->secsize;
		if (rmod > 0) {
			nsecs++;
			amount += rmod;
		}
	} else {
		amount = size;
	}
	if ((trackp->sectype & ST_MODE_RAW) == 0) {
		encsectors(trackp, (Uchar *)bp, secno, nsecs);
		fillsubch(trackp, (Uchar *)bp, secno, nsecs);
	} else {
		scrsectors(trackp, (Uchar *)bp, secno, nsecs);
		/*
		 * If we are in cuefile= mode with FILE type BINARY we need to
		 * take care as CUE files only know about a 2352 byte RAW mode.
		 * We need to add the missing subchannel data now in this case.
		 */
		if (trackp->isecsize == 2352)
			fillsubch(trackp, (Uchar *)bp, secno, nsecs);
	}
	return (amount);
}

EXPORT int
get_buf(f, trackp, secno, bpp, size)
	int	f;
	track_t	*trackp;
	long	secno;
	char	**bpp;
	int	size;
{
	if (fs > 0) {
/*		return (faio_read_buf(f, *bpp, size));*/
		return (faio_get_buf(f, bpp, size));
	} else {
		return (fill_buf(f, trackp, secno, *bpp, size));
	}
}

EXPORT int
write_secs(scgp, dp, bp, startsec, bytespt, secspt, islast)
	SCSI	*scgp;
	cdr_t	*dp;
	char	*bp;
	long	startsec;
	int	bytespt;
	int	secspt;
	BOOL	islast;
{
	int	amount;

again:
	scgp->silent++;
	amount = (*dp->cdr_write_trackdata)(scgp, bp, startsec, bytespt, secspt, islast);
	scgp->silent--;
	if (amount < 0) {
		if (scsi_in_progress(scgp)) {
			/*
			 * If we sleep too long, the drive buffer is empty
			 * before we start filling it again. The max. CD speed
			 * is ~ 10 MB/s (52x RAW writing). The max. DVD speed
			 * is ~ 28 MB/s (20x DVD 1385 kB/s).
			 * With 10 MB/s, a 1 MB buffer empties within 100ms.
			 * With 28 MB/s, a 1 MB buffer empties within 37ms.
			 */
			if ((dp->cdr_dstat->ds_flags & DSF_NOCD) == 0) {
				usleep(60000);	/* CD case */
			} else {
#ifndef	_SC_CLK_TCK
				usleep(20000);	/* DVD/BD case */
#else
				if (sysconf(_SC_CLK_TCK) < 100)
					usleep(20000);
				else
					usleep(10000);

#endif
			}
			goto again;
		}
		return (-1);
	}
	return (amount);
}

EXPORT int
write_track_data(scgp, dp, trackp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
{
	int	track = trackp->trackno;
	int	f = -1;
	int	isaudio;
	long	startsec;
	Llong	bytes_read = 0;
	Llong	bytes	= 0;
	Llong	savbytes = 0;
	int	count;
	Llong	tracksize;
	int	secsize;
	int	secspt;
	int	bytespt;
	int	bytes_to_read;
	long	amount;
	int	pad;
	BOOL	neednl	= FALSE;
	BOOL	islast	= FALSE;
	char	*bp	= buf;
	struct timeval tlast;
	struct timeval tcur;
	float	secsps = 75.0;
long bsize;
long bfree;
#define	BCAP
#ifdef	BCAP
int per = 0;
#ifdef	XBCAP
int oper = -1;
#endif
#endif

	if (dp->cdr_dstat->ds_flags & DSF_DVD)
		secsps = 676.27;
	if (dp->cdr_dstat->ds_flags & DSF_BD)
		secsps = 2195.07;

	scgp->silent++;
	if ((*dp->cdr_buffer_cap)(scgp, &bsize, &bfree) < 0)
		bsize = -1L;
	if (bsize == 0)		/* If we have no (known) buffer, we cannot */
		bsize = -1L;	/* retrieve the buffer fill ratio	   */
	scgp->silent--;


	if (is_packet(trackp))	/* XXX Ugly hack for now */
		return (write_packet_data(scgp, dp, trackp));

	if (trackp->xfp != NULL)
		f = xfileno(trackp->xfp);

	isaudio = is_audio(trackp);
	tracksize = trackp->tracksize;
	startsec = trackp->trackstart;

	secsize = trackp->secsize;
	secspt = trackp->secspt;
	bytespt = secsize * secspt;

	pad = !isaudio && is_pad(trackp);	/* Pad only data tracks */

	if (debug) {
		printf(_("secsize:%d secspt:%d bytespt:%d audio:%d pad:%d\n"),
			secsize, secspt, bytespt, isaudio, pad);
	}

	if (lverbose) {
		if (tracksize > 0)
			printf(_("\rTrack %02d:    0 of %4lld MB written."),
				track, tracksize >> 20);
		else
			printf(_("\rTrack %02d:    0 MB written."), track);
		flush();
		neednl = TRUE;
	}

	gettimeofday(&tlast, (struct timezone *)0);
	do {
		bytes_to_read = bytespt;
		if (tracksize > 0) {
			if ((tracksize - bytes_read) > bytespt)
				bytes_to_read = bytespt;
			else
				bytes_to_read = tracksize - bytes_read;
		}
		count = get_buf(f, trackp, startsec, &bp, bytes_to_read);

		if (count < 0)
			comerr(_("Read error on input file.\n"));
		if (count == 0)
			break;
		bytes_read += count;
		if (tracksize >= 0 && bytes_read >= tracksize) {
			count -= bytes_read - tracksize;
			/*
			 * Paranoia: tracksize is known (trackp->tracksize >= 0)
			 * At this point, trackp->padsize should alway be set
			 * if the tracksize is less than 300 sectors.
			 */
			if (trackp->padsecs == 0 &&
			    (is_shorttrk(trackp) || (bytes_read/secsize) >= 300))
				islast = TRUE;
		}

		if (count < bytespt) {
			if (debug) {
				printf(_("\nNOTICE: reducing block size for last record.\n"));
				neednl = FALSE;
			}

			if ((amount = count % secsize) != 0) {
				amount = secsize - amount;
				count += amount;
				printf(
				_("\nWARNING: padding up to secsize (by %ld bytes).\n"),
					amount);
				neednl = FALSE;
			}
			bytespt = count;
			secspt = count / secsize;
			/*
			 * If tracksize is not known (trackp->tracksize < 0)
			 * we may need to set trackp->padsize
			 * if the tracksize is less than 300 sectors.
			 */
			if (trackp->padsecs == 0 &&
			    (is_shorttrk(trackp) || (bytes_read/secsize) >= 300))
				islast = TRUE;
		}

		amount = write_secs(scgp, dp, bp, startsec, bytespt, secspt, islast);
		if (amount < 0) {
			printf(_("%swrite track data: error after %lld bytes\n"),
							neednl?"\n":"", bytes);
			return (-1);
		}
		bytes += amount;
		startsec += amount / secsize;

		if (lverbose && (bytes >= (savbytes + 0x100000))) {
			int	fper;
			int	nsecs = (bytes - savbytes) / secsize;
			float	fspeed;

			gettimeofday(&tcur, (struct timezone *)0);
			printf(_("\rTrack %02d: %4lld"), track, bytes >> 20);
			if (tracksize > 0)
				printf(_(" of %4lld MB"), tracksize >> 20);
			else
				printf(" MB");
			printf(_(" written"));
			fper = fifo_percent(TRUE);
			if (fper >= 0)
				printf(_(" (fifo %3d%%)"), fper);
#ifdef	BCAP
			/*
			 * Work around a bug in the firmware from drives
			 * developed by PIONEER in November 2009. This affects
			 * drives labelled "Pioneer", "Plextor" and "TEAC".
			 * Do no longer call cdr_buffer_cap() before the drive
			 * buffer was not at least filled once to avoid that
			 * the the drive throughs away all data.
			 */
			if (bsize > 0 && bytes > bsize) { /* buffer size known */
				scgp->silent++;
				per = (*dp->cdr_buffer_cap)(scgp, (long *)0, &bfree);
				scgp->silent--;
				if (per >= 0) {
					per = 100*(bsize - bfree) / bsize;
					if ((bsize - bfree) <= amount || per <= 5)
						dp->cdr_dstat->ds_buflow++;
					if (per < (int)dp->cdr_dstat->ds_minbuf &&
					    (startsec*secsize) > bsize) {
						dp->cdr_dstat->ds_minbuf = per;
					}
					printf(_(" [buf %3d%%]"), per);
#ifdef	BCAPDBG
					printf(" %3ld %3ld", bsize >> 10, bfree >> 10);
#endif
				}
			}
#endif

			tlast.tv_sec = tcur.tv_sec - tlast.tv_sec;
			tlast.tv_usec = tcur.tv_usec - tlast.tv_usec;
			while (tlast.tv_usec < 0) {
				tlast.tv_usec += 1000000;
				tlast.tv_sec -= 1;
			}
			fspeed = (nsecs / secsps) /
				(tlast.tv_sec * 1.0 + tlast.tv_usec * 0.000001);
			if (fspeed > 999.0)
				fspeed = 999.0;
#ifdef	BCAP
			if (bsize > 0 && per > dminbuf &&
			    dp->cdr_dstat->ds_cdrflags & RF_WR_WAIT) {
				int	wsecs = (per-dminbuf)*(bsize/secsize)/100;
				int	msecs = 0x100000/secsize;
				int	wt;
				int	mt;
				int	s = dp->cdr_dstat->ds_dr_cur_wspeed;


				if (s <= 0) {
					if (dp->cdr_dstat->ds_flags & DSF_NOCD)
						s = 4;
					else
						s = 50;
				}
				if (wsecs > msecs)	/* Less that 1 MB */
					wsecs = msecs;
				wt = wsecs * 1000 / secsps / fspeed;
				mt = (per-dminbuf)*(bsize/secsize)/100 * 1000 / secsps/s;

				if (wt > mt)
					wt = mt;
				if (wt > 1000)		/* Max 1 second */
					wt = 1000;
				if (wt < 20)		/* Min 20 ms */
					wt = 0;

				if (xdebug)
					printf(_(" |%3d %4dms %5dms|"), wsecs, wt, mt);
				else
					printf(_(" |%3d %4dms|"), wsecs, wt);
				if (wt > 0)
					usleep(wt*1000);
			}
#endif
			printf(" %5.1fx", fspeed);
			printf(".");
			savbytes = (bytes >> 20) << 20;
			flush();
			neednl = TRUE;
			tlast = tcur;
		}
#ifdef	XBCAP
		if (bsize > 0) {			/* buffer size known */
			(*dp->cdr_buffer_cap)(scgp, (long *)0, &bfree);
			per = 100*(bsize - bfree) / bsize;
			if (per != oper)
				printf("[buf %3d%%] %3ld %3ld\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b",
					per, bsize >> 10, bfree >> 10);
			oper = per;
			flush();
		}
#endif
	} while (tracksize < 0 || bytes_read < tracksize);

	if (!is_shorttrk(trackp) && (bytes / secsize) < 300) {
		/*
		 * If tracksize is not known (trackp->tracksize < 0) or
		 * for some strange reason we did not set padsecs properly
		 * we may need to modify trackp->padsecs if
		 * tracksize+padsecs is less than 300 sectors.
		 */
		if ((trackp->padsecs + (bytes / secsize)) < 300)
			trackp->padsecs = 300 - (bytes / secsize);
	}
	if (trackp->padsecs > 0) {
		Llong	padbytes;

		/*
		 * pad_track() is based on secsize. Compute the amount of bytes
		 * assumed by pad_track().
		 */
		padbytes = (Llong)trackp->padsecs * secsize;

		if (neednl) {
			printf("\n");
			neednl = FALSE;
		}
		if ((padbytes >> 20) > 0) {
			neednl = TRUE;
		} else if (lverbose) {
			printf(_("Track %02d: writing %3lld KB of pad data.\n"),
					track, (Llong)(padbytes >> 10));
			neednl = FALSE;
		}
		pad_track(scgp, dp, trackp, startsec, padbytes,
					TRUE, &savbytes);
		bytes += savbytes;
		startsec += savbytes / secsize;
	}
	printf(_("%sTrack %02d: Total bytes read/written: %lld/%lld (%lld sectors).\n"),
		neednl?"\n":"", track, bytes_read, bytes, bytes/secsize);
	flush();
	return (0);
}

EXPORT int
pad_track(scgp, dp, trackp, startsec, amt, dolast, bytesp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
	long	startsec;
	Llong	amt;
	BOOL	dolast;
	Llong	*bytesp;
{
	int	track = trackp->trackno;
	Llong	bytes	= 0;
	Llong	savbytes = 0;
	Llong	padsize = amt;
	int	secsize;
	int	secspt;
	int	bytespt;
	int	amount;
	BOOL	neednl	= FALSE;
	BOOL	islast	= FALSE;
	struct timeval tlast;
	struct timeval tcur;
	float	secsps = 75.0;
long bsize;
long bfree;
#define	BCAP
#ifdef	BCAP
int per;
#ifdef	XBCAP
int oper = -1;
#endif
#endif

	if (dp->cdr_dstat->ds_flags & DSF_DVD)
		secsps = 676.27;
	if (dp->cdr_dstat->ds_flags & DSF_BD)
		secsps = 2195.07;

	scgp->silent++;
	if ((*dp->cdr_buffer_cap)(scgp, &bsize, &bfree) < 0)
		bsize = -1L;
	if (bsize == 0)		/* If we have no (known) buffer, we cannot */
		bsize = -1L;	/* retrieve the buffer fill ratio	   */
	scgp->silent--;

	secsize = trackp->secsize;
	secspt = trackp->secspt;
	bytespt = secsize * secspt;

	fillbytes(buf, bytespt, '\0');

	if ((amt >> 20) > 0) {
		printf(_("\rTrack %02d:    0 of %4lld MB pad written."),
						track, amt >> 20);
		flush();
	}
	gettimeofday(&tlast, (struct timezone *)0);
	do {
		if (amt < bytespt) {
			bytespt = roundup(amt, secsize);
			secspt = bytespt / secsize;
		}
		if (dolast && (amt - bytespt) <= 0)
			islast = TRUE;

		if (is_raw(trackp)) {
			encsectors(trackp, (Uchar *)buf, startsec, secspt);
			fillsubch(trackp, (Uchar *)buf, startsec, secspt);
		}

		amount = write_secs(scgp, dp, buf, startsec, bytespt, secspt, islast);
		if (amount < 0) {
			printf(_("%swrite track pad data: error after %lld bytes\n"),
							neednl?"\n":"", bytes);
			if (bytesp)
				*bytesp = bytes;
(*dp->cdr_buffer_cap)(scgp, (long *)0, (long *)0);
			return (-1);
		}
		amt -= amount;
		bytes += amount;
		startsec += amount / secsize;

		if (lverbose && (bytes >= (savbytes + 0x100000))) {
			int	nsecs = (bytes - savbytes) / secsize;
			float	fspeed;

			gettimeofday(&tcur, (struct timezone *)0);
			printf(_("\rTrack %02d: %4lld"), track, bytes >> 20);
			if (padsize > 0)
				printf(_(" of %4lld MB"), padsize >> 20);
			else
				printf(" MB");
			printf(_(" pad written"));
			savbytes = (bytes >> 20) << 20;

#ifdef	BCAP
			/*
			 * Work around a bug in the firmware from drives
			 * developed by PIONEER in November 2009. This affects
			 * drives labelled "Pioneer", "Plextor" and "TEAC".
			 * Do no longer call cdr_buffer_cap() before the drive
			 * buffer was not at least filled once to avoid that
			 * the the drive throughs away all data.
			 */
			if (bsize > 0 && bytes > bsize) { /* buffer size known */
				scgp->silent++;
				per = (*dp->cdr_buffer_cap)(scgp, (long *)0, &bfree);
				scgp->silent--;
				if (per >= 0) {
					per = 100*(bsize - bfree) / bsize;
					if ((bsize - bfree) <= amount || per <= 5)
						dp->cdr_dstat->ds_buflow++;
					if (per < (int)dp->cdr_dstat->ds_minbuf &&
					    (startsec*secsize) > bsize) {
						dp->cdr_dstat->ds_minbuf = per;
					}
					printf(_(" [buf %3d%%]"), per);
#ifdef	BCAPDBG
					printf(" %3ld %3ld", bsize >> 10, bfree >> 10);
#endif
				}
			}
#endif
			tlast.tv_sec = tcur.tv_sec - tlast.tv_sec;
			tlast.tv_usec = tcur.tv_usec - tlast.tv_usec;
			while (tlast.tv_usec < 0) {
				tlast.tv_usec += 1000000;
				tlast.tv_sec -= 1;
			}
			fspeed = (nsecs / secsps) /
				(tlast.tv_sec * 1.0 + tlast.tv_usec * 0.000001);
			if (fspeed > 999.0)
				fspeed = 999.0;
			printf(" %5.1fx", fspeed);
			printf(".");
			flush();
			neednl = TRUE;
			tlast = tcur;
		}
	} while (amt > 0);

	if (bytesp)
		*bytesp = bytes;
	if (bytes == 0)
		return (0);
	return (bytes > 0 ? 1:-1);
}

#ifdef	USE_WRITE_BUF
EXPORT int
write_buf(scgp, dp, trackp, bp, startsec, amt, secsize, dolast, bytesp)
	SCSI	*scgp;
	cdr_t	*dp;
	track_t	*trackp;
	char	*bp;
	long	startsec;
	Llong	amt;
	int	secsize;
	BOOL	dolast;
	Llong	*bytesp;
{
	int	track = trackp->trackno;
	Llong	bytes	= 0;
	Llong	savbytes = 0;
/*	int	secsize;*/
	int	secspt;
	int	bytespt;
	int	amount;
	BOOL	neednl	= FALSE;
	BOOL	islast	= FALSE;

/*	secsize = trackp->secsize;*/
/*	secspt = trackp->secspt;*/

	secspt = bufsize/secsize;
	secspt = min(255, secspt);
	bytespt = secsize * secspt;

/*	fillbytes(buf, bytespt, '\0');*/

	if ((amt >> 20) > 0) {
		printf(_("\rTrack %02d:   0 of %4ld MB pad written."),
						track, amt >> 20);
		flush();
	}
	do {
		if (amt < bytespt) {
			bytespt = roundup(amt, secsize);
			secspt = bytespt / secsize;
		}
		if (dolast && (amt - bytespt) <= 0)
			islast = TRUE;

		amount = write_secs(scgp, dp, bp, startsec, bytespt, secspt, islast);
		if (amount < 0) {
			printf(_("%swrite track data: error after %ld bytes\n"),
							neednl?"\n":"", bytes);
			if (bytesp)
				*bytesp = bytes;
(*dp->cdr_buffer_cap)(scgp, (long *)0, (long *)0);
			return (-1);
		}
		amt -= amount;
		bytes += amount;
		startsec += amount / secsize;

		if (lverbose && (bytes >= (savbytes + 0x100000))) {
			printf(_("\rTrack %02d: %3ld"), track, bytes >> 20);
			savbytes = (bytes >> 20) << 20;
			flush();
			neednl = TRUE;
		}
	} while (amt > 0);

	if (bytesp)
		*bytesp = bytes;
	return (bytes);
}
#endif	/* USE_WRITE_BUF */

LOCAL void
printdata(track, trackp)
	int	track;
	track_t	*trackp;
{
	if (trackp->itracksize >= 0) {
		printf(_("Track %02d: data  %4lld MB        "),
					track, (Llong)(trackp->itracksize >> 20));
	} else {
		printf(_("Track %02d: data  unknown length"),
					track);
	}
	if (trackp->padsecs > 0) {
		Llong	padbytes = (Llong)trackp->padsecs * trackp->isecsize;

		if ((padbytes >> 20) > 0)
			printf(_(" padsize: %4lld MB"), (Llong)(padbytes >> 20));
		else
			printf(_(" padsize: %4lld KB"), (Llong)(padbytes >> 10));
	}
	if (trackp->pregapsize != (trackp->flags & TI_NOCD)? 0 : 150) {
		printf(_(" pregapsize: %3ld"), trackp->pregapsize);
	}
	if (xdebug)
		printf(" START: %ld SECTORS: %ld INDEX0 %ld",
			trackp->trackstart, trackp->tracksecs, trackp->index0start);
	printf("\n");
}

LOCAL void
printaudio(track, trackp)
	int	track;
	track_t	*trackp;
{
	if (trackp->itracksize >= 0) {
		printf(_("Track %02d: audio %4lld MB (%02d:%02d.%02d) %spreemp%s%s"),
			track, (Llong)(trackp->itracksize >> 20),
			minutes(trackp->itracksize),
			seconds(trackp->itracksize),
			hseconds(trackp->itracksize),
			is_preemp(trackp) ? "" : _("no "),
			is_swab(trackp) ? _(" swab"):"",
			((trackp->itracksize < 300L*trackp->isecsize) ||
			(trackp->itracksize % trackp->isecsize)) &&
			is_pad(trackp) ? _(" pad") : "");
	} else {
		printf(_("Track %02d: audio unknown length    %spreemp%s%s"),
			track, is_preemp(trackp) ? "" : _("no "),
			is_swab(trackp) ? _(" swab"):"",
			(trackp->itracksize % trackp->isecsize) && is_pad(trackp) ? _(" pad") : "");
	}
	if (is_scms(trackp))
		printf(" scms");
	else if (is_copy(trackp))
		printf(" copy");
	else
		printf("     ");

	if (trackp->padsecs > 0) {
		Llong	padbytes = (Llong)trackp->padsecs * trackp->isecsize;

		if ((padbytes >> 20) > 0)
			printf(_(" padsize: %4lld MB"), (Llong)(padbytes >> 20));
		else
			printf(_(" padsize: %4lld KB"), (Llong)(padbytes >> 10));
		printf(" (%02d:%02d.%02d)",
			Sminutes(trackp->padsecs),
			Sseconds(trackp->padsecs),
			Shseconds(trackp->padsecs));
	}
	if (trackp->pregapsize != ((trackp->flags & TI_NOCD)? 0 : 150) || xdebug > 0) {
		printf(_(" pregapsize: %3ld"), trackp->pregapsize);
	}
	if (xdebug)
		printf(" START: %ld SECTORS: %ld INDEX0 %ld",
			trackp->trackstart, trackp->tracksecs, trackp->index0start);
	printf("\n");
}

LOCAL void
checkfile(track, trackp)
	int	track;
	track_t	*trackp;
{
	if (trackp->itracksize > 0 &&
			is_audio(trackp) &&
			((!is_shorttrk(trackp) &&
			(trackp->itracksize < 300L*trackp->isecsize)) ||
			(trackp->itracksize % trackp->isecsize)) &&
						!is_pad(trackp)) {
		errmsgno(EX_BAD, _("Bad audio track size %lld for track %02d.\n"),
				(Llong)trackp->itracksize, track);
		errmsgno(EX_BAD, _("Audio tracks must be at least %ld bytes and a multiple of %d.\n"),
				300L*trackp->isecsize, trackp->isecsize);

		if (!is_shorttrk(trackp) && (trackp->itracksize < 300L*trackp->isecsize))
			comerrno(EX_BAD, _("See -shorttrack option.\n"));
		if (!is_pad(trackp) && (trackp->itracksize % trackp->isecsize))
			comerrno(EX_BAD, _("See -pad option.\n"));
	}

	if (lverbose == 0 && xdebug == 0)
		return;

	if (is_audio(trackp))
		printaudio(track, trackp);
	else
		printdata(track, trackp);
}

LOCAL int
checkfiles(tracks, trackp)
	int	tracks;
	track_t	*trackp;
{
	int	i;
	int	isaudio = 1;
	int	starttrack = 1;
	int	endtrack = tracks;

	if (xdebug) {
		/*
		 * Include Lead-in & Lead-out.
		 */
		starttrack--;
		endtrack++;
	}
	for (i = starttrack; i <= endtrack; i++) {
		if (!is_audio(&trackp[i]))
			isaudio = 0;
		if (xdebug)
			printf("SECTYPE %X ", trackp[i].sectype);
		checkfile(i, &trackp[i]);
	}
	return (isaudio);
}

LOCAL void
setleadinout(tracks, trackp)
	int	tracks;
	track_t	*trackp;
{
	/*
	 * Set some values for track 0 (the lead-in)
	 */
	if (!is_clone(&trackp[0])) {
		trackp[0].sectype = trackp[1].sectype;
		trackp[0].dbtype  = trackp[1].dbtype;
		trackp[0].dataoff = trackp[1].dataoff;

		/*
		 * XXX Which other flags should be copied to Track 0 ?
		 */
		if (is_audio(&trackp[1]))
			trackp[0].flags |= TI_AUDIO;
	}

	/*
	 * Set some values for track 0xAA (the lead-out)
	 */
	trackp[tracks+1].pregapsize = 0;
	trackp[tracks+1].isecsize   = trackp[tracks].isecsize;
	trackp[tracks+1].secsize    = trackp[tracks].secsize;

	if (!is_clone(&trackp[0])) {
		trackp[tracks+1].tracktype = trackp[tracks].tracktype;
		trackp[tracks+1].sectype   = trackp[tracks].sectype;
		trackp[tracks+1].dbtype    = trackp[tracks].dbtype;
		trackp[tracks+1].dataoff   = trackp[tracks].dataoff;
	}

	trackp[tracks+1].flags = trackp[tracks].flags;
}

LOCAL void
setpregaps(tracks, trackp)
	int	tracks;
	track_t	*trackp;
{
	int	i;
	int	sectype;
	long	pregapsize;
	track_t	*tp;

	sectype = trackp[1].sectype;
	sectype &= ST_MASK;

	for (i = 1; i <= tracks; i++) {
		tp = &trackp[i];
		if (tp->pregapsize == -1L) {
			tp->pregapsize = 150;		/* Default CD Pre GAP*/
			if (trackp->flags & TI_NOCD) {
				tp->pregapsize = 0;
			} else if (sectype != (tp->sectype & ST_MASK)) {
				tp->pregapsize = 255;	/* Pre GAP is 255 */
				tp->flags &= ~TI_PREGAP;
			}
		}
		sectype = tp->sectype & ST_MASK;	/* Save old sectype */
	}
	trackp[tracks+1].pregapsize = 0;
	trackp[tracks+1].index0start = 0;

	for (i = 1; i <= tracks; i++) {
		/*
		 * index0start is set below tracksecks if this track contains
		 * the pregap (index 0) of the next track.
		 */
		trackp[i].index0start = trackp[i].tracksecs;

		pregapsize = trackp[i+1].pregapsize;
		if (is_pregap(&trackp[i+1]) && pregapsize > 0)
			trackp[i].index0start -= pregapsize;
	}
}

/*
 * Check total size of the medium
 */
LOCAL long
checktsize(tracks, trackp)
	int	tracks;
	track_t	*trackp;
{
	int	i;
	Llong	curr;
	Llong	total = -150;	/* CD track #1 pregap compensation */
	Ullong	btotal;
	track_t	*tp;

	if (trackp->flags & TI_NOCD)
		total = 0;
	for (i = 1; i <= tracks; i++) {
		tp = &trackp[i];
		if (!is_pregap(tp))
			total += tp->pregapsize;

		if (lverbose > 1) {
			printf(_("track: %d start: %lld pregap: %ld\n"),
					i, total, tp->pregapsize);
		}
		tp->trackstart = total;
		if (tp->itracksize >= 0) {
			curr = (tp->itracksize + (tp->isecsize-1)) / tp->isecsize;
			curr += tp->padsecs;
			/*
			 * Minimum track size is 4s
			 */
			if (!is_shorttrk(tp) && curr < 300)
				curr = 300;
			if ((trackp->flags & TI_NOCD) == 0) {
				/*
				 * XXX Was passiert hier bei is_packet() ???
				 */
				if (is_tao(tp) && !is_audio(tp)) {
					curr += 2;
				}
			}
			total += curr;
		} else if (is_sao(tp) || is_raw(tp)) {
			errmsgno(EX_BAD, _("Track %d has unknown length.\n"), i);
			comerrno(EX_BAD,
			_("Use tsize= option in %s mode to specify track size.\n"),
			is_sao(tp) ? "SAO" : "RAW");
		}
	}
	tp = &trackp[i];
	tp->trackstart = total;
	tp->tracksecs = 6750;		/* Size of first session Lead-Out */
	if (!lverbose)
		return (total);

	if (trackp->flags & TI_NOCD)
		btotal = (Ullong)total * 2048;
	else
		btotal = (Ullong)total * 2352;
/* XXX CD Sector Size ??? */
	if (tracks > 0) {
		if (trackp->flags & TI_NOCD) {
			printf(_("Total size:     %4llu MB = %lld sectors\n"),
				btotal >> 20, total);
		} else {
			printf(_("Total size:     %4llu MB (%02d:%02d.%02d) = %lld sectors\n"),
				btotal >> 20,
				minutes(btotal),
				seconds(btotal),
				hseconds(btotal), total);
			btotal += 150 * 2352;
			printf(_("Lout start:     %4llu MB (%02d:%02d/%02d) = %lld sectors\n"),
				btotal >> 20,
				minutes(btotal),
				seconds(btotal),
				frames(btotal), total);
		}
	}
	return (total);
}

LOCAL void
opentracks(trackp)
	track_t	*trackp;
{
	track_t	*tp;
	int	i;
	int	tracks = trackp[0].tracks;

	Llong	tracksize;
	int	secsize;

	if (trackp[1].flags & TI_USEINFO &&
	    auinfhidden(trackp[1].filename, 1, trackp)) {
		tracks = cvt_hidden(trackp);
	}

	for (i = 0; i <= tracks; i++) {
		tp = &trackp[i];
		if (i == 0 && tp->filename == NULL)
			continue;

		if (auinfosize(tp->filename, tp)) {
			/*
			 * If auinfosize() returns TRUE, then tp->filename does
			 * not point to an audio file but to an *.inf file.
			 * In this case, we open stdin to allow
			 * cdda2wav | cdrecord
			 */
			tp->xfp = xopen(NULL, O_RDONLY|O_BINARY, 0, 0);
			tp->flags |= TI_STDIN;
		} else if (strcmp("-", tp->filename) == 0) {
			/*
			 * open stdin
			 */
			tp->xfp = xopen(NULL, O_RDONLY|O_BINARY, 0, 0);
			tp->flags |= TI_STDIN;
			if (((tp->flags & TI_ISOSIZE) != 0) &&
			    !is_audio(tp) && (is_sao(tp) || is_raw(tp))) {
				if ((exargs.flags & F_WAITI) == 0) {
					exargs.flags |= F_WAITI;
					wait_input();
				}
			}
		} else {
			if ((tp->xfp = xopen(tp->filename,
					O_RDONLY|O_BINARY, 0, 0)) == NULL) {
				comerr(_("Cannot open '%s'.\n"), tp->filename);
			}
		}

		checksize(tp);
		tracksize = tp->itracksize;
		secsize = tp->isecsize;
		if (!is_shorttrk(tp) &&
		    tracksize > 0 && (tracksize / secsize) < 300) {

			tracksize = roundup(tracksize, secsize);
			if ((tp->padsecs +
			    (tracksize / secsize)) < 300) {
				tp->padsecs =
					300 - tracksize / secsize;
			}
			if (xdebug) {
				printf(_("TRACK %d SECTORS: %ld"),
					i, tp->tracksecs);
				printf(_(" pasdize %lld (%ld sectors)\n"),
					(Llong)tp->padsecs * secsize,
					tp->padsecs);
			}
		}
#ifdef	AUINFO
		if (tp->flags & TI_USEINFO) {
			auinfo(tp->filename, i, trackp);
			if (lverbose > 0 && i == 1)
				printf("pregap1: %ld\n", trackp[1].pregapsize);
		}
#endif
		/*
		 * tracksecks is total numbers of sectors in track (starting from
		 * index 0).
		 */
		if (tp->padsecs > 0)
			tp->tracksecs += tp->padsecs;

		if (debug) {
			printf(_(
			    "File: '%s' itracksize: %lld isecsize: %d tracktype: %d = %s sectype: %X = %s dbtype: %s flags %X\n"),
				tp->filename, (Llong)tp->itracksize,
				tp->isecsize,
				tp->tracktype & TOC_MASK, toc2name[tp->tracktype & TOC_MASK],
				tp->sectype, st2name[tp->sectype & ST_MASK], db2name[tp->dbtype], tp->flags);
		}
	}
}

LOCAL int
cvt_hidden(trackp)
	track_t	*trackp;
{
	register int	i;
	register int	tracks;
		int	trackno;
		track_t	*tp0 = &trackp[0];
		track_t	*tp1 = &trackp[1];

	tp0->filename = tp1->filename;
	tp0->trackstart = tp1->trackstart;
	tp0->itracksize = tp1->itracksize;
	tp0->tracksize = tp1->tracksize;
	tp0->tracksecs = tp1->tracksecs;

	tracks = tp0->tracks - 1;	/* Reduce number of tracks by one */
	trackno = trackp[2].trackno;	/* Will become track # f. track 1 */
	trackno -= 2;
	if (trackno < 0)
		trackno = 0;

	for (i = 1; i < MAX_TRACK+1; i++) {
		movebytes(&trackp[i+1], &trackp[i], sizeof (trackp[0]));
	}
	for (i = 0; i < MAX_TRACK+2; i++) {
		trackp[i].track = i;
		trackp[i].trackno = trackno + i;
		trackp[i].tracks = tracks;
	}
	tp0->trackno = 0;
	tp0->flags |= TI_HIDDEN;
	tp1->flags |= TI_HIDDEN;
	tp1->flags &= ~TI_PREGAP;
	tp0->flags |= tp1->flags &
			(TI_SWAB|TI_AUDIO|TI_COPY|TI_QUADRO|TI_PREEMP|TI_SCMS);
	return (tracks);
}

LOCAL void
checksize(trackp)
	track_t	*trackp;
{
	struct stat	st;
	Llong		lsize;
	int		f = -1;

	if (trackp->xfp != NULL)
		f = xfileno(trackp->xfp);

	/*
	 * If the current input file is a regular file and
	 * 'padsize=' has not been specified,
	 * use fstat() or file parser to get the size of the file.
	 */
	if (trackp->itracksize < 0 && (trackp->flags & TI_ISOSIZE) != 0) {
		if ((trackp->flags & TI_STDIN) != 0) {
			if (trackp->track != 1)
				comerrno(EX_BAD, _("-isosize on stdin only works for the first track.\n"));
			isobuf = malloc(32 * 2048);
			if (isobuf == NULL)
				comerrno(EX_BAD, _("Cannot malloc iso size buffer.\n"));
			isobsize = read_buf(f, isobuf, 32 * 2048);
			lsize = bisosize(isobuf, isobsize);
		} else {
			lsize = isosize(f);
		}
		trackp->itracksize = lsize;
		if (trackp->itracksize != lsize)
			comerrno(EX_BAD, _("This OS cannot handle large ISO-9660 images.\n"));
	}
	if (trackp->itracksize < 0 && (trackp->flags & TI_NOAUHDR) == 0) {
		lsize = ausize(f);
		xmarkpos(trackp->xfp);
		trackp->itracksize = lsize;
		if (trackp->itracksize != lsize)
			comerrno(EX_BAD, _("This OS cannot handle large audio images.\n"));
	}
	if (trackp->itracksize < 0 && (trackp->flags & TI_NOAUHDR) == 0) {
		lsize = wavsize(f);
		xmarkpos(trackp->xfp);
		trackp->itracksize = lsize;
		if (trackp->itracksize != lsize)
			comerrno(EX_BAD, _("This OS cannot handle large WAV images.\n"));
		if (trackp->itracksize > 0)	/* Force little endian input */
			trackp->flags |= TI_SWAB;
	}
	if (trackp->itracksize == AU_BAD_CODING) {
		comerrno(EX_BAD, _("Inappropriate audio coding in '%s'.\n"),
							trackp->filename);
	}
	if (trackp->itracksize < 0 &&
			fstat(f, &st) >= 0 && S_ISREG(st.st_mode)) {
		trackp->itracksize = st.st_size;
	}
	if (trackp->itracksize >= 0) {
		/*
		 * We do not allow cdrecord to start if itracksize is not
		 * a multiple of isecsize or we are allowed to pad to secsize via -pad.
		 * For this reason, we may safely always assume padding.
		 */
		trackp->tracksecs = (trackp->itracksize + trackp->isecsize -1) / trackp->isecsize;
		trackp->tracksize = (trackp->itracksize / trackp->isecsize) * trackp->secsize
					+ trackp->itracksize % trackp->isecsize;
	} else {
		trackp->tracksecs = -1L;
	}
}

LOCAL BOOL
checkdsize(scgp, dp, tsize, flags)
	SCSI	*scgp;
	cdr_t	*dp;
	long	tsize;
	UInt32_t flags;
{
	long	startsec = 0L;
	long	endsec = 0L;
	dstat_t	*dsp = dp->cdr_dstat;

	/*
	 * Set the track pointer to NULL in order to signal the driver that we
	 * like to get the next writable address for the next (unwritten)
	 * session.
	 */
	scgp->silent++;
	(*dp->cdr_next_wr_address)(scgp, (track_t *)0, &startsec);
	scgp->silent--;

	/*
	 * This only should happen when the drive is currently in SAO mode.
	 * We rely on the drive being in TAO mode, a negative value for
	 * startsec is not correct here it may be caused by bad firmware or
	 * by a drive in SAO mode. In SAO mode the drive will report the
	 * pre-gap as part of the writable area.
	 */
	if (startsec < 0)
		startsec = 0;

	/*
	 * Size limitations (sectors) for CD's:
	 *
	 *		404850 == 90 min	Red book calls this the
	 *					first negative time
	 *					allows lead out start up to
	 *					block 404700
	 *
	 *		449850 == 100 min	This is the first time that
	 *					is no more representable
	 *					in a two digit BCD number.
	 *					allows lead out start up to
	 *					block 449700
	 *
	 *		~540000 == 120 min	The largest CD ever made.
	 *
	 *		~650000 == 1.3 GB	a Double Density (DD) CD.
	 */

	endsec = startsec + tsize;
	dsp->ds_startsec = startsec;
	dsp->ds_endsec = endsec;


	if (dsp->ds_maxblocks > 0) {
		/*
		 * dsp->ds_maxblocks > 0 (disk capacity is known).
		 */
		if (lverbose)
			printf(_("Blocks total: %ld Blocks current: %ld Blocks remaining: %ld\n"),
					(long)dsp->ds_maxblocks,
					(long)dsp->ds_maxblocks - startsec,
					(long)dsp->ds_maxblocks - endsec);

		if (endsec > dsp->ds_maxblocks) {
			if (dsp->ds_flags & DSF_NOCD) { /* Not a CD */
				/*
				 * There is no overburning on DVD/BD...
				 */
				errmsgno(EX_BAD,
				_("Data does not fit on current disk.\n"));
				goto toolarge;
			}
			errmsgno(EX_BAD,
			_("WARNING: Data may not fit on current disk.\n"));

			/* XXX Check for flags & CDR_NO_LOLIMIT */
/*			goto toolarge;*/
		}
		if (lverbose && dsp->ds_maxrblocks > 0)
			printf(_("RBlocks total: %ld RBlocks current: %ld RBlocks remaining: %ld\n"),
					(long)dsp->ds_maxrblocks,
					(long)dsp->ds_maxrblocks - startsec,
					(long)dsp->ds_maxrblocks - endsec);
		if (dsp->ds_maxrblocks > 0 && endsec > dsp->ds_maxrblocks) {
			errmsgno(EX_BAD,
			_("Data does not fit on current disk.\n"));
			goto toolarge;
		}
		if ((endsec > dsp->ds_maxblocks && endsec > 404700) ||
		    (dsp->ds_maxrblocks > 404700 && 449850 > dsp->ds_maxrblocks)) {
			/*
			 * Assume that this must be a CD and not a DVD.
			 * So this is a non Red Book compliant CD with a
			 * capacity between 90 and 99 minutes.
			 */
			if (dsp->ds_maxrblocks > 404700)
				printf(_("RedBook total: %ld RedBook current: %ld RedBook remaining: %ld\n"),
					404700L,
					404700L - startsec,
					404700L - endsec);
			if (endsec > dsp->ds_maxblocks && endsec > 404700) {
				if ((flags & (F_IGNSIZE|F_FORCE)) == 0) {
					errmsgno(EX_BAD,
					_("Notice: Most recorders cannot write CD's >= 90 minutes.\n"));
					errmsgno(EX_BAD,
					_("Notice: Use -ignsize option to allow >= 90 minutes.\n"));
				}
				goto toolarge;
			}
		}
	} else {
		/*
		 * dsp->ds_maxblocks == 0 (disk capacity is unknown).
		 */
		errmsgno(EX_BAD, _("Disk capacity is unknown.\n"));

		if (endsec >= (405000-300)) {			/*<90 min disk*/
			errmsgno(EX_BAD,
				_("Data will not fit on any CD.\n"));
			goto toolarge;
		} else if (endsec >= (333000-150)) {		/* 74 min disk*/
			errmsgno(EX_BAD,
			_("WARNING: Data may not fit on standard 74min CD.\n"));
		}
	}
	if (dsp->ds_maxblocks <= 0 || endsec <= dsp->ds_maxblocks)
		return (TRUE);
	/* FALLTHROUGH */
toolarge:
	if (dsp->ds_maxblocks > 0 && endsec > dsp->ds_maxblocks) {
		if ((flags & (F_OVERBURN|F_IGNSIZE|F_FORCE)) != 0) {
			if (dsp->ds_flags & DSF_NOCD) {		/* Not a CD */
				errmsgno(EX_BAD,
				_("Notice: -overburn is not expected to work with DVD/BD media.\n"));
			}
			errmsgno(EX_BAD,
				_("Notice: Overburning active. Trying to write more than the official disk capacity.\n"));
			return (TRUE);
		} else {
			if ((dsp->ds_flags & DSF_NOCD) == 0) {	/* A CD and not a DVD/BD */
				errmsgno(EX_BAD,
				_("Notice: Use -overburn option to write more than the official disk capacity.\n"));
				errmsgno(EX_BAD,
				_("Notice: Most CD-writers do overburning only on SAO or RAW mode.\n"));
			}
			return (FALSE);
		}
	}
	if (dsp->ds_maxblocks < 449850) {
		if (dsp->ds_flags & DSF_NOCD) {			/* A DVD/BD */
			if (endsec <= dsp->ds_maxblocks)
				return (TRUE);
			if (dsp->ds_maxblocks <= 0) {
				errmsgno(EX_BAD, _("DVD/BD capacity is unknown.\n"));
				if ((flags & (F_IGNSIZE|F_FORCE)) != 0) {
					errmsgno(EX_BAD,
						_("Notice: -ignsize active.\n"));
					return (TRUE);
				}
			}
			errmsgno(EX_BAD, _("Cannot write more than remaining DVD/BD capacity.\n"));
			return (FALSE);
		}
		/*
		 * Assume that this must be a CD and not a DVD.
		 */
		if (endsec > 449700) {
			errmsgno(EX_BAD, _("Cannot write CD's >= 100 minutes.\n"));
			return (FALSE);
		}
	}
	if ((flags & (F_IGNSIZE|F_FORCE)) != 0)
		return (TRUE);
	return (FALSE);
}

LOCAL void
raise_fdlim()
{
#ifdef	RLIMIT_NOFILE

	struct rlimit	rlim;

	/*
	 * Set max # of file descriptors to be able to hold all files open
	 */
	getrlimit(RLIMIT_NOFILE, &rlim);
	if (rlim.rlim_cur >= (MAX_TRACK + 10))
		return;

	rlim.rlim_cur = MAX_TRACK + 10;
	if (rlim.rlim_cur > rlim.rlim_max)
		errmsgno(EX_BAD,
			_("Warning: low file descriptor limit (%lld).\n"),
						(Llong)rlim.rlim_max);
	setrlimit(RLIMIT_NOFILE, &rlim);

#endif	/* RLIMIT_NOFILE */
}

LOCAL void
raise_memlock()
{
#ifdef	RLIMIT_MEMLOCK
	struct rlimit rlim;

	rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;

	if (setrlimit(RLIMIT_MEMLOCK, &rlim) < 0)
		errmsg(_("Warning: Cannot raise RLIMIT_MEMLOCK limits.\n"));
#endif	/* RLIMIT_MEMLOCK */
}

LOCAL int
getfilecount(ac, av, fmt)
	int		ac;
	char	*const *av;
	const char	*fmt;
{
	int	files = 0;

	for (; ; --ac, av++) {
		if (getfiles(&ac, &av, fmt) == 0)
			break;
		files++;
	}
	return (files);
}


char	*opts =
/* CSTYLED */
"help,version,checkdrive,prcap,inq,scanbus,reset,abort,overburn,ignsize,useinfo,dev*,scgopts*,timeout#,driver*,driveropts*,setdropts,tsize&,padsize&,pregap&,defpregap&,speed#,load,lock,eject,dummy,minfo,media-info,msinfo,toc,atip,multi,fix,nofix,waiti,immed,debug#,d+,kdebug#,kd#,verbose+,v+,Verbose+,V+,x+,xd#,silent,s,audio,data,mode2,xa,xa1,xa2,xamix,cdi,isosize,nopreemp,preemp,nocopy,copy,nopad,pad,swab,fs&,ts&,blank&,format,pktsize#,packet,noclose,force,tao,dao,sao,raw,raw96r,raw96p,raw16,clone,scms,isrc*,mcn*,index*,cuefile*,textfile*,text,shorttrack,noshorttrack,gracetime#,minbuf#";

/*
 * Defines used to find whether a write mode has been specified.
 */
#define	M_TAO		1	/* Track at Once mode */
#define	M_SAO		2	/* Session at Once mode (also known as DAO) */
#define	M_RAW		4	/* Raw mode */
#define	M_PACKET	8	/* Packed mode */

LOCAL void
gargs(ac, av, tracksp, trackp, devp, scgoptp, timeoutp, dpp, speedp, flagsp, blankp)
	int	ac;
	char	**av;
	int	*tracksp;
	track_t	*trackp;
	cdr_t	**dpp;
	char	**devp;
	char	**scgoptp;
	int	*timeoutp;
	int	*speedp;
	UInt32_t *flagsp;
	int	*blankp;
{
	int	cac;
	char	* const*cav;
	char	*driver = NULL;
	char	*dev = NULL;
	char	*isrc = NULL;
	char	*mcn = NULL;
	char	*tindex = NULL;
	char	*cuefile = NULL;
	char	*textfile = NULL;
	long	bltype = -1;
	int	doformat = 0;
	Llong	tracksize;
	Llong	padsize;
	long	pregapsize;
	long	defpregap = -1L;
	int	pktsize;
	int	speed = -1;
	int	help = 0;
	int	version = 0;
	int	checkdrive = 0;
	int	setdropts = 0;
	int	prcap = 0;
	int	inq = 0;
	int	scanbus = 0;
	int	reset = 0;
	int	doabort = 0;
	int	overburn = 0;
	int	ignsize = 0;
	int	useinfo = 0;
	int	load = 0;
	int	lock = 0;
	int	eject = 0;
	int	dummy = 0;
	int	mediainfo = 0;
	int	msinfo = 0;
	int	toc = 0;
	int	atip = 0;
	int	multi = 0;
	int	fix = 0;
	int	nofix = 0;
	int	waiti = 0;
	int	immed = 0;
	int	audio;
	int	autoaudio = 0;
	int	data;
	int	mode2;
	int	xa;
	int	xa1;
	int	xa2;
	int	xamix;
	int	cdi;
	int	isize;
	int	ispacket = 0;
	int	noclose = 0;
	int	force = 0;
	int	tao = 0;
	int	dao = 0;
	int	raw = 0;
	int	raw96r = 0;
	int	raw96p = 0;
	int	raw16 = 0;
	int	clone = 0;
	int	scms = 0;
	int	preemp = 0;
	int	nopreemp;
	int	copy = 0;
	int	nocopy;
	int	pad = 0;
	int	bswab = 0;
	int	nopad;
	int	usetext = 0;
	int	shorttrack = 0;
	int	noshorttrack;
	int	flags;
	int	tracks = *tracksp;
	int	tracktype = TOC_ROM;
/*	int	sectype = ST_ROM_MODE1 | ST_MODE_1;*/
	int	sectype = SECT_ROM;
	int	dbtype = DB_ROM_MODE1;
	int	secsize = DATA_SEC_SIZE;
	int	dataoff = 16;
	int	ga_ret;
	int	wm = 0;
	int	ntracks;

	trackp[0].flags |= TI_TAO;
	trackp[1].pregapsize = -1;
	*flagsp |= F_WRITE;

	cac = --ac;
	cav = ++av;
	ntracks = getfilecount(ac, av, opts);
	for (; ; cac--, cav++) {
		tracksize = (Llong)-1L;
		padsize = (Llong)0L;
		pregapsize = defpregap;
		audio = data = mode2 = xa = xa1 = xa2 = xamix = cdi = 0;
		isize = nopreemp = nocopy = nopad = noshorttrack = 0;
		pktsize = 0;
		isrc = NULL;
		tindex = NULL;
		/*
		 * Get options up to next file type arg.
		 */
		if ((ga_ret = getargs(&cac, &cav, opts,
				&help, &version, &checkdrive, &prcap,
				&inq, &scanbus, &reset, &doabort, &overburn, &ignsize,
				&useinfo,
				devp, scgoptp, timeoutp, &driver,
				&driveropts, &setdropts,
				getllnum, &tracksize,
				getllnum, &padsize,
				getnum, &pregapsize,
				getnum, &defpregap,
				&speed,
				&load, &lock,
				&eject, &dummy, &mediainfo, &mediainfo,
				&msinfo, &toc, &atip,
				&multi, &fix, &nofix, &waiti, &immed,
				&debug, &debug,
				&kdebug, &kdebug,
				&lverbose, &lverbose,
				&scsi_verbose, &scsi_verbose,
				&xdebug, &xdebug,
				&silent, &silent,
				&audio, &data, &mode2,
				&xa, &xa1, &xa2, &xamix, &cdi,
				&isize,
				&nopreemp, &preemp,
				&nocopy, &copy,
				&nopad, &pad, &bswab, getnum, &fs, getnum, &bufsize,
				getbltype, &bltype, &doformat, &pktsize,
				&ispacket, &noclose, &force,
				&tao, &dao, &dao, &raw, &raw96r, &raw96p, &raw16,
				&clone,
				&scms, &isrc, &mcn, &tindex,
				&cuefile, &textfile, &usetext,
				&shorttrack, &noshorttrack,
				&gracetime, &dminbuf)) < 0) {
			errmsgno(EX_BAD, _("Bad Option: %s.\n"), cav[0]);
			susage(EX_BAD);
		}
		if (help)
			usage(0);
		if (tracks == 0) {
			if (driver)
				set_cdrcmds(driver, dpp);
			if (version) {
				if (ntracks > 0)
					etracks("version");
				*flagsp |= F_VERSION;
			}
			if (checkdrive) {
				if (ntracks > 0)
					etracks("checkdrive");
				*flagsp |= F_CHECKDRIVE;
			}
			if (prcap) {
				if (ntracks > 0)
					etracks("prcap");
				*flagsp |= F_PRCAP;
			}
			if (inq) {
				if (ntracks > 0)
					etracks("inquiry");
				*flagsp |= F_INQUIRY;
			}
			if (scanbus) {
				if (ntracks > 0)
					etracks("scanbus");
				*flagsp |= F_SCANBUS;
			}
			if (reset) {
				if (ntracks > 0)
					etracks("reset");
				*flagsp |= F_RESET;
			}
			if (doabort) {
				if (ntracks > 0)
					etracks("abort");
				*flagsp |= F_ABORT;
			}
			if (overburn)
				*flagsp |= F_OVERBURN;
			if (ignsize)
				*flagsp |= F_IGNSIZE;
			if (load) {
				if (ntracks > 0)
					etracks("load");
				*flagsp |= F_LOAD;
			}
			if (lock) {
				if (ntracks > 0)
					etracks("lock");
				*flagsp |= F_DLCK;
			}
			if (eject)
				*flagsp |= F_EJECT;
			if (dummy)
				*flagsp |= F_DUMMY;
			if (setdropts) {
				if (ntracks > 0)
					etracks("setdropts");
				*flagsp |= F_SETDROPTS;
			}
			if (mediainfo) {
				if (ntracks > 0)
					etracks("minfo");
				*flagsp |= F_MEDIAINFO;
				*flagsp &= ~F_WRITE;
			}
			if (msinfo) {
				if (ntracks > 0)
					etracks("msinfo");
				*flagsp |= F_MSINFO;
				*flagsp &= ~F_WRITE;
			}
			if (toc) {
				if (ntracks > 0)
					etracks("toc");
				*flagsp |= F_TOC;
				*flagsp &= ~F_WRITE;
			}
			if (atip) {
				if (ntracks > 0)
					etracks("atip");
				*flagsp |= F_PRATIP;
				*flagsp &= ~F_WRITE;
			}
			if (multi) {
				/*
				 * 2048 Bytes user data
				 */
				*flagsp |= F_MULTI;
				tracktype = TOC_XA2;
				sectype = ST_ROM_MODE2 | ST_MODE_2_FORM_1;
				sectype = SECT_MODE_2_F1;
				dbtype = DB_XA_MODE2;	/* XXX -multi nimmt DB_XA_MODE2_F1 !!! */
				secsize = DATA_SEC_SIZE;	/* 2048 */
				dataoff = 24;
			}
			if (fix) {
				if (ntracks > 0)
					etracks("fix");
				*flagsp |= F_FIX;
			}
			if (nofix)
				*flagsp |= F_NOFIX;
			if (waiti)
				*flagsp |= F_WAITI;
			if (immed)
				*flagsp |= F_IMMED;
			if (force)
				*flagsp |= F_FORCE;

			if (bltype >= 0) {
				*flagsp |= F_BLANK;
				*blankp = bltype;
			}
			if (doformat)
				*flagsp |= F_FORMAT;
			if (ispacket)
				wm |= M_PACKET;
			if (tao)
				wm |= M_TAO;
			if (dao) {
				*flagsp |= F_SAO;
				trackp[0].flags &= ~TI_TAO;
				trackp[0].flags |= TI_SAO;
				wm |= M_SAO;

			} else if ((raw == 0) && (raw96r + raw96p + raw16) > 0)
				raw = 1;
			if ((raw != 0) && (raw96r + raw96p + raw16) == 0)
				raw96r = 1;
			if (raw96r) {
				if (!dao)
					*flagsp |= F_RAW;
				trackp[0].flags &= ~TI_TAO;
				trackp[0].flags |= TI_RAW;
				trackp[0].flags |= TI_RAW96R;
				wm |= M_RAW;
			}
			if (raw96p) {
				if (!dao)
					*flagsp |= F_RAW;
				trackp[0].flags &= ~TI_TAO;
				trackp[0].flags |= TI_RAW;
				wm |= M_RAW;
			}
			if (raw16) {
				if (!dao)
					*flagsp |= F_RAW;
				trackp[0].flags &= ~TI_TAO;
				trackp[0].flags |= TI_RAW;
				trackp[0].flags |= TI_RAW16;
				wm |= M_RAW;
			}
			if (mcn) {
#ifdef	AUINFO
				setmcn(mcn, &trackp[0]);
#else
				trackp[0].isrc = malloc(16);
				fillbytes(trackp[0].isrc, 16, '\0');
				strncpy(trackp[0].isrc, mcn, 13);
#endif
				mcn = NULL;
			}
			if ((raw96r + raw96p + raw16) > 1) {
				errmsgno(EX_BAD, _("Too many raw modes.\n"));
				comerrno(EX_BAD, _("Only one of -raw16, -raw96p, -raw96r allowed.\n"));
			}
			if ((tao + ispacket + dao + raw) > 1) {
				errmsgno(EX_BAD, _("Too many write modes.\n"));
				comerrno(EX_BAD, _("Only one of -packet, -dao, -raw allowed.\n"));
			}
			if (dao && (raw96r + raw96p + raw16) > 0) {
				if (raw16)
					comerrno(EX_BAD, _("SAO RAW writing does not allow -raw16.\n"));
				if (!clone)
					comerrno(EX_BAD, _("SAO RAW writing only makes sense in clone mode.\n"));
#ifndef	CLONE_WRITE
				comerrno(EX_BAD, _("SAO RAW writing not compiled in.\n"));
#endif
				comerrno(EX_BAD, _("SAO RAW writing not yet implemented.\n"));
			}
			if (clone) {
				*flagsp |= F_CLONE;
				trackp[0].flags |= TI_CLONE;
#ifndef	CLONE_WRITE
				comerrno(EX_BAD, _("Clone writing not compiled in.\n"));
#endif
			}
			if (textfile) {
				if (!checktextfile(textfile)) {
					if ((*flagsp & F_WRITE) != 0) {
						comerrno(EX_BAD,
							_("Cannot use '%s' as CD-Text file.\n"),
							textfile);
					}
				}
				if ((*flagsp & F_WRITE) != 0) {
					if ((dao + raw96r + raw96p) == 0)
						comerrno(EX_BAD,
							_("CD-Text needs -dao, -raw96r or -raw96p.\n"));
				}
				trackp[0].flags |= TI_TEXT;
			}
			version = checkdrive = prcap = inq = scanbus = reset = doabort =
			overburn = ignsize =
			load = lock = eject = dummy = mediainfo = msinfo =
			toc = atip = multi = fix = nofix =
			waiti = immed = force = dao = setdropts = 0;
			raw96r = raw96p = raw16 = clone = 0;
		} else if ((version + checkdrive + prcap + inq + scanbus +
			    reset + doabort + overburn + ignsize +
			    load + lock + eject + dummy + mediainfo + msinfo +
			    toc + atip + multi + fix + nofix +
			    waiti + immed + force + dao + setdropts +
			    raw96r + raw96p + raw16 + clone) > 0 ||
				mcn != NULL)
			comerrno(EX_BAD, _("Badly placed option. Global options must be before any track.\n"));

		if (nopreemp)
			preemp = 0;
		if (nocopy)
			copy = 0;
		if (nopad)
			pad = 0;
		if (noshorttrack)
			shorttrack = 0;

		if ((audio + data + mode2 + xa + xa1 + xa2 + xamix) > 1) {
			errmsgno(EX_BAD, _("Too many types for track %d.\n"), tracks+1);
			comerrno(EX_BAD, _("Only one of -audio, -data, -mode2, -xa, -xa1, -xa2, -xamix allowed.\n"));
		}
		if (ispacket && audio) {
			comerrno(EX_BAD, _("Audio data cannot be written in packet mode.\n"));
		}
		/*
		 * Check whether the next argument is a file type arg.
		 * If this is true, then we got a track file name.
		 * If getargs() did previously return NOTAFLAG, we may have hit
		 * an argument that has been escaped via "--", so we may not
		 * call getfiles() again in this case. If we would call
		 * getfiles() and the current arg has been escaped and looks
		 * like an option, a call to getfiles() would skip it.
		 */
		if (ga_ret < NOTAFLAG)
			ga_ret = getfiles(&cac, &cav, opts);
		if (autoaudio) {
			autoaudio = 0;
			tracktype = TOC_ROM;
			sectype = ST_ROM_MODE1 | ST_MODE_1;
			sectype = SECT_ROM;
			dbtype = DB_ROM_MODE1;
			secsize = DATA_SEC_SIZE;	/* 2048 */
			dataoff = 16;
		}
		if (ga_ret >= NOTAFLAG && (is_auname(cav[0]) || is_wavname(cav[0]))) {
			/*
			 * We got a track and autodetection decided that it
			 * is an audio track.
			 */
			autoaudio++;
			audio++;
		}
		if (data) {
			/*
			 * 2048 Bytes user data
			 */
			tracktype = TOC_ROM;
			sectype = ST_ROM_MODE1 | ST_MODE_1;
			sectype = SECT_ROM;
			dbtype = DB_ROM_MODE1;
			secsize = DATA_SEC_SIZE;	/* 2048 */
			dataoff = 16;
		}
		if (mode2) {
			/*
			 * 2336 Bytes user data
			 */
			tracktype = TOC_ROM;
			sectype = ST_ROM_MODE2 | ST_MODE_2;
			sectype = SECT_MODE_2;
			dbtype = DB_ROM_MODE2;
			secsize = MODE2_SEC_SIZE;	/* 2336 */
			dataoff = 16;
		}
		if (audio) {
			/*
			 * 2352 Bytes user data
			 */
			tracktype = TOC_DA;
			sectype = preemp ? ST_AUDIO_PRE : ST_AUDIO_NOPRE;
			sectype |= ST_MODE_AUDIO;
			sectype = SECT_AUDIO;
			if (preemp)
				sectype |= ST_PREEMPMASK;
			dbtype = DB_RAW;
			secsize = AUDIO_SEC_SIZE;	/* 2352 */
			dataoff = 0;
		}
		if (xa) {
			/*
			 * 2048 Bytes user data
			 */
			if (tracktype != TOC_CDI)
				tracktype = TOC_XA2;
			sectype = ST_ROM_MODE2 | ST_MODE_2_FORM_1;
			sectype = SECT_MODE_2_F1;
			dbtype = DB_XA_MODE2;
			secsize = DATA_SEC_SIZE;	/* 2048 */
			dataoff = 24;
		}
		if (xa1) {
			/*
			 * 8 Bytes subheader + 2048 Bytes user data
			 */
			if (tracktype != TOC_CDI)
				tracktype = TOC_XA2;
			sectype = ST_ROM_MODE2 | ST_MODE_2_FORM_1;
			sectype = SECT_MODE_2_F1;
			dbtype = DB_XA_MODE2_F1;
			secsize = 2056;
			dataoff = 16;
		}
		if (xa2) {
			/*
			 * 2324 Bytes user data
			 */
			if (tracktype != TOC_CDI)
				tracktype = TOC_XA2;
			sectype = ST_ROM_MODE2 | ST_MODE_2_FORM_2;
			sectype = SECT_MODE_2_F2;
			dbtype = DB_XA_MODE2_F2;
			secsize = 2324;
			dataoff = 24;
		}
		if (xamix) {
			/*
			 * 8 Bytes subheader + 2324 Bytes user data
			 */
			if (tracktype != TOC_CDI)
				tracktype = TOC_XA2;
			sectype = ST_ROM_MODE2 | ST_MODE_2_MIXED;
			sectype = SECT_MODE_2_MIX;
			dbtype = DB_XA_MODE2_MIX;
			secsize = 2332;
			dataoff = 16;
		}
		if (cdi) {
			tracktype = TOC_CDI;
		}
		if (tracks == 0) {
			trackp[0].tracktype = tracktype;
			trackp[0].dbtype = dbtype;
			trackp[0].sectype = sectype;
			trackp[0].isecsize = secsize;
			trackp[0].secsize = secsize;
			if ((*flagsp & F_RAW) != 0) {
				trackp[0].secsize = is_raw16(&trackp[0]) ?
						RAW16_SEC_SIZE:RAW96_SEC_SIZE;
			}
			if ((*flagsp & F_DUMMY) != 0)
				trackp[0].tracktype |= TOCF_DUMMY;
			if ((*flagsp & F_MULTI) != 0)
				trackp[0].tracktype |= TOCF_MULTI;
		}

		flags = trackp[0].flags;

		if ((sectype & ST_AUDIOMASK) != 0)
			flags |= TI_AUDIO;
		if (isize) {
			flags |= TI_ISOSIZE;
			if ((*flagsp & F_MULTI) != 0)
				comerrno(EX_BAD, _("Cannot get isosize for multi session disks.\n"));
			/*
			 * As we do not get the padding from the ISO-9660
			 * formatting utility, we need to force padding here.
			 */
			flags |= TI_PAD;
			if (padsize == (Llong)0L)
				padsize = (Llong)PAD_SIZE;
		}

		if ((flags & TI_AUDIO) != 0) {
			if (preemp)
				flags |= TI_PREEMP;
			if (copy)
				flags |= TI_COPY;
			if (scms)
				flags |= TI_SCMS;
		}
		if (pad || ((flags & TI_AUDIO) == 0 && padsize > (Llong)0L)) {
			flags |= TI_PAD;
			if ((flags & TI_AUDIO) == 0 && padsize == (Llong)0L)
				padsize = (Llong)PAD_SIZE;
		}
		if (shorttrack && (*flagsp & (F_SAO|F_RAW)) != 0)
			flags |= TI_SHORT_TRACK;
		if (noshorttrack)
			flags &= ~TI_SHORT_TRACK;
		if (bswab)
			flags |= TI_SWAB;
		if (ispacket) {
			flags |= TI_PACKET;
			trackp[0].flags &= ~TI_TAO;
		}
		if (noclose)
			flags |= TI_NOCLOSE;
		if (useinfo)
			flags |= TI_USEINFO;

		if (ga_ret <= NOARGS) {
			/*
			 * All options have already been parsed and no more
			 * file type arguments are present.
			 */
			break;
		}
		if (tracks == 0 && (wm == 0))
			flags = default_wr_mode(tracks, trackp, flagsp, &wm, flags);
		tracks++;

		if (tracks > MAX_TRACK)
			comerrno(EX_BAD, _("Track limit (%d) exceeded.\n"),
								MAX_TRACK);
		/*
		 * Make 'tracks' immediately usable in track structure.
		 */
		{	register int i;
			for (i = 0; i < MAX_TRACK+2; i++)
				trackp[i].tracks = tracks;
		}

		if (strcmp("-", cav[0]) == 0)
			*flagsp |= F_STDIN;

		if (!is_auname(cav[0]) && !is_wavname(cav[0]))
			flags |= TI_NOAUHDR;

		if ((*flagsp & (F_SAO|F_RAW)) != 0 && (flags & TI_AUDIO) != 0)
			flags |= TI_PREGAP;	/* Hack for now */
		if (tracks == 1)
			flags &= ~TI_PREGAP;

		if (tracks == 1 && (pregapsize != -1L && pregapsize != 150))
			pregapsize = -1L;
		trackp[tracks].filename = cav[0];
		trackp[tracks].trackstart = 0L;
		trackp[tracks].itracksize = tracksize;
		trackp[tracks].tracksize = tracksize;
		trackp[tracks].tracksecs = -1L;
		if (tracksize >= 0)
			trackp[tracks].tracksecs = (tracksize+secsize-1)/secsize;
		if (trackp[tracks].pregapsize < 0)
			trackp[tracks].pregapsize = pregapsize;
		trackp[tracks+1].pregapsize = -1;
		trackp[tracks].padsecs = (padsize+2047)/2048;
		trackp[tracks].isecsize = secsize;
		trackp[tracks].secsize = secsize;
		trackp[tracks].flags = flags;
		/*
		 * XXX Dies ist falsch: auch bei SAO/RAW kann
		 * XXX secsize != isecsize sein.
		 */
		if ((*flagsp & F_RAW) != 0) {
			if (is_raw16(&trackp[tracks]))
				trackp[tracks].secsize = RAW16_SEC_SIZE;
			else
				trackp[tracks].secsize = RAW96_SEC_SIZE;
#ifndef	HAVE_LIB_EDC_ECC
			if ((sectype & ST_MODE_MASK) != ST_MODE_AUDIO) {
				errmsgno(EX_BAD,
					_("EDC/ECC library not compiled in.\n"));
				comerrno(EX_BAD,
					_("Data sectors are not supported in RAW mode.\n"));
			}
#endif
		}
		trackp[tracks].secspt = 0;	/* transfer size is set up in set_trsizes() */
		trackp[tracks].pktsize = pktsize;
		trackp[tracks].trackno = tracks;
		trackp[tracks].sectype = sectype;
#ifdef	CLONE_WRITE
		if ((*flagsp & F_CLONE) != 0) {
			trackp[tracks].isecsize = 2448;
			trackp[tracks].sectype |= ST_MODE_RAW;
			dataoff = 0;
		}
#endif
		trackp[tracks].dataoff = dataoff;
		trackp[tracks].tracktype = tracktype;
		trackp[tracks].dbtype = dbtype;
		trackp[tracks].flags = flags;
		trackp[tracks].nindex = 1;
		trackp[tracks].tindex = 0;
		if (isrc) {
#ifdef	AUINFO
			setisrc(isrc, &trackp[tracks]);
#else
			trackp[tracks].isrc = malloc(16);
			fillbytes(trackp[tracks].isrc, 16, '\0');
			strncpy(trackp[tracks].isrc, isrc, 12);
#endif
		}
		if (tindex) {
#ifdef	AUINFO
			setindex(tindex, &trackp[tracks]);
#endif
		}
	}

	if (dminbuf >= 0) {
		if (dminbuf < 25 || dminbuf > 95)
			comerrno(EX_BAD,
			_("Bad minbuf=%d option (must be between 25 and 95).\n"),
				dminbuf);
	}

	if (speed < 0 && speed != -1)
		comerrno(EX_BAD, _("Bad speed option.\n"));

	if (fs < 0L && fs != -1L)
		comerrno(EX_BAD, _("Bad fifo size option.\n"));

	if (bufsize < 0L && bufsize != -1L)
		comerrno(EX_BAD, _("Bad transfer size option.\n"));

	dev = *devp;
	cdr_defaults(&dev, &speed, &fs, &bufsize, &driveropts);

	if (bufsize < 0L)
		bufsize = CDR_BUF_SIZE;
	if (bufsize > CDR_MAX_BUF_SIZE)
		bufsize = CDR_MAX_BUF_SIZE;

	if (debug) {
		printf(_("dev: '%s' speed: %d fs: %ld driveropts '%s'\n"),
					dev, speed, fs, driveropts);
	}
	if (speed >= 0)
		*speedp = speed;

	if (fs < 0L)
		fs = DEFAULT_FIFOSIZE;
	if (fs < 2*bufsize) {
		errmsgno(EX_BAD, _("Fifo size %ld too small, turning fifo off.\n"), fs);
		fs = 0L;
	}

	if (dev != *devp && (*flagsp & F_SCANBUS) == 0)
		*devp = dev;

#ifdef	NO_SEARCH_FOR_DEVICE
	if (!*devp && (*flagsp & (F_VERSION|F_SCANBUS)) == 0) {
		errmsgno(EX_BAD, _("No CD/DVD/BD-Recorder device specified.\n"));
		susage(EX_BAD);
	}
#endif
	if (*devp &&
	    ((strncmp(*devp, "HELP", 4) == 0) ||
	    (strncmp(*devp, "help", 4) == 0))) {
		*flagsp |= F_CHECKDRIVE; /* Set this for not calling mlockall() */
		return;
	}
	if (*flagsp &
	    (F_LOAD|F_DLCK|F_SETDROPTS|F_MEDIAINFO|F_MSINFO|F_TOC|F_PRATIP|
	    F_FIX|F_VERSION|
	    F_CHECKDRIVE|F_PRCAP|F_INQUIRY|F_SCANBUS|F_RESET|F_ABORT)) {
		if (tracks != 0) {
			errmsgno(EX_BAD, _("No tracks allowed with this option.\n"));
			susage(EX_BAD);
		}
		return;
	}
	*tracksp = tracks;
	if (*flagsp & F_SAO) {
		/*
		 * Make sure that you change WRITER_MAXWAIT & READER_MAXWAIT
		 * too if you change this timeout.
		 */
		if (*timeoutp < 200)		/* Lead in size is 2:30 */
			*timeoutp = 200;	/* 200s is 150s *1.33	*/
	}
	if (usetext) {
		trackp[MAX_TRACK+1].flags |= TI_TEXT;
	}
	if (cuefile) {
		if (tracks == 0 && (wm == 0))
			flags = default_wr_mode(tracks, trackp, flagsp, &wm, flags);

		if ((*flagsp & F_SAO) == 0 &&
		    (*flagsp & F_RAW) == 0) {
			errmsgno(EX_BAD, _("The cuefile= option only works with -sao/-raw.\n"));
			susage(EX_BAD);
		}
		if (pad)
			trackp[0].flags |= TI_PAD;
		if (tracks > 0) {
			errmsgno(EX_BAD, _("No tracks allowed with the cuefile= option.\n"));
			susage(EX_BAD);
		}
		cuefilename = cuefile;
		return;
	}
	if (tracks == 0 && (*flagsp & (F_LOAD|F_DLCK|F_EJECT|F_BLANK|F_FORMAT)) == 0) {
		errmsgno(EX_BAD, _("No tracks specified. Need at least one.\n"));
		susage(EX_BAD);
	}
}

LOCAL int
default_wr_mode(tracks, trackp, flagsp, wmp, flags)
	int	tracks;
	track_t	*trackp;
	UInt32_t *flagsp;
	int	*wmp;
	int	flags;
{
	if (tracks == 0 && (*wmp == 0)) {
		errmsgno(EX_BAD, _("No write mode specified.\n"));
		errmsgno(EX_BAD, _("Assuming %s mode.\n"),
				(*flagsp & F_MULTI)?"-tao":"-sao");
		if ((*flagsp & F_MULTI) == 0)
			errmsgno(EX_BAD, _("If your drive does not accept -sao, try -tao.\n"));
		errmsgno(EX_BAD, _("Future versions of cdrecord may have different drive dependent defaults.\n"));
		if (*flagsp & F_MULTI) {
			*wmp |= M_TAO;
		} else {
			*flagsp |= F_SAO;
			trackp[0].flags &= ~TI_TAO;
			trackp[0].flags |= TI_SAO;
			flags &= ~TI_TAO;
			flags |= TI_SAO;
			*wmp |= M_SAO;
		}
	}
	return (flags);
}

LOCAL void
etracks(opt)
	char	*opt;
{
	errmsgno(EX_BAD, _("No tracks allowed with '-%s'.\n"), opt);
	susage(EX_BAD);
}

LOCAL void
set_trsizes(dp, tracks, trackp)
	cdr_t	*dp;
	int	tracks;
	track_t	*trackp;
{
	int	i;
	int	secsize;
	int	secspt;

	trackp[1].flags		|= TI_FIRST;
	trackp[tracks].flags	|= TI_LAST;

	if (xdebug)
		printf(_("Set Transfersizes start\n"));
	for (i = 0; i <= tracks+1; i++) {
		if ((dp->cdr_flags & CDR_SWABAUDIO) != 0 &&
					is_audio(&trackp[i])) {
			trackp[i].flags ^= TI_SWAB;
		}
		if (!is_audio(&trackp[i]))
			trackp[i].flags &= ~TI_SWAB;	/* Only swab audio  */

		/*
		 * Use the biggest sector size to compute how many
		 * sectors may fit into one single DMA buffer.
		 */
		secsize = trackp[i].secsize;
		if (trackp[i].isecsize > secsize)
			secsize = trackp[i].isecsize;

		/*
		 * We are using SCSI Group 0 write
		 * and cannot write more than 255 secs at once.
		 */
		secspt = bufsize/secsize;
		secspt = min(255, secspt);
		trackp[i].secspt = secspt;

		if (is_packet(&trackp[i]) && trackp[i].pktsize > 0) {
			if (trackp[i].secspt >= trackp[i].pktsize) {
				trackp[i].secspt = trackp[i].pktsize;
			} else {
				comerrno(EX_BAD,
					_("Track %d packet size %d exceeds buffer limit of %d sectors"),
					i, trackp[i].pktsize, trackp[i].secspt);
			}
		}
		if (xdebug) {
			printf(_("Track %d flags %X secspt %d secsize: %d isecsize: %d\n"),
				i, trackp[i].flags, trackp[i].secspt,
				trackp[i].secsize, trackp[i].isecsize);
		}
	}
	if (xdebug)
		printf(_("Set Transfersizes end\n"));
}

EXPORT void
load_media(scgp, dp, doexit)
	SCSI	*scgp;
	cdr_t	*dp;
	BOOL	doexit;
{
	int	code;
	int	key;
	int	err;
	BOOL	immed = (dp->cdr_cmdflags&F_IMMED) != 0;

	/*
	 * Do some preparation before...
	 */
	scgp->silent++;			/* Be quiet if this fails		*/
	test_unit_ready(scgp);		/* First eat up unit attention		*/
	if ((*dp->cdr_load)(scgp, dp) < 0) {	/* now try to load media and	*/
		if (!doexit)
			return;
		comerrno(EX_BAD, _("Cannot load media.\n"));
	}
	scsi_start_stop_unit(scgp, 1, 0, immed); /* start unit in silent mode	*/
	scgp->silent--;

	if (!wait_unit_ready(scgp, 60)) {
		code = scg_sense_code(scgp);
		key = scg_sense_key(scgp);
		scgp->silent++;
		scsi_prevent_removal(scgp, 0); /* In case someone locked it */
		scgp->silent--;

		if (!doexit)
			return;
		if (key == SC_NOT_READY && (code == 0x3A || code == 0x30))
			comerrno(EX_BAD, _("No disk / Wrong disk!\n"));
		comerrno(EX_BAD, _("CD/DVD/BD-Recorder not ready.\n"));
	}

	scsi_prevent_removal(scgp, 1);
	scsi_start_stop_unit(scgp, 1, 0, immed);
	wait_unit_ready(scgp, 120);
	/*
	 * Linux-2.6.8.1 did break the SCSI pass through kernel interface.
	 * Since then, many SCSI commands are filtered away by the Linux kernel
	 * if we do not have root privilleges. Since REZERO UNIT is in the list
	 * of filtered SCSI commands, it is a good indicator on whether we run
	 * with the needed privileges. Failing here is better than failing later
	 * with e.g. vendor unique commands, where cdrecord would miss
	 * functionality or fail completely after starting to write.
	 */
	seterrno(0);
	scgp->silent++;
	code = rezero_unit(scgp); /* Not supported by some drives */
	scgp->silent--;
	err = geterrno();
	if (code < 0 && (err == EPERM || err == EACCES)) {
		linuxcheck();	/* For version 1.415 of cdrecord.c */
		scg_openerr("");
	}


	test_unit_ready(scgp);
	scsi_start_stop_unit(scgp, 1, 0, immed);
	wait_unit_ready(scgp, 120);
}

EXPORT void
unload_media(scgp, dp, flags)
	SCSI	*scgp;
	cdr_t	*dp;
	UInt32_t flags;
{
	scsi_prevent_removal(scgp, 0);
	if ((flags & F_EJECT) != 0) {
		if ((*dp->cdr_unload)(scgp, dp) < 0)
			errmsgno(EX_BAD, _("Cannot eject media.\n"));
	}
}

EXPORT void
reload_media(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	char	ans[2];
#ifdef	F_GETFL
	int	f = -1;
#endif

	errmsgno(EX_BAD, _("Drive needs to reload the media to return to proper status.\n"));
	unload_media(scgp, dp, F_EJECT);

	/*
	 * Note that even Notebook drives identify as CDR_TRAYLOAD
	 */
	if ((dp->cdr_flags & CDR_TRAYLOAD) != 0) {
		scgp->silent++;
		load_media(scgp, dp, FALSE);
		scgp->silent--;
	}

	scgp->silent++;
	if (((dp->cdr_flags & CDR_TRAYLOAD) == 0) ||
				!wait_unit_ready(scgp, 5)) {
		static FILE	*tty = NULL;

		printf(_("Re-load disk and hit <CR>"));
		if (isgui)
			printf("\n");
		flush();

		if (tty == NULL) {
			tty = stdin;
			if ((dp->cdr_cmdflags & F_STDIN) != 0)
				tty = fileluopen(STDERR_FILENO, "rw");
		}
#ifdef	F_GETFL
		if (tty != NULL)
			f = fcntl(fileno(tty), F_GETFL, 0);
		if (f < 0 || (f & O_ACCMODE) == O_WRONLY) {
#ifdef	SIGUSR1
			signal(SIGUSR1, catchsig);
			printf(_("Controlling file not open for reading, send SIGUSR1 to continue.\n"));
			flush();
			pause();
#endif
		} else
#endif
		if (fgetline(tty, ans, 1) < 0)
			comerrno(EX_BAD, _("Aborted by EOF on input.\n"));
	}
	scgp->silent--;

	load_media(scgp, dp, TRUE);
}

EXPORT void
set_secsize(scgp, secsize)
	SCSI	*scgp;
	int	secsize;
{
	if (secsize > 0) {
		/*
		 * Try to restore the old sector size.
		 */
		scgp->silent++;
		select_secsize(scgp, secsize);
		scgp->silent--;
	}
}

/*
 * Carefully read the drive buffer.
 * Work around the inability to get the DMA residual count on Linux.
 */
LOCAL int
_read_buffer(scgp, size)
	SCSI	*scgp;
	int	size;
{
	buf[size-2] = (char)0x55; buf[size-1] = (char)0xFF;
	if (read_buffer(scgp, buf, size, 0) < 0 ||
	    scg_getresid(scgp) != 0) {
		errmsgno(EX_BAD,
			_("Warning: 'read buffer' failed, DMA residual count %d.\n"),
			scg_getresid(scgp));
		return (-1);
	}
	/*
	 * Work around unfriendly OS that do not return the
	 * DMA residual count (e.g. Linux).
	 */
	if (buf[size-2] == (char)0x55 || buf[size-1] == (char)0xFF) {
		errmsgno(EX_BAD,
		_("Warning: DMA resid 0 for 'read buffer', actual data is too short.\n"));
		return (-1);
	}
	return (0);
}

LOCAL int
get_dmaspeed(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	int	i;
	long	t;
	int	bs;
	int	tsize;
	int	maxdma;

	fillbytes((caddr_t)buf, 4, '\0');
	tsize = 0;
	scgp->silent++;
	i = read_buffer(scgp, buf, 4, 0);
	scgp->silent--;
	if (i < 0 || scg_getresid(scgp) != 0) {
		errmsgno(EX_BAD, _("Warning: Cannot read drive buffer.\n"));
		return (-1);
	}
	tsize = a_to_u_3_byte(&buf[1]);
	if (tsize <= 0) {
		errmsgno(EX_BAD, _("Warning: Drive returned invalid buffer size.\n"));
		return (-1);
	}

	bs = bufsize;
	if (tsize < bs)
		bs = tsize;
	if (bs > 0xFFFE)		/* ReadBuffer may hang w. >64k & USB */
		bs = 0xFFFE;		/* Make it an even size < 64k	    */

	scgp->silent++;
	fillbytes((caddr_t)buf, bs, '\0');
	if (read_buffer(scgp, buf, bs, 0) < 0) {
		scgp->silent--;
		errmsgno(EX_BAD,
			_("Warning: Cannot read %d bytes from drive buffer.\n"),
			bs);
		return (-1);
	}
	for (i = bs-1; i >= 0; i--) {
		if (buf[i] != '\0') {
			break;
		}
	}
	i++;
	maxdma = i;
	fillbytes((caddr_t)buf, bs, 0xFF);
	if (read_buffer(scgp, buf, bs, 0) < 0) {
		scgp->silent--;
		return (-1);
	}
	scgp->silent--;
	for (i = bs-1; i >= 0; i--) {
		if ((buf[i] & 0xFF) != 0xFF) {
			break;
		}
	}
	i++;
	if (i > maxdma)
		maxdma = i;
	if (maxdma < bs && (scg_getresid(scgp) != (bs - maxdma))) {
		errmsgno(EX_BAD, _("Warning: OS does not return a correct DMA residual count.\n"));
		errmsgno(EX_BAD, _("Warning: expected DMA residual count %d but got %d.\n"),
			(bs - maxdma), scg_getresid(scgp));
	}
	/*
	 * Some drives (e.g. 'HL-DT-ST' 'DVD-RAM GSA-H55N') are unreliable.
	 * They return less data than advertized as buffersize (tsize).
	 */
	if (maxdma < bs) {
		errmsgno(EX_BAD, _("Warning: drive returns unreliable data from 'read buffer'.\n"));
		return (-1);
	}
	if (maxdma < bs)
		bs = maxdma;

	scgp->silent++;
	if (_read_buffer(scgp, bs) < 0) {
		scgp->silent--;
		return (-1);
	}
	scgp->silent--;

	if (gettimeofday(&starttime, (struct timezone *)0) < 0) {
		errmsg(_("Cannot get DMA start time.\n"));
		return (-1);
	}

	for (i = 0; i < 100; i++) {
		if (_read_buffer(scgp, bs) < 0)
			return (-1);
	}
	if (gettimeofday(&fixtime, (struct timezone *)0) < 0) {
		errmsg(_("Cannot get DMA stop time.\n"));
		return (-1);
	}
	timevaldiff(&starttime, &fixtime);
	tsize = bs * 100;
	t = fixtime.tv_sec * 1000 + fixtime.tv_usec / 1000;
	if (t <= 0)
		return (-1);
#ifdef	DEBUG
	error("Read Speed: %lu %ld %ld kB/s %ldx CD %ldx DVD %ldx BD\n",
		tsize, t, tsize/t, tsize/t/176, tsize/t/1385, tsize/t/4495);
#endif

	return (tsize/t);
}


LOCAL BOOL
do_opc(scgp, dp, flags)
	SCSI	*scgp;
	cdr_t	*dp;
	UInt32_t flags;
{
	if ((flags & F_DUMMY) == 0 && dp->cdr_opc) {
		if (debug || lverbose) {
			printf(_("Performing OPC...\n"));
			flush();
		}
		if (dp->cdr_opc(scgp, NULL, 0, TRUE) < 0) {
			errmsgno(EX_BAD, _("OPC failed.\n"));
			if ((flags & F_FORCE) == 0)
				return (FALSE);
		}
	}
	return (TRUE);
}

LOCAL void
check_recovery(scgp, dp, flags)
	SCSI	*scgp;
	cdr_t	*dp;
	UInt32_t flags;
{
	if ((*dp->cdr_check_recovery)(scgp, dp)) {
		errmsgno(EX_BAD, _("Recovery needed.\n"));
		unload_media(scgp, dp, flags);
		comexit(EX_BAD);
	}
}

#ifndef	DEBUG
#define	DEBUG
#endif
void
audioread(scgp, dp, flags)
	SCSI	*scgp;
	cdr_t	*dp;
	int	flags;
{
#ifdef	DEBUG
	int speed = 1;
	int	oflags = dp->cdr_cmdflags;

	dp->cdr_cmdflags &= ~F_DUMMY;
	if ((*dp->cdr_set_speed_dummy)(scgp, dp, &speed) < 0)
		comexit(-1);
	dp->cdr_dstat->ds_wspeed = speed; /* XXX Remove 'speed' in future */
	dp->cdr_cmdflags = oflags;

	if ((*dp->cdr_set_secsize)(scgp, 2352) < 0)
		comexit(-1);
	scgp->cap->c_bsize = 2352;

	read_scsi(scgp, buf, 1000, 1);
	printf("XXX:\n");
	write(1, buf, 512);
	unload_media(scgp, dp, flags);
	comexit(0);
#endif
}

LOCAL void
print_msinfo(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	long	off = 0;
	long	fa = 0;

	if ((*dp->cdr_session_offset)(scgp, &off) < 0) {
		errmsgno(EX_BAD, _("Cannot read session offset.\n"));
		return;
	}
	if (lverbose)
		printf(_("session offset: %ld\n"), off);

	/*
	 * Set the track pointer to NULL in order to signal the driver that we
	 * like to get the next writable address for the next (unwritten)
	 * session.
	 */
	if (dp->cdr_next_wr_address(scgp, (track_t *)0, &fa) < 0) {
		errmsgno(EX_BAD, _("Cannot read first writable address.\n"));
		return;
	}
	printf("%ld,%ld\n", off, fa);
}

LOCAL void
print_toc(scgp, dp)
	SCSI	*scgp;
	cdr_t	*dp;
{
	int	first;
	int	last;
	long	lba;
	long	xlba;
	struct msf msf;
	int	adr;
	int	control;
	int	mode;
	int	i;

	scgp->silent++;
	if (read_capacity(scgp) < 0) {
		scgp->silent--;
		errmsgno(EX_BAD, _("Cannot read capacity.\n"));
		return;
	}
	scgp->silent--;
	if (read_tochdr(scgp, dp, &first, &last) < 0) {
		errmsgno(EX_BAD, _("Cannot read TOC/PMA.\n"));
		return;
	}
	printf(_("first: %d last %d\n"), first, last);
	for (i = first; i <= last; i++) {
		read_trackinfo(scgp, i, &lba, &msf, &adr, &control, &mode);
		xlba = -150 +
			msf.msf_frame + (75*msf.msf_sec) + (75*60*msf.msf_min);
		if (xlba == lba/4)
			lba = xlba;
		print_track(i, lba, &msf, adr, control, mode);
	}
	i = 0xAA;
	read_trackinfo(scgp, i, &lba, &msf, &adr, &control, &mode);
	xlba = -150 +
		msf.msf_frame + (75*msf.msf_sec) + (75*60*msf.msf_min);
	if (xlba == lba/4)
		lba = xlba;
	print_track(i, lba, &msf, adr, control, mode);
	if (lverbose > 1) {
		scgp->silent++;
		if (read_cdtext(scgp) < 0)
			errmsgno(EX_BAD, _("No CD-Text or CD-Text unaware drive.\n"));
		scgp->silent++;
	}
}

LOCAL void
print_track(track, lba, msp, adr, control, mode)
	int	track;
	long	lba;
	struct msf *msp;
	int	adr;
	int	control;
	int	mode;
{
	long	lba_512 = lba*4;

	if (track == 0xAA)
		printf(_("track:lout "));
	else
		printf(_("track: %3d "), track);

	printf("lba: %9ld (%9ld) %02d:%02d:%02d adr: %X control: %X mode: %d\n",
			lba, lba_512,
			msp->msf_min,
			msp->msf_sec,
			msp->msf_frame,
			adr, control, mode);
}

#ifdef	HAVE_SYS_PRIOCNTL_H	/* The preferred SYSvR4 schduler */

#include <sys/procset.h>	/* Needed for SCO Openserver */
#include <sys/priocntl.h>
#include <sys/rtpriocntl.h>

EXPORT	void
raisepri(pri)
	int pri;
{
	int		pid;
	int		classes;
	int		ret;
	pcinfo_t	info;
	pcparms_t	param;
	rtinfo_t	rtinfo;
	rtparms_t	rtparam;

	pid = getpid();

	/* get info */
	strcpy(info.pc_clname, "RT");
	classes = priocntl(P_PID, pid, PC_GETCID, (void *)&info);
	if (classes == -1)
		comerr(_("Cannot get priority class id priocntl(PC_GETCID).\n"));

	movebytes(info.pc_clinfo, &rtinfo, sizeof (rtinfo_t));

	/* set priority to max */
	rtparam.rt_pri = rtinfo.rt_maxpri - pri;
	rtparam.rt_tqsecs = 0;
	rtparam.rt_tqnsecs = RT_TQDEF;
	param.pc_cid = info.pc_cid;
	movebytes(&rtparam, param.pc_clparms, sizeof (rtparms_t));
	ret = priocntl(P_PID, pid, PC_SETPARMS, (void *)&param);
	if (ret == -1) {
		errmsg(_("WARNING: Cannot set priority class parameters priocntl(PC_SETPARMS).\n"));
		errmsgno(EX_BAD, _("WARNING: This causes a high risk for buffer underruns.\n"));
	}
}

#else	/* !HAVE_SYS_PRIOCNTL_H */

#ifdef	USE_POSIX_PRIORITY_SCHEDULING
/*
 * The second best choice: POSIX real time scheduling.
 */
/*
 * XXX Ugly but needed because of a typo in /usr/iclude/sched.h on Linux.
 * XXX This should be removed as soon as we are sure that Linux-2.0.29 is gone.
 */
#ifdef	__linux
#define	_P	__P
#endif

#include <sched.h>

#ifdef	__linux
#undef	_P
#endif

LOCAL	int
rt_raisepri(pri)
	int pri;
{
	struct sched_param scp;

	/*
	 * Verify that scheduling is available
	 */
#ifdef	_SC_PRIORITY_SCHEDULING
	if (sysconf(_SC_PRIORITY_SCHEDULING) == -1) {
		errmsg(_("WARNING: RR-scheduler not available, disabling.\n"));
		return (-1);
	}
#endif
	fillbytes(&scp, sizeof (scp), '\0');
	scp.sched_priority = sched_get_priority_max(SCHED_RR) - pri;
	if (sched_setscheduler(0, SCHED_RR, &scp) < 0) {
		errmsg(_("WARNING: Cannot set RR-scheduler.\n"));
		return (-1);
	}
	return (0);
}

#else	/* !USE_POSIX_PRIORITY_SCHEDULING */

#if defined(__CYGWIN32__) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(_MSC_VER)
/*
 * Win32 specific priority settings.
 */
/*
 * NOTE: Base.h from Cygwin-B20 has a second typedef for BOOL.
 *	 We define BOOL to make all local code use BOOL
 *	 from Windows.h and use the hidden __SBOOL for
 *	 our global interfaces.
 *
 * NOTE: windows.h from Cygwin-1.x includes a structure field named sample,
 *	 so me may not define our own 'sample' or need to #undef it now.
 *	 With a few nasty exceptions, Microsoft assumes that any global
 *	 defines or identifiers will begin with an Uppercase letter, so
 *	 there may be more of these problems in the future.
 *
 * NOTE: windows.h defines interface as an alias for struct, this
 *	 is used by COM/OLE2, I guess it is class on C++
 *	 We man need to #undef 'interface'
 *
 *	 These workarounds are now applied in schily/windows.h
 */
#include <schily/windows.h>
#undef interface

LOCAL	int
rt_raisepri(pri)
	int pri;
{
	int prios[] = {THREAD_PRIORITY_TIME_CRITICAL, THREAD_PRIORITY_HIGHEST};

	/* set priority class */
	if (SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS) == FALSE) {
		errmsgno(EX_BAD, _("No realtime priority class possible.\n"));
		return (-1);
	}

	/* set thread priority */
	if (pri >= 0 && pri <= 1 && SetThreadPriority(GetCurrentThread(), prios[pri]) == FALSE) {
		errmsgno(EX_BAD, _("Could not set realtime priority.\n"));
		return (-1);
	}
	return (0);
}

#else
/*
 * This OS does not support real time scheduling.
 */
LOCAL	int
rt_raisepri(pri)
	int pri;
{
	return (-1);
}

#endif	/* __CYGWIN32__ || __CYGWIN__ || __MINGW32__ */

#endif	/* !USE_POSIX_PRIORITY_SCHEDULING */

EXPORT	void
raisepri(pri)
	int pri;
{
	if (rt_raisepri(pri) >= 0)
		return;
#if	defined(HAVE_SETPRIORITY) && defined(PRIO_PROCESS)

	if (setpriority(PRIO_PROCESS, getpid(), -20 + pri) < 0) {
		errmsg(_("WARNING: Cannot set priority using setpriority().\n"));
		errmsgno(EX_BAD, _("WARNING: This causes a high risk for buffer underruns.\n"));
	}
#else
#ifdef	HAVE_DOSSETPRIORITY	/* RT priority on OS/2 */
	/*
	 * Set priority to timecritical 31 - pri (arg)
	 */
	DosSetPriority(0, 3, 31, 0);
	DosSetPriority(0, 3, -pri, 0);
#else
#if	defined(HAVE_NICE) && !defined(__DJGPP__) /* DOS has nice but no multitasking */
	if (nice(-NZERO + pri) == -1) {
		errmsg(_("WARNING: Cannot set priority using nice().\n"));
		errmsgno(EX_BAD, _("WARNING: This causes a high risk for buffer underruns.\n"));
	}
#else
	errmsgno(EX_BAD, _("WARNING: Cannot set priority on this OS.\n"));
	errmsgno(EX_BAD, _("WARNING: This causes a high risk for buffer underruns.\n"));
#endif
#endif
#endif
}

#endif	/* HAVE_SYS_PRIOCNTL_H */

#ifdef	HAVE_SELECT
/*
 * sys/types.h and sys/time.h are already included.
 */
#else	/* !HAVE_SELECT */
#ifdef	HAVE_STROPTS_H
#	include	<stropts.h>
#endif
#ifdef	HAVE_POLL_H
#	include	<poll.h>
#endif

#ifndef	INFTIM
#define	INFTIM	(-1)
#endif
#endif	/* !HAVE_SELECT */

#include <schily/select.h>

LOCAL void
wait_input()
{
#ifdef	HAVE_SELECT
	fd_set	in;
#else
#ifdef	HAVE_POLL
	struct pollfd pfd;
#endif
#endif
	if (lverbose)
		printf(_("Waiting for data on stdin...\n"));
#ifdef	HAVE_SELECT
	FD_ZERO(&in);
	FD_SET(STDIN_FILENO, &in);
	select(1, &in, NULL, NULL, 0);
#else
#ifdef	HAVE_POLL
	pfd.fd = STDIN_FILENO;
	pfd.events = POLLIN;
	pfd.revents = 0;
	poll(&pfd, (unsigned long)1, INFTIM);
#endif
#endif
}

LOCAL void
checkgui()
{
	struct stat st;

	if (fstat(STDERR_FILENO, &st) >= 0 && !S_ISCHR(st.st_mode)) {
		isgui = TRUE;
		if (lverbose > 1)
			printf(_("Using remote (pipe) mode for interactive i/o.\n"));
	}
}

LOCAL int
getbltype(optstr, typep)
	char	*optstr;
	long	*typep;
{
	if (streql(optstr, "all")) {
		*typep = BLANK_DISC;
	} else if (streql(optstr, "disc")) {
		*typep = BLANK_DISC;
	} else if (streql(optstr, "disk")) {
		*typep = BLANK_DISC;
	} else if (streql(optstr, "fast")) {
		*typep = BLANK_MINIMAL;
	} else if (streql(optstr, "minimal")) {
		*typep = BLANK_MINIMAL;
	} else if (streql(optstr, "track")) {
		*typep = BLANK_TRACK;
	} else if (streql(optstr, "unreserve")) {
		*typep = BLANK_UNRESERVE;
	} else if (streql(optstr, "trtail")) {
		*typep = BLANK_TAIL;
	} else if (streql(optstr, "unclose")) {
		*typep = BLANK_UNCLOSE;
	} else if (streql(optstr, "session")) {
		*typep = BLANK_SESSION;
	} else if (streql(optstr, "help")) {
		blusage(0);
	} else {
		error(_("Illegal blanking type '%s'.\n"), optstr);
		blusage(EX_BAD);
		return (-1);
	}
	return (TRUE);
}

LOCAL void
print_drflags(dp)
	cdr_t	*dp;
{
	printf(_("Driver flags   : "));

	if ((dp->cdr_flags2 & CDR2_NOCD) != 0)
		printf("NO-CD ");
	if ((dp->cdr_flags2 & CDR2_BD) != 0)
		printf("BD ");
	if ((dp->cdr_flags & CDR_DVD) != 0)
		printf("DVD ");

	if ((dp->cdr_flags & CDR_MMC3) != 0)
		printf("MMC-3 ");
	else if ((dp->cdr_flags & CDR_MMC2) != 0)
		printf("MMC-2 ");
	else if ((dp->cdr_flags & CDR_MMC) != 0)
		printf("MMC ");

	if ((dp->cdr_flags & CDR_SWABAUDIO) != 0)
		printf("SWABAUDIO ");
	if ((dp->cdr_flags & CDR_BURNFREE) != 0)
		printf("BURNFREE ");
	if ((dp->cdr_flags & CDR_VARIREC) != 0)
		printf("VARIREC ");
	if ((dp->cdr_flags & CDR_GIGAREC) != 0)
		printf("GIGAREC ");
	if ((dp->cdr_flags & CDR_AUDIOMASTER) != 0)
		printf("AUDIOMASTER ");
	if ((dp->cdr_flags & CDR_FORCESPEED) != 0)
		printf("FORCESPEED ");
	if ((dp->cdr_flags & CDR_SPEEDREAD) != 0)
		printf("SPEEDREAD ");
	if ((dp->cdr_flags & CDR_DISKTATTOO) != 0)
		printf("DISKTATTOO ");
	if ((dp->cdr_flags & CDR_SINGLESESS) != 0)
		printf("SINGLESESSION ");
	if ((dp->cdr_flags & CDR_HIDE_CDR) != 0)
		printf("HIDECDR ");
	printf("\n");
}

LOCAL void
print_wrmodes(dp)
	cdr_t	*dp;
{
	BOOL	needblank = FALSE;

	printf("Supported modes: ");
	if ((dp->cdr_flags & CDR_TAO) != 0) {
		printf("TAO");
		needblank = TRUE;
	}
	if ((dp->cdr_flags & CDR_PACKET) != 0) {
		printf("%sPACKET", needblank?" ":"");
		needblank = TRUE;
	}
	if ((dp->cdr_flags & CDR_SAO) != 0) {
		printf("%sSAO", needblank?" ":"");
		needblank = TRUE;
	}
#ifdef	__needed__
	if ((dp->cdr_flags & (CDR_SAO|CDR_SRAW16)) == (CDR_SAO|CDR_SRAW16)) {
		printf("%sSAO/R16", needblank?" ":"");
		needblank = TRUE;
	}
#endif
	if ((dp->cdr_flags & (CDR_SAO|CDR_SRAW96P)) == (CDR_SAO|CDR_SRAW96P)) {
		printf("%sSAO/R96P", needblank?" ":"");
		needblank = TRUE;
	}
	if ((dp->cdr_flags & (CDR_SAO|CDR_SRAW96R)) == (CDR_SAO|CDR_SRAW96R)) {
		printf("%sSAO/R96R", needblank?" ":"");
		needblank = TRUE;
	}
	if ((dp->cdr_flags & (CDR_RAW|CDR_RAW16)) == (CDR_RAW|CDR_RAW16)) {
		printf("%sRAW/R16", needblank?" ":"");
		needblank = TRUE;
	}
	if ((dp->cdr_flags & (CDR_RAW|CDR_RAW96P)) == (CDR_RAW|CDR_RAW96P)) {
		printf("%sRAW/R96P", needblank?" ":"");
		needblank = TRUE;
	}
	if ((dp->cdr_flags & (CDR_RAW|CDR_RAW96R)) == (CDR_RAW|CDR_RAW96R)) {
		printf("%sRAW/R96R", needblank?" ":"");
		needblank = TRUE;
	}
	if ((dp->cdr_flags & CDR_LAYER_JUMP) == CDR_LAYER_JUMP) {
		printf("%sLAYER_JUMP", needblank?" ":"");
		needblank = TRUE;
	}
	printf("\n");
}

LOCAL BOOL
check_wrmode(dp, wmode, tflags)
	cdr_t	*dp;
	UInt32_t wmode;
	int	tflags;
{
	int	cdflags = dp->cdr_flags;

	if ((tflags & TI_PACKET) != 0 && (cdflags & CDR_PACKET) == 0) {
		errmsgno(EX_BAD, _("Drive does not support PACKET recording.\n"));
		return (FALSE);
	}
	if ((tflags & TI_TAO) != 0 && (cdflags & CDR_TAO) == 0) {
		errmsgno(EX_BAD, _("Drive does not support TAO recording.\n"));
		return (FALSE);
	}
	if ((wmode & F_SAO) != 0) {
		if ((cdflags & CDR_SAO) == 0) {
			errmsgno(EX_BAD, _("Drive does not support SAO recording.\n"));
			if ((cdflags & CDR_RAW) != 0)
				errmsgno(EX_BAD, _("Try -raw option.\n"));
			return (FALSE);
		}
#ifdef	__needed__
		if ((tflags & TI_RAW16) != 0 && (cdflags & CDR_SRAW16) == 0) {
			errmsgno(EX_BAD, _("Drive does not support SAO/RAW16.\n"));
			goto badsecs;
		}
#endif
		if ((tflags & (TI_RAW|TI_RAW16|TI_RAW96R)) == TI_RAW && (cdflags & CDR_SRAW96P) == 0) {
			errmsgno(EX_BAD, _("Drive does not support SAO/RAW96P.\n"));
			goto badsecs;
		}
		if ((tflags & (TI_RAW|TI_RAW16|TI_RAW96R)) == (TI_RAW|TI_RAW96R) && (cdflags & CDR_SRAW96R) == 0) {
			errmsgno(EX_BAD, _("Drive does not support SAO/RAW96R.\n"));
			goto badsecs;
		}
	}
	if ((wmode & F_RAW) != 0) {
		if ((cdflags & CDR_RAW) == 0) {
			errmsgno(EX_BAD, _("Drive does not support RAW recording.\n"));
			return (FALSE);
		}
		if ((tflags & TI_RAW16) != 0 && (cdflags & CDR_RAW16) == 0) {
			errmsgno(EX_BAD, _("Drive does not support RAW/RAW16.\n"));
			goto badsecs;
		}
		if ((tflags & (TI_RAW|TI_RAW16|TI_RAW96R)) == TI_RAW && (cdflags & CDR_RAW96P) == 0) {
			errmsgno(EX_BAD, _("Drive does not support RAW/RAW96P.\n"));
			goto badsecs;
		}
		if ((tflags & (TI_RAW|TI_RAW16|TI_RAW96R)) == (TI_RAW|TI_RAW96R) && (cdflags & CDR_RAW96R) == 0) {
			errmsgno(EX_BAD, _("Drive does not support RAW/RAW96R.\n"));
			goto badsecs;
		}
	}
	return (TRUE);

badsecs:
	if ((wmode & F_SAO) != 0)
		cdflags &= ~(CDR_RAW16|CDR_RAW96P|CDR_RAW96R);
	if ((wmode & F_RAW) != 0)
		cdflags &= ~(CDR_SRAW96P|CDR_SRAW96R);

	if ((cdflags & (CDR_SRAW96R|CDR_RAW96R)) != 0)
		errmsgno(EX_BAD, _("Try -raw96r option.\n"));
	else if ((cdflags & (CDR_SRAW96P|CDR_RAW96P)) != 0)
		errmsgno(EX_BAD, _("Try -raw96p option.\n"));
	else if ((cdflags & CDR_RAW16) != 0)
		errmsgno(EX_BAD, _("Try -raw16 option.\n"));
	return (FALSE);
}

LOCAL void
set_wrmode(dp, wmode, tflags)
	cdr_t	*dp;
	UInt32_t wmode;
	int	tflags;
{
	dstat_t	*dsp = dp->cdr_dstat;

	if ((tflags & TI_PACKET) != 0) {
		dsp->ds_wrmode = WM_PACKET;
		return;
	}
	if ((tflags & TI_TAO) != 0) {
		dsp->ds_wrmode = WM_TAO;
		return;
	}
	if ((wmode & F_SAO) != 0) {
		if ((tflags & (TI_RAW|TI_RAW16|TI_RAW96R)) == 0) {
			dsp->ds_wrmode = WM_SAO;
			return;
		}
		if ((tflags & TI_RAW16) != 0) {		/* Is this needed? */
			dsp->ds_wrmode = WM_SAO_RAW16;
			return;
		}
		if ((tflags & (TI_RAW|TI_RAW16|TI_RAW96R)) == TI_RAW) {
			dsp->ds_wrmode = WM_SAO_RAW96P;
			return;
		}
		if ((tflags & (TI_RAW|TI_RAW16|TI_RAW96R)) == (TI_RAW|TI_RAW96R)) {
			dsp->ds_wrmode = WM_SAO_RAW96R;
			return;
		}
	}
	if ((wmode & F_RAW) != 0) {
		if ((tflags & TI_RAW16) != 0) {
			dsp->ds_wrmode = WM_RAW_RAW16;
			return;
		}
		if ((tflags & (TI_RAW|TI_RAW16|TI_RAW96R)) == TI_RAW) {
			dsp->ds_wrmode = WM_RAW_RAW96P;
			return;
		}
		if ((tflags & (TI_RAW|TI_RAW16|TI_RAW96R)) == (TI_RAW|TI_RAW96R)) {
			dsp->ds_wrmode = WM_RAW_RAW96R;
			return;
		}
	}
	dsp->ds_wrmode = WM_NONE;
}

/*
 * I am sorry that even for version 1.415 of cdrecord.c, I am forced to do
 * things like this, but defective versions of cdrecord cause a lot of
 * work load to me.
 *
 * Note that the intention to have non bastardized versions.
 *
 * It is bad to see that in special in the "Linux" business, companies
 * prefer a model with many proprietary differing programs
 * instead of cooperating with the program authors.
 */
#if	defined(linux) || defined(__linux) || defined(__linux__)
#ifdef	HAVE_UNAME
#include <schily/utsname.h>
#endif
#endif

LOCAL void
linuxcheck()				/* For version 1.415 of cdrecord.c */
{
#if	defined(linux) || defined(__linux) || defined(__linux__)
#ifdef	HAVE_UNAME
	struct	utsname	un;

	if (uname(&un) >= 0) {
		if ((un.release[0] == '2' && un.release[1] == '.') &&
		    (un.release[2] > '6' ||
		    (un.release[2] == '6' && un.release[3] == '.' && un.release[4] >= '8'))) {
			errmsgno(EX_BAD,
			_("Warning: Linux-2.6.8 introduced incompatible interface changes.\n"));
			errmsgno(EX_BAD,
			_("Warning: SCSI transport only works for suid root programs.\n"));

			errmsgno(EX_BAD,
			_("If you have unsolvable problems, please try Linux-2.4 or Solaris.\n"));
		}
	}
#endif
#endif
}

LOCAL void
priv_warn(what, msg)
	const char	*what;
	const char	*msg;
{
	errmsgno(EX_BAD, "Insufficient '%s' privileges. %s\n", what, msg);
}

#ifdef	TR_DEBUG
EXPORT void
prtrack(trackp)
	track_t	*trackp;
{
	error("Track:		%d\n", (int)(trackp - track_base(trackp)));
	error("xfp:		%p\n", trackp->xfp);
	error("filename:	%s\n", trackp->filename);
	error("itracksize:	%lld\n", (Llong)trackp->itracksize);
	error("tracksize:	%lld\n", (Llong)trackp->tracksize);
	error("trackstart:	%ld\n", trackp->trackstart);
	error("tracksecs:	%ld\n", trackp->tracksecs);
	error("padsecs:	%ld\n", trackp->padsecs);
	error("pregapsize:	%ld\n", trackp->pregapsize);
	error("index0start:	%ld\n", trackp->index0start);
	error("isecsize:	%d\n", trackp->isecsize);
	error("secsize:	%d\n", trackp->secsize);
	error("secspt:		%d\n", trackp->secspt);
	error("pktsize:	%d\n", trackp->pktsize);
	error("dataoff:	%d\n", trackp->dataoff);
	error("tracks:		%d\n", trackp->tracks);
	error("track:		%d\n", trackp->track);
	error("trackno:	%d\n", trackp->trackno);
	error("tracktype:	%d\n", trackp->tracktype);
	error("dbtype:		%d\n", trackp->dbtype);
	error("sectype:	%2.2X\n", trackp->sectype);
	error("flags:		%X\n", trackp->flags);
	error("nindex:		%d\n", trackp->nindex);

}
#endif
