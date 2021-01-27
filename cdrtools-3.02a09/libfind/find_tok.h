/* @(#)find_tok.h	1.6 10/04/19 Copyright 2004-2010 J. Schilling */
/*
 *	Copyright (c) 2004-2010 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifndef	_FIND_TOK_H
#define	_FIND_TOK_H

#define	OPEN	0	/* (				*/
#define	CLOSE	1	/* )				*/
#define	LNOT	2	/* !				*/
#define	AND	3	/* a				*/
#define	LOR	4	/* o				*/
#define	ATIME	5	/* -atime			*/
#define	CTIME	6	/* -ctime			*/
#define	DEPTH	7	/* -depth			*/
#define	EXEC	8	/* -exec			*/
#define	FOLLOW	9	/* -follow	POSIX Extension	*/
#define	FSTYPE	10	/* -fstype	POSIX Extension	*/
#define	GROUP	11	/* -group			*/
#define	INUM	12	/* -inum	POSIX Extension	*/
#define	LINKS	13	/* -links			*/
#define	LOCL	14	/* -local	POSIX Extension	*/
#define	LS	15	/* -ls		POSIX Extension	*/
#define	MODE	16	/* -mode	POSIX Extension	*/
#define	MOUNT	17	/* -mount	POSIX Extension	*/
#define	MTIME	18	/* -mtime			*/
#define	NAME	19	/* -name			*/
#define	NEWER	20	/* -newer			*/
#define	NOGRP	21	/* -nogroup			*/
#define	NOUSER	22	/* -nouser			*/
#define	OK_EXEC	23	/* -ok				*/
#define	PERM	24	/* -perm			*/
#define	PRINT	25	/* -print			*/
#define	PRINTNNL 26	/* -printnnl	POSIX Extension	*/
#define	PRUNE	27	/* -prune			*/
#define	SIZE	28	/* -size			*/
#define	TIME	29	/* -time	POSIX Extension	*/
#define	TYPE	30	/* -type			*/
#define	USER	31	/* -user 			*/
#define	XDEV	32	/* -xdev			*/
#define	PATH	33	/* -path	POSIX Extension	*/
#define	LNAME	34	/* -lname	POSIX Extension	*/
#define	PAT	35	/* -pat		POSIX Extension	*/
#define	PPAT	36	/* -ppat	POSIX Extension	*/
#define	LPAT	37	/* -lpat	POSIX Extension	*/
#define	PACL	38	/* -ack		POSIX Extension	*/
#define	XATTR	39	/* -xattr	POSIX Extension	*/
#define	LINKEDTO 40	/* -linkedto	POSIX Extension	*/
#define	NEWERAA	41	/* -neweraa	POSIX Extension	*/
#define	NEWERAC	42	/* -newerac	POSIX Extension	*/
#define	NEWERAM	43	/* -neweram	POSIX Extension	*/
#define	NEWERCA	44	/* -newerca	POSIX Extension	*/
#define	NEWERCC	45	/* -newercc	POSIX Extension	*/
#define	NEWERCM	46	/* -newercm	POSIX Extension	*/
#define	NEWERMA	47	/* -newerma	POSIX Extension	*/
#define	NEWERMC	48	/* -newermc	POSIX Extension	*/
#define	NEWERMM	49	/* -newermm	POSIX Extension	*/
#define	SPARSE	50	/* -sparse	POSIX Extension	*/
#define	LTRUE	51	/* -true	POSIX Extension	*/
#define	LFALSE	52	/* -false	POSIX Extension	*/
#define	MAXDEPTH 53	/* -maxdepth	POSIX Extension	*/
#define	MINDEPTH 54	/* -mindepth	POSIX Extension	*/
#define	HELP	55	/* -help	POSIX Extension	*/
#define	CHOWN	56	/* -chown	POSIX Extension	*/
#define	CHGRP	57	/* -chgrp	POSIX Extension	*/
#define	CHMOD	58	/* -chmod	POSIX Extension	*/
#define	DOSTAT	59	/* -dostat	POSIX Extension	*/
#define	INAME	60	/* -iname	POSIX Extension	*/
#define	ILNAME	61	/* -ilname	POSIX Extension	*/
#define	IPATH	62	/* -ipath	POSIX Extension	*/
#define	IPAT	63	/* -ipat	POSIX Extension	*/
#define	IPPAT	64	/* -ippat	POSIX Extension	*/
#define	ILPAT	65	/* -ilpat	POSIX Extension	*/
#define	AMIN	66	/* -amin	POSIX Extension	*/
#define	CMIN	67	/* -cmin	POSIX Extension	*/
#define	MMIN	68	/* -mmin	POSIX Extension	*/
#define	PRINT0	69	/* -print0	POSIX Extension	*/
#define	FPRINT	70	/* -fprint	POSIX Extension	*/
#define	FPRINTNNL 71	/* -fprintnnl	POSIX Extension	*/
#define	FPRINT0	72	/* -fprint0	POSIX Extension	*/
#define	FLS	73	/* -fls		POSIX Extension	*/
#define	EMPTY	74	/* -empty	POSIX Extension	*/
#define	READABLE 75	/* -readable	POSIX Extension	*/
#define	WRITABLE 76	/* -writable	POSIX Extension	*/
#define	EXECUTABLE 77	/* -executable	POSIX Extension	*/
#define	EXECDIR	78	/* -execdir	POSIX Extension	*/
#define	OK_EXECDIR 79	/* -okdir	POSIX Extension	*/
#define	ENDPRIM	90	/* End of primary list		*/
#define	EXECPLUS 81	/* -exec			*/
#define	EXECDIRPLUS 82	/* -execdir			*/
#define	ENDTLIST 83	/* End of token list		*/

#define	tokennames	_find_tokennames

