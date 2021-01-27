/* @(#)scsitransp.h	1.58 16/01/21 Copyright 1995-2016 J. Schilling */
/*
 *	Definitions for commands that use functions from scsitransp.c
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

#ifndef	_SCG_SCSITRANSP_H
#define	_SCG_SCSITRANSP_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef	struct scg_scsi	SCSI;

typedef struct {
	int	scsibus;	/* SCSI bus #    for next I/O		*/
	int	target;		/* SCSI target # for next I/O		*/
	int	lun;		/* SCSI lun #    for next I/O		*/
} scg_addr_t;

#ifndef	_SCG_SCGOPS_H
#include <scg/scgops.h>
#endif

typedef	int	(*scg_cb_t)	__PR((void *));

struct scg_scsi {
	scg_ops_t *ops;		/* Ptr to low level SCSI transport ops	*/
	int	fd;		/* File descriptor for next I/O		*/
	scg_addr_t	addr;	/* SCSI address for next I/O		*/
	int	flags;		/* Libscg flags (see below)		*/
	int	dflags;		/* Drive specific flags (see below)	*/
	int	kdebug;		/* Kernel debug value for next I/O	*/
	int	debug;		/* Debug value for SCSI library		*/
	int	silent;		/* Be silent if value > 0		*/
	int	verbose;	/* Be verbose if value > 0		*/
	int	overbose;	/* Be verbose in open() if value > 0	*/
	int	disre_disable;
	int	deftimeout;
	int	noparity;	/* Do not use SCSI parity fo next I/O	*/
	int	dev;		/* from scsi_cdr.c			*/
	struct scg_cmd *scmd;
	char	*cmdname;
	char	*curcmdname;
	BOOL	running;
	int	error;		/* libscg error number			*/

	long	maxdma;		/* Max DMA limit for this open instance	*/
	long	maxbuf;		/* Cur DMA buffer limit for this inst.	*/
				/* This is the size behind bufptr	*/
	struct timeval	*cmdstart;
	struct timeval	*cmdstop;
	const char	**nonstderrs;
	void	*local;		/* Local data from the low level code	*/
	void	*bufbase;	/* needed for scsi_freebuf()		*/
	void	*bufptr;	/* DMA buffer pointer for appl. use	*/
	char	*errstr;	/* Error string for scsi_open/sendmcd	*/
	char	*errbeg;	/* Pointer to begin of not flushed data	*/
	char	*errptr;	/* Actual write pointer into errstr	*/
	void	*errfile;	/* FILE to write errors to. NULL for not*/
				/* writing and leaving errs in errstr	*/
	scg_cb_t cb_fun;
	void	*cb_arg;

	struct scsi_inquiry *inq;
	struct scsi_capacity *cap;
};

/*
 * Macros for accessing members of the scg address structure.
 * scg_settarget() is the only function that is allowed to modify
 * the values of the SCSI address.
 */
#define	scg_scsibus(scgp)	(scgp)->addr.scsibus
#define	scg_target(scgp)	(scgp)->addr.target
#define	scg_lun(scgp)		(scgp)->addr.lun

/*
 * Flags for struct SCSI:
 */
#define	SCGF_PERM_EXIT	0x01		/* Exit on permission problems */
#define	SCGF_PERM_PRINT	0x02		/* Print msg on permission problems */
#define	SCGF_IGN_RESID	0x04		/* Ignore DMA residual count	*/

#define	SCGF_USER_FLAGS	0x04		/* All user definable flags */

/*
 * Drive specific flags for struct SCSI:
 */
#define	DRF_MODE_DMA_OVR	0x0001		/* Drive gives DMA overrun */
						/* on mode sense	   */

#define	SCSI_ERRSTR_SIZE	4096

/*
 * Libscg error codes:
 */
#define	SCG_ERRBASE		1000000
#define	SCG_NOMEM		1000001

/*
 * Function codes for scg_version():
 */
#define	SCG_VERSION		0	/* libscg or transport version */
#define	SCG_AUTHOR		1	/* Author of above */
#define	SCG_SCCS_ID		2	/* SCCS id of above */
#define	SCG_RVERSION		10	/* Remote transport version */
#define	SCG_RAUTHOR		11	/* Remote transport author */
#define	SCG_RSCCS_ID		12	/* Remote transport SCCS ID */
#define	SCG_KVERSION		20	/* Kernel transport version */

/*
 * Function codes for scg_reset():
 */
#define	SCG_RESET_NOP		0	/* Test if reset is supported */
#define	SCG_RESET_TGT		1	/* Reset Target only */
#define	SCG_RESET_BUS		2	/* Reset complete SCSI Bus */

/*
 * Helpers for the error buffer in SCSI*
 */
#define	scg_errsize(scgp)	((scgp)->errptr - (scgp)->errstr)
#define	scg_errrsize(scgp)	(SCSI_ERRSTR_SIZE - scg_errsize(scgp))

/*
 * From scsitransp.c:
 */
