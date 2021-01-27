$! build_cdrecord.com Version 13.11.2003 for cdrecord 2.01alpha19
$ define/nolog scg [-.libscg.scg]
$ opts = "/float=ieee/pref=all"
$ incs = "/incl=([],[-.include],[-.libedc],[-.libschily])"
$ copy [-.libedc]encoder_tables. [-.libedc]encoder.tables
$ copy [-.libedc]scramble_table. [-.libedc]scramble.table
$ copy [-.libedc]L2SQ_TABLE. [-.libedc]L2SQ.table
$ cc'opts' 'incs' /def=("VMS") CDRECORD.C
$ cc'opts' 'incs' /def=("VMS") sector.c
$ cc'opts' 'incs' /define=("VMS") CDR_DRV.C
$ cc'opts' 'incs' /define=("VMS") DRV_JVC.C
$ cc'opts' 'incs' /define=("VMS") DRV_MMC.C
$ cc'opts' 'incs' /define=("VMS") DRV_PHILIPS.C
$ cc'opts' 'incs' /define=("VMS") DRV_SONY.C
$ cc'opts' 'incs' /define=("VMS") FIFO.C
$ cc'opts' 'incs' /define=("VMS") ISOSIZE.C
$ cc'opts' 'incs' /define=("VMS") MODES.C
$! cc'opts' 'incs' /define=("VMS") SCSIERRS.C
$! cc'opts' 'incs' /define=("VMS") SCSITRANSP.C
$! cc'opts' 'incs' /define=("VMS") SCSIOPEN.C
$! cc'opts' 'incs' /define=("VMS") SCSIHACK.C
$! cc'opts' 'incs' /define=("VMS") SCSIHELP.C
$! cc'opts' 'incs' /define=("VMS") RDUMMY.C
$! cc'opts' 'incs' /define=("VMS") SCGSETTARGET.C
$! cc'opts' 'incs' /define=("VMS") SCGTIMES.C
$ cc'opts' 'incs' /define=("VMS") SCSI_CDR.C
$ cc'opts' 'incs' /define=("VMS") SCSI_SCAN.C
$ cc'opts' 'incs' /define=("VMS") WM_PACKET.C
$ cc'opts' 'incs' /define=("VMS") WM_SESSION.C
$ cc'opts' 'incs' /define=("VMS") WM_TRACK.C
$ cc'opts' 'incs' /define=("VMS") AUDIOSIZE.C
$ cc'opts' 'incs' /define=("VMS") DRV_SIMUL.C
$ cc'opts' 'incs' /define=("VMS") DISKID.C
$ cc'opts' 'incs' /define=("VMS") CD_MISC.C
$ cc'opts' 'incs' /define=("VMS") MISC.C
$ cc'opts' 'incs' /define=("VMS") DEFAULTS.C
$ cc'opts' 'incs' /define=("VMS") cdtext.c
$ cc'opts' 'incs' /define=("VMS") crc16.c
$ cc'opts' 'incs' /define=("VMS") SUBCHAN.C
$ cc'opts' 'incs' /define=("VMS") GETNUM.C
$ cc'opts' 'incs' /define=("VMS") MOVESECT.C
$ cc'opts' 'incs' /define=("VMS") AUINFO.C
$ cc'opts' 'incs' /define=("VMS") DRV_7501.C
$ cc'opts' 'incs' /define=("VMS") CUE.C
$ cc'opts' 'incs' /define=("VMS") XIO.C
$ cc'opts' 'incs' /define=("VMS") SCSI_MMC.C
$ libr/cre cdr.olb
$ set default [-.LIBEDC]
$ define/user encoder_tables encoder.tables
$ define/user scramble_table scramble.table
$ define/user l2sq_table L2SQ.TABLE
$ cc'opts' 'incs' /def=("VMS")/obj=[-.cdrecord] edc_ecc.c
$ set default [-.LIBDEFLT]
$ cc'opts' 'incs' /define=("VMS")/obj=[-.cdrecord] DEFAULT.C
$ set default [-.cdrecord]
$ libr/ins cdr.olb *.obj
$ link /nosysshr/exe=[-.bins]cdrecord sys$input/option
  cluster=myclu,,,cdrecord.obj,cdr.olb/lib,[-.libs]libfile.olb/lib, -
  libscg.olb/lib,stdio.olb/lib,libschily.olb/lib
  sys$library:decc$shr/share/selective
  sys$library:cma$tis_shr/share/selective
$ delete *.obj;*
$ purge/nolog *.olb
