dnl @(#)aclocal.m4	1.109 17/06/28 Copyright 1998-2015 J. Schilling

dnl Set VARIABLE to VALUE in C-string form, verbatim, or 1.
dnl AC_DEFINE_STRING(VARIABLE [, VALUE])
define(AC_DEFINE_STRING,
[cat >> confdefs.h <<\EOF
[#define] $1 ifelse($#, 2, "[$2]", $#, 3, "[$2]", 1)
EOF
])

dnl Similar, but perform shell substitutions $ ` \ once on VALUE.
define(AC_DEFINE_UNQUOTED_STRING,
[cat >> confdefs.h <<EOF
[#define] $1 ifelse($#, 2, "[$2]", $#, 3, "[$2]", 1)
EOF
])

dnl ==== Check if we have  typedef

dnl AC_HAVE_TYPE(INCLUDES, TYPE)
AC_DEFUN(AC_HAVE_TYPE,
[AC_REQUIRE([AC_HEADER_STDC])dnl
AC_MSG_CHECKING(for $2)
AC_CACHE_VAL(ac_cv_have_type_$2,
[AC_EGREP_CPP(dnl
changequote(<<,>>)dnl
<<(^|[^a-zA-Z_0-9])$2[^a-zA-Z_0-9]>>dnl
changequote([,]), [#include <sys/types.h>
#if	HAVE_STDLIB_H || STDC_HEADERS
#include <stdlib.h>
#endif
#if	HAVE_STDDEF_H || STDC_HEADERS
#include <stddef.h>
#endif
$1], eval ac_cv_have_type_$2=yes, eval ac_cv_have_type_$2=no)])dnl
AC_MSG_RESULT($ac_cv_have_type_$2)
changequote(, )dnl
  ac_tr_type=HAVE_TYPE_`echo $2 | sed 'y%abcdefghijklmnopqrstuvwxyz./-%ABCDEFGHIJKLMNOPQRSTUVWXYZ___%'`
changequote([, ])dnl
if test $ac_cv_have_type_$2 = yes; then
  AC_DEFINE_UNQUOTED($ac_tr_type)
fi
])

AC_DEFUN(AC_ICONV_CONST,
[AC_CACHE_CHECK(whether iconv() uses const char **, ac_cv_iconv_const,
[cat > conftest.c <<EOF
#include <iconv.h>
EOF
if AC_TRY_COMMAND(${CC-cc} -E conftest.c) | grep 'size_t.*iconv.*iconv_t.*const' >/dev/null 2>&1; then
  ac_cv_iconv_const=yes
else
  ac_cv_iconv_const=no
fi])
if test $ac_cv_iconv_const = yes; then
  AC_DEFINE(HAVE_ICONV_CONST)
fi])

AC_DEFUN(AC_PROG_CPPX,
[AC_REQUIRE([AC_PROG_CPP])dnl
AC_MSG_CHECKING(how to run the C preprocessor for any type of file)
if test -z "$CPPX"; then
AC_CACHE_VAL(ac_cv_prog_CPPX,
[  # This must be in double quotes, not single quotes, because CPP may get
  # substituted into the Makefile and "${CC-cc}" will confuse make.
  CPPX="$CPP"
	cat > conftestcpp << EOF
	xxzzy
EOF
	# gcc -E does not accept any file suffix, so we need to test
	# and if $CC -E fails, we try to use dirname(which($CC))/cpp
	# We cannot use AC_TRY_EVAL(CPPX conftestcpp | grep xxzzy) because
	# of a bug in the parser from bash
	ac_result=`(echo configure:1288: "$CPPX conftestcpp | grep xxzzy" 1>&5; eval $CPPX conftestcpp | grep xxzzy) 2>&5`
	if test -z "$ac_result"; then
		changequote(, )dnl
		ac_file=`eval type ${CC-cc} 2>/dev/null | sed 's%[^/]*/%/%'`
		# Remove last slash and all that follows it.  Not all systems have dirname.
		ac_dir=`echo "$ac_file" | sed 's%/[^/][^/]*$%%'`
		changequote([, ])dnl
		if test -f "$ac_dir"/cpp; then
			CPPX="$ac_dir"/cpp
		fi
	fi
	ac_cv_prog_CPPX="$CPPX"])dnl
	CPPX="$ac_cv_prog_CPPX"
else
	ac_cv_prog_CPPX="$CPPX"
fi
AC_MSG_RESULT($CPPX)
AC_SUBST(CPPX)dnl
])

dnl Checks if /bin/sh is bash
dnl Defines BIN_SHELL_IS_BASH on success.
AC_DEFUN([AC_BIN_SHELL_BASH],
[AC_CACHE_CHECK([if /bin/sh is bash], ac_cv_bin_shell_is_bash,
                [
ac_err=`< /dev/null /bin/sh --version 2> /dev/null | egrep 'GNU sh|bash'`
if test -n "$ac_err"; then
	ac_cv_bin_shell_is_bash=yes
else
	ac_cv_bin_shell_is_bash=no
fi
])
if test $ac_cv_bin_shell_is_bash = yes; then
  AC_DEFINE(BIN_SHELL_IS_BASH)
fi])

dnl Checks if sh is bash
dnl Defines SHELL_IS_BASH on success.
AC_DEFUN([AC_SHELL_BASH],
[AC_CACHE_CHECK([if sh is bash], ac_cv_shell_is_bash,
                [
ac_err=`< /dev/null sh --version 2> /dev/null | egrep 'GNU sh|bash'`
if test -n "$ac_err"; then
	ac_cv_shell_is_bash=yes
else
	ac_cv_shell_is_bash=no
fi
])
if test $ac_cv_shell_is_bash = yes; then
  AC_DEFINE(SHELL_IS_BASH)
fi])

dnl Checks if /bin/sh -ce is broken
dnl Defines BIN_SHELL_CE_IS_BROKEN on success.
AC_DEFUN([AC_BIN_SHELL_CE_BROKEN],
[AC_CACHE_CHECK([whether /bin/sh -ce is broken], ac_cv_bin_shell_ce_is_broken,
                [
ac_err=`/bin/sh -ce 'for i in 1 2 3; do  ( echo $i; if test -d . ; then (false; echo 4);  fi ) ; done' | grep 2`
if test -n "$ac_err"; then
	ac_cv_bin_shell_ce_is_broken=yes
else
	ac_cv_bin_shell_ce_is_broken=no
fi
])
if test $ac_cv_bin_shell_ce_is_broken = yes; then
  AC_DEFINE(BIN_SHELL_CE_IS_BROKEN)
fi])

dnl Checks if /bin/bosh is a working shell
dnl Defines BIN_SHELL_BOSH on success.
AC_DEFUN([AC_BIN_SHELL_BOSH],
[AC_CACHE_CHECK([whether /bin/bosh is a working shell], ac_cv_bin_shell_bosh,
                [
ac_err=`< /dev/null /bin/bosh -c 'echo abc' 2> /dev/null | grep abc`
if test "$ac_err" != "abc"; then
	ac_cv_bin_shell_bosh=no
else
	ac_err=`/bin/bosh -ce 'for i in 1 2 3; do  ( echo $i; if test -d . ; then (false; echo 4);  fi ) ; done' | grep 2`
	if test -z "$ac_err"; then
		ac_cv_bin_shell_bosh=yes
	else
		ac_cv_bin_shell_bosh=no
	fi
fi
])
if test $ac_cv_bin_shell_bosh = yes; then
  AC_DEFINE(BIN_SHELL_BOSH)
fi])

dnl Checks if /opt/schily/bin/bosh is a working shell
dnl Defines OPT_SCHILY_BIN_SHELL_BOSH on success.
AC_DEFUN([AC_OPT_SCHILY_BIN_SHELL_BOSH],
[AC_CACHE_CHECK([whether /opt/schily/bin/bosh is a working shell], ac_cv_opt_schily_bin_shell_bosh,
                [
ac_err=`< /dev/null /opt/schily/bin/bosh -c 'echo abc' 2> /dev/null | grep abc`
if test "$ac_err" != "abc"; then
	ac_cv_opt_schily_bin_shell_bosh=no
else
	ac_err=`/opt/schily/bin/bosh -ce 'for i in 1 2 3; do  ( echo $i; if test -d . ; then (false; echo 4);  fi ) ; done' | grep 2`
	if test -z "$ac_err"; then
		ac_cv_opt_schily_bin_shell_bosh=yes
	else
		ac_cv_opt_schily_bin_shell_bosh=no
	fi
fi
])
if test $ac_cv_opt_schily_bin_shell_bosh = yes; then
  AC_DEFINE(OPT_SCHILY_BIN_SHELL_BOSH)
fi])

dnl Checks if sh -ce is broken
dnl Defines SHELL_CE_IS_BROKEN on success.
AC_DEFUN([AC_SHELL_CE_BROKEN],
[AC_CACHE_CHECK([whether sh -ce is broken], ac_cv_shell_ce_is_broken,
                [
ac_err=`sh -ce 'for i in 1 2 3; do  ( echo $i; if test -d . ; then (false; echo 4);  fi ) ; done' | grep 2`
if test -n "$ac_err"; then
	ac_cv_shell_ce_is_broken=yes
else
	ac_cv_shell_ce_is_broken=no
fi
])
if test $ac_cv_shell_ce_is_broken = yes; then
  AC_DEFINE(SHELL_CE_IS_BROKEN)
fi])

dnl Checks if Sun cc supports -m64
dnl Defines sun_cc64_opt on success.
AC_DEFUN([AC_SUN_CC64_OPT],
[AC_CACHE_CHECK([if suncc/cc supports -m64], ac_cv_sun_cc64_opt,
                [
sun_cc64=cc
sun_cc64_opt=''
ac_cv_sun_cc64_opt=no
if test "$GCC" != yes; then
	if test "$CC" = suncc; then
		sun_cc64=suncc
	fi
	ac_err=`< /dev/null eval $sun_cc64 -m64 -c 2>&1 | grep illegal`
	if test -n "$ac_err"; then
		ac_cv_sun_cc64_opt=no
	else
		ac_cv_sun_cc64_opt=yes
	fi
fi
])
if test $ac_cv_sun_cc64_opt = yes; then
  sun_cc64_opt='-m64'
fi])

dnl Checks if HP cc supports -Ae
dnl Defines hp_cc_ansi_opt on success.
AC_DEFUN([AC_HP_CC_ANSI_OPT],
[AC_CACHE_CHECK([if HP cc supports -Ae], ac_cv_hp_cc_ansi_opt,
                [
hp_cc=cc
hp_cc_ansi_opt=''
ac_cv_hp_cc_ansi_opt=no
if test "$GCC" != yes; then
	os_name=`(uname -s) 2> /dev/null`
	if test ."$os_name" = .HP-UX ; then
		ac_err=`< /dev/null eval $hp_cc -Ae -c 2>&1 | grep 'Bundled.*option is available only'`
		if test -n "$ac_err"; then
			ac_cv_hp_cc_ansi_opt=no
		else
			ac_cv_hp_cc_ansi_opt=yes
		fi
	fi
fi
])
if test $ac_cv_hp_cc_ansi_opt = yes; then
  hp_cc_ansi_opt='-Ae'
fi])

dnl Checks if HP cc supports -O
dnl Defines hp_cc_opt_opt on success.
AC_DEFUN([AC_HP_CC_OPT_OPT],
[AC_CACHE_CHECK([if HP cc supports -O], ac_cv_hp_cc_opt_opt,
                [
hp_cc=cc
hp_cc_opt_opt=''
ac_cv_hp_cc_opt_opt=no
if test "$GCC" != yes; then
	os_name=`(uname -s) 2> /dev/null`
	if test ."$os_name" = .HP-UX ; then
		ac_err=`< /dev/null eval $hp_cc -O -c 2>&1 | grep 'Bundled.*option is available only'`
		if test -n "$ac_err"; then
			ac_cv_hp_cc_opt_opt=no
		else
			ac_cv_hp_cc_opt_opt=yes
		fi
	fi
fi
])
if test $ac_cv_hp_cc_opt_opt = yes; then
  hp_cc_opt_opt='-O'
fi])

dnl Checks if HP cc supports -G
dnl Defines hp_cc_gprof_opt on success.
AC_DEFUN([AC_HP_CC_GPROF_OPT],
[AC_CACHE_CHECK([if HP cc supports -G], ac_cv_hp_cc_gprof_opt,
                [
hp_cc=cc
hp_cc_gprof_opt=''
ac_cv_hp_cc_gprof_opt=no
if test "$GCC" != yes; then
	os_name=`(uname -s) 2> /dev/null`
	if test ."$os_name" = .HP-UX ; then
		ac_err=`< /dev/null eval $hp_cc -O -c 2>&1 | grep 'Bundled.*option is available only'`
		if test -n "$ac_err"; then
			ac_cv_hp_cc_gprof_opt=no
		else
			ac_cv_hp_cc_gprof_opt=yes
		fi
	fi
fi
])
if test $ac_cv_hp_cc_gprof_opt = yes; then
  hp_cc_gprof_opt='-G'
fi])

dnl XXX this used to be:
dnl #ifndef $2 
dnl	char *p = (char *) $2; 
dnl #endif
dnl but we use this test in order to check whether we are able to get the
dnl address of a function from this name, so we did replace this by:
dnl  char *p = (char *) $2;
dnl  exit (p != (char *)0 && *p != 0);	Prevent optimizer from removing previous line
dnl 
dnl AC_CHECK_DECLARE(INCLUDES, SYMBOL)
dnl Checks if symbol is declared
dnl Defines HAVE_DECL_SYMBOL on success.
AC_DEFUN([AC_CHECK_DECLARE],
[AC_CACHE_CHECK([if $2 is declared], ac_cv_have_decl_$2,
                [AC_TRY_COMPILE([$1],
[ char *p = (char *) $2; exit (p != (char *)0 && *p != 0); ],
		[eval ac_cv_have_decl_$2=yes],
		[eval ac_cv_have_decl_$2=no])])
changequote(, )dnl
  ac_tr_decl=HAVE_DECL_`echo $2 | sed 'y%abcdefghijklmnopqrstuvwxyz./-%ABCDEFGHIJKLMNOPQRSTUVWXYZ___%'`
changequote([, ])dnl
if test $ac_cv_have_decl_$2 = yes; then
  AC_DEFINE_UNQUOTED($ac_tr_decl)
fi])

dnl AC_CHECK_DFUNC(INCLUDES, SYMBOL)
dnl Checks if symbol is defined or a function
dnl Defines HAVE_SYMBOL on success.
AC_DEFUN([AC_CHECK_DFUNC],
[AC_CACHE_CHECK([if $2 is defined or function], ["ac_cv_have_$2"],
                [AC_TRY_LINK([$1],
[
#ifndef $2
	char *p = (char *) $2;
	exit (p != (char *)0 && *p != 0);
#endif],
		[eval ac_cv_have_$2=yes],
		[eval ac_cv_have_$2=no])])
changequote(, )dnl
  ac_tr_dfunc=HAVE_`echo $2 | sed 'y%abcdefghijklmnopqrstuvwxyz./-%ABCDEFGHIJKLMNOPQRSTUVWXYZ___%'`
changequote([, ])dnl
if eval "test \"`echo '$ac_cv_have_'$2`\" = yes"; then
  AC_DEFINE_UNQUOTED($ac_tr_dfunc)
fi
])

dnl AC_CHECK_DFUNCS(INCLUDES, FUNCTION...)
AC_DEFUN(AC_CHECK_DFUNCS,
[for ac_func in [$2]
do
AC_CHECK_DFUNC([$1], $ac_func)dnl
done
])
 
dnl Checks whether symbol is defined or a function in a lib
dnl AC_CHECK_DLIB(INCLUDES, LIBRARY, FUNCTION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND
dnl              [, OTHER-LIBRARIES]]])
AC_DEFUN(AC_CHECK_DLIB,
[AC_MSG_CHECKING([for $3 in -l$2])
dnl Use a cache variable name containing both the library and function name,
dnl because the test really is for library $2 defining function $3, not
dnl just for library $2.  Separate tests with the same $2 and different $3s
dnl may have different results.
ac_lib_var=`echo $2['_']$3 | sed 'y%./+-%__p_%'`
AC_CACHE_VAL(ac_cv_lib_$ac_lib_var,
[ac_save_LIBS="$LIBS"
LIBS="-l$2 $6 $LIBS"
AC_TRY_LINK([$1],
[
#ifndef $3
	char *p = (char *) $3;
	exit (p != (char *)0 && *p != 0);
#endif],
	    eval "ac_cv_lib_$ac_lib_var=yes",
	    eval "ac_cv_lib_$ac_lib_var=no")
LIBS="$ac_save_LIBS"
])dnl
if eval "test \"`echo '$ac_cv_lib_'$ac_lib_var`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$4], ,
[changequote(, )dnl
  ac_tr_lib=HAVE_LIB`echo $2 | sed -e 's/[^a-zA-Z0-9_]/_/g' \
    -e 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/'`
changequote([, ])dnl
  AC_DEFINE_UNQUOTED($ac_tr_lib)
  LIBS="-l$2 $LIBS"
], [$4])
else
  AC_MSG_RESULT(no)
ifelse([$5], , , [$5
])dnl
fi
])

