/*
    Copyright (C) 1995-2021, The AROS Development Team. All rights reserved.

    C99 function scanf().
*/


#include <stdarg.h>

/*****************************************************************************

    NAME */
#include <stdio.h>

        int scanf (

/*  SYNOPSIS */
        const char * format,
        ...)

/*  FUNCTION

    INPUTS

    RESULT
        The number of converted parameters

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        fscanf(), vscanf(), vfscanf(),
        stdc.library/sscanf(), stdc.library/vsscanf()

    INTERNALS

******************************************************************************/
{
    struct PosixCBase *PosixCBase = __aros_getbase_PosixCBase();
    int     retval;
    va_list args;

    va_start (args, format);

    retval = vfscanf (PosixCBase->_stdin, format, args);

    va_end (args);

    return retval;
} /* scanf */

