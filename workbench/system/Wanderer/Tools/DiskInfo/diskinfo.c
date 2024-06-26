/*
    Copyright (C) 2005-2023, The AROS Development Team. All rights reserved.
*/

#define MUIMASTER_YES_INLINE_STDARG

#define DEBUG 0
#include <aros/debug.h>

#include <exec/types.h>
#include <utility/tagitem.h>
#include <libraries/mui.h>
#include <libraries/asl.h>
#include <dos/filehandler.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/icon.h>
#include <proto/muimaster.h>
#include <proto/workbench.h>

#include <stdio.h>
#include <string.h>

#include <zune/iconimage.h>
#include "diskinfo.h"
#include "locale.h"
#include "support.h"

/* TODO: Move Filesystem ID/Name mappings to external (editable) file */
#ifndef ID_FAT12_DISK
#define ID_FAT12_DISK      (0x46415400L)
#define ID_FAT16_DISK      (0x46415401L)
#define ID_FAT32_DISK      (0x46415402L)
#endif
#ifndef ID_CDFS_DISK
#define ID_CDFS_DISK       (0x43444653L)
#endif
#ifndef ID_NTFS_DISK
#define ID_NTFS_DISK       (0x4E544653L)
#endif

static ULONG dt[] =
{
    ID_NO_DISK_PRESENT,
    ID_UNREADABLE_DISK,
    ID_NOT_REALLY_DOS,
    ID_KICKSTART_DISK,
    
    ID_DOS_DISK,
    ID_FFS_DISK,
    ID_INTER_DOS_DISK,
    ID_INTER_FFS_DISK,
    ID_FASTDIR_DOS_DISK,
    ID_FASTDIR_FFS_DISK,
    
    ID_DOS_muFS_DISK,
    ID_FFS_muFS_DISK,
    ID_INTER_DOS_muFS_DISK,
    ID_INTER_FFS_muFS_DISK,
    ID_FASTDIR_DOS_muFS_DISK,
    ID_FASTDIR_FFS_muFS_DISK,

    ID_FLOPPY_PFS_DISK,
    ID_PFS_DISK,
    ID_PFS2_DISK,
    ID_PFS3_DISK,
    ID_PFS2_muFS_DISK,

    ID_SFS_BE_DISK,
    ID_SFS_LE_DISK,

    ID_MSDOS_DISK,
    ID_FAT12_DISK,
    ID_FAT16_DISK,
    ID_FAT32_DISK,
    ID_NTFS_DISK,

    ID_EXT2_DISK,

    ID_CDFS_DISK,
    ID_HSIERRA_DISK,
    ID_ISO9660_DISK,
    ID_ISO9660RR_DISK,
    ID_ISO9660JOL_DISK
};

static CONST_STRPTR disktypelist[] =
{
    "No Disk",
    "Unreadable",
    "Not DOS",
    "KickStart",

    "OFS",
    "FFS",
    "OFS-Intl",
    "FFS-Intl",
    "OFS-DC",
    "FFS-DC",

    "muFS OFS",
    "muFS FFS",
    "muFS OFS-Intl",
    "muFS FFS-Intl",
    "muFS OFS-DC",
    "muFS FFS-DC",

    "PFS Floppy",
    "PFS1",
    "PFS2",
    "PFS3",
    "muFS PFS2",

    "SFS BE",
    "SFS LE",
   
    "MSDOS",
    "FAT12",
    "FAT16",
    "FAT32",
    "NTFS",

    "Ext2 FS",

    "CD-ROM",
    "High Sierra CDFS",
    "ISO9660 CDFS",
    "ISO9660 + RockRidge CDFS",
    "ISO9660 + Joliet CDFS"
};

/*** Instance data **********************************************************/
struct DiskInfo_DATA
{
    Object                     *dki_Window;
    Object                     *dki_VolumeIcon;
    Object                     *dki_VolumeName;
    Object                     *dki_VolumeUseGauge;
    Object                     *dki_VolumeUsed;
    Object                     *dki_VolumeFree;
    STRPTR                      dki_DOSDev;
    STRPTR                      dki_DOSDevInfo;
    STRPTR                      dki_FileSys;
    struct MsgPort             *dki_NotifyPort;
    LONG                        dki_DiskType;
    LONG                        dki_Aspect;
    struct MUI_InputHandlerNode dki_NotifyIHN;
    struct NotifyRequest        dki_FSNotifyRequest;
};

