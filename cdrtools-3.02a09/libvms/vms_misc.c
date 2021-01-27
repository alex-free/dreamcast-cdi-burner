/*
 * Miscellaneous VMS-specific functions.
 */

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "schily/mconfig.h"
#include "vms_init.h"
#include "vms_misc.h"

/*--------------------------------------------------------------------*/

/*
 * 2005-03-14 SMS.
 * openfd_vms().
 *
 * VMS-specific _openfd() function.
 *
 * When open() (as in _openfd()) is used to open a file, and then
 * fdopen() is used to get a FILE pointer (as in _fcons()) with "b",
 * then the original open() must specify "ctx=bin".  Otherwise, fdopen()
 * fails with "%SYSTEM-?-BADPARAM, bad parameter value".
 *
 * We use a phony O_BINARY flag bit (defined in xmconfig.h) to retain
 * this datum, but (because it is phony) do not pass it through to
 * open() here.
 *
 * The original requirement for this feature was READCD, so it makes
 * sense to use the callback function to set the usual big-binary-file
 * RMS parameters.
 *
 * If we had clues to the caller's intent, we could, where appropriate,
 * add other performance helpers like "fop=sqo", but as-is we can't tell
 * where it would be appropriate.
 */

int
openfd_vms(const char *name, int omode)
{
	int sts;
	static int open_id = 2;

	if (omode& O_BINARY) {
		omode &= (~O_BINARY);
		sts = open(name,	/* File name. */
		omode,			/* Flags. */
		0777,			/* Mode for default protection. */
		"ctx=bin",		/* Binary. */
		"rfm=fix",		/* Fixed-length, */
		"mrs=512",		/* 512-byte records. */
		"acc", acc_cb,		/* Access callback function. */
		&open_id);		/* Access callback argument. */
	} else {
		sts = open(name, omode);
	}
	return (sts);
}

/*--------------------------------------------------------------------*/

/*
 * Judge availability of str[n]casecmp() in C RTL.
 * (Note: This must follow a "#include <decc$types.h>" in something to
 * ensure that __CRTL_VER is as defined as it will ever be.  DEC C on
 * VAX may not define it itself.)
 */

/*
 * 2004-09-25 SMS.
 * str[n]casecmp() replacement for old C RTL.
 * Assumes a prehistorically incompetent toupper().
 */
#ifndef HAVE_STRCASECMP

int
strncasecmp(char *s1, char *s2, size_t n)
{
	/* Initialization prepares for n == 0. */
	char c1 = '\0';
	char c2 = '\0';

	while (n-- > 0) {
		/* Set c1 and c2.  Convert l-case characters to u-case. */
		if (islower(c1 = *s1))
			c1 = toupper(c1);

		if (islower(c2 = *s2))
			c2 = toupper(c2);

		/* Quit at inequality or NUL. */
		if ((c1 != c2) || (c1 == '\0'))
			break;

		s1++;
		s2++;
	}
	return ((unsigned int) c1- (unsigned int) c2);
}

#ifndef UINT_MAX
#define	UINT_MAX	4294967295U
#endif

#define	strcasecmp(s1, s2)	strncasecmp(s1, s2, UINT_MAX)

#endif /* ndef HAVE_STRCASECMP */

/*--------------------------------------------------------------------*/

