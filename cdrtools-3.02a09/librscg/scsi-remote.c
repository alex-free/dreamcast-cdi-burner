#define	USE_REMOTE
/* @(#)scsi-remote.c	1.34 13/04/21 Copyright 1990,2000-2013 J. Schilling */
#ifndef lint
static	char __sccsid[] =
	"@(#)scsi-remote.c	1.34 13/04/21 Copyright 1990,2000-2013 J. Schilling";
#endif
/*
 *	Remote SCSI user level command transport routines
 *
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 *
 *	Copyright (c) 1990,2000-2013 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * The following exceptions apply:
 * CDDL §3.6 needs to be replaced by: "You may create a Larger Work by
 * combining Covered Software with other code if all other code is governed by
 * the terms of a license that is OSI approved (see www.opensource.org) and
 * you may distribute the Larger Work as a single product. In such a case,
 * You must make sure the requirements of this License are fulfilled for
 * the Covered Software."
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#include <schily/mconfig.h>

#if !defined(HAVE_FORK) || !defined(HAVE_SOCKETPAIR) || !defined(HAVE_DUP2)
#undef	USE_RCMD_RSH
#endif
/*
 * We may work without getservbyname() if we restructure the code not to
 * use the port number if we only use _rcmdrsh().
 */
#if !defined(HAVE_GETSERVBYNAME)
#undef	USE_REMOTE				/* Cannot get rcmd() port # */
#endif
#if (!defined(HAVE_NETDB_H) || !defined(HAVE_RCMD)) && !defined(USE_RCMD_RSH)
#undef	USE_REMOTE				/* There is no rcmd() */
#endif

#ifdef	USE_REMOTE
#include <schily/stdio.h>
#include <schily/types.h>
#include <schily/fcntl.h>
#include <schily/socket.h>
#include <schily/errno.h>
#include <schily/signal.h>
#include <schily/netdb.h>
#include <schily/pwd.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/priv.h>

#include <scg/scgcmd.h>
#include <scg/scsitransp.h>

#if	defined(SIGDEFER) || defined(SVR4)
#define	signal	sigset
#endif

/*
 * On Cygwin, there are no privilleged ports.
 * On UNIX, rcmd() uses privilleged port that only work for root.
 */
#if	defined(IS_CYGWIN) || defined(__MINGW32__)
#define	privport_ok()	(1)
#else
#if	defined(HAVE_SOLARIS_PPRIV) || defined(HAVE_LINUX_CAPS)
#define	privport_ok()	ppriv_ok()
#else
#define	privport_ok()	(geteuid() == 0)
#endif
#endif

#define	CMD_SIZE	80

/*
 * On Linux we have a max bus number of 1000 + 13
 */
#define	MAX_SCG		1024	/* Max # of SCSI controllers */
#define	MAX_TGT		16
#define	MAX_LUN		8

/*extern	BOOL	debug;*/
LOCAL	BOOL	debug = 1;

LOCAL	char	_scg_trans_version[] = "remote-1.34";	/* The version for remote SCSI	*/
LOCAL	char	_scg_auth_schily[]	= "schily";	/* The author for this module	*/

LOCAL	int	scgo_rsend		__PR((SCSI *scgp));
LOCAL	char *	scgo_rversion		__PR((SCSI *scgp, int what));
LOCAL	int	scgo_rhelp		__PR((SCSI *scgp, FILE *f));
LOCAL	int	scgo_ropen		__PR((SCSI *scgp, char *device));
LOCAL	int	scgo_rclose		__PR((SCSI *scgp));
LOCAL	long	scgo_rmaxdma		__PR((SCSI *scgp, long amt));
LOCAL	void *	scgo_rgetbuf		__PR((SCSI *scgp, long amt));
LOCAL	void	scgo_rfreebuf		__PR((SCSI *scgp));
LOCAL	int	scgo_rnumbus		__PR((SCSI *scgp));
LOCAL	BOOL	scgo_rhavebus		__PR((SCSI *scgp, int busno));
LOCAL	int	scgo_rfileno		__PR((SCSI *scgp, int busno, int tgt, int tlun));
LOCAL	int	scgo_rinitiator_id	__PR((SCSI *scgp));
LOCAL	int	scgo_risatapi		__PR((SCSI *scgp));
LOCAL	int	scgo_rreset		__PR((SCSI *scgp, int what));

/*
 * XXX We should rethink the fd parameter now that we introduced
 * XXX the rscsirchar() function and most access of remfd is done
 * XXX via scglocal(scgp)->remfd.
 */
