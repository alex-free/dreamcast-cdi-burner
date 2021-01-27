/* @(#)scsitransp.c	1.100 16/01/21 Copyright 1988,1995,2000-2016 J. Schilling */
/*#ifndef lint*/
static	char sccsid[] =
	"@(#)scsitransp.c	1.100 16/01/21 Copyright 1988,1995,2000-2016 J. Schilling";
/*#endif*/
/*
 *	SCSI user level command transport routines (generic part).
 *
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 *
 *	Copyright (c) 1988,1995,2000-2016 J. Schilling
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
#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/unistd.h>
#include <schily/errno.h>
#include <schily/time.h>
#include <schily/string.h>
#include <schily/schily.h>

#include <scg/scgcmd.h>
#include <scg/scsireg.h>
#include <scg/scsitransp.h>
#include "scgtimes.h"

/*
 *	Warning: you may change this source, but if you do that
 *	you need to change the _scg_version and _scg_auth* string below.
 *	You may not return "schily" for an SCG_AUTHOR request anymore.
 *	Choose your name instead of "schily" and make clear that the version
 *	string is related to a modified source.
 */
LOCAL	char	_scg_version[]		= "0.9";	/* The global libscg version	*/
LOCAL	char	_scg_auth_schily[]	= "schily";	/* The author for this module	*/

#define	DEFTIMEOUT	20	/* Default timeout for SCSI command transport */

EXPORT	char	*scg_version	__PR((SCSI *scgp, int what));
EXPORT	int	scg__open	__PR((SCSI *scgp, char *device));
EXPORT	int	scg__close	__PR((SCSI *scgp));
EXPORT	int	scg_numbus	__PR((SCSI *scgp));
EXPORT	BOOL	scg_havebus	__PR((SCSI *scgp, int));
EXPORT	int	scg_initiator_id __PR((SCSI *scgp));
EXPORT	int	scg_isatapi	__PR((SCSI *scgp));
EXPORT	int	scg_reset	__PR((SCSI *scgp, int what));
EXPORT	void	*scg_getbuf	__PR((SCSI *scgp, long));
EXPORT	void	scg_freebuf	__PR((SCSI *scgp));
EXPORT	long	scg_bufsize	__PR((SCSI *scgp, long));
EXPORT	void	scg_setnonstderrs __PR((SCSI *scgp, const char **));
EXPORT	BOOL	scg_yes		__PR((char *));
#ifdef	nonono
LOCAL	void	scg_sighandler	__PR((int));
#endif
EXPORT	int	scg_cmd		__PR((SCSI *scgp));
EXPORT	void	scg_vhead	__PR((SCSI *scgp));
EXPORT	int	scg_svhead	__PR((SCSI *scgp, char *buf, int maxcnt));
EXPORT	int	scg_vtail	__PR((SCSI *scgp));
EXPORT	int	scg_svtail	__PR((SCSI *scgp, int *retp, char *buf, int maxcnt));
EXPORT	void	scg_vsetup	__PR((SCSI *scgp));
EXPORT	int	scg_getresid	__PR((SCSI *scgp));
EXPORT	int	scg_getdmacnt	__PR((SCSI *scgp));
EXPORT	BOOL	scg_cmd_err	__PR((SCSI *scgp));
EXPORT	void	scg_printerr	__PR((SCSI *scgp));
EXPORT	void	scg_fprinterr	__PR((SCSI *scgp, FILE *f));
EXPORT	int	scg_sprinterr	__PR((SCSI *scgp, char *buf, int maxcnt));
EXPORT	int	scg__sprinterr	__PR((SCSI *scgp, char *buf, int maxcnt));
EXPORT	void	scg_printcdb	__PR((SCSI *scgp));
EXPORT	int	scg_sprintcdb	__PR((SCSI *scgp, char *buf, int maxcnt));
EXPORT	void	scg_printwdata	__PR((SCSI *scgp));
EXPORT	int	scg_sprintwdata	__PR((SCSI *scgp, char *buf, int maxcnt));
EXPORT	void	scg_printrdata	__PR((SCSI *scgp));
EXPORT	int	scg_sprintrdata	__PR((SCSI *scgp, char *buf, int maxcnt));
EXPORT	void	scg_printresult	__PR((SCSI *scgp));
EXPORT	int	scg_sprintresult __PR((SCSI *scgp, char *buf, int maxcnt));
EXPORT	void	scg_printstatus	__PR((SCSI *scgp));
EXPORT	int	scg_sprintstatus __PR((SCSI *scgp, char *buf, int maxcnt));
EXPORT	void	scg_fprbytes	__PR((FILE *, char *, unsigned char *, int));
EXPORT	void	scg_fprascii	__PR((FILE *, char *, unsigned char *, int));
EXPORT	void	scg_prbytes	__PR((char *, unsigned char *, int));
EXPORT	void	scg_prascii	__PR((char *, unsigned char *, int));
EXPORT	int	scg_sprbytes	__PR((char *buf, int maxcnt, char *, unsigned char *, int));
EXPORT	int	scg_sprascii	__PR((char *buf, int maxcnt, char *, unsigned char *, int));
EXPORT	void	scg_fprsense	__PR((FILE *f, unsigned char *, int));
EXPORT	int	scg_sprsense	__PR((char *buf, int maxcnt, unsigned char *, int));
EXPORT	void	scg_prsense	__PR((unsigned char *, int));
EXPORT	int	scg_cmd_status	__PR((SCSI *scgp));
EXPORT	int	scg_sense_key	__PR((SCSI *scgp));
EXPORT	int	scg_sense_code	__PR((SCSI *scgp));
EXPORT	int	scg_sense_qual	__PR((SCSI *scgp));
EXPORT	void	scg_fprintdev	__PR((FILE *, struct scsi_inquiry *));
EXPORT	void	scg_printdev	__PR((struct scsi_inquiry *));
EXPORT	int	scg_printf	__PR((SCSI *scgp, const char *form, ...));
EXPORT	int	scg_errflush	__PR((SCSI *scgp));
EXPORT	int	scg_errfflush	__PR((SCSI *scgp, FILE *f));

