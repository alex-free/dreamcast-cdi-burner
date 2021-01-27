/* @(#)scgcheck.c	1.22 16/01/24 Copyright 1998-2016 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)scgcheck.c	1.22 16/01/24 Copyright 1998-2016 J. Schilling";
#endif
/*
 *	Copyright (c) 1998-2016 J. Schilling
 *
 * Warning: This program has been written to verify the correctness
 * of the upper layer interface from the library "libscg". If you
 * modify code from the program "scgcheck", you must change the
 * name of the program.
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

#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>
#include <schily/standard.h>

#include <schily/utypes.h>
#include <schily/btorder.h>
#include <scg/scgcmd.h>
#include <scg/scsidefs.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>
#include "scsi_scan.h"

#include "cdrecord.h"
#include "scgcheck.h"
#include "version.h"

LOCAL	void	usage		__PR((int ret));
EXPORT	int	main		__PR((int ac, char *av[]));
LOCAL	SCSI	*doopen		__PR((char *dev));
LOCAL	void	checkversion	__PR((SCSI *scgp));
LOCAL	void	getbuf		__PR((SCSI *scgp));
LOCAL	void	scg_openerr	__PR((char *errstr));
LOCAL	int	find_drive	__PR((SCSI *scgp, char *sdevname, int flags));
EXPORT	void	flushit		__PR((void));
EXPORT	int	countopen	__PR((void));
EXPORT	int	chkprint	__PR((const char *, ...)) __printflike__(1, 2);
EXPORT	int	chkgetline	__PR((char *, int));


	char	*dev;
	char	*scgopts;
int		debug;		/* print debug messages */
int		kdebug;		/* kernel debug messages */
int		scsi_verbose;	/* SCSI verbose flag */
int		lverbose;	/* local verbose flag */
int		silent;		/* SCSI silent flag */
int		deftimeout = 40; /* default SCSI timeout */
int		xdebug;		/* extended debug flag */
BOOL		autotest;

char	*buf;			/* The transfer buffer */
long	bufsize;		/* The size of the transfer buffer */

FILE	*logfile;
char	unavail[] = "<data unavaiable>";
char	scgc_version[] = VERSION;
int	basefds;

#define	BUF_SIZE	(126*1024)
#define	MAX_BUF_SIZE	(16*1024*1024)

LOCAL void
usage(ret)
	int	ret;
{
	error(_("Usage:\tscgcheck [options]\n"));
	error(_("Options:\n"));
	error(_("\t-version	print version information and exit\n"));
	error(_("\tdev=target	SCSI target to use\n"));
	error(_("\tscgopts=spec	SCSI options for libscg\n"));
	error(_("\ttimeout=#	set the default SCSI command timeout to #.\n"));
	error(_("\tdebug=#,-d	Set to # or increment misc debug level\n"));
	error(_("\tkdebug=#,kd=#	do Kernel debugging\n"));
	error(_("\t-verbose,-v	increment general verbose level by one\n"));
	error(_("\t-Verbose,-V	increment SCSI command transport verbose level by one\n"));
	error(_("\t-silent,-s	do not print status of failed SCSI commands\n"));
	error(_("\tf=filename	Name of file to write log data to.\n"));
	error(_("\t-auto		try to do a fully automated test\n"));
	error("\n");
	exit(ret);
}

char	opts[]   = "debug#,d+,kdebug#,kd#,timeout#,verbose+,v+,Verbose+,V+,silent,s,x+,xd#,help,h,version,dev*,scgopts*,f*,auto";

