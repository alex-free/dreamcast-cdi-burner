#ifndef __VMS_MISC_H
#define	__VMS_MISC_H

/*
 * 2005-03-06 SMS.
 *
 * Header file for miscellaneous VMS-specific functions.
 *
 */

/* str[n]casecmp() for old C RTL. */

#include <decc$types.h>
#ifdef __CRTL_VER
#	if __CRTL_VER >= 70000000
#		define HAVE_STRCASECMP
#	endif /* __CRTL_VER >= 70000000 */
#endif /* def __CRTL_VER */

#ifdef HAVE_STRCASECMP
#	include <strings.h>    /* str[n]casecmp() */
#endif /* def HAVE_STRCASECMP */


/* Function prototypes. */

extern void eat_carets(char *str);
extern int find_dir(char *file_spec, char **start);

#ifndef HAVE_STRCASECMP

extern int strncasecmp(char *s1, char *s2, size_t n);

#endif /* ndef HAVE_STRCASECMP */

#endif /* ndef __VMS_INIT_H */
