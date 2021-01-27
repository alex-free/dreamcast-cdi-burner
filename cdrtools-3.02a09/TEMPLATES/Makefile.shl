#ident @(#)Makefile.shl	1.2 14/04/03 
###########################################################################
# Sample makefile for shared libraries
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
SUBARCHDIR=		/pic
INVERSE_SUBARCHDIR=	../
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		lib
TARGETLIB=	aal
CPPOPTS +=	-DFOKUS
CFILES=		aallib.c
LIBS=
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.shl
###########################################################################
