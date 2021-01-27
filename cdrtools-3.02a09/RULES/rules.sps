#ident "@(#)rules.sps	1.15 17/08/04 "
###########################################################################
# Written 2005-2013 by J. Schilling
###########################################################################
#
# Rules for wrapping around other make systems
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
PTARGET=	$(TARGET)
SRCFILE=	$(TARGET)
###########################################################################

_INSMODEI=	$(_UNIQ)$(INSMODE)
__INSMODEI=	$(_INSMODEI:$(_UNIQ)=$(INSMODEF))
INSMODEI=	$(__INSMODEI:$(_UNIQ)%=%)

#all:		$(SRCFILE)

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/sub.htm
include		$(SRCROOT)/$(RULESDIR)/rules.clr
#include		$(SRCROOT)/$(RULESDIR)/rules.ins
include		$(SRCROOT)/$(RULESDIR)/rules.tag
include		$(SRCROOT)/$(RULESDIR)/rules.hlp
include		$(SRCROOT)/$(RULESDIR)/dummy.dep
include		$(SRCROOT)/$(RULESDIR)/rules.tpk
###########################################################################

all: $(ARCHDIR)/config.status $(POSTCONFIG)
	cd $(ARCHDIR)/; "$(MAKE)" $(MAKEMACS) $(MAKEOPTS) $@

install: all
	cd $(ARCHDIR)/; DESTDIR=$(DEST_DIR) "$(MAKE)" $(MAKEMACS) $(MAKEOPTS) DESTDIR=$(DEST_DIR) $@

#
# Hack until the <mach>-<os>-*cc.rul files are updated
#
_HCC_COM=	$(OARCH:%-gcc=gcc)
HCC_COM=	$(_HCC_COM:%-cc=cc)

_XCC_COM=	$(_UNIQ)$(CC_COM)
__XCC_COM=	$(_XCC_COM:$(_UNIQ)=$(HCC_COM))
XCC_COM=	$(__XCC_COM:$(_UNIQ)%=%)

_CONF_SCRIPT=	$(_UNIQ)$(CONF_SCRIPT)
__CONF_SCRIPT=	$(_CONF_SCRIPT:$(_UNIQ)=configure)
CONFSCRIPT=	$(__CONF_SCRIPT:$(_UNIQ)%=%)

#
# Note: $(___CONF_SCR:$(_UNIQ)=:) is not accepted by Sun make
# because of a parser bug. We thus use "true".
#
COLON=:
_CONF_SCR=	$(CONFSCRIPT:none=)
__CONF_SCR=	$(_CONF_SCR:%=../../$(SRC_DIR)/%)
___CONF_SCR=	$(_UNIQ)$(__CONF_SCR)
____CONF_SCR=	$(___CONF_SCR:$(_UNIQ)=true)
PCONFSCRIPT=	$(____CONF_SCR:$(_UNIQ)%=%)

_LNDIR_PRG=	$(_UNIQ)$(LNDIR_PRG)
__LNDIR_PRG=	$(_LNDIR_PRG:$(_UNIQ)=:)
LNDIRPRG=	$(__LNDIR_PRG:$(_UNIQ)%=%)

#$(ARCHDIR)/config.status: $(SRC_DIR)/$(CONFSCRIPT)

			# Expands to: $(SRC_DIR)/$(CONFSCRIPT) is non-empty
$(ARCHDIR)/config.status: $(_CONF_SCR:%=$(SRC_DIR)/%)
	mkdir -p $(ARCHDIR)/; cd $(ARCHDIR)/; \
	$(LNDIRPRG) ../../$(SRC_DIR)/; \
	CC="$(XCC_COM)" CFLAGS="$(CFLAGS)" MAKE="$(MAKE)" $(MAKEMACS) $(PCONFSCRIPT) $(CONF_OPTS) && \
	( [ ! -f config.status ] && touch config.status || : )
