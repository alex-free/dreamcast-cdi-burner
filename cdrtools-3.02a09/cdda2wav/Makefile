#ident @(#)Makefile	1.29 13/04/21 
###########################################################################
SRCROOT=	..
RULESDIR=	RULES
LCONFIG=	local.cnf
LCONFIG_DEP=	install-sh

###########################################################################
all:
all clean depend config install: install-sh scsi_scan.c scsi_cdr.c priv.c

install-sh scsi_scan.c scsi_cdr.c cd_misc.c priv.c:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS
###########################################################################

include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

INSDIR=		bin
TARGET=		cdda2wav
INSMODE=	4711
INSUSR=		root
CPPOPTS +=	-I../libcdrdeflt
CPPOPTS +=	-DFIFO -I../libscg -I../libparanoia -I../cdrecord $(CDDA2WAVDEFS)
CPPOPTS +=	-DCDDA2WAV	# Signal that we compile cdda2wav
CPPOPTS +=	-DSCHILY_PRINT
CPPOPTS +=	-DINS_BASE=\"${INS_BASE}\"
CPPOPTS +=	-DTEXT_DOMAIN=\"SCHILY_cdrtools\"

CFILES=		cdda2wav.c interface.c semshm.c resample.c \
		scsi_scan.c \
		toc.c wav.c sun.c raw.c setuid.c ringbuff.c \
		sndconfig.c scsi_cmds.c aiff.c aifc.c \
		scsi_cdr.c cd_misc.c ioctl.c base64.c \
		parse.c priv.c
HFILES=		cdda2wav.h interface.h semshm.h global.h resample.h \
		exitcodes.h \
		scsi_scan.h \
		mytype.h toc.h wav.h sun.h raw.h setuid.h ringbuff.h \
		config.h saveargs.h sndfile.h aiff.h aifc.h \
		sndconfig.h scsi_cmds.h lowlevel.h byteorder.h mycdrom.h \
		base64.h mmsystem.h cd_text.c cd_extra.c ioctl.h \
		parse.h version.h

#
# LIB_CAP is needed for Linux capability support in librscg.
#
LIBS=		-lscgcmd -lrscg -lscg $(LIB_VOLMGT) -lparanoia -lcdrdeflt -ldeflt -lmdigest -lschily $(SCSILIB) $(EXTRALIB) $(LIB_SOCKET) $(LIB_INTL) $(LIB_CAP)
XMK_FILE=	Makefile.man Makefile.rd cdda2ogg.mk cdda2mp3.mk
# cdda2ogg.mk1 fehlt noch für make rel

xall:		all
#$(ARCHDIR)/local.cnf $(ARCHDIR)/lconfig.h:	../cdrecord/cdrecord.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.cmd
###########################################################################
count:	$(CFILES) $(HFILES)
	count $r1
###########################################################################

_ARCH_=		$(_UNIQ)$(O_ARCH)
_HAVE_LINUX=	$(_ARCH_:$(_UNIQ)linux=1)
HAVE_LINUX=	$(_HAVE_LINUX:$(_ARCH_)%=%)
_HAVE_SUN_OS=	$(_ARCH_:$(_UNIQ)sunos%=1)
HAVE_SUN_OS=	$(_HAVE_SUN_OS:$(_ARCH_)%=%)
_HAVE_BSD_OS=	$(_ARCH_:$(_UNIQ)%bsd=1)
HAVE_BSD_OS=	$(_HAVE_BSD_OS:$(_ARCH_)%=%)
_HAVE_CYGWIN=	$(_ARCH_:$(_UNIQ)cygwin32_nt=1)
HAVE_CYGWIN=	$(_HAVE_CYGWIN:$(_ARCH_)%=%)
_HAVE_OS2=	$(_ARCH_:$(_UNIQ)os2=1)
HAVE_OS2=	$(_HAVE_OS2:$(_ARCH_)%=%)

