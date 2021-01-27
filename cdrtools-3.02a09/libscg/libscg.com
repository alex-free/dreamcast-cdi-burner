$ defs = "/float=ieee/pref=all"
$ incs = "/incl=([],[-.include])"
$ cc 'defs' 'incs' /define=("VMS") SCSIERRS.C
$ cc 'defs' 'incs' /define=("VMS")  SCSITRANSP.C
$ cc 'defs' 'incs' /define=("VMS")  SCSIOPEN.C
$ cc 'defs' 'incs' /define=("VMS")  SCSIHACK.C
$ cc 'defs' 'incs' /define=("VMS")  SCSIHELP.C
$ cc 'defs' 'incs' /define=("VMS")  SCGSETTARGET.C
$ cc 'defs' 'incs' /define=("VMS")  SCGTIMES.C
$ cc 'defs' 'incs' /define=("VMS")  rdummy.c
$ library/crea [-.libs]libscg.olb
$ library/insert [-.libs]libscg.olb *.obj
$ delete *.obj;*
$ purge/nolog [-.libs]*.olb