void GetFSNameFromID(ULONG fsid, STRPTR *fsname_ptr)
{
    int i;
    for (i = 0; i < sizeof(dt) / sizeof(ULONG); ++i)
    {
        if (fsid == dt[i])
        {
            int fsnamlen = strlen(disktypelist[i]);
            if (*fsname_ptr)
                FreeVec(*fsname_ptr);
            if ((*fsname_ptr = AllocVec(fsnamlen + 1, MEMF_ANY)) != NULL)
            {
                CopyMem(disktypelist[i], *fsname_ptr, fsnamlen);
                (*fsname_ptr)[fsnamlen] = '\0';
            }
            break;
        }
    }
}

/*** Methods ****************************************************************/
Object *DiskInfo__OM_NEW
(
    Class *CLASS, Object *self, struct opSet *message
)
{
    struct Screen              *pubscrn        = NULL;
    struct DosList             *dl;
    struct DiskInfo_DATA       *data           = NULL;
    struct TagItem             *tstate         = message->ops_AttrList;
    struct TagItem             *tag            = NULL;
    BPTR                        initial        = BNULL;
    Object                     *window,
                               *volnameobj, *voliconobj, *volusegaugeobj,
                               *volusedobj, *volfreeobj,
                               *grp, *grpformat;
    ULONG                       percent        = 0;
    LONG                        aspect         = 0;
    TEXT                        volname[108];
    TEXT                        size[64];
    TEXT                        used[64];
    TEXT                        free[64];
    TEXT                        blocksize[16];
    STRPTR                      status         = NULL;
    STRPTR                      dosdevname     = NULL;
    STRPTR                      deviceinfo     = NULL;

    STRPTR                      filesystem     = NULL;
    STRPTR                      volicon        = NULL;
#if (1)
    STRPTR                      handlertype    = "";
#endif
    STRPTR                      unknown    = _(MSG_UNKNOWN);

    static struct InfoData id;

    /* Parse initial taglist -----------------------------------------------*/
    D(bug("[DiskInfo] %s()\n", __func__));

    while ((tag = NextTagItem(&tstate)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_DiskInfo_Initial:
                initial = (BPTR) tag->ti_Data;
                D(bug("[DiskInfo] %s: initial lock @ 0x%p\n", __func__, initial));
                break;
                /* TODO: Remove MUIA_DiskInfo_Aspect */
            case MUIA_DiskInfo_Aspect:
                aspect = tag->ti_Data;
                D(bug("[DiskInfo] %s: aspect: %d\n", __func__, aspect));
                break;
        }
    }

    if ((pubscrn = LockPubScreen(NULL)) != NULL)
        UnlockPubScreen(NULL, pubscrn);

    /* Initial lock is required */
    if (initial == BNULL)
    {
        if (pubscrn)
            DisplayBeep(pubscrn);
        return NULL;
    }

    /* obtain volume's name from the lock */
    if (!NameFromLock(initial, volname, sizeof(volname))) {
        if (pubscrn) DisplayBeep(pubscrn);
        SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
        return NULL;
    }

    int volname_len = strlen(volname);
    if ((volicon = AllocVec(volname_len + 5, MEMF_CLEAR)) == NULL)
    {
        if (pubscrn) DisplayBeep(pubscrn);
        SetIoErr(ERROR_DEVICE_NOT_MOUNTED);
        return NULL;
    }
    strcpy(volicon, volname);
    strcat(volicon, "disk");
    volname[strlen(volname)-1] = '\0';
    D(bug("[DiskInfo] %s: Volume '%s'\n", __func__, volname));

    volname[strlen(volname)] = ':';

    /* Extract volume info from InfoData */
    if (Info(initial, &id) == DOSTRUE)
    {
        D(bug("[DiskInfo] %s: Info FSID = %08x\n", __func__, id.id_DiskType));
        GetFSNameFromID(id.id_DiskType, &filesystem);
        if (!filesystem)
        {
            filesystem = AllocVec(strlen(unknown) + 1, MEMF_ANY|MEMF_CLEAR);
            CopyMem(unknown, filesystem, strlen(unknown));
        }

        FormatSize(blocksize, sizeof blocksize, id.id_BytesPerBlock);
        FormatBlocksSized(size, sizeof size, id.id_NumBlocks, id.id_NumBlocks, id.id_BytesPerBlock, FALSE);
        percent = FormatBlocksSized(used, sizeof used, id.id_NumBlocksUsed, id.id_NumBlocks, id.id_BytesPerBlock, TRUE);
        FormatBlocksSized(free, sizeof free, id.id_NumBlocks - id.id_NumBlocksUsed, id.id_NumBlocks, id.id_BytesPerBlock, TRUE);

        switch (id.id_DiskState)
        {
        case (ID_WRITE_PROTECTED):
            status = _(MSG_READABLE);
            break;
        case (ID_VALIDATING):
            status = _(MSG_VALIDATING);
            break;
        case (ID_VALIDATED):
            status = _(MSG_READABLE_WRITABLE);
            break;
        default:
            status = unknown;
        }

        dl = LockDosList(LDF_VOLUMES | LDF_READ);
        dl = FindDosEntry(dl, volname, LDF_VOLUMES | LDF_READ);
        UnLockDosList(LDF_VOLUMES | LDF_READ);

        if (dl != NULL)
        {
            APTR voltask = dl->dol_Task;
            if (dl->dol_misc.dol_volume.dol_DiskType &&
                (dl->dol_misc.dol_volume.dol_DiskType != id.id_DiskType))
            {
                D(bug("[DiskInfo] %s: Volume FSID = %08x\n", __func__, dl->dol_misc.dol_volume.dol_DiskType);)
                GetFSNameFromID(dl->dol_misc.dol_volume.dol_DiskType, &filesystem);
            }
            dl = LockDosList(LDF_DEVICES|LDF_READ);
            if (dl) {
                while((dl = NextDosEntry(dl, LDF_DEVICES)))
                {
                    if (dl->dol_Task == voltask)
                    {
                        struct FileSysStartupMsg *fsstartup = (struct FileSysStartupMsg *)BADDR(dl->dol_misc.dol_handler.dol_Startup);
                        dosdevname = (UBYTE*)AROS_BSTR_ADDR(dl->dol_Name);
                        if (dl->dol_misc.dol_handler.dol_Handler)
                        {
                            char *fscur = filesystem;
                            filesystem = AllocVec(strlen(fscur) + AROS_BSTR_strlen(dl->dol_misc.dol_handler.dol_Handler) + 4, MEMF_ANY);
                            sprintf(filesystem,"%s (%s)", fscur, AROS_BSTR_ADDR(dl->dol_misc.dol_handler.dol_Handler));
                        }
                        if (fsstartup != NULL)
                        {
                           deviceinfo = AllocVec(AROS_BSTR_strlen(fsstartup->fssm_Device) + (fsstartup->fssm_Unit/10 + 1) + 7, MEMF_CLEAR);
                           sprintf(deviceinfo,"%s %s %d", (UBYTE*)AROS_BSTR_ADDR(fsstartup->fssm_Device), _(MSG_UNIT), (int)fsstartup->fssm_Unit);
                        }
                        break;
                    }
                }
                UnLockDosList(LDF_VOLUMES|LDF_READ);
            }
        }
    }

    if (!filesystem)
    {
        filesystem = AllocVec(strlen(unknown) + 1, MEMF_ANY);
        CopyMem(unknown, filesystem, strlen(unknown));
    }

    /* Create application and window objects -------------------------------*/
    self = (Object *) DoSuperNewTags
    (
        CLASS, self, NULL,

        MUIA_Application_Title, __(MSG_TITLE),
        MUIA_Application_Version, (IPTR) "$VER: DiskInfo 0.8 ("ADATE") \xA9 2006-2023 The AROS Dev Team",
        MUIA_Application_Copyright, __(MSG_COPYRIGHT),
        MUIA_Application_Author, __(MSG_AUTHOR),
        MUIA_Application_Description, __(MSG_DESCRIPTION),
        MUIA_Application_Base, (IPTR) "DISKINFO",
        SubWindow, (IPTR) (window = (Object *)WindowObject,
            MUIA_Window_Title,       __(MSG_TITLE),
            MUIA_Window_Activate,    TRUE,
            MUIA_Window_NoMenus,     TRUE,
            MUIA_Window_CloseGadget, TRUE,

            WindowContents, (IPTR) VGroup,
                Child, (IPTR) HGroup,
                    Child, (IPTR) VGroup,
                        Child, (IPTR) HVSpace,
                            Child, (IPTR) HGroup,
                                Child, (IPTR) HVSpace,
                                Child, (IPTR)(voliconobj = (Object *)IconImageObject,
                                    MUIA_InputMode, MUIV_InputMode_Toggle,
                                    MUIA_IconImage_File, (IPTR) volicon,
                                End),
                                Child, (IPTR) HVSpace,
                            End,
                            Child, (IPTR) HVSpace,
                        End,
                        Child, (IPTR) (grp = (Object *)VGroup,
                            Child, (IPTR) HVSpace,
                            Child, (IPTR) ColGroup(2),
                                /* TODO: Build this list only when data is realy available, and localise */
                                (dosdevname) ? Child : TAG_IGNORE, (IPTR) TextObject,
                                    MUIA_Text_PreParse, (IPTR) "\33r",
                                    MUIA_Text_Contents, __(MSG_DOSDEVICE),
                                End,
                                (dosdevname) ? Child : TAG_IGNORE, (IPTR) TextObject,
                                    MUIA_Text_PreParse, (IPTR) "\33l",
                                    MUIA_Text_Contents, (IPTR) dosdevname,
                                End,
                                (deviceinfo) ? Child : TAG_IGNORE, (IPTR) TextObject,
                                    MUIA_Text_PreParse, (IPTR) "\33r",
                                    MUIA_Text_Contents, __(MSG_DEVICEINFO),
                                End,
                                (deviceinfo) ? Child : TAG_IGNORE, (IPTR) TextObject,
                                    MUIA_Text_PreParse, (IPTR) "\33l",
                                    MUIA_Text_Contents, (IPTR) deviceinfo,
                                End,
                                Child, (IPTR) TextObject,
                                    MUIA_Text_PreParse, (IPTR) "\33r",
                                    MUIA_Text_Contents, __(MSG_FILESYSTEM),
                                End,
                                Child, (IPTR) TextObject,
                                    MUIA_Text_PreParse, (IPTR) "\33l",
                                    MUIA_Text_Contents, (IPTR) filesystem,
                                End,
#if (1)
                                Child, (IPTR) HVSpace,
                                Child, (IPTR) TextObject,
                                    MUIA_Text_PreParse, (IPTR) "\33l",
                                    MUIA_Text_Contents, (IPTR) handlertype,
                                End,
#endif
                            End,
                            Child, (IPTR) HVSpace,
                        End),
                        Child, (IPTR) HVSpace,
                    End,
                    Child, (IPTR) VGroup,
                        Child, (IPTR) HGroup,
                            MUIA_Weight, 100,
                            GroupFrame,
                            Child, (IPTR) HVSpace,
                            Child, (IPTR) ColGroup(2),
                                Child, (IPTR) TextObject,
                                    MUIA_Text_PreParse, (IPTR) "\33r",
                                    MUIA_Text_Contents, __(MSG_NAME),
                                End,
                                Child, (IPTR)(volnameobj = (Object *)TextObject,
                                    TextFrame,
                                    MUIA_Background, MUII_TextBack,
                                    MUIA_Text_PreParse, (IPTR) "\33b\33l",
                                    MUIA_Text_Contents, (IPTR) volname,
                                End),
                                Child, (IPTR) VGroup,
                                    Child, (IPTR) TextObject,
                                    TextFrame,
                                    MUIA_FramePhantomHoriz, (IPTR)TRUE,
                                    MUIA_Text_PreParse, (IPTR) "\33r",
                                    MUIA_Text_Contents, __(MSG_SIZE),
                                End,
                                Child, (IPTR) TextObject,
                                    TextFrame,
                                    MUIA_FramePhantomHoriz, (IPTR)TRUE,
                                    MUIA_Text_PreParse, (IPTR) "\33r",
                                    MUIA_Text_Contents, __(MSG_USED),
                                End,
                                Child, (IPTR) TextObject,
                                    TextFrame,
                                    MUIA_FramePhantomHoriz, (IPTR)TRUE,
                                    MUIA_Text_PreParse, (IPTR) "\33r",
                                    MUIA_Text_Contents, __(MSG_FREE),
                                End,
                            End,
                            Child, (IPTR) HGroup,
                                Child, (IPTR) VGroup,
                                    Child, (IPTR) TextObject,
                                        TextFrame,
                                        MUIA_Background, MUII_TextBack,
                                        MUIA_Text_PreParse, (IPTR) "\33l",
                                        MUIA_Text_Contents, (IPTR) size,
                                        End,
                                        Child, (IPTR)(volusedobj = (Object *)TextObject,
                                            TextFrame,
                                            MUIA_Background, MUII_TextBack,
                                            MUIA_Text_PreParse, (IPTR) "\33l",
                                            MUIA_Text_Contents, (IPTR) used,
                                        End),
                                        Child, (IPTR)(volfreeobj = (Object *)TextObject,
                                            TextFrame,
                                            MUIA_Background, MUII_TextBack,
                                            MUIA_Text_PreParse, (IPTR) "\33l",
                                            MUIA_Text_Contents, (IPTR) free,
                                        End),
                                    End,
                                    Child, (IPTR)(volusegaugeobj = (Object *)GaugeObject,
                                        GaugeFrame,
                                        MUIA_Gauge_InfoText, (IPTR) "",
                                        MUIA_Gauge_Horiz, FALSE,
                                        MUIA_Gauge_Current, percent,
                                    End),
                                End,
                                Child, (IPTR) TextObject,
                                    MUIA_Text_PreParse, (IPTR) "\33r",
                                    MUIA_Text_Contents, __(MSG_BLOCK_SIZE),
                                End,
                                Child, (IPTR) TextObject,
                                    TextFrame,
                                    MUIA_Background, MUII_TextBack,
                                    MUIA_Text_PreParse, (IPTR) "\33l",
                                    MUIA_Text_Contents, (IPTR) blocksize,
                                End,
                                Child, (IPTR) TextObject,
                                    MUIA_Text_PreParse, (IPTR) "\33r",
                                    MUIA_Text_Contents, __(MSG_STATUS),
                                End,
                                Child, (IPTR) TextObject,
                                    TextFrame,
                                    MUIA_Background, MUII_TextBack,
                                    MUIA_Text_PreParse, (IPTR) "\33l",
                                    MUIA_Text_Contents, (IPTR) status,
                                End,
                                Child, (IPTR) HVSpace,
                                Child, (IPTR) HVSpace,
                            End,
                            Child, (IPTR) HVSpace,
                        End,
                        Child, (IPTR) (grpformat = (Object *)HGroup,
                            // grpformat object userlevel sensitive
                            Child, (IPTR) HVSpace,
                        End),
                        Child, (IPTR) HVSpace,
                    End,
                End,
            End),
        TAG_DONE);

    /* Check if object creation succeeded */
    if (self == NULL)
    {
        if (pubscrn) DisplayBeep(pubscrn);
        return NULL;
    }

    /* Store instance data -------------------------------------------------*/
    data = INST_DATA(CLASS, self);

    data->dki_NotifyPort = CreateMsgPort();

    data->dki_Window        = window;

    data->dki_VolumeName    = volnameobj;
    data->dki_VolumeIcon    = voliconobj;
    data->dki_VolumeUseGauge    = volusegaugeobj;
    data->dki_VolumeUsed    = volusedobj;
    data->dki_VolumeFree    = volfreeobj;

    data->dki_DOSDev        = AllocVec(strlen(dosdevname) +2, MEMF_CLEAR);
    sprintf(data->dki_DOSDev, "%s:", dosdevname);

    data->dki_DOSDevInfo = deviceinfo;
    data->dki_FileSys = filesystem;
    data->dki_Aspect        = aspect;

    /* Setup notifications -------------------------------------------------*/
    DoMethod( window, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
        (IPTR) self, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);

    if (data->dki_NotifyPort)
    {
        /* Setup filesystem notification handler ---------------------------*/
        data->dki_NotifyIHN.ihn_Signals = 1UL << data->dki_NotifyPort->mp_SigBit;
        data->dki_NotifyIHN.ihn_Object  = self;
        data->dki_NotifyIHN.ihn_Method  = MUIM_DiskInfo_HandleNotify;

        DoMethod(self, MUIM_Application_AddInputHandler, (IPTR)&data->dki_NotifyIHN);

        data->dki_FSNotifyRequest.nr_Name                 = volname;
        data->dki_FSNotifyRequest.nr_Flags                = NRF_SEND_MESSAGE;
        data->dki_FSNotifyRequest.nr_stuff.nr_Msg.nr_Port = data->dki_NotifyPort;
        if (StartNotify(&data->dki_FSNotifyRequest))
        {
            D(bug("[DiskInfo] %s: FileSystem-Notification setup for '%s'\n", __func__, data->dki_FSNotifyRequest.nr_Name));
        }
        else
        {
            D(bug("[DiskInfo] %s: FAILED to setup FileSystem-Notification for '%s'\n", __func__, data->dki_FSNotifyRequest.nr_Name));
            DoMethod(self, MUIM_Application_RemInputHandler, (IPTR)&data->dki_NotifyIHN);
            DeleteMsgPort(data->dki_NotifyPort);
            data->dki_NotifyPort = NULL;
        }
    }
    return self;
}