LOCAL	void	rscsiabrt		__PR((int sig));
LOCAL	int	rscsigetconn		__PR((SCSI *scgp, char *host));
LOCAL	char	*rscsiversion		__PR((SCSI *scgp, int fd, int what));
LOCAL	int	rscsiopen		__PR((SCSI *scgp, int fd, char *fname));
LOCAL	int	rscsiclose		__PR((SCSI *scgp, int fd));
LOCAL	int	rscsimaxdma		__PR((SCSI *scgp, int fd, long amt));
LOCAL	int	rscsigetbuf		__PR((SCSI *scgp, int fd, long amt));
LOCAL	int	rscsifreebuf		__PR((SCSI *scgp, int fd));
LOCAL	int	rscsinumbus		__PR((SCSI *scgp, int fd));
LOCAL	int	rscsihavebus		__PR((SCSI *scgp, int fd, int bus));
LOCAL	int	rscsifileno		__PR((SCSI *scgp, int fd, int busno, int tgt, int tlun));
LOCAL	int	rscsiinitiator_id	__PR((SCSI *scgp, int fd));
LOCAL	int	rscsiisatapi		__PR((SCSI *scgp, int fd));
LOCAL	int	rscsireset		__PR((SCSI *scgp, int fd, int what));
LOCAL	int	rscsiscmd		__PR((SCSI *scgp, int fd, struct scg_cmd *sp));
LOCAL	int	rscsifillrbuf		__PR((SCSI *scgp));
LOCAL	int	rscsirchar		__PR((SCSI *scgp, char *cp));
LOCAL	int	rscsireadbuf		__PR((SCSI *scgp, int fd, char *buf, int count));
LOCAL	void	rscsivoidarg		__PR((SCSI *scgp, int fd, int count));
LOCAL	int	rscsicmd		__PR((SCSI *scgp, int fd, char *name, char *cbuf));
LOCAL	void	rscsisendcmd		__PR((SCSI *scgp, int fd, char *name, char *cbuf));
LOCAL	int	rscsigetline		__PR((SCSI *scgp, int fd, char *line, int count));
LOCAL	int	rscsireadnum		__PR((SCSI *scgp, int fd));
LOCAL	int	rscsigetstatus		__PR((SCSI *scgp, int fd, char *name));
LOCAL	int	rscsiaborted		__PR((SCSI *scgp, int fd));
#ifdef	USE_RCMD_RSH
LOCAL	int	_rcmdrsh		__PR((char **ahost, int inport,
						const char *locuser,
						const char *remuser,
						const char *cmd,
						const char *rsh));
#if	defined(HAVE_SOLARIS_PPRIV) || defined(HAVE_LINUX_CAPS)
LOCAL	BOOL	ppriv_ok		__PR((void));
#endif
#endif

/*--------------------------------------------------------------------------*/

#define	READBUF_SIZE	128

struct scg_local {
	int	remfd;
	char	readbuf[READBUF_SIZE];
	char	*readbptr;
	int	readbcnt;
	BOOL	isopen;
	int	rsize;
	int	wsize;
	char	*v_version;
	char	*v_author;
	char	*v_sccs_id;
};


#define	scglocal(p)	((struct scg_local *)((p)->local))

scg_ops_t remote_ops = {
	scgo_rsend,		/* "S" end	*/
	scgo_rversion,		/* "V" ersion	*/
	scgo_rhelp,		/*     help	*/
	scgo_ropen,		/* "O" pen	*/
	scgo_rclose,		/* "C" lose	*/
	scgo_rmaxdma,		/* "D" MA	*/
	scgo_rgetbuf,		/* "M" alloc	*/
	scgo_rfreebuf,		/* "F" free	*/
	scgo_rnumbus,		/* "N" um Bus	*/
	scgo_rhavebus,		/* "B" us	*/
	scgo_rfileno,		/* "T" arget	*/
	scgo_rinitiator_id,	/* "I" nitiator	*/
	scgo_risatapi,		/* "A" tapi	*/
	scgo_rreset,		/* "R" eset	*/
};

/*
 * Return our ops ptr.
 */
scg_ops_t *
scg_remote()
{
	return (&remote_ops);
}

/*
 * Return version information for the low level SCSI transport code.
 * This has been introduced to make it easier to trace down problems
 * in applications.
 */
LOCAL char *
scgo_rversion(scgp, what)
	SCSI	*scgp;
	int	what;
{
	int	f;

	if (scgp == (SCSI *)0 || scgp->local == NULL)
		return ((char *)0);

	f = scglocal(scgp)->remfd;

	switch (what) {

	case SCG_VERSION:
		return (_scg_trans_version);
	/*
	 * If you changed this source, you are not allowed to
	 * return "schily" for the SCG_AUTHOR request.
	 */
	case SCG_AUTHOR:
		return (_scg_auth_schily);
	case SCG_SCCS_ID:
		return (__sccsid);

	case SCG_RVERSION:
		if (scglocal(scgp)->v_version == NULL)
			scglocal(scgp)->v_version = rscsiversion(scgp, f, SCG_VERSION);
		return (scglocal(scgp)->v_version);
	/*
	 * If you changed this source, you are not allowed to
	 * return "schily" for the SCG_AUTHOR request.
	 */
	case SCG_RAUTHOR:
		if (scglocal(scgp)->v_author == NULL)
			scglocal(scgp)->v_author = rscsiversion(scgp, f, SCG_AUTHOR);
		return (scglocal(scgp)->v_author);
	case SCG_RSCCS_ID:
		if (scglocal(scgp)->v_sccs_id == NULL)
		scglocal(scgp)->v_sccs_id = rscsiversion(scgp, f, SCG_SCCS_ID);
		return (scglocal(scgp)->v_sccs_id);
	}
	return ((char *)0);
}