dnl Checks whether symbol is in a lib
dnl this does not work for void func()
dnl AC_CHECK_ILIB(INCLUDES, LIBRARY, FUNCTION [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND
dnl              [, OTHER-LIBRARIES]]])
AC_DEFUN(AC_CHECK_ILIB,
[AC_MSG_CHECKING([for $3 in -l$2])
dnl Use a cache variable name containing both the library and function name,
dnl because the test really is for library $2 defining function $3, not
dnl just for library $2.  Separate tests with the same $2 and different $3s
dnl may have different results.
ac_lib_var=`echo $2['_']$3 | sed 'y%./+-%__p_%'`
AC_CACHE_VAL(ac_cv_lib_$ac_lib_var,
[ac_save_LIBS="$LIBS"
LIBS="-l$2 $6 $LIBS"
AC_TRY_LINK([$1],
[
	char *p = (char *) $3;

	return ((int)p);
],
	    eval "ac_cv_lib_$ac_lib_var=yes",
	    eval "ac_cv_lib_$ac_lib_var=no")
LIBS="$ac_save_LIBS"
])dnl
if eval "test \"`echo '$ac_cv_lib_'$ac_lib_var`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$4], ,
[changequote(, )dnl
  ac_tr_lib=HAVE_LIB`echo $2 | sed -e 's/[^a-zA-Z0-9_]/_/g' \
    -e 'y/abcdefghijklmnopqrstuvwxyz/ABCDEFGHIJKLMNOPQRSTUVWXYZ/'`
changequote([, ])dnl
  AC_DEFINE_UNQUOTED($ac_tr_lib)
  LIBS="-l$2 $LIBS"
], [$4])
else
  AC_MSG_RESULT(no)
ifelse([$5], , , [$5
])dnl
fi
])

dnl getpagesize is a inline function in unistd.h on Android
AC_DEFUN([AC_FUNC_GETPAGESIZE],
[AC_CHECK_DFUNC([
#ifdef	HAVE_UNISTD_H
#include <unistd.h>
#endif
], getpagesize)])


dnl Checks if structure 'stat' have field 'st_spare1'.
dnl Defines HAVE_ST_SPARE1 on success.
AC_DEFUN([AC_STRUCT_ST_SPARE1],
[AC_CACHE_CHECK([if struct stat contains st_spare1], ac_cv_struct_st_spare1,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/stat.h>],
                                [struct  stat s; s.st_spare1 = 0;],
                                [ac_cv_struct_st_spare1=yes],
                                [ac_cv_struct_st_spare1=no])])
if test $ac_cv_struct_st_spare1 = yes; then
  AC_DEFINE(HAVE_ST_SPARE1)
fi])

dnl Checks if structure 'stat' have field 'st_atimensec'.
dnl Defines HAVE_ST_ATIMENSEC on success.
AC_DEFUN([AC_STRUCT_ST_ATIMENSEC],
[AC_CACHE_CHECK([if struct stat contains st_atimensec], ac_cv_struct_st_atimensec,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/stat.h>],
                                [struct  stat s; s.st_atimensec = 0;],
                                [ac_cv_struct_st_atimensec=yes],
                                [ac_cv_struct_st_atimensec=no])])
if test $ac_cv_struct_st_atimensec = yes; then
  AC_DEFINE(HAVE_ST_ATIMENSEC)
fi])

dnl Checks if structure 'stat' have field 'st_atime_n'.
dnl Defines HAVE_ST_ATIME_N on success.
AC_DEFUN([AC_STRUCT_ST_ATIME_N],
[AC_CACHE_CHECK([if struct stat contains st_atime_n], ac_cv_struct_st_atime_n,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/stat.h>],
                                [struct  stat s; s.st_atime_n = 0;],
                                [ac_cv_struct_st_atime_n=yes],
                                [ac_cv_struct_st_atime_n=no])])
if test $ac_cv_struct_st_atime_n = yes; then
  AC_DEFINE(HAVE_ST_ATIME_N)
fi])

dnl Checks if structure 'stat' have field 'st_atim.tv_nsec'.
dnl Defines HAVE_ST_NSEC on success.
AC_DEFUN([AC_STRUCT_ST_NSEC],
[AC_CACHE_CHECK([if struct stat contains st_atim.tv_nsec], ac_cv_struct_st_nsec,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/stat.h>],
                                [struct  stat s; s.st_atim.tv_nsec = 0;],
                                [ac_cv_struct_st_nsec=yes],
                                [ac_cv_struct_st_nsec=no])])
if test $ac_cv_struct_st_nsec = yes; then
  AC_DEFINE(HAVE_ST_NSEC)
fi])

dnl Checks if structure 'stat' have field 'st_atim.st__tim.tv_nsec'.
dnl Defines HAVE_ST__TIM on success.
AC_DEFUN([AC_STRUCT_ST__TIM],
[AC_CACHE_CHECK([if struct stat contains st_atim.st__tim.tv_nsec], ac_cv_struct_st__tim,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/stat.h>],
                                [struct  stat s; s.st_atim.st__tim.tv_nsec = 0;],
                                [ac_cv_struct_st__tim=yes],
                                [ac_cv_struct_st__tim=no])])
if test $ac_cv_struct_st__tim = yes; then
  AC_DEFINE(HAVE_ST__TIM)
fi])

dnl Checks if structure 'stat' have field 'st_atimspec.tv_nsec'.
dnl Defines HAVE_ST_ATIMESPEC on success.
AC_DEFUN([AC_STRUCT_ST_ATIMESPEC],
[AC_CACHE_CHECK([if struct stat contains st_atimespec.tv_nsec], ac_cv_struct_st_atimespec,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/stat.h>],
                                [struct  stat s; s.st_atimespec.tv_nsec = 0;],
                                [ac_cv_struct_st_atimespec=yes],
                                [ac_cv_struct_st_atimespec=no])])
if test $ac_cv_struct_st_atimespec = yes; then
  AC_DEFINE(HAVE_ST_ATIMESPEC)
fi])

dnl Checks if structure 'stat' have field 'st_flag'.
dnl Defines HAVE_ST_FLAG on success.
AC_DEFUN([AC_STRUCT_ST_FLAG],
[AC_CACHE_CHECK([if struct stat contains st_flag], ac_cv_struct_st_flag,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/stat.h>],
                                [struct  stat s; s.st_flag = 0;],
                                [ac_cv_struct_st_flag=yes],
                                [ac_cv_struct_st_flag=no])])
if test $ac_cv_struct_st_flag = yes; then
  AC_DEFINE(HAVE_ST_FLAG)
fi])

dnl Checks if structure 'stat' have field 'st_flags'.
dnl Defines HAVE_ST_FLAGS on success.
AC_DEFUN([AC_STRUCT_ST_FLAGS],
[AC_CACHE_CHECK([if struct stat contains st_flags], ac_cv_struct_st_flags,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/stat.h>],
                                [struct  stat s; s.st_flags = 0;],
                                [ac_cv_struct_st_flags=yes],
                                [ac_cv_struct_st_flags=no])])
if test $ac_cv_struct_st_flags = yes; then
  AC_DEFINE(HAVE_ST_FLAGS)
fi])

dnl Checks if structure 'stat' have field 'st_fstype'.
dnl Defines HAVE_ST_FSTYPE on success.
AC_DEFUN([AC_STRUCT_ST_FSTYPE],
[AC_CACHE_CHECK([if struct stat contains st_fstype], ac_cv_struct_st_fstype,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/stat.h>],
                                [struct  stat s; s.st_fstype[0] = 0;],
                                [ac_cv_struct_st_fstype=yes],
                                [ac_cv_struct_st_fstype=no])])
if test $ac_cv_struct_st_fstype = yes; then
  AC_DEFINE(HAVE_ST_FSTYPE)
fi])

dnl Checks if structure 'stat' have field 'st_aclcnt'.
dnl Defines HAVE_ST_ACLCNT on success.
AC_DEFUN([AC_STRUCT_ST_ACLCNT],
[AC_CACHE_CHECK([if struct stat contains st_aclcnt], ac_cv_struct_st_aclcnt,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/stat.h>],
                                [struct  stat s; s.st_aclcnt = 0;],
                                [ac_cv_struct_st_aclcnt=yes],
                                [ac_cv_struct_st_aclcnt=no])])
if test $ac_cv_struct_st_aclcnt = yes; then
  AC_DEFINE(HAVE_ST_ACLCNT)
fi])

dnl Checks if structure 'utsname' have field 'arch'.
dnl Defines HAVE_UTSNAME_ARCH on success.
AC_DEFUN([AC_STRUCT_UTSNAME_ARCH],
[AC_CACHE_CHECK([if struct utsname contains arch], ac_cv_struct_utsname_arch,
                [AC_TRY_COMPILE([#include <sys/utsname.h>],
                                [struct  utsname u; u.arch[0] = 0;],
                                [ac_cv_struct_utsname_arch=yes],
                                [ac_cv_struct_utsname_arch=no])])
if test $ac_cv_struct_utsname_arch = yes; then
  AC_DEFINE(HAVE_UTSNAME_ARCH)
fi])

dnl Checks if structure 'utsname' have field 'processor'.
dnl Defines HAVE_UTSNAME_PROCESSOR on success.
AC_DEFUN([AC_STRUCT_UTSNAME_PROCESSOR],
[AC_CACHE_CHECK([if struct utsname contains processor], ac_cv_struct_utsname_processor,
                [AC_TRY_COMPILE([#include <sys/utsname.h>],
                                [struct  utsname u; u.processor[0] = 0;],
                                [ac_cv_struct_utsname_processor=yes],
                                [ac_cv_struct_utsname_processor=no])])
if test $ac_cv_struct_utsname_processor = yes; then
  AC_DEFINE(HAVE_UTSNAME_PROCESSOR)
fi])

dnl Checks if structure 'utsname' have field 'sysname_host'.
dnl Defines HAVE_UTSNAME_SYSNAME_HOST on success.
AC_DEFUN([AC_STRUCT_UTSNAME_SYSNAME_HOST],
[AC_CACHE_CHECK([if struct utsname contains sysname_host], ac_cv_struct_utsname_sysname_host,
                [AC_TRY_COMPILE([#include <sys/utsname.h>],
                                [struct  utsname u; u.sysname_host[0] = 0;],
                                [ac_cv_struct_utsname_sysname_host=yes],
                                [ac_cv_struct_utsname_sysname_host=no])])
if test $ac_cv_struct_utsname_sysname_host = yes; then
  AC_DEFINE(HAVE_UTSNAME_SYSNAME_HOST)
fi])

dnl Checks if structure 'utsname' have field 'release_host'.
dnl Defines HAVE_UTSNAME_RELEASE_HOST on success.
AC_DEFUN([AC_STRUCT_UTSNAME_RELEASE_HOST],
[AC_CACHE_CHECK([if struct utsname contains release_host], ac_cv_struct_utsname_release_host,
                [AC_TRY_COMPILE([#include <sys/utsname.h>],
                                [struct  utsname u; u.release_host[0] = 0;],
                                [ac_cv_struct_utsname_release_host=yes],
                                [ac_cv_struct_utsname_release_host=no])])
if test $ac_cv_struct_utsname_release_host = yes; then
  AC_DEFINE(HAVE_UTSNAME_RELEASE_HOST)
fi])

dnl Checks if structure 'utsname' have field 'version_host'.
dnl Defines HAVE_UTSNAME_VERSION_HOST on success.
AC_DEFUN([AC_STRUCT_UTSNAME_VERSION_HOST],
[AC_CACHE_CHECK([if struct utsname contains version_host], ac_cv_struct_utsname_version_host,
                [AC_TRY_COMPILE([#include <sys/utsname.h>],
                                [struct  utsname u; u.version_host[0] = 0;],
                                [ac_cv_struct_utsname_version_host=yes],
                                [ac_cv_struct_utsname_version_host=no])])
if test $ac_cv_struct_utsname_version_host = yes; then
  AC_DEFINE(HAVE_UTSNAME_VERSION_HOST)
fi])

dnl Checks if structure 'mtget' have field 'mt_type'.
dnl Defines HAVE_MTGET_TYPE on success.
AC_DEFUN([AC_STRUCT_MTGET_TYPE],
[AC_CACHE_CHECK([if struct mtget contains mt_type], ac_cv_struct_mtget_type,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/mtio.h>],
                                [struct  mtget t; t.mt_type = 0;],
                                [ac_cv_struct_mtget_type=yes],
                                [ac_cv_struct_mtget_type=no])])
if test $ac_cv_struct_mtget_type = yes; then
  AC_DEFINE(HAVE_MTGET_TYPE)
fi])

dnl Checks if structure 'mtget' have field 'mt_model'.
dnl Defines HAVE_MTGET_MODEL on success.
AC_DEFUN([AC_STRUCT_MTGET_MODEL],
[AC_CACHE_CHECK([if struct mtget contains mt_model], ac_cv_struct_mtget_model,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/mtio.h>],
                                [struct  mtget t; t.mt_model = 0;],
                                [ac_cv_struct_mtget_model=yes],
                                [ac_cv_struct_mtget_model=no])])
if test $ac_cv_struct_mtget_model = yes; then
  AC_DEFINE(HAVE_MTGET_MODEL)
fi])

dnl Checks if structure 'mtget' have field 'mt_dsreg'.
dnl Defines HAVE_MTGET_DSREG on success.
AC_DEFUN([AC_STRUCT_MTGET_DSREG],
[AC_CACHE_CHECK([if struct mtget contains mt_dsreg], ac_cv_struct_mtget_dsreg,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/mtio.h>],
                                [struct  mtget t; t.mt_dsreg = 0;],
                                [ac_cv_struct_mtget_dsreg=yes],
                                [ac_cv_struct_mtget_dsreg=no])])
if test $ac_cv_struct_mtget_dsreg = yes; then
  AC_DEFINE(HAVE_MTGET_DSREG)
fi])

dnl Checks if structure 'mtget' have field 'mt_dsreg1'.
dnl Defines HAVE_MTGET_DSREG on success.
AC_DEFUN([AC_STRUCT_MTGET_DSREG1],
[AC_CACHE_CHECK([if struct mtget contains mt_dsreg1], ac_cv_struct_mtget_dsreg1,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/mtio.h>],
                                [struct  mtget t; t.mt_dsreg1 = 0;],
                                [ac_cv_struct_mtget_dsreg1=yes],
                                [ac_cv_struct_mtget_dsreg1=no])])
if test $ac_cv_struct_mtget_dsreg1 = yes; then
  AC_DEFINE(HAVE_MTGET_DSREG1)
fi])

dnl Checks if structure 'mtget' have field 'mt_dsreg2'.
dnl Defines HAVE_MTGET_DSREG2 on success.
AC_DEFUN([AC_STRUCT_MTGET_DSREG2],
[AC_CACHE_CHECK([if struct mtget contains mt_dsreg2], ac_cv_struct_mtget_dsreg2,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/mtio.h>],
                                [struct  mtget t; t.mt_dsreg2 = 0;],
                                [ac_cv_struct_mtget_dsreg2=yes],
                                [ac_cv_struct_mtget_dsreg2=no])])
if test $ac_cv_struct_mtget_dsreg2 = yes; then
  AC_DEFINE(HAVE_MTGET_DSREG2)
fi])

dnl Checks if structure 'mtget' have field 'mt_gstat'.
dnl Defines HAVE_MTGET_GSTAT on success.
AC_DEFUN([AC_STRUCT_MTGET_GSTAT],
[AC_CACHE_CHECK([if struct mtget contains mt_gstat], ac_cv_struct_mtget_gstat,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/mtio.h>],
                                [struct  mtget t; t.mt_gstat = 0;],
                                [ac_cv_struct_mtget_gstat=yes],
                                [ac_cv_struct_mtget_gstat=no])])
if test $ac_cv_struct_mtget_gstat = yes; then
  AC_DEFINE(HAVE_MTGET_GSTAT)
fi])

dnl Checks if structure 'mtget' have field 'mt_erreg'.
dnl Defines HAVE_MTGET_ERREG on success.
AC_DEFUN([AC_STRUCT_MTGET_ERREG],
[AC_CACHE_CHECK([if struct mtget contains mt_erreg], ac_cv_struct_mtget_erreg,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/mtio.h>],
                                [struct  mtget t; t.mt_erreg = 0;],
                                [ac_cv_struct_mtget_erreg=yes],
                                [ac_cv_struct_mtget_erreg=no])])
if test $ac_cv_struct_mtget_erreg = yes; then
  AC_DEFINE(HAVE_MTGET_ERREG)
fi])

dnl Checks if structure 'mtget' have field 'mt_resid'.
dnl Defines HAVE_MTGET_RESID on success.
AC_DEFUN([AC_STRUCT_MTGET_RESID],
[AC_CACHE_CHECK([if struct mtget contains mt_resid], ac_cv_struct_mtget_resid,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/mtio.h>],
                                [struct  mtget t; t.mt_resid = 0;],
                                [ac_cv_struct_mtget_resid=yes],
                                [ac_cv_struct_mtget_resid=no])])
if test $ac_cv_struct_mtget_resid = yes; then
  AC_DEFINE(HAVE_MTGET_RESID)
fi])

dnl Checks if structure 'mtget' have field 'mt_fileno'.
dnl Defines HAVE_MTGET_FILENO on success.
AC_DEFUN([AC_STRUCT_MTGET_FILENO],
[AC_CACHE_CHECK([if struct mtget contains mt_fileno],
                ac_cv_struct_mtget_fileno,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/mtio.h>],
		                [struct  mtget t; t.mt_fileno = 0;],
	        	        [ac_cv_struct_mtget_fileno=yes],
                		[ac_cv_struct_mtget_fileno=no])])
if test $ac_cv_struct_mtget_fileno = yes; then
  AC_DEFINE(HAVE_MTGET_FILENO)
fi])

