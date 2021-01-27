#!/bin/sh
#ident "@(#)mkdep-msc-interix.sh	1.4 07/04/24 "
###########################################################################
# Copyright 1999,2006-2007 by J. Schilling
###########################################################################
#
# Create dependency list with Microsoft's cl from Interix
#
###########################################################################
#
# This script will probably not work correctly with a list of C-files
# but as we don't need it with 'smake' or 'gmake' it seems to be sufficient.
#
###########################################################################
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only
# (the "License").  You may not use this file except in compliance
# with the License.
#
# See the file CDDL.Schily.txt in this distribution for details.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file CDDL.Schily.txt from this distribution.
###########################################################################
FILES=

for i in "$@"; do

	case "$i" in

	-*)	# ignore options
		;;

	*.c | *.C | *.cc | *.cxx | *.cpp)

		if [ ! -z "$FILES" ]; then
			FILES="$FILES "
		fi
		# base name from $i
		base=`echo $i | sed -e 's;[^/]*/;;'`
		FILES="$FILES$base"
		;;
	esac
done

OFILES=`echo "$FILES" | sed -e 's;\([^.]*\)\.[cC]$;\1.obj;g' -e 's;\([^.]*\)\.cc$;\1.obj;g' -e 's;\([^.]*\)\.c..$;\1.obj;g' `

echo ".SPACE_IN_NAMES: true"
cl.exe -E -nologo 2> /dev/null "$@" | grep '^\#line' | cut -d\" -f2 | sort -u | \
	sed '
s,^a:,/dev/fs/A,
s,^b:,/dev/fs/B,
s,^c:,/dev/fs/C,
s,^d:,/dev/fs/D,
s,^e:,/dev/fs/E,
s,^f:,/dev/fs/F,
s,^g:,/dev/fs/G,
s,^h:,/dev/fs/H,
s,^i:,/dev/fs/I,
s,^j:,/dev/fs/J,
s,^k:,/dev/fs/K,
s,^l:,/dev/fs/L,
s,^m:,/dev/fs/M,
s,^n:,/dev/fs/N,
s,^o:,/dev/fs/O,
s,^p:,/dev/fs/P,
s,^q:,/dev/fs/Q,
s,^r:,/dev/fs/R,
s,^s:,/dev/fs/S,
s,^t:,/dev/fs/T,
s,^u:,/dev/fs/U,
s,^v:,/dev/fs/V,
s,^w:,/dev/fs/W,
s,^x:,/dev/fs/X,
s,^y:,/dev/fs/Y,
s,^z:,/dev/fs/Z,
s,\([A-Z]\):,/dev/fs/\1,
s,\\\\,/,g
s/\([^\]\) /\1\\ /g' | sed -e "s;^;$OFILES: ;"
echo ".SPACE_IN_NAMES:"