LOCAL int
scgo_rhelp(scgp, f)
	SCSI	*scgp;
	FILE	*f;
{
	__scg_help(f, "RSCSI", "Remote SCSI",
		"REMOTE:", "rscsi@host:bus,target,lun", "REMOTE:rscsi@host:1,2,0", TRUE, FALSE);
	return (0);
}

LOCAL int
scgo_ropen(scgp, device)
	SCSI	*scgp;
	char	*device;
{
		int	busno	= scg_scsibus(scgp);
		int	tgt	= scg_target(scgp);
		int	tlun	= scg_lun(scgp);
	register int	f;
	register int	nopen = 0;
	char		sdevname[128];
	char		*p;

	if (scgp->overbose)
		error("Warning: Using remote SCSI interface.\n");

	if (busno >= MAX_SCG || tgt >= MAX_TGT || tlun >= MAX_LUN) {
		errno = EINVAL;
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
			"Illegal value for busno, target or lun '%d,%d,%d'",
			busno, tgt, tlun);

		return (-1);
	}
	if (scgp->local == NULL) {
		scgp->local = malloc(sizeof (struct scg_local));
		if (scgp->local == NULL)
			return (0);
		scglocal(scgp)->remfd = -1;
		scglocal(scgp)->readbptr = scglocal(scgp)->readbuf;
		scglocal(scgp)->readbcnt = 0;
		scglocal(scgp)->isopen = FALSE;
		scglocal(scgp)->rsize = 0;
		scglocal(scgp)->wsize = 0;
		scglocal(scgp)->v_version = NULL;
		scglocal(scgp)->v_author  = NULL;
		scglocal(scgp)->v_sccs_id = NULL;
	}

	if (device == NULL || (strncmp(device, "REMOTE", 6) != 0) ||
				(device = strchr(device, ':')) == NULL) {
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Illegal remote device syntax");
		return (-1);
	}
	device++;
	/*
	 * Save non user@host:device
	 */
	js_snprintf(sdevname, sizeof (sdevname), "%s", device);

	if ((p = strchr(sdevname, ':')) != NULL)
		*p++ = '\0';

	f = rscsigetconn(scgp, sdevname);
	if (f < 0) {
		if (scgp->errstr)
			js_snprintf(scgp->errstr, SCSI_ERRSTR_SIZE,
				"Cannot get connection to remote host");
		return (-1);
	}
	scglocal(scgp)->remfd = f;
	debug = scgp->debug;
	if (rscsiopen(scgp, f, p) >= 0) {
		nopen++;
		scglocal(scgp)->isopen = TRUE;
	}
	return (nopen);
}

LOCAL int
scgo_rclose(scgp)
	SCSI	*scgp;
{
	register int	f;
		int	ret;

	if (scgp->local == NULL)
		return (-1);

	if (scglocal(scgp)->v_version != NULL) {
		free(scglocal(scgp)->v_version);
		scglocal(scgp)->v_version = NULL;
	}
	if (scglocal(scgp)->v_author != NULL) {
		free(scglocal(scgp)->v_author);
		scglocal(scgp)->v_author  = NULL;
	}
	if (scglocal(scgp)->v_sccs_id != NULL) {
		free(scglocal(scgp)->v_sccs_id);
		scglocal(scgp)->v_sccs_id = NULL;
	}

	f = scglocal(scgp)->remfd;
	if (f < 0 || !scglocal(scgp)->isopen)
		return (0);
	ret = rscsiclose(scgp, f);
	scglocal(scgp)->isopen = FALSE;
	close(f);
	scglocal(scgp)->remfd = -1;
	return (ret);
}

LOCAL long
scgo_rmaxdma(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	if (scgp->local == NULL)
		return (-1L);

	return (rscsimaxdma(scgp, scglocal(scgp)->remfd, amt));
}

LOCAL void *
scgo_rgetbuf(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	int	ret;

	if (scgp->local == NULL)
		return ((void *)0);

	ret = rscsigetbuf(scgp, scglocal(scgp)->remfd, amt);
	if (ret < 0)
		return ((void *)0);

#ifdef	HAVE_VALLOC
	scgp->bufbase = (void *)valloc((size_t)amt);
#else
	scgp->bufbase = (void *)malloc((size_t)amt);
#endif
	if (scgp->bufbase == NULL) {
		scgo_rfreebuf(scgp);
		return ((void *)0);
	}
	return (scgp->bufbase);
}

