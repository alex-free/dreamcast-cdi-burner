/* @(#)btree.h	1.1 00/03/05 joerg */
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

int bt_getnode		__PR((node *));
int bt_putnode		__PR((node *));

int bt_readhdr		__PR((btree *));
int bt_writehdr		__PR((btree *));

int bt_space		__PR((btree *, unsigned int));

int bt_insertx		__PR((node *, unsigned char *, int *));
int bt_insert		__PR((btree *, unsigned char *, int));

int bt_deletex		__PR((node *, unsigned char *, unsigned char *, int *));
int bt_delete		__PR((btree *, unsigned char *));

int bt_search		__PR((btree *, unsigned char *, node *));
