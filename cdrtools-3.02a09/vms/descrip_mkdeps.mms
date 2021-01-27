#                                               5 December 2006.  SMS.
#
#    CDRTOOLS 2.0 for VMS - MMS Dependency Description File.
#
#    MMS /EXTENDED_SYNTAX description file to generate a C source
#    dependencies file.  Unsightly errors result when /EXTENDED_SYNTAX
#    is not specified.  Typical usage:
#
#    $ MMS /EXTEND /DESCRIP = [-.VMS]DESCRIP_MKDEPS.MMS /SKIP
#
#    which discards individual source dependency files, or:
#
#    $ MMS /EXTEND /DESCRIP = [-.VMS]DESCRIP_MKDEPS.MMS /MACRO = NOSKIP=1
#
#    which retains them.  Retaining them can save time when doing code
#    development.
#
#
# The default target is the comprehensive source dependency file,
# $(DEPS_FILE) = "descrip_deps.mms".
#
# Other targets:
#
#    CLEAN      deletes the individual source dependency files,
#               *.MMSD;*, but leaves the comprehensive source dependency
#               file.
#
#    CLEAN_ALL  deletes all source dependency files, including the
#               individual *.MMSD;* files and the comprehensive file,
#               DESCRIP_DEPS.MMS.*.
#
# MMK users without MMS will be unable to generate the dependencies file
# using this description file, however there should be one supplied in
# the kit.  If this file has been deleted, users in this predicament
# will need to recover it from the original distribution kit.
#
# Note:  This dependency generation scheme assumes that the dependencies
# do not depend on host architecture type or other such variables. 
# Therefore, no "#include" directive in the C source itself should be
# conditional on such variables.
#
# Note:  CDRTOOLS code uses "system include" brackets (<>) everywhere,
# so /MMS_DEPENDENCIES = NOSYSTEM_INCLUDE_FILES is useless here.  Thus,
# we rely on COLLECT_DEPS.COM to filter out the system includes from the
# dependencies.
#
# This description file uses this command procedure:
#
#    [-.VMS]COLLECT_DEPS.COM
#

# Required command procedures.

COLLECT_DEPS = [-.VMS]COLLECT_DEPS.COM

COMS = $(COLLECT_DEPS)

# Include the source file lists (among other data).

INCL_DESCRIP_SRC = 1
.INCLUDE DESCRIP_SRC.MMS

# The ultimate individual product, a comprehensive dependency list.

DEPS_FILE = DESCRIP_DEPS.MMS

# Detect valid qualifier and/or macro options.

.IF $(FINDSTRING Skip, $(MMSQUALIFIERS)) .eq Skip
DELETE_MMSD = 1
.ELSIF NOSKIP
PURGE_MMSD = 1
.ELSE # [complex]
UNK_MMSD = 1
.ENDIF # [else, complex]

# Dependency suffixes and rules.
#
# .FIRST is assumed to be used already, so the MMS qualifier/macro check
# is included in each rule (one way or another).

.SUFFIXES_BEFORE .C .MMSD

.C.MMSD :
.IF UNK_MMSD                    # UNK_MMSD
	@ write sys$output -
 "   /SKIP_INTERMEDIATES is expected on the MMS command line."
	@ write sys$output -
 "   For normal behavior (delete .MMSD files), specify ""/SKIP""."
	@ write sys$output -
 "   To retain the .MMSD files, specify ""/MACRO = NOSKIP=1""."
	@ exit %x00000004
.ENDIF                          # UNK_MMSD
	$(CC) $(CFLAGS_ARCH) $(CFLAGS_INCL) $(CFLAGS_SPEC) -
	 /define = ($(CDEFS)) $(MMS$SOURCE) -
	 /NOLIST /NOOBJECT  /MMS_DEPENDENCIES = (FILE = $(MMS$TARGET))

# List of MMS dependency files.

# In case it's not obvious...
# To extract module name lists from object library module=object lists:
# 1.  Transform "module=[.dest]name.OBJ" into "module=[.dest] name".
# 2.  For a subdirectory, add "[.subdir]".
# 3.  Delete "*]" words.

