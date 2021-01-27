/* @(#)setuid.c	1.23 15/09/15 Copyright 1998,1999,2004 Heiko Eissfeldt, Copyright 2004-2011 J. Schilling */
#include "config.h"
#ifndef lint
static	UConst char sccsid[] =
"@(#)setuid.c	1.23 15/09/15 Copyright 1998,1999,2004 Heiko Eissfeldt, Copyright 2004-2011 J. Schilling";

#endif
/*
 * Security functions
 *
 * If these functions fail, it is because there was an installation error
 * or a programming error, and we can't be sure about what privileges
 * we do or do not have.  This means we might not be able to recover
 * the privileges we need to fix anything that may be broken (e.g. the
 * CDDA state of some interface types), and we may in fact do something
 * quite dangerous (like write to the WAV file as root).
 *
 * In any case, it is unsafe to do anything but exit *now*.  Ideally we'd
 * kill -9 our process group too, just to be sure.  Root privileges are not
 * something you want floating around at random in user-level applications.
 *
 * If any signal handlers or child processes are introduced into this
 * program, it will be necessary to call dontneedroot() or neverneedroot()
 * on entry, respectively; otherwise, it will be possible to trick
 * the program into executing the signal handler or child process with
 * root privileges by sending signals at the right time.
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

#include "config.h"
#include <schily/unistd.h>
#include <schily/stdio.h>
#include <schily/stdlib.h>
#include <schily/schily.h>
#include <schily/nlsdefs.h>

#include "exitcodes.h"
#include "setuid.h"

/*#undef DEBUG*/
/*#define DEBUG*/

/* True at return from initsecurity */
static uid_t real_uid = (uid_t) (-1);
static uid_t effective_uid = (uid_t) (-1);
static gid_t real_gid = (gid_t) (-1);
static gid_t effective_gid = (gid_t) (-1);

/*
 * Run this at the beginning of the program to initialize this code and
 * to drop privileges before someone uses them to shoot us in the foot.
 * Do not pass(go), do not dollars += 200.
 */
void
initsecurity()
{
	int	leffective_uid;

#ifdef	HAVE_ALARM
	alarm(0);		/* can be inherited from parent process */
#endif
	real_uid = getuid();
	leffective_uid = geteuid();
	if ((int) real_uid != leffective_uid && leffective_uid != 0) { /* sanity check */
		errmsgno(EX_BAD,
		_("Warning: setuid but not to root (uid=%ld, euid=%d)\n"),
				(long) real_uid, leffective_uid);
		fprintf(stderr, _("Dropping setuid privileges now.\n"));
		neverneedroot();
	} else {
		effective_uid = leffective_uid;
	}
	real_gid = getgid();
	effective_gid = getegid();
	dontneedroot();
	dontneedgroup();
}

/* Temporarily gain root privileges. */

#if defined _POSIX_SAVED_IDS && defined(HAVE_SETEUID) && defined SCO
/* SCO seems to lack the prototypes... */
int	seteuid __PR((uid_t euid));
int	setegid __PR((gid_t guid));
#endif

void
needroot(necessary)
	int	necessary;
{
#ifdef DEBUG
	fprintf(stderr,
	"call to     needroot (_euid_=%d, uid=%d), current=%d/%d, pid=%d\n",
			effective_uid, real_uid,
			geteuid(), getuid(), getpid());
#endif
	if (effective_uid) {
		if (necessary) {
			errmsgno(EX_BAD,
			_("Fatal error: require root privilege but not setuid root.\n"));
			exit(PERM_ERROR);
		} else
			return;
	}
	if (real_uid == (uid_t) (-1)) {
		errmsgno(EX_BAD, _("Fatal error: initsecurity() not called.\n"));
		exit(INTERNAL_ERROR);
	}

	if (geteuid() == 0)
		return; /* nothing to do */

#if defined _POSIX_SAVED_IDS && defined(HAVE_SETEUID)
	if (seteuid(effective_uid)) {
		errmsg(_("Error with seteuid in needroot().\n"));
		exit(PERM_ERROR);
	}
#else
#if defined(HAVE_SETREUID)
	if (setreuid(real_uid, effective_uid)) {
		errmsg(_("Error with setreuid in needroot().\n"));
		exit(PERM_ERROR);
	}
#endif
#endif
	    if (geteuid() != 0 && necessary) {
		errmsgno(EX_BAD, _("Fatal error: did not get root privilege.\n"));
		exit(PERM_ERROR);
	}
#ifdef DEBUG
	fprintf(stderr,
	"exit of     needroot (_euid_=%d, uid=%d), current=%d/%d, pid=%d\n",
			effective_uid, real_uid,
			geteuid(), getuid(), getpid());
#endif
}

