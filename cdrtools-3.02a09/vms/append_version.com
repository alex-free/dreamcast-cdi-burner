$!                                              6 March 2005.  SMS.
$!
$! Cdrtools VMS accessory procedure.
$!
$!    Extract a quoted string from a line in file P1 containing the
$!    string P2.  Append that value in a "#define VERSION" directive to
$!    the header file, P3.
$!
$!
$! Generate a temporary file name.
$!
$ temp_file_open = 0
$ temp_file_name = -
   f$parse( f$environment( "PROCEDURE"), , , "NAME", "SYNTAX_ONLY")+ -
   "_"+ f$getjpi( "", "PID")+ ".TMP"
$!
$ on error then goto clean_up
$ on control_y then goto clean_up
$!
$! Search P1 source file for P2 string.
$!
$ define /user_mode sys$output 'temp_file_name'
$!
$ search 'p1' "''p2'"
$!
$ status = $status
$!
$ if ((f$integer( status) .and. 7) .eq. 1)
$ then
$!
$! P2 string found.  Extract quoted string from that line.
$!
$    open /error = clean_up /read temp_file 'temp_file_name'
$    temp_file_open = 1
$!
$    read /error = clean_up temp_file line
$    close temp_file
$    temp_file_open = 0
$!
$    q1 = f$locate( """", line)+ 1
$    q2 = f$locate( """", f$extract( q1, 100, line))
$    quotation == f$extract( q1, q2, line)
$    delete /nolog 'temp_file_name';*
$!
$! Write the '#define VERSION "xxx"' directive into a stream_lf temp file.
$!
$    create /fdl = sys$input 'temp_file_name'
RECORD
        Carriage_Control carriage_return
        Format stream_lf
$!
$    open /error = clean_up /append temp_file 'temp_file_name'
$    temp_file_open = 1
$    write /error = clean_up temp_file "#define VERSION ""''quotation'"""
$    close temp_file
$    temp_file_open = 0
$!
$! Append the "#define" directive temp file onto the P3 destination file.
$!
$    append 'temp_file_name' 'p3'
$!
$ endif
$!
$ clean_up:
$!
$ if (temp_file_open)
$ then
$    close temp_file
$ endif
$!
$ if (f$search( temp_file_name) .nes. "")
$ then
$    delete /nolog 'temp_file_name';*
$ endif
$!
