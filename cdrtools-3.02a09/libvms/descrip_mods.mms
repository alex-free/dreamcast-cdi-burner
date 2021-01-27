#                                               5 December 2006.  SMS.
#
#    CDRTOOLS 2.0 for VMS - MMS (or MMK) Object Module List File.
#

# This description file is included by other description files.  It is
# not intended to be used alone.  Verify proper inclusion.

.IFDEF INCL_DESCRIP_SRC
.ELSE
$$$$ THIS DESCRIPTION FILE IS NOT INTENDED TO BE USED THIS WAY.
.ENDIF

# Object library modules.

MODS_OBJS_LIB_VMS = \
 DECC_VER=[.$(DEST)]DECC_VER.OBJ \
 VMS_INIT=[.$(DEST)]VMS_INIT.OBJ \
 VMS_MISC=[.$(DEST)]VMS_MISC.OBJ