#ifdef	TOKEN_NAMES
LOCAL	char	*tokennames[] = {
	"(",		/* 0 OPEN			*/
	")",		/* 1 CLOSE			*/
	"!",		/* 2 LNOT			*/
	"a",		/* 3 AND			*/
	"o",		/* 4 LOR			*/
	"atime",	/* 5 ATIME			*/
	"ctime",	/* 6 CTIME			*/
	"depth",	/* 7 DEPTH			*/
	"exec",		/* 8 EXEC			*/
	"follow",	/* 9 FOLLOW	POSIX Extension	*/
	"fstype",	/* 10 FSTYPE	POSIX Extension	*/
	"group",	/* 11 GROUP			*/
	"inum",		/* 12 INUM	POSIX Extension	*/
	"links",	/* 13 LINKS			*/
	"local",	/* 14 LOCL	POSIX Extension	*/
	"ls",		/* 15 LS	POSIX Extension	*/
	"mode",		/* 16 MODE	POSIX Extension	*/
	"mount",	/* 17 MOUNT	POSIX Extension	*/
	"mtime",	/* 18 MTIME			*/
	"name",		/* 19 NAME			*/
	"newer",	/* 20 NEWER			*/
	"nogroup",	/* 21 NOGRP			*/
	"nouser",	/* 22 NOUSER			*/
	"ok",		/* 23 OK_EXEC			*/
	"perm",		/* 24 PERM			*/
	"print",	/* 25 PRINT			*/
	"printnnl",	/* 26 PRINTNNL	POSIX Extension	*/
	"prune",	/* 27 PRUNE			*/
	"size",		/* 28 SIZE			*/
	"time",		/* 29 TIME	POSIX Extension	*/
	"type",		/* 30 TYPE			*/
	"user",		/* 31 USER			*/
	"xdev",		/* 32 XDEV			*/
	"path",		/* 33 PATH	POSIX Extension	*/
	"lname",	/* 34 LNAME	POSIX Extension	*/
	"pat",		/* 35 PAT	POSIX Extension	*/
	"ppat",		/* 36 PPAT	POSIX Extension	*/
	"lpat",		/* 37 LPAT	POSIX Extension	*/
	"acl",		/* 38 PACL	POSIX Extension	*/
	"xattr",	/* 39 XATTR	POSIX Extension	*/
	"linkedto",	/* 40 LINKEDTO	POSIX Extension	*/
	"neweraa",	/* 41 NEWERAA	POSIX Extension	*/
	"newerac",	/* 42 NEWERAC	POSIX Extension	*/
	"neweram",	/* 43 NEWERAM	POSIX Extension	*/
	"newerca",	/* 44 NEWERCA	POSIX Extension	*/
	"newercc",	/* 45 NEWERCC	POSIX Extension	*/
	"newercm",	/* 46 NEWERCM	POSIX Extension	*/
	"newerma",	/* 47 NEWERMA	POSIX Extension	*/
	"newermc",	/* 48 NEWERMC	POSIX Extension	*/
	"newermm",	/* 49 NEWERMM	POSIX Extension	*/
	"sparse",	/* 50 SPARSE	POSIX Extension	*/
	"true",		/* 51 LTRUE	POSIX Extension	*/
	"false",	/* 52 LFALSE	POSIX Extension	*/
	"maxdepth",	/* 53 MAXDEPTH	POSIX Extension	*/
	"mindepth",	/* 54 MINDEPTH	POSIX Extension	*/
	"help",		/* 55 HELP	POSIX Extension	*/
	"chown",	/* 56 CHOWN	POSIX Extension	*/
	"chgrp",	/* 57 CHGRP	POSIX Extension	*/
	"chmod",	/* 58 CHMOD	POSIX Extension	*/
	"dostat",	/* 59 DOSTAT	POSIX Extension	*/
	"iname",	/* 60 INAME	POSIX Extension	*/
	"ilname",	/* 61 ILNAME	POSIX Extension	*/
	"ipath",	/* 62 IPATH	POSIX Extension	*/
	"ipat",		/* 63 IPAT	POSIX Extension	*/
	"ippat",	/* 64 IPPAT	POSIX Extension	*/
	"ilpat",	/* 65 ILPAT	POSIX Extension	*/
	"amin",		/* 66 AMIN	POSIX Extension	*/
	"cmin",		/* 67 CMIN	POSIX Extension	*/
	"mmin",		/* 68 MMIN	POSIX Extension	*/
	"print0",	/* 69 PRINT0	POSIX Extension	*/
	"fprint",	/* 70 FPRINT	POSIX Extension	*/
	"fprintnnl",	/* 71 FPRINTNNL	POSIX Extension	*/
	"fprint0",	/* 72 FPRINT0	POSIX Extension	*/
	"fls",		/* 73 FLS	POSIX Extension	*/
	"empty",	/* 74 EMPTY	POSIX Extension	*/
	"readable",	/* 75 READABLE	POSIX Extension	*/
	"writable",	/* 76 WRITABLE	POSIX Extension	*/
	"executable",	/* 77 EXECUTABLE POSIX Extension */
	"execdir",	/* 78 EXECDIR	POSIX Extension */
	"okdir",	/* 79 OK_EXECDIR POSIX Extension */
	0,		/* 80 End of primary list	*/
	"exec",		/* 81 Map EXECPLUS -> "exec"	*/
	"execdir",	/* 82 Map EXECDIRPLUS -> "execdir" */
	0		/* 83 End of list		*/
};
#define	NTOK	((sizeof (tokennames) / sizeof (tokennames[0])) - 1)

#else	/* TOKEN_NAMES */

#define	NTOK	ENDTLIST

#endif	/* TOKEN_NAMES */

#endif	/* _FIND_TOK_H */