LOCAL void
scgo_rfreebuf(scgp)
	SCSI	*scgp;
{
	int	f;

	if (scgp->bufbase)
		free(scgp->bufbase);
	scgp->bufbase = NULL;

	if (scgp->local == NULL)
		return;

	f = scglocal(scgp)->remfd;
	if (f < 0 || !scglocal(scgp)->isopen)
		return;
	rscsifreebuf(scgp, f);
}

LOCAL int
scgo_rnumbus(scgp)
	SCSI	*scgp;
{
	return (rscsinumbus(scgp, scglocal(scgp)->remfd));
}

LOCAL BOOL
scgo_rhavebus(scgp, busno)
	SCSI	*scgp;
	int	busno;
{
	if (scgp->local == NULL || busno < 0 || busno >= MAX_SCG)
		return (FALSE);

	return (rscsihavebus(scgp, scglocal(scgp)->remfd, busno));
}

LOCAL int
scgo_rfileno(scgp, busno, tgt, tlun)
	SCSI	*scgp;
	int	busno;
	int	tgt;
	int	tlun;
{
	int	f;

	if (scgp->local == NULL ||
	    busno < 0 || busno >= MAX_SCG ||
	    tgt < 0 || tgt >= MAX_TGT ||
	    tlun < 0 || tlun >= MAX_LUN)
		return (-1);

	f = scglocal(scgp)->remfd;
	if (f < 0 || !scglocal(scgp)->isopen)
		return (-1);
	return (rscsifileno(scgp, f, busno, tgt, tlun));
}

LOCAL int
scgo_rinitiator_id(scgp)
	SCSI	*scgp;
{
	if (scgp->local == NULL)
		return (-1);

	return (rscsiinitiator_id(scgp, scglocal(scgp)->remfd));
}

LOCAL int
scgo_risatapi(scgp)
	SCSI	*scgp;
{
	if (scgp->local == NULL)
		return (-1);

	return (rscsiisatapi(scgp, scglocal(scgp)->remfd));
}

LOCAL int
scgo_rreset(scgp, what)
	SCSI	*scgp;
	int	what;
{
	if (scgp->local == NULL)
		return (-1);

	return (rscsireset(scgp, scglocal(scgp)->remfd, what));
}

LOCAL int
scgo_rsend(scgp)
	SCSI		*scgp;
{
	struct scg_cmd	*sp = scgp->scmd;
	int		ret;

	if (scgp->local == NULL)
		return (-1);

	if (scgp->fd < 0) {
		sp->error = SCG_FATAL;
		return (0);
	}
	ret = rscsiscmd(scgp, scglocal(scgp)->remfd, scgp->scmd);

	return (ret);
}

/*--------------------------------------------------------------------------*/
LOCAL void
rscsiabrt(sig)
	int	sig;
{
	rscsiaborted((SCSI *)0, -1);
}

LOCAL int
rscsigetconn(scgp, host)
	SCSI	*scgp;
	char	*host;
{
	static	struct servent	*sp = 0;
	static	struct passwd	*pw = 0;
		char		*name = "root";
		char		*p;
		char		*rscsi;
		char		*rsh;
		int		rscsisock;
		char		*rscsipeer;
		char		rscsiuser[128];


	signal(SIGPIPE, rscsiabrt);
	if (sp == 0) {
		sp = getservbyname("shell", "tcp");
		if (sp == 0) {
			comerrno(EX_BAD, "shell/tcp: unknown service\n");
			/* NOTREACHED */
		}
		pw = getpwuid(getuid());
		if (pw == 0) {
			comerrno(EX_BAD, "who are you? No passwd entry found.\n");
			/* NOTREACHED */
		}
	}
	if ((p = strchr(host, '@')) != NULL) {
		size_t d = p - host;

		if (d > sizeof (rscsiuser))
			d = sizeof (rscsiuser);
		js_snprintf(rscsiuser, sizeof (rscsiuser), "%.*s", (int)d, host);
		name = rscsiuser;
		host = &p[1];
	} else {
		name = pw->pw_name;
	}
	if (scgp->debug > 0)
		errmsgno(EX_BAD, "locuser: '%s' rscsiuser: '%s' host: '%s'\n",
						pw->pw_name, name, host);
	rscsipeer = host;

	if ((rscsi = getenv("RSCSI")) == NULL)
		rscsi = "/opt/schily/sbin/rscsi";
	rsh = getenv("RSH");

#ifdef	USE_RCMD_RSH
	if (!privport_ok() || rsh != NULL)
		rscsisock = _rcmdrsh(&rscsipeer, (unsigned short)sp->s_port,
					pw->pw_name, name, rscsi, rsh);
	else
#endif
#ifdef	HAVE_RCMD
		rscsisock = rcmd(&rscsipeer, (unsigned short)sp->s_port,
					pw->pw_name, name, rscsi, 0);
#else
		rscsisock = _rcmdrsh(&rscsipeer, (unsigned short)sp->s_port,
					pw->pw_name, name, rscsi, rsh);
#endif

	return (rscsisock);
}

