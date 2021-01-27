/* @(#)shedit.h	1.14 17/01/18 Copyright 2006-2017 J. Schilling */
/*
 *	Definitions for libshedit, the history editor for the shell.
 *
 *	Copyright (c) 2006-2017 J. Schilling
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

#ifndef	_SCHILY_SHEDIT_H
#define	_SCHILY_SHEDIT_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif
#ifndef	_SCHILY_TYPES_H
#include <schily/types.h>
#endif
#ifndef	_SCHILY_SCHILY_H
#include <schily/schily.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Exported functions:
 *
 * shedit_egetc():	read one character from the edited input
 * shedit_getdelim():	get input delimiter
 * shedit_treset():	reset terminal modes to non-edit
 * shedit_bhist():	the builtin history command
 * shedit_bshist():	the builtin savehistory command
 * shedit_append_line(): to append a line to the history
 * shedit_remap():	the builtin to reread termcap/mapping defaults
 * shedit_list_map():	the builtin to list mappings
 * shedit_del_map():	the builtin to delete mappings
 * shedit_add_map():	the builtin to add mappings
 * shedit_getenv():	set up pointer to local getenv() from the shell
 * shedit_putenv():	set up pointer to local putenv() from the shell
 * shedit_igneof():	set up pointer to local ignoreeof() from the shell
 */
extern	int	shedit_egetc	__PR((void));
extern	int	shedit_getdelim	__PR((void));
extern	void	shedit_treset	__PR((void));
extern	void	shedit_bhist	__PR((int **intrcpp));
extern	void	shedit_bshist	__PR((int **intrpp));
extern	void	shedit_append_line __PR((char *linep, unsigned int len,
						unsigned int pos));
extern	void	shedit_chghistory __PR((char *__val));
extern	void	shedit_histrange __PR((unsigned *firstp,
					unsigned *lastp, unsigned *nextp));
extern	void	shedit_remap	__PR((void));
extern	void	shedit_list_map	__PR((int *f));
extern	int	shedit_del_map	__PR((char *from));
extern	int	shedit_add_map	__PR((char *from, char *to, char *comment));
extern	void	shedit_getenv	__PR((char *(*genv)(char *name)));
extern	void	shedit_putenv	__PR((void (*penv) (char *name)));
extern	void	shedit_igneof	__PR((BOOL (*ieof) (void)));
extern	void	shedit_setprompts __PR((int promptidx, int nprompts,
							char *newprompts[]));

/*
 * Keep #defines in sync with bsh/bsh.h
 */
#define	HI_NOINTR	0	/* History traversal noninterruptable	*/
#define	HI_INTR		1	/* History traversal is interruptable	*/
#define	HI_NONUM	2	/* Do not print numbers			*/
#define	HI_TAB		4	/* Print TABs				*/
#define	HI_REVERSE	8	/* Print in reverse order		*/
#define	HI_PRETTYP	16	/* Pretty Type non-printable chars	*/
#define	HI_ANSI_NL	32	/* Convert ASCII newlines to ANSI nl	*/

extern	int	shedit_history		__PR((int *f, int **intrcpp, int flg,
					int _first, int _last, char *_subst));
extern	int	shedit_search_history	__PR((int **intrcpp, int flg,
					int _first, char *_pat));
extern	int	shedit_remove_history	__PR((int **intrcpp, int flg,
					int _first, char *_pat));
extern	int	shedit_read_history	__PR((int *f, int **intrcpp, int flg));
#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_SHEDIT_H */
