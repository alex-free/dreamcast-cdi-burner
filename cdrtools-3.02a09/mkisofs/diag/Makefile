#ident "@(#)Makefile	1.10 10/12/19 "
###########################################################################
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2
# as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; see the file COPYING.  If not, write to the Free Software
# Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
###########################################################################
SRCROOT=	../..
RULESDIR=	RULES
include		$(SRCROOT)/$(RULESDIR)/rules.top
###########################################################################

MK_FILES=	dump.mk isodump.mk isoinfo.mk isovfy.mk isodebug.mk
#XMK_FILE=	Makefile.man

scsi.c:
	@echo "	==> MAKING SYMLINKS in ." && sh ./MKLINKS
$(ALLTARGETS): scsi.c

###########################################################################
include		$(SRCROOT)/$(RULESDIR)/rules.mks
###########################################################################

CFILES: dump.c isodebug.c isodump.c isoinfo.c isovfy.c

xtxt:	$(CFILES)
	gxgettext --no-wrap -k_ -k__ $(CFILES)