/*
 * Temporarily drop root privileges.
 */
void
dontneedroot()
{
#ifdef DEBUG
	fprintf(stderr,
	"call to dontneedroot (_euid_=%d, uid=%d), current=%d/%d, pid=%d\n",
			effective_uid, real_uid,
			geteuid(), getuid(), getpid());
#endif
	if (real_uid == (uid_t) (-1)) {
		errmsgno(EX_BAD, _("Fatal error: initsecurity() not called.\n"));
		exit(INTERNAL_ERROR);
	}
	if (effective_uid)
		return;
	if (geteuid() != 0)
		return; /* nothing to do */

#if defined _POSIX_SAVED_IDS && defined(HAVE_SETEUID)
	if (seteuid(real_uid)) {
		errmsg(_("Error with seteuid in dontneedroot().\n"));
		exit(PERM_ERROR);
	}
#else
#if defined(HAVE_SETREUID)
	if (setreuid(effective_uid, real_uid)) {
		errmsg(_("Error with setreuid in dontneedroot().\n"));
		exit(PERM_ERROR);
	}
#endif
#endif
	if (geteuid() != real_uid) {
		errmsgno(EX_BAD,
			_("Fatal error: did not drop root privilege.\n"));
#ifdef DEBUG
		fprintf(stderr,
		"in   to dontneedroot (_euid_=%d, uid=%d), current=%d/%d, pid=%d\n",
			effective_uid, real_uid,
			geteuid(), getuid(), getpid());
#endif
		exit(PERM_ERROR);
	}
}

/*
 * Permanently drop root privileges.
 */
void
neverneedroot()
{
#ifdef DEBUG
	fprintf(stderr,
	"call to neverneedroot (_euid_=%d, uid=%d), current=%d/%d, pid=%d\n",
			effective_uid, real_uid,
			geteuid(), getuid(), getpid());
#endif
	if (real_uid == (uid_t) (-1)) {
		errmsgno(EX_BAD, _("Fatal error: initsecurity() not called.\n"));
		exit(INTERNAL_ERROR);
	}
	if (geteuid() != effective_uid) {
		needroot(1);
	}
	if (geteuid() == effective_uid) {
#if defined(HAVE_SETUID)
		if (setuid(real_uid)) {
			errmsg(_("Error with setuid in neverneedroot().\n"));
			exit(PERM_ERROR);
		}
#endif
	}
#if	defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || \
	defined(__DragonFly__)	/* XXX this is a big hack and and not a permanent solution */
	else {
#if defined(HAVE_SETUID)
		if (setuid(real_uid)) {
			errmsg(_("Error with setuid in neverneedroot().\n"));
			exit(PERM_ERROR);
		}
#endif
	}
#endif
	if (geteuid() != real_uid || getuid() != real_uid) {
		errmsgno(EX_BAD,
		_("Fatal error: did not drop root privilege.\n"));
#ifdef DEBUG
		fprintf(stderr,
		"in  to neverneedroot (_euid_=%d, uid=%d), current=%d/%d, pid=%d\n",
			effective_uid, real_uid,
			geteuid(), getuid(), getpid());
#endif
		exit(PERM_ERROR);
	}
	effective_uid = real_uid;
#ifdef DEBUG
	fprintf(stderr,
	"exit of neverneedroot (_euid_=%d, uid=%d), current=%d/%d, pid=%d\n",
			effective_uid, real_uid,
			geteuid(), getuid(), getpid());
#endif
}

/* Temporarily gain group privileges. */

