#!/bin/sh
#ident "@(#)mkdep-hpux.sh	1.2 11/11/12 "
###########################################################################
# Copyright 2002 by J. Schilling
###########################################################################
#
# Create dependency list with the HP-UX bundled cc
#
###########################################################################
#
# This script will probably not work correctly with a list of C-files
# but as we don't need it with 'smake' or 'gmake' it seems to be sufficient.
#
# Due to a design bug in the compiler interface we cannot tell the compiler
# to output the list on stdout, -Wp,-M- does not work. As the default
# output is on stderr and the output is even completely confused with
# error messages and warnings, we cannot just grep away the warnings
# like we do on SCO unix.
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

trap 'rm -fr /tmp/cpp-m.$$ ; exit 1' 1 2 15

cc -Wp,-M/tmp/cpp-m.$$ -E > /dev/null "$@" 

#
# The HP compiler creates xxx/file.o: xxx/file.c in case that
# file.c is in a sub.directory. This is wrong. The following sed
# command removes "^xxx/" from the output
#
sed -e 's;^[^/ ]*/\(.*\)\.o;\1.o;' < /tmp/cpp-m.$$

rm -f /tmp/cpp-m.$$