# Complete list of C object dependency file names.


#    CDDA2WAV.
				
.IFDEF MODS_OBJS_LIB_CDDA2WAV   # MODS_OBJS_LIB_CDDA2WAV

SUBSIDIARY = 1

MODS_LIB_CDDA2WAV = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] *, $(MODS_OBJS_LIB_CDDA2WAV)))

DEPS = $(FOREACH NAME, $(MODS_LIB_CDDA2WAV), $(NAME).MMSD)

.ENDIF                          # MODS_OBJS_LIB_CDDA2WAV


#    CDRECORD.

.IFDEF MODS_OBJS_LIB_CDRECORD   # MODS_OBJS_LIB_CDRECORD

SUBSIDIARY = 1

MODS_LIB_CDRECORD = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] *, $(MODS_OBJS_LIB_CDRECORD)))

DEPS = $(FOREACH NAME, $(MODS_LIB_CDRECORD), $(NAME).MMSD)

.ENDIF                          # MODS_OBJS_LIB_CDRECORD


#    INC.

.IFDEF OBJS_ALIGN_TEST          # OBJS_ALIGN_TEST

SUBSIDIARY = 1

MODS_INC = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] *, $(OBJS_ALIGN_TEST) $(OBJS_AVOFFSET)))

DEPS = $(FOREACH NAME, $(MODS_INC), $(NAME).MMSD)

.ENDIF                          # OBJS_ALIGN_TEST


#    LIBDEFLT.

.IFDEF MODS_OBJS_LIB_DEFLT      # MODS_OBJS_LIB_DEFLT

SUBSIDIARY = 1

MODS_LIB_DEFLT = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] *, $(MODS_OBJS_LIB_DEFLT)))

DEPS = $(FOREACH NAME, $(MODS_LIB_DEFLT), $(NAME).MMSD)

.ENDIF                          # MODS_OBJS_LIB_DEFLT


#    LIBEDC.

.IFDEF MODS_OBJS_LIB_EDC        # MODS_OBJS_LIB_EDC

SUBSIDIARY = 1

MODS_LIB_EDC = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] *, $(MODS_OBJS_LIB_EDC)))

DEPS = $(FOREACH NAME, $(MODS_LIB_EDC), $(NAME).MMSD)

.ENDIF                          # MODS_OBJS_LIB_EDC


#    LIBFILE.

.IFDEF MODS_OBJS_LIB_FILE       # MODS_OBJS_LIB_FILE

SUBSIDIARY = 1

MODS_LIB_FILE = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] *, $(MODS_OBJS_LIB_FILE)))

DEPS = $(FOREACH NAME, $(MODS_LIB_FILE), $(NAME).MMSD)

.ENDIF                          # MODS_OBJS_LIB_FILE


#    LIBFIND.

.IFDEF MODS_OBJS_LIB_FIND       # MODS_OBJS_LIB_FIND

SUBSIDIARY = 1

MODS_LIB_FIND = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] *, $(MODS_OBJS_LIB_FIND)))

DEPS = $(FOREACH NAME, $(MODS_LIB_FIND), $(NAME).MMSD)

.ENDIF                          # MODS_OBJS_LIB_FIND


#    LIBHFS_ISO.

.IFDEF MODS_OBJS_LIB_HFS_ISO    # MODS_OBJS_LIB_HFS_ISO

SUBSIDIARY = 1

MODS_LIB_HFS_ISO = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] *, $(MODS_OBJS_LIB_HFS_ISO)))

DEPS = $(FOREACH NAME, $(MODS_LIB_HFS_ISO), $(NAME).MMSD)

.ENDIF                          # MODS_OBJS_LIB_HFS_ISO


#    LIBPARANOIA.

.IFDEF MODS_OBJS_LIB_PARANOIA   # MODS_OBJS_LIB_PARANOIA

SUBSIDIARY = 1

MODS_LIB_PARANOIA = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] *, $(MODS_OBJS_LIB_PARANOIA)))