EXPORT int
main(ac, av)
	int	ac;
	char	*av[];
{
	int	cac;
	char	* const *cav;
	SCSI	*scgp = NULL;
	char	device[128];
	char	abuf[2];
	int	ret;
	int	fcount;
	BOOL	help = FALSE;
	BOOL	pversion = FALSE;
	char	*filename = "check.log";
#if	defined(USE_NLS)
	char	*dir;
#endif

	save_args(ac, av);

#if	defined(USE_NLS)
	(void) setlocale(LC_ALL, "");
#if !defined(TEXT_DOMAIN)	/* Should be defined by cc -D */
#define	TEXT_DOMAIN "scgcheck"	/* Use this only if it weren't */
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

	cac = --ac;
	cav = ++av;

	if (getallargs(&cac, &cav, opts,
			&debug, &debug,
			&kdebug, &kdebug,
			&deftimeout,
			&lverbose, &lverbose,
			&scsi_verbose, &scsi_verbose,
			&silent, &silent,
			&xdebug, &xdebug,
			&help, &help, &pversion,
			&dev, &scgopts,
			&filename, &autotest) < 0) {
		errmsgno(EX_BAD, _("Bad flag: %s.\n"), cav[0]);
		usage(EX_BAD);
	}
	if (help)
		usage(0);
	if (pversion) {
		printf(_("scgcheck %s (%s-%s-%s) Copyright (C) 1998-2016 %s\n"),
								scgc_version,
								HOST_CPU, HOST_VENDOR, HOST_OS,
								_("Joerg Schilling"));
		exit(0);
	}

	fcount = 0;
	cac = ac;
	cav = av;

	while (getfiles(&cac, &cav, opts) > 0) {
		fcount++;
		cac--;
		cav++;
	}
	if (fcount > 0)
		comerrno(EX_BAD, _("Bad argument(s).\n"));
/*error("dev: '%s'\n", dev);*/

	logfile = fileopen(filename, "wct");
	if (logfile == NULL)
		comerr(_("Cannot open logfile.\n"));

	chkprint(_("Scgcheck %s (%s-%s-%s) SCSI user level transport library ABI checker.\n\
Copyright (C) 1998-2016 %s\n"),
						scgc_version,
						HOST_CPU, HOST_VENDOR, HOST_OS,
						_("Joerg Schilling"));
	/*
	 * Call scg_remote() to force loading the remote SCSI transport library
	 * code that is located in librscg instead of the dummy remote routines
	 * that are located inside libscg.
	 */
	scg_remote();

	basefds = countopen();
	if (xdebug)
		error(_("nopen: %d\n"), basefds);

	chkprint(_("**********> Checking whether your implementation supports to scan the SCSI bus.\n"));
	chkprint(_("Trying to open device: '%s'.\n"), dev);
	scgp = doopen(dev);

	if (xdebug) {
		error(_("nopen: %d\n"), countopen());
		error(_("Scanopen opened %d new files.\n"), countopen() - basefds);
	}

	device[0] = '\0';
	if (scgp == NULL) do {
		error(_("SCSI open failed...\n"));
		if (!scg_yes(_("Retry with different device name? ")))
			break;
		error(_("Enter SCSI device name for bus scanning [%s]: "), device);
		flushit();
		(void) getline(device, sizeof (device));
		if (device[0] == '\0')
			strcpy(device, "0,6,0");

		chkprint(_("Trying to open device: '%s'.\n"), device);
		scgp = doopen(device);
	} while (scgp == NULL);
	if (scgp) {
		checkversion(scgp);
		getbuf(scgp);

		ret = select_target(scgp, stdout);
		select_target(scgp, logfile);
		if (ret < 1) {
			chkprint(_("----------> SCSI scan bus test: found NO TARGETS\n"));
		} else {
			chkprint(_("----------> SCSI scan bus test PASSED\n"));
		}
	} else {
		chkprint(_("----------> SCSI scan bus test FAILED\n"));
	}

	if (xdebug)
		error(_("nopen: %d\n"), countopen());
	chkprint(_("For the next test we need to open a single SCSI device.\n"));
	chkprint(_("Best results will be obtained if you specify a modern CD-ROM drive.\n"));

	if (scgp) {		/* Scanbus works / may work */
		int	i;

		i = find_drive(scgp, dev, 0);
		if (i < 0) {
			scg_openerr("");
			/* NOTREACHED */
		}
		snprintf(device, sizeof (device),
			"%s%s%d,%d,%d",
			dev?dev:"", dev?(dev[strlen(dev)-1] == ':'?"":":"):"",
			scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp));
		scg_close(scgp);
		scgp = NULL;
	}

	if (device[0] == '\0')
		strcpy(device, "0,6,0");
	do {
		char	Device[128];

		error(_("Enter SCSI device name [%s]: "), device);
		(void) chkgetline(Device, sizeof (Device));
		if (Device[0] != '\0')
			strcpy(device, Device);

		chkprint(_("Trying to open device: '%s'.\n"), device);
		scgp = doopen(device);
		if (scgp) {
			checkversion(scgp);
			getbuf(scgp);
		}
		/*
		 * XXX hier muß getestet werden ob das Gerät brauchbar für die folgenden Tests ist.
		 */
	} while (scgp == NULL);
	if (xdebug)
		error(_("nopen: %d\n"), countopen());
	/*
	 * First try to check which type of SCSI device we
	 * have.
	 */
	scgp->silent++;
	(void) unit_ready(scgp);	/* eat up unit attention */
	scgp->silent--;
	getdev(scgp, TRUE);
	printinq(scgp, logfile);

	printf(_("Ready to start test for second SCSI open? Enter <CR> to continue: "));
	(void) chkgetline(abuf, sizeof (abuf));