/*
 * Return version information for the SCSI transport code.
 * This has been introduced to make it easier to trace down problems
 * in applications.
 *
 * If scgp is NULL, return general library version information.
 * If scgp is != NULL, return version information for the low level transport.
 */
EXPORT char *
scg_version(scgp, what)
	SCSI	*scgp;
	int	what;
{
	if (scgp == (SCSI *)0) {
		switch (what) {

		case SCG_VERSION:
			return (_scg_version);
		/*
		 * If you changed this source, you are not allowed to
		 * return "schily" for the SCG_AUTHOR request.
		 */
		case SCG_AUTHOR:
			return (_scg_auth_schily);
		case SCG_SCCS_ID:
			return (sccsid);
		default:
			return ((char *)0);
		}
	}
	return (SCGO_VERSION(scgp, what));
}

/*
 * Call low level SCSI open routine from transport abstraction layer.
 */
EXPORT int
scg__open(scgp, device)
	SCSI	*scgp;
	char	*device;
{
	int	ret;
	scg_ops_t *ops;
extern	scg_ops_t scg_std_ops;

	/*
	 * Begin restricted code for quality assurance.
	 *
	 * Warning: you are not allowed to modify the quality ensurance code below.
	 *
	 * This restiction is introduced because this way, I hope that people
	 * contribute to the project instead of creating branches.
	 */
#if	!defined(IS_SCHILY_XCONFIG)
	printf("\nWarning: This version of libscg has not been configured via the standard\n");
	printf("autoconfiguration method of the Schily makefile system. There is a high risk\n");
	printf("that the code is not configured correctly and for this reason will not behave\n");
	printf("as expected.\n");
#endif
	/*
	 * End restricted code for quality assurance.
	 */

	scgp->ops = &scg_std_ops;

	if (device && strncmp(device, "REMOTE", 6) == 0) {
		ops = scg_remote();
		if (ops != NULL)
			scgp->ops = ops;
	}

	ret = SCGO_OPEN(scgp, device);
	if (ret < 0)
		return (ret);

	/*
	 * Now make scgp->fd valid if possible.
	 * Note that scg_scsibus(scgp)/scg_target(scgp)/scg_lun(scgp) may have
	 * changed in SCGO_OPEN().
	 */
	scg_settarget(scgp, scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp));
	return (ret);
}

/*
 * Call low level SCSI close routine from transport abstraction layer.
 */
EXPORT int
scg__close(scgp)
	SCSI	*scgp;
{
	return (SCGO_CLOSE(scgp));
}

/*
 * Retrieve max DMA count for this target.
 */
EXPORT long
scg_bufsize(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	long	maxdma;

	maxdma = SCGO_MAXDMA(scgp, amt);
	if (amt <= 0 || amt > maxdma)
		amt = maxdma;

	scgp->maxdma = maxdma;	/* Max possible  */
	scgp->maxbuf = amt;	/* Current value */

	return (amt);
}

/*
 * Allocate a buffer that may be used for DMA.
 */
EXPORT void *
scg_getbuf(scgp, amt)
	SCSI	*scgp;
	long	amt;
{
	void	*buf;

	if (amt <= 0 || amt > scg_bufsize(scgp, amt))
		return ((void *)0);

	buf = SCGO_GETBUF(scgp, amt);
	scgp->bufptr = buf;
	return (buf);
}

/*
 * Free DMA buffer.
 */
EXPORT void
scg_freebuf(scgp)
	SCSI	*scgp;
{
	SCGO_FREEBUF(scgp);
	scgp->bufptr = NULL;
}

/*
 * Return the max. number of SCSI busses.
 */
EXPORT BOOL
scg_numbus(scgp)
	SCSI	*scgp;
{
	return (SCGO_NUMBUS(scgp));
}

/*
 * Check if 'busno' is a valid SCSI bus number.
 */
EXPORT BOOL
scg_havebus(scgp, busno)
	SCSI	*scgp;
	int	busno;
{
	return (SCGO_HAVEBUS(scgp, busno));
}

/*
 * Return SCSI initiator ID for current SCSI bus if available.
 */
EXPORT int
scg_initiator_id(scgp)
	SCSI	*scgp;
{
	return (SCGO_INITIATOR_ID(scgp));
}

/*
 * Return a hint whether current SCSI target refers to a ATAPI device.
 */
EXPORT int
scg_isatapi(scgp)
	SCSI	*scgp;
{
	return (SCGO_ISATAPI(scgp));
}

/*
 * Reset SCSI bus or target.
 */
EXPORT int
scg_reset(scgp, what)
	SCSI	*scgp;
	int	what;
{
	return (SCGO_RESET(scgp, what));
}

