/*
    Copyright © 2009-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc: arosc library - support code for entering and leaving a program
    Lang: english
*/
#include "__arosc_privdata.h"
#include "__exitfunc.h"

#define DEBUG 0
#include <aros/debug.h>

void __arosc_program_startup(void)
{
    D(bug("[__arosc_program_startup] aroscbase 0x%p\n", __get_aroscbase()));

    /* Function is just a placeholder for the future */
}

void __arosc_program_end(void)
{
    D(bug("[__arosc_program_end]\n"));

    __callexitfuncs();
}
