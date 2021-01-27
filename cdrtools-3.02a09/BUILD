# @(#)README.compile	1.35 17/11/28 Copyright 1997-2017 J. Schilling

Short overview for those who don't read manuals:

	Calling configure manually is outdated because this is a task of the
	makefile system.

	There is no 'configure', simply call 'make' on the top level
	directory.

	***** If this does not work for you, read the rest if this file   *****
	***** If you have any problem, also first read the topic specific *****
	***** README.* files (e.g. README.linux for Linux problems).	  *****

	All results in general will be placed into a directory named 
	OBJ/<arch-name>/ in the current projects leaf directory.

	You **need** either the Schily "smake" program, the SunPRO make 
	from /usr/bin/make (SunOS 4.x) or /usr/ccs/bin/make (SunOS 5.x)
	or GNU make to compile this program. Read READMEs/README.gmake for 
	more information on gmake and a list of the most annoying bugs in gmake.

	All other make programs are either not smart enough or have bugs.

	My "smake" source is at:

	https://sourceforge.net/projects/s-make/files/

	It is easy to compile and doesn't need a working make program
	on your machine. If you don't have a working "make" program on the
	machine where you like to compile "smake" read the file "BOOTSTRAP".

	If you have the choice between all three make programs, the
	preference would be 

		1)	smake		(preferred)
		2)	SunPRO make
		3)	GNU make	(this is the last resort)

	Important notice: "smake" that comes with SGI/IRIX will not work!!!
	This is not the Schily "smake" but a dumb make program from SGI.

	***** If you are on a platform that is not yet known by the	 *****
	***** Schily makefilesystem you cannot use GNU make.		 *****
	***** In this case, the automake features of smake are required. *****

	Note that GNU make has major bugs on various platforms and thus cannot
	be used at all on VMS and OS/2. GNU make on Cygwin causes problems
	because it does not deal with spaces and newlines correctly.

	Please read the README's for your operating system too.

			WARNING
	Do not use 'mc' to extract the tar file!
	All mc versions before 4.0.14 cannot extract symbolic links correctly.

	The versions of WinZip that support tar archives cannot be used either.
	The reason is that they don't support symbolic links.
	Star and Gnutar do support symbolic links even on win32 systems.
	To support symbolic links on win32, you need to link with the
	Cygwin32 POSIX library.

	To unpack an archive, use:

		gzip -d < some-arch.tar.gz | tar xpf -

	Replace 'some-arch.tar.gz' by the actual archive name.

	If your platform does not support hard links or symbolic links, you
	first need to compile "star" and then call:

		star -xp -copy-links < some-arch.tar.gz

	If your platform does not support hard links but supports
	symbolic links, you only need to call the command above once.
	If your platform does not support symbolic links, you need to call
	the command twice because a symbolic link may occur in the archive
	before the file it points to.
		


Here comes the long form:


PREFACE:

	Calling configure manually is outdated because this is a task of the
	makefile system.

	You don't have to call configure with this make file system.

	Calling	'make' or 'make all' on the top level directory will create
	all needed targets. Calling 'make install' will install all needed
	files.

	This program uses a new makefilesystem, introduced in 1993. This
	makefilesystem uses techniques and ideas from the 1980s and 1990s,
	is designed in a modular way and allows sources to be combined in a
	modular way. For mor information on the modular features read
	README.SSPM.

	The makefilesystem is optimized for a program called 'smake'
	Copyright 1985 by Jörg Schilling, but SunPro make (the make program
	that comes with SunOS >= 4.0 and Solaris) as well as newer versions
	of GNU make will work also. BSDmake could be made working, if it
	supports pattern matching rules correctly.

	The makefile system allows simultaneous compilation on a wide
	variety of target systems if the source tree is accessible via NFS.


