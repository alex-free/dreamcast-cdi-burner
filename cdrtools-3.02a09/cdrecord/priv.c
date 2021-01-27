/* @(#)priv.c	1.4 13/10/12 Copyright 2006-2013 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)priv.c	1.4 13/10/12 Copyright 2006-2013 J. Schilling";
#endif
/*
 *	Cdrecord support functions to support fine grained privileges.
 *
 *	Copyright (c) 2006-2013 J. Schilling
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

#include <schily/mconfig.h>
#include <schily/standard.h>
#include <schily/stdlib.h>
#include <schily/string.h>
#include <schily/schily.h>
#include <schily/priv.h>

#ifdef	HAVE_LINUX_CAPS
#define	NCAPS		(sizeof (caplist) / sizeof (caplist[0]))
#endif

#ifdef	CDDA2WAV
EXPORT	void	priv_init	__PR((void));
EXPORT	void	priv_on		__PR((void));
EXPORT	void	priv_off	__PR((void));
#endif
#if	defined(CDRECORD) || defined(READCD)
EXPORT	void	priv_drop	__PR((void));
EXPORT	BOOL	priv_from_priv	__PR((void));
#endif
EXPORT	BOOL	priv_eff_priv	__PR((int pname));
#ifdef	HAVE_SOLARIS_PPRIV
LOCAL	BOOL	priv_eff	__PR((const char *pname));
#endif
#ifdef	HAVE_LINUX_CAPS
LOCAL	BOOL	priv_eff	__PR((int pname));
#endif

#ifdef	HAVE_SOLARIS_PPRIV
EXPORT	void	do_pfexec	__PR((int ac, char *av[], ...));
#endif

/*
 * Solaris uses the advanced privileges(5) mechanism for fine grained
 * prilileges.
 *
 * The following Solaris privileges are needed by cdrecord:
 *
 *	PRIV_FILE_DAC_READ	only to open /dev/ nodes for USCSICMD
 *	PRIV_FILE_DAC_WRITE	in addition to open /dev/scg* for SCGIO_CMD
 *	PRIV_SYS_DEVICES	during whole runtime: for USCSICMD
 *	PRIV_PROC_LOCK_MEMORY	only to call mlockall()
 *	PRIV_PROC_PRIOCNTL	only to call priocntl()
 *	PRIV_NET_PRIVADDR	only to create privileged socket (remote SCSI)
 *
 *
 * The POSIX.1e draft has been withdrawn in 1997.
 * Linux started to implement this outdated concept in 1997.
 *
 * The following Linux capabilities are needed by cdrecord:
 *
 *	CAP_DAC_OVERRIDE	only to open /dev/ nodes
 *	CAP_NET_BIND_SERVICE	only to create privileged socket (remote SCSI)
 *	CAP_IPC_LOCK		only to call mlockall()
 *	CAP_SYS_RAWIO		during whole runtime: for SCSI commands
 *	CAP_SYS_ADMIN		during whole runtime: for SCSI commands
 *	CAP_SYS_NICE		only to call sched_*() and setpriority()
 *	CAP_SYS_RESOURCE	only to support mlockall()
 *
 * Use:
 * setcap cap_sys_resource,cap_dac_override,cap_sys_admin,cap_sys_nice,cap_net_bind_service,cap_ipc_lock,cap_sys_rawio+ep cdrecord
 * to set the capabilities for the executable file.
 */

#ifdef	CDDA2WAV
/*
 * Initial removal of privileges:
 * -	remove all inheritable additional privileges
 * -	Remove all effective privileges that are not needed all the time
 */
EXPORT void
priv_init()
{
#ifdef	HAVE_PRIV_SET
	/*
	 * Give up privs we do not need anymore.
	 * We no longer need:
	 *	file_dac_read,sys_devices,proc_priocntl,net_privaddr
	 */
	priv_set(PRIV_OFF, PRIV_EFFECTIVE,
		PRIV_FILE_DAC_READ, PRIV_PROC_PRIOCNTL,
		PRIV_NET_PRIVADDR, NULL);
	priv_set(PRIV_OFF, PRIV_INHERITABLE,
		PRIV_FILE_DAC_READ, PRIV_PROC_PRIOCNTL,
		PRIV_NET_PRIVADDR, PRIV_SYS_DEVICES, NULL);
#else
#ifdef	HAVE_LINUX_CAPS
	/*
	 * Give up privs we do not need for the moment.
	 * We no longer need:
	 *	cap_dac_override,cap_net_bind_service,cap_sys_nice
	 */
	cap_t		cset;
	cap_value_t	caplist[] = {
					CAP_DAC_OVERRIDE,
					CAP_NET_BIND_SERVICE,
					CAP_SYS_NICE,
					CAP_SYS_RAWIO,		/* Keep as CAP_EFFECTIVE */
					CAP_SYS_ADMIN		/* Keep as CAP_EFFECTIVE */
				};

	cset = cap_get_proc();

	cap_set_flag(cset, CAP_EFFECTIVE, NCAPS-2, caplist, CAP_CLEAR);
	cap_set_flag(cset, CAP_INHERITABLE, NCAPS, caplist, CAP_CLEAR);
	if (cap_set_proc(cset) < 0)
		errmsg("Cannot set initial process capabilities.\n");
#endif	/* HAVE_LINUX_CAPS */
#endif	/* HAVE_PRIV_SET */
}

