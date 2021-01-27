/* @(#)dlfcn.c	1.1 14/05/15 Copyright 2014 J. Schilling */
/*
 *	Functions to support POSIX shared library handling
 *
 *	Copyright (c) 2014 J. Schilling
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

#include <schily/standard.h>
#include <schily/dlfcn.h>
#include <schily/errno.h>

#ifndef	HAVE_DLOPEN

#if !defined(DID_DLOPEN) && defined(HAVE_LOADLIBRARY)	/* Win-DOS */
#define	DID_DLOPEN

#include <schily/windows.h>

LOCAL	int	_dl_lasterror;

EXPORT void *
dlopen(pathname, mode)
	const char	*pathname;
	int		mode;
{
	void	*ret = NULL;

	/*
	 * Nur eine Directory, kein PATH:
	 *
	 * BOOL SetDllDirectory(char *pathname)
	 * DWORD GetDllDirectory(DWORD nBufferLength, LPTSTR lpBuffer) -> len
	 */
	ret = LoadLibrary(pathname);
	if (ret == NULL)
		_dl_lasterror = GetLastError();

	return (ret);
}

EXPORT int
dlclose(handle)
	void		*handle;
{
	int	ret = 0;

	if (!FreeLibrary(handle)) {
		_dl_lasterror = GetLastError();
		ret = -1;
	}

	return (ret);
}

EXPORT void *
dlsym(handle, name)
	void		*handle;
	const char	*name;
{
	void	*ret = NULL;

	ret = GetProcAddress(handle, name);
	if (ret == NULL)
		_dl_lasterror = GetLastError();

	return (ret);
}

#define	ERROR_BUFFER_SIZE	1024
EXPORT const char *
dlerror()
{
	const char	*ret = NULL;
	DWORD		dwMsgLen;
static	char	buff[ERROR_BUFFER_SIZE + 1];

	if (_dl_lasterror == 0)
		return (ret);

	buff[0] = 0;
	dwMsgLen = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM
				|FORMAT_MESSAGE_IGNORE_INSERTS
				|FORMAT_MESSAGE_MAX_WIDTH_MASK,
				NULL,
				_dl_lasterror,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPSTR)buff,
				ERROR_BUFFER_SIZE,
				NULL);
	_dl_lasterror = 0;
	ret = (const char *)buff;

	return (ret);
}

#endif	/* HAVE_LOADLOBRARY */

#if !defined(DID_DLOPEN) && defined(HAVE_SHL_LOAD)	/* HP-UX */
#define	DID_DLOPEN

#include <schily/string.h>				/* for sterror() */

LOCAL	int	_dl_lasterror;

EXPORT void *
dlopen(pathname, mode)
	const char	*pathname;
	int		mode;
{
	void	*ret;
	int	flags = BIND_IMMEDIATE;

	if (mode == RTLD_LAZY)
		flags = BIND_DEFERRED;

	ret = shl_load(pathname, flags, 0L);
	if (ret == NULL)
		_dl_lasterror = errno;

	return (ret);
}

EXPORT int
dlclose(handle)
	void		*handle;
{
	return (shl_unload(handle));
}

EXPORT void *
dlsym(handle, name)
	void		*handle;
	const char	*name;
{
	void	*ret = NULL;

	if (shl_findsym(handle, name, TYPE_UNDEFINED, &ret) != 0)
		_dl_lasterror = errno;

	return (ret);
}

EXPORT const char *
dlerror()
{
	const char	*ret = NULL;

	if (_dl_lasterror == 0)
		return (ret);
	ret = strerror(_dl_lasterror);
	_dl_lasterror = 0;

	return (ret);
}

#endif	/* HAVE_SHL_LOAD */

#if !defined(DID_DLOPEN)			/* unknown OS */
#define	DID_DLOPEN

/*
 * If we do not have dlopen(), we at least need to define a
 * dummy function of that name.
 */

EXPORT void *
dlopen(pathname, mode)
	const char	*pathname;
	int		mode;
{
	void	*ret = NULL;

	return (ret);
}

EXPORT int
dlclose(handle)
	void		*handle;
{
	int	ret = 0;

	return (ret);
}

EXPORT void *
dlsym(handle, name)
	void		*handle;
	const char	*name;
{
	void	*ret = NULL;

	return (ret);
}

EXPORT const char *
dlerror()
{
	const char	*ret = NULL;

	return (ret);
}

#endif	/* HAVE_SHL_LOAD */


#else	/* HAVE_DLOPEN */
#endif	/* HAVE_DLOPEN */