LOCAL char *
rscsiversion(scgp, fd, what)
	SCSI	*scgp;
	int	fd;
	int	what;
{
	char	cbuf[CMD_SIZE];
	char	*p;
	int	ret;

	js_snprintf(cbuf, sizeof (cbuf), "V%d\n", what);
	ret = rscsicmd(scgp, fd, "version", cbuf);
	if (ret <= 0)
		return (NULL);
	p = malloc(ret);
	if (p == NULL)
		return (p);
	rscsireadbuf(scgp, fd, p, ret);
	return (p);
}

LOCAL int
rscsiopen(scgp, fd, fname)
	SCSI	*scgp;
	int	fd;
	char	*fname;
{
	char	cbuf[CMD_SIZE];
	int	ret;
	int	bus;
	int	chan;
	int	tgt;
	int	lun;

	js_snprintf(cbuf, sizeof (cbuf), "O%s\n", fname?fname:"");
	ret = rscsicmd(scgp, fd, "open", cbuf);
	if (ret < 0)
		return (ret);

	bus = rscsireadnum(scgp, fd);
	chan = rscsireadnum(scgp, fd);
	tgt = rscsireadnum(scgp, fd);
	lun = rscsireadnum(scgp, fd);

	scg_settarget(scgp, bus, tgt, lun);
	return (ret);
}

LOCAL int
rscsiclose(scgp, fd)
	SCSI	*scgp;
	int	fd;
{
	return (rscsicmd(scgp, fd, "close", "C\n"));
}

LOCAL int
rscsimaxdma(scgp, fd, amt)
	SCSI	*scgp;
	int	fd;
	long	amt;
{
	char	cbuf[CMD_SIZE];

	js_snprintf(cbuf, sizeof (cbuf), "D%ld\n", amt);
	return (rscsicmd(scgp, fd, "maxdma", cbuf));
}

LOCAL int
rscsigetbuf(scgp, fd, amt)
	SCSI	*scgp;
	int	fd;
	long	amt;
{
	char	cbuf[CMD_SIZE];
	int	size;
	int	ret;

	js_snprintf(cbuf, sizeof (cbuf), "M%ld\n", amt);
	ret = rscsicmd(scgp, fd, "getbuf", cbuf);
	if (ret < 0)
		return (ret);

	size = ret + 1024;	/* Add protocol overhead */

#ifdef	SO_SNDBUF
	if (size > scglocal(scgp)->wsize) while (size > 512 &&
		setsockopt(fd, SOL_SOCKET, SO_SNDBUF,
					(char *)&size, sizeof (size)) < 0) {
		size -= 512;
	}
	if (size > scglocal(scgp)->wsize) {
		scglocal(scgp)->wsize = size;
		if (scgp->debug > 0)
			errmsgno(EX_BAD, "sndsize: %d\n", size);
	}
#endif
#ifdef	SO_RCVBUF
	if (size > scglocal(scgp)->rsize) while (size > 512 &&
		setsockopt(fd, SOL_SOCKET, SO_RCVBUF,
					(char *)&size, sizeof (size)) < 0) {
		size -= 512;
	}
	if (size > scglocal(scgp)->rsize) {
		scglocal(scgp)->rsize = size;
		if (scgp->debug > 0)
			errmsgno(EX_BAD, "rcvsize: %d\n", size);
	}
#endif
	return (ret);
}

LOCAL int
rscsifreebuf(scgp, fd)
	SCSI	*scgp;
	int	fd;
{
	return (rscsicmd(scgp, fd, "freebuf", "F\n"));
}

LOCAL int
rscsinumbus(scgp, fd)
	SCSI	*scgp;
	int	fd;
{
	return (rscsicmd(scgp, fd, "numbus", "N\n"));
}

LOCAL int
rscsihavebus(scgp, fd, busno)
	SCSI	*scgp;
	int	fd;
	int	busno;
{
	char	cbuf[2*CMD_SIZE];

	js_snprintf(cbuf, sizeof (cbuf), "B%d\n%d\n",
		busno,
		0);
	return (rscsicmd(scgp, fd, "havebus", cbuf));
}

LOCAL int
rscsifileno(scgp, fd, busno, tgt, tlun)
	SCSI	*scgp;
	int	fd;
	int	busno;
	int	tgt;
	int	tlun;
{
	char	cbuf[3*CMD_SIZE];

	js_snprintf(cbuf, sizeof (cbuf), "T%d\n%d\n%d\n%d\n",
		busno,
		0,
		tgt,
		tlun);
	return (rscsicmd(scgp, fd, "fileno", cbuf));
}

