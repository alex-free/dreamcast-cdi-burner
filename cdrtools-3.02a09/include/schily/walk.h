/* @(#)walk.h	1.31 16/03/10 Copyright 2004-2016 J. Schilling */
/*
 *	Definitions for directory tree walking
 *
 *	Copyright (c) 2004-2016 J. Schilling
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

#ifndef	_SCHILY_WALK_H
#define	_SCHILY_WALK_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

#ifndef _SCHILY_STAT_H
#include <schily/stat.h>
#endif

#ifndef _SCHILY_STDIO_H
#include <schily/stdio.h>
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Flags to control treewalk() via 'walkflags'.
 *
 *	WALK_CHDIR	is not implemented, treewalk() always does chdir()
 *
 *	WALK_PHYS	Clearing WALK_PHYS has highest precedence and equals
 *			'find -follow'. If WALK_PHYS is clear, always use stat.
 *			By default, WALK_PHYS should be set.
 *
 *	WALK_ARGFOLLOW	If WALK_ARGFOLLOW is set, symlinks that are used as
 *			the first argument for trewalk() are followed even
 *			if WALK_PHYS is set. Setting WALK_ARGFOLLOW equals
 *			'find -H'.
 *
 *	WALK_ALLFOLLOW	If WALK_ALLFOLLOW is set, all symlinks are followed
 *			even if WALK_PHYS is set. Setting WALK_ALLFOLLOW
 *			equals 'find -L'.
 */
#define	WALK_PHYS	1	/* Use lstat() instead of stat()	*/
#define	WALK_MOUNT	2	/* Do not cross mount points		*/
#define	WALK_DEPTH	4	/* Call content before calling the dir	*/
#define	WALK_CHDIR	8	/* Use chdir() to each directory	*/
#define	WALK_ARGFOLLOW	0x10	/* Use stat() for top level args only	*/
#define	WALK_ALLFOLLOW	0x20	/* Use stat() for all files		*/
#define	WALK_NOSTAT	0x40	/* Avoid to call stat() if st_nlink =>2 */
#define	WALK_NOEXIT	0x100	/* Do not exit() in case of hard errors	*/
#define	WALK_NOMSG	0x200	/* Do not write messages to stderr	*/
#define	WALK_LS_ATIME	0x1000	/* -ls lists atime instead of mtime	*/
#define	WALK_LS_CTIME	0x2000	/* -ls lists ctime instead of mtime	*/
#define	WALK_STRIPLDOT	0x4000	/* Strip leading "./" from path		*/

/*
 * The 'type' argument to walkfun.
 */
#define	WALK_F		1	/* File	*/
#define	WALK_SL		2	/* Symbolic Link */
#define	WALK_D		3	/* Directory */
#define	WALK_DP		4	/* Directory previously visited */
#define	WALK_DNR	5	/* Directory with no read permission */
#define	WALK_NS		6	/* Unknown file type stat failed */
#define	WALK_SLN	7	/* Symbolic Link points to nonexistent file */

#ifndef	__sqfun_t_defined
typedef	int	(*sqfun_t)	__PR((void *arg));
#define	__sqfun_t_defined
#endif

struct WALK {
	int	flags;		/* Flags for communication with (*walkfun)() */
	int	base;		/* Filename offset in path for  (*walkfun)() */
	int	level;		/* The nesting level set up for (*walkfun)() */
	int	walkflags;	/* treewalk() control flags		    */
	void	*twprivate;	/* treewalk() private do not touch	    */
	FILE	*std[3];	/* To redirect stdin/stdout/err in treewalk  */
	char	**env;		/* To allow different env in treewalk/exec   */
	sqfun_t	quitfun;	/* Function to query for shell signal quit   */
	void	*qfarg;		/* Generic arg for shell builtin quit fun    */
	int	maxdepth;	/* (*walkfun)() private, unused by treewalk  */
	int	mindepth;	/* (*walkfun)() private, unused by treewalk  */
	char	*lname;		/* (*walkfun)() private, unused by treewalk  */
	void	*tree;		/* (*walkfun)() private, unused by treewalk  */
	void	*patstate;	/* (*walkfun)() private, unused by treewalk  */
	int	err;		/* (*walkfun)() private, unused by treewalk  */
	int	pflags;		/* (*walkfun)() private, unused by treewalk  */
	int	auxi;		/* (*walkfun)() private, unused by treewalk  */
	void	*auxp;		/* (*walkfun)() private, unused by treewalk  */
};

/*
 * Flags in struct WALK used to communicate with (*walkfun)()
 */
#define	WALK_WF_PRUNE	1	/* (*walkfun)() -> walk(): abort waking tree */
#define	WALK_WF_QUIT	2	/* (*walkfun)() -> walk(): quit completely   */
#define	WALK_WF_NOCHDIR	4	/* walk() -> (*walkfun)(): WALK_DNR w chdir() */
#define	WALK_WF_NOCWD	8	/* walk() -> caller: cannot get working dir  */
#define	WALK_WF_NOHOME	16	/* walk() -> caller: cannot chdir("..")	    */
#define	WALK_WF_NOTDIR	32	/* walk() -> walk(): file is not a directory */

typedef	int	(*walkfun)	__PR((char *_nm, struct stat *_fs, int _type,
						struct WALK *_state));

extern	int	treewalk	__PR((char *_nm, walkfun _fn,
						struct WALK *_state));
extern	void	walkinitstate	__PR((struct WALK *_state));
extern	void	*walkopen	__PR((struct WALK *_state));
extern	int	walkgethome	__PR((struct WALK *_state));
extern	int	walkhome	__PR((struct WALK *_state));
extern	int	walkclose	__PR((struct WALK *_state));

#ifdef	__cplusplus
}
#endif

#endif	/* _SCHILY_WALK_H */
