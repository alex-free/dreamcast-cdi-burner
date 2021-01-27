#ident @(#)xconfig.mk	1.1 06/10/31 
###########################################################################
# Sample makefile for installing non-localized auxiliary files
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

.SEARCHLIST:	../incs/$(OARCH) ../incs/$(OARCH)
VPATH=		../incs/$(OARCH)

INSDIR=		include/schily/$(OARCH)
TARGET=		xconfig.h
#XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.aux
###########################################################################

PTARGET=	../incs/$(OARCH)/xconfig.h
