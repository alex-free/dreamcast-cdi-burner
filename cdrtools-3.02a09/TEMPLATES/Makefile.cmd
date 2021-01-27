#ident %W% %E% %Q%
###########################################################################
# Sample makefile for general application programs
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		cfform
#CPPOPTS +=	-Ispecincl
CFILES=		cfform.c
LIBS=		-lat
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
