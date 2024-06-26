/*
    Copyright (C) 1995-2021, The AROS Development Team. All rights reserved.

    C99 function rename() with optional Amiga<>Posix file name conversion.
*/

#include <aros/debug.h>

#include <proto/dos.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "__upath.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

        int rename (

/*  SYNOPSIS */
        const char * oldpath,
        const char * newpath)

/*  FUNCTION
        Renames a file or directory.

    INPUTS
        oldpath - Complete path to existing file or directory.
        newpath - Complete path to the new file or directory.

    RESULT
        0 on success and -1 on error. In case of an error, errno is set.
        
    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

******************************************************************************/
{
          STRPTR aoldpath = (STRPTR)strdup((const char*)__path_u2a(oldpath));
    CONST_STRPTR anewpath = __path_u2a(newpath);
    int ret;

    /* __path_u2a has resolved paths like /toto/../a */
    if (anewpath[0] == '.')
    {
        if (anewpath[1] == '\0' || (anewpath[1] == '.' && anewpath[2] == '\0'))
        {
            errno = EEXIST;
            free(aoldpath);
            return -1;
        }
    }

    ret = __stdc_rename(aoldpath, anewpath);

    free(aoldpath);

    return ret;
} /* rename */

