#                                               5 December 2006.  SMS.
#
#    CDRTOOLS 2.0 for VMS - MMS (or MMK) Description File.
#
# Usage:
#
#    SET DEFAULT [.directory]   ! [.VMS] to build all products.
#    MMS /DESCRIPTION = [-.VMS] [/MACRO = (<see_below>)] target
#
# Optional macros:
#
#    CCOPTS=xxx     Compile with CC options xxx.  For example:
#                   "CCOPTS=/ARCH=HOST"
#
#    CDEFS_USER=xxx  Compile with C macro definition(s) xxx.  For
#                    example:
#                   CDEFS_USER=IS_SCHILY_XCONFIG=1
#
#    DBG=1          Compile with /DEBUG /NOOPTIMIZE.
#                   Link with /DEBUG /TRACEBACK.
#                   (Default is /NOTRACEBACK.)
#
#    FIND=1         Enable support for "-find" in mkisofs.  See
#                   [.VMS]VMS_NOTES.TXT for cautions and details.
#
#    LARGE=1        Enable large-file (>2GB) support.  Non-VAX only.
#
#    LINKOPTS=xxx   Link with LINK options xxx.  For example:
#                   "LINKOPTS=/NOINFO"   
#
#    LIST=1         Compile with /LIST /SHOW = (ALL, NOMESSAGES).
#                   Link with /MAP /CROSS_REFERENCE /FULL.
#
#
# The default target, ALL, builds all the product executables.
#
# Other targets:
#
#    CLEAN      deletes architecture-specific files, but leaves any
#               individual source dependency files.
#
#    CLEAN_ALL  deletes all generated files, except the main (collected)
#               source dependency files.
#
#    CLEAN_EXE  deletes only the architecture-specific executables. 
#               Handy if all you wish to do is re-link the executables.
#
#    CLEAN_OLB  deletes only the architecture-specific object libraries. 
#
#
# Example commands:
#
# To build the conventional small-file product using the DEC/Compaq/HP C
# compiler (Note: DESCRIP.MMS is the default description file name.):
#
#    MMS
#
# To get the large-file executables (on a non-VAX system):
#
#    MMS /MACRO = (LARGE=1)
#
# To delete the architecture-specific generated files for this system
# type:
#
#    MMS /MACRO = (LARGE=1) CLEAN       ! Large-file.
# or
#    MMS CLEAN                          ! Small-file.
#
# To build a complete small-file product for debug with compiler
# listings and link maps:
#
#    MMS CLEAN
#    MMS /MACRO = (DBG=1, LIST=1)
#
########################################################################

# Include primary product description file.

INCL_DESCRIP_SRC = 1
.INCLUDE descrip_src.mms


# System-specific header files.

ALIGN_H = $(DIR_INC_DEST)align.h
ALIGN_TEST_EXE = $(DIR_INC_DEST)ALIGN_TEST.EXE

AVOFFSET_H = $(DIR_INC_DEST)avoffset.h
AVOFFSET_EXE = $(DIR_INC_DEST)AVOFFSET.EXE

LCONFIG_H = [-.$(DIR_CDDA2WAV)]lconfig.h
LCONFIG_H_VMS = [-.$(DIR_CDDA2WAV)]lconfig.h_vms
APPEND_VERSION_COM = [-.VMS]APPEND_VERSION.COM

# TARGETS.

# Default subsidiary targets.

# Build CDDA2WAV executable or object library (default target).

.IFDEF TARGET_CDDA2WAV          # TARGET_CDDA2WAV

SUBSIDIARY = 1

CDDA2WAV : $(CDDA2WAV_EXE)
	@ write sys$output ""
	@ write sys$output "   CDDA2WAV done."
	@ write sys$output ""

$(LIB_CDDA2WAV) : $(LIB_CDDA2WAV)($(MODS_OBJS_LIB_CDDA2WAV))
	@ write sys$output "$(MMS$TARGET) updated."

# Special rule for SHA_FUNC.C on VAX where Compaq C V6.4-005 (like,
# probably, other versions) loops with /optimize = disjoint.

.IFDEF __VAX__                      # __VAX__

[.$(DEST)]SHA_FUNC.OBJ : SHA_FUNC.C
	@ write sys$output "***************************************"
	@ write sys$output "* Note: Exceptional rule in use here: *"
	@ write sys$output "***************************************"
	$(CC) $(CFLAGS) /optimize = nodisjoint /define = ($(CDEFS)) -
	 $(MMS$SOURCE)

.ENDIF                              # __VAX__