Finding Compilation Results:

	To allow simultaneous compilations, all binaries and results of a
	'compilation' in any form are placed in sub-directories. This includes
	automatically generated include files. Results will in general be
	placed into a directory named OBJ/<arch-name>/ in the current project's
	leaf directory, libraries will be placed into a directory called
	libs/<arch-name>/ that is located in the source tree root directory.

		<arch-name> will be something like 'sparc-sunos5-cc'

	This is the main reason why simultaneous compilation is possible on
	all supported platforms if the source is mounted via NFS.


How to compile:

	To compile a system or sub-system, simply enter 'smake', 'make' or 
	'Gmake'. Compilation may be initialized at any point of the source
	tree of a system.

	WARNING: If compilation is started in a sub tree, only all objects
	in that sub tree will be made. This usually excludes needed libraries.


How to install results:

	To install the product of a compilation in your system, call:

		smake install

	at top level. The binaries will usually be installed in 
	/opt/schily/bin. The directory /opt/<vendor-name>/ has been agreed
	on by all major UNIX vendors in 1989. Unfortunately, still not all
	vendors follow this agreement.

	If you want to change the default installation directory, edit the
	appropriate (system dependent) files in the DEFAULTS directory
	(e.g. DEFAULTS/Defaults.sunos5).

	***** If "smake install" doesn't do anything, you are on a broken *****
	***** File System. Remove the file INSTALL in this case (the FS   *****
	***** does not handle upper/lower case characters correctly).	  *****
	***** This is true for all DOS based filesystems and for Apple's  *****
	***** HFS+ filesystem.						  *****


Using a different installation directory:

	If your system does not yet use the standard installation path in

		/opt/<vendor>

	or if you don't like this installation directory, you can easily 
	change the installation directory. You may edit the DEFAULTS file 
	for your system and modify the macro INS_BASE.

	You may  use a different installation directory without editing the
	DEFAULTS files. If you like to install everything in the deprecated path
	/usr/local, the next paragraph describes the procedure.

	If your make program supports to propagate make macros to sub make programs
	which is the case for recent smake releases as well as for a recent gnumake:

		smake INS_BASE=/usr/local install
	or
		gmake INS_BASE=/usr/local install

	If your make program doesn't propagate make macros (e.g. SunPRO make),
	call:

		env INS_BASE=/usr/local make -e install

	Note that INS_BASE=/usr/local needs to be specified for every operation
	that compiles or links programs, as the path may be stored inside the
	binaries.

	The location for the root specific configuratin files is controlled
	via the INS_RBASE= make macro. The default vaulue for this macro is "/".
	If you like to install global default configuration files into 
	/usr/local/etc instead of /etc, you need to spefify INS_RBASE=/usr/local

	Note that some binaries have $(INS_BASE) and $(INS_RBASE) compiled into.
	If you like to like to modify the compiled-in path values, call:

		smake clean
		smake INS_BASE=/usr/local INS_RBASE=/usr/local


Setting up a different Link mode:

	The following link modes are available:

		static		statical linking as in historical UNIX

		dynamic		dynamic linking as introduced by SunOS
				in 1987, Microsoft's DLLs, ...
				The knowledge on how to achieve this for
				a particular platform is hidden in the
				makefile system.

		profiled	Linking against profiled libraries.
				Profiled libraries are prepared for the
				use with "gprof" (introduced by BSD in the
				late 1970s).

	The makefile system sets up a default linkmode in the patform
	related defaults file (typically in the file DEFAULTS/Defaults.<platform>)
	in the projects root directory. This is done with the entry:

		DEFLINKMODE=   <linkmode>

	A different linkmode may be selected at compile/link time by e.g. calling:

		smake LINKMODE=dynamic

	If there are already existing binaries, call:

		smake relink LINKMODE=dynamic

	instead.


Compiling a different ELF RUNPATH into the binaries:

	In order to allow binaries to work correctly even if the shared
	libraries are not in the default search path of the runtime linker,
	a RUNPATH needs to be set.

	The ELF RUNPATH is by default derived from $(INS_BASE). If you like to
	set INS_BASE=/usr and create binaries that do not include a RUNPATH at all,
	call:

		smake relink RUNPATH=


