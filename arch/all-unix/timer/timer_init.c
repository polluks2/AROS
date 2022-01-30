/*
    Copyright (C) 1995-2019, The AROS Development Team. All rights reserved.
*/

#include <aros/config.h>
#include <aros/bootloader.h>
#include <aros/debug.h>
#include <aros/symbolsets.h>
#include <exec/execbase.h>
#include <exec/interrupts.h>
#include <hardware/intbits.h>
#include <proto/arossupport.h>
#include <proto/bootloader.h>
#include <proto/exec.h>
#include <proto/hostlib.h>
#include <proto/kernel.h>

#if defined(__AROSEXEC_SMP__)
#include <proto/execlock.h>
#include <resources/execlock.h>
#endif

#include "timer_intern.h"
#include "timer_macros.h"

#define timeval sys_timeval

#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

#undef timeval

/* Android is not a true Linux ;-) */
#ifdef HOST_OS_android
#undef HOST_OS_linux
#endif

#ifdef HOST_OS_linux
#define LIBC_NAME "libc.so.6"
#endif

#ifdef HOST_OS_darwin
#define LIBC_NAME "libSystem.dylib"
#endif

#ifndef LIBC_NAME
#define LIBC_NAME "libc.so"
#endif

/*
 * In Linux stdlib.h #define's atoi() to something internal, causing link failure
 * because we link our kickstart binaries against AROS libraries, and not against
 * host OS ones.
 * Explicit prototype here avoids this.
 */
int atoi(const char *nptr);

/* Handle periodic timer and drive exec VBlank */
static void TimerTick(struct TimerBase *TimerBase, struct ExecBase *SysBase)
{
#if defined(__AROSEXEC_SMP__)
    struct ExecLockBase *ExecLockBase = TimerBase->tb_ExecLockBase;
#endif
    /* Increment EClock value and process microhz requests */
    ADDTIME(&TimerBase->tb_CurrentTime, &TimerBase->tb_Platform.tb_VBlankTime);
    ADDTIME(&TimerBase->tb_Elapsed, &TimerBase->tb_Platform.tb_VBlankTime);
    TimerBase->tb_ticks_total++;

    handleMicroHZ(TimerBase, SysBase);

    /* Now handle VBlank emulation via divisor */
    TimerBase->tb_Platform.tb_TimerCount++;
    if (TimerBase->tb_Platform.tb_TimerCount == TimerBase->tb_Platform.tb_VBlankTicks)
    {
        /* Protect the interrupt chain list when "causing" INTB_VERTB

           alsa.audio is continuously doing Add/Rem IntServer which
           was randomly triggering a crash here as the list was being
           modified while the interrupt code was running.
        */
#if defined(__AROSEXEC_SMP__)
        struct ExecLockBase *ExecLockBase = TimerBase->tb_ExecLockBase;
        if (ExecLockBase) ObtainSystemLock(&SysBase->IntrList, SPINLOCK_MODE_READ, LOCKF_DISABLE);
#endif
        /* vblank_Cuase */
        struct IntVector *iv = &SysBase->IntVects[INTB_VERTB];

        if (iv->iv_Code)
            AROS_INTC2(iv->iv_Code, iv->iv_Data, INTF_VERTB);
#if defined(__AROSEXEC_SMP__)
        if (ExecLockBase) ReleaseSystemLock(&SysBase->IntrList, LOCKF_DISABLE);
#endif

        handleVBlank(TimerBase, SysBase);

        TimerBase->tb_Platform.tb_TimerCount = 0;
    }
}
static VOID timerTask(struct TimerBase *TimerBase, struct ExecBase *sysBase)
{
    while (1)
    {
        usleep(TimerBase->tb_Platform.tb_VBlankTime.tv_micro);
        TimerTick(TimerBase, sysBase);
    }
}

static int Timer_Init(struct TimerBase *TimerBase)
{
    APTR BootLoaderBase;
    struct itimerval interval;
    int ret;

#if defined(__AROSEXEC_SMP__)
    struct ExecLockBase *ExecLockBase;
    if ((ExecLockBase = OpenResource("execlock.resource")) != NULL)
    {
        TimerBase->tb_ExecLockBase = ExecLockBase;
        TimerBase->tb_ListLock = AllocLock();
    }
#endif

    HostLibBase = OpenResource("hostlib.resource");
    if (!HostLibBase)
        return FALSE;

    TimerBase->tb_Platform.libcHandle = HostLib_Open(LIBC_NAME, NULL);
    if (!TimerBase->tb_Platform.libcHandle)
        return FALSE;

    TimerBase->tb_Platform.setitimer = HostLib_GetPointer(TimerBase->tb_Platform.libcHandle, "setitimer", NULL);
    if (!TimerBase->tb_Platform.setitimer)
        return FALSE;

    /* Our defaults: 50 Hz VBlank and 4x timer rate. 1x gives very poor results. */
    SysBase->VBlankFrequency = 50;
    TimerBase->tb_Platform.tb_VBlankTicks = 4;

    /*
     * Since we are software-driven, we can just ask the user which
     * frequencies he wishes to use.
     */
    BootLoaderBase = OpenResource("bootloader.resource");
    if (BootLoaderBase)
    {
        struct List *args = GetBootInfo(BL_Args);

        if (args)
        {
            struct Node *node;

            for (node = args->lh_Head; node->ln_Succ; node = node->ln_Succ)
            {
                if (strncasecmp(node->ln_Name, "vblank=", 7) == 0)
                    SysBase->VBlankFrequency = atoi(&node->ln_Name[7]);
                else if (strncasecmp(node->ln_Name, "tickrate=", 9) == 0)
                    TimerBase->tb_Platform.tb_VBlankTicks = atoi(&node->ln_Name[9]);
            }
        }
    }

    /* Calculate effective EClock timer frequency. Set it also in ExecBase public field. */
    TimerBase->tb_eclock_rate = SysBase->VBlankFrequency * TimerBase->tb_Platform.tb_VBlankTicks;
    SysBase->ex_EClockFrequency = TimerBase->tb_eclock_rate;
    D(bug("[Timer_Init] Timer frequency is %d\n", TimerBase->tb_eclock_rate));

    /* Calculate timer period in us */
    TimerBase->tb_Platform.tb_VBlankTime.tv_secs  = 0;
    TimerBase->tb_Platform.tb_VBlankTime.tv_micro = 1000000 / TimerBase->tb_eclock_rate;

    /* Start up our system timer. */
    interval.it_interval.tv_sec  = interval.it_value.tv_sec  = 0;
    interval.it_interval.tv_usec = interval.it_value.tv_usec = TimerBase->tb_Platform.tb_VBlankTime.tv_micro;

    HostLib_Lock();

    NewCreateTask(TASKTAG_PC, timerTask, TASKTAG_NAME, "Timer Heartbeat",
            TASKTAG_ARG1, TimerBase, TASKTAG_ARG2, SysBase, TAG_DONE);
    ret = 0;

    AROS_HOST_BARRIER

    HostLib_Unlock();

    D(bug("[Timer_Init] setitimer() returned %d\n", ret));
    return !ret;
}

static int Timer_Expunge(struct TimerBase *TimerBase)
{
    if (!HostLibBase)
        return TRUE;

    if (TimerBase->tb_Platform.libcHandle)
        HostLib_Close(TimerBase->tb_Platform.libcHandle, NULL);

    return TRUE;
}

ADD2INITLIB(Timer_Init, 0)
ADD2EXPUNGELIB(Timer_Expunge, 0)