$(CDDA2WAV_EXE) : $(LIB_CDDA2WAV) $(LIB_CDRECORD) $(LIB_DEFLT) \
                  $(LIB_PARANOIA) $(LIB_SCG) $(LIB_SCHILY) $(LIB_VMS)
	$(LINK) $(LINKFLAGS) -
	 $(LIB_CDDA2WAV) /library /include = (cdda2wav, vms), -
	 $(LIB_CDRECORD) /library, -
	 $(LIB_DEFLT) /library, -
	 $(LIB_PARANOIA) /library, -
	 $(LIB_SCG) /library, -
	 $(LIB_SCHILY) /library, -
	 $(LIB_VMS) /library /include = vms_init -
	 $(LFLAGS_ARCH)

.ENDIF                          # TARGET_CDDA2WAV


# Build CDRECORD executable or object library (default target).

.IFDEF TARGET_CDRECORD          # TARGET_CDRECORD

SUBSIDIARY = 1

CDRECORD : $(CDRECORD_EXE)
	@ write sys$output ""
	@ write sys$output "   CDRECORD done."
	@ write sys$output ""

$(LIB_CDRECORD) : $(LIB_CDRECORD)($(MODS_OBJS_LIB_CDRECORD))
	@ write sys$output "$(MMS$TARGET) updated."

$(CDRECORD_EXE) : $(LIB_CDRECORD) $(LIB_DEFLT) $(LIB_EDC) $(LIB_SCG) \
                  $(LIB_SCHILY) $(LIB_VMS)
	$(LINK) $(LINKFLAGS) -
	 $(LIB_CDRECORD) /library /include = (cdrecord, vms), -
	 $(LIB_DEFLT) /library, -
	 $(LIB_EDC) /library, -
	 $(LIB_SCG) /library, -
	 $(LIB_SCHILY) /library, -
	 $(LIB_VMS) /library /include = vms_init -
	 $(LFLAGS_ARCH)

.ENDIF                          # TARGET_CDRECORD


# Build some system-dependent header files.

.IFDEF TARGET_INC               # TARGET_INC

SUBSIDIARY = 1

INC : $(ALIGN_H) $(AVOFFSET_H)
	@ write sys$output ""
	@ write sys$output "   INC done."
	@ write sys$output ""

.ENDIF                          # TARGET_INC


# Build LIBDEFLT object library.

.IFDEF TARGET_LIBDEFLT          # TARGET_LIBDEFLT

SUBSIDIARY = 1

LIBDEFLT : $(LIB_DEFLT)
	@ write sys$output ""
	@ write sys$output "   LIBDEFLT done."
	@ write sys$output ""

.ENDIF                          # TARGET_LIBDEFLT


# Build LIBEDC object library.

.IFDEF TARGET_LIBEDC            # TARGET_LIBEDC

SUBSIDIARY = 1

LIBEDC : $(LIB_EDC)
	@ write sys$output ""
	@ write sys$output "   LIBEDC done."
	@ write sys$output ""

.ENDIF                          # TARGET_LIBEDC


# Build LIBFILE object library.

.IFDEF TARGET_LIBFILE           # TARGET_LIBFILE

SUBSIDIARY = 1

LIBFILE : $(LIB_FILE)
	@ write sys$output ""
	@ write sys$output "   LIBFILE done."
	@ write sys$output ""

.ENDIF                          # TARGET_LIBFILE


# Build LIBFIND object library.

.IFDEF TARGET_LIBFIND           # TARGET_LIBFIND

SUBSIDIARY = 1

LIBFIND : $(LIB_FIND)
	@ write sys$output ""
	@ write sys$output "   LIBFIND done."
	@ write sys$output ""

.ENDIF                          # TARGET_LIBFIND


# Build LIBHFS_ISO object library.

.IFDEF TARGET_LIBHFS_ISO        # TARGET_LIBHFS_ISO

SUBSIDIARY = 1

LIBHFS_ISO : $(LIB_HFS_ISO)
	@ write sys$output ""
	@ write sys$output "   LIBHFS_ISO done."
	@ write sys$output ""

.ENDIF                          # TARGET_LIBHFS_ISO


# Build LIBPARANOIA object library.

.IFDEF TARGET_LIBPARANOIA       # TARGET_LIBPARANOIA

SUBSIDIARY = 1

LIBPARANOIA : $(LIB_PARANOIA)
	@ write sys$output ""
	@ write sys$output "   LIBPARANOIA done."
	@ write sys$output ""

.ENDIF                          # TARGET_LIBPARANOIA


# Build LIBSCG object library.

.IFDEF TARGET_LIBSCG            # TARGET_LIBSCG

SUBSIDIARY = 1

LIBSCG : $(LIB_SCG)
	@ write sys$output ""
	@ write sys$output "   LIBSCG done."
	@ write sys$output ""

.ENDIF                          # TARGET_LIBSCG


# Build SCGCHECK executable or object library (default target).

.IFDEF TARGET_SCGCHECK       # TARGET_SCGCHECK

SUBSIDIARY = 1

SCGCHECK : $(SCGCHECK_EXE)
	@ write sys$output ""
	@ write sys$output "   SCGCHECK done."
	@ write sys$output ""

