#ifndef DOS_FILESYSTEM_H
#define DOS_FILESYSTEM_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: AROS specific structures and definitions for filesystems.
    Lang: english
*/
#ifndef EXEC_IO_H
#   include <exec/io.h>
#endif
#ifndef DOS_FILEHANDLER_H
#   include <dos/filehandler.h>
#endif
#ifndef DOS_EXALL_H
#   include <dos/exall.h>
#endif

/* This file is AROS specific. Do not use the structures and #defines contained
   in here if you want to stay compatible with AmigaOS! */


/* Filesystem actions. Not all filesystems support all actions. Filesystems
   have to return ERROR_ACTION_NOT_KNOWN (<dos/dos.h>) in
   IOFileSys->io_DosError if they do not support an action, with which they
   were called. If they know an action but ignore it by purpose, they might
   return ERROR_NOT_IMPLEMENTED. One of these purposes is that the hardware
   or software the filesystem relies on does not support this kind of action.
   (eg a net-filesystems may not support renaming of files, so its filesystem
   should return ERROR_NOT_IMPLEMENTED for FSA_RENAME. Another example is the
   nil-device, which does not implement FSA_CREATE_DIR or any of this fancy
   stuff.) What does that mean for an application? If an application receives
   ERROR_NOT_IMPLEMENTED, it knows that the action, it wanted to perform,
   makes no sense for that filesystem. If it receives ERROR_ACTION_NOT_KNOWN,
   it knows that the filesystem does not know about this action for whatever
   reason (including that it makes no sense to perform that action on that
   specific filesystem).

   All actions work relative to the relative directory (where it makes sense),
   whichs filehandle is to be set in the IOFileSys->IOFS.io_Unit field. This
   field also serves as a container for filehandles for actions that either
   need a filehandle as argument or return one. When not stated otherwise this
   field has to be set to the filehandle to affect or is set to the filehandle
   that is returned from the action.

   Whenever a filename is required as argument, this filename has to be
   stripped from the devicename, ie it has to be relative to the current
   directory on that volume (set in the io_Unit field). */

/* Returns a new filehandle. The file may be newly created (depending on
   io_FileMode. */
#define FSA_OPEN 1
struct IFS_OPEN
{
    STRPTR io_Filename; /* File to open. */
    IPTR   io_FileMode; /* see below */
};

/* Closes an opened filehandle. Takes no extra arguments. */
#define FSA_CLOSE 2

/* Reads from a filehandle into a buffer. */
#define FSA_READ 3
struct IFS_READ
{
    char * io_Buffer; /* The buffer to read into. */
    IPTR   io_Length; /* The length of the buffer. This is filled by the
                         filehandler with the number of bytes actually read. */
};

/* Writes the contents of a buffer into a filehandle. */
#define FSA_WRITE 4
struct IFS_WRITE
{
    char * io_Buffer; /* The buffer to write into the file. */
      /* The number of bytes to write. This is filled by the filehandler with
         the number of bytes actually written. */
    IPTR   io_Length;
};

/* The action does exactly the same as the function Seek(). */
#define FSA_SEEK 5
struct IFS_SEEK
{
    IPTR io_Negative; /* TRUE, if offset is negative, otherwise FALSE. */
      /* Offset from position, specified as mode. This is filled by the
         filehandler with the old position in the file. */
    IPTR io_Offset;
    IPTR io_SeekMode; /* Seek mode as defined in <dos/dos.h> (OFFSET_*). */
};

/* Sets the size of filehandle. Uses IFS_SEEK (see above) as argument array. */
#define FSA_SET_FILE_SIZE 6

/* Currently undocumented. */
#define FSA_WAIT_CHAR 7

/* Applies a new mode to a file. If you supply io_Mask with a value of 0,
   no changes are made and you can just read the resulting io_FileMode. */
#define FSA_FILE_MODE 8
struct IFS_FILE_MODE
{
      /* The new mode to apply to the filehandle. See below for definitions.
         The filehandler fills this with the old mode bits. */
    IPTR io_FileMode;
      /* This mask defines, which flags are to be changed. */
    IPTR io_Mask;
};

/* This action can be used to query if a filehandle is interactive, ie if it
     is a terminal or not. */
#define FSA_IS_INTERACTIVE 9
struct IFS_IS_INTERACTIVE
{
      /* This boolean is filled by the filehandler. It is set to TRUE, if the
         filehandle is interactive, otherwise it is set to FALSE. */
    IPTR io_IsInteractive;
};

/* Compares two locks for equality. */
#define FSA_SAME_LOCK 10
struct IFS_SAME_LOCK
{
    void * io_Lock[2]; /* The two locks to compare. */
    IPTR   io_Same;    /* This set to one of LOCK_DIFFERENT or LOCK_SAME (see
                          <dos/dos.h> by the filehandler. */
};

