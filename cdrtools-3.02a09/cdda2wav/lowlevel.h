/* @(#)lowlevel.h	1.3 06/05/13 Copyright 1998,1999 Heiko Eissfeldt */
/*
 * os dependent functions
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifndef LOWLEVEL
# define LOWLEVEL 1

# if defined(__linux__)
#  include <linux/version.h>
#  include <linux/major.h>

# endif /* defined __linux__ */

#endif /* ifndef LOWLEVEL */