extern	char	*scg_version		__PR((SCSI *scgp, int what));
extern	int	scg__open		__PR((SCSI *scgp, char *device));
extern	int	scg__close		__PR((SCSI *scgp));
extern	int	scg_numbus		__PR((SCSI *scgp));
extern	BOOL	scg_havebus		__PR((SCSI *scgp, int));
extern	int	scg_initiator_id	__PR((SCSI *scgp));
extern	int	scg_isatapi		__PR((SCSI *scgp));
extern	int	scg_reset		__PR((SCSI *scgp, int what));
extern	void	*scg_getbuf		__PR((SCSI *scgp, long));
extern	void	scg_freebuf		__PR((SCSI *scgp));
extern	long	scg_bufsize		__PR((SCSI *scgp, long));
extern	void	scg_setnonstderrs	__PR((SCSI *scgp, const char **));
extern	BOOL	scg_yes			__PR((char *));
extern	int	scg_cmd			__PR((SCSI *scgp));
extern	void	scg_vhead		__PR((SCSI *scgp));
extern	int	scg_svhead		__PR((SCSI *scgp, char *buf, int maxcnt));
extern	int	scg_vtail		__PR((SCSI *scgp));
extern	int	scg_svtail		__PR((SCSI *scgp, int *retp, char *buf, int maxcnt));
extern	void	scg_vsetup		__PR((SCSI *scgp));
extern	int	scg_getresid		__PR((SCSI *scgp));
extern	int	scg_getdmacnt		__PR((SCSI *scgp));
extern	BOOL	scg_cmd_err		__PR((SCSI *scgp));
extern	void	scg_printerr		__PR((SCSI *scgp));
#ifdef	EOF	/* stdio.h has been included */
extern	void	scg_fprinterr		__PR((SCSI *scgp, FILE *f));
#endif
extern	int	scg_sprinterr		__PR((SCSI *scgp, char *buf, int maxcnt));
extern	int	scg__sprinterr		__PR((SCSI *scgp, char *buf, int maxcnt));
extern	void	scg_printcdb		__PR((SCSI *scgp));
extern	int	scg_sprintcdb		__PR((SCSI *scgp, char *buf, int maxcnt));
extern	void	scg_printwdata		__PR((SCSI *scgp));
extern	int	scg_sprintwdata		__PR((SCSI *scgp, char *buf, int maxcnt));
extern	void	scg_printrdata		__PR((SCSI *scgp));
extern	int	scg_sprintrdata		__PR((SCSI *scgp, char *buf, int maxcnt));
extern	void	scg_printresult		__PR((SCSI *scgp));
extern	int	scg_sprintresult 	__PR((SCSI *scgp, char *buf, int maxcnt));
extern	void	scg_printstatus		__PR((SCSI *scgp));
extern	int	scg_sprintstatus 	__PR((SCSI *scgp, char *buf, int maxcnt));
#ifdef	EOF	/* stdio.h has been included */
extern	void	scg_fprbytes		__PR((FILE *, char *, unsigned char *, int));
extern	void	scg_fprascii		__PR((FILE *, char *, unsigned char *, int));
#endif
extern	void	scg_prbytes		__PR((char *, unsigned char *, int));
extern	void	scg_prascii		__PR((char *, unsigned char *, int));
extern	int	scg_sprbytes		__PR((char *buf, int maxcnt, char *, unsigned char *, int));
extern	int	scg_sprascii		__PR((char *buf, int maxcnt, char *, unsigned char *, int));
#ifdef	EOF	/* stdio.h has been included */
extern	void	scg_fprsense		__PR((FILE *f, unsigned char *, int));
#endif
extern	void	scg_prsense		__PR((unsigned char *, int));
extern	int	scg_sprsense		__PR((char *buf, int maxcnt, unsigned char *, int));
extern	int	scg_cmd_status		__PR((SCSI *scgp));
extern	int	scg_sense_key		__PR((SCSI *scgp));
extern	int	scg_sense_code		__PR((SCSI *scgp));
extern	int	scg_sense_qual		__PR((SCSI *scgp));
#ifdef	_SCG_SCSIREG_H
#ifdef	EOF	/* stdio.h has been included */
extern	void	scg_fprintdev		__PR((FILE *, struct scsi_inquiry *));
#endif
extern	void	scg_printdev		__PR((struct scsi_inquiry *));
#endif
extern	int	scg_printf		__PR((SCSI *scgp, const char *form, ...));
extern	int	scg_errflush		__PR((SCSI *scgp));
#ifdef	EOF	/* stdio.h has been included */
extern	int	scg_errfflush		__PR((SCSI *scgp, FILE *f));
#endif

/*
 * From scsierrmsg.c:
 */
extern	const char	*scg_sensemsg	__PR((int, int, int,
						const char **, char *, int maxcnt));
#ifdef	_SCG_SCSISENSE_H
extern	int		scg__errmsg	__PR((SCSI *scgp, char *obuf, int maxcnt,
						struct scsi_sense *,
						struct scsi_status *,
						int));
#endif

/*
 * From scsiopen.c:
 */
#ifdef	EOF	/* stdio.h has been included */
extern	int	scg_help	__PR((FILE *f));
#endif
extern	SCSI	*scg_open	__PR((char *scsidev, char *errs, int slen, int odebug, int be_verbose));
extern	int	scg_close	__PR((SCSI * scgp));
extern	void	scg_settimeout	__PR((SCSI * scgp, int timeout));
extern	SCSI	*scg_smalloc	__PR((void));
extern	void	scg_sfree	__PR((SCSI *scgp));

/*
 * From scsiopts.c:
 */
extern	int	scg_opts	__PR((SCSI *scgp, const char *scgoptions));

/*
 * From scgsettarget.c:
 */
extern	int	scg_settarget		__PR((SCSI *scgp, int scsibus, int target, int lun));

/*
 * From scsi-remote.c:
 */
extern	scg_ops_t *scg_remote	__PR((void));

/*
 * From scsihelp.c:
 */
#ifdef	EOF	/* stdio.h has been included */
extern	void	__scg_help	__PR((FILE *f, char *name, char *tcomment,
					char *tind,
					char *tspec,
					char *texample,
					BOOL mayscan,
					BOOL bydev));
#endif

#ifdef	__cplusplus
}
#endif

#endif	/* _SCG_SCSITRANSP_H */
