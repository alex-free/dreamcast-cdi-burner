#!/bin/sh
# @(#)setup.sh  1.18 14/12/09 Copyright 2005 J. Schilling
###########################################################################
# Written 2005 by J. Schilling
###########################################################################
# Set up bins/<archdir>/{bzip2}!{gzip}!{patch}!{smake}!{star}!{wget}
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

# Benoetigte Kommandos:
#
#	makefiles	http://sourceforge.net/projects/schilytools/files/makefiles/
#	makefiles	http://sourceforge.net/projects/schilytools/files/makefiles/testing/makefiles.tar
#	smake		http://sourceforge.net/projects/s-make/files/alpha/
#	star		http://sourceforge.net/projects/s-tar/files/alpha/
#	gzip		ftp://wuarchive.wustl.edu/mirrors/gnu/gzip/gzip-1.2.4.tar
#	wget		ftp://ftp.gnu.org/gnu/wget/wget-1.9.1.tar.gz
#	wget		http://sourceforge.net/projects/schilytools/files/makefiles/testing/wget-1.9.1-2.tar
#	gzip		http://www.gzip.org/gzip-1.3.3.tar.gz
#	gzip		http://sourceforge.net/projects/schilytools/files/makefiles/testing/gzip-1.3.3.tar.gz
#	bzip2		ftp://sources.redhat.com/pub/bzip2/v102/bzip2-1.0.2.tar.gz
#	bzip2		http://www.bzip.org/1.0.3/bzip2-1.0.3.tar.gz
#	bzip2		http://sourceforge.net/projects/schilytools/files/makefiles/testing/bzip2-1.0.3.tar.gz
#	patch		ftp://ftp.gnu.org/pub/gnu/patch/patch-2.5.4.tar.gz
#	patch		http://sourceforge.net/projects/schilytools/files/makefiles/testing/patch-2.5.4-1.tar.bz2

trap 'if [ -f ./$xfile ]; then rm -f ./$xfile; fi; rm -rf /tmp/i.$$/; exit 1' 1 2 15

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

cd "$SRCROOT"

if [ ! -r RULES/rules.top ]; then
	echo "Cannot chdir to SRCROOT" 1>&2
	exit 1
fi

OARCH=`conf/oarch.sh`
BINS=bins/"$OARCH"
PWD=`pwd`
mkdir -p "$BINS"
PATH="${PWD}/${BINS}:${PATH}"
export PATH
MAKE="make"

if [	-r ${BINS}/bzip2 -a	\
	-r ${BINS}/gzip -a	\
	-r ${BINS}/lndir -a	\
	-r ${BINS}/patch -a	\
	-r ${BINS}/smake -a	\
	-r ${BINS}/star -a	\
	-r ${BINS}/wget		]; then
	echo "Setup for $OARCH already done." 1>&2
	exit
fi

CC="${CC-`conf/oarch.sh -c`}"

ZIP_SUFFIX=
ZIP=cat
UNZIP=cat

if bzip2 --help > /dev/null 2>&1; then
	UNZIP="bzip2 -d"
	ZIP=bzip2
	ZIP_SUFFIX=.bz2
elif gzip --help > /dev/null 2>&1; then
	UNZIP="gzip -d"
	ZIP=gzip
	ZIP_SUFFIX=.gz
fi

WGET="${WGET-sh conf/wget.sh}"

do_wget() {
	URL="$1"
	while true; do
		xfile=`echo "$URL" | sed -e 's,.*/,,'`
		$WGET "$URL"

		if [ -f "$xfile" ]; then
			break
		fi
		echo
		echo Retrying in 10 seconds
		sleep 10
	done
	xfile=
}

