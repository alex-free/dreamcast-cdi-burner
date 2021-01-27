/* @(#)data.h	1.2 01/11/11 joerg */
/*
 * hfsutils - tools for reading and writing Macintosh HFS volumes
 * Copyright (C) 1996, 1997 Robert Leslie
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

char d_getb		__PR((unsigned char *));
short d_getw		__PR((unsigned char *));
long d_getl		__PR((unsigned char *));

void d_putb		__PR((unsigned char *, char));
void d_putw		__PR((unsigned char *, short));
void d_putl		__PR((unsigned char *, long));

void d_fetchb		__PR((unsigned char **, char *));
void d_fetchw		__PR((unsigned char **, short *));
void d_fetchl		__PR((unsigned char **, long *));
void d_fetchs		__PR((unsigned char **, char *, int));

void d_storeb		__PR((unsigned char **, char));
void d_storew		__PR((unsigned char **, short));
void d_storel		__PR((unsigned char **, long));
void d_stores		__PR((unsigned char **, char *, int));

unsigned long d_tomtime	__PR((unsigned long));
unsigned long d_toutime	__PR((unsigned long));
#ifdef APPLE_HYB
unsigned long d_dtoutime __PR((long));
#endif /* APPLE_HYB */

int d_relstring		__PR((char *, char *));