/*
 * Character property table for (re-)escaping ODS5 extended file names.
 * Note that this table ignore Unicode, and does not identify invalid
 * characters.
 *
 * ODS2 valid characters: 0-9 A-Z a-z $ - _
 *
 * ODS5 Invalid characters:
 *    C0 control codes (0x00 to 0x1F inclusive)
 *    Asterisk (*)
 *    Question mark (?)
 *
 * ODS5 Invalid characters only in VMS V7.2 (which no one runs, right?):
 *    Double quotation marks (")
 *    Backslash (\)
 *    Colon (:)
 *    Left angle bracket (<)
 *    Right angle bracket (>)
 *    Slash (/)
 *    Vertical bar (|)
 *
 * Characters escaped by "^":
 *    SP  !  #  %  &  '  (  )  +  ,  .  ;  =  @  [  ]  ^  `  {  }  ~
 *
 * Either "^_" or "^ " is accepted as a space.  Period (.) is a special
 * case.  Note that un-escaped < and > can also confuse a directory
 * spec.
 *
 * Characters put out as ^xx:
 *    7F (DEL)
 *    80-9F (C1 control characters)
 *    A0 (nonbreaking space)
 *    FF (Latin small letter y diaeresis)
 *
 * Other cases:
 *    Unicode: "^Uxxxx", where "xxxx" is four hex digits.
 *
 *  Property table values:
 *    Normal escape:    1
 *    Space:            2
 *    Dot:              4
 *    Hex-hex escape:   8
 *    -------------------
 *    Hex digit:       64
 */

unsigned char char_prop[ 256] = {

/*	NUL SOH STX ETX EOT ENQ ACK BEL   BS  HT  LF  VT  FF  CR  SO  SI */
	0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,

/*	DLE DC1 DC2 DC3 DC4 NAK SYN ETB  CAN  EM SUB ESC  FS  GS  RS  US */
	0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,

/*	SP  !   "   #   $   %   &   '    (   )   *   +   ,   -   .   /  */
	2,  1,  0,  1,  0,  1,  1,  1,   1,  1,  0,  1,  1,  0,  4,  0,

/*	0   1   2   3   4   5   6   7    8   9   :   ;   <   =   >   ?  */
	64, 64, 64, 64, 64, 64, 64, 64,  64, 64,  0,  1,  1,  1,  1,  1,

/*	@   A   B   C   D   E   F   G    H   I   J   K   L   M   N   O  */
	1, 64, 64, 64, 64, 64, 64,  0,   0,  0,  0,  0,  0,  0,  0,  0,

/*	P   Q   R   S   T   U   V   W    X   Y   Z   [   \   ]   ^   _  */
	0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  1,  0,  1,  1,  0,

/*	`   a   b   c   d   e   f   g    h   i   j   k   l   m   n   o  */
	1, 64, 64, 64, 64, 64, 64,  0,   0,  0,  0,  0,  0,  0,  0,  0,

/*	p   q   r   s   t   u   v   w    x   y   z   {   |   }   ~  DEL */
	0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  1,  0,  1,  1,  8,

	8,  8,  8,  8,  8,  8,  8,  8,   8,  8,  8,  8,  8,  8,  8,  8,
	8,  8,  8,  8,  8,  8,  8,  8,   8,  8,  8,  8,  8,  8,  8,  8,
	8,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  8
};

/*--------------------------------------------------------------------*/

/*
 * 2004-09-27 SMS.
 * eat_carets().
 *
 * Delete ODS5 extended file name escape characters ("^") in the
 * original buffer.
 * Note that the current scheme does not handle all EFN cases, but it
 * could be made more complicated.
 */

void
eat_carets(char *str)
/* char *str;	Source pointer. */
{
	char *strd;	/* Destination pointer. */
	char hdgt;
	unsigned char uchr;
	unsigned char prop;

	/* Skip ahead to the first "^", if any. */
	while ((*str != '\0') && (*str != '^'))
		str++;

	/* If no caret was found, quit early. */
	if (*str != '\0') {
		/* Shift characters leftward as carets are found. */
		strd = str;
		while (*str != '\0') {
			uchr = *str;
			if (uchr == '^') {
				/* Found a caret. */
				/*  Skip it, and check the next character. */
				uchr = *(++str);
				prop = char_prop[ uchr];
				if (prop& 64) {
					/* Hex digit.  Get char code from this and next hex digit. */
					if (uchr <= '9') {
						hdgt = uchr- '0';		/* '0' - '9' -> 0 - 9. */
					} else {
						hdgt = ((uchr- 'A')& 7)+ 10;	/* [Aa] - [Ff] -> 10 - 15. */
					}
					hdgt <<= 4;				/* X16. */
					uchr = *(++str);			/* Next char must be hex digit. */
					if (uchr <= '9') {
						uchr = hdgt+ uchr- '0';
					} else {
						uchr = hdgt+ ((uchr- 'A')& 15)+ 10;
					}
				} else if (uchr == '_') {
					/* Convert escaped "_" to " ". */
					uchr = ' ';
				} else if (uchr == '/') {
					/* Convert escaped "/" (invalid UNIX) to "?" (invalid VMS). */
					uchr = '?';
				}
				/*
				 * Else, not a hex digit.  Must be a simple escaped character
				 * (or Unicode, which is not yet handled here).
				 */
			}
			/* Else, not a caret.  Use as-is. */
			*strd = uchr;

			/* Advance destination and source pointers. */
			strd++;
			str++;
		}
		/* Terminate the destination string. */
		*strd = '\0';
	}
}