/*
 * Set up nonstd error vector for curren target.
 * To clear additional error table, call scg_setnonstderrs(scgp, NULL);
 * Note: do not use this when scanning the SCSI bus.
 */
EXPORT void
scg_setnonstderrs(scgp, vec)
	SCSI	*scgp;
	const char **vec;
{
	scgp->nonstderrs = vec;
}

/*
 * Simple Yes/No answer checker.
 */
EXPORT BOOL
scg_yes(msg)
	char	*msg;
{
	char okbuf[10];

	js_printf("%s", msg);
	flush();
	if (getline(okbuf, sizeof (okbuf)) == EOF)
		exit(EX_BAD);
	if (streql(okbuf, "y") || streql(okbuf, "yes") ||
	    streql(okbuf, "Y") || streql(okbuf, "YES"))
		return (TRUE);
	else
		return (FALSE);
}

#ifdef	nonono
LOCAL void
scg_sighandler(sig)
	int	sig;
{
	js_printf("\n");
	if (scsi_running) {
		js_printf("Running command: %s\n", scsi_command);
		js_printf("Resetting SCSI - Bus.\n");
		if (scg_reset(scgp) < 0)
			errmsg("Cannot reset SCSI - Bus.\n");
	}
	if (scg_yes("EXIT ? "))
		exit(sig);
}
#endif

/*
 * Send a SCSI command.
 * Do error checking and reporting depending on the values of
 * scgp->verbose, scgp->debug and scgp->silent.
 */
EXPORT int
scg_cmd(scgp)
	SCSI	*scgp;
{
		int		ret;
	register struct	scg_cmd	*scmd = scgp->scmd;

	/*
	 * Reset old error messages in scgp->errstr
	 */
	scgp->errptr = scgp->errbeg = scgp->errstr;

	scmd->kdebug = scgp->kdebug;
	if (scmd->timeout == 0 || scmd->timeout < scgp->deftimeout)
		scmd->timeout = scgp->deftimeout;
	if (scgp->disre_disable)
		scmd->flags &= ~SCG_DISRE_ENA;
	if (scgp->noparity)
		scmd->flags |= SCG_NOPARITY;

	scmd->u_sense.cmd_sense[0] = 0;		/* Paranioa */
	if (scmd->sense_len > SCG_MAX_SENSE)
		scmd->sense_len = SCG_MAX_SENSE;
	else if (scmd->sense_len < 0)
		scmd->sense_len = 0;

	if (scgp->verbose) {
		scg_vhead(scgp);
		scg_errflush(scgp);
	}

	if (scgp->running) {
		if (scgp->curcmdname) {
			error("Currently running '%s' command.\n",
							scgp->curcmdname);
		}
		raisecond("SCSI ALREADY RUNNING !!", 0L);
	}
	scgp->cb_fun = NULL;
	gettimeofday(scgp->cmdstart, (struct timezone *)0);
	scgp->curcmdname = scgp->cmdname;
	scgp->running = TRUE;
	ret = SCGO_SEND(scgp);
	scgp->running = FALSE;
	__scg_times(scgp);
	if (scgp->flags & SCGF_IGN_RESID)
		scmd->resid = 0;
	if (ret < 0) {
		if (scmd->ux_errno == 0)
			scmd->ux_errno = geterrno();
		if (scmd->error == SCG_NO_ERROR)
			scmd->error = SCG_FATAL;
		if (scgp->debug > 0) {
			errmsg("ret < 0 errno: %d ux_errno: %d error: %d\n",
					geterrno(), scmd->ux_errno, scmd->error);
		}
		if (scmd->ux_errno == EPERM && scgp->flags & SCGF_PERM_PRINT) {
			char	errbuf[SCSI_ERRSTR_SIZE];
			int	amt;

			amt = scg__sprinterr(scgp, errbuf, sizeof (errbuf));
			if (amt > 0) {
				FILE	*f = scgp->errfile;

				if (f == NULL)
					f = stderr;
				filewrite(f, errbuf, amt);
				ferrmsgno(f, scmd->ux_errno,
					"Cannot send SCSI cmd via ioctl.\n");
				fflush(f);
			}
		}
		/*
		 * Old /dev/scg versions will not allow to access targets > 7.
		 * Include a workaround to make this non fatal.
		 */
		if (scg_target(scgp) < 8 || scmd->ux_errno != EINVAL) {

			if (scmd->ux_errno != EPERM ||
			    (scgp->flags & SCGF_PERM_PRINT) == 0) {
				errmsgno(scmd->ux_errno,
					"Cannot send SCSI cmd via ioctl.\n");
			}
			if (scgp->flags & SCGF_PERM_EXIT)
				comexit(scmd->ux_errno);
		}
	}

	ret = scg_vtail(scgp);
	scg_errflush(scgp);
	if (scgp->cb_fun != NULL)
		(*scgp->cb_fun)(scgp->cb_arg);
	return (ret);
}

/*
 * Fill the head of verbose printing into the SCSI error buffer.
 * Action depends on SCSI verbose status.
 */
EXPORT void
scg_vhead(scgp)
	SCSI	*scgp;
{
	scgp->errptr += scg_svhead(scgp, scgp->errptr, scg_errrsize(scgp));
}

/*
 * Fill the head of verbose printing into a buffer.
 * Action depends on SCSI verbose status.
 */
