#ident @(#)rules.mod	1.7 07/05/06 
###########################################################################
# Written 1996 by J. Schilling
###########################################################################
#
# Rules for loadable streams modules
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
include		$(SRCROOT)/$(RULESDIR)/rules.obj
###########################################################################

_INSMODEI=	$(_UNIQ)$(INSMODE)
__INSMODEI=	$(_INSMODEI:$(_UNIQ)=$(INSMODEX))
INSMODEI=	$(__INSMODEI:$(_UNIQ)%=%)

all:		$(PTARGET)

$(PTARGET):	$(OFILES) $(SRCLIBS)
		$(LD) -r -o $@ $(OFILES) $(SRCLIBS) $(LIBS)

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/sub.htm
include		$(SRCROOT)/$(RULESDIR)/rules.clr
include		$(SRCROOT)/$(RULESDIR)/rules.ins
include		$(SRCROOT)/$(RULESDIR)/rules.tag
include		$(SRCROOT)/$(RULESDIR)/rules.hlp
include		$(SRCROOT)/$(RULESDIR)/rules.dep
###########################################################################