/* Examines a filehandle, giving various information about it. */
#define FSA_EXAMINE 11
struct IFS_EXAMINE
{
      /* ExAllData structure buffer to be filled by the filehandler. */
    struct ExAllData * io_ead;
    IPTR               io_Size; /* Size of the buffer. */
      /* With which kind of information shall the buffer be filled with? See
         ED_* definitions in <dos/exall.h> for more information. */
    IPTR               io_Mode;
};

/* Works exactly like FSA_EXAMINE with the exeption that multiple files may be
   examined, ie the filehandle must be a directory. */
#define FSA_EXAMINE_ALL 12

/* This has to be called, if FSA_EXAMINE_ALL is stopped before all examined
   files were returned. It takes no arguments except the filehandle in
   io_Unit. */
#define FSA_EXAMINE_ALL_END 13

/* Works exactly like FSA_OPEN, but you can additionally specify protection
   bits to be applied to new files. */
#define FSA_OPEN_FILE 14
struct IFS_OPEN_FILE
{
    STRPTR io_Filename;   /* File to open. */
    IPTR   io_FileMode;   /* see below */
    IPTR   io_Protection; /* The protection bits. */
};

/* Creates a new directory. The filehandle of that new directory is returned.
*/
#define FSA_CREATE_DIR 15
struct IFS_CREATE_DIR
{
    STRPTR io_Filename;   /* Name of directory to create. */
    IPTR   io_Protection; /* The protection bits. */
};

/* Creates a hard link (ie gives one file a second name). */
#define FSA_CREATE_HARDLINK 16
struct IFS_CREATE_HARDLINK
{
    STRPTR   io_Filename; /* The filename of the link to create. */
    void   * io_OldFile;  /* Filehandle of the file to link to. */
};

/* Creates a soft link (ie a file is created, which references another by its
   name). */
#define FSA_CREATE_SOFTLINK 17
struct IFS_CREATE_SOFTLINK
{
    STRPTR io_Filename;  /* The filename of the link to create. */
    STRPTR io_Reference; /* The name of the file to link to. */
};

/* Renames a file. To the old and the new name, the current directory is
   applied to. */
#define FSA_RENAME 18
struct IFS_RENAME
{
    STRPTR io_Filename; /* The old filename. */
    STRPTR io_NewName;  /* The new filename. */
};

/* Currently undocumented. */
#define FSA_READ_SOFTLINK 19

/* Deletes an object on the volume. */
#define FSA_DELETE_OBJECT 20
struct IFS_DELETE_OBJECT
{
    STRPTR io_Filename; /* The name of the file to delete. */
};

/* Sets a filecomment for a file. */
#define FSA_SET_COMMENT 21
struct IFS_SET_COMMENT
{
    STRPTR io_Filename; /* The file of the file to be commented. */
    STRPTR io_Comment;  /* The new filecomment. May be NULL, in which case the
                           current filecomment is deleted. */
};

/* Sets the protection bits of a file. */
#define FSA_SET_PROTECT 22
struct IFS_SET_PROTECT
{
    STRPTR io_Filename;   /* The file to change. */
    IPTR   io_Protection; /* The new protection bits. */
};

/* Sets the ownership of a file. */
#define FSA_SET_OWNER 23
struct IFS_SET_OWNER
{
    STRPTR io_Filename; /* The file to change. */
    IPTR   io_UID;      /* The new owner. */
    IPTR   io_GID;      /* The new group owner. */
};

  /* Sets the last modification date of the filename given as first argument.
     The date is given as standard TimeStamp structure (see <dos/dos.h>) as
     second to fourth argument (ie as days, minutes and ticks). */
#define FSA_SET_DATE 24
struct IFS_SET_DATE
{
    STRPTR io_Filename; /* The file to change. */
    /* The following three fields are used to specify the date, the file should
       get. They have the same meaning as the fields in struct DateStamp
       (<dos/dos.h>), but note well that these fields are IPTRs, while the
       fields in struct DateStamp are ULONGs! */
    IPTR   io_Days;
    IPTR   io_Minute;
    IPTR   io_Ticks;
};

/* Currently undocumented. */
#define FSA_IS_FILESYSTEM 25

/* Currently undocumented. */
#define FSA_MORE_CACHE 26

/* Currently undocumented. */
#define FSA_FORMAT 27

  /* Resets/Reads the mount-mode of the volume passed in as io_Unit. The first
     and second argument work exactly like FSA_FILE_MODE, but the third
     argument can contain a password, if MMF_LOCKED is set. */