$(LIB_SCGCHECK) : $(LIB_SCGCHECK)($(MODS_OBJS_LIB_SCGCHECK))
	@ write sys$output "$(MMS$TARGET) updated."

$(SCGCHECK_EXE) : $(LIB_SCGCHECK) $(LIB_CDRECORD) $(LIB_SCG) \
                  $(LIB_SCHILY) $(LIB_VMS)
	$(LINK) $(LINKFLAGS) -
	 $(LIB_SCGCHECK) /library /include = (scgcheck, vms), -
	 $(LIB_CDRECORD) /library, -
	 $(LIB_SCG) /library, -
	 $(LIB_SCHILY) /library, -
	 $(LIB_VMS) /library /include = vms_init -
	 $(LFLAGS_ARCH)

.ENDIF                          # TARGET_SCGCHECK


# Build LIBSCHILY object library.

.IFDEF TARGET_LIBSCHILY         # TARGET_LIBSCHILY

SUBSIDIARY = 1

LIBSCHILY : $(LIB_SCHILY)
	@ write sys$output ""
	@ write sys$output "   LIBSCHILY done."
	@ write sys$output ""

.ENDIF                          # TARGET_LIBSCHILY


# Build LIBUNLS object library.

.IFDEF TARGET_LIBUNLS           # TARGET_LIBUNLS

SUBSIDIARY = 1

LIBUNLS : $(LIB_UNLS)
	@ write sys$output ""
	@ write sys$output "   LIBUNLS done."
	@ write sys$output ""

.ENDIF                          # TARGET_LIBUNLS


# Build DECC_VER executable or LIBVMS object library.

.IFDEF TARGET_LIBVMS            # TARGET_LIBVMS

SUBSIDIARY = 1

LIBVMS : $(DECC_VER_EXE)
	@ write sys$output ""
	@ write sys$output "   DECC_VER done."
	@ write sys$output ""

$(LIB_VMS) : $(LIB_VMS)($(MODS_OBJS_LIB_VMS))
        @ write sys$output "$(MMS$TARGET) updated."

$(DECC_VER_EXE) : $(LIB_VMS)
	$(LINK) $(LINKFLAGS) -
	 $(LIB_VMS) /library /include = decc_ver -
	 $(LFLAGS_ARCH)

.ENDIF                          # TARGET_LIBVMS


# Build MKISOFS executable or object library (default target).

.IFDEF TARGET_MKISOFS           # TARGET_MKISOFS

SUBSIDIARY = 1

MKISOFS : $(MKISOFS_EXE)
	@ write sys$output ""
	@ write sys$output "   MKISOFS done."
	@ write sys$output ""

$(LIB_MKISOFS) : $(LIB_MKISOFS)($(MODS_OBJS_LIB_MKISOFS))
	@ write sys$output "$(MMS$TARGET) updated."

$(LIB_MKISOFS_DIAG) : $(LIB_MKISOFS_DIAG)($(MODS_OBJS_LIB_MKISOFS_DIAG))
	@ write sys$output "$(MMS$TARGET) updated."

$(MKISOFS_EXE) : $(LIB_MKISOFS) $(LIB_CDRECORD) $(LIB_FILE) \
                 $(LIB_FIND_DEP) $(LIB_HFS_ISO) $(LIB_DEFLT) \
                 $(LIB_SCG) $(LIB_SCHILY) $(LIB_UNLS) $(LIB_VMS)
	$(LINK) $(LINKFLAGS) -
	 $(LIB_MKISOFS) /library /include = (mkisofs, vms, walk), -
	 $(LIB_CDRECORD) /library, -
	 $(LIB_FILE) /library, -
	 $(LIB_FIND_OPTS) -
	 $(LIB_HFS_ISO) /library, -
	 $(LIB_DEFLT) /library, -
	 $(LIB_SCG) /library, -
	 $(LIB_SCHILY) /library, -
	 $(LIB_UNLS) /library, -
	 $(LIB_VMS) /library /include = vms_init -
	 $(LFLAGS_ARCH)

$(ISODEBUG_EXE) : $(ISODEBUG_OBJ) \
                  $(LIB_CDRECORD) $(LIB_DEFLT) $(LIB_MKISOFS) \
                  $(LIB_MKISOFS_DIAG) $(LIB_SCG) $(LIB_SCHILY) \
                  $(LIB_UNLS) $(LIB_VMS)
	$(LINK) $(LINKFLAGS) -
	 $(ISODEBUG_OBJ), -
	 $(LIB_MKISOFS) /library, -
	 $(LIB_CDRECORD) /library, -
	 $(LIB_DEFLT) /library, -
	 $(LIB_SCG) /library, -
	 $(LIB_SCHILY) /library, -
	 $(LIB_UNLS) /library, -
	 $(LIB_MKISOFS_DIAG) /library /include = vms_diag, -
	 $(LIB_VMS) /library /include = vms_init -
	 $(LFLAGS_ARCH)

