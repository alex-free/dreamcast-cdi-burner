#ident @(#)shlschily.mk	1.8 12/08/14 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

SUBARCHDIR=	/pic
.SEARCHLIST:	. $(ARCHDIR) stdio $(ARCHDIR)
VPATH=		.:stdio:$(ARCHDIR)
INSDIR=		lib
TARGETLIB=	schily
CPPOPTS +=	-Istdio
CPPOPTS +=	-DUSE_SCANSTACK	# Try to scan stack frames
CPPOPTS +=	-DPORT_ONLY	# Add missing funcs line snprintf for porting
include		Targets
LIBS=		$(LIB_INTL) -lc	# Now needs gettext()

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.shl
###########################################################################
