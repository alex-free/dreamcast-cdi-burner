/* @(#)pmalloc.h	1.1 04/02/20 Copyright 2004 J. Schilling */
/*
 *	Paranoia malloc() functions
 *
 *	Copyright (c) 2004 J. Schilling
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

#ifndef	_PMALLOC_H
#define	_PMALLOC_H

extern	void	_pfree		__PR((void *ptr));
extern	void	*_pmalloc	__PR((size_t size));
extern	void	*_pcalloc	__PR((size_t nelem, size_t elsize));
extern	void	*_prealloc	__PR((void *ptr, size_t size));

#endif	/* _PMALLOC_H */