$(ISOINFO_EXE) : $(ISOINFO_OBJ) \
                  $(LIB_CDRECORD) $(LIB_DEFLT) $(LIB_MKISOFS) \
                  $(LIB_MKISOFS_DIAG) $(LIB_SCG) $(LIB_SCHILY) \
                  $(LIB_UNLS) $(LIB_VMS)
	$(LINK) $(LINKFLAGS) -
	 $(ISOINFO_OBJ), -
	 $(LIB_MKISOFS) /library, -
	 $(LIB_CDRECORD) /library, -
	 $(LIB_DEFLT) /library, -
	 $(LIB_SCG) /library, -
	 $(LIB_SCHILY) /library, -
	 $(LIB_UNLS) /library, -
	 $(LIB_MKISOFS_DIAG) /library /include = vms_diag, -
	 $(LIB_VMS) /library /include = vms_init -
	 $(LFLAGS_ARCH)

$(ISOVFY_EXE) : $(ISOVFY_OBJ) \
                  $(LIB_CDRECORD) $(LIB_DEFLT) $(LIB_MKISOFS) \
                  $(LIB_MKISOFS_DIAG) $(LIB_SCG) $(LIB_SCHILY) \
                  $(LIB_VMS)
	$(LINK) $(LINKFLAGS) -
	 $(ISOVFY_OBJ), -
	 $(LIB_MKISOFS) /library, -
	 $(LIB_CDRECORD) /library, -
	 $(LIB_DEFLT) /library, -
	 $(LIB_SCG) /library, -
	 $(LIB_SCHILY) /library, -
	 $(LIB_MKISOFS_DIAG) /library /include = vms_diag, -
	 $(LIB_VMS) /library /include = vms_init -
	 $(LFLAGS_ARCH)

.ENDIF                          # TARGET_MKISOFS


# Build READCD executable or object library (default target).

.IFDEF TARGET_READCD            # TARGET_READCD

SUBSIDIARY = 1

READCD : $(READCD_EXE)
	@ write sys$output ""
	@ write sys$output "   READCD done."
	@ write sys$output ""

$(LIB_READCD) : $(LIB_READCD)($(MODS_OBJS_LIB_READCD))
	@ write sys$output "$(MMS$TARGET) updated."

$(READCD_EXE) : $(LIB_READCD) $(LIB_CDRECORD) $(LIB_DEFLT) $(LIB_EDC) \
                $(LIB_SCG) $(LIB_SCHILY) $(LIB_UNLS) $(LIB_VMS)
	$(LINK) $(LINKFLAGS) -
	 $(LIB_READCD) /library /include = (readcd, vms), -
	 $(LIB_CDRECORD) /library, -
	 $(LIB_DEFLT) /library, -
	 $(LIB_EDC) /library, -
	 $(LIB_SCG) /library, -
	 $(LIB_SCHILY) /library, -
	 $(LIB_UNLS) /library, -
	 $(LIB_VMS) /library /include = vms_init -
	 $(LFLAGS_ARCH)

.ENDIF                          # TARGET_READCD


# Default global target.

ALL : $(EXES)
	@ show time
	@ write sys$output ""
	@ write sys$output "   ALL done."
	@ write sys$output ""


# Global rules for executables and object libraries.

.IFDEF TARGET_CDDA2WAV          # TARGET_CDDA2WAV
.ELSE                           # TARGET_CDDA2WAV

$(LIB_CDDA2WAV) :
	dev_dir = f$environment( "DEFAULT")
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_CDDA2WAV)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(LIB_CDDA2WAV)
	set default 'dev_dir'
	show default
	@ write sys$output ""

$(CDDA2WAV_EXE) :
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_CDDA2WAV)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(CDDA2WAV_EXE)

.ENDIF                          # TARGET_CDDA2WAV [else]


.IFDEF TARGET_CDRECORD          # TARGET_CDRECORD
.ELSE                           # TARGET_CDRECORD

$(LIB_CDRECORD) :
	dev_dir = f$environment( "DEFAULT")
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_CDRECORD)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(LIB_CDRECORD)
	set default 'dev_dir'
	show default
	@ write sys$output ""

$(CDRECORD_EXE) :
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_CDRECORD)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(CDRECORD_EXE)

.ENDIF                          # TARGET_CDRECORD [else]


.IFDEF TARGET_LIBVMS            # TARGET_LIBVMS
.ELSE                           # TARGET_LIBVMS

$(LIB_VMS) :
	dev_dir = f$environment( "DEFAULT")
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_VMS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(LIB_VMS)
	set default 'dev_dir'
	show default
	@ write sys$output ""

$(DECC_VER_EXE) :
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_VMS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(DECC_VER_EXE)

.ENDIF                          # TARGET_VMSLIB [else]


.IFDEF TARGET_MKISOFS           # TARGET_MKISOFS
.ELSE                           # TARGET_MKISOFS