dnl Checks if structure 'mtget' have field 'mt_blkno'.
dnl Defines HAVE_MTGET_BLKNO on success.
AC_DEFUN([AC_STRUCT_MTGET_BLKNO],
[AC_CACHE_CHECK([if struct mtget contains mt_blkno], ac_cv_struct_mtget_blkno,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/mtio.h>],
                                [struct  mtget t; t.mt_blkno = 0;],
                                [ac_cv_struct_mtget_blkno=yes],
                                [ac_cv_struct_mtget_blkno=no])])
if test $ac_cv_struct_mtget_blkno = yes; then
  AC_DEFINE(HAVE_MTGET_BLKNO)
fi])

dnl Checks if structure 'mtget' have field 'mt_flags'.
dnl Defines HAVE_MTGET_FLAGS on success.
AC_DEFUN([AC_STRUCT_MTGET_FLAGS],
[AC_CACHE_CHECK([if struct mtget contains mt_flags], ac_cv_struct_mtget_flags,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/mtio.h>],
                                [struct  mtget t; t.mt_flags = 0;],
                                [ac_cv_struct_mtget_flags=yes],
                                [ac_cv_struct_mtget_flags=no])])
if test $ac_cv_struct_mtget_flags = yes; then
  AC_DEFINE(HAVE_MTGET_FLAGS)
fi])

dnl Checks if structure 'mtget' have field 'mt_bf'.
dnl Defines HAVE_MTGET_BF on success.
AC_DEFUN([AC_STRUCT_MTGET_BF],
[AC_CACHE_CHECK([if struct mtget contains mt_bf], ac_cv_struct_mtget_bf,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/mtio.h>],
                                [struct  mtget t; t.mt_bf = 0;],
                                [ac_cv_struct_mtget_bf=yes],
                                [ac_cv_struct_mtget_bf=no])])
if test $ac_cv_struct_mtget_bf = yes; then
  AC_DEFINE(HAVE_MTGET_BF)
fi])

dnl Checks for illegal declaration of 'union semun' in sys/sem.h.
dnl Defines HAVE_UNION_SEMUN on success.
AC_DEFUN([AC_STRUCT_UNION_SEMUN],
[AC_CACHE_CHECK([if an illegal declaration for union semun in sys/sem.h exists], ac_cv_struct_union_semun,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>], [union semun s;],
                [ac_cv_struct_union_semun=yes],
                [ac_cv_struct_union_semun=no])])
if test $ac_cv_struct_union_semun = yes; then
  AC_DEFINE(HAVE_UNION_SEMUN)
fi])

dnl Checks if 'union wait' is declared in 'wait.h' or 'sys/wait.h'.
dnl Defines HAVE_UNION_WAIT on success.
AC_DEFUN([AC_STRUCT_UNION_WAIT],
[AC_CACHE_CHECK([if union wait is declared in wait.h or sys/wait.h], ac_cv_struct_union_wait,
                [AC_TRY_COMPILE([#include <sys/types.h>
#if	defined(HAVE_WAIT_H)
#	include <wait.h>
#else
#include <sys/wait.h>
#endif], [union wait w;],
                [ac_cv_struct_union_wait=yes],
                [ac_cv_struct_union_wait=no])])
if test $ac_cv_struct_union_wait = yes; then
  AC_DEFINE(HAVE_UNION_WAIT)
fi])

dnl Checks if union wait is used by default
dnl Defines USE_UNION_WAIT to int on failure.
AC_DEFUN([AC_USE_STRUCT_UNION_WAIT],
[AC_REQUIRE([AC_HEADER_STDC])dnl
AC_MSG_CHECKING(if union wait is used by default)
AC_CACHE_VAL(ac_cv_use_union_wait,
[AC_EGREP_CPP(dnl
changequote(<<,>>)dnl
<<xxzzy.*x_|xxzzy.*union>>dnl
changequote([,]), [#include <sys/types.h>
#if	defined(HAVE_WAIT_H)
#	include <wait.h>
#else
#include <sys/wait.h>
#endif
xxzzy WTERMSIG(status)
xxzzy WCOREDUMP(status)
xxzzy WEXITSTATUS(status)
xxzzy WSTOPSIG(status)
xxzzy WIFSTOPPED(status)
xxzzy WIFSIGNALED(status)
xxzzy WIFEXITED(status)], ac_cv_use_union_wait=yes, ac_cv_use_union_wait=no)])dnl
AC_MSG_RESULT($ac_cv_use_union_wait)
if test $ac_cv_use_union_wait = yes; then
  AC_DEFINE(USE_UNION_WAIT)
fi])

dnl Checks if 'struct rusage' is declared in sys/resource.h.
dnl Defines HAVE_STRUCT_RUSAGE on success.
AC_DEFUN([AC_STRUCT_RUSAGE],
[AC_CACHE_CHECK([if struct rusage is declared in sys/resource.h], ac_cv_struct_rusage,
                [AC_TRY_COMPILE([#include <sys/time.h>
#include <sys/resource.h>], [struct rusage r;],
                [ac_cv_struct_rusage=yes],
                [ac_cv_struct_rusage=no])])
if test $ac_cv_struct_rusage = yes; then
  AC_DEFINE(HAVE_STRUCT_RUSAGE)
fi])

dnl Checks if structure 'siginfo' have field 'si_utime'.
dnl Defines HAVE_SI_UTIME on success.
AC_DEFUN([AC_STRUCT_SI_UTIME],
[AC_CACHE_CHECK([if struct siginfo contains si_utime], ac_cv_struct_si_utime,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/siginfo.h>],
                                [struct  siginfo si; si.si_utime = 0;],
                                [ac_cv_struct_si_utime=yes],
                                [ac_cv_struct_si_utime=no])])
if test $ac_cv_struct_si_utime = yes; then
  AC_DEFINE(HAVE_SI_UTIME)
fi])


dnl Checks if structure 'dirent' have field 'd_ino'.
dnl Defines HAVE_DIRENT_D_INO on success.
AC_DEFUN([AC_STRUCT_DIRENT_D_INO],
[AC_CACHE_CHECK([if struct dirent contains d_ino], ac_cv_struct_dirent_d_ino,
                [AC_TRY_COMPILE([
/*
 * This must be kept in sync with schily/dirent.h
 */
#ifdef	HAVE_SYS_TYPES_H
#	include <sys/types.h>
#endif
#ifdef	HAVE_SYS_STAT_H
#	include <sys/stat.h>
#endif
#	ifdef	HAVE_DIRENT_H		/* This a POSIX compliant system */
#		include <dirent.h>
#		define	_FOUND_DIR_
#	else				/* This is a Pre POSIX system	 */

#	define 	dirent	direct

#	if	defined(HAVE_SYS_DIR_H)
#		include <sys/dir.h>
#		define	_FOUND_DIR_
#	endif

#	if	defined(HAVE_NDIR_H) && !defined(_FOUND_DIR_)
#		include <ndir.h>
#		define	_FOUND_DIR_
#	endif

#	if	defined(HAVE_SYS_NDIR_H) && !defined(_FOUND_DIR_)
#		include <sys/ndir.h>
#		define	_FOUND_DIR_
#	endif
#	endif	/* HAVE_DIRENT_H */
				],
                                [struct  dirent d; d.d_ino = 0;],
                                [ac_cv_struct_dirent_d_ino=yes],
                                [ac_cv_struct_dirent_d_ino=no])])
if test $ac_cv_struct_dirent_d_ino = yes; then
  AC_DEFINE(HAVE_DIRENT_D_INO)
fi])

dnl Checks if structure 'dirent' have field 'd_type'.
dnl Defines HAVE_DIRENT_D_TYPE on success.
AC_DEFUN([AC_STRUCT_DIRENT_D_TYPE],
[AC_CACHE_CHECK([if struct dirent contains d_type], ac_cv_struct_dirent_d_type,
                [AC_TRY_COMPILE([
/*
 * This must be kept in sync with schily/dirent.h
 */
#ifdef	HAVE_SYS_TYPES_H
#	include <sys/types.h>
#endif
#ifdef	HAVE_SYS_STAT_H
#	include <sys/stat.h>
#endif
#	ifdef	HAVE_DIRENT_H		/* This a POSIX compliant system */
#		include <dirent.h>
#		define	_FOUND_DIR_
#	else				/* This is a Pre POSIX system	 */

#	define 	dirent	direct

#	if	defined(HAVE_SYS_DIR_H)
#		include <sys/dir.h>
#		define	_FOUND_DIR_
#	endif

#	if	defined(HAVE_NDIR_H) && !defined(_FOUND_DIR_)
#		include <ndir.h>
#		define	_FOUND_DIR_
#	endif

#	if	defined(HAVE_SYS_NDIR_H) && !defined(_FOUND_DIR_)
#		include <sys/ndir.h>
#		define	_FOUND_DIR_
#	endif
#	endif	/* HAVE_DIRENT_H */
				],
                                [struct  dirent d; d.d_type = 0;],
                                [ac_cv_struct_dirent_d_type=yes],
                                [ac_cv_struct_dirent_d_type=no])])
if test $ac_cv_struct_dirent_d_type = yes; then
  AC_DEFINE(HAVE_DIRENT_D_TYPE)
fi])

dnl Checks if structure 'DIR' have field 'dd_fd'.
dnl Defines HAVE_DIR_DD_FD on success.
AC_DEFUN([AC_STRUCT_DIR_DD_FD],
[AC_CACHE_CHECK([if DIR * contains dd_fd], ac_cv_struct_dir_dd_fd,
                [AC_TRY_COMPILE([#include <dirent.h>],
                                [DIR d; d.dd_fd = 0; exit (d.dd_fd == 0);],
                                [ac_cv_struct_dir_dd_fd=yes],
                                [ac_cv_struct_dir_dd_fd=no])])
if test $ac_cv_struct_dir_dd_fd = yes; then
  AC_DEFINE(HAVE_DIR_DD_FD)
fi])

dnl Checks wether major(), minor() and makedev() are defined in
dnl 'sys/mkdev.h' or in 'sys/sysmacros.h. Defines MAJOR_IN_MKDEV or
dnl MAJOR_IN_SYSMACROS or nothing.
AC_DEFUN([AC_HEADER_MAKEDEV],
[AC_CACHE_CHECK([for header file containing  major(), minor() and makedev()],
               ac_cv_header_makedev,
[ac_cv_header_makedev=none
AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/mkdev.h>],
               [int i = major(0); i = minor(0); i = makedev(0,0);],
                [ac_cv_header_makedev=sys/mkdev.h])
if test $ac_cv_header_makedev = none; then
  AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/sysmacros.h>],
                 [int i = major(0); i = minor(0); i = makedev(0,0);],
                 [ac_cv_header_makedev=sys/sysmacros.h])
fi])
if test $ac_cv_header_makedev = sys/mkdev.h; then
  AC_DEFINE(MAJOR_IN_MKDEV)
fi
if test $ac_cv_header_makedev = sys/sysmacros.h; then
  AC_DEFINE(MAJOR_IN_SYSMACROS)
fi])

