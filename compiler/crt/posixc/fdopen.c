/*
    Copyright (C) 1995-2013, The AROS Development Team. All rights reserved.

    POSIX.1-2008 function fdopen().
*/

#include "__posixc_intbase.h"

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include <exec/lists.h>
#include <proto/exec.h>

#include "__stdio.h"
#include "__fdesc.h"

/*****************************************************************************

    NAME */
#include <stdio.h>

        FILE *fdopen (

/*  SYNOPSIS */
        int         filedes,
        const char *mode
        )

/*  FUNCTION
        function associates a stream with an existing file descriptor.

    INPUTS
        filedes - The descriptor the stream has to be associated with
        mode    - The mode of the stream  (same as with fopen()) must be com�patible
                  with the mode of the file  descriptor.   The  file
                  position  indicator  of  the  new  stream  is  set to that
                  belonging to filedes, and the error and end-of-file indicators
                  are cleared.  Modes "w" or "w+" do not cause truncation of the
                  file.  The file descriptor is not duplicated,  and
                  will  be  closed  when  the  stream  created  by fdopen is
                  closed.

    RESULT
        NULL on error or the new stream associated with the descriptor.

        The new descriptor returned by the call is the lowest numbered
        descriptor currently not in use by the process.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO
         open(), fclose(), fileno()

    INTERNALS

******************************************************************************/
{
    struct PosixCIntBase *PosixCBase =
        (struct PosixCIntBase *)__aros_getbase_PosixCBase();
    int oflags, wanted_accmode, current_accmode;
    fdesc *fdesc;
    FILENODE *fn;

    if (!(fdesc = __getfdesc(filedes)))
    {
        errno = EBADF;
        return NULL;
    }

    oflags = fdesc->fcb->flags;

    if (mode)
    {
        oflags          = __smode2oflags(mode);
        
        wanted_accmode  = oflags & O_ACCMODE;
        current_accmode = fdesc->fcb->flags & O_ACCMODE;
        
        /*
           Check if the requested access mode flags are a valid subset of the
           flags the already open file has. Thus, if the file's access mode
           is O_RDWR the requested mode can be anything (O_RDONLY, O_WRONLY or
           O_RDWR), else they must match exactly.
        */
        
        if ((current_accmode != O_RDWR) && (wanted_accmode != current_accmode))
        {
            errno = EINVAL;
            return NULL;
        }
    }

    fn = AllocPooled(PosixCBase->internalpool, sizeof(FILENODE));
    if (!fn) return NULL;

    AddTail ((struct List *)&PosixCBase->stdio_files, (struct Node *)fn);

    fn->File.flags = __oflags2sflags(oflags);
    fn->File.fd    = filedes;

    return FILENODE2FILE(fn);
}
