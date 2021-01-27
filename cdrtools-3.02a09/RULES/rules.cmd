#ident @(#)rules.cmd	1.15 16/08/10 
###########################################################################
# Written 1996-2013 by J. Schilling
###########################################################################
#
# Rules for user level commands (usually found in .../bin)
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
# A copy of the CDDL is also available via the Internet at
# http://www.opensource.org/licenses/cddl1.txt
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file CDDL.Schily.txt from this distribution.
###########################################################################
_EXEEXT=	$(EXEEXT)
_XEXEEXT=	$(XEXEEXT)
###########################################################################
INSFLAGS +=	$(STRIPFLAGS)
###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.obj
include		$(SRCROOT)/$(RULESDIR)/rules.dyn
###########################################################################

_INSMODEI=	$(_UNIQ)$(INSMODE)
__INSMODEI=	$(_INSMODEI:$(_UNIQ)=$(INSMODEX))
INSMODEI=	$(__INSMODEI:$(_UNIQ)%=%)

__LD_OUTPUT_OPTION=	$(_UNIQ)$(LD_OUTPUT_OPTION)
___LD_OUTPUT_OPTION=	$(__LD_OUTPUT_OPTION:$(_UNIQ)=-o $@)
_LD_OUTPUT_OPTION=	$(___LD_OUTPUT_OPTION:$(_UNIQ)%=%)

LIBS_PATH += $(LIBS_PATH_STATIC)

all:		$(PTARGET)

$(PTARGET):	$(OFILES) $(SRCLIBS)
		$(LDCC) $(_LD_OUTPUT_OPTION) $(POFILES) $(LDFLAGS) $(LDLIBS)

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/sub.htm
include		$(SRCROOT)/$(RULESDIR)/rules.lnt
include		$(SRCROOT)/$(RULESDIR)/rules.clr
include		$(SRCROOT)/$(RULESDIR)/rules.ins
include		$(SRCROOT)/$(RULESDIR)/rules.tag
include		$(SRCROOT)/$(RULESDIR)/rules.hlp
include		$(SRCROOT)/$(RULESDIR)/rules.dep
include		$(SRCROOT)/$(RULESDIR)/rules.cst
###########################################################################
