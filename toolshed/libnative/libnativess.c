/********************************************************************
 * ss.c - Native SetStatus routines
 *
 * $Id$
 * Changed several conditionals for msys compatibility
 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <utime.h>
#include <unistd.h>
#include <sys/stat.h>

#include "cocotypes.h"
#include "cococonv.h"
#include "cocopath.h"



error_code _native_ss_attr(native_path_id path, int perms)
{
	error_code ec = 0;
	struct stat statbuf;


	ec = _native_gs_fd(path, &statbuf);

	if (ec == 0)
	{
		statbuf.st_mode = perms;

		ec = _native_ss_fd(path, &statbuf);
	}


	return ec;
}



error_code _native_ss_fd(native_path_id path, struct stat *statbuf)
{
	error_code ec = 0;
	struct utimbuf tbuff;


	/* Use mtime for both access-time and modification-time.
	 * POSIX does not allow applications to set st_ctime directly (it is
	 * managed by the kernel), so we cannot meaningfully restore a "creation
	 * time" here.  Setting actime to st_mtime keeps access-time in sync with
	 * the modification-time that was read from the source file descriptor. */
	tbuff.actime  = statbuf->st_mtime;
	tbuff.modtime = statbuf->st_mtime;


	/* 1. Update times. */

	utime(path->pathlist, &tbuff);


	/* 2. Update permissions. */

	chmod(path->pathlist, statbuf->st_mode);


	return ec;
}



error_code _native_ss_size(native_path_id path, int size)
{
	error_code ec = 0;

#if defined(__APPLE__) || defined(WIN32) || defined(sun) || defined(__CYGWIN__)
	ftruncate(path->fd->_file, size);
#else
	ftruncate(path->fd->_fileno, size);
#endif
	return ec;
}
