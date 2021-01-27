/* @(#)priv.h	1.5 13/05/28 Copyright 2009-2013 J. Schilling */
/*
 *	Abstraction code for fine grained process privileges
 *
 *	Copyright (c) 2009-2013 J. Schilling
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

#ifndef	_SCHILY_PRIV_H
#define	_SCHILY_PRIV_H

#ifndef _SCHILY_MCONFIG_H
#include <schily/mconfig.h>
#endif

/*
 * The Solaris process privileges interface.
 */
#if	defined(HAVE_PRIV_H) && \
	defined(HAVE_GETPPRIV) && defined(HAVE_SETPPRIV) && \
	defined(HAVE_PRIV_SET)

#define	HAVE_SOLARIS_PPRIV
#endif

#ifdef	NO_SOLARIS_PPRIV
#undef	HAVE_SOLARIS_PPRIV
#endif

#ifdef	HAVE_SOLARIS_PPRIV
#ifndef	_INCL_PRIV_H
#define	_INCL_PRIV_H
#include <priv.h>
#endif
#endif

/*
 * AIX implements an incompatible process privileges interface.
 * On AIX, we have sys/priv.h, getppriv(), setppriv() but no priv_set().
 */
#if	defined(HAVE_SYS_PRIV_H) && \
	defined(HAVE_GETPPRIV) && defined(HAVE_SETPPRIV) && \
	defined(HAVE_PRIVBIT_SET)

#define	HAVE_AIX_PPRIV
#endif

#ifdef	NO_AIX_PPRIV
#undef	HAVE_AIX_PPRIV
#endif

#ifdef	HAVE_AIX_PPRIV
#ifndef	_INCL_SYS_PRIV_H
#define	_INCL_SYS_PRIV_H
#include <sys/priv.h>
#endif
#endif

/*
 * The POSIX.1e draft has been withdrawn in 1997.
 * Linux started to implement this outdated concept in 1997.
 * On Linux, we have sys/capability.h, cap_get_proc(), cap_set_proc(),
 * cap_set_flag() cap_clear_flag()
 */
#if	defined(HAVE_SYS_CAPABILITY_H) && \
	defined(HAVE_CAP_GET_PROC) && defined(HAVE_CAP_SET_PROC) && \
	defined(HAVE_CAP_SET_FLAG) && defined(HAVE_CAP_CLEAR_FLAG)

#define	HAVE_LINUX_CAPS
#endif

#ifdef	NO_LINUX_CAPS
#undef	HAVE_LINUX_CAPS
#endif

#ifdef	HAVE_LINUX_CAPS
#ifndef	_INCL_SYS_CAPABILITY_H
#define	_INCL_SYS_CAPABILITY_H
#include <sys/capability.h>
#endif
#endif

/*
 * Privileges abstraction layer definitions
 */
#define	SCHILY_PRIV_FILE_CHOWN		10	/* Allow to chown any file */
#define	SCHILY_PRIV_FILE_CHOWN_SELF	11	/* Allow to chown own files */
#define	SCHILY_PRIV_FILE_DAC_EXECUTE	12	/* Overwrite execute permission */
#define	SCHILY_PRIV_FILE_DAC_READ	13	/* Overwrite read permission */
#define	SCHILY_PRIV_FILE_DAC_SEARCH	14	/* Overwrite dir search permission */
#define	SCHILY_PRIV_FILE_DAC_WRITE	15	/* Overwrite write permission */
#define	SCHILY_PRIV_FILE_DOWNGRADE_SL	16	/* Downgrade sensivity label */
#define	SCHILY_PRIV_FILE_LINK_ANY	17	/* Hard-link files not owned */
#define	SCHILY_PRIV_FILE_OWNER		18	/* Allow chmod ... to unowned files */
#define	SCHILY_PRIV_FILE_SETID		19	/* Allow chown or suid/sgid without being owner */
#define	SCHILY_PRIV_FILE_UPGRADE_SL	20	/* Upgrade sensivity label */
#define	SCHILY_PRIV_FILE_FLAG_SET	22	/* Allow set file attributes as "immutable" */