IPTR DiskInfo__OM_DISPOSE(Class *CLASS, Object *self, Msg message)
{
    struct DiskInfo_DATA *data = INST_DATA(CLASS, self);

    D(bug("[DiskInfo] %s()\n", __func__));

    if (data->dki_NotifyPort)
    {
        DoMethod(self, MUIM_Application_RemInputHandler, (IPTR) &data->dki_NotifyIHN);

        EndNotify(&data->dki_FSNotifyRequest);

        DeleteMsgPort(data->dki_NotifyPort);
    }

    FreeVec(data->dki_DOSDev);
    FreeVec(data->dki_DOSDevInfo);
    FreeVec(data->dki_FileSys);

    return DoSuperMethodA(CLASS, self, message);
}

IPTR DiskInfo__MUIM_Application_Execute(Class *CLASS, Object *self, Msg message)
{
    struct DiskInfo_DATA *data = INST_DATA(CLASS, self);

    D(bug("[DiskInfo] %s()\n", __func__));

    SET(data->dki_Window, MUIA_Window_Open, TRUE);

    DoSuperMethodA(CLASS, self, message);

    SET(data->dki_Window, MUIA_Window_Open, FALSE);

    return (IPTR) NULL;
}

IPTR DiskInfo__MUIM_DiskInfo_HandleNotify
(
    Class *CLASS, Object *self, Msg message
)
{
    struct DiskInfo_DATA *data = INST_DATA(CLASS, self);
    struct NotifyMessage *npMessage = NULL;
    static struct InfoData id;
    BPTR fsdevlock = BNULL;
    BOOL di_Quit = FALSE;

    D(bug("[DiskInfo] %s()\n", __func__));

    if (data->dki_NotifyPort)
    {
        while ((npMessage = (struct NotifyMessage *)GetMsg(data->dki_NotifyPort)) != NULL)
        {
            D(bug("[DiskInfo] %s: FS notification received\n", __func__));

            if ((fsdevlock = Lock((STRPTR)XGET(data->dki_VolumeName, MUIA_Text_Contents), SHARED_LOCK)) != BNULL)
            {
                /* Extract volume info from InfoData */
                if (Info(fsdevlock, &id) == DOSTRUE)
                {
                    if (id.id_DiskType != ID_NO_DISK_PRESENT)
                    {
                        ULONG                       percent;
                        TEXT                        used[64];
                        TEXT                        free[64];

                        D(bug("[DiskInfo] %s: Updating Window from DOS Device '%s'\n", __func__, (STRPTR)XGET(data->dki_VolumeName, MUIA_Text_Contents)));

                        //FormatBlocksSized(size, id.id_NumBlocks, id.id_NumBlocks, id.id_BytesPerBlock, FALSE);
                        percent = FormatBlocksSized(used, sizeof used, id.id_NumBlocksUsed, id.id_NumBlocks, id.id_BytesPerBlock, TRUE);
                        FormatBlocksSized(free, sizeof free, id.id_NumBlocks - id.id_NumBlocksUsed, id.id_NumBlocks, id.id_BytesPerBlock, TRUE);
                        //sprintf(blocksize, "%d %s", id.id_BytesPerBlock, _(MSG_BYTES));

                        //data->dki_VolumeName    = volnameobj;
                        SET(data->dki_VolumeUsed, MUIA_Text_Contents, used);
                        SET(data->dki_VolumeFree, MUIA_Text_Contents, free);
                        SET(data->dki_VolumeUseGauge, MUIA_Gauge_Current, percent);
                    }
                    else
                    {
                        D(bug("[DiskInfo] %s: Volume no longer available on DOS Device '%s'\n", __func__, data->dki_DOSDev));
                        di_Quit = TRUE;
                    }
                }
                else
                {
                    D(bug("[DiskInfo] %s: Failed to obtain Info for DOS Device '%s'\n", __func__, data->dki_DOSDev));
                    di_Quit = TRUE;
                }

                UnLock(fsdevlock);
            }
            else
            {
                D(bug("[DiskInfo] %s: Failed to lock DOS Device '%s'\n", __func__, data->dki_DOSDev));
                di_Quit = TRUE;
            }
            ReplyMsg((struct Message *)npMessage);
        }
    }
    if (di_Quit)
    {
        /* TODO: set MUIV_Application_ReturnID_Quit */
    }
    return (IPTR)NULL;
}

