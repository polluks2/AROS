/*
    Copyright (C) 1995-2020, The AROS Development Team. All rights reserved.

    Desc: Add a task.
*/

#include <exec/execbase.h>
#include <exec/memory.h>
#include <utility/tagitem.h>
#include <aros/libcall.h>
#include <proto/exec.h>
#include <exec/rawfmt.h>

#include <string.h>

#include "etask.h"
#include "exec_util.h"
#include "exec_debug.h"
#include "taskstorage.h"

#if defined(__AROSEXEC_SMP__)
#define __KERNEL_NOLIBBASE__
#include <proto/kernel.h>
#endif

#define TASKTAG_PRELAUNCHHOOK   (TASKTAG_Dummy + 30)

#include <pthread.h>
#include <sched.h>
#include "exec_locks.h"
extern int pthread_setname_np (pthread_t __target_thread, const char *__name);
struct TaskParameters
{
    struct Task     *tp_Task;
    APTR            tp_InitialPC;
    IPTR            tp_Arguments[8];
    ULONG           tp_ArgumentCount;
};
static void __pthread_trampoline(void *ptr);
static void process_tags(struct TagItem *tagList, struct TaskParameters *params);


static void TaskLaunch(struct Task *parent, struct Task *task, struct Hook *plHook)
{
    struct IntETask *etask = GetIntETask(task);

    /* Add the new task to the ready list. */
    task->tc_State = TS_READY;
    EXEC_LOCK_LIST_WRITE(&SysBase->TaskReady);
    Enqueue(&SysBase->TaskReady, &task->tc_Node);

    /* Task is actually already running */
    Remove(&task->tc_Node);
    EXEC_UNLOCK_LIST(&SysBase->TaskReady);
    task->tc_State = TS_RUN;
    pthread_mutex_unlock(&etask->iet_StartupMutex);
    Enable();

    /*
        Determine if a task switch is necessary. (If the new task has a
        higher priority than the current one and the current one
        is still active.) If the current task isn't of type TS_RUN it
        is already gone.
    */
    if (
        parent && task->tc_Node.ln_Pri > parent->tc_Node.ln_Pri &&
        parent->tc_State == TS_RUN)
    {
        DADDTASK("TaskLaunch: Rescheduling...\n");
        if (plHook)
        {
            DADDTASK("TaskLaunch: Calling pre-launch hook\n");
            CALLHOOKPKT(plHook, task, 0);
        }

        sched_yield();
    }
    else if (plHook)
    {
        CALLHOOKPKT(plHook, task, 0);
    }
    Enable();
}

/*****************************************************************************

    NAME */

        AROS_LH4(APTR, NewAddTask,

/*  SYNOPSIS */
        AROS_LHA(struct Task *,     task,      A1),
        AROS_LHA(APTR,              initialPC, A2),
        AROS_LHA(APTR,              finalPC,   A3),
        AROS_LHA(struct TagItem *,  tagList,   A4),

/*  LOCATION */
        struct ExecBase *, SysBase, 176, Exec)