/*--------------------------------------------------------------------*/

/*
 * 2005-02-04 SMS.
 * find_dir().
 *
 * Find directry boundaries in an ODS2 or ODS5 file spec.
 * Returns length (zero if no directory, negative if error),
 * and sets "start" argument to first character (typically "[") location.
 *
 * No one will care about the details, but the return values are:
 *
 *     0  No dir.
 *    -2  [, no end.              -3  <, no end.
 *    -4  [, multiple start.      -5  <, multiple start.
 *    -8  ], no start.            -9  >, no start.
 *   -16  ], wrong end.          -17  >, wrong end.
 *   -32  ], multiple end.       -33  >, multiple end.
 *
 * Note that the current scheme handles only simple EFN cases, but it
 * could be made more complicated.
 */
int
find_dir(char *file_spec, char **start)
{
	char *cp;
	char chr;

	char *end_tmp = NULL;
	char *start_tmp = NULL;
	int lenth = 0;

	for (cp = file_spec; cp < file_spec+ strlen(file_spec); cp++) {
		chr = *cp;
		if (chr == '^') {
			/* Skip ODS5 extended name escaped characters. */
			cp++;
			/* If escaped char is a hex digit, skip the second hex digit, too. */
			if (char_prop[ (unsigned char) *cp]& 64)
				cp++;
			} else if (chr == '[') {
				/* Found start. */
				if (start_tmp == NULL) {
					/* First time.  Record start location. */
					start_tmp = cp;
					/* Error if no end. */
					lenth = -2;
				} else {
					/* Multiple start characters.  */
					lenth = -4;
					break;
				}
			} else if (chr == '<') {
				/* Found start. */
				if (start_tmp == NULL) {
					/* First time.  Record start location. */
					start_tmp = cp;
					/* Error if no end. */
					lenth = -3;
				} else {
					/* Multiple start characters.  */
					lenth = -5;
					break;
				}
			} else if (chr == ']') {
				/* Found end. */
				if (end_tmp == NULL) {
					/* First time. */
					if (lenth == 0) {
						/* End without start. */
						lenth = -8;
						break;
					} else if (lenth != -2) {
						/* Wrong kind of end. */
						lenth = -16;
						break;
					}
					/* End ok.  Record end location. */
					end_tmp = cp;
					lenth = end_tmp+ 1- start_tmp;
					/* Could break here, ignoring excessive end characters. */
				} else {
					/* Multiple end characters. */
					lenth = -32;
					break;
				}
			} else if (chr == '>') {
				/* Found end. */
				if (end_tmp == NULL) {
					/* First time. */
					if (lenth == 0) {
						/* End without start. */
						lenth = -9;
						break;
					} else if (lenth != -3) {
						/* Wrong kind of end. */
						lenth = -17;
						break;
					}
					/* End ok.  Record end location. */
					end_tmp = cp;
					lenth = end_tmp+ 1- start_tmp;
					/* Could break here, ignoring excessive end characters. */
				} else {
					/* Multiple end characters. */
					lenth = -33;
					break;
				}
			}
		}

		/* If both start and end were found, then set result pointer where safe. */
		if (lenth > 0) {
			if (start != NULL) {
				*start = start_tmp;
			}
		}
	return (lenth);
}

/*--------------------------------------------------------------------*/
