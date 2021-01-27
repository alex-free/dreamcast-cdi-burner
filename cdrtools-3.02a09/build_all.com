$ set default [.LIBFILE]
$ write sys$output "Creating LIBFILE.OLB"
$ @LIBFILE.COM
$ set default [-.LIBHFS_ISO]
$ write sys$output "Creating LIBHFS_ISO.OLB"
$ @LIBHFS_ISO.COM
$ set default [-.LIBUNLS]
$ write sys$output "Creating LIBUNLS.OLB"
$ @LIBUNLS.COM
$ set default [-.LIBSCG]
$ write sys$output "Creating LIBSCG.OLB"
$ @libscg.com
$ set default [-.libschily]
$ write sys$output "Creating LIBSCHILY.OLB"
$ @libschily.com
$ set default [-.MKISOFS]
$ write sys$output "BUILDING MKISOFS.EXE"
$ @BUILD_MKISOFS.COM
$ set default [-.cdrecord]
$ write sys$output "BUILDING CDRECORD.EXE"
$ @build_cdrecord.com
$ set default [-.readcd]
$ write sys$output "BUILDING READCD.EXE"
$ @build_readcd.com
$ set default [-.CDDA2WAV]
$ write sys$output "BUILDING CDDA2WAV.EXE"
$ @BUILD_CDDA2WAV.com
$ set default [-]
$ dir [.bins]*.exe 
