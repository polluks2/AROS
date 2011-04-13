/*
    Copyright � 2011, The AROS Development Team. All rights reserved.
    $Id: querypartitionattrs.c 38180 2011-04-12 12:32:14Z sonic $

*/
#include "partition_support.h"
#include "platform.h"

/*****************************************************************************

    NAME */
#include <libraries/partition.h>

    AROS_LH1(BPTR, LoadFileSystem,

/*  SYNOPSIS */
    AROS_LHA(struct Node *, handle, A1),

/*  LOCATION */
    struct Library *, PartitionBase, 20, Partition)

/*  FUNCTION
    Load the specified filesystem as DOS segment list.

    INPUTS
    handle - Filesystem handle obtained by FindFileSystemA()

    RESULT
    DOS seglist or NULL in case of failure.

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    const struct PTFunctionTable *handler = ((struct FileSysHandle *)handle)->part->table->handler;

    if (handler->loadFileSystem)
        return handler->loadFileSystem((struct PartitionBase_intern *)PartitionBase, (struct FileSysHandle *)handle);

    return BNULL;

    AROS_LIBFUNC_EXIT
}