dnl Checks for USG derived STDIO that uses _filbuf()
dnl Defines HAVE__FILBUF on success.
AC_DEFUN([AC_HEADER__FILBUF],
[AC_CACHE_CHECK([for _filbuf()], ac_cv_func__filbuf,
                [AC_TRY_LINK([#include <stdio.h>],
[FILE    *f;
int     flag;
int     count;
char    *ptr;
char	c = 0;
f = fopen("confdefs.h", "r");
_filbuf(f);
_flsbuf(c, f);
flag  = f->_flag & _IONBF;
flag |= f->_flag & _IOERR;
flag |= f->_flag & _IOEOF;
count = f->_cnt;
ptr = (char *)f->_ptr;
fclose(f);],
                [ac_cv_func__filbuf=yes],
                [ac_cv_func__filbuf=no])])
if test $ac_cv_func__filbuf = yes; then
  AC_DEFINE(HAVE__FILBUF)
fi])

dnl Checks for USG derived STDIO that uses __filbuf()
dnl Defines HAVE___FILBUF on success.
AC_DEFUN([AC_HEADER___FILBUF],
[AC_CACHE_CHECK([for __filbuf()], ac_cv_func___filbuf,
                [AC_TRY_LINK([#include <stdio.h>],
[FILE    *f;
int     flag;
int     count;
char    *ptr;
char	c = 0;
f = fopen("confdefs.h", "r");
__filbuf(f);
__flsbuf(c, f);
flag  = f->_flag & _IONBF;
flag |= f->_flag & _IOERR;
flag |= f->_flag & _IOEOF;
count = f->_cnt;
ptr = (char *)f->_ptr;
fclose(f);],
                [ac_cv_func___filbuf=yes],
                [ac_cv_func___filbuf=no])])
if test $ac_cv_func___filbuf = yes; then
  AC_DEFINE(HAVE___FILBUF)
fi])

dnl Checks for USG derived STDIO
dnl Defines HAVE_USG_STDIO on success.
AC_DEFUN([AC_HEADER_USG_STDIO],
[AC_REQUIRE([AC_HEADER__FILBUF])AC_REQUIRE([AC_HEADER___FILBUF])dnl
AC_CACHE_CHECK([for USG derived STDIO], ac_cv_header_usg_stdio,
                [AC_TRY_LINK([#include <stdio.h>],
[FILE    *f;
int     flag;
int     count;
char    *ptr;
char	c = 0;
f = fopen("confdefs.h", "r");
#ifdef	HAVE___FILBUF
__filbuf(f);
__flsbuf(c, f);
#else
#	ifdef	HAVE__FILBUF
_filbuf(f);
_flsbuf(c, f);
#	else
no filbuf()
#	endif
#endif
flag  = f->_flag & _IONBF;
flag |= f->_flag & _IOERR;
flag |= f->_flag & _IOEOF;
count = f->_cnt;
ptr = (char *)f->_ptr;
fclose(f);],
                [ac_cv_header_usg_stdio=yes],
                [ac_cv_header_usg_stdio=no])])
if test $ac_cv_header_usg_stdio = yes; then
  AC_DEFINE(HAVE_USG_STDIO)
fi])

dnl Checks for Linux stdio with f->_flags
dnl Defines HAVE_FILE__FLAGS on success.
AC_DEFUN([AC_HEADER_FILE__FLAGS],
[AC_CACHE_CHECK([for f->_flags in FILE *], ac_cv_file__flags,
                [AC_TRY_LINK([#include <stdio.h>],
[FILE    *f;
int     flags;
f = fopen("confdefs.h", "r");
flags  = f->_flags;
fclose(f);],
                [ac_cv_file__flags=yes],
                [ac_cv_file__flags=no])])
if test $ac_cv_file__flags = yes; then
  AC_DEFINE(HAVE_FILE__FLAGS)
fi])

dnl Checks for Linux stdio with f->_IO_buf_base
dnl Defines HAVE_FILE__IO_BUF_BASE on success.
AC_DEFUN([AC_HEADER_FILE__IO_BUF_BASE],
[AC_CACHE_CHECK([for f->_IO_buf_base in FILE *], ac_cv_file__io_buf_base,
                [AC_TRY_LINK([#include <stdio.h>],
[FILE    *f;
char     *ptr;
f = fopen("confdefs.h", "r");
ptr = f->_IO_buf_base;
fclose(f);],
                [ac_cv_file__io_buf_base=yes],
                [ac_cv_file__io_buf_base=no])])
if test $ac_cv_file__io_buf_base = yes; then
  AC_DEFINE(HAVE_FILE__IO_BUF_BASE)
fi])

dnl Checks for errno definition in <errno.h>
dnl Defines HAVE_ERRNO_DEF on success.
AC_DEFUN([AC_HEADER_ERRNO_DEF],
[AC_CACHE_CHECK([for errno definition in errno.h], ac_cv_header_errno_def,
                [AC_TRY_COMPILE([#include <errno.h>],
[errno = 0;],
                [ac_cv_header_errno_def=yes],
                [ac_cv_header_errno_def=no])])
if test $ac_cv_header_errno_def = yes; then
  AC_DEFINE(HAVE_ERRNO_DEF)
fi])

dnl Checks for environ definition in <unistd.h>
dnl Defines HAVE_ENVIRON_DEF on success.
AC_DEFUN([AC_HEADER_ENVIRON_DEF],
[AC_CACHE_CHECK([for environ definition in unistd.h/stdlib.h], ac_cv_header_environ_def,
                [AC_TRY_COMPILE([
#ifdef	HAVE_UNISTD_H
#include <unistd.h>
#else
#ifdef	HAVE_STDLIB_H
#include <stdlib.h>	/* MSC no unistd.h but environ in stdlib.h */
#endif
#endif
],
[environ = 0;],
                [ac_cv_header_environ_def=yes],
                [ac_cv_header_environ_def=no])])
if test $ac_cv_header_environ_def = yes; then
  AC_DEFINE(HAVE_ENVIRON_DEF)
fi])

dnl Checks for sys_siglist definition in <signal.h>
dnl Defines HAVE_SYS_SIGLIST_DEF on success.
AC_DEFUN([AC_HEADER_SYS_SIGLIST_DEF],
[AC_CACHE_CHECK([for sys_siglist definition in signal.h], ac_cv_header_sys_siglist_def,
                [AC_TRY_COMPILE([#include <signal.h>],
[char *cp = (char *)sys_siglist[0]; exit (cp != (char *)0 && *cp != 0);],
                [ac_cv_header_sys_siglist_def=yes],
                [ac_cv_header_sys_siglist_def=no])])
if test $ac_cv_header_sys_siglist_def = yes; then
  AC_DEFINE(HAVE_SYS_SIGLIST_DEF)
fi])

dnl Checks for extern timezone in time.h or sys/time.h
dnl Defines HAVE_VAR_TIMEZONE_DEF on success.
AC_DEFUN([AC_VAR_TIMEZONE_DEF],
[AC_REQUIRE([AC_HEADER_TIME])dnl
AC_CACHE_CHECK([for extern timezone in time.h or sys/time.h], ac_cv_var_timezone_def,
                [AC_TRY_COMPILE([
#include <sys/types.h>
#ifdef	TIME_WITH_SYS_TIME_H
#	include <sys/time.h>
#	include <time.h>
#else
#ifdef	HAVE_SYS_TIME_H
#	include <sys/time.h>
#else
#	include <time.h>
#endif
#endif], [timezone = 1;],
                [ac_cv_var_timezone_def=yes],
                [ac_cv_var_timezone_def=no])])
if test $ac_cv_var_timezone_def = yes; then
  AC_DEFINE(HAVE_VAR_TIMEZONE_DEF)
fi])

dnl Checks if extern long timezone exists in libc
dnl Defines HAVE_VAR_TIMEZONE on success.
AC_DEFUN([AC_VAR_TIMEZONE],
[AC_CACHE_CHECK([for working extern long timezone ], ac_cv_var_timezone,
                [AC_TRY_RUN([
extern	long	timezone;

int
main()
{
	exit(timezone != 0);
}],
                [ac_cv_var_timezone=yes],
                [ac_cv_var_timezone=no])])
if test $ac_cv_var_timezone = yes; then
  AC_DEFINE(HAVE_VAR_TIMEZONE)
fi])

dnl Checks if extern char *__progname exists in libc
dnl Defines HAVE_VAR___PROGNAME on success.
AC_DEFUN([AC_VAR___PROGNAME],
[AC_CACHE_CHECK([for working extern char *__progname ], ac_cv_var___progname,
                [AC_TRY_RUN([
extern	char	*__progname;

int
main()
{
	exit (!(__progname != 0 && *__progname != '\0'));
}],
                [ac_cv_var___progname=yes],
                [ac_cv_var___progname=no])])
if test $ac_cv_var___progname = yes; then
  AC_DEFINE(HAVE_VAR___PROGNAME)
fi])

dnl Checks if extern char *__progname_full exists in libc
dnl Defines HAVE_VAR___PROGNAME_FULL on success.
AC_DEFUN([AC_VAR___PROGNAME_FULL],
[AC_CACHE_CHECK([for working extern char *__progname_full ], ac_cv_var___progname_full,
                [AC_TRY_RUN([
extern	char	*__progname_full;

int
main()
{
	exit (!(__progname_full != 0 && *__progname_full != '\0'));
}],
                [ac_cv_var___progname_full=yes],
                [ac_cv_var___progname_full=no])])
if test $ac_cv_var___progname_full = yes; then
  AC_DEFINE(HAVE_VAR___PROGNAME_FULL)
fi])

dnl Checks for UNIX-98 compliant <inttypes.h>
dnl Defines HAVE_INTTYPES_H on success.
AC_DEFUN([AC_HEADER_INTTYPES],
[AC_CACHE_CHECK([for UNIX-98 compliant inttypes.h], ac_cv_header_inttypes,
                [AC_TRY_COMPILE([#include <inttypes.h>],
[int8_t c; uint8_t uc; int16_t s; uint16_t us; int32_t i; uint32_t ui;
int64_t ll; uint64_t ull;
intptr_t ip; uintptr_t uip;],
                [ac_cv_header_inttypes=yes],
                [ac_cv_header_inttypes=no])])
if test $ac_cv_header_inttypes = yes; then
  AC_DEFINE(HAVE_INTTYPES_H)
fi])

dnl Checks for SUSv3 compliant <stdint.h>
dnl Defines HAVE_STDINT_H and HAVE_TYPE_INTMAX_T/HAVE_TYPE_UINTMAX_T on success.
AC_DEFUN([AC_HEADER_STDINT],
[AC_CACHE_CHECK([for SUSv3 compliant stdint.h], ac_cv_header_stdint,
                [AC_TRY_COMPILE([#include <stdint.h>],
[int8_t c; uint8_t uc; int16_t s; uint16_t us; int32_t i; uint32_t ui;
int64_t ll; uint64_t ull;
intptr_t ip; uintptr_t uip;
intmax_t im; uintmax_t uim;],
                [ac_cv_header_stdint=yes],
                [ac_cv_header_stdint=no])])
if test $ac_cv_header_stdint = yes; then
  AC_DEFINE(HAVE_STDINT_H)
  AC_DEFINE(HAVE_TYPE_INTMAX_T)
  AC_DEFINE(HAVE_TYPE_UINTMAX_T)
fi])

dnl Checks for struct timeval in time.h or sys/time.h
dnl Defines HAVE_STRUCT_TIMEVAL on success.
AC_DEFUN([AC_STRUCT_TIMEVAL],
[AC_REQUIRE([AC_HEADER_TIME])dnl
AC_CACHE_CHECK([for struct timeval in time.h or sys/time.h], ac_cv_struct_timeval,
                [AC_TRY_COMPILE([
#include <sys/types.h>
#ifdef	TIME_WITH_SYS_TIME_H
#	include <sys/time.h>
#	include <time.h>
#else
#ifdef	HAVE_SYS_TIME_H
#	include <sys/time.h>
#else
#	include <time.h>
#endif
#endif], [struct timeval tv;],
                [ac_cv_struct_timeval=yes],
                [ac_cv_struct_timeval=no])])
if test $ac_cv_struct_timeval = yes; then
  AC_DEFINE(HAVE_STRUCT_TIMEVAL)
fi])

dnl Checks for struct timezone in time.h or sys/time.h
dnl Defines HAVE_STRUCT_TIMEZONE on success.
AC_DEFUN([AC_STRUCT_TIMEZONE],
[AC_REQUIRE([AC_HEADER_TIME])dnl
AC_CACHE_CHECK([for struct timezone in time.h or sys/time.h], ac_cv_struct_timezone,
                [AC_TRY_COMPILE([
#include <sys/types.h>
#ifdef	TIME_WITH_SYS_TIME_H
#	include <sys/time.h>
#	include <time.h>
#else
#ifdef	HAVE_SYS_TIME_H
#	include <sys/time.h>
#else
#	include <time.h>
#endif
#endif], [struct timezone tz;],
                [ac_cv_struct_timezone=yes],
                [ac_cv_struct_timezone=no])])
if test $ac_cv_struct_timezone = yes; then
  AC_DEFINE(HAVE_STRUCT_TIMEZONE)
fi])

dnl Checks for struct timespec in time.h or sys/time.h
dnl Defines HAVE_STRUCT_TIMESPEC on success.
AC_DEFUN([AC_STRUCT_TIMESPEC],
[AC_REQUIRE([AC_HEADER_TIME])dnl
AC_CACHE_CHECK([for struct timespec in time.h or sys/time.h], ac_cv_struct_timespec,
                [AC_TRY_COMPILE([
#include <sys/types.h>
#ifdef	TIME_WITH_SYS_TIME_H
#	include <sys/time.h>
#	include <time.h>
#else
#ifdef	HAVE_SYS_TIME_H
#	include <sys/time.h>
#else
#	include <time.h>
#endif
#endif], [struct timespec ts;],
                [ac_cv_struct_timespec=yes],
                [ac_cv_struct_timespec=no])])
if test $ac_cv_struct_timespec = yes; then
  AC_DEFINE(HAVE_STRUCT_TIMESPEC)
fi])

dnl Checks for type time_t
dnl Defines time_t to long on failure.
AC_DEFUN([AC_TYPE_TIME_T],
[AC_REQUIRE([AC_HEADER_TIME])dnl
AC_CACHE_CHECK([for time_t], ac_cv_type_time_t,
                [AC_TRY_COMPILE([
#include <sys/types.h>
#ifdef	TIME_WITH_SYS_TIME_H
#	include <sys/time.h>
#	include <time.h>
#else
#ifdef	HAVE_SYS_TIME_H
#	include <sys/time.h>
#else
#	include <time.h>
#endif
#endif], [time_t t;],
                [ac_cv_type_time_t=yes],
                [ac_cv_type_time_t=no])])
if test $ac_cv_type_time_t = no; then
  AC_DEFINE(time_t, long)
fi])

dnl AC_CHECK_SIZE_TIME([CROSS-SIZE])
dnl This must be called past AC_CHECK_SIZEOF(long int)
AC_DEFUN(AC_CHECK_SIZE_TIME_T,
[changequote(<<, >>)dnl
dnl The name to #define.
define(<<AC_TYPE_NAME>>, translit(sizeof_time_t, [a-z *], [A-Z_P]))dnl
dnl The cache variable name.
define(<<AC_CV_NAME>>, translit(ac_cv_sizeof_time_t, [ *], [_p]))dnl
changequote([, ])dnl
AC_MSG_CHECKING(size of time_t)
AC_CACHE_VAL(AC_CV_NAME,
[AC_TRY_RUN([#include <stdio.h>
#include <sys/types.h>
#ifdef	TIME_WITH_SYS_TIME_H
#	include <sys/time.h>
#	include <time.h>
#else
#ifdef	HAVE_SYS_TIME_H
#	include <sys/time.h>
#else
#	include <time.h>
#endif
#endif
main()
{
  FILE *f=fopen("conftestval", "w");
  if (!f) exit(1);
  fprintf(f, "%d\n", sizeof(time_t));
  exit(0);
}], AC_CV_NAME=`cat conftestval`, AC_CV_NAME=SIZEOF_LONG_INT, ifelse([$1], , , AC_CV_NAME=$1))])dnl
AC_MSG_RESULT($AC_CV_NAME)
AC_DEFINE_UNQUOTED(AC_TYPE_NAME, $AC_CV_NAME)
undefine([AC_TYPE_NAME])dnl
undefine([AC_CV_NAME])dnl
])

dnl AC_CHECK_SIZE_WCHAR([CROSS-SIZE])
AC_DEFUN(AC_CHECK_SIZE_WCHAR,
[changequote(<<, >>)dnl
dnl The name to #define.
define(<<AC_TYPE_NAME>>, translit(sizeof_wchar, [a-z *], [A-Z_P]))dnl
dnl The cache variable name.
define(<<AC_CV_NAME>>, translit(ac_cv_sizeof_wchar, [ *], [_p]))dnl
changequote([, ])dnl
AC_MSG_CHECKING(size of wchar)
AC_CACHE_VAL(AC_CV_NAME,
[AC_TRY_RUN([#include <stdio.h>
main()
{
  FILE *f=fopen("conftestval", "w");
  if (!f) exit(1);
  fprintf(f, "%d\n", sizeof(L'a'));
  exit(0);
}], AC_CV_NAME=`cat conftestval`, AC_CV_NAME=0, ifelse([$1], , , AC_CV_NAME=$1))])dnl
AC_MSG_RESULT($AC_CV_NAME)
AC_DEFINE_UNQUOTED(AC_TYPE_NAME, $AC_CV_NAME)
undefine([AC_TYPE_NAME])dnl
undefine([AC_CV_NAME])dnl
])

dnl AC_CHECK_SIZE_WCHAR([CROSS-SIZE])
AC_DEFUN(AC_CHECK_SIZE_WCHAR_T,
[changequote(<<, >>)dnl
dnl The name to #define.
define(<<AC_TYPE_NAME>>, translit(sizeof_wchar_t, [a-z *], [A-Z_P]))dnl
dnl The cache variable name.
define(<<AC_CV_NAME>>, translit(ac_cv_sizeof_wchar_t, [ *], [_p]))dnl
changequote([, ])dnl
AC_MSG_CHECKING(size of wchar_t)
AC_CACHE_VAL(AC_CV_NAME,
[AC_TRY_RUN([#include <stdio.h>
#ifdef	HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef	HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef	HAVE_WCHAR_H
#include <wchar.h>
#endif
main()
{
  FILE *f=fopen("conftestval", "w");
  if (!f) exit(1);
  fprintf(f, "%d\n", sizeof(wchar_t));
  exit(0);
}], AC_CV_NAME=`cat conftestval`, AC_CV_NAME=SIZEOF_CHAR, ifelse([$1], , , AC_CV_NAME=$1))])dnl
AC_MSG_RESULT($AC_CV_NAME)
AC_DEFINE_UNQUOTED(AC_TYPE_NAME, $AC_CV_NAME)
undefine([AC_TYPE_NAME])dnl
undefine([AC_CV_NAME])dnl
])


dnl Checks for type clock_t
dnl Defines clock_t to long on failure.
dnl XXX we cannot check for AC_CHECK_HEADERS(sys/times.h)
AC_DEFUN([AC_TYPE_CLOCK_T],
[AC_REQUIRE([AC_HEADER_TIME])dnl
AC_CACHE_CHECK([for clock_t], ac_cv_type_clock_t,
                [AC_TRY_COMPILE([
#include <sys/types.h>
/*
 * time.h is needed because of a bug in Next Step.
 * Next Step needs time.h for clock_t
 */
#ifdef	TIME_WITH_SYS_TIME
#	ifndef	_INCL_SYS_TIME_H
#	include <sys/time.h>
#	define	_INCL_SYS_TIME_H
#	endif
#	ifndef	_INCL_TIME_H
#	include <time.h>
#	define	_INCL_TIME_H
#	endif
#else
#ifdef	HAVE_SYS_TIME_H
#	ifndef	_INCL_SYS_TIME_H
#	include <sys/time.h>
#	define	_INCL_SYS_TIME_H
#	endif
#else
#	ifndef	_INCL_TIME_H
#	include <time.h>
#	define	_INCL_TIME_H
#	endif
#endif
#endif
#ifdef	HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
		], [clock_t t;],
                [ac_cv_type_clock_t=yes],
                [ac_cv_type_clock_t=no])])
if test $ac_cv_type_clock_t = no; then
  AC_DEFINE(clock_t, long)
fi])

dnl Checks for type socklen_t
dnl Defines socklen_t to int on failure.
AC_DEFUN([AC_TYPE_SOCKLEN_T],
[AC_REQUIRE([AC_HEADER_STDC])dnl
AC_MSG_CHECKING(for socklen_t)
AC_CACHE_VAL(ac_cv_type_socklen_t,
[AC_EGREP_CPP(dnl
changequote(<<,>>)dnl
<<(^|[^a-zA-Z_0-9])socklen_t[^a-zA-Z_0-9]>>dnl
changequote([,]), [#include <sys/types.h>
#if	HAVE_STDLIB_H || STDC_HEADERS
#include <stdlib.h>
#endif
#if	HAVE_STDDEF_H || STDC_HEADERS
#include <stddef.h>
#endif
#include <sys/socket.h>], ac_cv_type_socklen_t=yes, ac_cv_type_socklen_t=no)])dnl
AC_MSG_RESULT($ac_cv_type_socklen_t)
if test $ac_cv_type_socklen_t = no; then
  AC_DEFINE(socklen_t, int)
fi])

dnl Checks for type stack_t
dnl Defines HAVE_STACK_T on success.
AC_DEFUN([AC_TYPE_STACK_T],
[AC_CACHE_CHECK([if stack_t is declared in signal.h], ac_cv_stack_t,
                [AC_TRY_COMPILE([#include <signal.h>],
                                [stack_t ss; ss.ss_size = 0; exit (ss.ss_size == 0);],
                                [ac_cv_stack_t=yes],
                                [ac_cv_stack_t=no])])
if test $ac_cv_stack_t = yes; then
  AC_DEFINE(HAVE_STACK_T)
fi])

dnl Checks for type siginfo_t
dnl Defines HAVE_SIGINFO_T on success.
AC_DEFUN([AC_TYPE_SIGINFO_T],
[AC_CACHE_CHECK([if siginfo_t is declared in signal.h], ac_cv_siginfo_t,
                [AC_TRY_COMPILE([#include <signal.h>
#ifdef	HAVE_SIGINFO_H
#include <siginfo.h>
#else
#ifdef	HAVE_SYS_SIGINFO_H
#include <sys/siginfo.h>
#endif
#endif],
                                [siginfo_t si; si.si_signo = 0; exit (si.si_signo == 0);],
                                [ac_cv_siginfo_t=yes],
                                [ac_cv_siginfo_t=no])])
if test $ac_cv_siginfo_t = yes; then
  AC_DEFINE(HAVE_SIGINFO_T)
fi])

dnl Checks for type idtype_t
dnl Defines HAVE_TYPE_IDTYPE_T on success.
AC_DEFUN([AC_TYPE_IDTYPE_T],
[AC_CACHE_CHECK([if idtype_t is declared correctly in wait.h], ac_cv_have_type_idtype_t,
                [AC_TRY_COMPILE([
#if	defined(HAVE_WAIT_H)
#	include <wait.h>
#else
#include <sys/wait.h>
#endif],
                                [idtype_t idt; idt = P_ALL; idt = P_PGID; idt = P_PID; exit (idt == P_PID);],
                                [ac_cv_have_type_idtype_t=yes],
                                [ac_cv_have_type_idtype_t=no])])
if test $ac_cv_have_type_idtype_t = yes; then
  AC_DEFINE(HAVE_TYPE_IDTYPE_T)
fi])

dnl Checks for type struct sockaddr_storage
dnl Defines HAVE_SOCKADDR_STORAGE on success.
AC_DEFUN([AC_STRUCT_SOCKADDR_STORAGE],
[AC_CACHE_CHECK([if struct sockaddr_storage is declared in socket.h], ac_cv_struct_sockaddr_storage,
                [AC_TRY_COMPILE([#include <sys/socket.h>],
                                [struct  sockaddr_storage ss; ss.ss_family = 0; exit (ss.ss_family == 0);],
                                [ac_cv_struct_sockaddr_storage=yes],
                                [ac_cv_struct_sockaddr_storage=no])])
if test $ac_cv_struct_sockaddr_storage = yes; then
  AC_DEFINE(HAVE_SOCKADDR_STORAGE)
fi])

dnl Checks for type long long
dnl Defines HAVE_LONGLONG on success.
AC_DEFUN([AC_TYPE_LONGLONG],
[AC_CACHE_CHECK([for type long long], ac_cv_type_longlong,
                [AC_TRY_COMPILE([], [long long i;],
                [ac_cv_type_longlong=yes],
                [ac_cv_type_longlong=no])])
if test $ac_cv_type_longlong = yes; then
  AC_DEFINE(HAVE_LONGLONG)
fi])

dnl Checks for type __int64
dnl Defines HAVE___INT64 on success.
AC_DEFUN([AC_TYPE___INT64],
[AC_CACHE_CHECK([for type __int64], ac_cv_type___int64,
                [AC_TRY_COMPILE([], [__int64 i;],
                [ac_cv_type___int64=yes],
                [ac_cv_type___int64=no])])
if test $ac_cv_type___int64 = yes; then
  AC_DEFINE(HAVE___INT64)
fi])

dnl Checks for type long double
dnl Defines HAVE_LONGDOUBLE on success.
AC_DEFUN([AC_TYPE_LONGDOUBLE],
[AC_CACHE_CHECK([for type long double], ac_cv_type_longdouble,
                [AC_TRY_COMPILE([], [long double ld;],
                [ac_cv_type_longdouble=yes],
                [ac_cv_type_longdouble=no])])
if test $ac_cv_type_longdouble = yes; then
  AC_DEFINE(HAVE_LONGDOUBLE)
fi])

dnl Checks if C-compiler orders bitfields htol
dnl Defines BITFIELDS_HTOL on success.
AC_DEFUN([AC_C_BITFIELDS],
[AC_CACHE_CHECK([whether bitorder in bitfields is htol], ac_cv_c_bitfields_htol,
                [AC_TRY_RUN([
struct {
	unsigned char	x1:4;
	unsigned char	x2:4;
} a;
int
main()
{
char	*cp;

cp = (char *)&a;
*cp = 0x12;
exit(a.x1 == 2);}],
                [ac_cv_c_bitfields_htol=yes],
                [ac_cv_c_bitfields_htol=no])])
if test $ac_cv_c_bitfields_htol = yes; then
  AC_DEFINE(BITFIELDS_HTOL)
fi])

dnl Checks if C-compiler understands prototypes
dnl Defines PROTOTYPES on success.
AC_DEFUN([AC_TYPE_PROTOTYPES],
[AC_CACHE_CHECK([for prototypes], ac_cv_type_prototypes,
                [AC_TRY_RUN([
doit(int i, ...)
{return 0;}
int
main(int ac, char *av[])
{ doit(1, 2, 3);
exit(0);}],
                [ac_cv_type_prototypes=yes],
                [ac_cv_type_prototypes=no])])
if test $ac_cv_type_prototypes = yes; then
  AC_DEFINE(PROTOTYPES)
fi])

dnl Checks for type size_t
dnl Defines HAVE_SIZE_T_ on success.
AC_DEFUN([AC_TYPE_SIZE_T_],
[AC_CACHE_CHECK([for type size_t], ac_cv_type_size_t_,
                [AC_TRY_COMPILE([
#ifdef	HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef	HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef	HAVE_STDDEF_H
#include <stddef.h>
#endif
#ifdef	HAVE_STDIO_H
#include <stdio.h>
#endif
		], [size_t s;],
                [ac_cv_type_size_t_=yes],
                [ac_cv_type_size_t_=no])])
if test $ac_cv_type_size_t_ = yes; then
  AC_DEFINE(HAVE_SIZE_T)
else
  AC_DEFINE(NO_SIZE_T)
fi])

dnl Checks if type char is unsigned
dnl Defines CHAR_IS_UNSIGNED on success.
AC_DEFUN([AC_TYPE_CHAR],
[AC_CACHE_CHECK([if char is unsigned], ac_cv_type_char_unsigned,
                [AC_TRY_RUN([
int
main()
{
	char c;

	c = -1;
	exit(c < 0);}],
		[ac_cv_type_char_unsigned=yes],
		[ac_cv_type_char_unsigned=no],
		[ac_cv_type_char_unsigned=no])]) 
if test $ac_cv_type_char_unsigned = yes; then
  AC_DEFINE(CHAR_IS_UNSIGNED)
fi])

dnl Checks if function/macro va_copy() is available
dnl Defines HAVE_VA_COPY on success.
AC_DEFUN([AC_FUNC_VA_COPY],
[AC_CACHE_CHECK([for va_copy], ac_cv_func_va_copy,
                [AC_TRY_LINK([
#ifdef	HAVE_STDARG_H
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif], 
		[
va_list a, b;

va_copy(a, b);],
                [ac_cv_func_va_copy=yes],
                [ac_cv_func_va_copy=no])])
if test $ac_cv_func_va_copy = yes; then
  AC_DEFINE(HAVE_VA_COPY)
fi])

dnl Checks if function/macro __va_copy() is available
dnl Defines HAVE__VA_COPY on success.
AC_DEFUN([AC_FUNC__VA_COPY],
[AC_CACHE_CHECK([for __va_copy], ac_cv_func__va_copy,
                [AC_TRY_LINK([
#ifdef	HAVE_STDARG_H
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif], 
		[
va_list a, b;

__va_copy(a, b);],

                [ac_cv_func__va_copy=yes],
                [ac_cv_func__va_copy=no])])
if test $ac_cv_func__va_copy = yes; then
  AC_DEFINE(HAVE__VA_COPY)
fi])

dnl Checks if va_list is an array
dnl Defines VA_LIST_IS_ARRAY on success.
AC_DEFUN([AC_TYPE_VA_LIST],
[AC_CACHE_CHECK([if va_list is an array], ac_cv_type_va_list_array,
                [AC_TRY_LINK([
#ifdef	HAVE_STDARG_H
#	include <stdarg.h>
#else
#	include <varargs.h>
#endif
],
	[
va_list a, b;

a = b;],
		[ac_cv_type_va_list_array=no],
		[ac_cv_type_va_list_array=yes])]) 
if test $ac_cv_type_va_list_array = yes; then
  AC_DEFINE(VA_LIST_IS_ARRAY)
fi])

dnl Checks if quotactl is present as ioctl
dnl Defines HAVE_QUOTAIOCTL on success.
AC_DEFUN([AC_FUNC_QUOTAIOCTL],
[AC_CACHE_CHECK([if quotactl is an ioctl], ac_cv_func_quotaioctl,
                [AC_TRY_LINK([#include <sys/types.h>
#include <sys/fs/ufs_quota.h>],
		[struct quotctl q; ioctl(0, Q_QUOTACTL, &q)],
		[ac_cv_func_quotaioctl=yes],
		[ac_cv_func_quotaioctl=no])]) 
if test $ac_cv_func_quotaioctl = yes; then
  AC_DEFINE(HAVE_QUOTAIOCTL)
fi])

dnl Checks if function __dtoa() is available
dnl Defines HAVE_DTOA on success.
AC_DEFUN([AC_FUNC_DTOA],
[AC_CACHE_CHECK([for __dtoa], ac_cv_func_dtoa,
                [AC_TRY_LINK([extern  char *__dtoa();], 
[int decpt; int sign; char *ep; char *bp;
bp = __dtoa(0.0, 2, 6, &decpt, &sign, &ep);],
                [ac_cv_func_dtoa=yes],
                [ac_cv_func_dtoa=no])])
if test $ac_cv_func_dtoa = yes; then
  AC_DEFINE(HAVE_DTOA)
fi])

dnl Checks if reentrant __dtoa() exists (needs a result prt)
dnl Defines HAVE_DTOA_R on success.
AC_DEFUN([AC_FUNC_DTOA_R],
[AC_REQUIRE([AC_FUNC_DTOA])dnl
AC_CACHE_CHECK([for __dtoa that needs result ptr], ac_cv_func_dtoa_r,
                [AC_TRY_RUN([
extern	char *__dtoa();
int
main()
{
#ifdef	HAVE_DTOA
	int	decpt, sign;
	char	*bp;
	char	*ep;
	char	*result;

	result = 0;
	bp = __dtoa(1.9, 2, 5, &decpt, &sign, &ep, &result);
	exit(result == 0);
#else
	exit(1);
#endif
}],
                [ac_cv_func_dtoa_r=yes],
                [ac_cv_func_dtoa_r=no])])
if test $ac_cv_func_dtoa_r = yes; then
  AC_DEFINE(HAVE_DTOA_R)
fi])

dnl Checks if working ecvt() exists
dnl Defines HAVE_ECVT on success.
AC_DEFUN([AC_FUNC_ECVT],
[AC_CACHE_CHECK([for working ecvt() ], ac_cv_func_ecvt,
                [AC_TRY_RUN([
extern	char *ecvt();

sprintf(s)
	char	*s;
{
	strcpy(s, "DEAD");
}

int
main()
{
	int a, b;

/*	exit (strcmp("DEAD", ecvt(1.9, 2, &a, &b)) == 0);*/
	exit (strcmp("19", ecvt(1.9, 2, &a, &b)) != 0);
}],
                [ac_cv_func_ecvt=yes],
                [ac_cv_func_ecvt=no])])
if test $ac_cv_func_ecvt = yes; then
  AC_DEFINE(HAVE_ECVT)
fi])

dnl Checks if working fcvt() exists
dnl Defines HAVE_FCVT on success.
AC_DEFUN([AC_FUNC_FCVT],
[AC_CACHE_CHECK([for working fcvt() ], ac_cv_func_fcvt,
                [AC_TRY_RUN([
extern	char *fcvt();

sprintf(s)
	char	*s;
{
	strcpy(s, "DEAD");
}

int
main()
{
	int a, b;

/*	exit (strcmp("DEAD", fcvt(1.9, 2, &a, &b)) == 0);*/
	exit (strcmp("190", fcvt(1.9, 2, &a, &b)) != 0);
}],
                [ac_cv_func_fcvt=yes],
                [ac_cv_func_fcvt=no])])
if test $ac_cv_func_fcvt = yes; then
  AC_DEFINE(HAVE_FCVT)
fi])

dnl Checks if working gcvt() exists
dnl Defines HAVE_GCVT on success.
AC_DEFUN([AC_FUNC_GCVT],
[AC_CACHE_CHECK([for working gcvt() ], ac_cv_func_gcvt,
                [AC_TRY_RUN([
extern	char *gcvt();

sprintf(s)
	char	*s;
{
	strcpy(s, "DEAD");
}

int
main()
{
	char	buf[32];

/*	exit (strcmp("DEAD", gcvt(1.9, 10, buf)) == 0);*/
	exit (strcmp("1.9", gcvt(1.9, 10, buf)) != 0);
}],
                [ac_cv_func_gcvt=yes],
                [ac_cv_func_gcvt=no])])
if test $ac_cv_func_gcvt = yes; then
  AC_DEFINE(HAVE_GCVT)
fi])

dnl Checks if function uname() is available
dnl Defines HAVE_UNAME on success.
AC_DEFUN([AC_FUNC_UNAME],
[AC_CACHE_CHECK([for uname], ac_cv_func_uname,
                [AC_TRY_LINK([#include <sys/utsname.h>], 
[struct	utsname un;
uname(&un);],
                [ac_cv_func_uname=yes],
                [ac_cv_func_uname=no])])
if test $ac_cv_func_uname = yes; then
  AC_DEFINE(HAVE_UNAME)
fi])

dnl Checks if function mlock() is available
dnl beware HP-UX 10.x it contains a bad mlock() in libc
dnl Defines HAVE_MLOCK on success.
AC_DEFUN([AC_FUNC_MLOCK],
[AC_REQUIRE([AC_HEADER_ERRNO_DEF])dnl
AC_CACHE_CHECK([for mlock], ac_cv_func_mlock,
                [AC_TRY_RUN([
#include <sys/types.h>
#include <errno.h>
#ifndef	HAVE_ERRNO_DEF
extern	int	errno;
#endif

main()
{
	if (mlock(0, 0) < 0) {
		if (errno == EINVAL || errno ==  ENOMEM ||
		    errno == EPERM  || errno ==  EACCES)
			exit(0);
		exit(-1);
	}
	exit(0);
}],
                [ac_cv_func_mlock=yes],
                [ac_cv_func_mlock=no])
rm -f core core.* *.core])
if test $ac_cv_func_mlock = yes; then
  AC_DEFINE(HAVE_MLOCK)
fi])

dnl Checks if function mlockall() is available
dnl beware HP-UX 10.x it contains a bad mlockall() in libc
dnl Defines HAVE_MLOCKALL on success.
AC_DEFUN([AC_FUNC_MLOCKALL],
[AC_REQUIRE([AC_HEADER_ERRNO_DEF])dnl
AC_CACHE_CHECK([for mlockall], ac_cv_func_mlockall,
                [AC_TRY_RUN([
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>
#ifndef	HAVE_ERRNO_DEF
extern	int	errno;
#endif

int
main()
{
	if (mlockall(MCL_CURRENT|MCL_FUTURE) < 0) {
		if (errno == EINVAL || errno ==  ENOMEM ||
		    errno == EPERM  || errno ==  EACCES)
			exit(0);
		exit(-1);
	}
	exit(0);
}
],
                [ac_cv_func_mlockall=yes],
                [ac_cv_func_mlockall=no])
rm -f core core.* *.core])
if test $ac_cv_func_mlockall = yes; then
  AC_DEFINE(HAVE_MLOCKALL)
fi])

AC_DEFUN([jsAC_FUNC_MMAP],
[AC_CHECK_HEADERS(unistd.h)dnl
AC_REQUIRE([AC_MMAP_SIZEP])dnl
AC_REQUIRE([AC_FUNC_GETPAGESIZE])dnl
AC_CACHE_CHECK(for working mmap, ac_cv_func_mmap_fixed_mapped,
[AC_TRY_RUN([
/* Thanks to Mike Haertel and Jim Avera for this test.
   Here is a matrix of mmap possibilities:
	mmap private not fixed
	mmap private fixed at somewhere currently unmapped
	mmap private fixed at somewhere already mapped
	mmap shared not fixed
	mmap shared fixed at somewhere currently unmapped
	mmap shared fixed at somewhere already mapped
   For private mappings, we should verify that changes cannot be read()
   back from the file, nor mmap's back from the file at a different
   address.  (There have been systems where private was not correctly
   implemented like the infamous i386 svr4.0, and systems where the
   VM page cache was not coherent with the filesystem buffer cache
   like early versions of FreeBSD and possibly contemporary NetBSD.)
   For shared mappings, we should conversely verify that changes get
   propogated back to all the places they're supposed to be.

   Grep wants private fixed already mapped.
   The main things grep needs to know about mmap are:
   * does it exist and is it safe to write into the mmap'd area
   * how to use it (BSD variants)  */
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>

/*
 * Needed for Apollo Domain/OS and may be for others?
 */
#ifdef	_MMAP_WITH_SIZEP
#	define	mmap_sizeparm(s)	(&(s))
#else
#	define	mmap_sizeparm(s)	(s)
#endif

/* This mess was copied from the GNU getpagesize.h.  */
#ifndef HAVE_GETPAGESIZE
# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif

/* Assume that all systems that can run configure have sys/param.h.  */
# ifndef HAVE_SYS_PARAM_H
#  define HAVE_SYS_PARAM_H 1
# endif

# ifdef _SC_PAGESIZE
#  define getpagesize() sysconf(_SC_PAGESIZE)
# else /* no _SC_PAGESIZE */
#  ifdef HAVE_SYS_PARAM_H
#   include <sys/param.h>
#   ifdef EXEC_PAGESIZE
#    define getpagesize() EXEC_PAGESIZE
#   else /* no EXEC_PAGESIZE */
#    ifdef NBPG
#     define getpagesize() NBPG * CLSIZE
#     ifndef CLSIZE
#      define CLSIZE 1
#     endif /* no CLSIZE */
#    else /* no NBPG */
#     ifdef NBPC
#      define getpagesize() NBPC
#     else /* no NBPC */
#      ifdef PAGESIZE
#       define getpagesize() PAGESIZE
#      endif /* PAGESIZE */
#     endif /* no NBPC */
#    endif /* no NBPG */
#   endif /* no EXEC_PAGESIZE */
#  else /* no HAVE_SYS_PARAM_H */
#   define getpagesize() 8192	/* punt totally */
#  endif /* no HAVE_SYS_PARAM_H */
# endif /* no _SC_PAGESIZE */

#endif /* no HAVE_GETPAGESIZE */

#ifdef __cplusplus
extern "C" { void *malloc(unsigned); }
#else
char *malloc();
#endif

int
main()
{
	char *data, *data2, *data3;
	int i, pagesize;
	int fd;

	pagesize = getpagesize();

	/*
	 * First, make a file with some known garbage in it.
	 */
	data = malloc(pagesize);
	if (!data)
		exit(1);
	for (i = 0; i < pagesize; ++i)
		*(data + i) = rand();
	umask(0);
	fd = creat("conftestmmap", 0600);
	if (fd < 0)
		exit(1);
	if (write(fd, data, pagesize) != pagesize)
		exit(1);
	close(fd);

	/*
	 * Next, try to mmap the file at a fixed address which
	 * already has something else allocated at it.  If we can,
	 * also make sure that we see the same garbage.
	 */
	fd = open("conftestmmap", O_RDWR);
	if (fd < 0)
		exit(1);
	data2 = malloc(2 * pagesize);
	if (!data2)
		exit(1);
	data2 += (pagesize - ((int) data2 & (pagesize - 1))) & (pagesize - 1);
	if (data2 != mmap(data2, mmap_sizeparm(pagesize), PROT_READ | PROT_WRITE,
	    MAP_PRIVATE | MAP_FIXED, fd, 0L))
		exit(1);
	for (i = 0; i < pagesize; ++i)
		if (*(data + i) != *(data2 + i))
			exit(1);

	/*
	 * Finally, make sure that changes to the mapped area
	 * do not percolate back to the file as seen by read().
	 * (This is a bug on some variants of i386 svr4.0.)
	 */
	for (i = 0; i < pagesize; ++i)
		*(data2 + i) = *(data2 + i) + 1;
	data3 = malloc(pagesize);
	if (!data3)
		exit(1);
	if (read(fd, data3, pagesize) != pagesize)
		exit(1);
	for (i = 0; i < pagesize; ++i)
		if (*(data + i) != *(data3 + i))
			exit(1);
	close(fd);
	unlink("conftestmmap");
	exit(0);
}
], ac_cv_func_mmap_fixed_mapped=yes, ac_cv_func_mmap_fixed_mapped=no,
ac_cv_func_mmap_fixed_mapped=no)])
if test $ac_cv_func_mmap_fixed_mapped = yes; then
  AC_DEFINE(HAVE_MMAP)
fi
])

AC_DEFUN([AC_MMAP_SIZEP],
[AC_CHECK_HEADERS(unistd.h)dnl
AC_REQUIRE([AC_FUNC_GETPAGESIZE])dnl
AC_CACHE_CHECK(for mmap that needs ptr to size, ac_cv_func_mmap_sizep,
[AC_TRY_RUN([
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/mman.h>

#ifndef MAP_FILE
#define MAP_FILE   0		/* Needed on Apollo Domain/OS */
#endif

/* This mess was copied from the GNU getpagesize.h.  */
#ifndef HAVE_GETPAGESIZE
# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif

/* Assume that all systems that can run configure have sys/param.h.  */
# ifndef HAVE_SYS_PARAM_H
#  define HAVE_SYS_PARAM_H 1
# endif

# ifdef _SC_PAGESIZE
#  define getpagesize() sysconf(_SC_PAGESIZE)
# else /* no _SC_PAGESIZE */
#  ifdef HAVE_SYS_PARAM_H
#   include <sys/param.h>
#   ifdef EXEC_PAGESIZE
#    define getpagesize() EXEC_PAGESIZE
#   else /* no EXEC_PAGESIZE */
#    ifdef NBPG
#     define getpagesize() NBPG * CLSIZE
#     ifndef CLSIZE
#      define CLSIZE 1
#     endif /* no CLSIZE */
#    else /* no NBPG */
#     ifdef NBPC
#      define getpagesize() NBPC
#     else /* no NBPC */
#      ifdef PAGESIZE
#       define getpagesize() PAGESIZE
#      endif /* PAGESIZE */
#     endif /* no NBPC */
#    endif /* no NBPG */
#   endif /* no EXEC_PAGESIZE */
#  else /* no HAVE_SYS_PARAM_H */
#   define getpagesize() 8192	/* punt totally */
#  endif /* no HAVE_SYS_PARAM_H */
# endif /* no _SC_PAGESIZE */

#endif /* no HAVE_GETPAGESIZE */

#ifdef __cplusplus
extern "C" { void *malloc(unsigned); }
#else
char *malloc();
#endif

jmp_buf jenv;

int
intr()
{
	signal(SIGSEGV, intr);
	longjmp(jenv, 1);
}

int
main()
{
	char *data, *data2;
	int i, pagesize, ps;
	int fd;

	pagesize = getpagesize();

	/*
	 * First, make a file with some known garbage in it.
	 */
	data = malloc(pagesize);
	if (!data)
		exit(1);
	for (i = 0; i < pagesize; ++i)
		*(data + i) = rand();
	umask(0);
	fd = creat("conftestmmap", 0600);
	if (fd < 0)
		exit(1);
	if (write(fd, data, pagesize) != pagesize)
		exit(1);
	close(fd);

	/*
	 * Next, try to mmap the file at a fixed address which
	 * already has something else allocated at it.  If we can,
	 * also make sure that we see the same garbage.
	 */
	fd = open("conftestmmap", O_RDWR);
	if (fd < 0)
		exit(1);

	/*
	 * Keep a copy, Apollo modifies the value...
	 */
	ps = pagesize;

	/*
	 * Apollo mmap() is not a syscall but a library function and fails
	 * if it tries to dereference 'ps'. We must use setjmp in order to
	 * catch the failure.
	 */
	signal(SIGSEGV, intr);
	if (setjmp(jenv) == 0) {
		data2 = mmap(0, ps, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0L);
	} else {
		data2 = (char *)-1;
	}
	if (data2 != (char *)-1)
		exit(1);

	signal(SIGSEGV, intr);
	if (setjmp(jenv) == 0) {
		data2 = mmap(0, &ps, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0L);
	} else {
		data2 = (char *)-1;
	}
	if (data2 == (char *)-1)
		exit(1);

	for (i = 0; i < pagesize; ++i)
		if (*(data + i) != *(data2 + i))
			exit(1);

	close(fd);
	unlink("conftestmmap");
	exit(0);
}
], ac_cv_func_mmap_sizep=yes, ac_cv_func_mmap_sizep=no,
ac_cv_func_mmap_sizep=no)])
if test $ac_cv_func_mmap_sizep = yes; then
  AC_DEFINE(_MMAP_WITH_SIZEP)
fi
])

dnl Checks if mmap() works to get shared memory
dnl Defines HAVE_SMMAP on success.
AC_DEFUN([AC_FUNC_SMMAP],
[AC_CACHE_CHECK([if mmap works to get shared memory], ac_cv_func_smmap,
                [AC_TRY_RUN([
#include <sys/types.h>
#include <sys/mman.h>

#ifndef	MAP_ANONYMOUS
#	ifdef	MAP_ANON
#		define MAP_ANONYMOUS MAP_ANON
#	endif
#endif

/*
 * Needed for Apollo Domain/OS and may be for others?
 */
#ifdef	_MMAP_WITH_SIZEP
#	define	mmap_sizeparm(s)	(&(s))
#else
#	define	mmap_sizeparm(s)	(s)
#endif

char *
mkshare()
{
        int     size = 8192;
        int     f;
        char    *addr;

#ifdef  MAP_ANONYMOUS   /* HP/UX */
        f = -1;
        addr = mmap(0, mmap_sizeparm(size), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, f, 0);
#else
        if ((f = open("/dev/zero", 2)) < 0)
                exit(1);
        addr = mmap(0, mmap_sizeparm(size), PROT_READ|PROT_WRITE, MAP_SHARED, f, 0);
#endif
        if (addr == (char *)-1)
                exit(1);
        close(f);

        return (addr);
}

int
main()
{
        char    *addr;
        
        addr = mkshare();
        *addr = 'I';

        switch (fork()) {

        case -1:
                printf("help\n"); exit(1);

        case 0: /* child */
                *addr = 'N';
                _exit(0);
                break;
        default: /* parent */
                wait(0);
                sleep(1);
                break;
        }

        if (*addr != 'N')
                exit(1);
        exit(0);
}
], 
                [ac_cv_func_smmap=yes],
                [ac_cv_func_smmap=no],
                [ac_cv_func_smmap=no])])
if test $ac_cv_func_smmap = yes; then
  AC_DEFINE(HAVE_SMMAP)
fi])

dnl Checks if shmget() works to get shared memory
dnl Defines HAVE_SHMGET on success.
AC_DEFUN([AC_FUNC_SHMGET],
[AC_CACHE_CHECK([if shmget works to get shared memory], ac_cv_func_shmget,
                [AC_TRY_RUN([
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
int
main()
{
	int	id;

	id = shmget(IPC_PRIVATE, 0x1, IPC_CREAT|0600);
	if (id == -1)
		exit(1);
	shmctl(id, IPC_RMID, 0);
        exit(0);
}
], 
                [ac_cv_func_shmget=yes],
                [ac_cv_func_shmget=no],
                [ac_cv_func_shmget=no])])
if test $ac_cv_func_shmget = yes; then
  AC_DEFINE(HAVE_SHMGET)
fi])

dnl Checks if sys_siglist[] exists
dnl Defines HAVE_SYS_SIGLIST on success.
AC_DEFUN([AC_FUNC_SYS_SIGLIST],
[AC_CACHE_CHECK([for sys_siglist], ac_cv_func_sys_siglist,
                [AC_TRY_RUN([
int
main()
{ extern char *sys_siglist[];
if (sys_siglist[1] == 0)
	exit(1);
exit(0);}],
                [ac_cv_func_sys_siglist=yes],
                [ac_cv_func_sys_siglist=no])])
if test $ac_cv_func_sys_siglist = yes; then
  AC_DEFINE(HAVE_SYS_SIGLIST)
fi])

dnl Checks for maximum number of bits in minor device number
AC_DEFUN([AC_CHECK_MINOR_BITS],
[AC_REQUIRE([AC_HEADER_MAKEDEV])dnl
changequote(<<, >>)dnl
define(<<AC_MACRO_NAME>>, DEV_MINOR_BITS)dnl
dnl The cache variable name.
define(<<AC_CV_NAME>>, ac_cv_dev_minor_bits)dnl
changequote([, ])dnl
AC_MSG_CHECKING(bits in minor device number)
AC_CACHE_VAL(AC_CV_NAME,
[AC_TRY_RUN([#include <stdio.h>
#include <sys/types.h>
#ifdef major
#	define _FOUND_MAJOR_
#endif

#ifdef MAJOR_IN_MKDEV
#	include <sys/mkdev.h>
#	define _FOUND_MAJOR_
#endif

#ifndef _FOUND_MAJOR_
#	ifdef MAJOR_IN_SYSMACROS
#		include <sys/sysmacros.h>
#		define _FOUND_MAJOR_
#	endif
#endif

#ifndef _FOUND_MAJOR_
#	if defined(hpux) || defined(__hpux__) || defined(__hpux)
#		include <sys/mknod.h>
#		define _FOUND_MAJOR_
#	endif
#endif

#ifndef _FOUND_MAJOR_
#	define major(dev)		(((dev) >> 8) & 0xFF)
#	define minor(dev)		((dev) & 0xFF)
#	define makedev(majo, mino)	(((majo) << 8) | (mino))
#endif
int
main()
{
	long	l = 1;
	int	i;
	int	m;
	int	c = 0;
	FILE	*f=fopen("conftestval", "w");
	int	maxloop = 32;

	if (sizeof (long) > 4)
		maxloop = 64;

	if (!f) exit(1);

	for (i=1, m=0; i <= maxloop; i++, l<<=1) {
		if (minor(l) == 0 && c == 0)
			c = m;
		if (minor(l) != 0)
			m = i;
	}
	fprintf(f, "%d\n", m);
	exit(0);
}], AC_CV_NAME=`cat conftestval`, AC_CV_NAME=0, ifelse([$2], , , AC_CV_NAME=$2))])dnl
AC_MSG_RESULT($AC_CV_NAME)
AC_DEFINE_UNQUOTED(AC_MACRO_NAME, $AC_CV_NAME)
undefine([AC_MACRO_NAME])dnl
undefine([AC_CV_NAME])dnl
])

dnl Checks for maximum number of bits in minor device numbers are non contiguous
dnl Defines DEV_MINOR_NONCONTIG on success.
AC_DEFUN([AC_CHECK_MINOR_NONCONTIG],
[AC_REQUIRE([AC_HEADER_MAKEDEV])dnl
AC_CACHE_CHECK([whether bits in minor device numbers are non contiguous], ac_cv_dev_minor_noncontig,
                [AC_TRY_RUN([
#include <sys/types.h>
#ifdef major
#	define _FOUND_MAJOR_
#endif

#ifdef MAJOR_IN_MKDEV
#	include <sys/mkdev.h>
#	define _FOUND_MAJOR_
#endif

#ifndef _FOUND_MAJOR_
#	ifdef MAJOR_IN_SYSMACROS
#		include <sys/sysmacros.h>
#		define _FOUND_MAJOR_
#	endif
#endif

#ifndef _FOUND_MAJOR_
#	if defined(hpux) || defined(__hpux__) || defined(__hpux)
#		include <sys/mknod.h>
#		define _FOUND_MAJOR_
#	endif
#endif

#ifndef _FOUND_MAJOR_
#	define major(dev)		(((dev) >> 8) & 0xFF)
#	define minor(dev)		((dev) & 0xFF)
#	define makedev(majo, mino)	(((majo) << 8) | (mino))
#endif
int
main()
{
	long	l = 1;
	int	i;
	int	m;
	int	c = 0;
	int	maxloop = 32;

	if (sizeof (long) > 4)
		maxloop = 64;

	for (i=1, m=0; i <= maxloop; i++, l<<=1) {
		if (minor(l) == 0 && c == 0)
			c = m;
		if (minor(l) != 0)
			m = i;
	}
exit (m == c);}],
                [ac_cv_dev_minor_noncontig=yes],
                [ac_cv_dev_minor_noncontig=no])])
if test $ac_cv_dev_minor_noncontig = yes; then
  AC_DEFINE(DEV_MINOR_NONCONTIG)
fi])

dnl Checks if we may not define our own malloc()
dnl Defines NO_USER_MALLOC if we cannot.
AC_DEFUN([AC_USER_MALLOC],
[AC_CACHE_CHECK([if we may not define our own malloc()], ac_cv_no_user_malloc,
                [AC_TRY_RUN([
#if !defined(__CYGWIN32__) && !defined(__CYGWIN__)
static int mcalled;
char *
malloc(s)
	int	s;
{
	extern	char *sbrk();

	/*
	 * Don't ask me why, but with Cygwin on 64 Bit Vista this hangs
	 * infinitely and is undebuggable unless we call write() here...
	 */
	write(2, "", 0);
	mcalled++;
	_exit(0);
	return (sbrk(s));
}

free(p) char *p;{}
#endif	/* !defined(__CYGWIN32__) && !defined(__CYGWIN__) */
	
main()
{
#if !defined(__CYGWIN32__) && !defined(__CYGWIN__)
#ifdef	HAVE_STRDUP
	strdup("aaa");
#else
	exit(0);
#endif
#endif	/* !defined(__CYGWIN32__) && !defined(__CYGWIN__) */
	exit(1);}],
                [ac_cv_no_user_malloc=no],
                [ac_cv_no_user_malloc=yes])])
if test $ac_cv_no_user_malloc = yes; then
  AC_DEFINE(NO_USER_MALLOC)
fi])

dnl Checks if ecvt()/fcvt()/gcvt() may be replaced by local implementations
dnl There are known problems on ATARI MINT and older Linux version and statical linking
dnl Defines NO_USER_XCVT on failure.
AC_DEFUN([AC_USER_XCVT],
[AC_CACHE_CHECK([if we may not define our own ecvt()/fcvt()/gcvt()], ac_cv_no_user_xcvt,
                [AC_TRY_LINK([
#include <stdio.h>

char *
ecvt(value, ndig, decpt, sign)
	double	value;
	int	ndig;
	int	*decpt;
	int	*sign;
{
	return ("ecvt-test");
}

char *
fcvt(value, ndig, decpt, sign)
	double	value;
	int	ndig;
	int	*decpt;
	int	*sign;
{
	return ("fcvt-test");
}

char *
gcvt(value, ndig, bp)
	double	value;
	int	ndig;
	char	*bp;
{
	return ("gcvt-test");
}
], 
		[
	char	buf[64];

	printf("E: %e F: %f G: %g local gcvt: %s\n", 1.234, 1.234, 1.234, gcvt(1.234, 4, buf));
],
                [ac_cv_no_user_xcvt=no],
                [ac_cv_no_user_xcvt=yes])])
if test $ac_cv_no_user_xcvt = yes; then
  AC_DEFINE(NO_USER_XCVT)
fi])

dnl Checks if BSD-4.2 compliant getpgrp() exists
dnl Defines HAVE_BSD_GETPGRP on success.
AC_DEFUN([AC_FUNC_BSD_GETPGRP],
[AC_CACHE_CHECK([for BSD compliant getpgrp], ac_cv_func_bsd_getpgrp,
                [AC_TRY_RUN([
int
main()
{ long p;
/*
 * POSIX getpgrp() has void parameter...
 */
	p = getpgrp(-1);
	if (p == getpgrp(1) && p == getpgrp(getpid()))
		exit(1);
exit(0);}],
                [ac_cv_func_bsd_getpgrp=yes],
                [ac_cv_func_bsd_getpgrp=no])])
if test $ac_cv_func_bsd_getpgrp = yes; then
  AC_DEFINE(HAVE_BSD_GETPGRP)
fi])

dnl Checks if BSD-4.2 compliant setpgrp() exists
dnl Defines HAVE_BSD_SETPGRP on success.
AC_DEFUN([AC_FUNC_BSD_SETPGRP],
[AC_REQUIRE([AC_HEADER_ERRNO_DEF])dnl
AC_CACHE_CHECK([for BSD compliant setpgrp], ac_cv_func_bsd_setpgrp,
                [AC_TRY_RUN([
#include <errno.h>
#ifndef	HAVE_ERRNO_DEF
extern	int	errno;
#endif
int
main()
{ errno = 0;
/*
 * Force illegal pid on BSD
 */
if (setpgrp(-1, 100) < 0 && errno == ESRCH)
	exit(0);
exit(1);}],
                [ac_cv_func_bsd_setpgrp=yes],
                [ac_cv_func_bsd_setpgrp=no])])
if test $ac_cv_func_bsd_setpgrp = yes; then
  AC_DEFINE(HAVE_BSD_SETPGRP)
fi])

dnl Checks if *printf() supports %jd
dnl Defines HAVE_PRINTF_J on success.
AC_DEFUN([AC_FUNC_PRINTF_J],
[AC_CACHE_CHECK([whether *printf() supports %jd], ac_cv_func_printf_j,
                [AC_TRY_RUN([
#include <inttypes.h>
int
main()
{ intmax_t m = 123456789012LL;
char buf[32];
sprintf(buf, "%jd", m);
if (strcmp(buf, "123456789012") == 0)
	exit(0);
exit(1);}],
                [ac_cv_func_printf_j=yes],
                [ac_cv_func_printf_j=no])])
if test $ac_cv_func_printf_j = yes; then
  AC_DEFINE(HAVE_PRINTF_J)
fi])

dnl Checks if *printf() supports %zd
dnl Defines HAVE_PRINTF_Z on success.
AC_DEFUN([AC_FUNC_PRINTF_Z],
[AC_CACHE_CHECK([whether *printf() supports %zd], ac_cv_func_printf_z,
                [AC_TRY_RUN([
#include <sys/types.h>
int
main()
{ size_t m = 1234567890;
char buf[32];
sprintf(buf, "%zd", m);
if (strcmp(buf, "1234567890") == 0)
	exit(0);
exit(1);}],
                [ac_cv_func_printf_z=yes],
                [ac_cv_func_printf_z=no])])
if test $ac_cv_func_printf_z = yes; then
  AC_DEFINE(HAVE_PRINTF_Z)
fi])

dnl Checks if *printf() supports %lld
dnl Defines HAVE_PRINTF_LL on success.
AC_DEFUN([AC_FUNC_PRINTF_LL],
[AC_CACHE_CHECK([whether *printf() supports %lld], ac_cv_func_printf_ll,
                [AC_TRY_RUN([
int
main()
{ long long ll = 123456789012LL;
char buf[32];
sprintf(buf, "%lld", ll);
if (strcmp(buf, "123456789012") == 0)
	exit(0);
exit(1);}],
                [ac_cv_func_printf_ll=yes],
                [ac_cv_func_printf_ll=no])])
if test $ac_cv_func_printf_ll = yes; then
  AC_DEFINE(HAVE_PRINTF_LL)
fi])

dnl Checks if C99 compliant isinf() exists
dnl Defines HAVE_C99_ISINF on success.
AC_DEFUN([AC_FUNC_C99_ISINF],
[AC_CACHE_CHECK([for C99 compliant isinf], ac_cv_func_c99_isinf,
                [AC_TRY_LINK([#include <math.h>], [ double	d;
#ifndef	isinf
The isinf macro is not defined
#endif
return (isinf(d));
],
                [ac_cv_func_c99_isinf=yes],
                [ac_cv_func_c99_isinf=no])])
if test $ac_cv_func_c99_isinf = yes; then
  AC_DEFINE(HAVE_C99_ISINF)
fi])

dnl Checks if C99 compliant isnan() exists
dnl Defines HAVE_C99_ISNAN on success.
AC_DEFUN([AC_FUNC_C99_ISNAN],
[AC_CACHE_CHECK([for C99 compliant isnan], ac_cv_func_c99_isnan,
                [AC_TRY_LINK([#include <math.h>], [ double	d;
#ifndef	isnan
The isnan macro is not defined
#endif
return (isnan(d));
],
                [ac_cv_func_c99_isnan=yes],
                [ac_cv_func_c99_isnan=no])])
if test $ac_cv_func_c99_isnan = yes; then
  AC_DEFINE(HAVE_C99_ISNAN)
fi])

dnl Checks if select() needs more than sys/time.h & sys/types.h
dnl Defines SELECT_NONSTD_HDR on success.
AC_DEFUN([AC_HEADER_SELECT_NONSTD],
[AC_CACHE_CHECK([if select needs nonstd include files], ac_cv_header_slect_nonstd_hdr,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/time.h>],
			[fd_set rfd; FD_ZERO(&rfd); select(1, &rfd, 0, 0, 0);],
				[ac_cv_header_slect_nonstd_hdr=no],
				[ac_cv_header_slect_nonstd_hdr=yes])])
if test $ac_cv_header_slect_nonstd_hdr = yes; then
  AC_DEFINE(SELECT_NONSTD_HDR)
fi])

dnl Checks if select() needs sys/select.h
dnl Defines NEED_SYS_SELECT_H on success.
AC_DEFUN([AC_HEADER_SYS_SELECT],
[AC_REQUIRE([AC_HEADER_SELECT_NONSTD])dnl
AC_CACHE_CHECK([if sys/select.h is needed for select], ac_cv_header_need_sys_select_h,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#ifndef	SELECT_NONSTD_HDR
do not compile if we do not need nonstandard headers
#endif],
			[fd_set rfd; FD_ZERO(&rfd); select(1, &rfd, 0, 0, 0);],
				[ac_cv_header_need_sys_select_h=yes],
				[ac_cv_header_need_sys_select_h=no])])
if test $ac_cv_header_need_sys_select_h = yes; then
  AC_DEFINE(NEED_SYS_SELECT_H)
fi])

dnl Checks if select() needs sys/socket.h
dnl Defines NEED_SYS_SOCKET_H on success.
AC_DEFUN([AC_HEADER_SELECT2],
[AC_REQUIRE([AC_HEADER_SELECT_NONSTD])dnl
AC_CACHE_CHECK([if sys/socket.h is needed for select], ac_cv_header_need_sys_socket_h,
                [AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#ifndef	SELECT_NONSTD_HDR
do not compile if we do not need nonstandard headers
#endif],
			[fd_set rfd; FD_ZERO(&rfd); select(1, &rfd, 0, 0, 0);],
				[ac_cv_header_need_sys_socket_h=yes],
				[ac_cv_header_need_sys_socket_h=no])])
if test $ac_cv_header_need_sys_socket_h = yes; then
  AC_DEFINE(NEED_SYS_SOCKET_H)
fi])

dnl Checks if file locking via fcntl() is available
dnl Defines HAVE_FCNTL_LOCKF on success.
AC_DEFUN([AC_FUNC_FCNTL_LOCKF],
[AC_CACHE_CHECK([for file locking via fcntl], ac_cv_func_fcntl_lock,
                [AC_TRY_LINK([
#include <sys/types.h>
#include <fcntl.h>], 
		[
struct flock fl;
fcntl(0, F_SETLK, &fl);],
                [ac_cv_func_fcntl_lock=yes],
                [ac_cv_func_fcntl_lock=no])])
if test $ac_cv_func_fcntl_lock = yes; then
  AC_DEFINE(HAVE_FCNTL_LOCKF)
fi])


dnl Checks if sigsetjmp() is available
dnl Defines HAVE_SIGSETJMP on success.
AC_DEFUN([AC_FUNC_SIGSETJMP],
[AC_CACHE_CHECK([for sigsetjmp], ac_cv_func_sigsetjmp,
                [AC_TRY_LINK([
#include <setjmp.h>],
		[
sigjmp_buf jb;
sigsetjmp(jb, 1);],
                [ac_cv_func_sigsetjmp=yes],
                [ac_cv_func_sigsetjmp=no])])
if test $ac_cv_func_sigsetjmp = yes; then
  AC_DEFINE(HAVE_SIGSETJMP)
fi])

dnl Checks if siglongjmp() is available
dnl Defines HAVE_SIGLONGJMP on success.
AC_DEFUN([AC_FUNC_SIGLONGJMP],
[AC_CACHE_CHECK([for siglongjmp], ac_cv_func_siglongjmp,
                [AC_TRY_LINK([
#include <setjmp.h>],
		[
sigjmp_buf jb;
sigsetjmp(jb, 1);
siglongjmp(jb, 1);],
                [ac_cv_func_siglongjmp=yes],
                [ac_cv_func_siglongjmp=no])])
if test $ac_cv_func_siglongjmp = yes; then
  AC_DEFINE(HAVE_SIGLONGJMP)
fi])


dnl Checks if link() allows hard links on symlinks
dnl Defines HAVE_HARD_SYMLINKS on success.
AC_DEFUN([AC_HARD_SYMLINKS],
[AC_CACHE_CHECK([if link() allows hard links on symlinks], ac_cv_hard_symlinks,
                [AC_TRY_RUN([
main()
{
	int	ret = 0;

	unlink("confdefs.f1");
	unlink("confdefs.l1");
	unlink("confdefs.h1");

	if (symlink("confdefs.f1", "confdefs.l1") < 0)
		ret = 1;
	if (link("confdefs.l1", "confdefs.h1") < 0)
		ret = 1;

	unlink("confdefs.l1");
	unlink("confdefs.h1");

	exit(ret);
}],
                [ac_cv_hard_symlinks=yes],
                [ac_cv_hard_symlinks=no])])
if test $ac_cv_hard_symlinks = yes; then
  AC_DEFINE(HAVE_HARD_SYMLINKS)
fi])


dnl Checks if link() does not follow symlinks
dnl Defines HAVE_LINK_NOFOLLOW on success.
AC_DEFUN([AC_LINK_NOFOLLOW],
[AC_CACHE_CHECK([if link() does not folow symlinks], ac_cv_link_nofollow,
                [AC_TRY_RUN([
#include <sys/types.h>
#include <sys/stat.h>

main()
{
	int	ret = 0;
	int	f;
	struct stat sb;

	unlink("confdefs.f1");
	unlink("confdefs.l1");
	unlink("confdefs.h1");

	f = creat("confdefs.f1", 0666);
	close(f);
	if (symlink("confdefs.f1", "confdefs.l1") < 0)
		ret = 1;
	if (link("confdefs.l1", "confdefs.h1") < 0)
		ret = 1;

	stat("confdefs.f1", &sb);
	if (sb.st_nlink == 2)
		ret = 1;

	unlink("confdefs.f1");
	unlink("confdefs.l1");
	unlink("confdefs.h1");

	exit(ret);
}],
                [ac_cv_link_nofollow=yes],
                [ac_cv_link_nofollow=no])])
if test $ac_cv_link_nofollow = yes; then
  AC_DEFINE(HAVE_LINK_NOFOLLOW)
fi])

dnl Checks if access() does implement E_OK (010) for effective UIDs
dnl Defines HAVE_ACCESS_E_OK on success.
AC_DEFUN([AC_ACCESS_E_OK],
[AC_REQUIRE([AC_HEADER_ERRNO_DEF])dnl
AC_CHECK_HEADERS(unistd.h)
AC_CACHE_CHECK([if access() does implement E_OK], ac_cv_access_e_ok,
                [AC_TRY_RUN([
# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif
#ifndef	R_OK
#define	R_OK	4	/* Test for Read permission */
#define	W_OK	2	/* Test for Write permission */
#define	X_OK	1	/* Test for eXecute permission */
#define	F_OK	0	/* Test for existence of File */
#endif

#ifndef	E_OK
#ifdef	EFF_ONLY_OK
#define	E_OK	EFF_ONLY_OK /* Irix */
#else
#ifdef	EUID_OK
#define	E_OK	EUID_OK	/* UNICOS (0400) */
#else
#define	E_OK	010	/* Test effective uids */
#endif
#endif
#endif

#include <errno.h>
#ifndef	HAVE_ERRNO_DEF
extern	int	errno;
#endif

main()
{
#ifdef	_MSC_VER
/*
 * If we use "cl" to compile and call access(".", E_OK|F_OK), the program will
 * cause an exception that results in a popup window. For this reason, the test
 * is disabled for "cl",  it would not work anyway.
 */	
	int	ret = 1;
#else
	int	ret = 0;

	if (access(".", F_OK) != 0)
		ret = 1;
	else if (access(".", E_OK|F_OK) != 0)
		ret = 1;
	else if (access(".", (R_OK<<4)|F_OK) == 0)
		ret = 1;
#endif

	exit(ret);
}],
                [ac_cv_access_e_ok=yes],
                [ac_cv_access_e_ok=no])])
if test $ac_cv_access_e_ok = yes; then
  AC_DEFINE(HAVE_ACCESS_E_OK)
fi])

dnl Checks if fnmatch() does implement FNM_IGNORECASE
dnl Defines HAVE_FNMATCH_IGNORECASE on success.
AC_DEFUN([AC_FNMATCH_IGNORECASE],
[AC_CHECK_HEADERS(fnmatch.h)
AC_CACHE_CHECK([if fnmatch() does implement FNM_IGNORECASE or FNM_CASEFOLD], ac_cv_fnmatch_igncase,
                [AC_TRY_RUN([
#ifdef	HAVE_FNMATCH
#include <fnmatch.h>
#endif

#ifndef	FNM_IGNORECASE
#ifdef	FNM_CASEFOLD
#define	FNM_IGNORECASE	FNM_CASEFOLD
#endif
#endif

main()
{
	int	ret;

	ret = fnmatch("a", "A", FNM_IGNORECASE);
	exit(ret);
}],
                [ac_cv_fnmatch_igncase=yes],
                [ac_cv_fnmatch_igncase=no])])
if test $ac_cv_fnmatch_igncase = yes; then
  AC_DEFINE(HAVE_FNMATCH_IGNORECASE)
fi])



dnl Checks if realloc() does implement realloc(NULL, size)
dnl Defines HAVE_REALLOC_NULL on success.
AC_DEFUN([AC_FUNC_REALLOC_NULL],
[AC_CHECK_HEADERS(stdlib.h)
AC_TYPE_SIGNAL
AC_CACHE_CHECK([if realloc() does implement realloc(NULL, size)], ac_cv_realloc_null,
                [AC_TRY_RUN([
#ifdef	HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <signal.h>

RETSIGTYPE
sig(s)
	int	s;
{
	exit(1);
}
int
main()
{
	char	*p;

	signal(SIGSEGV, sig);
	p = realloc((char *)0, 10);
	if (p == (char *)0)
		exit(1);
	exit(0);
}],
                [ac_cv_realloc_null=yes],
                [ac_cv_realloc_null=no])])
if test $ac_cv_realloc_null = yes; then
  AC_DEFINE(HAVE_REALLOC_NULL)
fi])


dnl Checks if waitid() is present and is at least minimally usable.
dnl Mac OS X is POSIX certified but definitely not POSIX compliant
dnl so we need to to implement a complex test for waitid().
dnl Defines HAVE_WAITID on success.
AC_DEFUN([AC_FUNC_WAITID],
[AC_CHECK_HEADERS(stdlib.h)
AC_CHECK_HEADERS(unistd.h)
AC_CHECK_HEADERS(wait.h)
AC_HEADER_SYS_WAIT
AC_CACHE_CHECK([for halfway POSIX compliant waitid()], ac_cv_func_waitid,
                [AC_TRY_RUN([
#ifdef	HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef	HAVE_UNISTD_H
#include <unistd.h>
#endif
#if	defined(HAVE_WAIT_H)
#	include <wait.h>
#else
#include <sys/wait.h>
#endif
#include <signal.h>
/*
 * Non-standard compliant platforms may need 
 * #include <signal.h> or something similar
 * in addition to the include files above.
 */

int
main()
{
	siginfo_t	si;
	pid_t	pid;
	int	ret;

	if ((pid = fork()) < 0)
		exit(1);
	if (pid == 0) {
		_exit(1234567890);
	}
	ret = waitid(P_PID, pid, &si, WEXITED);
	if (ret < 0)
		exit(1);
	if (pid != si.si_pid)		/* Mac OS X has si.si_pid == 0 */
		exit(2);
	if (si.si_code != CLD_EXITED)	/* Mac OS X has si.si_code == 0 */
		exit(3);
	if ((si.si_status & 0xFFFF) != (1234567890 & 0xFFFF))
		exit(4);		/* Should deliver more than 8 bits */
					/* Linux only delivers 8 bits */
					/* Mac OS X delivers 24 bits */

	exit(0);
}],
                [ac_cv_func_waitid=yes],
                [ac_cv_func_waitid=no])])
if test $ac_cv_func_waitid = yes; then
  AC_DEFINE(HAVE_WAITID)
fi])


dnl Checks whether waitpid() is present and supports WNOWAIT.
dnl SVr4 supports it but POSIX does not list it.
dnl Defines HAVE_WNOWAIT_WAITPID on success.
AC_DEFUN([AC_WNOWAIT_WAITPID],
[AC_CHECK_HEADERS(stdlib.h)
AC_CHECK_HEADERS(unistd.h)
AC_CHECK_HEADERS(wait.h)
AC_HEADER_SYS_WAIT
AC_HEADER_ERRNO_DEF
AC_CACHE_CHECK([for SVr4 compliant waitpid() with WNOWAIT support], ac_cv_wnowait_waitpid,
                [AC_TRY_RUN([
#ifdef	HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef	HAVE_UNISTD_H
#include <unistd.h>
#endif
#if	defined(HAVE_WAIT_H)
#	include <wait.h>
#else
#include <sys/wait.h>
#endif
#include <errno.h>
#ifndef	HAVE_ERRNO_DEF
extern	int	errno;
#endif

int
main()
{
	int	xstat;
	pid_t	pid = -1;
	int	ret;

	ret = waitpid(pid, &xstat, WNOWAIT);
	if (ret < 0 && errno == EINVAL)
		exit(1);
	exit(0);
}],
                [ac_cv_wnowait_waitpid=yes],
                [ac_cv_wnowait_waitpid=no])])
if test $ac_cv_wnowait_waitpid = yes; then
  AC_DEFINE(HAVE_WNOWAIT_WAITPID)
fi])


dnl XXXXXXXXXXXXXXXXXX Begin Stolen (but modified) from GNU tar XXXXXXXXXXXXXXXXXXXXXXXXXXX
dnl Changes:

dnl One line has been changed to:    [ac_save_CC="${CC-cc}" to default to "'cc"

dnl AC_SYS_LARGEFILE_MACRO_VALUE test moved from AC_FUNC_FSEEKO into AC_SYS_LARGEFILE
dnl Do not call AC_FUNC_FSEEKO because it does not check whether fseeko() is
dnl available on non Large File mode. There are additionoal tests for fseeko()/ftello()
dnl inside the AC_HAVE_LARGEFILES test.

dnl largefile_cc_opt definition added

#serial 18

dnl By default, many hosts won't let programs access large files;
dnl one must use special compiler options to get large-file access to work.
dnl For more details about this brain damage please see:
dnl http://www.sas.com/standards/large.file/x_open.20Mar96.html

dnl Written by Paul Eggert <eggert@twinsun.com>.

dnl Internal subroutine of AC_SYS_LARGEFILE.
dnl AC_SYS_LARGEFILE_TEST_INCLUDES
AC_DEFUN([AC_SYS_LARGEFILE_TEST_INCLUDES],
  [[#include <sys/types.h>
    /* Check that off_t can represent 2**63 - 1 correctly.
       We can't simply "#define LARGE_OFF_T 9223372036854775807",
       since some C++ compilers masquerading as C compilers
       incorrectly reject 9223372036854775807.  */
#   define LARGE_OFF_T (((off_t) 1 << 62) - 1 + ((off_t) 1 << 62))
    int off_t_is_large[(LARGE_OFF_T % 2147483629 == 721
			&& LARGE_OFF_T % 2147483647 == 1)
		       ? 1 : -1];
  ]])

dnl Internal subroutine of AC_SYS_LARGEFILE.
dnl AC_SYS_LARGEFILE_MACRO_VALUE(C-MACRO, VALUE, CACHE-VAR, COMMENT, INCLUDES, FUNCTION-BODY)
AC_DEFUN([AC_SYS_LARGEFILE_MACRO_VALUE],
  [AC_CACHE_CHECK([for $1 value needed for large files], $3,
     [$3=no
      AC_TRY_COMPILE([$5],
	[$6], 
	,
	[AC_TRY_COMPILE([#define $1 $2]
[$5]
	   ,
	   [$6],
	   [$3=$2])])])
   if test "[$]$3" != no; then
     AC_DEFINE_UNQUOTED([$1], [$]$3, [$4])
   fi])

AC_DEFUN([AC_SYS_LARGEFILE],
  [AC_ARG_ENABLE(largefile,
     [  --disable-largefile     omit support for large files])
   if test "$enable_largefile" != no; then

     AC_CACHE_CHECK([for special C compiler options needed for large files],
       ac_cv_sys_largefile_CC,
       [ac_cv_sys_largefile_CC=no
        largefile_cc_opt=""
        if test "$GCC" != yes; then
	  # IRIX 6.2 and later do not support large files by default,
	  # so use the C compiler's -n32 option if that helps.
	  AC_TRY_COMPILE(AC_SYS_LARGEFILE_TEST_INCLUDES, , ,
	    [ac_save_CC="${CC-cc}"
	     CC="$CC -n32"
	     AC_TRY_COMPILE(AC_SYS_LARGEFILE_TEST_INCLUDES, ,
	       ac_cv_sys_largefile_CC=' -n32')
	     CC="$ac_save_CC"])
        fi])
     if test "$ac_cv_sys_largefile_CC" != no; then
       CC="$CC$ac_cv_sys_largefile_CC"
       largefile_cc_opt="$ac_cv_sys_largefile_CC"
     fi

     AC_SYS_LARGEFILE_MACRO_VALUE(_FILE_OFFSET_BITS, 64,
       ac_cv_sys_file_offset_bits,
       [Number of bits in a file offset, on hosts where this is settable.],
       AC_SYS_LARGEFILE_TEST_INCLUDES)
     AC_SYS_LARGEFILE_MACRO_VALUE(_LARGE_FILES, 1,
       ac_cv_sys_large_files,
       [Define for large files, on AIX-style hosts.],
       AC_SYS_LARGEFILE_TEST_INCLUDES)
     AC_SYS_LARGEFILE_MACRO_VALUE(_LARGEFILE_SOURCE, 1,
       ac_cv_sys_largefile_source,
       [Define to make fseeko visible on some hosts (e.g. glibc 2.2).],
       [#include <stdio.h>], [return !fseeko;])
   fi
  ])


AC_DEFUN([AC_FUNC_FSEEKO],
  [AC_SYS_LARGEFILE_MACRO_VALUE(_LARGEFILE_SOURCE, 1,
     ac_cv_sys_largefile_source,
     [Define to make fseeko visible on some hosts (e.g. glibc 2.2).],
     [#include <stdio.h>], [return !fseeko;])
   # We used to try defining _XOPEN_SOURCE=500 too, to work around a bug
   # in glibc 2.1.3, but that breaks too many other things.
   # If you want fseeko and ftello with glibc, upgrade to a fixed glibc.

   AC_CACHE_CHECK([for fseeko], ac_cv_func_fseeko,
     [ac_cv_func_fseeko=no
      AC_TRY_LINK([#include <stdio.h>],
        [return fseeko && fseeko (stdin, 0, 0);],
	[ac_cv_func_fseeko=yes])])
   if test $ac_cv_func_fseeko != no; then
     AC_DEFINE(HAVE_FSEEKO, 1,
       [Define if fseeko (and presumably ftello) exists and is declared.])
   fi])


dnl XXXXXXXXXXXXXXXXXX End Stolen (but modified) from GNU tar XXXXXXXXXXXXXXXXXXXXXXXXXXXXX

AC_DEFUN([AC_HAVE_LARGEFILES],
[AC_CACHE_CHECK([if system supports Large Files at all], ac_cv_largefiles,
     	[AC_TRY_COMPILE([#include <stdio.h>
#include <sys/types.h>],
     		[
/*
 * Check that off_t can represent 2**63 - 1 correctly.
 * We can't simply "#define LARGE_OFF_T 9223372036854775807",
 * since some C++ compilers masquerading as C compilers
 * incorrectly reject 9223372036854775807.
 */
#   define LARGE_OFF_T (((off_t) 1 << 62) - 1 + ((off_t) 1 << 62))
    int off_t_is_large[(LARGE_OFF_T % 2147483629 == 721
			&& LARGE_OFF_T % 2147483647 == 1)
		       ? 1 : -1];
return !fseeko;
return !ftello;],
     		[ac_cv_largefiles=yes],
     		[ac_cv_largefiles=no])])
	if test $ac_cv_largefiles = yes; then
		AC_DEFINE(HAVE_LARGEFILES)
	fi])

dnl Checks for whether fseeko() is available in non large file mode
dnl and whether there is a prototype for fseeko()
dnl Defines HAVE_FSEEKO on success.
AC_DEFUN([AC_SMALL_FSEEKO],
[AC_CACHE_CHECK([for fseeko()], ac_cv_func_fseeko,
                [AC_TRY_LINK([#include <stdio.h>],
[return !fseeko;],
                [ac_cv_func_fseeko=yes],
                [ac_cv_func_fseeko=no])])
if test $ac_cv_func_fseeko = yes; then
  AC_DEFINE(HAVE_FSEEKO)
fi])

dnl Checks for whether ftello() is available in non large file mode
dnl and whether there is a prototype for ftello()
dnl Defines HAVE_FTELLO on success.
AC_DEFUN([AC_SMALL_FTELLO],
[AC_CACHE_CHECK([for ftello()], ac_cv_func_ftello,
                [AC_TRY_LINK([#include <stdio.h>],
[return !ftello;],
                [ac_cv_func_ftello=yes],
                [ac_cv_func_ftello=no])])
if test $ac_cv_func_ftello = yes; then
  AC_DEFINE(HAVE_FTELLO)
fi])

dnl Checks if compiler allows dynamic arrays.
dnl Defines HAVE_DYN_ARRAYS on success.
AC_DEFUN([AC_DYN_ARRAYS],
[AC_CACHE_CHECK([if compiler allows dynamic arrays], ac_cv_dyn_arrays,
                [AC_TRY_COMPILE([],
                                [extern int __aa(); int len = __aa(); char some_array[len];],
                                [ac_cv_dyn_arrays=yes],
                                [ac_cv_dyn_arrays=no])])
if test $ac_cv_dyn_arrays = yes; then
  AC_DEFINE(HAVE_DYN_ARRAYS)
fi])

dnl Checks if the compiler allows #pragma weak
dnl Defines HAVE_PRAGMA_WEAK on success.
AC_DEFUN([AC_PRAGMA_WEAK],
[AC_CACHE_CHECK([if compiler allows pragma weak], ac_cv_pragma_weak,
                [AC_TRY_LINK([extern int test2();
#pragma weak test2 = test1
int test1() {return 0;}],
[return test2();],
                [ac_cv_pragma_weak=yes],
                [ac_cv_pragma_weak=no])])
if test $ac_cv_pragma_weak = yes; then
  AC_DEFINE(HAVE_PRAGMA_WEAK)
fi])

dnl Checks if Linux include file linux/ext2_fs.h is broken
dnl Defines HAVE_BROKEN_LINUX_EXT2_FS_H on success.
AC_DEFUN([AC_BROKEN_LINUX_EXT2_FS_H],
[AC_CACHE_CHECK([if Linux include file linux/ext2_fs.h is broken], ac_cv_broken_linux_ext2_fs_h,
                [AC_TRY_COMPILE([
#if defined(__linux__) || defined(__linux) || defined(linux)
#include <linux/ext2_fs.h>
#endif],
                                [],
                                [ac_cv_broken_linux_ext2_fs_h=no],
                                [ac_cv_broken_linux_ext2_fs_h=yes])])
if test $ac_cv_broken_linux_ext2_fs_h = yes; then
  AC_DEFINE(HAVE_BROKEN_LINUX_EXT2_FS_H)
fi])

dnl Checks if Linux include file /usr/src/linux/include/linux/ext2_fs.h is broken
dnl Defines HAVE_BROKEN_SRC_LINUX_EXT2_FS_H on success.
AC_DEFUN([AC_BROKEN_SRC_LINUX_EXT2_FS_H],
[AC_CACHE_CHECK([if Linux include file /usr/src/linux/include/linux/ext2_fs.h is broken], ac_cv_broken_src_linux_ext2_fs_h,
                [___CPPFLAGS=$CPPFLAGS; CPPFLAGS="-I/usr/src/linux/include $CPPFLAGS"
		AC_TRY_COMPILE([
#if defined(__linux__) || defined(__linux) || defined(linux)
#include <linux/ext2_fs.h>
#endif],
                                [],
                                [ac_cv_broken_src_linux_ext2_fs_h=no],
                                [ac_cv_broken_src_linux_ext2_fs_h=yes])])
CPPFLAGS=$___CPPFLAGS
if test $ac_cv_broken_src_linux_ext2_fs_h = yes; then
  AC_DEFINE(HAVE_BROKEN_SRC_LINUX_EXT2_FS_H)
fi])

dnl Checks if Linux include file scsi/scsi.h is broken
dnl Defines HAVE_BROKEN_SCSI_SCSI_H on success.
AC_DEFUN([AC_BROKEN_SCSI_SCSI_H],
[AC_CACHE_CHECK([if Linux include file scsi/scsi.h is broken], ac_cv_broken_scsi_scsi_h,
                [AC_TRY_COMPILE([
#if defined(__linux__) || defined(__linux) || defined(linux)
#include <scsi/scsi.h>
#endif],
                                [],
                                [ac_cv_broken_scsi_scsi_h=no],
                                [ac_cv_broken_scsi_scsi_h=yes])])
if test $ac_cv_broken_scsi_scsi_h = yes; then
  AC_DEFINE(HAVE_BROKEN_SCSI_SCSI_H)
fi])

dnl Checks if Linux include file /usr/src/linux/include/scsi/scsi.h is broken
dnl Defines HAVE_BROKEN_SRC_SCSI_SCSI_H on success.
AC_DEFUN([AC_BROKEN_SRC_SCSI_SCSI_H],
[AC_CACHE_CHECK([if Linux include file /usr/src/linux/include/scsi/scsi.h is broken], ac_cv_broken_src_scsi_scsi_h,
                [___CPPFLAGS=$CPPFLAGS; CPPFLAGS="-I/usr/src/linux/include $CPPFLAGS"
                AC_TRY_COMPILE([
#if defined(__linux__) || defined(__linux) || defined(linux)
#include <scsi/scsi.h>
#endif],
                                [],
                                [ac_cv_broken_src_scsi_scsi_h=no],
                                [ac_cv_broken_src_scsi_scsi_h=yes])])
CPPFLAGS=$___CPPFLAGS
if test $ac_cv_broken_src_scsi_scsi_h = yes; then
  AC_DEFINE(HAVE_BROKEN_SRC_SCSI_SCSI_H)
fi])

dnl Checks if Linux include file scsi/sg.h is broken
dnl Defines HAVE_BROKEN_SCSI_SG_H on success.
AC_DEFUN([AC_BROKEN_SCSI_SG_H],
[AC_CACHE_CHECK([if Linux include file scsi/sg.h is broken], ac_cv_broken_scsi_sg_h,
                [AC_TRY_COMPILE([
#if defined(__linux__) || defined(__linux) || defined(linux)
#include <sys/types.h>
#include <scsi/sg.h>
#endif],
                                [],
                                [ac_cv_broken_scsi_sg_h=no],
                                [ac_cv_broken_scsi_sg_h=yes])])
if test $ac_cv_broken_scsi_sg_h = yes; then
  AC_DEFINE(HAVE_BROKEN_SCSI_SG_H)
fi])

dnl Checks if Linux include file /usr/src/linux/include/scsi/sg.h is broken
dnl Defines HAVE_BROKEN_SRC_SCSI_SG_H on success.
AC_DEFUN([AC_BROKEN_SRC_SCSI_SG_H],
[AC_CACHE_CHECK([if Linux include file /usr/src/linux/include/scsi/sg.h is broken], ac_cv_broken_src_scsi_sg_h,
                [___CPPFLAGS=$CPPFLAGS; CPPFLAGS="-I/usr/src/linux/include $CPPFLAGS"
                AC_TRY_COMPILE([
#if defined(__linux__) || defined(__linux) || defined(linux)
#include <sys/types.h>
#include <scsi/sg.h>
#endif],
                                [],
                                [ac_cv_broken_src_scsi_sg_h=no],
                                [ac_cv_broken_src_scsi_sg_h=yes])])
CPPFLAGS=$___CPPFLAGS
if test $ac_cv_broken_src_scsi_sg_h = yes; then
  AC_DEFINE(HAVE_BROKEN_SRC_SCSI_SG_H)
fi])

AC_DEFUN(AC_PROG_SHELL_BROKEN_E,
[AC_CACHE_CHECK(whether handling of /bin/sh -ce 'command' is broken, ac_cv_prog_shell_broken_e,
[if AC_TRY_COMMAND(/bin/sh -ce 'if false; true; then echo yes; else echo no; fi') | egrep yes >/dev/null 2>&1; then
  ac_cv_prog_shell_broken_e=no
else
  ac_cv_prog_shell_broken_e=yes
fi
if test $ac_cv_prog_shell_broken_e = yes; then
  AC_DEFINE(HAVE_PROG_SHELL_BROKEN_E)
fi])])

dnl Checks if a file exists
dnl Do not use test -r <file> as bash on Haiku returns 0 for test -r /dev/stdin
dnl even though /dev/stdin does not exist.
dnl
dnl Defines HAVE_<PATH> on success.
dnl AC_STAT_FILE(FILE, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
AC_DEFUN([AC_STAT_FILE],
[AC_REQUIRE([AC_PROG_CC]) 
dnl Do the transliteration at runtime so arg 1 can be a shell variable. 
ac_safe=`echo "$1" | sed 'y%./+-%__p_%'` 
AC_MSG_CHECKING([for $1])
AC_CACHE_VAL(ac_cv_file_$ac_safe,
                [AC_TRY_RUN([
#include <sys/types.h>
#include <sys/stat.h>

int
main()
{
	struct	stat	sb;

	exit(stat("$1", &sb) < 0);
}],
                [eval ac_cv_file_$ac_safe=yes],
                [eval ac_cv_file_$ac_safe=no])])
if eval "test \"`echo '$ac_cv_file_'$ac_safe`\" = yes"; then
  AC_MSG_RESULT(yes)
  ifelse([$2], , :, [$2])
else
  AC_MSG_RESULT(no)
  ifelse([$3], , , [$3])
fi
])

dnl AC_STAT_FILES(FILE... [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
AC_DEFUN(AC_STAT_FILES,
[for ac_file in $1
do
AC_STAT_FILE($ac_file,
[changequote(, )dnl
  ac_tr_file=HAVE_`echo $ac_file | sed 'y%abcdefghijklmnopqrstuvwxyz./-%ABCDEFGHIJKLMNOPQRSTUVWXYZ___%'`
changequote([, ])dnl
  AC_DEFINE_UNQUOTED($ac_tr_file) $2], $3)dnl
done
])