EXPORT int
scg_svhead(scgp, buf, maxcnt)
	SCSI	*scgp;
	char	*buf;
	int	maxcnt;
{
	register char	*p = buf;
	register int	amt;

	if (scgp->verbose <= 0)
		return (0);

	amt = js_snprintf(p, maxcnt,
		"\nExecuting '%s' command on Bus %d Target %d, Lun %d timeout %ds\n",
								/* XXX Really this ??? */
/*		scgp->cmdname, scg_scsibus(scgp), scg_target(scgp), scgp->scmd->cdb.g0_cdb.lun,*/
		scgp->cmdname, scg_scsibus(scgp), scg_target(scgp), scg_lun(scgp),
		scgp->scmd->timeout);
	if (amt < 0)
		return (amt);
	p += amt;
	maxcnt -= amt;

	amt = scg_sprintcdb(scgp, p, maxcnt);
	if (amt < 0)
		return (amt);
	p += amt;
	maxcnt -= amt;

	if (scgp->verbose > 1) {
		amt = scg_sprintwdata(scgp, p, maxcnt);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
	}
	return (p - buf);
}

/*
 * Fill the tail of verbose printing into the SCSI error buffer.
 * Action depends on SCSI verbose status.
 */
EXPORT int
scg_vtail(scgp)
	SCSI	*scgp;
{
	int	ret;

	scgp->errptr += scg_svtail(scgp, &ret, scgp->errptr, scg_errrsize(scgp));
	return (ret);
}

/*
 * Fill the tail of verbose printing into a buffer.
 * Action depends on SCSI verbose status.
 */
EXPORT int
scg_svtail(scgp, retp, buf, maxcnt)
	SCSI	*scgp;
	int	*retp;
	char	*buf;
	int	maxcnt;
{
	register char	*p = buf;
	register int	amt;
	int	ret;

	ret = scg_cmd_err(scgp) ? -1 : 0;
	if (retp)
		*retp = ret;
	if (ret) {
		if (scgp->silent <= 0 || scgp->verbose) {
			amt = scg__sprinterr(scgp, p, maxcnt);
			if (amt < 0)
				return (amt);
			p += amt;
			maxcnt -= amt;
		}
	}
	if ((scgp->silent <= 0 || scgp->verbose) && scgp->scmd->resid) {
		if (scgp->scmd->resid < 0) {
			/*
			 * An operating system that does DMA the right way
			 * will not allow DMA overruns - it will stop DMA
			 * before bad things happen.
			 * A DMA residual count < 0 (-1) is a hint for a DMA
			 * overrun but does not affect the transfer count.
			 */
			amt = js_snprintf(p, maxcnt, "DMA overrun, ");
			if (amt < 0)
				return (amt);
			p += amt;
			maxcnt -= amt;
		}
		amt = js_snprintf(p, maxcnt, "resid: %d\n", scgp->scmd->resid);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
	}
	if (scgp->verbose > 0 || (ret < 0 && scgp->silent <= 0)) {
		amt = scg_sprintresult(scgp, p, maxcnt);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
	}
	return (p - buf);
}

/*
 * Set up SCSI error buffer with verbose print data.
 * Action depends on SCSI verbose status.
 */
EXPORT void
scg_vsetup(scgp)
	SCSI	*scgp;
{
	scg_vhead(scgp);
	scg_vtail(scgp);
}

/*
 * Return the residual DMA count for last command.
 * If this count is < 0, then a DMA overrun occured.
 */
EXPORT int
scg_getresid(scgp)
	SCSI	*scgp;
{
	return (scgp->scmd->resid);
}

/*
 * Return the actual DMA count for last command.
 */
EXPORT int
scg_getdmacnt(scgp)
	SCSI	*scgp;
{
	register struct scg_cmd *scmd = scgp->scmd;

	if (scmd->resid < 0)
		return (scmd->size);

	return (scmd->size - scmd->resid);
}

/*
 * Test if last SCSI command got an error.
 */
EXPORT BOOL
scg_cmd_err(scgp)
	SCSI	*scgp;
{
	register struct scg_cmd *cp = scgp->scmd;

	if (cp->error != SCG_NO_ERROR ||
				cp->ux_errno != 0 ||
				*(Uchar *)&cp->scb != 0 ||
				cp->u_sense.cmd_sense[0] != 0)	/* Paranioa */
		return (TRUE);
	return (FALSE);
}

/*
 * Used to print error messges if the command itself has been run silently.
 *
 * print the following SCSI codes:
 *
 * -	command transport status
 * -	CDB
 * -	SCSI status byte
 * -	Sense Bytes
 * -	Decoded Sense data
 * -	DMA status
 * -	SCSI timing
 *
 * to SCSI errfile.
 */
EXPORT void
scg_printerr(scgp)
	SCSI	*scgp;
{
	scg_fprinterr(scgp, (FILE *)scgp->errfile);
}

/*
 * print the following SCSI codes:
 *
 * -	command transport status
 * -	CDB
 * -	SCSI status byte
 * -	Sense Bytes
 * -	Decoded Sense data
 * -	DMA status
 * -	SCSI timing
 *
 * to a file.
 */
EXPORT void
scg_fprinterr(scgp, f)
	SCSI	*scgp;
	FILE	*f;
{
	char	errbuf[SCSI_ERRSTR_SIZE];
	int	amt;

	amt = scg_sprinterr(scgp, errbuf, sizeof (errbuf));
	if (amt > 0) {
		filewrite(f, errbuf, amt);
		fflush(f);
	}
}