/*
 * Get back those effective privileges that are needed for parts of the time
 */
EXPORT void
priv_on()
{
#ifdef	HAVE_PRIV_SET
	/*
	 * Get back privs we may need now.
	 * We need:
	 *	file_dac_read,sys_devices,proc_priocntl,net_privaddr
	 */
	priv_set(PRIV_ON, PRIV_EFFECTIVE,
		PRIV_FILE_DAC_READ, PRIV_PROC_PRIOCNTL,
		PRIV_NET_PRIVADDR, NULL);
#else
#ifdef	HAVE_LINUX_CAPS
	/*
	 * Get back privs we may need now.
	 * We need:
	 *	cap_dac_override,cap_net_bind_service,cap_sys_nice
	 */
	cap_t		cset;
	cap_value_t	caplist[] = {
					CAP_DAC_OVERRIDE,
					CAP_NET_BIND_SERVICE,
					CAP_SYS_NICE
				};

	cset = cap_get_proc();

	cap_set_flag(cset, CAP_EFFECTIVE, NCAPS, caplist, CAP_SET);
	if (cap_set_proc(cset) < 0)
		errmsg("Cannot regain process capabilities.\n");
#endif	/* HAVE_LINUX_CAPS */
#endif	/* HAVE_PRIV_SET */
}

/*
 * Get rid of those effective privileges that are needed for parts of the time
 */
EXPORT void
priv_off()
{
#ifdef	HAVE_PRIV_SET
	/*
	 * Give up privs we do not need at the moment.
	 * We no longer need:
	 *	file_dac_read,sys_devices,proc_priocntl,net_privaddr
	 */
	priv_set(PRIV_OFF, PRIV_EFFECTIVE,
		PRIV_FILE_DAC_READ, PRIV_PROC_PRIOCNTL,
		PRIV_NET_PRIVADDR, NULL);
#else
#ifdef	HAVE_LINUX_CAPS
	/*
	 * Give up privs we do not need anymore.
	 * We no longer need:
	 *	cap_dac_override,cap_net_bind_service,cap_sys_nice
	 */
	cap_t		cset;
	cap_value_t	caplist[] = {
					CAP_DAC_OVERRIDE,
					CAP_NET_BIND_SERVICE,
					CAP_SYS_NICE
				};

	cset = cap_get_proc();

	cap_set_flag(cset, CAP_EFFECTIVE, NCAPS, caplist, CAP_CLEAR);
	if (cap_set_proc(cset) < 0)
		errmsg("Cannot deactivate process capabilities.\n");
#endif	/* HAVE_LINUX_CAPS */
#endif	/* HAVE_PRIV_SET */
}
#endif	/* CDDA2WAV */

#if	defined(CDRECORD) || defined(READCD)
/*
 * Drop all privileges that are no longer needed after startup
 */