/*  FUNCTION
        Add a new task to the system. If the new task has the highest
        priority of all and task switches are allowed it will be started
        immediately.
        Certain task fields should be intitialized and a stack must be
        allocated before calling this function. tc_SPReg will be used as the
        starting location for the stack pointer, i.e. a part of the stack can
        be reserved to pass the task some initial arguments.
        Memory can be added to the tc_MemEntry list and will be freed when the
        task dies. The new task's registers are set to 0.

    INPUTS
        task      - Pointer to task structure.
        initialPC - Entry point for the new task.
        finalPC   - Routine that is called if the initialPC() function returns.
                    A NULL pointer installs the default finalizer.

    RESULT
        The address of the new task or NULL if the operation failed (can only
        happen with TF_ETASK set - currenty not implemented).

    NOTES
        This function is private. Use MorphOS-compatible NewCreateTaskA()
        in your applications.

    EXAMPLE

    BUGS

    SEE ALSO
        RemTask()

    INTERNALS

    HISTORY

******************************************************************************/
{
    AROS_LIBFUNC_INIT

    struct Task *parent;
    struct MemList *mlExtra = NULL;
    struct Hook *plHook = NULL;
    if (tagList)
        plHook = (struct Hook *)LibGetTagData(TASKTAG_PRELAUNCHHOOK, 0, tagList);

    ASSERT_VALID_PTR(task);

    parent = GET_THIS_TASK;

    /* Sigh - you should provide a name for your task. */
    if ((task->tc_Node.ln_Name == NULL) && (parent) && (parent->tc_Node.ln_Name))
    {
        IPTR fmtname[] =
        {
            (IPTR)parent->tc_Node.ln_Name
        };

        if ((mlExtra = AllocMem(sizeof(struct MemList), MEMF_PUBLIC|MEMF_CLEAR)) != NULL)
        {
            mlExtra->ml_NumEntries = 1;
            mlExtra->ml_ME[0].me_Length = strlen(parent->tc_Node.ln_Name) + 10;
            if ((mlExtra->ml_ME[0].me_Addr = AllocMem(mlExtra->ml_ME[0].me_Length, MEMF_PUBLIC|MEMF_CLEAR)) != NULL)
            {
                task->tc_Node.ln_Name = mlExtra->ml_ME[0].me_Addr;
                RawDoFmt("%s.subtask", (RAWARG)&fmtname, RAWFMTFUNC_STRING, task->tc_Node.ln_Name);
            }
            else
            {
                FreeMem(mlExtra, sizeof(struct MemList));
                mlExtra = NULL;
            }
        }
    }

    if (task->tc_Node.ln_Name == NULL)
            task->tc_Node.ln_Name = "bad task";

    DADDTASK("NewAddTask (0x%p (\"%s\"), 0x%p, 0x%p)", task, task->tc_Node.ln_Name, initialPC, finalPC);

    /* Initialize the memory entry list if the caller forgot */
    if (!task->tc_MemEntry.lh_Head)
        NEWLIST(&task->tc_MemEntry);

    if (mlExtra)
        AddTail(&task->tc_MemEntry, &mlExtra->ml_Node);

    DADDTASK("NewAddTask MemEntry head: 0x%p", GetHead(&task->tc_MemEntry.lh_Head));

    /* Set node type to NT_TASK if not set to something else. */
    if (!task->tc_Node.ln_Type)
        task->tc_Node.ln_Type = NT_TASK;

    /* This is moved into SysBase at the tasks's startup */
    task->tc_IDNestCnt = -1;
    task->tc_TDNestCnt = -1;

    task->tc_State = TS_ADDED;
    task->tc_Flags = 0;

    task->tc_SigWait = 0;
    task->tc_SigRecvd = 0;
    task->tc_SigExcept = 0;

    /* Signals default to all system signals allocated. */
    if (task->tc_SigAlloc == 0)
        task->tc_SigAlloc = SysBase->TaskSigAlloc;

    /* Currently only used for segmentation violation */
    if (task->tc_TrapCode == NULL)
        task->tc_TrapCode = SysBase->TaskTrapCode;

    if (task->tc_ExceptCode == NULL)
        task->tc_ExceptCode = SysBase->TaskExceptCode;

    /*
     * EXECF_StackSnoop can be set or reset at runtime.
     * However task's stack is either snooped or not, it's problematic
     * to turn it on at runtime. So we initialize it when the task starts up.
     */
    if (PrivExecBase(SysBase)->IntFlags & EXECF_StackSnoop)
        task->tc_Flags |= TF_STACKCHK;

    /* Initialize ETask */
    if (!InitETask(task, parent))
        return NULL;

    /* Get new stackpointer. */
    if (task->tc_SPReg == NULL)
        task->tc_SPReg = (UBYTE *)(task->tc_SPUpper) - SP_OFFSET;

#ifdef AROS_STACKALIGN
    if ((IPTR)task->tc_SPReg & (AROS_STACKALIGN - 1))
    {
        DADDTASK("NewAddTask with unaligned stack pointer (0x%p)! Fixing...", task->tc_SPReg);
        task->tc_SPReg = (APTR)((IPTR)task->tc_SPReg & ~(AROS_STACKALIGN - 1));
    }
#endif
    DADDTASK("NewAddTask: SPLower: 0x%p SPUpper: 0x%p SP: 0x%p", task->tc_SPLower, task->tc_SPUpper, task->tc_SPReg);

    if (task->tc_Flags & TF_STACKCHK)
    {
        UBYTE *startfill, *endfill;

        startfill = (UBYTE *)task->tc_SPLower;
        endfill   = ((UBYTE *)task->tc_SPReg) - 16;

        while (startfill <= endfill)
        {
            *startfill++ = 0xE1;
        }
    }

    /* Default finalizer? */
    if (finalPC == NULL)
        finalPC = SysBase->TaskExitCode;

    /* Init new context. */
    pthread_t taskThread;
    TEXT pthreadName[16] = {0};
    struct TaskParameters *params = AllocMem(sizeof(struct TaskParameters), MEMF_CLEAR);
    struct IntETask *etask = GetIntETask(task);

    strncpy(pthreadName, task->tc_Node.ln_Name, 15);

    params->tp_InitialPC    = initialPC;
    params->tp_Task         = task;
    process_tags(tagList, params);

    pthread_mutex_lock(&etask->iet_StartupMutex);
    pthread_create(&taskThread, NULL, (APTR)__pthread_trampoline, params);
    pthread_setname_np(taskThread, pthreadName);

    /* Set the task flags for switch and launch. */
    if(task->tc_Switch)
        task->tc_Flags|=TF_SWITCH;

    if(task->tc_Launch)
        task->tc_Flags|=TF_LAUNCH;

    /*
        Protect the task lists. This must be done with Disable() because
        of Signal() which is usable from interrupts and may change those
        lists.
     */

    DADDTASK("NewAddTask: launching task ...");

    TaskLaunch(parent, task, plHook);

    DADDTASK("NewAddTask: task 0x%p launched", task);

    return task;

    AROS_LIBFUNC_EXIT
} /* NewAddTask */