void
needgroup(necessary)
	int	necessary;
{
#ifdef DEBUG
	fprintf(stderr,
	"call to     needgroup (egid=%d, gid=%d), current=%d/%d, pid=%d\n",
			effective_gid, real_gid,
			getegid(), getgid(), getpid());
#endif
	if (real_gid == (gid_t) (-1)) {
		errmsgno(EX_BAD,
		_("Fatal error: initsecurity() not called.\n"));
		exit(INTERNAL_ERROR);
	}

	if (getegid() == effective_gid)
		return; /* nothing to do */

#if defined _POSIX_SAVED_IDS && defined(HAVE_SETEGID)
	if (setegid(effective_gid)) {
		errmsg(_("Error with setegid in needgroup().\n"));
		exit(PERM_ERROR);
	}
#else
#if defined(HAVE_SETREGID)
	if (setregid(real_gid, effective_gid)) {
		errmsg(_("Error with setregid in needgroup().\n"));
		exit(PERM_ERROR);
	}
#endif
#endif
	if (necessary && getegid() != effective_gid) {
		errmsgno(EX_BAD,
			_("Fatal error: did not get group privilege.\n"));
		exit(PERM_ERROR);
	}
}

/*
 * Temporarily drop group privileges.
 */
void
dontneedgroup()
{
#ifdef DEBUG
	fprintf(stderr,
	"call to dontneedgroup (egid=%d, gid=%d), current=%d/%d, pid=%d\n",
			effective_gid, real_gid,
			getegid(), getgid(), getpid());
#endif
	if (real_gid == (gid_t) (-1)) {
		errmsgno(EX_BAD, _("Fatal error: initsecurity() not called.\n"));
		exit(INTERNAL_ERROR);
	}
	if (getegid() != effective_gid)
		return; /* nothing to do */
#if defined _POSIX_SAVED_IDS && defined(HAVE_SETEGID)
	if (setegid(real_gid)) {
		errmsg(_("Error with setegid in dontneedgroup().\n"));
		exit(PERM_ERROR);
	}
#else
#if defined(HAVE_SETREGID)
	if (setregid(effective_gid, real_gid)) {
		errmsg(_("Error with setregid in dontneedgroup().\n"));
		exit(PERM_ERROR);
	}
#endif
#endif
	if (getegid() != real_gid) {
		errmsgno(EX_BAD,
			_("Fatal error: did not drop group privilege.\n"));
		exit(PERM_ERROR);
	}
#ifdef DEBUG
	fprintf(stderr,
	"exit if dontneedgroup (egid=%d, gid=%d), current=%d/%d, pid=%d\n",
			effective_gid, real_gid,
			getegid(), getgid(), getpid());
#endif
}

/*
 * Permanently drop group privileges.
 */
void
neverneedgroup()
{
#ifdef DEBUG
	fprintf(stderr,
	"call to neverneedgroup (egid=%d, gid=%d), current=%d/%d, pid=%d\n",
			effective_gid, real_gid,
			getegid(), getgid(), getpid());
#endif
	if (real_gid == (gid_t) (-1)) {
		errmsgno(EX_BAD, _("Fatal error: initsecurity() not called.\n"));
		exit(INTERNAL_ERROR);
	}
	if (getegid() != effective_gid) {
		needgroup(1);
	}
	if (getegid() == effective_gid) {
#if defined(HAVE_SETGID)
		if (setgid(real_gid)) {
			errmsg(_("Error with setgid in neverneedgroup().\n"));
			exit(PERM_ERROR);
		}
#endif
	}
#if	defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || \
	defined(__DragonFly__)	/* XXX this is a big hack and and not a permanent solution */
	else {
#if defined(HAVE_SETGID)
		if (setgid(real_gid)) {
			errmsg(_("Error with setgid in neverneedgroup().\n"));
			exit(PERM_ERROR);
		}
#endif
	}
#endif
	if (getegid() != real_gid || getgid() != real_gid) {
		errmsgno(EX_BAD,
			_("Fatal error: did not drop group privilege.\n"));
#ifdef DEBUG
		fprintf(stderr,
		"in  to neverneedgroup (_egid_=%d, gid=%d), current=%d/%d, pid=%d\n",
			effective_gid, real_gid,
			getegid(), getgid(), getpid());
#endif
		exit(PERM_ERROR);
	}
	effective_gid = real_gid;
}

#if defined(HPUX)
int
seteuid(uid)
	uid_t	uid;
{
	return (setresuid(-1, uid, -1));
}

int
setreuid(uid1, uid2)
	uid_t	uid1;
	uid_t	uid2;
{
	return (setresuid(uid2, uid2, uid1 == uid2 ? uid2 : 0));
}

int
setregid(gid1, gid2)
	gid_t	gid1;
	gid_t	gid2;
{
	return (setresgid(gid2, gid2, gid1 == gid2 ? gid2 : 0));
}
#endif
