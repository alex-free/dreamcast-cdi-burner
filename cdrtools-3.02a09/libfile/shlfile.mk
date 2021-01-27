#ident @(#)shlfile.mk	1.3 07/02/08 
###########################################################################
# Sample makefile for non-shared libraries
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/pic
INSDIR=		lib
TARGETLIB=	file
#CPPOPTS +=	-DFOKUS
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		file.c apprentice.c softmagic.c 
LIBS=		-lschily -lc
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.shl
###########################################################################
