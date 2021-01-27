#!/bin/sh
# @(#)oarch.sh  1.17 17/07/08 Copyright 2005-2016 J. Schilling
###########################################################################
# Written 2005 by J. Schilling
###########################################################################
# A simulation of $(OARCH) from the Schily Makefile system
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
# Processor	P_ARCH	MAKE_ARCH	Instruction Set Architecture or Processor type
# Kernel	K_ARCH	MAKE_MACH	Machine hardware class
# Machine	M_ARCH	MAKE_M_ARCH	ISA or Processor type
#
# P	mach	== * uname -p			(i386/mc68020/sparc/powerpc
# K	arch -k ==   uname -m			(i86pc/sun3x/sun4u/power-macintosh
# M	arch	== * uname -p + map sparc->sun4	(i86pc/sun3/sun4/ppc
#
#	* Not available on vanilla SVr4
#
# If P_ARCH or M_ARCH are not set, K_ARCH us used.
#
# P_ARCH -> OARCH (sparc-sunos5-cc) used as default OBJ arch subdir
# K_ARCH -> XARCH (sun4u-sunos5-cc) used as OBJ arch subdir for kernel modules

dbgflag=FALSE
xflag=FALSE
cflag=FALSE
oflag=FALSE
pflag=FALSE
if [ .$1 = .-help ]; then
	echo "Usage: oarch.sh [options]"
	echo "Options:"
	echo "	-debug	Print debugging information"
	echo "	-c	Print C_ARCH instead of OARCH"
	echo "	-o	Print O_ARCH instead of OARCH"
	echo "	-p	Print PARCH instead of OARCH"
	echo "	-x	Print XARCH instead of OARCH"
	exit
fi
if [ .$1 = .-debug ]; then
	dbgflag=TRUE
fi
if [ .$1 = .-x ]; then
	xflag=TRUE
fi
if [ .$1 = .-c ]; then
	cflag=TRUE
fi
if [ .$1 = .-o ]; then
	oflag=TRUE
fi
if [ .$1 = .-p ]; then
	pflag=TRUE
fi

#
# '((' does not work because of a parser bug in ksh on Interix.
# This is why we use '( (' here.
#
MACHCMD="( (mach || uname -p || true)		2> /dev/null)"
ARCHCMD="( (arch || /usr/ucb/arch || true)	2> /dev/null)"

XP_ARCH=`eval ${MACHCMD} | tr '[A-Z]' '[a-z]' | tr 'ABCDEFGHIJKLMNOPQRSTUVWXYZ, /\\()"' 'abcdefghijklmnopqrstuvwxyz,//////' | tr ',/' ',\-'`
XK_ARCH=`uname -m        | tr '[A-Z]' '[a-z]' | tr 'ABCDEFGHIJKLMNOPQRSTUVWXYZ, /\\()"' 'abcdefghijklmnopqrstuvwxyz,//////' | tr ',/' ',\-'`
XM_ARCH=`eval ${ARCHCMD} | tr '[A-Z]' '[a-z]' | tr 'ABCDEFGHIJKLMNOPQRSTUVWXYZ, /\\()"' 'abcdefghijklmnopqrstuvwxyz,//////' | tr ',/' ',\-'`

#echo XP_ARCH $XP_ARCH
#echo XK_ARCH $XK_ARCH
#echo XM_ARCH $XM_ARCH

P_ARCH="${XP_ARCH}"
K_ARCH="${XK_ARCH}"
M_ARCH="${XM_ARCH}"

if [ ."$XP_ARCH" = .unknown ]; then
	P_ARCH="${K_ARCH}"
fi
if [ ."$XP_ARCH" = . ]; then
	P_ARCH="${K_ARCH}"
fi

if [ ."${XM_ARCH}" = . ]; then
	M_ARCH="${K_ARCH}"
fi

OSNAME=`uname -s | tr 'ABCDEFGHIJKLMNOPQRSTUVWXYZ, /\\()"' 'abcdefghijklmnopqrstuvwxyz,//////' | tr ',/' ',\-'`
OSHOST=`uname -n | tr 'ABCDEFGHIJKLMNOPQRSTUVWXYZ, /\\()"' 'abcdefghijklmnopqrstuvwxyz,//////' | tr ',/' ',\-'`
OSREL=`uname -r | tr 'ABCDEFGHIJKLMNOPQRSTUVWXYZ, /\\()"' 'abcdefghijklmnopqrstuvwxyz,//////' | tr ',/' ',\-'`
OSVERS=`uname -v | tr 'ABCDEFGHIJKLMNOPQRSTUVWXYZ, /\\()"' 'abcdefghijklmnopqrstuvwxyz,//////' | tr ',/' ',\-'`

O_ARCH="$OSNAME"

case "$OSNAME" in
	cygwin*) O_ARCH=cygwin32_nt;;
