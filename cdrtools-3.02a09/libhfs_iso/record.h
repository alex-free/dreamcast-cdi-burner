/* @(#)record.h	1.1 00/03/05 joerg */
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

void r_packcatkey	__PR((CatKeyRec *, unsigned char *, int *));
void r_unpackcatkey	__PR((unsigned char *, CatKeyRec *));

void r_packextkey	__PR((ExtKeyRec *, unsigned char *, int *));
void r_unpackextkey	__PR((unsigned char *, ExtKeyRec *));

int r_comparecatkeys	__PR((unsigned char *, unsigned char *));
int r_compareextkeys	__PR((unsigned char *, unsigned char *));

void r_packcatdata	__PR((CatDataRec *, unsigned char *, int *));
void r_unpackcatdata	__PR((unsigned char *, CatDataRec *));

void r_packextdata	__PR((ExtDataRec *, unsigned char *, int *));
void r_unpackextdata	__PR((unsigned char *, ExtDataRec *));

void r_makecatkey	__PR((CatKeyRec *, long, char *));
void r_makeextkey	__PR((ExtKeyRec *, int, long, unsigned int));

void r_unpackdirent	__PR((long, char *, CatDataRec *, hfsdirent *));
void r_packdirent	__PR((CatDataRec *, hfsdirent *));
