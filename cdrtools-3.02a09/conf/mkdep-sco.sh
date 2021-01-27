#!/bin/sh
#ident "@(#)mkdep-sco.sh	1.5 03/03/05 "
###########################################################################
# Copyright 1999 by J. Schilling
###########################################################################
#
# Create dependency list with SCO's cc
#
###########################################################################
#
# This script will probably not work correctly with a list of C-files
# but as we don't need it with 'smake' or 'gmake' it seems to be sufficient.
#
###########################################################################
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only.
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
	*.c)	if [ ! -z "$FILES" ]; then
			FILES="$FILES "
		fi
		# base name from $i
		base=`echo $i | sed -e 's;[^/]*/;;'`
		FILES="$FILES$base"
		;;
	esac
done

OFILES=`echo "$FILES" | sed -e 's;\([^.]*\)\.c;\1.o;g'`

cc -H -E 2>&1 > /dev/null "$@" | grep -hv '^"' | grep -hv '^UX:' | sed -e "s;^;$OFILES: ;"