$(LIB_MKISOFS) :
	dev_dir = f$environment( "DEFAULT")
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_MKISOFS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(LIB_MKISOFS)
	set default 'dev_dir'
	show default
	@ write sys$output ""

$(MKISOFS_EXE) :
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_MKISOFS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(MKISOFS_EXE)

$(ISODEBUG_EXE) :
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_MKISOFS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(ISODEBUG_EXE)

$(ISOINFO_EXE) :
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_MKISOFS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(ISOINFO_EXE)

$(ISOVFY_EXE) :
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_MKISOFS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(ISOVFY_EXE)

.ENDIF                          # TARGET_MKISOFS [else]


.IFDEF TARGET_READCD            # TARGET_READCD
.ELSE                           # TARGET_READCD

$(LIB_READCD) :
	dev_dir = f$environment( "DEFAULT")
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_READCD)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(LIB_READCD)
	set default 'dev_dir'
	show default
	@ write sys$output ""

$(READCD_EXE) :
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_READCD)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(READCD_EXE)

.ENDIF                          # TARGET_READCD [else]


.IFDEF TARGET_SCGCHECK          # TARGET_SCGCHECK
.ELSE                           # TARGET_SCGCHECK

$(LIB_SCGCHECK) :
	dev_dir = f$environment( "DEFAULT")
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_SCGCHECK)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(LIB_SCGCHECK)
	set default 'dev_dir'
	show default
	@ write sys$output ""

$(SCGCHECK_EXE) :
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_SCGCHECK)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(SCGCHECK_EXE)

.ENDIF                          # TARGET_SCGCHECK [else]


# CLEAN target.  Delete the [.$(DEST)] directory and everything in it.

.IFDEF SUBSIDIARY               # SUBSIDIARY

CLEAN :
	if (f$search( "[.$(DEST)]*.*") .nes. "") then -
	 delete [.$(DEST)]*.*;*
	if (f$search( "$(DEST).dir", 1) .nes. "") then -
	 set protection = w:d $(DEST).dir;*
	if (f$search( "$(DEST).dir", 2) .nes. "") then -
	 delete $(DEST).dir;*

# CLEAN_ALL target.  Delete:
#    The [...$(DEST)] directories and everything in them.
#    All individual C dependency files.
#    Generated [.CDDA2WAV]LCONFIG.H.
# Also mention:
#    Comprehensive dependency file.

CLEAN_ALL :
	@ write sys$output "   SUBS - CLEAN_ALL"
	show default
	@ write sys$output ""
	if (f$search( "[...ALPHA*]*.*") .nes. "") then -
	 delete [...ALPHA*]*.*;*
	if (f$search( "[...]ALPHA*.dir", 1) .nes. "") then -
	 set protection = w:d [...]ALPHA*.dir;*
	if (f$search( "[...]ALPHA*.dir", 2) .nes. "") then -
	 delete [...]ALPHA*.dir;*
	if (f$search( "[...IA64*]*.*") .nes. "") then -
	 delete [...IA64*]*.*;*
	if (f$search( "[...]IA64*.dir", 1) .nes. "") then -
	 set protection = w:d [...]IA64*.dir;*
	if (f$search( "[...]IA64*.dir", 2) .nes. "") then -
	 delete [...]IA64*.dir;*
	if (f$search( "[...VAX*]*.*") .nes. "") then -
	 delete [...VAX*]*.*;*
	if (f$search( "[...]VAX*.dir", 1) .nes. "") then -
	 set protection = w:d [...]VAX*.dir;*
	if (f$search( "[...]VAX*.dir", 2) .nes. "") then -
	 delete [...]VAX*.dir;*
	if (f$search( "[...]*.MMSD") .nes. "") then -
	 delete [...]*.MMSD;*
	if (f$search( "$(LCONFIG_H)") .nes. "") then -
	 delete $(LCONFIG_H);*
	@ write sys$output ""
	@ write sys$output "Note:  This procedure will not"
	@ write sys$output "   DELETE DESCRIP_DEPS.MMS;*"
	@ write sys$output -
 "You may choose to, but a recent version of MMS (V3.5 or newer?) is"
	@ write sys$output -
 "needed to regenerate it.  (It may also be recovered from the original"
	@ write sys$output -
 "distribution kit.)  See DESCRIP_MKDEPS.MMS for instructions on"
	@ write sys$output -
 "generating DESCRIP_DEPS.MMS."
	@ write sys$output ""

# CLEAN_EXE target.  Delete the executables in [.$(DEST)].

CLEAN_EXE :
	if (f$search( "[.$(DEST)]*.EXE") .nes. "") then -
	 delete [.$(DEST)]*.EXE;*

# CLEAN_OLB target.  Delete the object libraries in [.$(DEST)].

CLEAN_OLB :
	if (f$search( "[.$(DEST)]*.OLB") .nes. "") then -
	 delete [.$(DEST)]*.OLB;*

