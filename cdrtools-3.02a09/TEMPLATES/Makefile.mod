#ident %W% %E% %Q%
###########################################################################
# Sample makefile for loadable streams modules
###########################################################################
SRCROOT=	../../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		usr/kernel/strmod
TARGET=		ga
CPPOPTS +=	-DFOKUS -DATMNA -DBROKEN_PROM -DTPI_COMPAT \
		-DGADEBUG -DGADEBUG_MASK=0x0 
CFILES=		ga.c
SRCLIBS=
LIBS=		
XMK_FILE=	Makefile.man

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.mod
###########################################################################
