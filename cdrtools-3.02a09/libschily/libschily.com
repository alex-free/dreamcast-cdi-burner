$ define/nolog scg [-.libscg.scg]
$ set default [-.inc]
$ cc/float=ieee/pref=all/obj/incl=([],[-.include])/define=("VMS") ALIGN_TEST.C
$ link ALIGN_TEST
$ delete ALIGN_TEST.obj;*
$ def/user sys$output [-.libschily]ALIGN.H
$ r ALIGN_TEST
$ cc/float=ieee/pref=all/obj/incl=([],[-.include])/define=("VMS") AVOFFSET.C
$ link AVOFFSET
$ delete AVOFFSET.obj;*
$ def/user sys$output [-.libschily]AVOFFSET.H
$ r AVOFFSET
$ set default [-.libschily]
$ opts = "/float=ieee/pref=all/obj"
$ incs = "/incl=([],[-.include],[-.inc])"
$ cc 'opts' 'incs' /define=("VMS") ASTOI.C
$ cc 'opts' 'incs' /define=("VMS") ASTOLL.C
$ cc 'opts' 'incs' /define=("VMS") COMERR.C
$ cc 'opts' 'incs' /define=("VMS") ERROR.C
$ cc 'opts' 'incs' /define=("VMS") FCONV.C
$ cc 'opts' 'incs' /define=("VMS") FILLBYTES.C
$ cc 'opts' 'incs' /define=("VMS") FORMAT.C
$ cc 'opts' 'incs' /define=("VMS") GETARGS.C
$ cc 'opts' 'incs' /define=("VMS") GETAV0.C
$ cc 'opts' 'incs' /define=("VMS") GETERRNO.C
$ cc 'opts' 'incs' /define=("VMS") GETFP.C
$ cc 'opts' 'incs' /define=("VMS") MOVEBYTES.C
$ cc 'opts' 'incs' /define=("VMS") PRINTF.C
$ cc 'opts' 'incs' /define=("VMS") RAISECOND.C
$ cc 'opts' 'incs' /define=("VMS") SAVEARGS.C
$ cc 'opts' 'incs' /define=("VMS") jsprintf.C
$ cc 'opts' 'incs' /define=("VMS") jssnprintf.C
$ cc 'opts' 'incs' /define=("VMS") STREQL.C
$ cc 'opts' 'incs' /define=("VMS") SWABBYTES.C
$ cc 'opts' 'incs' /define=("VMS") SERRMSG.C
$ cc 'opts' 'incs' /define=("VMS") CMPBYTES.C
$ cc 'opts' 'incs' /define=("VMS") SETERRNO.C
$ libr/crea [-.libs]libschily.olb
$ libr/ins [-.libs]libschily.olb *.obj
$ delete *.obj;*
$ purge/nolog [-.libs]*.olb
$ set default [.stdio]
$ incs1 = "/INCL=([],[--.INCLUDE])"
$ cc'opts' 'incs1'/DEFINE=("VMS") CVMOD.C
$ cc'opts' 'incs1'/DEFINE=("VMS") DAT.C
$ cc'opts' 'incs1'/DEFINE=("VMS") FCONS.C
$ cc'opts' 'incs1'/DEFINE=("VMS") FGETLINE.C
$ cc'opts' 'incs1'/DEFINE=("VMS") FILEOPEN.C
$ cc'opts' 'incs1'/DEFINE=("VMS") FILEREAD.C
$ cc'opts' 'incs1'/DEFINE=("VMS") FILEWRITE.C
$ cc'opts' 'incs1'/DEFINE=("VMS") FLAG.C
$ cc'opts' 'incs1'/DEFINE=("VMS") FLUSH.C
$ cc'opts' 'incs1'/DEFINE=("VMS") NIREAD.C
$ cc'opts' 'incs1'/DEFINE=("VMS") FILELUOPEN.C
$ cc'opts' 'incs1'/DEFINE=("VMS") FILESIZE.C
$ libr/cre [--.libs]stdio.olb
$ libr/ins [--.libs]stdio.olb *.obj
$ delete *.obj;*
$ purge/nolog [--.libs]*.olb
$ set default [-]
