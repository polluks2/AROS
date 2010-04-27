/*
    Copyright � 1995-2004, The AROS Development Team. All rights reserved.
    $Id$
*/

/*********************************************************************************************/

#include "locale_intern.h"

#include <exec/execbase.h>
#include <aros/libcall.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/locale.h>

/*********************************************************************************************/

#define LIB_EXEC    1
#define LIB_UTILITY 2
#define LIB_DOS     3
#define LIB_LOCALE  4

/*********************************************************************************************/

#ifdef __MORPHOS__

extern void LIB_LocRawDoFmt(void);
extern void LIB_LocStrnicmp(void);
extern void LIB_LocStricmp(void);
extern void LIB_LocToLower(void);
extern void LIB_LocToUpper(void);
extern void LIB_LocDateToStr(void);
extern void LIB_LocStrToDate(void);
extern void LIB_LocDosGetLocalizedString(void);

#else

AROS_LD4(APTR, LocRawDoFmt,
	 AROS_LDA(CONST_STRPTR, FormatString, A0),
	 AROS_LDA(APTR        , DataStream, A1),
	 AROS_LDA(VOID_FUNC   , PutChProc, A2),
	 AROS_LDA(APTR        , PutChData, A3),
	 struct ExecBase *, SysBase, 31, Locale);
AROS_LD4(APTR, LocVNewRawDoFmt,
	 AROS_LDA(CONST_STRPTR, FormatString, A0),
	 AROS_LDA(VOID_FUNC   , PutChProc, A2),
	 AROS_LDA(APTR        , PutChData, A3),
	 AROS_LDA(va_list     , DataStream, A1),
	 struct ExecBase *, SysBase, 137, Locale);
AROS_LD3(LONG, LocStrnicmp,
	 AROS_LDA(CONST_STRPTR, string1, A0),
	 AROS_LDA(CONST_STRPTR, string2, A1),
	 AROS_LDA(LONG        , length , D0),
	 struct UtilityBase *, UtilityBase, 32, Locale);
AROS_LD2(LONG, LocStricmp,
	 AROS_LDA(CONST_STRPTR, string1, A0),
	 AROS_LDA(CONST_STRPTR, string2, A1),
	 struct UtilityBase *, UtilityBase, 33, Locale);
AROS_LD1(ULONG, LocToLower,
	 AROS_LDA(ULONG, character, D0),
	 struct UtilityBase *, UtilityBase, 34, Locale);
AROS_LD1(ULONG, LocToUpper,
	 AROS_LDA(ULONG, character, D0),
	 struct UtilityBase *, UtilityBase, 35, Locale);
AROS_LD1(LONG, LocDateToStr,
	 AROS_LDA(struct DateTime *, datetime, D1),
	 struct DosLibrary *, DOSBase, 36, Locale);
AROS_LD1(LONG, LocStrToDate,
	 AROS_LDA(struct DateTime *, datetime, D1),
	 struct DosLibrary *, DOSBase, 37, Locale);
AROS_LD1(CONST_STRPTR, LocDosGetLocalizedString,
	 AROS_LDA(LONG, stringNum, D1),
	 struct DosLibrary *, DOSBase, 38, Locale);

#endif

/*********************************************************************************************/

static struct patchinfo
{
    WORD    library;
    WORD    whichfunc;
    APTR    whichpatchfunc;
} pi [] = {

#ifdef __MORPHOS__
    {LIB_EXEC   , 87 , LIB_LocRawDoFmt},
    {LIB_UTILITY, 28 , LIB_LocStrnicmp},
    {LIB_UTILITY, 27 , LIB_LocStricmp},
    {LIB_UTILITY, 30 , LIB_LocToLower},
    {LIB_UTILITY, 29 , LIB_LocToUpper},
    {LIB_DOS    , 124, LIB_LocDateToStr},
    {LIB_DOS    , 125, LIB_LocStrToDate},
    {LIB_DOS    , 163, LIB_LocDosGetLocalizedString},
#else
    {LIB_EXEC   , 87 , AROS_SLIB_ENTRY(LocRawDoFmt		, Locale)},
/* Please test on various architectures before enabling
    {LIB_EXEC   , 137, AROS_SLIB_ENTRY(LocVNewRawDoFmt		, Locale)}, */
    {LIB_UTILITY, 28 , AROS_SLIB_ENTRY(LocStrnicmp		, Locale)},
    {LIB_UTILITY, 27 , AROS_SLIB_ENTRY(LocStricmp		, Locale)},
    {LIB_UTILITY, 30 , AROS_SLIB_ENTRY(LocToLower		, Locale)},
    {LIB_UTILITY, 29 , AROS_SLIB_ENTRY(LocToUpper		, Locale)},
    {LIB_DOS    , 124, AROS_SLIB_ENTRY(LocDateToStr		, Locale)},
    {LIB_DOS    , 125, AROS_SLIB_ENTRY(LocStrToDate		, Locale)},
    {LIB_DOS    , 154, AROS_SLIB_ENTRY(LocDosGetLocalizedString	, Locale)},
 #endif
    {0}
};

/*********************************************************************************************/

static struct Library *GetLib(WORD which)
{
    struct Library *lib = NULL;

    switch(which)
    {
    	case LIB_EXEC:
	    lib = (struct Library *)SysBase;
	    break;

	case LIB_DOS:
	    lib = (struct Library *)DOSBase;
	    break;

	case LIB_UTILITY:
	    lib = (struct Library *)UtilityBase;
	    break;

	case LIB_LOCALE:
	    lib = (struct Library *)LocaleBase;
	    break;
    }

    return lib;
}

/*********************************************************************************************/

void InstallPatches(void)
{
    WORD i;

#ifdef __MORPHOS__
    static const struct TagItem PatchTags[] =
    {
	{SETFUNCTAG_MACHINE, MACHINE_PPC},
	{SETFUNCTAG_TYPE, SETFUNCTYPE_NORMAL},
	{SETFUNCTAG_IDNAME, (ULONG) "locale.library Language Patch"},
	{SETFUNCTAG_DELETE, TRUE},
	{TAG_DONE}
    };
#endif

    Forbid();
    for(i = 0; pi[i].library; i++)
    {
#ifdef __MORPHOS__
        NewSetFunction(GetLib(pi[i].library), pi[i].whichpatchfunc, -pi[i].whichfunc * LIB_VECTSIZE, PatchTags);
#else
        SetFunction(GetLib(pi[i].library),
	    	    -pi[i].whichfunc * LIB_VECTSIZE, pi[i].whichpatchfunc);
#endif
    }
    Permit();
    LocaleBase->lb_SysPatches = TRUE;

}

/*********************************************************************************************/