LOCAL int
rscsiinitiator_id(scgp, fd)
	SCSI	*scgp;
	int	fd;
{
	return (rscsicmd(scgp, fd, "initiator id", "I\n"));
}

LOCAL int
rscsiisatapi(scgp, fd)
	SCSI	*scgp;
	int	fd;
{
	return (rscsicmd(scgp, fd, "isatapi", "A\n"));
}

LOCAL int
rscsireset(scgp, fd, what)
	SCSI	*scgp;
	int	fd;
	int	what;
{
	char	cbuf[CMD_SIZE];

	js_snprintf(cbuf, sizeof (cbuf), "R%d\n", what);
	return (rscsicmd(scgp, fd, "reset", cbuf));
}

LOCAL int
rscsiscmd(scgp, fd, sp)
	SCSI	*scgp;
	int	fd;
	struct scg_cmd  *sp;
{
	char	cbuf[1600];
	int	ret;
	int	amt = 0;
	int	voidsize = 0;

	ret = js_snprintf(cbuf, sizeof (cbuf), "S%d\n%d\n%d\n%d\n%d\n",
		sp->size, sp->flags,
		sp->cdb_len, sp->sense_len,
		sp->timeout);
	movebytes(sp->cdb.cmd_cdb, &cbuf[ret], sp->cdb_len);
	ret += sp->cdb_len;

	if ((sp->flags & SCG_RECV_DATA) == 0 && sp->size > 0) {
		amt = sp->size;
		if ((ret + amt) <= sizeof (cbuf)) {
			movebytes(sp->addr, &cbuf[ret], amt);
			ret += amt;
			amt = 0;
		}
	}
	errno = 0;
	if (_nixwrite(fd, cbuf, ret) != ret)
		rscsiaborted(scgp, fd);

	if (amt > 0) {
		if (_nixwrite(fd, sp->addr, amt) != amt)
			rscsiaborted(scgp, fd);
	}

	ret = rscsigetstatus(scgp, fd, "sendcmd");
	if (ret < 0)
		return (ret);

	sp->resid = sp->size - ret;
	sp->error = rscsireadnum(scgp, fd);
	sp->ux_errno = rscsireadnum(scgp, fd);
	*(Uchar *)&sp->scb = rscsireadnum(scgp, fd);
	sp->sense_count = rscsireadnum(scgp, fd);

	if (sp->sense_count > SCG_MAX_SENSE) {
		voidsize = sp->sense_count - SCG_MAX_SENSE;
		sp->sense_count = SCG_MAX_SENSE;
	}
	if (sp->sense_count > 0) {
		rscsireadbuf(scgp, fd, (char *)sp->u_sense.cmd_sense, sp->sense_count);
		rscsivoidarg(scgp, fd, voidsize);
	}

	if ((sp->flags & SCG_RECV_DATA) != 0 && ret > 0)
		rscsireadbuf(scgp, fd, sp->addr, ret);

	return (0);
}

LOCAL int
rscsifillrbuf(scgp)
	SCSI	*scgp;
{
	scglocal(scgp)->readbptr = scglocal(scgp)->readbuf;

	return (scglocal(scgp)->readbcnt =
			_niread(scglocal(scgp)->remfd,
			    scglocal(scgp)->readbuf, READBUF_SIZE));
}

LOCAL int
rscsirchar(scgp, cp)
	SCSI	*scgp;
	char	*cp;
{
	if (--(scglocal(scgp)->readbcnt) < 0) {
		if (rscsifillrbuf(scgp) <= 0)
			return (scglocal(scgp)->readbcnt);
		--(scglocal(scgp)->readbcnt);
	}
	*cp = *(scglocal(scgp)->readbptr)++;
	return (1);
}

LOCAL int
rscsireadbuf(scgp, fd, buf, count)
	SCSI	*scgp;
	int	fd;
	char	*buf;
	int	count;
{
	register int	n = count;
	register int	amt = 0;
	register int	cnt;

	if (scglocal(scgp)->readbcnt > 0) {
		cnt = scglocal(scgp)->readbcnt;
		if (cnt > n)
			cnt = n;
		movebytes(scglocal(scgp)->readbptr, buf, cnt);
		scglocal(scgp)->readbptr += cnt;
		scglocal(scgp)->readbcnt -= cnt;
		amt += cnt;
	}
	while (amt < n) {
		if ((cnt = _niread(fd, &buf[amt], n - amt)) <= 0) {
			return (rscsiaborted(scgp, fd));
		}
		amt += cnt;
	}
	return (amt);
}

LOCAL void
rscsivoidarg(scgp, fd, n)
	SCSI	*scgp;
	int	fd;
	register int	n;
{
	register int	i;
	register int	amt;
		char	buf[512];

	for (i = 0; i < n; i += amt) {
		amt = sizeof (buf);
		if ((n - i) < amt)
			amt = n - i;
		rscsireadbuf(scgp, fd, buf, amt);
	}
}

