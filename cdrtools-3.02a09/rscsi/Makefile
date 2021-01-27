#ident @(#)Makefile	1.7 08/08/01 
###########################################################################
#include		$(MAKE_M_ARCH).def
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		sbin
TARGET=		rscsi
INSMODE=	4711
INSUSR=		root
#CPPOPTS +=	-DUSE_REMOTE
CPPOPTS +=	-I../libscg

CFILES=		rscsi.c
#HFILES=
LIBS=		-lscg $(LIB_VOLMGT) -ldeflt -lschily $(SCSILIB) $(LIB_SOCKET)
XMK_FILE=	Makefile.man Makefile.dfl Makefile.doc

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