.ELSE                           # SUBSIDIARY

.IFDEF MMSTARGETS                   # MMSTARGETS

#
# MMS (or MMK) with the MMSTARGETS macro needs only one real CLEAN rule.
#

CLEAN, CLEAN_ALL, CLEAN_EXE, CLEAN_OLB :
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_CDDA2WAV)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(MMSTARGETS)
	set default [-.$(DIR_CDRECORD)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(MMSTARGETS)
	set default [-.$(DIR_DEFLT)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(MMSTARGETS)
	set default [-.$(DIR_EDC)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(MMSTARGETS)
	set default [-.$(DIR_FILE)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(MMSTARGETS)
	set default [-.$(DIR_FIND)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(MMSTARGETS)
	set default [-.$(DIR_HFS_ISO)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(MMSTARGETS)
	set default [-.$(DIR_INC)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(MMSTARGETS)
	set default [-.$(DIR_MKISOFS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(MMSTARGETS)
	set default [-.$(DIR_PARANOIA)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(MMSTARGETS)
	set default [-.$(DIR_READCD)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(MMSTARGETS)
	set default [-.$(DIR_SCG)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(MMSTARGETS)
	set default [-.$(DIR_SCGCHECK)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(MMSTARGETS)
	set default [-.$(DIR_SCHILY)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(MMSTARGETS)
	set default [-.$(DIR_UNLS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(MMSTARGETS)
	set default [-.$(DIR_VMS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(MMSTARGETS)

.ELSE                               # MMSTARGETS

#
# MMK without the MMSTARGETS macro needs more rules.
#

CLEAN :
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_CDDA2WAV)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN
	set default [-.$(DIR_CDRECORD)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN
	set default [-.$(DIR_DEFLT)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN
	set default [-.$(DIR_EDC)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN
	set default [-.$(DIR_FILE)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN
	set default [-.$(DIR_FIND)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN
	set default [-.$(DIR_HFS_ISO)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN
	set default [-.$(DIR_INC)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN
	set default [-.$(DIR_MKISOFS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN
	set default [-.$(DIR_PARANOIA)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN
	set default [-.$(DIR_READCD)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN
	set default [-.$(DIR_SCG)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN
	set default [-.$(DIR_SCGCHECK)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN
	set default [-.$(DIR_SCHILY)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN
	set default [-.$(DIR_UNLS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN
	set default [-.$(DIR_VMS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN

CLEAN_ALL :
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_CDDA2WAV)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_ALL
	set default [-.$(DIR_CDRECORD)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_ALL
	set default [-.$(DIR_DEFLT)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_ALL
	set default [-.$(DIR_EDC)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_ALL
	set default [-.$(DIR_FILE)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_ALL
	set default [-.$(DIR_FIND)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_ALL
	set default [-.$(DIR_HFS_ISO)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_ALL
	set default [-.$(DIR_INC)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_ALL
	set default [-.$(DIR_MKISOFS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_ALL
	set default [-.$(DIR_PARANOIA)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_ALL
	set default [-.$(DIR_READCD)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_ALL
	set default [-.$(DIR_SCG)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_ALL
	set default [-.$(DIR_SCGCHECK)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_ALL
	set default [-.$(DIR_SCHILY)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_ALL
	set default [-.$(DIR_UNLS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_ALL
	set default [-.$(DIR_VMS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_ALL

CLEAN_EXE :
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_CDDA2WAV)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_EXE
	set default [-.$(DIR_CDRECORD)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_EXE
	set default [-.$(DIR_DEFLT)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_EXE
	set default [-.$(DIR_EDC)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_EXE
	set default [-.$(DIR_FILE)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_EXE
	set default [-.$(DIR_FIND)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_EXE
	set default [-.$(DIR_HFS_ISO)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_EXE
	set default [-.$(DIR_INC)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_EXE
	set default [-.$(DIR_MKISOFS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_EXE
	set default [-.$(DIR_PARANOIA)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_EXE
	set default [-.$(DIR_READCD)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_EXE
	set default [-.$(DIR_SCG)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_EXE
	set default [-.$(DIR_SCGCHECK)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_EXE
	set default [-.$(DIR_SCHILY)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_EXE
	set default [-.$(DIR_UNLS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_EXE
	set default [-.$(DIR_VMS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_EXE

CLEAN_OLB :
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_CDDA2WAV)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_OLB
	set default [-.$(DIR_CDRECORD)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_OLB
	set default [-.$(DIR_DEFLT)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_OLB
	set default [-.$(DIR_EDC)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_OLB
	set default [-.$(DIR_FILE)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_OLB
	set default [-.$(DIR_FIND)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_OLB
	set default [-.$(DIR_HFS_ISO)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_OLB
	set default [-.$(DIR_INC)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_OLB
	set default [-.$(DIR_MKISOFS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_OLB
	set default [-.$(DIR_PARANOIA)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_OLB
	set default [-.$(DIR_READCD)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_OLB
	set default [-.$(DIR_SCG)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_OLB
	set default [-.$(DIR_SCGCHECK)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_OLB
	set default [-.$(DIR_SCHILY)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_OLB
	set default [-.$(DIR_UNLS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_OLB
	set default [-.$(DIR_VMS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 CLEAN_OLB

.ENDIF                              # MMSTARGETS [else]

.ENDIF                          # SUBSIDIARY

# Generated header files.

GENERATED_HEADERS : $(LCONFIG_H)
	@ write sys$output "$(MMS$TARGET) updated."

DEFAULT :
	@ write sys$output "No target, specified or default."


# Object library module dependencies.


# DEFLT object library.

.IFDEF MODS_OBJS_LIB_DEFLT      # MODS_OBJS_LIB_DEFLT

$(LIB_DEFLT) : $(LIB_DEFLT)($(MODS_OBJS_LIB_DEFLT))
	@ write sys$output "$(MMS$TARGET) updated."

.ELSE                           # MODS_OBJS_LIB_DEFLT

$(LIB_DEFLT) :
	dev_dir = f$environment( "DEFAULT")
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_DEFLT)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(LIB_DEFLT)
	set default 'dev_dir'
	show default
	@ write sys$output ""

.ENDIF                          # MODS_OBJS_LIB_DEFLT [else]


# EDC object library.

.IFDEF MODS_OBJS_LIB_EDC        # MODS_OBJS_LIB_EDC

$(LIB_EDC) : $(LIB_EDC)($(MODS_OBJS_LIB_EDC))
	@ write sys$output "$(MMS$TARGET) updated."

.ELSE                           # MODS_OBJS_LIB_EDC

$(LIB_EDC) :
	dev_dir = f$environment( "DEFAULT")
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_EDC)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(LIB_EDC)
	set default 'dev_dir'
	show default
	@ write sys$output ""

.ENDIF                          # MODS_OBJS_LIB_EDC [else]


# FILE object library.

.IFDEF MODS_OBJS_LIB_FILE       # MODS_OBJS_LIB_FILE

$(LIB_FILE) : $(LIB_FILE)($(MODS_OBJS_LIB_FILE))
	@ write sys$output "$(MMS$TARGET) updated."

.ELSE                           # MODS_OBJS_LIB_FILE

$(LIB_FILE) :
	dev_dir = f$environment( "DEFAULT")
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_FILE)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(LIB_FILE)
	set default 'dev_dir'
	show default
	@ write sys$output ""

.ENDIF                          # MODS_OBJS_LIB_FILE [else]


# FIND object library.

.IFDEF MODS_OBJS_LIB_FIND       # MODS_OBJS_LIB_FIND

$(LIB_FIND) : $(LIB_FIND)($(MODS_OBJS_LIB_FIND))
	@ write sys$output "$(MMS$TARGET) updated."

.ELSE                           # MODS_OBJS_LIB_FIND

$(LIB_FIND) :
	dev_dir = f$environment( "DEFAULT")
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_FIND)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(LIB_FIND)
	set default 'dev_dir'
	show default
	@ write sys$output ""

.ENDIF                          # MODS_OBJS_LIB_FIND [else]


# HFS_ISO object library.

.IFDEF MODS_OBJS_LIB_HFS_ISO    # MODS_OBJS_LIB_HFS_ISO

$(LIB_HFS_ISO) : $(LIB_HFS_ISO)($(MODS_OBJS_LIB_HFS_ISO))
	@ write sys$output "$(MMS$TARGET) updated."

.ELSE                           # MODS_OBJS_LIB_HFS_ISO

$(LIB_HFS_ISO) :
	dev_dir = f$environment( "DEFAULT")
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_HFS_ISO)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(LIB_HFS_ISO)
	set default 'dev_dir'
	show default
	@ write sys$output ""

.ENDIF                          # MODS_OBJS_LIB_HFS_ISO [else]


# PARANOIA object library.

.IFDEF MODS_OBJS_LIB_PARANOIA   # MODS_OBJS_LIB_PARANOIA

$(LIB_PARANOIA) : $(LIB_PARANOIA)($(MODS_OBJS_LIB_PARANOIA))
	@ write sys$output "$(MMS$TARGET) updated."

.ELSE                           # MODS_OBJS_LIB_PARANOIA

$(LIB_PARANOIA) :
	dev_dir = f$environment( "DEFAULT")
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_PARANOIA)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(LIB_PARANOIA)
	set default 'dev_dir'
	show default
	@ write sys$output ""

.ENDIF                          # MODS_OBJS_LIB_PARANOIA [else]


# SCG object library.

.IFDEF MODS_OBJS_LIB_SCG        # MODS_OBJS_LIB_SCG

$(LIB_SCG) : $(LIB_SCG)($(MODS_OBJS_LIB_SCG))
	@ write sys$output "$(MMS$TARGET) updated."

.ELSE                           # MODS_OBJS_LIB_SCG

$(LIB_SCG) :
	dev_dir = f$environment( "DEFAULT")
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_SCG)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(LIB_SCG)
	set default 'dev_dir'
	show default
	@ write sys$output ""

.ENDIF                          # MODS_OBJS_LIB_SCG [else]


# SCHILY object library.

.IFDEF MODS_OBJS_LIB_SCHILY_ALL # MODS_OBJS_LIB_SCHILY_ALL

$(LIB_SCHILY) : $(DIR_INC_DEST_FILE) $(LIB_SCHILY)($(MODS_OBJS_LIB_SCHILY_ALL))
	@ write sys$output "$(MMS$TARGET) updated."

.ELSE                           # MODS_OBJS_LIB_SCHILY_ALL

$(LIB_SCHILY) :
	dev_dir = f$environment( "DEFAULT")
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_SCHILY)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(LIB_SCHILY)
	set default 'dev_dir'
	show default
	@ write sys$output ""

.ENDIF                          # MODS_OBJS_LIB_SCHILY_ALL [else]


# UNLS object library.

.IFDEF MODS_OBJS_LIB_UNLS       # MODS_OBJS_LIB_UNLS

$(LIB_UNLS) : $(LIB_UNLS)($(MODS_OBJS_LIB_UNLS))
	@ write sys$output "$(MMS$TARGET) updated."

.ELSE                           # MODS_OBJS_LIB_UNLS

$(LIB_UNLS) :
	dev_dir = f$environment( "DEFAULT")
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_UNLS)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(LIB_UNLS)
	set default 'dev_dir'
	show default
	@ write sys$output ""

.ENDIF                          # MODS_OBJS_LIB_UNLS [else]


# Default C compile rule.

.C.OBJ :
	$(CC) $(CFLAGS) /define = ($(CDEFS)) $(MMS$SOURCE)


# ALIGN.H header file.

.IFDEF TARGET_INC               # TARGET_INC

$(ALIGN_H) : $(ALIGN_TEST_EXE)
	create /fdl = [-.VMS]STREAM_LF.FDL $(MMS$TARGET)
	define /user_mode sys$output $(MMS$TARGET)
	run $(ALIGN_TEST_EXE)

$(ALIGN_TEST_EXE) : $(OBJS_ALIGN_TEST)
	$(LINK) $(LINKFLAGS) $(OBJS_ALIGN_TEST) -
	 $(LFLAGS_ARCH)

.ELSE                           # TARGET_INC

$(ALIGN_H) :
	dev_dir = f$environment( "DEFAULT")
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_INC)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(ALIGN_H)
	set default 'dev_dir'
	show default
	@ write sys$output ""

$(ALIGN_TEST_EXE) :
	dev_dir = f$environment( "DEFAULT")
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_INC)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(ALIGN_TEST_EXE)
	set default 'dev_dir'
	show default
	@ write sys$output ""

.ENDIF                          # TARGET_INC [else]

# AVOFFSET.H header file.

.IFDEF TARGET_INC               # TARGET_INC

$(AVOFFSET_H) : $(AVOFFSET_EXE)
	create /fdl = [-.VMS]STREAM_LF.FDL $(MMS$TARGET)
	define /user_mode sys$output $(MMS$TARGET)
	run $(AVOFFSET_EXE)

$(AVOFFSET_EXE) : $(OBJS_AVOFFSET)
	$(LINK) $(LINKFLAGS) $(OBJS_AVOFFSET) -
	 $(LFLAGS_ARCH)

.ELSE                           # TARGET_INC

$(AVOFFSET_H) :
	dev_dir = f$environment( "DEFAULT")
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_INC)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(AVOFFSET_H)
	set default 'dev_dir'
	show default
	@ write sys$output ""

$(AVOFFSET_EXE) :
	dev_dir = f$environment( "DEFAULT")
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	set default [-.$(DIR_INC)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(AVOFFSET_EXE)
	set default 'dev_dir'
	show default
	@ write sys$output ""

.ENDIF                          # TARGET_INC [else]

# LCONFIG.H header file.

$(LCONFIG_H) : $(LCONFIG_H_VMS) [-.$(DIR_CDRECORD)]cdrecord.c \
               $(APPEND_VERSION_COM)
	copy $(LCONFIG_H_VMS) $(MMS$TARGET)
	@$(APPEND_VERSION_COM) [-.$(DIR_CDRECORD)]cdrecord.c -
	 cdr_version $(MMS$TARGET)
	purge /keep = 2 /log $(MMS$TARGET)


# Include generated source dependencies.

.IFDEF SUBSIDIARY               # SUBSIDIARY

INCL_DESCRIP_DEPS = 1

.INCLUDE descrip_deps.mms

.ENDIF                          # SUBSIDIARY

