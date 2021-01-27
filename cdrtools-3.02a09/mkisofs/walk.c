/* @(#)walk.c	1.11 10/12/19 Copyright 2005-2010 J. Schilling */
#include <schily/mconfig.h>
#ifndef lint
static	UConst char sccsid[] =
	"@(#)walk.c	1.11 10/12/19 Copyright 2005-2010 J. Schilling";
#endif
/*
 *	This file contains the callback code for treewalk() as used
 *	with mkisofs -find.
 *
 *	Copyright (c) 2005-2010 J. Schilling
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; see the file COPYING.  If not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <schily/stat.h>
#include <schily/walk.h>
#include <schily/find.h>
#include "mkisofs.h"
#include <schily/schily.h>

#ifdef	USE_FIND
/*
 * The callback function for treewalk()
 */
EXPORT int
walkfunc(nm, fs, type, state)
	char		*nm;
	struct stat	*fs;
	int		type;
	struct WALK	*state;
{
	if (type == WALK_NS) {
		errmsg(_("Cannot stat '%s'.\n"), nm);
		state->err = 1;
		return (0);
	} else if (type == WALK_SLN && (state->walkflags & WALK_PHYS) == 0) {
		errmsg(_("Cannot follow symlink '%s'.\n"), nm);
		state->err = 1;
		return (0);
	} else if (type == WALK_DNR) {
		if (state->flags & WALK_WF_NOCHDIR)
			errmsg(_("Cannot chdir to '%s'.\n"), nm);
		else
			errmsg(_("Cannot read '%s'.\n"), nm);
		state->err = 1;
		return (0);
	}
	if (state->maxdepth >= 0 && state->level >= state->maxdepth)
		state->flags |= WALK_WF_PRUNE;
	if (state->mindepth >= 0 && state->level < state->mindepth)
		return (0);

	if (state->tree == NULL ||
	    find_expr(nm, nm + state->base, fs, state, state->tree)) {
		struct directory	*de;
		char			*p;
		char			*xp;
		struct wargs		*wa = state->auxp;

		de = wa->dir;
		/*
		 * Loop down deeper and deeper until we find the
		 * correct insertion spot.
		 * Canonicalize the filename while parsing it.
		 */
		for (xp = &nm[state->auxi]; xp < nm + state->base; ) {
			do {
				while (xp[0] == '.' && xp[1] == '/')
					xp += 2;
				while (xp[0] == '/')
					xp += 1;
				if (xp[0] == '.' && xp[1] == '.' && xp[2] == '/') {
					/*
					 * de == NULL cannot really happen
					 */
					if (de && de != root) {
						de = de->parent;
						xp += 2;
					}
				}
			} while ((xp[0] == '/') || (xp[0] == '.' && xp[1] == '/'));
			p = strchr(xp, PATH_SEPARATOR);
			if (p == NULL)
				break;
			*p = '\0';
			if (debug) {
				error(_("BASE Point:'%s' in '%s : %s' (%s)\n"),
					xp,
					de?de->whole_name:"[null]",
					de?de->de_name:"[null]",
					nm);
			}
			de = find_or_create_directory(de,
				nm,
				NULL, TRUE);
			*p = PATH_SEPARATOR;
			xp = p + 1;
		}

		if (state->base == 0) {
			char	*short_name;
			/*
			 * This is one of the path type arguments to -find
			 */
			short_name = wa->name;
			if (short_name == NULL) {
				short_name = strrchr(nm, PATH_SEPARATOR);
				if (short_name == NULL || short_name < nm) {
					short_name = nm;
				} else {
					short_name++;
					if (*short_name == '\0')
						short_name = nm;
				}
			}
			if (S_ISDIR(fs->st_mode))
				attach_dot_entries(de, fs, fs);
			else
				insert_file_entry(de, nm, short_name, NULL, 0);
		} else {
			int	ret = insert_file_entry(de,
						nm, nm + state->base, fs, 0);

			if (S_ISDIR(fs->st_mode)) {
				int		sret;
				struct stat	parent;

				if (ret == 0) {
					/*
					 * Direcory nesting too deep.
					 */
					state->flags |= WALK_WF_PRUNE;
					return (0);
				}
				sret = stat(".", &parent);
				de = find_or_create_directory(de,
						nm,
						NULL, TRUE);
				attach_dot_entries(de,
						fs,
						sret < 0 ? NULL:&parent);
			}
		}
	}
	return (0);
}
#endif