#define FSA_MOUNT_MODE 28
struct IFS_MOUNT_MODE
{
      /* The new mode to apply to the volume. See below for definitions. The
         filehandler fills this with the old mode bits. */
    IPTR   io_MountMode;
    /* This mask defines, which flags are to be changed. */
    IPTR   io_Mask;
      /* A passwort, which is needed, if MMF_LOCKED is set. */
    STRPTR io_Password;
};

/* The following actions are currently not supported. */
#if 0
#define FSA_SERIALIZE_DISK  29
#define FSA_FLUSH	    30
#define FSA_INHIBIT	    31
#define FSA_WRITE_PROTECT   32
#define FSA_DISK_CHANGE     33
#define FSA_ADD_NOTIFY	    34
#define FSA_REMOVE_NOTIFY   35
#define FSA_DISK_INFO	    36
#define FSA_CHANGE_SIGNAL   37
#define FSA_LOCK_RECORD     38
#define FSA_UNLOCK_RECORD   39
#endif

/* io_FileMode for FSA_OPEN, FSA_OPEN_FILE and FSA_FILE_MODE. These are flags
   and may be or'ed. Note that not all filesystems support all flags. */
#define FMF_LOCK    (1L<<0) /* Lock exclusively. */
#define FMF_EXECUTE (1L<<1) /* Open for executing. */
/* At least one of the following two flags must be specified. Otherwise expect
   strange things to happen. */
#define FMF_WRITE   (1L<<2) /* Open for writing. */
#define FMF_READ    (1L<<3) /* Open for reading. */
#define FMF_CREATE  (1L<<4) /* Create file if it doesn't exist. */
#define FMF_CLEAR   (1L<<5) /* Truncate file on open. */
#define FMF_RAW     (1L<<6) /* Switch cooked to raw and vice versa. */

/* io_MountMode for FSA_MOUNT_MODE. These are flags and may be or'ed. */
#define MMF_READ	(1L<<0) /* Mounted for reading. */
#define MMF_WRITE	(1L<<1) /* Mounted for writing. */
#define MMF_READ_CACHE	(1L<<2) /* Read cache enabled. */
#define MMF_WRITE_CACHE (1L<<3) /* Write cache enabled. */
#define MMF_OFFLINE	(1L<<4) /* Filesystem doesn't use the device currently.
                                */
#define MMF_LOCKED	(1L<<5) /* Mount mode is password protected. */


/* This structure is an extended IORequest. It is used for messages to
   AROS filesystem handlers. */
struct IOFileSys
{
    struct IORequest IOFS;	  /* Standard I/O request. */
    LONG	     io_DosError; /* Dos error code. */

    /* This union contains all the data needed for the various actions. */
    union
    {
        IPTR io_ArgArray[4]; /* Generic argument space. */
	struct {
	    STRPTR io_Filename;
	} io_NamedFile;

        struct IFS_OPEN            io_OPEN;           /* FSA_OPEN */
        struct IFS_READ            io_READ;           /* FSA_READ */
        struct IFS_WRITE           io_WRITE;          /* FSA_WRITE */
        struct IFS_SEEK            io_SEEK;           /* FSA_SEEK */
        struct IFS_FILE_MODE       io_FILE_MODE;      /* FSA_FILE_MODE */
        struct IFS_IS_INTERACTIVE  io_IS_INTERACTIVE; /* FSA_IS_INTERACTIVE */
        struct IFS_SAME_LOCK       io_SAME_LOCK;      /* FSA_SAME_LOCK */
        struct IFS_EXAMINE         io_EXAMINE;        /* FSA_EXAMINE */
        struct IFS_OPEN_FILE       io_OPEN_FILE;      /* FSA_OPEN_FILE */
        struct IFS_CREATE_DIR      io_CREATE_DIR;     /* FSA_CREATE_DIR */
        struct IFS_CREATE_HARDLINK io_CREATE_HARDLINK;/* FSA_CREATE_HARDLINK */
        struct IFS_CREATE_SOFTLINK io_CREATE_SOFTLINK;/* FSA_CREATE_SOFTLINK */
        struct IFS_RENAME          io_RENAME;         /* FSA_RENAME */
        struct IFS_DELETE_OBJECT   io_DELETE_OBJECT;  /* FSA_DELETE_OBJECT */
        struct IFS_SET_COMMENT     io_SET_COMMENT;    /* FSA_SET_COMMENT */
        struct IFS_SET_PROTECT     io_SET_PROTECT;    /* FSA_SET_PROTECT */
        struct IFS_SET_OWNER       io_SET_OWNER;      /* FSA_SET_OWNER */
        struct IFS_SET_DATE        io_SET_DATE;       /* FSA_SET_DATE */
        struct IFS_MOUNT_MODE      io_MOUNT_MODE;     /* FSA_MOUNT_MODE */
    } io_Union;
};
#define io_Args io_Union.io_ArgArray

#endif /* DOS_FILESYSTEM_H */
