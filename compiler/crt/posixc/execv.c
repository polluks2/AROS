/*
    Copyright (C) 2008-2023, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function execv().
*/

#include <aros/debug.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>

#include "../crt/__exec.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

        int __progonly_execv(

/*  SYNOPSIS */
        const char *path,
        char *const argv[])
        
/*  FUNCTION
        Executes a file located in given path with specified arguments.

    INPUTS
        path - Pathname of the file to execute.
        argv - Array of arguments given to main() function of the executed
        file.

    RESULT
        Returns -1 and sets errno appropriately in case of error, otherwise
        doesn't return.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        execve(), execl(), execlp(), execvp()
        
    INTERNALS

******************************************************************************/
{
    char ***environptr = __posixc_get_environptr();
    char **environ = (environptr != NULL) ? *environptr : NULL;
    APTR id = __progonly_exec_prepare(path, 0, argv, environ);
    if (!id)
        return -1;
    
    __progonly_exec_do(id);
    
    assert(0); /* Should not be reached */
    return -1;
} /* execv() */
