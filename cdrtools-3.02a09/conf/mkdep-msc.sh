#!/bin/sh
#ident "@(#)mkdep-msc.sh	1.3 11/08/11 "
###########################################################################
# Copyright 1999,2006 by J. Schilling
###########################################################################
#
# Create dependency list with Microsoft's cl
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
cl -E -nologo 2> /dev/null "$@"  | grep '\#line' | sed -e 's,^.*\#line[ \t]*[^ ]*[ \t]*",,' \
							-e 's,"$,,' -e 's/\([^\]\) /\1\\ /g' \
							-e  's,\.\\\\*,.\\,g' \
							-e "s;^;$OFILES: ;" | sort -u
echo ".SPACE_IN_NAMES:"