LOCAL int
rscsicmd(scgp, fd, name, cbuf)
	SCSI	*scgp;
	int	fd;
	char	*name;
	char	*cbuf;
{
	rscsisendcmd(scgp, fd, name, cbuf);
	return (rscsigetstatus(scgp, fd, name));
}

LOCAL void
rscsisendcmd(scgp, fd, name, cbuf)
	SCSI	*scgp;
	int	fd;
	char	*name;
	char	*cbuf;
{
	int	buflen = strlen(cbuf);

	errno = 0;
	if (_nixwrite(fd, cbuf, buflen) != buflen)
		rscsiaborted(scgp, fd);
}

LOCAL int
rscsigetline(scgp, fd, line, count)
	SCSI	*scgp;
	int	fd;
	char	*line;
	int	count;
{
	register char	*cp;

	for (cp = line; cp < &line[count]; cp++) {
		if (rscsirchar(scgp, cp) != 1)
			return (rscsiaborted(scgp, fd));

		if (*cp == '\n') {
			*cp = '\0';
			return (cp - line);
		}
	}
	return (rscsiaborted(scgp, fd));
}

LOCAL int
rscsireadnum(scgp, fd)
	SCSI	*scgp;
	int	fd;
{
	char	cbuf[CMD_SIZE];

	rscsigetline(scgp, fd, cbuf, sizeof (cbuf));
	return (atoi(cbuf));
}

LOCAL int
rscsigetstatus(scgp, fd, name)
	SCSI	*scgp;
	int	fd;
	char	*name;
{
	char	cbuf[CMD_SIZE];
	char	code;
	int	number;
	int	count;
	int	voidsize = 0;

	rscsigetline(scgp, fd, cbuf, sizeof (cbuf));
	code = cbuf[0];
	number = atoi(&cbuf[1]);

	if (code == 'E' || code == 'F') {
		rscsigetline(scgp, fd, cbuf, sizeof (cbuf));
		if (code == 'F')	/* should close file ??? */
			rscsiaborted(scgp, fd);

		rscsigetline(scgp, fd, cbuf, sizeof (cbuf));
		count = atoi(cbuf);
		if (count > 0) {
			if (scgp->errstr == NULL) {
				voidsize = count;
				count = 0;
			} else if (count > SCSI_ERRSTR_SIZE) {
				voidsize = count - SCSI_ERRSTR_SIZE;
				count = SCSI_ERRSTR_SIZE;
			}
			if (count > 0)
				rscsireadbuf(scgp, fd, scgp->errstr, count);
			rscsivoidarg(scgp, fd, voidsize);
		}
		if (scgp->debug > 0)
			errmsgno(number, "Remote status(%s): %d '%s'.\n",
							name, number, cbuf);
		errno = number;
		return (-1);
	}
	if (code != 'A') {
		/* XXX Hier kommt evt Command not found ... */
		if (scgp->debug > 0)
			errmsgno(EX_BAD, "Protocol error (got %s).\n", cbuf);
		return (rscsiaborted(scgp, fd));
	}
	return (number);
}

LOCAL int
rscsiaborted(scgp, fd)
	SCSI	*scgp;
	int	fd;
{
	if ((scgp && scgp->debug > 0) || debug)
		errmsgno(EX_BAD, "Lost connection to remote host ??\n");
	/* if fd >= 0 */
	/* close file */
	if (errno == 0)
		errno = EIO;
	return (-1);
}

/*--------------------------------------------------------------------------*/
#ifdef	USE_RCMD_RSH
/*
 * If we make a separate file for libschily, we would need these include files:
 *
 * socketpair():	sys/types.h + sys/socket.h
 * dup2():		unixstd.h (hat auch sys/types.h)
 * strrchr():		strdefs.h
 *
 * and make sure that we use sigset() instead of signal() if possible.
 */
