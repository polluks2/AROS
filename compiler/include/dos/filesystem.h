#ifndef DOS_FILESYSTEM_H
#define DOS_FILESYSTEM_H

/*
    (C) 1995-97 AROS - The Amiga Replacement OS
    $Id$

    Desc: AROS specific structures and constants for filesystems.
    Lang: english
*/
#ifndef EXEC_IO_H
#   include <exec/io.h>
#endif
#ifndef DOS_FILEHANDLER_H
#   include <dos/filehandler.h>
#endif


/* This structure is an extended IORequest. It is used for messages to
   filesystem handlers. */
struct IOFileSys
{
    struct IORequest IOFS;	   /* Standard I/O request. */
    LONG	     io_DosError;  /* Dos error code. */
    IPTR	     io_Args[4];   /* Array of arguments. */
};

/* Filesystem actions. Look into one of the filesystem sources for more
   information on required parameters. */
#define FSA_OPEN		1
#define FSA_CLOSE		2
#define FSA_READ		3
#define FSA_WRITE		4
#define FSA_SEEK		5
#define FSA_SET_FILE_SIZE	6
#define FSA_WAIT_CHAR		7
#define FSA_FILE_MODE		8
#define FSA_IS_INTERACTIVE	9
#define FSA_SAME_LOCK		10
#define FSA_EXAMINE		11
#define FSA_EXAMINE_ALL 	12
#define FSA_EXAMINE_ALL_END	13
#define FSA_OPEN_FILE		14
#define FSA_CREATE_DIR		15
#define FSA_CREATE_HARDLINK	16
#define FSA_CREATE_SOFTLINK	17
#define FSA_RENAME		18
#define FSA_READ_SOFTLINK	19
#define FSA_DELETE_OBJECT	20
#define FSA_SET_COMMENT 	21
#define FSA_SET_PROTECT 	22
#define FSA_SET_OWNER		23
#define FSA_SET_DATE		24
#define FSA_IS_FILESYSTEM	25
#define FSA_MORE_CACHE		26
#define FSA_FORMAT		27
#define FSA_MOUNT_MODE		28
#if 0
#define FSA_SERIALIZE_DISK	29
#define FSA_FLUSH		30
#define FSA_INHIBIT		31
#define FSA_WRITE_PROTECT	32
#define FSA_DISK_CHANGE 	33
#define FSA_ADD_NOTIFY		34
#define FSA_REMOVE_NOTIFY	35
#define FSA_DISK_INFO		36
#define FSA_CHANGE_SIGNAL	37
#define FSA_LOCK_RECORD 	38
#define FSA_UNLOCK_RECORD	39
#endif

/* Modes for FSA_OPEN, FSA_OPEN_FILE and FSA_FILE_MODE. */
#define FMF_LOCK    (1L<<0) /* Lock exclusively. */
#define FMF_EXECUTE (1L<<1) /* Open for executing. */
#define FMF_WRITE   (1L<<2) /* Open for writing. */
#define FMF_READ    (1L<<3) /* Open for reading. */
#define FMF_CREATE  (1L<<4) /* Create file if it doesn't exist. */
#define FMF_CLEAR   (1L<<5) /* Clear file on open. */
#define FMF_RAW     (1L<<6) /* Switch cooked to raw and vice versa. */

/* Mount modes. */
#define MMF_READ	(1L<<0) /* Mounted for reading. */
#define MMF_WRITE	(1L<<1) /* Mounted for writing. */
#define MMF_READ_CACHE	(1L<<2) /* Read cache enabled. */
#define MMF_WRITE_CACHE (1L<<3) /* Write cache enabled. */
#define MMF_OFFLINE	(1L<<4) /* Filesystem doesn't use the device
                                   currently. */
#define MMF_LOCKED	(1L<<5) /* Mount mode is password protected. */

#endif /* DOS_FILESYSTEM_H */
