/*
    Copyright (C) 2008-2023, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function execlp().
*/

#include <aros/debug.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>

#include "../crt/__exec.h"

/*****************************************************************************

    NAME */
#include <unistd.h>

        int __progonly_execlp(

/*  SYNOPSIS */
        const char *file,
        const char *arg, va_list args)
        
/*  FUNCTION
        Executes a file with given name. The search paths for the executed
        file are paths specified in the PATH environment variable.

    INPUTS
        file - Name of the file to execute.
        arg - First argument passed to the executed file.
        ... - Other arguments passed to the executed file.

    RESULT
        Returns -1 and sets errno appropriately in case of error, otherwise
        doesn't return.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
        execve(), execl(), execv(), execvp()
        
    INTERNALS

******************************************************************************/
{
    char *const *argv;
    char ***environptr = __posixc_get_environptr();
    char **environ = (environptr != NULL) ? *environptr : NULL;

    if(!(argv = __progonly_exec_valist2array(arg, args)))
    {
        errno = ENOMEM;
        return -1;
    }

    APTR id = __progonly_exec_prepare(file, 1, argv, environ);
    __progonly_exec_cleanup_array();
    if (!id)
        return -1;

    __progonly_exec_do(id);
    
    assert(0); /* Should not be reached */
    return -1;
} /* execlp() */