IPTR DiskInfo__OM_GET(Class *CLASS, Object *self, struct opGet *msg)
{
    struct DiskInfo_DATA *data = INST_DATA(CLASS, self);
    IPTR retval = TRUE;

    D(bug("[DiskInfo] %s()\n", __func__));

    switch(msg->opg_AttrID)
    {
        case MUIA_DiskInfo_Volname:
            retval = (IPTR)XGET(data->dki_VolumeName, MUIA_Text_Contents);
            break;
        case MUIA_DiskInfo_Percent:
            retval = (ULONG)XGET(data->dki_VolumeUseGauge, MUIA_Gauge_Current);
            break;
        case MUIA_DiskInfo_Aspect:
            retval = (ULONG) data->dki_Aspect;
            break;
        default:
            retval = DoSuperMethodA(CLASS, self, (Msg)msg);
            break;
    }
    return retval;
}

IPTR DiskInfo__OM_SET(Class *CLASS, Object *self, struct opSet *msg)
{
    struct DiskInfo_DATA *data = INST_DATA(CLASS, self);
    struct TagItem *tags = msg->ops_AttrList;
    struct TagItem *tag;

    D(bug("[DiskInfo] %s()\n", __func__));

    while ((tag = NextTagItem(&tags)) != NULL)
    {
        switch (tag->ti_Tag)
        {
            case MUIA_DiskInfo_Aspect:
                data->dki_Aspect = tag->ti_Data;
                break;
        }
    }
    return DoSuperMethodA(CLASS, self, (Msg)msg);
}