/*
 * print the following SCSI codes:
 *
 * -	command transport status
 * -	CDB
 * -	SCSI status byte
 * -	Sense Bytes
 * -	Decoded Sense data
 * -	DMA status
 * -	SCSI timing
 *
 * into a buffer.
 */
EXPORT int
scg_sprinterr(scgp, buf, maxcnt)
	SCSI	*scgp;
	char	*buf;
	int	maxcnt;
{
	int	amt;
	int	osilent = scgp->silent;
	int	overbose = scgp->verbose;

	scgp->silent = 0;
	scgp->verbose = 0;
	amt = scg_svtail(scgp, NULL, buf, maxcnt);
	scgp->silent = osilent;
	scgp->verbose = overbose;
	return (amt);
}

/*
 * print the following SCSI codes:
 *
 * -	command transport status
 * -	CDB
 * -	SCSI status byte
 * -	Sense Bytes
 * -	Decoded Sense data
 *
 * into a buffer.
 */
EXPORT int
scg__sprinterr(scgp, buf, maxcnt)
	SCSI	*scgp;
	char	*buf;
	int	maxcnt;
{
	register struct scg_cmd *cp = scgp->scmd;
	register char		*err;
		char		*cmdname = "SCSI command name not set by caller";
		char		errbuf[64];
	register char		*p = buf;
	register int		amt;

	switch (cp->error) {

	case SCG_NO_ERROR :	err = "no error"; break;
	case SCG_RETRYABLE:	err = "retryable error"; break;
	case SCG_FATAL    :	err = "fatal error"; break;
				/*
				 * We need to cast timeval->* to long because
				 * of the broken sys/time.h in Linux.
				 */
	case SCG_TIMEOUT  :	js_snprintf(errbuf, sizeof (errbuf),
					"cmd timeout after %ld.%03ld (%d) s",
					(long)scgp->cmdstop->tv_sec,
					(long)scgp->cmdstop->tv_usec/1000,
								cp->timeout);
				err = errbuf;
				break;
	default:		js_snprintf(errbuf, sizeof (errbuf),
					"error: %d", cp->error);
				err = errbuf;
	}

	if (scgp->cmdname != NULL && scgp->cmdname[0] != '\0')
		cmdname = scgp->cmdname;
	amt = serrmsgno(cp->ux_errno, p, maxcnt, "%s: scsi sendcmd: %s\n", cmdname, err);
	if (amt < 0)
		return (amt);
	p += amt;
	maxcnt -= amt;

	amt = scg_sprintcdb(scgp, p, maxcnt);
	if (amt < 0)
		return (amt);
	p += amt;
	maxcnt -= amt;

	if (cp->error <= SCG_RETRYABLE) {
		amt = scg_sprintstatus(scgp, p, maxcnt);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
	}

	if (cp->scb.chk) {
		amt = scg_sprsense(p, maxcnt, (Uchar *)&cp->sense, cp->sense_count);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
		amt = scg__errmsg(scgp, p, maxcnt, &cp->sense, &cp->scb, -1);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
	}
	return (p - buf);
}

/*
 * XXX Do we need this function?
 *
 * print the SCSI Command descriptor block to XXX stderr.
 */
EXPORT void
scg_printcdb(scgp)
	SCSI	*scgp;
{
	scg_prbytes("CDB: ", scgp->scmd->cdb.cmd_cdb, scgp->scmd->cdb_len);
}

/*
 * print the SCSI Command descriptor block into a buffer.
 */
EXPORT int
scg_sprintcdb(scgp, buf, maxcnt)
	SCSI	*scgp;
	char	*buf;
	int	maxcnt;
{
	int	cnt;

	cnt = scg_sprbytes(buf, maxcnt, "CDB: ", scgp->scmd->cdb.cmd_cdb, scgp->scmd->cdb_len);
	if (cnt < 0)
		cnt = 0;
	return (cnt);
}

/*
 * XXX Do we need this function?
 * XXX scg_printrdata() is used.
 * XXX We need to check if we should write to stderr or better to scg->errfile
 *
 * print the SCSI send data to stderr.
 */
EXPORT void
scg_printwdata(scgp)
	SCSI	*scgp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;

	if (scmd->size > 0 && (scmd->flags & SCG_RECV_DATA) == 0) {
		js_fprintf(stderr, "Sending %d (0x%X) bytes of data.\n",
			scmd->size, scmd->size);
		scg_prbytes("Write Data: ",
			(Uchar *)scmd->addr,
			scmd->size > 100 ? 100 : scmd->size);
	}
}

/*
 * print the SCSI send data into a buffer.
 */
EXPORT int
scg_sprintwdata(scgp, buf, maxcnt)
	SCSI	*scgp;
	char	*buf;
	int	maxcnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;
	register char		*p = buf;
	register int		amt;

	if (scmd->size > 0 && (scmd->flags & SCG_RECV_DATA) == 0) {
		amt = js_snprintf(p, maxcnt,
			"Sending %d (0x%X) bytes of data.\n",
			scmd->size, scmd->size);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
		amt = scg_sprbytes(p, maxcnt, "Write Data: ",
			(Uchar *)scmd->addr,
			scmd->size > 100 ? 100 : scmd->size);
		if (amt < 0)
			return (amt);
		p += amt;
	}
	return (p - buf);
}