#define	SCHILY_PRIV_IPC_DAC_READ	40	/* Overwrite read permission */
#define	SCHILY_PRIV_IPC_DAC_WRITE	41	/* Overwrite write permission */
#define	SCHILY_PRIV_IPC_OWNER		42	/* Allow chmod ... to unowned files */

#define	SCHILY_PRIV_NET_BINDMLP		50	/* Allow to bind multi-level ports */
#define	SCHILY_PRIV_NET_ICMPACCESS	51	/* Allow to send/receive ICMP packets */
#define	SCHILY_PRIV_NET_MAC_AWARE	52	/* Allow to set NET_MAC_AWARE flag */
#define	SCHILY_PRIV_NET_OBSERVABILITY	53	/* Allow tp access network device for receiving traffic */
#define	SCHILY_PRIV_NET_PRIVADDR	54	/* Allow to bind priv ports */
#define	SCHILY_PRIV_NET_RAWACCESS	55	/* Allow raw network access */

#define	SCHILY_PRIV_PROC_AUDIT		60	/* Allow to create audit records */
#define	SCHILY_PRIV_PROC_CHROOT		61	/* Allow chroot */
#define	SCHILY_PRIV_PROC_CLOCK_HIGHRES	62	/* Allow to use high resulution timers */
#define	SCHILY_PRIV_PROC_EXEC		63	/* Allow to call exec*() */
#define	SCHILY_PRIV_PROC_FORK		64	/* Allow to call fork*()/vfork*() */
#define	SCHILY_PRIV_PROC_INFO		65	/* Allow to examine /proc status without sendsig priv */
#define	SCHILY_PRIV_PROC_LOCK_MEMORY	66	/* Allow to lock pages into physical memory */
#define	SCHILY_PRIV_PROC_OWNER		67	/* Allow sendsig and /proc to other procs */
#define	SCHILY_PRIV_PROC_PRIOCNTL	68	/* Allow to send sognals or trace outside session */
#define	SCHILY_PRIV_PROC_SESSION	68	/* Allow to send sognals or trace outside session */
#define	SCHILY_PRIV_PROC_SETID		69	/* Allow set proc's UID/GID */

#define	SCHILY_PRIV_SYS_ACCT		80	/* Allow process accounting */
#define	SCHILY_PRIV_SYS_ADMIN		81	/* Allow system administration */
#define	SCHILY_PRIV_SYS_AUDIT		82	/* Allow so start kernel auditing */
#define	SCHILY_PRIV_SYS_CONFIG		83	/* Allow various system config tasks */
#define	SCHILY_PRIV_SYS_DEVICES		84	/* Allow device specific stuff */
#define	SCHILY_PRIV_SYS_DL_CONFIG	85	/* Allow tp configure datalink interfaces */
#define	SCHILY_PRIV_SYS_IP_CONFIG	86	/* Allow to configure IP interfaces */
#define	SCHILY_PRIV_SYS_LINKDIR		87	/* Allow to link/unlink directories */
#define	SCHILY_PRIV_SYS_MOUNT		88	/* Allow file-system administration */
#define	SCHILY_PRIV_SYS_NET_CONFIG	89	/* Allow to configure the network */
#define	SCHILY_PRIV_SYS_NFS		90	/* Allow to configure NFS */
#define	SCHILY_PRIV_SYS_PPP_CONFIG	91	/* Allow to configure PPP */
#define	SCHILY_PRIV_SYS_RES_CONFIG	92	/* Allow to configure system resources */
#define	SCHILY_PRIV_SYS_RESOURCE	93	/* Allow setrlimit */
#define	SCHILY_PRIV_SYS_SMB		94	/* Allow to configure SMB */
#define	SCHILY_PRIV_SYS_SUSER_COMPAT	95	/* Allow to load modules that call suser() */
#define	SCHILY_PRIV_SYS_TIME		96	/* Allow to set time */
#define	SCHILY_PRIV_SYS_TRANS_LABEL	97	/* Allow to translate labels in trusted extensions */

#endif	/* _SCHILY_PRIV_H */