DEPS = $(FOREACH NAME, $(MODS_LIB_PARANOIA), $(NAME).MMSD)

.ENDIF                          # MODS_OBJS_LIB_PARANOIA


#    LIBSCG.

.IFDEF MODS_OBJS_LIB_SCG        # MODS_OBJS_LIB_SCG

SUBSIDIARY = 1

MODS_LIB_SCG = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] *, $(MODS_OBJS_LIB_SCG)))

DEPS = $(FOREACH NAME, $(MODS_LIB_SCG), $(NAME).MMSD)

.ENDIF                          # MODS_OBJS_LIB_SCG


#    LIBSCGCHECK.

.IFDEF MODS_OBJS_LIB_SCGCHECK   # MODS_OBJS_LIB_SCGCHECK

SUBSIDIARY = 1

MODS_LIB_SCGCHECK = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] *, $(MODS_OBJS_LIB_SCGCHECK)))

DEPS = $(FOREACH NAME, $(MODS_LIB_SCGCHECK), $(NAME).MMSD)

.ENDIF                          # MODS_OBJS_LIB_SCGCHECK


#    LIBSCHILY.

.IFDEF MODS_OBJS_LIB_SCHILY     # MODS_OBJS_LIB_SCHILY

SUBSIDIARY = 1

MODS_LIB_SCHILY = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] *, $(MODS_OBJS_LIB_SCHILY)))

MODS_LIB_SCHILY_STDIO = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] [.STDIO]*, $(MODS_OBJS_LIB_SCHILY_STDIO)))

DEPS = $(FOREACH NAME, \
 $(MODS_LIB_SCHILY) $(MODS_LIB_SCHILY_STDIO), \
 $(NAME).MMSD)

.ENDIF                          # MODS_OBJS_LIB_SCHILY


#    LIBUNLS.

.IFDEF MODS_OBJS_LIB_UNLS       # MODS_OBJS_LIB_UNLS

SUBSIDIARY = 1

MODS_LIB_UNLS = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] *, $(MODS_OBJS_LIB_UNLS)))

DEPS = $(FOREACH NAME, $(MODS_LIB_UNLS), $(NAME).MMSD)

.ENDIF                          # MODS_OBJS_LIB_UNLS


#    LIBVMS.

.IFDEF MODS_OBJS_LIB_VMS        # MODS_OBJS_LIB_VMS

SUBSIDIARY = 1

MODS_LIB_VMS = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] *, $(MODS_OBJS_LIB_VMS)))

DEPS = $(FOREACH NAME, $(MODS_LIB_VMS), $(NAME).MMSD)

.ENDIF                          # MODS_OBJS_LIB_VMS


#    MKISOFS.

.IFDEF MODS_OBJS_LIB_MKISOFS    # MODS_OBJS_LIB_MKISOFS

SUBSIDIARY = 1

MODS_LIB_MKISOFS = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] *, $(MODS_OBJS_LIB_MKISOFS)))

MODS_LIB_MKISOFS_DIAG = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] [.DIAG]*, $(MODS_OBJS_LIB_MKISOFS_DIAG)))

MODS_EXE_MKISOFS_DIAG = $(FILTER-OUT *], \
 $(PATSUBST *]*.EXE, *] [.DIAG]*, $(MKISOFS_DIAG_EXE) ))

DEPS = $(FOREACH NAME, \
 $(MODS_LIB_MKISOFS) \
 $(MODS_LIB_MKISOFS_DIAG) \
 $(MODS_EXE_MKISOFS_DIAG), \
 $(NAME).MMSD)

.ENDIF                          # MODS_OBJS_LIB_MKISOFS


#    READCD.

.IFDEF MODS_OBJS_LIB_READCD     # MODS_OBJS_LIB_READCD

SUBSIDIARY = 1

MODS_LIB_READCD = $(FILTER-OUT *], \
 $(PATSUBST *]*.OBJ, *] *, $(MODS_OBJS_LIB_READCD)))

DEPS = $(FOREACH NAME, $(MODS_LIB_READCD), $(NAME).MMSD)

.ENDIF                          # MODS_OBJS_LIB_READCD


