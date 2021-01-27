#ident %W% %E% %Q%
###########################################################################
# Sample makefile for non-shared libraries
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		lib
TARGETLIB=	aal
CPPOPTS +=	-DFOKUS
CFILES=		aallib.c
LIBS=
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
