/*
    (C) 1997 AROS - The Amiga Replacement OS
    $Id$

    Desc:
    Lang: english
*/
#include <aros/libcall.h>
#include <proto/graphics.h>

#define DEBUG 0
#include <aros/debug.h>
#undef kprintf

/*****************************************************************************

    NAME */
#include <proto/layers.h>
#include "layers_intern.h"

	AROS_LH9(struct Layer *, CreateUpfrontHookLayer,

/*  SYNOPSIS */
	AROS_LHA(struct Layer_Info *, li, A0),
	AROS_LHA(struct BitMap     *, bm, A1),
	AROS_LHA(LONG               , x0, D0),
	AROS_LHA(LONG               , y0, D1),
	AROS_LHA(LONG               , x1, D2),
	AROS_LHA(LONG               , y1, D3),
	AROS_LHA(LONG               , flags, D4),
	AROS_LHA(struct Hook       *, hook, A3),
	AROS_LHA(struct BitMap     *, bm2, A2),

/*  LOCATION */
	struct LayersBase *, LayersBase, 31, Layers)

/*  FUNCTION

    INPUTS

    RESULT

    NOTES

    EXAMPLE

    BUGS

    SEE ALSO

    INTERNALS

    HISTORY
	27-11-96    digulla automatically created from
			    layers_lib.fd and clib/layers_protos.h

*****************************************************************************/
{
    AROS_LIBFUNC_INIT
    AROS_LIBBASE_EXT_DECL(struct LayersBase *,LayersBase)

    struct Layer *l;

    D(bug("CreateUpFrontHookLayer(li@$lx, bm@$lx, x0 %ld, y0 %ld, x1 %ld, y1 %ld, flags %ld, hook@$%lx, bm2@$lx)\n",
	li, bm, x0, y0, x1, y1, flags, hook, bm2));

    /* First create a regular layer in the back... */
    if(!(l = CreateBehindHookLayer(li, bm, x0, y0, x1, y1, flags, hook, bm2)))
	return NULL;

    /* ...and then bring it up to the front. */
    if(!UpfrontLayer(NULL, l))
    {
	DeleteLayer(NULL, l);
	return NULL;
    }

    /* Since we're Upfront now, we don't have Damage anymore... */
    ClearRegion(l->DamageList);

    /* ...and we don't have to refresh anymore either. */
    l->Flags &= ~(LAYERREFRESH|LAYERIREFRESH|LAYERIREFRESH2);

    return l;

    AROS_LIBFUNC_EXIT
} /* CreateUpfrontHookLayer */