/*** Dispatcher *************************************************************/
BOOPSI_DISPATCHER(IPTR, DiskInfo_Dispatcher, CLASS, self, message)
{
    switch (message->MethodID)
    {
        case OM_NEW:                return (IPTR) DiskInfo__OM_NEW(CLASS, self, (struct opSet *) message);
        case OM_DISPOSE:            return DiskInfo__OM_DISPOSE(CLASS, self, message);
        case OM_GET:                return (IPTR) DiskInfo__OM_GET(CLASS, self, (struct opGet *)message);
        case OM_SET:                return (IPTR) DiskInfo__OM_SET(CLASS, self, (struct opSet *)message);
        case MUIM_DiskInfo_HandleNotify:    return DiskInfo__MUIM_DiskInfo_HandleNotify(CLASS, self, message);
        case MUIM_Application_Execute:        return DiskInfo__MUIM_Application_Execute(CLASS, self, message);
        default:                    return DoSuperMethodA(CLASS, self, message);
    }
    return 0;
}
BOOPSI_DISPATCHER_END

/*** Setup ******************************************************************/
struct MUI_CustomClass *DiskInfo_CLASS;

BOOL DiskInfo_Initialize()
{
    D(bug("[DiskInfo] %s()\n", __func__));

    DiskInfo_CLASS = MUI_CreateCustomClass(
        NULL, MUIC_Application, NULL,
        sizeof(struct DiskInfo_DATA), DiskInfo_Dispatcher);

    return DiskInfo_CLASS ? TRUE : FALSE;
}

VOID DiskInfo_Deinitialize()
{
    D(bug("[DiskInfo] %s()\n", __func__));

    if (DiskInfo_CLASS != NULL)
    {
        MUI_DeleteCustomClass(DiskInfo_CLASS);
    }
}
