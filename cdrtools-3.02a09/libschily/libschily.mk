#ident @(#)libschily.mk	1.10 16/11/06 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
TARGETLIB=	schily
CPPOPTS +=	-Istdio
CPPOPTS +=	-DUSE_SCANSTACK	# Try to scan stack frames
CPPOPTS +=	-DPORT_ONLY	# Add missing funcs line snprintf for porting
include		Targets
include		Targets.man
LIBS=		

XMK_FILE=	Makefile.man
###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.lib
###########################################################################
