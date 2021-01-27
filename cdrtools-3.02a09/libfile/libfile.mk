#ident @(#)libfile.mk	1.2 07/02/04 
###########################################################################
# Sample makefile for non-shared libraries
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		lib
TARGETLIB=	file
#CPPOPTS +=	-DFOKUS
CPPOPTS +=	-DSCHILY_PRINT

CFILES=		file.c apprentice.c softmagic.c 
LIBS=
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
