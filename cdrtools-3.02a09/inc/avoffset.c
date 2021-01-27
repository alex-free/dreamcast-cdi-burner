/* @(#)avoffset.c	1.35 17/08/01 Copyright 1987, 1995-2017 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)avoffset.c	1.35 17/08/01 Copyright 1987, 1995-2017 J. Schilling";
#endif
/*
 * This program is a tool to generate the file "avoffset.h".
 * It is used by functions that trace the stack to get to the top of the stack.
 *
 * It generates two defines:
 *	AV_OFFSET	- offset of argv relative to the main() frame pointer
 *	FP_INDIR	- number of stack frames above main()
 *			  before encountering a NULL pointer.
 *
 *	Copyright (c) 1987, 1995-2017 J. Schilling
 */
/*@@C@@*/

#include <schily/stdio.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include <schily/stdlib.h>
#include <schily/signal.h>

#ifdef	HAVE_SCANSTACK
#	include <schily/stkframe.h>
#endif

LOCAL	RETSIGTYPE	handler 	__PR((int signo));
EXPORT	int		main		__PR((int ac, char **av));
LOCAL	int		stack_direction	__PR((long *lp));

LOCAL	RETSIGTYPE
handler(signo)
	int	signo;
{
	fprintf(stderr, "Warning: Cannot scan stack on this environment.\n");

	printf("\n#endif	/* __AVOFFSET_H */\n");
	fflush(stdout);
	exit(0);
}


int
main(ac, av)
	int	ac;
	char	**av;
{
	int		stdir;
#ifdef	HAVE_SCANSTACK
	register struct frame *fp;
	register int	i = 0;
	register int	o = 0;

	/*
	 * As the SCO OpenServer C-Compiler has a bug that may cause
	 * the first function call to getfp() been done before the
	 * new stack frame is created, we call getfp() twice.
	 */
	(void) getfp();
	fp = (struct frame *)getfp();
#endif

#ifdef	SIGBUS
	signal(SIGBUS, handler);
#endif
	signal(SIGSEGV, handler);
#ifdef	SIGILL
	signal(SIGILL, handler);	/* For gcc -m64/sparc on FreeBSD */
#endif

	printf("/*\n");
	printf(" * This file has been generated automatically\n");
	printf(" * by %s\n", sccsid);
	printf(" * do not edit by hand.\n");
	printf(" *\n");
	printf(" * This file includes definitions for AV_OFFSET and FP_INDIR.\n");
	printf(" * FP_INDIR is the number of fp chain elements above 'main'.\n");
	printf(" * AV_OFFSET is the offset of &av[0] relative to the frame pointer in 'main'.\n");
	printf(" *\n");
	printf(" * If getav0() does not work on a specific architecture\n");
	printf(" * the program which generated this include file may dump core.\n");
	printf(" * In this case, the generated include file does not include\n");
	printf(" * definitions for AV_OFFSET and FP_INDIR but ends after this comment.\n");
	printf(" * If AV_OFFSET or FP_INDIR are missing in this file, all programs\n");
	printf(" * which use the definitions are automatically disabled.\n");
	printf(" */\n");
	printf("#ifndef	__AVOFFSET_H\n");
	printf("#define	__AVOFFSET_H\n\n");

	stdir = stack_direction(0);
	printf("#define	STACK_DIRECTION	%d\n", stdir);
	fflush(stdout);

#ifdef	HAVE_SCANSTACK
	/*
	 * Note: Scanning the stack to look for argc/argv
	 *	 works only in the main thread.
	 *
	 * llvm-gcc-4.2 has a bug and creates an endless loop if we call:
	 *	while (fp->fr_savfp) {
	 * We now try to limit this to 1000 loops in hope that the bug
	 * does not affect the new code extended as well.
	 */
	while (i <= 1000 && fp->fr_savfp) {
		/*
		 * Workaround for the still buggy clang...
		 * clang version 4.0.0 on ARM64 FreeBSD has become worse.
		 * If we do not have the strange write() call, this loop
		 * becomes an endless loop and the last line in the loop
		 * is never touched.
		 */
		write(-1, "", 0);
		if (fp->fr_savpc == 0)
			break;

		fp = (struct frame *)fp->fr_savfp;

		i++;
	}
	/*
	 * Do not add any printf()'s before this line to allow avoffset
	 * to abort without printing more than the comment above.
	 */
	fp = (struct frame *)getfp();
	o = ((char *)av) - ((char *)fp);
	if ((o % sizeof (char *)) != 0) {
		fprintf(stderr, "AV_OFFSET value (%d) not a multiple of pointer size.\n", o);
		fprintf(stderr, "Disabling scanning the stack.\n");

		printf("\n#endif	/* __AVOFFSET_H */\n");
		exit(0);
	}
	if (o < -1000 || o > 1000) {
		fprintf(stderr, "AV_OFFSET value (%d) does not look reasonable.\n", o);
		fprintf(stderr, "Disabling scanning the stack.\n");

		printf("\n#endif	/* __AVOFFSET_H */\n");
		exit(0);
	}
	if (i > 1000) {
		fprintf(stderr, "FP_INDIR value (%d) does not look reasonable.\n", i);
		fprintf(stderr, "Disabling scanning the stack.\n");

		printf("\n#endif	/* __AVOFFSET_H */\n");
		exit(0);
	}
	printf("#define	AV_OFFSET	%d\n", o);
	printf("#define	FP_INDIR	%d\n", i);
#endif
	printf("\n#endif	/* __AVOFFSET_H */\n");
	fflush(stdout);
	exit(0);
	return (0);	/* keep lint happy */
}

#if	__clang__ || (__GNUC__ > 3 || __GNUC__ == 3 && __GNUC_MINOR__ >= 2)
#define	__NO_INL__	__attribute__((noinline))
#else
#define	__NO_INL__
#endif

LOCAL int __NO_INL__
stack_direction(lp)
	long	*lp;
{
	auto long	*dummy[4];
	int		i;

	for (i = 0; i < 4; i++)
		dummy[i] = lp;

	if (lp == 0) {
		return (stack_direction((long *)dummy));
	} else {
		if ((long *)dummy == lp)
			return (0);
		return (((long *)dummy > lp) ? 1 : -1);
	}
}

#ifdef	HAVE_SCANSTACK
#define	IS_AVOFFSET
#include "getfp.c"
#endif