EXPORT void
priv_drop()
{
#ifdef	HAVE_PRIV_SET
#ifdef	PRIVILEGES_DEBUG	/* PRIV_DEBUG is defined in <sys/priv.h> */
	error("file_dac_read: %d\n", priv_ineffect(PRIV_FILE_DAC_READ));
#endif
	/*
	 * Give up privs we do not need anymore.
	 * We no longer need:
	 *	file_dac_read,proc_lock_memory,proc_priocntl,net_privaddr
	 * We still need:
	 *	sys_devices
	 */
	priv_set(PRIV_OFF, PRIV_EFFECTIVE,
		PRIV_FILE_DAC_READ, PRIV_PROC_LOCK_MEMORY,
		PRIV_PROC_PRIOCNTL, PRIV_NET_PRIVADDR, NULL);
	priv_set(PRIV_OFF, PRIV_PERMITTED,
		PRIV_FILE_DAC_READ, PRIV_PROC_LOCK_MEMORY,
		PRIV_PROC_PRIOCNTL, PRIV_NET_PRIVADDR, NULL);
	priv_set(PRIV_OFF, PRIV_INHERITABLE,
		PRIV_FILE_DAC_READ, PRIV_PROC_LOCK_MEMORY,
		PRIV_PROC_PRIOCNTL, PRIV_NET_PRIVADDR, PRIV_SYS_DEVICES, NULL);
#else
#ifdef	HAVE_LINUX_CAPS
	/*
	 * Give up privs we do not need anymore.
	 * We no longer need:
	 *	cap_dac_override,cap_net_bind_service,cap_ipc_lock,cap_sys_nice,cap_sys_resource
	 * We still need:
	 *	cap_sys_rawio,cap_sys_admin
	 */
	cap_t		cset;
	cap_value_t	caplist[] = {
					CAP_DAC_OVERRIDE,
					CAP_NET_BIND_SERVICE,
					CAP_IPC_LOCK,
					CAP_SYS_NICE,
					CAP_SYS_RESOURCE
				};

	cset = cap_get_proc();

	cap_set_flag(cset, CAP_EFFECTIVE, NCAPS, caplist, CAP_CLEAR);
	cap_set_flag(cset, CAP_INHERITABLE, NCAPS, caplist, CAP_CLEAR);
	cap_set_flag(cset, CAP_PERMITTED, NCAPS, caplist, CAP_CLEAR);
	if (cap_set_proc(cset) < 0)
		errmsg("Cannot drop process capabilities.\n");
#endif	/* HAVE_LINUX_CAPS */
#endif	/* HAVE_PRIV_SET */
}

/*
 * Return TRUE if we have privileges that are not from a suid-root operation.
 */
EXPORT BOOL
priv_from_priv()
{
#ifdef	HAVE_PRIV_SET
	return (priv_ineffect(PRIV_FILE_DAC_READ) &&
		    !priv_ineffect(PRIV_PROC_SETID));
#else
#ifdef	HAVE_LINUX_CAPS
	cap_t			cset;
	cap_flag_value_t	val = CAP_CLEAR;

	cset = cap_get_proc();

	cap_get_flag(cset, CAP_DAC_OVERRIDE, CAP_EFFECTIVE, &val);
	if (val == CAP_CLEAR)
		return (FALSE);
	val = CAP_SET;
	cap_get_flag(cset, CAP_SETUID, CAP_EFFECTIVE, &val);
	if (val == CAP_SET)
		return (FALSE);
	return (TRUE);

#else	/* HAVE_LINUX_CAPS */
	return (FALSE);
#endif	/* HAVE_LINUX_CAPS */
#endif	/* HAVE_PRIV_SET */
}
#endif	/* defined(CDRECORD) || defined(READCD) */

/*
 * An attempt to implement an abstraction layer to detect fine grained
 * privileges. This is not implemented in an efficient way (there are multiple
 * syscalls to fetch the privileges from the kernel) but a few milliseconds
 * should not count.
 */
EXPORT BOOL
priv_eff_priv(pname)
	int	pname;
{
#if	!defined(HAVE_SETEUID) || !defined(HAVE_GETEUID)
	return (TRUE);
#else
#ifdef	HAVE_SOLARIS_PPRIV
#define	DID_PRIV
	switch (pname) {

	case SCHILY_PRIV_FILE_DAC_READ:
		return (priv_eff(PRIV_FILE_DAC_READ));
	case SCHILY_PRIV_FILE_DAC_WRITE:
		return (priv_eff(PRIV_FILE_DAC_WRITE));
	case SCHILY_PRIV_SYS_DEVICES:
		return (priv_eff(PRIV_SYS_DEVICES));
	case SCHILY_PRIV_PROC_LOCK_MEMORY:
		return (priv_eff(PRIV_PROC_LOCK_MEMORY));
	case SCHILY_PRIV_PROC_PRIOCNTL:
		return (priv_eff(PRIV_PROC_PRIOCNTL));
	case SCHILY_PRIV_NET_PRIVADDR:
		return (priv_eff(PRIV_NET_PRIVADDR));
	}
#endif
#ifdef	HAVE_LINUX_CAPS
#define	DID_PRIV
	switch (pname) {

	case SCHILY_PRIV_FILE_DAC_READ:
		return (priv_eff(CAP_DAC_READ_SEARCH) || priv_eff(CAP_DAC_OVERRIDE));
	case SCHILY_PRIV_FILE_DAC_WRITE:
		return (priv_eff(CAP_DAC_OVERRIDE));
	case SCHILY_PRIV_SYS_DEVICES:
		return (priv_eff(CAP_SYS_RAWIO) && priv_eff(CAP_SYS_ADMIN));
	case SCHILY_PRIV_PROC_LOCK_MEMORY:
		return (priv_eff(CAP_IPC_LOCK) && priv_eff(CAP_SYS_RESOURCE));
	case SCHILY_PRIV_PROC_PRIOCNTL:
		return (priv_eff(CAP_SYS_NICE));
	case SCHILY_PRIV_NET_PRIVADDR:
		return (priv_eff(CAP_NET_BIND_SERVICE));
	}
#endif

#ifndef	DID_PRIV
	if (geteuid() == 0)
		return (TRUE);
#endif
	return (FALSE);
#endif
}