/*
 * XXX We need to check if we should write to stderr or better to scg->errfile
 *
 * print the SCSI received data to stderr.
 */
EXPORT void
scg_printrdata(scgp)
	SCSI	*scgp;
{
	register struct	scg_cmd	*scmd = scgp->scmd;
	register int		trcnt = scg_getdmacnt(scgp);

	if (scmd->size > 0 && (scmd->flags & SCG_RECV_DATA) != 0) {
		js_fprintf(stderr, "Got %d (0x%X), expecting %d (0x%X) bytes of data.\n",
			trcnt, trcnt,
			scmd->size, scmd->size);
		scg_prbytes("Received Data: ",
			(Uchar *)scmd->addr,
			trcnt > 100 ? 100 : trcnt);
	}
}

/*
 * print the SCSI received data into a buffer.
 */
EXPORT int
scg_sprintrdata(scgp, buf, maxcnt)
	SCSI	*scgp;
	char	*buf;
	int	maxcnt;
{
	register struct	scg_cmd	*scmd = scgp->scmd;
	register char		*p = buf;
	register int		amt;
	register int		trcnt = scg_getdmacnt(scgp);

	if (scmd->size > 0 && (scmd->flags & SCG_RECV_DATA) != 0) {
		amt = js_snprintf(p, maxcnt,
			"Got %d (0x%X), expecting %d (0x%X) bytes of data.\n",
			trcnt, trcnt,
			scmd->size, scmd->size);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
		amt = scg_sprbytes(p, maxcnt,
			"Received Data: ",
			(Uchar *)scmd->addr,
			trcnt > 100 ? 100 : trcnt);
		if (amt < 0)
			return (amt);
		p += amt;
	}
	return (p - buf);
}

/*
 * XXX We need to check if we should write to stderr or better to scg->errfile
 *
 * print the SCSI timings and (depending on verbose) received data to stderr.
 */
EXPORT void
scg_printresult(scgp)
	SCSI	*scgp;
{
	js_fprintf(stderr, "cmd finished after %ld.%03lds timeout %ds\n",
		(long)scgp->cmdstop->tv_sec,
		(long)scgp->cmdstop->tv_usec/1000,
		scgp->scmd->timeout);
	if (scgp->verbose > 1)
		scg_printrdata(scgp);
	flush();
}

/*
 * print the SCSI timings and (depending on verbose) received data into a buffer.
 */
EXPORT int
scg_sprintresult(scgp, buf, maxcnt)
	SCSI	*scgp;
	char	*buf;
	int	maxcnt;
{
	register char		*p = buf;
	register int		amt;

	amt = js_snprintf(p, maxcnt,
		"cmd finished after %ld.%03lds timeout %ds\n",
		(long)scgp->cmdstop->tv_sec,
		(long)scgp->cmdstop->tv_usec/1000,
		scgp->scmd->timeout);
	if (amt < 0)
		return (amt);
	p += amt;
	maxcnt -= amt;
	if (scgp->verbose > 1) {
		amt = scg_sprintrdata(scgp, p, maxcnt);
		if (amt < 0)
			return (amt);
		p += amt;
	}
	return (p - buf);
}

/*
 * XXX Do we need this function?
 *
 * print the SCSI status byte in human readable form to the SCSI error file.
 */
EXPORT void
scg_printstatus(scgp)
	SCSI	*scgp;
{
	char	errbuf[SCSI_ERRSTR_SIZE];
	int	amt;

	amt = scg_sprintstatus(scgp, errbuf, sizeof (errbuf));
	if (amt > 0) {
		filewrite((FILE *)scgp->errfile, errbuf, amt);
		fflush((FILE *)scgp->errfile);
	}
}

/*
 * print the SCSI status byte in human readable form into a buffer.
 */
EXPORT int
scg_sprintstatus(scgp, buf, maxcnt)
	SCSI	*scgp;
	char	*buf;
	int	maxcnt;
{
	register struct scg_cmd *cp = scgp->scmd;
		char	*err;
		char	*err2 = "";
	register char	*p = buf;
	register int	amt;

	amt = js_snprintf(p, maxcnt, "status: 0x%x ", *(Uchar *)&cp->scb);
	if (amt < 0)
		return (amt);
	p += amt;
	maxcnt -= amt;
#ifdef	SCSI_EXTENDED_STATUS
	if (cp->scb.ext_st1) {
		amt = js_snprintf(p, maxcnt, "0x%x ", ((Uchar *)&cp->scb)[1]);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
	}
	if (cp->scb.ext_st2) {
		amt = js_snprintf(p, maxcnt, "0x%x ", ((Uchar *)&cp->scb)[2]);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
	}
#endif
	switch (*(Uchar *)&cp->scb & 036) {

	case 0  : err = "GOOD STATUS";			break;
	case 02 : err = "CHECK CONDITION";		break;
	case 04 : err = "CONDITION MET/GOOD";		break;
	case 010: err = "BUSY";				break;
	case 020: err = "INTERMEDIATE GOOD STATUS";	break;
	case 024: err = "INTERMEDIATE CONDITION MET/GOOD"; break;
	case 030: err = "RESERVATION CONFLICT";		break;
	default : err = "Reserved";			break;
	}
#ifdef	SCSI_EXTENDED_STATUS
	if (cp->scb.ext_st1 && cp->scb.ha_er)
		err2 = " host adapter detected error";
#endif
	amt = js_snprintf(p, maxcnt, "(%s%s)\n", err, err2);
	if (amt < 0)
		return (amt);
	p += amt;
	return (p - buf);
}

