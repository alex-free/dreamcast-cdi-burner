#!/bin/sh
# @(#)srcroot.sh  1.3 05/05/01 Copyright 2005 J. Schilling
###########################################################################
# Written 2005 by J. Schilling
###########################################################################
# Find and print SRCROOT
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

SRCROOT=.
loop=1
while [ $loop -lt 100 ]; do
	#echo "$SRCROOT"
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
	echo "Cannot find SRCROOT" 1>&2
	exit 1
fi
echo "$SRCROOT"
