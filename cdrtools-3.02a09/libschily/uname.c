/* @(#)uname.c	1.2 11/08/16 Copyright 2011 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)uname.c	1.2 11/08/16 Copyright 2011 J. Schilling";
#endif
/*
 *	uname() replacement in case it does not exist
 *
 *	Copyright (c) 2011 J. Schilling
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

#ifndef	HAVE_UNAME

#include <schily/standard.h>
#include <schily/utypes.h>
#include <schily/utsname.h>
#include <schily/schily.h>

#if	defined(__MINGW32__) || defined(_MSC_VER)
#define	gethostname	__winsock_gethostname
#include <schily/windows.h>
#undef	gethostname

/*
 * cystyle believes BOOL is a function name and does not like:
 * typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
 */
typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

LOCAL BOOL
iswow64()
{
	BOOL			IsWow64 = FALSE;
	LPFN_ISWOW64PROCESS	IsWow64Processp;

	/*
	 * "IsWow64Process" is not supported on all platforms
	 */
	IsWow64Processp = (LPFN_ISWOW64PROCESS)
			GetProcAddress(GetModuleHandle(TEXT("kernel32")),
			"IsWow64Process");

	if (IsWow64Processp) {
		if (!IsWow64Processp(GetCurrentProcess(), &IsWow64))
			return (FALSE);
	}
	return (IsWow64 != 0);
}

EXPORT int
uname(ubuf)
	struct utsname	*ubuf;
{
	SYSTEM_INFO	sysinfo;
	uint32_t	version;
	uint32_t	majversion;
	uint32_t	minversion;
	uint32_t	builtnum = 0;
	char		dbuf[16];
	char		*p;

	strlcpy(ubuf->sysname, "Win32", sizeof (ubuf->sysname));
	if (iswow64())
		strlcat(ubuf->sysname, "-WOW64", sizeof (ubuf->sysname));

	gethostname(ubuf->nodename, sizeof (ubuf->nodename));
	ubuf->nodename[sizeof (ubuf->nodename)-1] = '\0';

	version = GetVersion();
	majversion = version & 0xFF;
	minversion = (version >> 8) & 0xFF;
	if (version < 0x80000000)
		builtnum = (version >> 16) & 0xFFFF;

	strlcpy(ubuf->release, "NT-0.0", sizeof (ubuf->release));
	if (sizeof (ubuf->release) > 7) {
		ubuf->release[3] = majversion + '0';
		ubuf->release[5] = minversion + '0';
	}

	p = &dbuf[sizeof (dbuf)-1];
	*p = '\0';
	do {
		*--p = builtnum % 10 + '0';
		builtnum /= 10;
	} while (builtnum > 0);
	strlcpy(ubuf->version, p, sizeof (ubuf->version));

	/*
	 * cpu, pagesize, numproc, ...
	 * See also IsWow64Process()
	 */
	GetSystemInfo(&sysinfo);