#include <schily/wait.h>
LOCAL int
_rcmdrsh(ahost, inport, locuser, remuser, cmd, rsh)
	char		**ahost;
	int		inport;		/* port is ignored */
	const char	*locuser;
	const char	*remuser;
	const char	*cmd;
	const char	*rsh;
{
	struct passwd	*pw;
	int	pp[2];
	int	pid;

	if (rsh == 0)
		rsh = "rsh";

	/*
	 * Verify that 'locuser' is present on local host.
	 */
	if ((pw = getpwnam(locuser)) == NULL) {
		errmsgno(EX_BAD, "Unknown user: %s\n", locuser);
		return (-1);
	}
	/* XXX Check the existence for 'ahost' here? */

	/*
	 * rcmd(3) creates a single socket to be used for communication.
	 * We need a bi-directional pipe to implement the same interface.
	 * On newer OS that implement bi-directional we could use pipe(2)
	 * but it makes no sense unless we find an OS that implements a
	 * bi-directional pipe(2) but no socketpair().
	 */
	if (socketpair(AF_UNIX, SOCK_STREAM, PF_UNSPEC, pp) == -1) {
		errmsg("Cannot create socketpair.\n");
		return (-1);
	}

	pid = fork();
	if (pid < 0) {
		return (-1);
	} else if (pid == 0) {
		const char	*p;
		const char	*av0;
		int		xpid;

		(void) close(pp[0]);
		if (dup2(pp[1], 0) == -1 ||	/* Pipe becomes 'stdin'  */
		    dup2(0, 1) == -1) {		/* Pipe becomes 'stdout' */

			errmsg("dup2 failed.\n");
			_exit(EX_BAD);
			/* NOTREACHED */
		}
		(void) close(pp[1]);		/* We don't need this anymore*/

		/*
		 * Become 'locuser' to tell the rsh program the local user id.
		 */
		if (getuid() != pw->pw_uid &&
		    setuid(pw->pw_uid) == -1) {
			errmsg("setuid(%lld) failed.\n",
							(Llong)pw->pw_uid);
			_exit(EX_BAD);
			/* NOTREACHED */
		}
		if (getuid() != geteuid() &&
#ifdef	HAVE_SETREUID
		    setreuid(-1, pw->pw_uid) == -1) {
#else
#ifdef	HAVE_SETEUID
		    seteuid(pw->pw_uid) == -1) {
#else
		    setuid(pw->pw_uid) == -1) {
#endif
#endif
			errmsg("seteuid(%lld) failed.\n",
							(Llong)pw->pw_uid);
			_exit(EX_BAD);
			/* NOTREACHED */
		}

		/*
		 * Fork again to completely detach from parent
		 * and avoid the need to wait(2).
		 */
		if ((xpid = fork()) == -1) {
			errmsg("rcmdsh: fork to lose parent failed.\n");
			_exit(EX_BAD);
			/* NOTREACHED */
		}
		if (xpid > 0) {
			_exit(0);
			/* NOTREACHED */
		}

		/*
		 * Always use remote shell programm (even for localhost).
		 * The client command may call getpeername() for security
		 * reasons and this would fail on a simple pipe.
		 */


		/*
		 * By default, 'rsh' handles terminal created signals
		 * but this is not what we like.
		 * For this reason, we tell 'rsh' to ignore these signals.
		 * Ignoring these signals is important to allow 'star' / 'sdd'
		 * to e.g. implement SIGQUIT as signal to trigger intermediate
		 * status printing.
		 *
		 * For now (late 2002), we know that the following programs
		 * are broken and do not implement signal handling correctly:
		 *
		 *	rsh	on SunOS-5.0...SunOS-5.9
		 *	ssh	from ssh.com
		 *	ssh	from openssh.org
		 *
		 * Sun already did accept a bug report for 'rsh'. For the ssh
		 * commands we need to send out bug reports. Meanwhile it could
		 * help to call setsid() if we are running under X so the ssh
		 * X pop up for passwd reading will work.
		 */
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
#ifdef	SIGTSTP
		signal(SIGTSTP, SIG_IGN); /* We would not be able to continue*/
#endif

		av0 = rsh;
		if ((p = strrchr(rsh, '/')) != NULL)
			av0 = ++p;
		execlp(rsh, av0, *ahost, "-l", remuser, cmd, (char *)NULL);

		errmsg("execlp '%s' failed.\n", rsh);
		_exit(EX_BAD);
		/* NOTREACHED */
	} else {
		(void) close(pp[1]);
		/*
		 * Wait for the intermediate child.
		 * The real 'rsh' program is completely detached from us.
		 */
		wait(0);
		return (pp[0]);
	}
	return (-1);	/* keep gcc happy */
}

#ifdef	HAVE_SOLARIS_PPRIV

LOCAL BOOL
ppriv_ok()
{
	priv_set_t	*privset;
	BOOL		net_privaddr = FALSE;


	if ((privset = priv_allocset()) == NULL) {
		return (FALSE);
	}
	if (getppriv(PRIV_EFFECTIVE, privset) == -1) {
		priv_freeset(privset);
		return (FALSE);
	}
	if (priv_ismember(privset, PRIV_NET_PRIVADDR)) {
		net_privaddr = TRUE;
	}
	priv_freeset(privset);

	return (net_privaddr);
}
#else	/* HAVE_SOLARIS_PPRIV */

#ifdef	HAVE_LINUX_CAPS
LOCAL BOOL
ppriv_ok()
{
	cap_t			cset;
	cap_flag_value_t	val = CAP_CLEAR;

	cset = cap_get_proc();

	cap_get_flag(cset, CAP_NET_BIND_SERVICE, CAP_EFFECTIVE, &val);
	return (val == CAP_SET);
}
#endif	/* HAVE_LINUX_CAPS */

#endif	/* HAVE_SOLARIS_PPRIV */

#endif	/* USE_RCMD_RSH */

#endif	/* USE_REMOTE */