esac

if [ ."$OSNAME" = .aix ]; then
	P_ARCH=rs6000
fi
if [ ."$OSNAME" = .bsd-os ]; then
	case "$OSREL" in
		3.*)	O_ARCH=bsd-os3;;
		4.*)	O_ARCH=bsd-os;;
	esac
fi
if [ ."$OSNAME" = .bsd-os ]; then
	case "$OSREL" in
		3.*)	O_ARCH=bsd-os3;;
		4.*)	O_ARCH=bsd-os;;
	esac
fi
if [ ."$OSNAME" = .dgux ]; then
	case "$OSREL" in
		r4.*)	O_ARCH=dgux4;;
		5.4r3*)	O_ARCH=dgux3;;
	esac
fi
if [ ."$OSNAME" = .irix64 ]; then
	O_ARCH=irix
fi
if [ ."$OSNAME" = .mac-os ]; then
	case "$OSREL" in
		9.*)	O_ARCH=mac-os9;;
		10.*)	O_ARCH=mac-os10;;
	esac
fi
case "$OSREL" in
	mingw*) O_ARCH= mingw32_nt;;
esac
if [ ."$OSNAME" = .newsos ]; then
	case "$OSREL" in
		5.*)	O_ARCH=newsos5;;
		6.*)	O_ARCH=newsos6;;
	esac
fi
if [ ."$OSNAME" = .openunix ]; then
	O_ARCH=unixware
fi
if [ ."$OSNAME" = .os-2 ]; then
	O_ARCH=os2
fi
if [ ."$OSNAME" = .sco_sv ]; then
	O_ARCH=openserver
fi
if [ ."$OSNAME" = .sunos ]; then
	case "$OSREL" in
		4.*)	O_ARCH=sunos4;;
		5.*)	O_ARCH=sunos5;;
	esac
fi
if [ ."$OSNAME" = .unix_sv ]; then
	O_ARCH=unixware
fi

###########################################################################
#
# Interix: Avoid to have something like "Intel_x86_Family15_Model4_Stepping1"
# in $(OARCH), we rather use $(K_ARCH) which is e.g. "x86".
# XXX If this is changed, both files in DEFAULTS/ and DEFAULTS_ENG as
# XXX well as the file conf/oarch.sh need to be changed.
#
###########################################################################
if [ ."$OSNAME" = .interix ]; then
	P_ARCH=${K_ARCH}
fi

MARCH="$M_ARCH"
if [ ."$ARCH" != . ]; then
	MARCH="$ARCH"
fi

PARCH="$P_ARCH"
if [ ."$ARCH" != . ]; then
	PARCH="$ARCH"
fi

KARCH="$K_ARCH"
if [ ."$ARCH" != . ]; then
	KARCH="$ARCH"
fi

confdir=
if [ -z "$confdir" ]; then
	#
	# Try the directory containing this script.
	#
	prog=$0 
	confdir=`echo $prog|sed 's,/[^/][^/]*$,,'` 
	test ".$confdir" = ".$prog" && confdir=. 
	confdir=$confdir
fi 

CC="`grep '^DEFCCOM' $confdir/../incs/Dcc.${PARCH}-${O_ARCH} 2>/dev/null | sed -e 's,^DEFCCOM=[      ]*,,'`"
if [ ".$CC" = . ]; then
	CC="`grep '^DEFCCOM' $confdir/../DEFAULTS/Defaults.${O_ARCH} 2>/dev/null | sed -e 's,^DEFCCOM=[ 	]*,,'`"
fi
if [ ".$CC" = . ]; then
	CC=cc
fi

C_ARCH="`sh $confdir/../conf/cc-config.sh -echo $CC $CC ${O_ARCH}`"

OARCH="${PARCH}-${O_ARCH}-${C_ARCH}"
XARCH="${KARCH}-${O_ARCH}-${C_ARCH}"

if [ $dbgflag = TRUE ]; then
#echo xx"$P_ARCH"
	echo C_ARCH "	$C_ARCH"
	echo P_ARCH "	$P_ARCH"
	echo K_ARCH "	$K_ARCH"
	echo M_ARCH "	$M_ARCH"
	echo OSNAME "	$OSNAME"
	echo OSHOST "	$OSHOST"
	echo OSREL  "	$OSREL"
	echo OSVERS "	$OSVERS"
	echo OARCH  "	$OARCH"
	echo XARCH  "	$XARCH"
	exit
fi
if [ $cflag = TRUE ]; then
	echo "${C_ARCH}"
	exit
fi
if [ $oflag = TRUE ]; then
	echo "${O_ARCH}"
	exit
fi
if [ $pflag = TRUE ]; then
	echo "${PARCH}"
	exit
fi
if [ $xflag = TRUE ]; then
	echo "${XARCH}"
	exit
fi
echo "$OARCH"
