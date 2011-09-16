/*
    Copyright � 1995-2011, The AROS Development Team. All rights reserved.
    $Id$

    Desc:
    Lang: English
*/

#include <aros/debug.h>
#include <utility/tagitem.h>
#include <dos/dostags.h>
#include <proto/utility.h>
#include <dos/dosextens.h>
#include <aros/asmcall.h>
#include <exec/ports.h>

#include "dos_newcliproc.h"
#include "dos_intern.h"
#include "fs_driver.h"

/*****************************************************************************

    NAME */

#include <proto/dos.h>

	AROS_LH2(LONG, SystemTagList,

/*  SYNOPSIS */
	AROS_LHA(CONST_STRPTR    , command, D1),
	AROS_LHA(struct TagItem *, tags,    D2),

/*  LOCATION */
	struct DosLibrary *, DOSBase, 101, Dos)

/*  FUNCTION

    Execute a command via a shell. As defaults, the process will use the
    current Input() and Output(), and the current directory as well as the
    path will be inherited from your process. If no path is specified, this
    path will be used to find the command.
        Normally, the boot shell is used but other shells may be specified
    via tags. The tags are passed through to CreateNewProc() except those
    that conflict with SystemTagList(). Currently, these are

        NP_Seglist
	NP_FreeSeglist
	NP_Entry
	NP_Input
	NP_Error
	NP_Output
	NP_CloseInput
	NP_CloseOutput
	NP_CloseError
	NP_HomeDir
	NP_Cli
        NP_Arguments
	NP_Synchrounous
	NP_UserData


    INPUTS

    command  --  program and arguments as a string
    tags     --  see <dos/dostags.h>. Note that both SystemTagList() tags and
                 tags for CreateNewProc() may be passed.

    RESULT

    The return code of the command executed or -1 if the command could
    not run because the shell couldn't be created. If the command is not
    found, the shell will return an error code, usually RETURN_ERROR.

    NOTES

    You must close the input and output filehandles yourself (if needed)
    after System() returns if they were specified via SYS_Input or
    SYS_Output (also, see below).
        You may NOT use the same filehandle for both SYS_Input and SYS_Output.
    If you want them to be the same CON: window, set SYS_Input to a filehandle
    on the CON: window and set SYS_Output to NULL. Then the shell will
    automatically set the output by opening CONSOLE: on that handler.
        If you specified SYS_Asynch, both the input and the output filehandles
    will be closed when the command is finished (even if this was your Input()
    and Output().

    EXAMPLE

    BUGS

    SEE ALSO

    Execute(), CreateNewProc(), Input(), Output(), <dos/dostags.h>

    INTERNALS

*****************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Process *me = (struct Process *)FindTask(NULL);
    BPTR   cis = Input(), cos = Output(), ces = me->pr_CES, script = BNULL;
    BPTR   shellseg = BNULL;
    STRPTR cShell      = "C:Shell";
    STRPTR shellName   = "Boot Shell";
    BOOL script_opened = FALSE;
    BOOL cis_opened    = FALSE;
    BOOL cos_opened    = FALSE;
    BOOL ces_opened    = FALSE;
    BOOL isBoot        = TRUE;
    BOOL isCustom      = FALSE;
    BOOL isBackground  = TRUE;
    BOOL isAsynch      = FALSE;
    BOOL needUnload    = FALSE;
    STRPTR cmdcopy     = NULL;
    LONG commandlen;
    LONG rc            = -1;
    LONG *cliNumPtr    = NULL;
    struct RootNode *rn = DOSBase->dl_Root;
    struct MsgPort *ct = NULL;

    const struct TagItem *tags2 = tags;
    struct TagItem *newtags, *tag;

    D(bug("SystemTagList('%s',%p)\n", command, tags));
    while ((tag = NextTagItem(&tags2)))
    {
    	D(bug("Tag=%08x Data=%08x\n", tag->ti_Tag, tag->ti_Data));
        switch (tag->ti_Tag)
	{
	    case SYS_ScriptInput:
	        script = (BPTR)tag->ti_Data;
		break;

	    case SYS_Input:
	        cis = (BPTR)tag->ti_Data;
		break;

	    case SYS_Output:
	        cos = (BPTR)tag->ti_Data;
		break;

	    case SYS_Error:
	        ces = (BPTR)tag->ti_Data;
		break;

	    case SYS_CustomShell:
	        cShell = (STRPTR)tag->ti_Data;
	        isCustom = TRUE;
		break;

            case SYS_UserShell:
	        isBoot = !tag->ti_Data;
	        break;

	    case SYS_Background:
                isBackground = tag->ti_Data ? TRUE : FALSE;
		break;

	    case SYS_Asynch:
		isAsynch = tag->ti_Data ? TRUE : FALSE;
		break;

	    case SYS_CliNumPtr:
	        cliNumPtr = (LONG *)tag->ti_Data;
		break;
	}
    }

    /* Set up the streams */
    if (!cis)
    {
        cis = Open("NIL:", MODE_OLDFILE);
	if (!cis) goto end;

	cis_opened = TRUE;
    }
    else
    if (cis == (BPTR)SYS_DupStream)
    {
        cis = OpenFromLock(DupLockFromFH(Input()));
	if (!cis) goto end;

	cis_opened = TRUE;
    }
    if (IsInteractive(cis))
    	ct = ((struct FileHandle*)BADDR(cis))->fh_Type;
    D(bug("CIS: %p\n", cis));

    if (!cos)
    {
        if (ct != NULL)
        {
            APTR old_ct = GetConsoleTask();
            SetConsoleTask(ct);
            cos = Open("*", MODE_NEWFILE);
            SetConsoleTask(old_ct);
        } else {
            cos = Open("NIL:", MODE_NEWFILE);
        }

        if (!cos) goto end;

	cos_opened = TRUE;
    }
    else
    if (cos == (BPTR)SYS_DupStream)
    {
        cos = OpenFromLock(DupLockFromFH(Output()));
	if (!cos) goto end;

	cos_opened = TRUE;
    }
    D(bug("COS: %p\n", cos));

    if (!ces)
    {
	ces = OpenFromLock(DupLockFromFH(cos));
        if (!ces) goto end;

	ces_opened = TRUE;
    }
    else
    if (ces == (BPTR)SYS_DupStream)
    {
        ces = OpenFromLock(DupLockFromFH(Output()));
	if (!ces) goto end;

	ces_opened = TRUE;
    }
    D(bug("CES: %p\n", ces));


    /* Load the shell */
    if (isCustom)
    	shellName = cShell;
    else {
    	/* Seglist of default shell is stored in RootNode when loaded */
    	shellseg = rn->rn_ShellSegment;
    	/*
    	 * Set shell process name.
    	 * On AROS we have no actual difference between user and boot shell.
    	 */
    	if (!isBoot)
    	    shellName = isBackground ? "Background CLI" : "New Shell";
    }

    /* First, try loading from disk. */
    if (shellseg == BNULL) {
    	shellseg = LoadSeg(cShell);
    	if (isCustom)
    	    needUnload = TRUE;
    }

    /* Next, look for a resident */
    if (shellseg == BNULL)
    {
    	struct Segment *seg;
    	STRPTR segName = FilePart(cShell);

        D(bug("Could not load C:Shell\n"));
        Forbid();
        seg = FindSegment(segName, NULL, TRUE);
        if (seg != NULL && seg->seg_UC <= 0)
            shellseg = seg->seg_Seg;
        Permit();
        needUnload = FALSE;
    }

    /* Otherwise, we're dead. No shell. */
    if (shellseg == BNULL)
    {
        D(bug("Could not load shell\n"));
        goto end;
    }

    /* Inject the arguments, adding a trailing '\n'
     * if the user did not.
     */
    if (command == NULL || command[0] == 0)
        command = "\n";

    commandlen = strlen(command);
    if (commandlen) {
        STRPTR cmdcopy = NULL;

        if (command[commandlen-1] != '\n') {
            cmdcopy = AllocVec(commandlen + 2, MEMF_ANY);
            if (cmdcopy == NULL)
                goto end;

            CopyMem(command, cmdcopy, commandlen);
            cmdcopy[commandlen++] = '\n';
            cmdcopy[commandlen] = 0;
            command = cmdcopy;
        }
    }

    if (!isCustom)
    	rn->rn_ShellSegment = shellseg;

    newtags = CloneTagItems(tags);
    if (newtags)
    {
	struct CliStartupMessage csm;
	struct Process *cliproc;

	struct TagItem proctags[] =
	{
	    { NP_Entry      , (IPTR) NewCliProc             }, /* 0  */
	    { NP_Priority   , me->pr_Task.tc_Node.ln_Pri    }, /* 1  */
	    { NP_Name       , (IPTR)shellName               }, /* 2  */
	    { NP_Input      , (IPTR)cis                     },
	    { NP_Output     , (IPTR)cos                     },
	    { NP_CloseInput , (isAsynch || cis_opened)      },
	    { NP_CloseOutput, (isAsynch || cos_opened)      },
	    { NP_Cli        , (IPTR)TRUE                    },
	    { NP_WindowPtr  , isAsynch ? (IPTR)NULL :
	                      (IPTR)me->pr_WindowPtr        },
	    { NP_Seglist    , (IPTR)shellseg                },
	    { NP_FreeSeglist, needUnload                    },
	    { NP_Arguments  , (IPTR)command                 },
	    { NP_Synchronous, FALSE                         },
	    { NP_Error      , (IPTR)ces                     },
	    { NP_CloseError , (isAsynch || ces_opened) &&
            /* 
                Since old AmigaOS programs don't know anything about pr_CES
                being handled by this function, don't close the Error stream
                if it's the same as the caller's one.
            */
			      ces != me->pr_CES             }, /* 14 */
	    { NP_ConsoleTask, (IPTR)ct                      }, /* 15 */
	    { TAG_END       , 0                             }  /* 16 */
	};

	Tag filterList[] =
	{
	    NP_Seglist,
	    NP_FreeSeglist,
	    NP_Entry,
	    NP_Input,
	    NP_Output,
	    NP_CloseInput,
            NP_CloseOutput,
	    NP_CloseError,
	    NP_HomeDir,
	    NP_Cli,
	    0
	};

	FilterTagItems(newtags, filterList, TAGFILTER_NOT);

	proctags[sizeof(proctags)/(sizeof(proctags[0])) - 1].ti_Tag  = TAG_MORE;
	proctags[sizeof(proctags)/(sizeof(proctags[0])) - 1].ti_Data = (IPTR)newtags;

	cliproc = CreateNewProc(proctags);

	if (cliproc)
	{
	    csm.csm_Msg.mn_Node.ln_Type = NT_MESSAGE;
	    csm.csm_Msg.mn_Length       = sizeof(csm);
	    csm.csm_Msg.mn_ReplyPort    = &me->pr_MsgPort;

	    csm.csm_CurrentInput = script;
            csm.csm_Background   = isBackground;
	    csm.csm_Asynch       = isAsynch;

	    PutMsg(&cliproc->pr_MsgPort, (struct Message *)&csm);
	    WaitPort(&me->pr_MsgPort);
	    GetMsg(&me->pr_MsgPort);

  	    if (cliNumPtr) *cliNumPtr = csm.csm_CliNumber;

	    rc = csm.csm_ReturnCode;

	    script_opened =
	    cis_opened    =
	    cos_opened    =
	    ces_opened    = FALSE;
	    
	    /* The process was started, do not unload the shell */
	    needUnload = FALSE;
	}
	FreeTagItems(newtags);
    }

end:
    if (cmdcopy)       FreeVec(cmdcopy);
    if (needUnload)    UnLoadSeg(shellseg);
    if (script_opened) Close(script);
    if (cis_opened)    Close(cis);
    if (cos_opened)    Close(cos);
    if (ces_opened)    Close(ces);

    return rc;

    AROS_LIBFUNC_EXIT
} /* SystemTagList */
