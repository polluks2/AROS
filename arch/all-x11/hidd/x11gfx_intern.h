#ifndef X11GFX_INTERN_H
#define X11GFX_INTERN_H
/*
    (C) 1997 AROS - The Amiga Research OS
    $Id$

    Desc: X11 gfx HIDD for AROS.
    Lang: English.
*/

#ifndef EXEC_TYPES_H
#   include <exec/types.h>
#endif
#ifndef EXEC_LIBRARIES_H
#   include <exec/libraries.h>
#endif
#ifndef DOS_BPTR_H
#   include <dos/bptr.h>
#endif
#ifndef OOP_OOP_H
#   include <oop/oop.h>
#endif

#include <X11/Xlib.h>
struct bitmap_data
{
    Window 	xwindow;
    Display   * sysdisplay;
    int		sysscreen;
    Cursor	syscursor;
    long 	maxpen;
    unsigned long sysplanemask;
    Colormap	colmap;
    int		depth;
};



/* Library stuff */
struct x11gfxbase
{
    struct Library 	library;
    struct ExecBase	*sysbase;
    BPTR		seglist;

    struct Library	*utilitybase;
    struct Library	*oopbase;
    
    Class		*gfxclass;
    Class		*gcclass;
    Class		*bitmapclass;
	
};


Class *init_gfxclass   ( struct x11gfxbase *x11gfxbase );
Class *init_gcclass    ( struct x11gfxbase *X11GfxBase );
Class *init_bitmapclass( struct x11gfxbase *X11GfxBase );

VOID free_gcclass	( struct x11gfxbase *X11GfxBase );
VOID free_bitmapclass	( struct x11gfxbase *X11GfxBase );
VOID free_gfxclass	( struct x11gfxbase *X11GfxBase );


#define expunge() \
AROS_LC0(BPTR, expunge, struct x11gfxbase *, LIBBASE, 3, X11Gfx)

#endif /* X11GFX_INTERN_H */
