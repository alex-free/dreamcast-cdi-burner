#!/bin/sh
# @(#)mkdir.sh	1.5 09/12/22 Copyright 1998 J. Schilling
###########################################################################
# Written 1998-2005 by J. Schilling
###########################################################################
#
# Schily Makefile slottable source high level mkdir -p command
# Creates TARGETS/ and needed content
#
# Replacement for mkdir -p ... functionality.
#
# mkdir(1) on BSD-4.3, NeXT Step <= 3.x and similar don't accept the -p flag
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

dop=false
doT=false
ok=true
while test $# -ne 0; do
	case $1 in
	-p)
		dop=true
		shift
		;;
	-T)
		doT=true
		shift
		;;
	-*)
		echo "Unknown Option '$1'"	1>&2
		ok=false
		break
		;;
	*)
		break
		;;
	esac
done

if [ $doT = true -a $dop = false ]; then
	echo "Option -T needs -p in addition"      1>&2
	ok=false
fi

if [ $# -lt 1 ]; then
	echo "Missing <dir> parameter"      1>&2
fi
if [ $# -lt 1 -o $ok = false ]; then
	echo "Usage: $0 [-p] [-T] dir ...."
	echo "Options:"
	echo "	-p	Create <dir> and including all non existing parent directories"
	echo "	-T	Add Schily Makefile directory template files"
	exit 1
fi

if [ $doT = true -a ."$SRCROOT" = . ]; then
	SRCROOT=.
	loop=1
	while [ $loop -lt 100 ]; do
		if [ ! -d $SRCROOT ]; then
			# Abort on ENAMETOOLONG
			break
		fi
		if [ -r $SRCROOT/RULES/rules.top ]; then
			break
		fi
		if [ "$SRCROOT" = . ]; then
			SRCROOT=".."
		else
			SRCROOT="$SRCROOT/.."
		fi
		loop="`expr $loop + 1`"
	done
	if [ ! -r $SRCROOT/RULES/rules.top ]; then
		echo "Cannot find SRCROOT directory" 1>&2
		exit 1
	fi
	export SRCROOT
fi

if [ $dop = false ]; then
	for i in "$@"; do
		mkdir "$i"
	done
else
	for i in "$@"; do
		# Don't use ${i:-.}, BSD4.3 doen't like it
		#
		fullname="$i"
		if [ ."$i" = . ]; then
			fullname=.
		fi
		if [ "$fullname" = . ]; then
			continue
		fi
		dirname=`expr \
			"$fullname/" : '\(/\)/*[^/]*//*$'  \| \
			"$fullname/" : '\(.*[^/]\)//*[^/][^/]*//*$' \| \
			.`

		if [ $doT = false ]; then 
			sh $0 -p "$dirname"
			if [ ! -d "$i" ]; then
				mkdir "$i"
			fi
			continue
		else
			sh $0 -p -T "$dirname"
		fi

		if [ ! -d "$dirname"/TARGETS ]; then
			mkdir "$dirname"/TARGETS
		fi
		if [ ! -r "$dirname"/TARGETS/__slot ]; then
			echo 'This file enables the "slot" feature of the Schily SING makefile system' > "$dirname"/TARGETS/__slot
		fi
		dirbase=`echo "$i" | sed -e 's,/*$,,;s,.*/\([^/]*\),\1,'`
		if [ ! -r "$dirname"/TARGETS/55"$dirbase" ]; then
			: > "$dirname"/TARGETS/55"$dirbase"
		fi
		if [ ! -r "$dirname"/Makefile ]; then
			srcroot=`echo "$dirname" | sed -e 's,[^/][^/]*,..,g'`
			srcroot=`echo "$SRCROOT/$srcroot" | sed -e 's,^\./,,'`

			echo '#ident %'W'% %'E'% %'Q'%'								 > "$dirname"/Makefile
			echo '###########################################################################'	>> "$dirname"/Makefile
			echo '# Sample makefile for sub directory makes'					>> "$dirname"/Makefile
			echo '###########################################################################'	>> "$dirname"/Makefile
			echo "SRCROOT=	$srcroot"								>> "$dirname"/Makefile
			echo 'RULESDIR=	RULES'									>> "$dirname"/Makefile
			echo 'include		$(SRCROOT)/$(RULESDIR)/rules.top'				>> "$dirname"/Makefile
			echo '###########################################################################'	>> "$dirname"/Makefile
			echo											>> "$dirname"/Makefile
			echo '###########################################################################'	>> "$dirname"/Makefile
			echo 'include		$(SRCROOT)/$(RULESDIR)/rules.dir'				>> "$dirname"/Makefile
			echo '###########################################################################'	>> "$dirname"/Makefile
		fi

		if [ ! -d "$i" ]; then
			mkdir "$i"
		fi
	done
fi
