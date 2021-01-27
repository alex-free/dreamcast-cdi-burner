#!/bin/sh
# @(#)cc-config.sh	1.9 14/03/24 Copyright 2002-2014 J. Schilling
###########################################################################
# Written 2002-2014 by J. Schilling
###########################################################################
# Configuration script called to verify system default C-compiler.
# It tries to fall back to GCC if the system default could not be found.
###########################################################################
# The contents of this file are subject to the terms of the
# Common Development and Distribution License, Version 1.0 only
# (the "License").  You may not use this file except in compliance
# with the License.
#
# See the file CDDL.Schily.txt in this distribution for details.
# A copy of the CDDL is also available via the Internet at
# http://www.opensource.org/licenses/cddl1.txt
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file CDDL.Schily.txt from this distribution.
###########################################################################

#
# Usage:
#	sh ./conf/cc-config.sh cc default-cc incs/Dcc.<platform>
#
if [ $# -lt 3 ]; then
	echo 'Usage: sh ./conf/cc-config.sh [-echo] <cc> <default-cc> incs/Dcc.<platform>'
	echo 'Options:'
	echo '	-echo	Do not write into incs/Dcc.<platform> but echo to stdout'
	echo
	echo 'The "cc"         parameter is the name of the preferred C-compiler'
	echo 'The "default-cc" parameter is the name of the default   C-compiler'
	exit 1
fi

echo=echo
if [ ".$1" = .-echo ]; then
	echo=:
	shift
fi

#
# Try to make sure all error messages are in english.
#
LC_ALL=C
LANG=C

CC=$1
ARG_CC=$1
DEF_CC=$2
PLATFORM_FILE=$3
CC_FOUND=FALSE
${echo} "Trying to find $CC"

#
# Check if we are on SunOS-5.x and /usr/ucb is in PATH before /opt/SUNWspro/bin
# /usr/ucb/cc will not work correctly to compile things on Solaris.
#
# This check will also catch the case where no Sun C-compiler is installed and
# calling cc results in the message:
#	/usr/ucb/cc:  language optional software package not installed
#
xos=`echo "$PLATFORM_FILE" | grep sunos5 `
if [ -n "$xos" ]; then
	#
	# On Solaris, the type builtin is working.
	#
	xcc=`type "$CC" | grep '/usr/ucb/*cc' `
	if [ -n "$xcc" ]; then
		#
		# We did find /usr/ucb/cc
		#
		echo											1>&2
		echo 'Warning:' "$xcc"									1>&2
		echo '         You should not have "/usr/ucb" in your PATH if you like to compile.'	1>&2
		echo '         "/usr/ucb/cc" is not working correclty on Solaris.'			1>&2
		echo '         If you did install a C-compiler in /opt/SUNWspro/bin, abort'		1>&2
		echo '         fix your PATH and start again.'						1>&2
		echo '         Otherwise GCC will be used.'						1>&2
		echo											1>&2
		sleep 60
		CC=do-no-use-ucb-cc
	fi
fi

#
# There are old shells that don't support the 'type' builtin.
# For this reason it is not a simple task to find out whether
# this compiler exists and works.
#
# First try to run the default C-compiler without args
#
eval "$CC > /dev/null 2>&1" 2> /dev/null
if [ $? = 0 ]; then
	CC_FOUND=TRUE
else
	#
	# Now try to run the default C-compiler and check whether it creates
	# any output (usually an error message).
	#
	# This test will fail if the shell does redirect the error message
	# "cc: not found". All shells I tested (except ksh) send this message to
	# the stderr stream the shell itself is attached to and only redirects
	# the output from the command. As there may no output if there is no
	# binary, this proves the existence of the default compiler.
	#
	ccout=`eval "$CC 2>&1" 2>/dev/null`
	ret=$?

	nf=`echo "$ccout" | grep 'not found' `
	if [ $ret = 127 -a -n "$nf" ]; then
		#
		# ksh redirects even the error message from the shell, but we
		# see that there is no executable because the exit code is 127
		# we detect "command not found" if exit code is 127 and
		# the message contains 'not found'
		#
		ccout=""
	fi

	if [ -n "$ccout" ]; then
		CC_FOUND=TRUE
	fi
fi


if [ $CC_FOUND = TRUE ]; then
	${echo} "Found $CC"

	#
	# Call $CC and try to find out whether it might be "gcc" or "clang".
	#
	CC_V=`eval "$CC -v > /dev/null" 2>&1`
	GCC_V=`echo "$CC_V" | grep -i gcc-version `
	CLANG_V=`echo "$CC_V" | grep -i clang `

	if [ ".$GCC_V" != . ]; then
		if eval "gcc -v 2> /dev/null" 2>/dev/null; then
			CC="gcc"
		fi
	elif [ ".$CLANG_V" != . ]; then
		if eval "clang -v 2> /dev/null" 2>/dev/null; then
			CC="clang"
		fi
	fi
	#
	# Check whether "cc" or "gcc" are emulated by another compiler
	#
	if [ ".$ARG_CC" = .cc -o ".$ARG_CC" = .gcc ]; then
		if [ "$CC" != "$ARG_CC" ]; then
			${echo} "$ARG_CC is $CC"
		fi
	fi

	if [  ".$CC" = ".$DEF_CC" ]; then
		${echo} "Creating empty '$PLATFORM_FILE', using $DEF_CC as default compiler"
		if [ ${echo} = echo ]; then
			:> $PLATFORM_FILE
		else
			echo "$DEF_CC"
		fi
	else
		${echo} "Making $CC the default compiler in '$PLATFORM_FILE'"
		if [ ${echo} = echo ]; then
			:> $PLATFORM_FILE
			echo DEFCCOM=$CC > $PLATFORM_FILE
		else
			echo "$CC"
		fi
	fi
	exit
fi

#
# If the current default is gcc or anything != cc, try cc.
# Last resort: try gcc.
#
if [ ".$CC" = ".gcc" -o ".$CC" != ".cc" ]; then
	XCC=cc
	${echo} 'Trying to find cc'
	ccout=`eval "$XCC -c tt.$$.c 2>&1" 2> /dev/null`
	ret=$?
	nf=`echo "$ccout" | grep 'not found' `
	if [ $ret = 127 -a -n "$nf" ]; then
		#
		# ksh redirects even the error message from the shell, but we
		# see that there is no executable because the exit code is 127
		# we detect "command not found" if exit code is 127 and
		# the message contains 'not found'
		#
		ccout=""
	fi
	xos=`echo "$PLATFORM_FILE" | grep sunos5 `
	if [ -n "$xos" ]; then
		xcc=`type "$XCC" | grep '/usr/ucb/*cc' `
		if [ -n "$xcc" -a  -n "$ccout" ]; then
			echo "Cannot use $XCC because $XCC is /usr/ucb/cc"
		fi
		if [ -z "$xcc" -a  -n "$ccout" ]; then
			CC="$XCC"
		fi
	fi
fi
if [ ".$CC" = ".$ARG_CC" ]; then
	XCC=gcc
	${echo} 'Trying to find GCC'
	eval "gcc -v" 2> /dev/null && CC=gcc
fi

#
# Call $CC and try to find out whether it might be "gcc" or "clang".
#
CC_V=`eval "$CC -v > /dev/null" 2>&1`
GCC_V=`echo "$CC_V" | grep -i gcc-version `
CLANG_V=`echo "$CC_V" | grep -i clang `

if [ ".$GCC_V" != . ]; then
	if eval "gcc -v 2> /dev/null" 2>/dev/null; then
		CC="gcc"
	fi
elif [ ".$CLANG_V" != . ]; then
	if eval "clang -v 2> /dev/null" 2>/dev/null; then
		CC="clang"
	fi
fi
#
# Check whether "cc" or "gcc" are emulated by another compiler
#
if [ ".$ARG_CC" = .cc -o ".$ARG_CC" = .gcc ]; then
	if [ "$CC" != "$ARG_CC" ]; then
		${echo} "$ARG_CC is $CC"
	fi
fi

if [ ".$CC" = ".$DEF_CC" ]; then
	${echo} "$XCC not found, keeping current global default"
	${echo} "Creating empty '$PLATFORM_FILE', using $DEF_CC as default compiler"
	if [ ${echo} = echo ]; then
		:> $PLATFORM_FILE
	else
		echo "$DEF_CC"
	fi
else
	${echo} "Found $CC"
	${echo} "Making $CC the default compiler in '$PLATFORM_FILE'"
	if [ ${echo} = echo ]; then
		echo DEFCCOM=$CC > $PLATFORM_FILE
	else
		echo "$CC"
	fi
fi
