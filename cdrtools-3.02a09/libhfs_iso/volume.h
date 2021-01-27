/* @(#)volume.h	1.1 00/03/05 joerg */
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

int v_catsearch		__PR((hfsvol *, long, char *, CatDataRec *, char *, node *));
int v_extsearch		__PR((hfsfile *, unsigned int, ExtDataRec *, node *));

int v_getthread		__PR((hfsvol *, long, CatDataRec *, node *, int));

# define v_getdthread(vol, id, thread, np)  \
    v_getthread(vol, id, thread, np, cdrThdRec)
# define v_getfthread(vol, id, thread, np)  \
    v_getthread(vol, id, thread, np, cdrFThdRec)

int v_putcatrec		__PR((CatDataRec *, node *));
int v_putextrec		__PR((ExtDataRec *, node *));

int v_allocblocks	__PR((hfsvol *, ExtDescriptor *));
void v_freeblocks	__PR((hfsvol *, ExtDescriptor *));

int v_resolve		__PR((hfsvol **, char *, CatDataRec *, long *, char *, node *));

void v_destruct		__PR((hfsvol *));
int v_getvol		__PR((hfsvol **));
int v_flush		__PR((hfsvol *, int));

int v_adjvalence	__PR((hfsvol *, long, int, int));
int v_newfolder		__PR((hfsvol *, long, char *));

int v_scavenge		__PR((hfsvol *));