Using a different man path prefix:

	Manual pages are by default installed under:

	$(INS_BASE)/$(MANBASE)/man
	and MANBASE=share

	If you like a different prefix for man pages, call:

		smake DEFMANBASE=something install

	to install man pages into $(INS_BASE)/something/man/*

	If you like to install man pages under $(INS_BASE)/man/*, call

		smake DEFMANBASE=. install

Installing stripped binaries:

	If you like to install stripped binaries via "smake install", call:

		smake STRIPFLAGS=-s install

	This calls "strip" on every final install path for all executable
	binaries.

Installing to a prototype directory to implement package creation staging:

	If you like to create a prototype directory tree that is used as an
	intermediate store for package creation, use the DESTDIR macro:

		smake INS_BASE=/usr/local DESTDIR=/tmp install

	This will compile in "/usr/local" as prefix into all related binaries
	and then create a usr/local tree below /tmp (i.e. /tmp/usr/local).

	Note that you need to call "smake clean" before in case that the code
	was previously compiled with different defaults with regards to INS_BASE

Setting different default directory permissions for install directories:

	All directories that are created by the Schily makefile system in the
	target directory path when

		smake install

	is called system use a special default 022 that is in DEFINSUMASK=
	This causes all directories in the target install path to be created
	with 0755 permissions.

	All other directories that are created by the Schily makefile system 
	use a single global default 002 that is in DEFUMASK=

	If you like to create install directories with e.g. 0775 permissions,
	call:

		smake DEFINSUMASK=002 install

Using a different C compiler:

	The *compiler family* is configured via the CCOM= make macro. This
	selects a whole set of related macros that are needed to support a
	specific compiler family.

	The *compiler family* usually defines a C compiler and a related
	C++ compiler.

	If the configured default compiler family is not present on the current
	machine, the makefilesystem will try an automatic fallback to GCC. For
	this reason, in most cases, you will not need to manually select a
	compiler.

	The default compiler family can be modified in the files in the
	DEFAULT directory. If you want to have a different compiler family
	for one compilation, call:

		make CCOM=gcc
	or
		make CCOM=cc

	This works even when your make program doesn't propagate make macros.


Creating 64 bit executables on Solaris:

	Simply call:

		make CCOM=gcc64
	or
		make CCOM=cc64

	It is not clear if GCC already supports other platforms in 64 bit mode.
	As all GCC versions before 3.1 did emit hundreds of compilation
	warnings related to 64 bit bugs when compiling itself, so there may be
	other platforms are not supported in 64 bit mode.

Creating executables using the Sun Studio compiler on Linux:

	Simply call:

		make CCOM=suncc

	If the compilation does not work, try:

	mkdir   /opt/sunstudio12/prod/include/cc/linux 
	cp      /usr/include/linux/types.h  /opt/sunstudio12/prod/include/cc/linux

	Then edit /opt/sunstudio12/prod/include/cc/linux/types.h and remove all
	lines like: "#if defined(__GNUC__) && !defined(__STRICT_ANSI__)"
	as well as the related #endif.

Creating executables using the clang compiler:

	Simply call:

		make CCOM=clang

	And in order to intentionally create 32 bit or 64 bit binaries, call:

		make CCOM=clang64
	or
		make CCOM=clang64


Using a different compiler binary name:

	Call:

		make CC=/opt/instrumented/bin/cc

	Note that all knowledge about the options of a compiler is derived
	from the CCOM= variable, so if you like to use an instrumented gcc
	variant, you may like to call:

		make CCOM=gcc CC=fluffy-gcc

	You may use CC="fluffy-gcc fluffy-gcc-specific options" if you like
	to enforce specific options with the compiler. See hints on cross
	compilation below.


Getting help from the make file system:

	For a list of targets call:

		make .help

	.help is a special target that prints help for the makefile system.


Getting more information on the make file system:

	The man page makefiles.4 located in man/man4/makefiles.4 contains
	the documentation on general use and for leaf makefiles.

	The man page makerules.4 located in man/man4/makerules.4 contains
	the documentation for system programmers who want to modify
	the make rules of the makefile system.

	For further information read

	http://sf.net/projects/schilytools/files/makefiles/PortableSoftware.ps.gz


Hints for compilation:

	The makefile system is optimized for 'smake'. Smake will give the
	fastest processing and best debugging output.

	SunPro make will work as is. GNU make need some special preparation.

	Read READMEs/README.gmake for more information on gmake.

	To use GNU make create a file called 'Gmake' in your search path
	that contains:

		#!/bin/sh
		MAKEPROG=gmake
		export MAKEPROG
		exec gmake "$@"

	and call 'Gmake' instead of gmake. On Linux, there is no gmake, the
	program installed as 'make' on Linux is really a gmake.

	'Gmake' and 'Gmake.linux' are part of this distribution.

	Some versions of gmake are very buggy. There are e.g. versions of gmake
	on some architectures that will not correctly recognize the default
	target. In this case, call 'make all' or '../Gmake all'.

	Note that pseudo error messages from gmake similar to:

	gmake[1]: Entering directory `cdrtools-1.10/conf'
	../RULES/rules.cnf:58: ../incs/sparc-sunos5-cc/Inull: No such file or directory
	../RULES/rules.cnf:59: ../incs/sparc-sunos5-cc/rules.cnf: No such file or directory

	are a result of a bug in GNU make. The make file system itself is
	correct (as you could prove by using smake).
	If your gmake version still has this bug, send a bug report to:

		"Paul D. Smith" <psmith@gnu.org>

	He is the current GNU make maintainer.

	If you like to use 'smake', please always compile it from source.
	The packages are located on:

		https://sourceforge.net/projects/s-make/files/alpha/

	Smake has a -D flag to see the actual makefile source used
	and a -d flag that gives easy to read debugging info. Use smake -xM
	to get a makefile dependency list. Try smake -help


Compiling the project using engineering defaults:

	The defaults found in the directory DEFAULTS are configured to
	give minimum warnings. This is made because many people will
	be irritated by warning messages and because the GNU C compiler
	will give warnings for perfectly correct and portable C code.

	If you want to port code to new platforms or do engineering
	on the code, you should use the alternate set of defaults found
	in the directory DEFAULTS_ENG.
	You may do this permanently by renaming the directories or
	for one compilation by calling:

		make DEFAULTSDIR=DEFAULTS_ENG

	Note however, that some GCC versions print a lot of wrong warnings
	in this mode. Well known problems with GCC warnings are:

	-	The recursive printf format "%r" that is in use since ~ 1980
		is not supported and causes a lot of incorrect warnings as
		GCC does not know that "%r" takes 2 parameters.

	-	The standard C construct "(void) read(fd, buf, sizeof (buf))"
		is flagged by some versions of GCC even though the void cast
		is a clear expression of the fact that the return code from read
		is intentionally ignored. This may cause many useless warnings
		for last resort error messages used in programs.


Compiling the project to allow debugging with dbx/gdb:

	If you like to compile with debugging information for dbx or gdb,
	call:

		make clean
		make COPTX=-g LDOPTX=-g

	If your debugger does not like optimized binaries, call something
	like:

		 make "COPTX=-g -xO0" LDOPTX=-g
	or
		 make "COPTX=-g -O0" LDOPTX=-g

	depending on the option system used by your C compiler.


Compiling the project to allow performance monitoring with gprof from BSD:

	If you like to compile for performance monitoriing with gprof,
	call:

		make clean
		make COPTX=-xpg LDOPTX=-xpg LINKMODE=profiled

	or
		make COPTX=-pg LDOPTX=-pg LINKMODE=profiled

	depending on the option system used by your C compiler.


Creating Blastwave packages:

	Call:
		.clean
		smake -f Mcsw

	You need the program "fakeroot" and will find the results
	in packages/<arch-dir>.

	Note that a single program source tree will allow you to create
	packages like CSWstar but not the packages CSWschilybase and
	CSWschilyutils on which CSWstar depends.




	If you want to see an example, please have a look at the "star"
	source. It may be found on:

		http://sourceforge.net/projects/s-tar/files/

	Have a look at the manual page, it is included in the distribution.
	Install the manual page with 

	make install first and include /opt/schily/man in your MANPATH

	Note that some systems (e.g. Solaris 2.x) require you either to call
	/usr/lib/makewhatis /opt/schily/man or to call 

		man -F <man-page-name>


Compiling in a cross compilation environment:

	The Schily autoconf system has been enhanced to support cross
	compilation. Schily autoconf is based on GNU autoconf-2.13 and
	GNU autoconf does not support cross compilation because it needs
	to run scripts on the target system for some of the tests.

	The "configure" script that is delivered with the Schily makefile
	system runs more than 770 tests and aprox 70 of them need to be 
	run on the target system.

	The Schily autoconf system now supports a method to run these ~70
	tests natively on a target system. You either need a target machine
	with remote login features or you need an emulator with a method to
	copy files into the emulated system and to run binaries on the
	emulated system as e.g. the Android emulator.

	We currently deliver three scripts for "remote" execution of
	programs on the target system:

	runrmt_ssh		runs the commands remove via ssh
	runrmt_rsh		runs the commands remove via rsh
	runrmt_android		runs the commands remove via the debug bridge

	If you need to remotely run programs on a system that is not
	supported by one of there three scripts, you need to modify one
	of them to match your needs.

	To enable Cross Compilation use the following environment variables:

	CONFIG_RMTCALL=		Set up to point to a script that does
				the remote execution, e.g.:

				CONFIG_RMTCALL=`pwd`/conf/runrmt_ssh

	CONFIG_RMTHOST=		Set up to point to your remote host, e.g.:

				CONFIG_RMTHOST=hostname 
				or
				CONFIG_RMTHOST=user@hostname

				use a dummy if you like to use something
				like the Android emulator.

	CONFIG_RMTDEBUG=	Set to something non-null in order to 
				let the remote execution script mark
				remote comands. This will result in
				configure messages like:

				checking bits in minor device number... REMOTE 8

				If you cannot run commands on the target
				platform, you may set:

				CONFIG_RMTDEBUG=true
				CONFIG_RMTCALL=:

				carefully watch for the "REMOTE" flagged test
				output and later manually edit the file:

				incs/<arch-dir>/xconfig.h

				Do not forget to manually edit the files:

				incs/<arch-dir>/align.h
				and
				incs/<arch-dir>/avoffset.h

	Note that smake includes automake features that automatically
	retrieve system ID information. For this reason, you need to overwrite 
	related macros from the command line if you like to do a
	cross compilation.

	Related make macros:

	K_ARCH=			# (sun4v) Kernel ARCH filled from uname -m / arch -k
	M_ARCH=			# (sun4)  Machine filled from arch
	P_ARCH=			# (sparc) CPU ARCH filled from uname -p / mach
	OSNAME=			# sunos, linux, ....
	OSREL=			# 5.11
	OSVERSION=		# snv_130
	CCOM=			# generic compiler name (e.g. "gcc")
	CC=			# compiler to call (name for binary)
	CC_COM=			# compiler to call (name + basic args)

	ARCH=			overwrites M_ARCH and P_ARCH

	It is usually suffucient to set ARCH and OSNAME.

	In order to use a cross compiler environment instead of a native compiler,
	set the make macro CC_COM or CC to something different than "cc".

	If you are on Linux and like to compile for Android, do the following:

	1) 	set up CC acording to the instructions from the cross compiler
		tool chain. Important: you need to read the information for your
		tool chain. A working setup may look similar to:

		NDK=/home/joerg/android-ndk-r7
		SYSROOT=\$NDK/platforms/android-14/arch-arm
		CC="\$NDK/toolchains/arm-linux-androideabi-4.4.3/prebuilt/linux-x86/bin/arm-linux-androideabi-gcc --sysroot=\$SYSROOT"
		export NDK
		export SYSROOT
		export CC


	2)	set environment variables CONFIG_RMTCALL / CONFIG_RMTHOST, e.g.:
		setenv CONFIG_RMTCALL `pwd`/conf/runrmt_android
		setenv CONFIG_RMTHOST NONE

	3)	call smake:

		smake ARCH=armv5 OSNAME=linux CCOM=gcc "CC_COM=$CC"

		or

		smake ARCH=armv5 OSNAME=linux CCOM=gcc "CC=$CC"


Compiling with the address sanitizer:

	Be careful with a compiler enhancement called "addess sanitizer".

	First a note: the address sanitizer needs a lot of memory when in
	64-bit mode. For this reason, it is recommended to run the tests
	in 32-bit mode as it may be impossible to provdie a sufficient amount
	of memory for the 64-bit mode.

	1) The address sanitizer may cause autoconf to behave incorrectly in
	case that the compiler options used by the "configure" run include the
	address sanitizer. It seems that in addition, the address sanitizer
	adds more libraries to the link list and as a result prevents
	the correct autoconf decision on whether a specific library from
	a "configure" test is needed by some binaries.

	If you are not sure about the current state of the source tree, start
	with calling:

		./.clean

	in the top level source directory. This makes the source tree to behave
	as if if was recently unpacked from the tar archive.

	Then run run e.g.:

		cd inc/
		smake CCOM=gcc32
		cd ..

	to prepare the auto-configuration without using the address sanitizer.
	This special treatment is needed as the assumptions in the address
	sanitizer would not allow us to run the autoconfiguration code
	correctly.

	2) The address sanitizer by default ignores installed SIGSEGV handlers
	and thus ignores the intention of the author of the code.

	The correct behavior may be switched on via setting the environment
	variable:

		ASAN_OPTIONS=allow_user_segv_handler=true

	As a redult, the command line to compile the code after the
	auto-configuration has been done as mentioned above is:

	ASAN_OPTIONS=allow_user_segv_handler=true smake CCOM=gcc32 COPTX="-g -O0 -fsanitize=address" LDOPTX="-g -fsanitize=address" 

	3) If you are on Linux, do not forget to call "ulimit -c unlimited",
	before calling the binary. This is needed as the default on Linux is
	not to create a core file.

	4) Set the environment ASAN_OPTIONS= for the execution of the binary
	to control the behavior of the Address Sanitizer while the binary
	is run.

	If you like to disable the memory leak detection because your program
	is a short running program that intentionally does not free() resources
	before calling exit(), use:

		ASAN_OPTIONS=allow_user_segv_handler=true:detect_leaks=0

	If you also like to get a core dump on error to debug, you may like
	to use:

		ASAN_OPTIONS=allow_user_segv_handler=true:detect_leaks=0:abort_on_error=1

	Note that the Address Sanitizer disables the creation of a core file
	for 64 bit binaries as the tables used by the Address Sanitizer may
	cause the core file to have a size of 16 TB.


Compiling with the "Americal fuzzy lop":

	Follow the instruction from above for the address sanitizer, but
	use this command line to call the compiler:

	ASAN_OPTIONS=allow_user_segv_handler=true AFL_HARDEN=1 AFL_USE_ASAN=1 smake CC=afl-gcc CCOM=gcc32


Author:

Joerg Schilling
Seestr. 110
D-13353 Berlin
Germany

Email: 	joerg@schily.isdn.cs.tu-berlin.de, js@cs.tu-berlin.de
	joerg.schilling@fokus.fraunhofer.de

Please mail bugs and suggestions to me.
