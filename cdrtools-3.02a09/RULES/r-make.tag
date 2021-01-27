#ident "@(#)r-make.tag	1.3 07/12/01 "
###########################################################################
# Written 1996 by J. Schilling
###########################################################################
#
# Tag building rules for make
#
###########################################################################
# Copyright (c) J. Schilling
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
TAGS:		$(CFILES) $(CFILES_TAG) $(HFILES) $(HFILES_TAG)
		$(ETAGS) $(CFILES) $(CFILES_TAG) $(HFILES) $(HFILES_TAG)

tags:		$(CFILES) $(CFILES_TAG) $(HFILES) $(HFILES_TAG)
		$(CTAGS) -t $(CFILES) $(CFILES_TAG) $(HFILES) $(HFILES_TAG)