#ifdef	HAVE_SOLARIS_PPRIV
LOCAL BOOL
priv_eff(pname)
	const char	*pname;
{
	return (priv_ineffect(pname));
}
#endif

#ifdef	HAVE_LINUX_CAPS
LOCAL BOOL
priv_eff(pname)
	int		pname;
{
	cap_t			cset;
	cap_flag_value_t	val = CAP_CLEAR;

	cset = cap_get_proc();
	cap_get_flag(cset, pname, CAP_EFFECTIVE, &val);
	if (val == CAP_CLEAR)
		return (FALSE);
	return (TRUE);
}
#endif

#ifdef	HAVE_SOLARIS_PPRIV
#include <schily/varargs.h>
/*
 * Allow to compile binaries that work on Solaris 11 build 140 and later
 * as well as for older Solaris versions.
 * If getpflags(PRIV_PFEXEC) fails, we have no in-kernel pfexec() and we
 * just do nothing.
 */
#ifndef	PRIV_PFEXEC
#define	PRIV_PFEXEC	0x0100
#endif
/*
 * If PRIV_PFEXEC is defined, we have an in-kernel pfexec() that allows
 * suid-root less installation and let programs gain the needed additional
 * privileges even without a wrapper script.
 */
/* VARARGS2 */
#ifdef	PROTOTYPES
EXPORT void
do_pfexec(int ac, char *av[], ...)
#else
EXPORT void
do_pfexec(ac, av, va_list)
	int	ac;
	char	*av[];
	va_decl
#endif
{
#ifdef	PRIV_PFEXEC
	va_list		args;
	BOOL		priv_ok = TRUE;
	priv_set_t	*pset;
	int		oflag;
	char		*av0;
	const char	*priv;

	/*
	 * Avoid looping over execv().
	 * Return if we see our modified argv[0].
	 * If the first character of the last name component is a '+',
	 * just leave it as it is. We cannot restore the original character
	 * in this case. If it is an uppercase character, we assume
	 * that it was a translated lowercace character from our execv().
	 */
	av0 = strrchr(av[0], '/');
	if (av0 == NULL)
		av0 = av[0];
	else
		av0++;
	if (*av0 == '+')
		return;
	if (*av0 >= 'A' && *av0 <= 'Z') {
		*av0 = *av0 + 'a' - 'A';
		return;
	}

	/*
	 * Check for the current privileges.
	 * Silently abort attempting to gain more privileges
	 * in case any error occurs.
	 */
	pset = priv_allocset();
	if (pset == NULL)
		return;
	if (getppriv(PRIV_EFFECTIVE, pset) < 0)
		return;

	/*
	 * If we already have the needed privileges, we are done.
	 */
#ifdef	PROTOTYPES
	va_start(args, av);
#else
	va_start(args);
#endif
	while (priv_ok && (priv = va_arg(args, const char *)) != NULL) {
		if (!priv_ismember(pset, priv))
			priv_ok = FALSE;
	}
	va_end(args);
	priv_freeset(pset);
	if (priv_ok)
		return;

	oflag = getpflags(PRIV_PFEXEC);
	if (oflag < 0)		/* Pre kernel-pfexec system? */
		return;
	if (oflag == 0) {	/* Kernel pfexec flag not yet set? */
		/*
		 * Set kernel pfexec flag.
		 * Return if this doesn't work for some reason.
		 */
		if (setpflags(PRIV_PFEXEC, 1) != 0) {
			return;
		}
	}
	/*
	 * Modify argv[0] to mark that we did already call execv().
	 * This is done in order to avoid infinite execv() loops caused by
	 * a missconfigured security system in /etc/security.
	 *
	 * In the usual case (a lowercase letter in the firsh character of the
	 * last pathname component), convert it to an uppercase character.
	 * Otherwise overwrite this character by a '+' sign.
	 */
	if (*av0 >= 'a' && *av0 <= 'z')
		*av0 = *av0 - 'a' + 'A';
	else
		*av0 = '+';
	execv(getexecname(), av);
#endif	/* PRIV_PFEXEC */
}
#endif