#define	CHECK_SECOND_OPEN
#ifdef	CHECK_SECOND_OPEN
	if (!streql(abuf, "n")) {
		SCSI	*scgp2 = NULL;
		int	oldopen = countopen();
		BOOL	second_ok = TRUE;

		scgp->silent++;
		ret = inquiry(scgp, buf, sizeof (struct scsi_inquiry));
		scgp->silent--;
		if (xdebug)
			error(_("ret: %d key: %d\n"), ret, scg_sense_key(scgp));
		if (ret >= 0 || scgp->scmd->error == SCG_RETRYABLE) {
			chkprint(_("First SCSI open OK - device usable\n"));
			chkprint(_("**********> Checking for second SCSI open.\n"));
			if ((scgp2 = doopen(device)) != NULL) {
				/*
				 * XXX Separates getbuf() fuer scgp2?
				 */
				chkprint(_("Second SCSI open for same device succeeded, %d additional file descriptor(s) used.\n"),
					countopen() - oldopen);
				scgp->silent++;
				ret = inquiry(scgp, buf, sizeof (struct scsi_inquiry));
				scgp->silent--;
				if (ret >= 0 || scgp->scmd->error == SCG_RETRYABLE) {
					chkprint(_("Second SCSI open is usable\n"));
				}
				chkprint(_("Closing second SCSI.\n"));
				scg_close(scgp2);
				scgp2 = NULL;
				chkprint(_("Checking first SCSI.\n"));
				scgp->silent++;
				ret = inquiry(scgp, buf, sizeof (struct scsi_inquiry));
				scgp->silent--;
				if (ret >= 0 || scgp->scmd->error == SCG_RETRYABLE) {
					second_ok = TRUE;
					chkprint(_("First SCSI open is still usable\n"));
					chkprint(_("----------> Second SCSI open test PASSED.\n"));
				} else if (ret < 0 && scgp->scmd->error == SCG_FATAL) {
					second_ok = FALSE;
					chkprint(_("First SCSI open does not work anymore.\n"));
					chkprint(_("----------> Second SCSI open test FAILED.\n"));
				} else {
					second_ok = FALSE;
					chkprint(_("First SCSI open has strange problems.\n"));
					chkprint(_("----------> Second SCSI open test FAILED.\n"));
				}
			} else {
				second_ok = FALSE;
				chkprint(_("Cannot open same SCSI device a second time.\n"));
				chkprint(_("----------> Second SCSI open test FAILED.\n"));
			}
		} else {
			second_ok = FALSE;
			chkprint(_("First SCSI open is not usable\n"));
			chkprint(_("----------> Second SCSI open test FAILED.\n"));
		}
		if (xdebug > 1)
			error("scgp %p scgp2 %p\n", scgp, scgp2);
		if (scgp2)
			scg_close(scgp2);
		if (scgp) {
			scgp->silent++;
			ret = inquiry(scgp, buf, sizeof (struct scsi_inquiry));
			scgp->silent--;
			if (ret >= 0 || scgp->scmd->error == SCG_RETRYABLE) {
				chkprint(_("First SCSI open is still usable\n"));
			} else {
				scg_freebuf(scgp);
				scg_close(scgp);
				scgp = doopen(device);
				getbuf(scgp);
				if (xdebug > 1)
					error("scgp %p\n", scgp);
			}
		}
	}