	switch (sysinfo.wProcessorArchitecture) {
#ifdef	PROCESSOR_ARCHITECTURE_INTEL
	case PROCESSOR_ARCHITECTURE_INTEL:
		switch (sysinfo.dwProcessorType) {

#ifdef	PROCESSOR_INTEL_386
		case PROCESSOR_INTEL_386:
			/* FALLTHRU */
#endif
		default:
			strlcpy(ubuf->machine, "i386", sizeof (ubuf->machine));
			break;
#ifdef	PROCESSOR_INTEL_486
		case PROCESSOR_INTEL_486:
			strlcpy(ubuf->machine, "i486", sizeof (ubuf->machine));
			break;
#endif
#ifdef	PROCESSOR_INTEL_PENTIUM
		case PROCESSOR_INTEL_PENTIUM: {
				int level = sysinfo.wProcessorLevel;
				if (level > 9)
					level = 6;
				strlcpy(ubuf->machine, "i586", sizeof (ubuf->machine));
				if (level > 5)
					ubuf->machine[1] = level + '0';
			}
			break;
#endif
		}
		break;
#endif
#ifdef	PROCESSOR_ARCHITECTURE_MIPS
	case PROCESSOR_ARCHITECTURE_MIPS:
		strlcpy(ubuf->machine, "mips", sizeof (ubuf->machine));
		break;
#endif
#ifdef	PROCESSOR_ARCHITECTURE_ALPHA
	case PROCESSOR_ARCHITECTURE_ALPHA:
		strlcpy(ubuf->machine, "alpha", sizeof (ubuf->machine));
		break;
#endif
#ifdef	PROCESSOR_ARCHITECTURE_PPC
	case PROCESSOR_ARCHITECTURE_PPC:
		strlcpy(ubuf->machine, "PPC", sizeof (ubuf->machine));
		break;
#endif
#ifdef	PROCESSOR_ARCHITECTURE_SHX
	case PROCESSOR_ARCHITECTURE_SHX:
		strlcpy(ubuf->machine, "shx", sizeof (ubuf->machine));
		break;
#endif
#ifdef	PROCESSOR_ARCHITECTURE_ARM
	case PROCESSOR_ARCHITECTURE_ARM:
		strlcpy(ubuf->machine, "arm", sizeof (ubuf->machine));
		break;
#endif
#ifdef	PROCESSOR_ARCHITECTURE_IA64
	case PROCESSOR_ARCHITECTURE_IA64:
		strlcpy(ubuf->machine, "ia64", sizeof (ubuf->machine));
		break;
#endif
#ifdef	PROCESSOR_ARCHITECTURE_ALPHA64
	case PROCESSOR_ARCHITECTURE_ALPHA64:
		strlcpy(ubuf->machine, "alpha64", sizeof (ubuf->machine));
		break;
#endif
#ifdef	PROCESSOR_ARCHITECTURE_MSIL
	case PROCESSOR_ARCHITECTURE_MSIL:
		strlcpy(ubuf->machine, "msil", sizeof (ubuf->machine));
		break;
#endif
#ifdef	PROCESSOR_ARCHITECTURE_AMD64
	case PROCESSOR_ARCHITECTURE_AMD64:
		strlcpy(ubuf->machine, "amd64", sizeof (ubuf->machine));
		break;
#endif
#ifdef	PROCESSOR_ARCHITECTURE_IA32_ON_WIN64
	case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
		strlcpy(ubuf->machine, "ia32_win64", sizeof (ubuf->machine));
		break;
#endif
	default:
		strlcpy(ubuf->machine, "unknown", sizeof (ubuf->machine));
	}
	return (0);
}

#else
EXPORT int
uname(ubuf)
	struct utsname	*ubuf;
{
	strlcpy(ubuf->sysname, "unknown", sizeof (ubuf->sysname));
	strlcpy(ubuf->nodename, "unknown", sizeof (ubuf->nodename));
	strlcpy(ubuf->release, "unknown", sizeof (ubuf->release));
	strlcpy(ubuf->version, "unknown", sizeof (ubuf->version));

	strlcpy(ubuf->machine, "unknown", sizeof (ubuf->machine));
#ifdef	mc68000
#ifdef	mc68020
	strlcpy(ubuf->machine, "mc68020", sizeof (ubuf->machine));
#else
	strlcpy(ubuf->machine, "mc68000", sizeof (ubuf->machine));
#endif
#endif
#if	defined(__sparc) || defined(sparc)
	strlcpy(ubuf->machine, "sparc", sizeof (ubuf->machine));
#endif
#if	defined(__i386__) || defined(__i386) || defined(i386)
#if	defined(__amd64__) || defined(__amd64)
	strlcpy(ubuf->machine, "amd64", sizeof (ubuf->machine));
#else
	strlcpy(ubuf->machine, "i386", sizeof (ubuf->machine));
#endif
#endif
	return (0);
}
#endif

#endif	/* HAVE_UNAME */