/*
 * print some bytes in hex to a file.
 */
EXPORT void
scg_fprbytes(f, s, cp, n)
		FILE	*f;
		char	*s;
	register Uchar	*cp;
	register int	n;
{
	js_fprintf(f, "%s", s);
	while (--n >= 0)
		js_fprintf(f, " %02X", *cp++);
	js_fprintf(f, "\n");
}

/*
 * print some bytes in ascii to a file.
 */
EXPORT void
scg_fprascii(f, s, cp, n)
		FILE	*f;
		char	*s;
	register Uchar	*cp;
	register int	n;
{
	register int	c;

	js_fprintf(f, "%s", s);
	while (--n >= 0) {
		c = *cp++;
		if (c >= ' ' && c < 0177)
			js_fprintf(f, "%c", c);
		else
			js_fprintf(f, ".");
	}
	js_fprintf(f, "\n");
}

/*
 * XXX We need to check if we should write to stderr or better to scg->errfile
 *
 * print some bytes in hex to stderr.
 */
EXPORT void
scg_prbytes(s, cp, n)
		char	*s;
	register Uchar	*cp;
	register int	n;
{
	scg_fprbytes(stderr, s, cp, n);
}

/*
 * XXX We need to check if we should write to stderr or better to scg->errfile
 *
 * print some bytes in ascii to stderr.
 */
EXPORT void
scg_prascii(s, cp, n)
		char	*s;
	register Uchar	*cp;
	register int	n;
{
	scg_fprascii(stderr, s, cp, n);
}

/*
 * print some bytes in hex into a buffer.
 */
EXPORT int
scg_sprbytes(buf, maxcnt, s, cp, n)
		char	*buf;
		int	maxcnt;
		char	*s;
	register Uchar	*cp;
	register int	n;
{
	register char	*p = buf;
	register int	amt;

	amt = js_snprintf(p, maxcnt, "%s", s);
	if (amt < 0)
		return (amt);
	p += amt;
	maxcnt -= amt;

	while (--n >= 0) {
		amt = js_snprintf(p, maxcnt, " %02X", *cp++);
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
	}
	amt = js_snprintf(p, maxcnt, "\n");
	if (amt < 0)
		return (amt);
	p += amt;
	return (p - buf);
}

/*
 * print some bytes in ascii into a buffer.
 */
EXPORT int
scg_sprascii(buf, maxcnt, s, cp, n)
		char	*buf;
		int	maxcnt;
		char	*s;
	register Uchar	*cp;
	register int	n;
{
	register char	*p = buf;
	register int	amt;
	register int	c;

	amt = js_snprintf(p, maxcnt, "%s", s);
	if (amt < 0)
		return (amt);
	p += amt;
	maxcnt -= amt;

	while (--n >= 0) {
		c = *cp++;
		if (c >= ' ' && c < 0177)
			amt = js_snprintf(p, maxcnt, "%c", c);
		else
			amt = js_snprintf(p, maxcnt, ".");
		if (amt < 0)
			return (amt);
		p += amt;
		maxcnt -= amt;
	}
	amt = js_snprintf(p, maxcnt, "\n");
	if (amt < 0)
		return (amt);
	p += amt;
	return (p - buf);
}

/*
 * print the SCSI sense data for last command in hex to a file.
 */
EXPORT void
scg_fprsense(f, cp, n)
	FILE	*f;
	Uchar	*cp;
	int	n;
{
	scg_fprbytes(f, "Sense Bytes:", cp, n);
}

/*
 * XXX We need to check if we should write to stderr or better to scg->errfile
 *
 * print the SCSI sense data for last command in hex to stderr.
 */
EXPORT void
scg_prsense(cp, n)
	Uchar	*cp;
	int	n;
{
	scg_fprsense(stderr, cp, n);
}

/*
 * print the SCSI sense data for last command in hex into a buffer.
 */
EXPORT int
scg_sprsense(buf, maxcnt, cp, n)
	char	*buf;
	int	maxcnt;
	Uchar	*cp;
	int	n;
{
	return (scg_sprbytes(buf, maxcnt, "Sense Bytes:", cp, n));
}

/*
 * Return the SCSI status byte for last command.
 */
EXPORT int
scg_cmd_status(scgp)
	SCSI	*scgp;
{
	struct scg_cmd	*cp = scgp->scmd;
	int		cmdstatus = *(Uchar *)&cp->scb;

	return (cmdstatus);
}

/*
 * Return the SCSI sense key for last command.
 */
EXPORT int
scg_sense_key(scgp)
	SCSI	*scgp;
{
	register struct scg_cmd *cp = scgp->scmd;
	int	key = -1;

	if (!scg_cmd_err(scgp))
		return (0);

	if (cp->sense.code >= 0x70)
		key = ((struct scsi_ext_sense *)&(cp->sense))->key;
	return (key);
}

/*
 * Return the SCSI sense code for last command.
 */
EXPORT int
scg_sense_code(scgp)
	SCSI	*scgp;
{
	register struct scg_cmd *cp = scgp->scmd;
	int	code = -1;

	if (!scg_cmd_err(scgp))
		return (0);

	if (cp->sense.code >= 0x70)
		code = ((struct scsi_ext_sense *)&(cp->sense))->sense_code;
	else
		code = cp->sense.code;
	return (code);
}