# Rules and actions depend on circumstances, main or subsidiary.

.IFDEF SUBSIDIARY               # SUBSIDIARY

# Default target is the comprehensive dependency list.

$(DEPS_FILE) : $(DEPS) $(COMS)
.IF UNK_MMSD                        # UNK_MMSD
	@ write sys$output -
 "   /SKIP_INTERMEDIATES is expected on the MMS command line."
	@ write sys$output -
 "   For normal behavior (delete individual .MMSD files), specify ""/SKIP""."
	@ write sys$output -
 "   To retain the individual .MMSD files, specify ""/MACRO = NOSKIP=1""."
	@ exit %x00000004
.ENDIF                              # UNK_MMSD
#
#       Note that the space in P3, which prevents immediate macro
#       expansion, is removed by COLLECT_DEPS.COM.
#
	@$(COLLECT_DEPS) "Cdrtools for VMS" "$(MMS$TARGET)" -
	 "[...]*.MMSD" "[.$ (DEST)]" $(MMSDESCRIPTION_FILE) -
	 "[-." $(DEST)
	@ write sys$output -
	 "Created a new dependency file: $(MMS$TARGET)"
.IF DELETE_MMSD                     # DELETE_MMSD
	@ write sys$output -
	 "Deleting intermediate .MMSD files..."
	if (f$search( "*.MMSD") .nes. "") then -
	 delete /log *.MMSD;*
.ELSE                               # DELETE_MMSD
	@ write sys$output -
	 "Purging intermediate .MMSD files..."
	if (f$search( "*.MMSD") .nes. "") then -
	 purge /keep = 2 /log *.MMSD
.ENDIF                              # DELETE_MMSD [else]

# CLEAN target.  Delete the individual C dependency files.

CLEAN :
	if (f$search( "[...]*.MMSD") .nes. "") then -
	 delete [...]*.MMSD;*

# CLEAN_ALL target.  Delete:
#    The individual C dependency files.
#    The collected source dependency file.

CLEAN_ALL :
	if (f$search( "[...]*.MMSD") .nes. "") then -
	 delete [...]*.MMSD;*
	if (f$search( "DESCRIP_DEPS.MMS") .nes. "") then -
	 delete DESCRIP_DEPS.MMS;*

.ELSE                           # SUBSIDIARY

#
# Main target is the specified target, everywhere.
#
# Note that the first actions use the normal description file to create
# some generated header files, before they are referenced.
#
# Generated headers must not be generated for any of the CLEAN* targets,
# because the CLEAN* actions may remove the required
# [.INC]DESCRIP_DEPS.MMS file.  The trick/mess here using "TARGET_xxx"
# does the job.
#
TARGET_CLEAN = X
TARGET_CLEAN_ALL = X
TARGET_CLEAN_EXE = X
TARGET_CLEAN_OLB = X

.IFDEF TARGET_$(MMSTARGETS)         # TARGET_xxx
.ELSE                               # TARGET_xxx
MAKE_GENERATED_HEADERS = X
.ENDIF                              # TARGET_xxx [else]

ALL, CLEAN, CLEAN_ALL, CLEAN_EXE, CLEAN_OLB :
	set default 'f$parse( "$(MMSDESCRIPTION_FILE)", , , "DIRECTORY")'
	show default
	@ write sys$output ""
	$(MMS) /description = [-.VMS]DESCRIP.MMS $(MMSQUALIFIERS) -
	 GENERATED_HEADERS
	set default [-.$(DIR_INC)]
	show default
	@ write sys$output ""
	$(MMS) /description = $(MMSDESCRIPTION_FILE) $(MMSQUALIFIERS) -
	 $(MMSTARGETS)
.IFDEF MAKE_GENERATED_HEADERS       # MAKE_GENERATED_HEADERS
	$(MMS) /description = [-.VMS]DESCRIP.MMS $(MMSQUALIFIERS) -
	 $(MMSTARGETS)
.ENDIF                              # MAKE_GENERATED_HEADERS
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

.ENDIF                          # SUBSIDIARY [else]