#endif	/* CHECK_SECOND_OPEN */

	printf(_("Ready to start test for succeeded command? Enter <CR> to continue: "));
	(void) chkgetline(abuf, sizeof (abuf));
	chkprint(_("**********> Checking for succeeded SCSI command.\n"));
	scgp->verbose++;
	ret = inquiry(scgp, buf, sizeof (struct scsi_inquiry));
	scg_vsetup(scgp);
	scg_errfflush(scgp, logfile);
	scgp->verbose--;
	scg_fprbytes(logfile, _("Inquiry Data   :"), (Uchar *)buf,
			sizeof (struct scsi_inquiry) - scg_getresid(scgp));

	if (ret >= 0 && !scg_cmd_err(scgp)) {
		chkprint(_("----------> SCSI succeeded command test PASSED\n"));
	} else {
		chkprint(_("----------> SCSI succeeded command test FAILED\n"));
	}

	sensetest(scgp);
	if (!autotest)
		chkprint(_("----------> SCSI status byte test NOT YET READY\n"));
/*
 * scan OK
 * work OK
 * fail OK
 * sense data/count OK
 * SCSI status
 * dma resid
 * ->error GOOD/FAIL/timeout/noselect
 *    ??
 *
 * reset
 */

	dmaresid(scgp);
	chkprint(_("----------> SCSI transport code test NOT YET READY\n"));
	return (0);
}

LOCAL SCSI *
doopen(sdevname)
	char	*sdevname;
{
	SCSI	*scgp;
	char	errstr[128];

	if ((scgp = scg_open(sdevname, errstr, sizeof (errstr), debug, lverbose)) == (SCSI *)0) {
		errmsg(_("%s%sCannot open SCSI driver.\n"), errstr, errstr[0]?". ":"");
		fprintf(logfile, _("%s. %s%sCannot open SCSI driver.\n"),
			errmsgstr(geterrno()), errstr, errstr[0]?". ":"");
		errmsgno(EX_BAD, _("For possible targets try 'cdrecord -scanbus'. Make sure you are root.\n"));
		return (scgp);
	}
	scg_settimeout(scgp, deftimeout);
	if (scgopts) {
		int	i = scg_opts(scgp, scgopts);
		if (i <= 0)
			exit(i < 0 ? EX_BAD : 0);
	}
	scgp->verbose = scsi_verbose;
	scgp->silent = silent;
	scgp->debug = debug;
	scgp->kdebug = kdebug;
	scgp->cap->c_bsize = 2048;

	return (scgp);
}

LOCAL void
checkversion(scgp)
	SCSI	*scgp;
{
	char	*vers;
	char	*auth;

	/*
	 * Warning: If you modify this section of code, you must
	 * change the name of the program.
	 */
	vers = scg_version(0, SCG_VERSION);
	auth = scg_version(0, SCG_AUTHOR);
	chkprint(_("Using libscg version '%s-%s'\n"), auth, vers);
	if (auth == 0 || strcmp("schily", auth) != 0) {
		errmsgno(EX_BAD,
		_("Warning: using inofficial version of libscg (%s-%s '%s').\n"),
			auth, vers, scg_version(0, SCG_SCCS_ID));
	}

	vers = scg_version(scgp, SCG_VERSION);
	auth = scg_version(scgp, SCG_AUTHOR);
	if (lverbose > 1)
		error(_("Using libscg transport code version '%s-%s'\n"), auth, vers);
	fprintf(logfile, _("Using libscg transport code version '%s-%s'\n"), auth, vers);
	if (auth == 0 || strcmp("schily", auth) != 0) {
		errmsgno(EX_BAD,
		_("Warning: using inofficial libscg transport code version (%s-%s '%s').\n"),
			auth, vers, scg_version(scgp, SCG_SCCS_ID));
	}
	vers = scg_version(scgp, SCG_KVERSION);
	if (vers == NULL)
		vers = unavail;
	fprintf(logfile, _("Using kernel transport code version '%s'\n"), vers);

	vers = scg_version(scgp, SCG_RVERSION);
	auth = scg_version(scgp, SCG_RAUTHOR);
	if (lverbose > 1 && vers && auth)
		error(_("Using remote transport code version '%s-%s'\n"), auth, vers);

	if (auth != 0 && strcmp("schily", auth) != 0) {
		errmsgno(EX_BAD,
		_("Warning: using inofficial remote transport code version (%s-%s '%s').\n"),
			auth, vers, scg_version(scgp, SCG_RSCCS_ID));
	}
	if (auth == NULL)
		auth = unavail;
	if (vers == NULL)
		vers = unavail;
	fprintf(logfile, _("Using remote transport code version '%s-%s'\n"), auth, vers);
}