/*
 * Return the SCSI sense qualifier for last command.
 */
EXPORT int
scg_sense_qual(scgp)
	SCSI	*scgp;
{
	register struct scg_cmd *cp = scgp->scmd;

	if (!scg_cmd_err(scgp))
		return (0);

	if (cp->sense.code >= 0x70)
		return (((struct scsi_ext_sense *)&(cp->sense))->qual_code);
	else
		return (0);
}

/*
 * Print the device type from the SCSI inquiry buffer to file.
 */
EXPORT void
scg_fprintdev(f, ip)
		FILE	*f;
	struct	scsi_inquiry *ip;
{
	if (ip->removable)
		js_fprintf(f, "Removable ");
	if (ip->data_format >= 2) {
		switch (ip->qualifier) {

		case INQ_DEV_PRESENT:
			break;
		case INQ_DEV_NOTPR:
			js_fprintf(f, "not present ");
			break;
		case INQ_DEV_RES:
			js_fprintf(f, "reserved ");
			break;
		case INQ_DEV_NOTSUP:
			if (ip->type == INQ_NODEV) {
				js_fprintf(f, "unsupported\n"); return;
			}
			js_fprintf(f, "unsupported ");
			break;
		default:
			js_fprintf(f, "vendor specific %d ",
						(int)ip->qualifier);
		}
	}
	switch (ip->type) {

	case INQ_DASD:
		js_fprintf(f, "Disk");		break;
	case INQ_SEQD:
		js_fprintf(f, "Tape");		break;
	case INQ_PRTD:
		js_fprintf(f, "Printer");	break;
	case INQ_PROCD:
		js_fprintf(f, "Processor");	break;
	case INQ_WORM:
		js_fprintf(f, "WORM");		break;
	case INQ_ROMD:
		js_fprintf(f, "CD-ROM");	break;
	case INQ_SCAN:
		js_fprintf(f, "Scanner");	break;
	case INQ_OMEM:
		js_fprintf(f, "Optical Storage"); break;
	case INQ_JUKE:
		js_fprintf(f, "Juke Box");	break;
	case INQ_COMM:
		js_fprintf(f, "Communication");	break;
	case INQ_IT8_1:
		js_fprintf(f, "IT8 1");		break;
	case INQ_IT8_2:
		js_fprintf(f, "IT8 2");		break;
	case INQ_STARR:
		js_fprintf(f, "Storage array");	break;
	case INQ_ENCL:
		js_fprintf(f, "Enclosure services"); break;
	case INQ_SDAD:
		js_fprintf(f, "Simple direct access"); break;
	case INQ_OCRW:
		js_fprintf(f, "Optical card r/w"); break;
	case INQ_BRIDGE:
		js_fprintf(f, "Bridging expander"); break;
	case INQ_OSD:
		js_fprintf(f, "Object based storage"); break;
	case INQ_ADC:
		js_fprintf(f, "Automation/Drive Interface"); break;
	case INQ_WELLKNOWN:
		js_fprintf(f, "Well known lun"); break;

	case INQ_NODEV:
		if (ip->data_format >= 2) {
			js_fprintf(f, "unknown/no device");
			break;
		} else if (ip->qualifier == INQ_DEV_NOTSUP) {
			js_fprintf(f, "unit not present");
			break;
		}
	default:
		js_fprintf(f, "unknown device type 0x%x",
						(int)ip->type);
	}
	js_fprintf(f, "\n");
}

/*
 * Print the device type from the SCSI inquiry buffer to stdout.
 */
EXPORT void
scg_printdev(ip)
	struct	scsi_inquiry *ip;
{
	scg_fprintdev(stdout, ip);
}

#include <schily/varargs.h>

/*
 * print into the SCSI error buffer, adjust the next write pointer.
 */
/* VARARGS2 */
EXPORT int
#ifdef	PROTOTYPES
scg_printf(SCSI *scgp, const char *form, ...)
#else
scg_printf(scgp, form, va_alist)
	SCSI	*scgp;
	char	*form;
	va_dcl
#endif
{
	int	cnt;
	va_list	args;

#ifdef	PROTOTYPES
	va_start(args, form);
#else
	va_start(args);
#endif
	cnt = js_snprintf(scgp->errptr, scg_errrsize(scgp), "%r", form, args);
	va_end(args);

	if (cnt < 0) {
		scgp->errptr[0] = '\0';
	} else {
		scgp->errptr += cnt;
	}
	return (cnt);
}

/*
 * Flush the SCSI error buffer to SCSI errfile.
 * Clear error buffer after flushing.
 */
EXPORT int
scg_errflush(scgp)
	SCSI	*scgp;
{
	if (scgp->errfile == NULL)
		return (0);

	return (scg_errfflush(scgp, (FILE *)scgp->errfile));
}

/*
 * Flush the SCSI error buffer to a file.
 * Clear error buffer after flushing.
 */
EXPORT int
scg_errfflush(scgp, f)
	SCSI	*scgp;
	FILE	*f;
{
	int	cnt;

	cnt = scgp->errptr - scgp->errbeg;
	if (cnt > 0) {
		filewrite(f, scgp->errbeg, cnt);
		fflush(f);
		scgp->errbeg = scgp->errptr;
	}
	return (cnt);
}