do_wget ftp://ftp.berlios.de/pub/smake/testing/smake.tar${ZIP_SUFFIX}
${UNZIP} < smake.tar${ZIP_SUFFIX} | tar xf -
#
# tar on HP-UX-10.x does not like tar xpf
#
cd smake-*/psmake/.
sh ./MAKE-all
cd ..
psmake/smake -r DESTDIR=/tmp/i.$$/ COPTX=-DDEFAULTS_PATH_SEARCH_FIRST install
cd ..
echo "Configuring Makefile system"
cd conf
/tmp/i.$$/opt/schily/bin/smake -r
cd ..
cd inc
/tmp/i.$$/opt/schily/bin/smake -r
cd ..
strip /tmp/i.$$/opt/schily/bin/smake
rm -f "$BINS"/smake
cp -p /tmp/i.$$/opt/schily/bin/smake "$BINS"
mkdir -p "$BINS"/lib
rm -f "$BINS"/lib/defaults.smk
cp -p /tmp/i.$$/opt/schily/lib/defaults.smk "$BINS"/lib
rm -rf smake-* smake.tar*
MAKE="${PWD}/${BINS}/smake"

if [ $ZIP = cat ]; then
	#do_wget ftp://wuarchive.wustl.edu/mirrors/gnu/gzip/gzip-1.2.4.tar
	do_wget ftp://ftp.berlios.de/pub/makefiles/testing/gzip-1.2.4.tar
	tar xf gzip-1.2.4.tar
	#
	# tar on HP-UX-10.x does not like tar xpf
	#
	cd gzip-1.2.4
	CC="$CC" MAKE="$MAKE" CFLAGS=-O ./configure
	smake
	cd ..
	strip gzip-1.2.4/gzip
	rm -f "${BINS}"/gzip
	cp -p gzip-1.2.4/gzip "${BINS}"
	rm -rf gzip-1.2.4*
	UNZIP="gzip -d"
	ZIP=gzip
	ZIP_SUFFIX=.gz
fi

do_wget ftp://ftp.berlios.de/pub/star/testing/star.tar${ZIP_SUFFIX}
${UNZIP} < star.tar${ZIP_SUFFIX} | tar xf -
#
# tar on HP-UX-10.x does not like tar xpf
#
cd star-*/.
smake DESTDIR=/tmp/i.$$/ install
cd ..
strip /tmp/i.$$/opt/schily/bin/star
rm -f "${BINS}"/star
cp -p /tmp/i.$$/opt/schily/bin/star "$BINS"
rm -rf star-* star.tar*

do_compile() {
	url="$1"
	dir="$2"
	binpath="$3"
	fname=`echo "$url" | sed -e 's,.*/,,'`
	do_wget "$url"
	star xpf "$fname"
	cd "$dir"
	if [ -r ./configure ]; then
		CC="$CC" MAKE="$MAKE" CFLAGS=-O ./configure
	elif [ -r ./Configure ]; then
		CC="$CC" MAKE="$MAKE" CFLAGS=-O ./Configure
	fi
	smake CC="$CC" CFLAGS=-O
	cd ..
	if [ ! -r "$binpath" ]; then
		echo "Cound not make $binpath"	1>&2
	fi
	fname=`echo "$binpath" | sed -e 's,.*/,,'`
	strip "$binpath"
	rm -f "${BINS}"/"$fname"
	cp -p "$binpath" "${BINS}"
	rm -rf "$dir"*
}

do_install() {
	url="$1"
	dir="$2"

	fname=`echo "$url" | sed -e 's,.*/,,'`
	do_wget "$url"
	star xpf "$fname"
	cd "$dir"
	smake
	smake ibins
	cd ..
	rm -rf "$dir"*
}

do_compile 	ftp://ftp.berlios.de/pub/makefiles/testing/wget-1.9.1-2.tar${ZIP_SUFFIX} \
		wget-1.9.1 \
		wget-1.9.1/src/wget

WGET="${BINS}/wget --passive-ftp"

do_compile	ftp://ftp.berlios.de/pub/makefiles/testing/gzip-1.3.3.tar.gz \
		gzip-1.3.3 \
		gzip-1.3.3/gzip

do_compile	ftp://ftp.berlios.de/pub/makefiles/testing/bzip2-1.0.3.tar.gz \
		bzip2-1.0.3 \
		bzip2-1.0.3/bzip2

do_compile	ftp://ftp.berlios.de/pub/makefiles/testing/patch-2.5.4-1.tar.bz2 \
		patch-2.5.4 \
		patch-2.5.4/patch
ln -s patch ${BINS}/gpatch

do_install	ftp://ftp.berlios.de/pub/makefiles/testing/lndir.tar.bz2 \
		lndir

rm -rf /tmp/i.$$/