static void __pthread_trampoline(void *ptr)
{
    struct TaskParameters *params = (struct TaskParameters *)ptr;
    struct IntETask *etask = GetIntETask(params->tp_Task);

    /* Obtaining semaphore means that task setup in parent is complete */
    pthread_mutex_lock(&etask->iet_StartupMutex);
    SET_THIS_TASK(params->tp_Task);
    switch (params->tp_ArgumentCount)
    {
    case (0):
        ((void(*)(VOID))params->tp_InitialPC)();
        break;
    case (1):
        ((void(*)(IPTR))params->tp_InitialPC)(params->tp_Arguments[0]);
        break;
    case (2):
        ((void(*)(IPTR, IPTR))params->tp_InitialPC)(params->tp_Arguments[0], params->tp_Arguments[1]);
        break;
    default:
        asm("int3");
        break;
    }

    FreeMem(params, sizeof(struct TaskParameters));
}

static void process_tags(struct TagItem *tagList, struct TaskParameters *params)
{
    struct TagItem *tag, *tstate = tagList;
    params->tp_ArgumentCount = 0;

    while ((tag = LibNextTagItem(&tstate)))
    {
    switch (tag->ti_Tag)
    {
    case TASKTAG_ARG1:
        params->tp_Arguments[0]     = tag->ti_Data;
        if (params->tp_ArgumentCount < 1) params->tp_ArgumentCount = 1;
        break;
    case TASKTAG_ARG2:
        params->tp_Arguments[1]     = tag->ti_Data;
        if (params->tp_ArgumentCount < 2) params->tp_ArgumentCount = 2;
        break;

    }
    }
}
