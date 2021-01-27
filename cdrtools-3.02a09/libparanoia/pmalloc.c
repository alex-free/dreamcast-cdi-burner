/* @(#)pmalloc.c	1.7 09/07/11 Copyright 2004-2009 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)pmalloc.c	1.7 09/07/11 Copyright 2004-2009 J. Schilling";
#endif
/*
 *	Paranoia malloc() functions
 *
 *	Copyright (c) 2004-2009 J. Schilling
 */
/*
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2.1
 * as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <schily/stdlib.h>
#include <schily/standard.h>
#include <schily/schily.h>
#include "pmalloc.h"

#ifdef	PM_ADD_DEBUG
LOCAL int madd = 8192;
LOCAL int cadd = 8192;
LOCAL int radd = 8192;
#else
LOCAL int madd = 0;
/*LOCAL int cadd = 0;*/
LOCAL int radd = 0;
#endif

EXPORT void
_pfree(ptr)
	void	*ptr;
{
	if (ptr)
		free(ptr);
}

EXPORT void *
_pmalloc(size)
	size_t	size;
{
	void	*p;

	p = malloc(size + madd);
	if (p == NULL)
		raisecond("NO MEM", 0L);
	return (p);
}

EXPORT void *
_pcalloc(nelem, elsize)
	size_t	nelem;
	size_t	elsize;
{
	void	*p;
#ifdef	PM_ADD_DEBUG
	size_t	n = nelem * elsize;

	n += cadd;
	p = calloc(1, n);
#else
	p = calloc(nelem, elsize);
#endif
	if (p == NULL)
		raisecond("NO MEM", 0L);
	return (p);
}

EXPORT void *
_prealloc(ptr, size)
	void	*ptr;
	size_t	size;
{
	void	*p;

	if (ptr == 0)
		return (_pmalloc(size));

	p = realloc(ptr, size + radd);
	if (p == NULL)
		raisecond("NO MEM", 0L);
	return (p);
}
