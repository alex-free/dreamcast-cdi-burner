#ident @(#)Makefile	1.6 07/10/24 
###########################################################################
# Written 1998 by J. Schilling
###########################################################################
# Makefile for local dynamic configuration
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
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

CONFFILES=	Inull rules.cnf xconfig.h

PCONFFILES=	$(CONFFILES:%=$(OINCSDIR)/%)

_INSMODEI=	$(_UNIQ)$(INSMODE)
__INSMODEI=	$(_INSMODEI:$(_UNIQ)=$(INSMODEF))
INSMODEI=	$(__INSMODEI:$(_UNIQ)%=%)

INSDIR=		include/schily/$(OARCH)
TARGET=		xconfig.h
PTARGET=	../incs/$(OARCH)/xconfig.h

all: config

clobber:
	$(RM_F) $(PCONFFILES)

distclean:
		$(RM) $(RM_RF)		$(PCONFFILES) \
						.vedtmp* *.bak OBJ/* \
						$(SRCROOT)/libs/* $(SRCROOT)/incs/*

XMK_FILE=	xconfig.mk

###########################################################################
# Now done in rules.top
#include		$(SRCROOT)/$(RULESDIR)/rules.cnf
include		$(SRCROOT)/$(RULESDIR)/rules.ins
###########################################################################

OTHERTARGETS=	clean ibins depend rmdep \
		TAGS tags rmtarget relink

$(OTHERTARGETS):
	@echo "$@: nothing to make"
