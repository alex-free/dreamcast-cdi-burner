/* @(#)fstream.h	1.17 14/04/14 Copyright 1985-2014 J. Schilling */
/*
 *	Definitions for the file stream package
 *
 *	Copyright (c) 1985-2014 J. Schilling
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

#ifndef _SCHILY_FSTREAM_H
#define	_SCHILY_FSTREAM_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

#define	STR_SBUF_SIZE	127	/* Size of "static" stream buffer	*/

#ifdef	WSTRINGS
typedef	short	CHAR;
#else
typedef	char	CHAR;
#endif

/*
 * The stream filter structure.
 *
 * If the filter function (*fstr_func)() is a NULL pointer, then fstr_file is
 * a FILE * object and input is read from that FILE * by calling the read
 * function (*fstr_rfunc)().
 *
 * If the filter function (*fstr_func)() is not a NULL pointer, then fstr_file
 * is a stream * object and input is read from that stream object by calling
 * the filter function (*fstr_func)().
 *
 * The two callback functions have the following tasks:
 *
 * (*fstr_func)(ostream, istream)	filters from istream to ostream
 * (*fstr_rfunc)(istream)		reads a new input from is->fstr_file
 *
 * As long as fstr_bp points to a non-null character, this character is
 * returned. If the local buffer is empty, the functions are checked:
 *
 * If (*fstr_func)() is not a NULL pointer, then it is a filter function
 * that is called whenever fstr_buf is empty.
 *
 * If (*fstr_func)() is a NULL pointer and (*fstr_rfunc)() is not a NULL pointer
 * then (*fstr_rfunc)() is called to fill the buffer and to return the first
 * charcter from the buffer.
 */
typedef struct fstream	fstream;

struct fstream {
	FILE	*fstr_file;	/* The input FILE * or input fstream *  */
	int	fstr_flags;	/* Flags available for the caller	*/
				/* End of the public part of the structure */
	fstream	*fstr_pushed;	/* Chain of pushed strcutures		*/
	CHAR	*fstr_bp;	/* The current pointer to coocked input */
	CHAR	*fstr_buf;	/* The current buffer for coocked input */
	int 	(*fstr_func) __PR((struct fstream *__out, FILE *__in));
	int	(*fstr_rfunc) __PR((struct fstream *__in));
	CHAR	fstr_sbuf[STR_SBUF_SIZE + 1];
};

typedef	int	(*fstr_fun) __PR((struct fstream *, FILE *));
typedef	int	(*fstr_efun) __PR((char *));
typedef	int	(*fstr_rfun) __PR((struct fstream *));

extern	fstream	*mkfstream	__PR((FILE *f, fstr_fun, fstr_rfun, fstr_efun));
extern	fstream *fspush		__PR((fstream *fsp, fstr_efun efun));
extern	fstream *fspop		__PR((fstream *fsp));
extern	fstream *fspushed	__PR((fstream *fsp));
extern	void	fsclose		__PR((fstream *fsp));
extern	FILE	*fssetfile	__PR((fstream *fsp, FILE *f));
extern	int	fsgetc		__PR((fstream *fsp));
extern	void	fspushstr	__PR((fstream *fsp, char *ss));
extern	void	fspushcha	__PR((fstream *fsp, int c));

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_FSTREAM_H */