LOCAL void
getbuf(scgp)
	SCSI	*scgp;
{
	bufsize = scg_bufsize(scgp, MAX_BUF_SIZE);
	chkprint(_("Max DMA buffer size: %ld\n"), bufsize);
	seterrno(0);
	if ((buf = scg_getbuf(scgp, bufsize)) == NULL) {
		errmsg(_("Cannot get SCSI buffer (%ld bytes).\n"), bufsize);
		fprintf(logfile, _("%s. Cannot get SCSI buffer (%ld bytes).\n"),
			errmsgstr(geterrno()), bufsize);
	} else {
		scg_freebuf(scgp);
	}

	bufsize = scg_bufsize(scgp, BUF_SIZE);
	if (debug)
		error(_("SCSI buffer size: %ld\n"), bufsize);
	if ((buf = scg_getbuf(scgp, bufsize)) == NULL)
		comerr(_("Cannot get SCSI I/O buffer.\n"));
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
find_drive(scgp, sdevname, flags)
	SCSI	*scgp;
	char	*sdevname;
	int	flags;
{
	int	ntarget;
	int	type = INQ_ROMD;

	if ((flags & F_MSINFO) == 0)
		error(_("No target specified, trying to find one...\n"));
	ntarget = find_target(scgp, type, -1);
	if (ntarget < 0)
		return (ntarget);
	if (ntarget == 1) {
		/*
		 * Simple case, exactly one CD-ROM found.
		 */
		find_target(scgp, type, 1);
	} else if (ntarget <= 0 && (ntarget = find_target(scgp, type = INQ_WORM, -1)) == 1) {
		/*
		 * Exactly one CD-ROM acting as WORM found.
		 */
		find_target(scgp, type, 1);
	} else if (ntarget <= 0) {
		/*
		 * No single CD-ROM or WORM found.
		 */
		errmsgno(EX_BAD, _("No CD/DVD/BD-Recorder target found.\n"));
		errmsgno(EX_BAD, _("Your platform may not allow to scan for SCSI devices.\n"));
		comerrno(EX_BAD, _("Call 'cdrecord dev=help' or ask your sysadmin for possible targets.\n"));
	} else {
		errmsgno(EX_BAD, _("Too many CD/DVD/BD-Recorder targets found.\n"));
#ifdef	nonono
		select_target(scgp, stdout);
		comerrno(EX_BAD, _("Select a target from the list above and use 'cdrecord dev=%s%sb,t,l'.\n"),
			sdevname?sdevname:"",
			sdevname?(sdevname[strlen(sdevname)-1] == ':'?"":":"):"");
#endif	/* nonono */
		find_target(scgp, type, 1);
	}
	if ((flags & F_MSINFO) == 0)
		error(_("Using dev=%s%s%d,%d,%d.\n"),
			sdevname?sdevname:"",
			sdevname?(sdevname[strlen(sdevname)-1] == ':'?"":":"):"",
			scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp));

	return (ntarget);
}


EXPORT void
flushit()
{
	flush();
	fflush(logfile);
}

/*--------------------------------------------------------------------------*/
#include <schily/fcntl.h>

int
countopen()
{
	int	nopen = 0;
	int	i;

#ifdef	F_GETFD				/* XXX how to count open fd on MSC */
	for (i = 0; i < 1000; i++) {
		if (fcntl(i, F_GETFD, 0) >= 0)
			nopen++;
	}
#endif
	return (nopen);
}
/*--------------------------------------------------------------------------*/
#include <schily/varargs.h>

/* VARARGS1 */
#ifdef	PROTOTYPES
EXPORT int
chkprint(const char *fmt, ...)
#else
EXPORT int
chkprint(fmt, va_alist)
	char	*fmt;
	va_dcl
#endif
{
	va_list	args;
	int	ret;

#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	ret = js_fprintf(stdout, "%r", fmt, args);
	va_end(args);
	if (ret < 0)
		return (ret);
#ifdef	PROTOTYPES
	va_start(args, fmt);
#else
	va_start(args);
#endif
	ret = js_fprintf(logfile, "%r", fmt, args);
	va_end(args);
	return (ret);
}

EXPORT int
chkgetline(lbuf, len)
	char	*lbuf;
	int	len;
{
	flushit();
	if (autotest) {
		printf("\n");
		flush();
		if (len > 0)
			lbuf[0] = '\0';
		return (0);
	}
	return (getline(lbuf, len));
}
