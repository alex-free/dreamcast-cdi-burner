/* @(#)at-base2.c	1.1 13/10/30 Copyright 2011-2013 J. Schilling */
/*
 *	Emulate the behavior of openat(fd, name, flag, mode)
 *
 *	Note that emulation methods that do not use the /proc filesystem are
 *	not MT safe. In the non-MT-safe case, we do:
 *
 *		savewd()/fchdir()/open(name)/restorewd()
 *
 *	Errors may force us to abort the program as our caller is not expected
 *	to know that we do more than a simple open() here and that the
 *	working directory may be changed by us.
 *
 *	Copyright (c) 2011-13 J. Schilling
 */
/*
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License, Version 1.0 only
 * (the "License").  You may not use this file except in compliance
 * with the License.
 *
 * See the file CDDL.Schily.txt in this distribution for details.
 * A copy of the CDDL is also available via the Internet at
 * http://www.opensource.org/licenses/cddl1.txt
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file CDDL.Schily.txt from this distribution.
 */

#ifndef	ENAMETOOLONG
#define	ENAMETOOLONG	EINVAL
#endif

EXPORT FUNC_RESULT
FUNC_NAME(fd1, name1, fd2, name2 KR_ARGS)
	int		fd1;
	const char	*name1;
	int		fd2;
	const char	*name2;
	KR_DECL
{
	int	n;
	BOOL	same_fd =  FALSE;
	FUNC_RESULT	ret = 0;
	char	buf1[PATH_MAX];
	char	buf2[PATH_MAX];
	char	*proc_name1;
	char	*proc_name2;
	struct save_wd save_wd;

	FLAG_CHECK();

/*
 * There are 17 cases to implement:
 *
 * case	fd1	name1	 fd2	name2
 * 1	AT_CWD	absolute AT_CWD	absolute   call xxx(name1, name2)
 * 2	AT_CWD	absolute AT_CWD	relative   call xxx(name1, name2)
 * 3	AT_CWD	relative AT_CWD	absolute   call xxx(name1, name2)
 * 4	AT_CWD	relative AT_CWD	relative   call xxx(name1, name2)
 * 5	AT_CWD	absolute fd	absolute   call xxx(name1, name2)
 * 6	AT_CWD	relative fd	absolute   call xxx(name1, name2)
 * 7	fd	absolute AT_CWD	absolute   call xxx(name1, name2)
 * 8	fd	absolute AT_CWD	relative   call xxx(name1, name2)
 * 9	fd	absolute fd	absolute   call xxx(name1, name2)
 *
 * 10	AT_CWD	absolute fd	relative   chdir to fd2; call xxx(name1, name2)
 * 11	AT_CWD	relative fd	relative   convert name1 to abs; case 10
 * 12	fd	relative AT_CWD	absolute   chdir to fd1; call xxx(name1, name2)
 * 13	fd	relative AT_CWD	relative   convert name2 to abs; case 12
 *
 * 14	fd	absolute fd	relative   chdir to fd2; call xxx(name1, name2)
 * 15	fd	relative fd	absolute   chdir to fd1; call xxx(name1, name2)
 *
 * 16	fd1	relative fd1	relative   chdir to fd1; call xxx(name1, name2)
 * 17	fd	relative fd	relative   chdir to fd1; case 11
 */

	/*
	 * Case 1..9
	 */
	if ((fd1 == AT_FDCWD || ABS_NAME(name1)) &&
	    (fd2 == AT_FDCWD || ABS_NAME(name2)))
		return (FUNC_CALL(name1, name2));

	/*
	 * If the procfs interface works, we can catch all other cases here:
	 */
	if ((proc_name1 = proc_fd2name(buf1, fd1, name1)) != NULL &&
	    (proc_name2 = proc_fd2name(buf2, fd2, name2)) != NULL) {
		ret = FUNC_CALL(proc_name1, proc_name2);
		if (ret >= 0 || NON_PROCFS_ERRNO(errno))
			return (ret);
	} else if (geterrno() == ENAMETOOLONG) {
		return (-1);
	}

	if (savewd(&save_wd) < 0) {
		/*
		 * We abort here as the caller may not know that we are forced
		 * to savewd/fchdir/restorewd here and misinterpret errno.
		 */
		savewd_abort(geterrno());
		/* NOTREACHED */
		return (-1);
	}
	if ((fd1 >= 0 && save_wd.fd == fd1) ||
	    (fd2 >= 0 && save_wd.fd == fd2)) {
		/*
		 * If we just opened "fd" with the same number in savewd(),
		 * fd1/fd2 must have been illegal when calling us();
		 */
		closewd(&save_wd);
		seterrno(EBADF);
		return (-1);
	}
	if (!ABS_NAME(name1) && !ABS_NAME(name2)) {
		/*
		 * Check for case 16
		 */
		if (fd1 == fd2) {
			same_fd = TRUE;
		}
#ifndef	_MSC_VER
		/*
		 * Are there other non-cooperative platforms where stat()
		 * does not allow to compare files?
		 */
		else {
			struct stat sb1;
			struct stat sb2;

			if (fstat(fd1, &sb1) >= 0 &&
			    fstat(fd2, &sb2) >= 0) {
				if (sb1.st_dev == sb2.st_dev &&
				    sb1.st_ino == sb2.st_ino)
					same_fd = TRUE;
			}
		}
#endif
	}
case_10_12:
	if (ABS_NAME(name1) || ABS_NAME(name2) || same_fd) {
		/*
		 * Handle case 10, 12, 14, 15, 16
		 */
		int	fd;

		if (ABS_NAME(name1))
			fd = fd2;
		else
			fd = fd1;
		if ((n = fchdir(fd)) < 0) {
			int	err = geterrno();
			/*
			 * In case that fchdir() is emulated via chdir()
			 * and we use a multi hop chdir(), we may be on
			 * an undefined intermediate directory. Try to
			 * return to last working directory and if this
			 * fails, abort for security.
			 */
			if (n == -2 && restorewd(&save_wd) < 0) {
				restorewd_abort(geterrno());
				/* NOTREACHED */
			}
			closewd(&save_wd);
			seterrno(err);
			return (-1);
		}
	} else {
		/*
		 * Handle case 11, 13, 17
		 */
		if (fd1 == AT_FDCWD) {
case_11:
			/*
			 * Existing file: follow all but last component, abort
			 * if the file does not exist.
			 */
			if (absfpath(name1, buf1, sizeof (buf1),
				    RSPF_EXIST | RSPF_NOFOLLOW_LAST) == NULL) {
				ret = -1;
				goto fail;
			}
			name1 = buf1;
			goto case_10_12;
		} else if (fd2 == AT_FDCWD) {
			/*
			 * New file: may not exist,
			 * use absnpath() -> absfpath(... 0)
			 */
			if (absnpath(name2, buf2, sizeof (buf2)) == NULL) {
				ret = -1;
				goto fail;
			}
			name2 = buf2;
			goto case_10_12;
		} else {
			if ((n = fchdir(fd1)) < 0) {
				int	err = geterrno();
				/*
				 * In case that fchdir() is emulated via chdir()
				 * and we use a multi hop chdir(), we may be on
				 * an undefined intermediate directory. Try to
				 * return to last working directory and if this
				 * fails, abort for security.
				 */
				if (n == -2 && restorewd(&save_wd) < 0) {
					restorewd_abort(geterrno());
					/* NOTREACHED */
				}
				closewd(&save_wd);
				seterrno(err);
				return (-1);
			}
			goto case_11;
		}
	}

	ret = FUNC_CALL(name1, name2);		/* The actual call() */

fail:
	if (restorewd(&save_wd) < 0) {
		int	err = geterrno();

		closewd(&save_wd);
		restorewd_abort(err);
		/* NOTREACHED */
		seterrno(err);
		return (-1);
	}
	closewd(&save_wd);

	return (ret);
}
