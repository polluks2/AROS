/*
    (C) 1995 AROS - The Amiga Replacement OS
    $Id$    $Log

    Desc: Graphics function SetWriteMask()
    Lang: english
*/
#include "graphics_intern.h"

/*****************************************************************************

    NAME */
#include <graphics/rastport.h>
#include <clib/graphics_protos.h>

	AROS_LH2(ULONG, SetWriteMask,

/*  SYNOPSIS */
	AROS_LHA(struct RastPort *, rp,   A0),
	AROS_LHA(ULONG            , mask, D0),

/*  LOCATION */
	struct GfxBase *, GfxBase, 164, Graphics)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	29-10-95    digulla automatically created from
			    graphics_lib.fd and clib/graphics_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct GfxBase *,GfxBase)
    ULONG ret;

    ret = driver_SetWriteMask (rp, mask, GfxBase);

    if (ret)
	rp->Mask = mask;

    return ret;
    AROS_LIBFUNC_EXIT
} /* SetWriteMask */
