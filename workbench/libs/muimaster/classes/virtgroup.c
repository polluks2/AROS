/*
    Copyright � 2002-2003, The AROS Development Team. All rights reserved.
    $Id$
*/

#define MUIMASTER_YES_INLINE_STDARG

#include <exec/memory.h>
#include <intuition/icclass.h>
#include <intuition/gadgetclass.h>
#include <intuition/imageclass.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/graphics.h>
#include <proto/muimaster.h>

#include "mui.h"
#include "muimaster_intern.h"
#include "support.h"
#include "support_classes.h"

extern struct Library *MUIMasterBase;

struct Virtgroup_DATA
{
   int dummy;
};

/**************************************************************************
 OM_NEW
**************************************************************************/
static ULONG Virtgroup_New(struct IClass *cl, Object *obj, struct opSet *msg)
{
    //struct Virtgroup_DATA *data;
    //int i;

    return DoSuperNewTags(cl, obj, NULL, MUIA_Group_Virtual, TRUE, TAG_MORE, msg->ops_AttrList);
}

#if ZUNE_BUILTIN_VIRTGROUP
BOOPSI_DISPATCHER(IPTR, Virtgroup_Dispatcher, cl, obj, msg)
{
    switch (msg->MethodID)
    {
	case OM_NEW: return Virtgroup_New(cl, obj, (struct opSet *)msg);
    }
    return DoSuperMethodA(cl, obj, msg);
}

const struct __MUIBuiltinClass _MUI_Virtgroup_desc =
{ 
    MUIC_Virtgroup, 
    MUIC_Group, 
    sizeof(struct Virtgroup_DATA), 
    (void*)Virtgroup_Dispatcher 
};
#endif /* ZUNE_BUILTIN_VIRTGROUP */
