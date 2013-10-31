/*
 *************************************************************************
 *
 * flash.h --
 *
 *	Declarations for the Flash layer. 
 * 
 *      NOTE: The Flash layer is not thread safe. If your LFS is multi-threaded
 *      you'll have to put the proper synchronization functions around your
 *      calls to these routines.
 *
 *
 *************************************************************************
 */

#ifndef _FLASH_H
#define _FLASH_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *************************************************************************
 *
 * Useful definitions.
 *
 *************************************************************************
 */

typedef int Boolean;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/*
 *************************************************************************
 *
 * Number of sectors in an erase block. Don't change this.
 *
 *************************************************************************
 */

#define FLASH_SECTORS_PER_BLOCK 16

/*
 *************************************************************************
 *
 * Flash sector size in bytes. Don't change this, I haven't tested with other values.
 *
 *************************************************************************
 */

#define FLASH_SECTOR_SIZE 512

/*
 *************************************************************************
 *
 * Flash erase block size. Don't change this, I haven't tested with other values.
 *
 *************************************************************************
 */

#define FLASH_BLOCK_SIZE (FLASH_SECTOR_SIZE * FLASH_SECTORS_PER_BLOCK)

/*
 *************************************************************************
 * int
 * Flash_Create
 *
 * Parameters:
 *
 *  	char 		*file	-- name of the flash file to create.
 *      u_int           wearLimit -- erase block wear limit (<= 100000)
 *  	u_int		*blocks -- # of blocks in the flash (<= 1000000)
 *
 * Returns:
 *	0 on success, 1 otherwise and errno is set.
 *
 *
 * Flash_Create creates a flash memory in file "file". It will contain "blocks" erase blocks
 * each of which has a wear limit of "wearLimit". The wear on each block is initially 0,
 * and each block is initially erased. 
 *
 *************************************************************************
 */

int Flash_Create(char *file, u_int wearLimit, u_int blocks);

/*
 *************************************************************************
 *
 * Handle for a flash returned by Flash_Open and passed to the other routines.
 *
 *************************************************************************
 */

typedef void *Flash;

/*
 *************************************************************************
 * Flash
 * Flash_Open
 *
 * Parameters:
 *
 *  	char 		*file	-- name of the flash file to open.
 *  	Flash_Flags	flags	-- FLASH_SILENT and/or FLASH_ASYNC
 *  	u_int		*blocks -- # of blocks in the flash
 *
 * Returns:
 *	Flash handle on success, NULL otherwise and errno is set.
 *
 *
 * Flash_Open opens a flash and returns a handle for it that is used
 * in subsequent calls to Flash_Read, Flash_Write, and Flash_Close. 
 * The specified file must exist. The flash size in erease blocks is returned
 * in "blocks".
 *
 *************************************************************************
 */

typedef u_int Flash_Flags;

#define FLASH_SILENT	0x1  // don't print statistics when Flash_Close is called	
#define FLASH_ASYNC	0x2  // don't simulate synchronous flash operations


Flash	Flash_Open(char *file, Flash_Flags flags, u_int *blocks);

/*
 *************************************************************************
 * int
 * Flash_Read
 *
 * Parameters:
 *
 *  	Flash 		flash	-- flash to read
 *	u_int		sector -- starting offset, in sectors
 * 	u_int		count -- # of sectors to read
 *	void		*buffer -- buffer into which flash is read
 *
 * Returns:
 *	0 on success, 1 otherwise and errno is set.
 *
 *
 * Flash_Read reads "count" sectors from "flash" into "buffer" 
 * starting at sector "sector". 
 *
 *************************************************************************
 */

int	Flash_Read(Flash flash, u_int sector, u_int count, void *buffer);

/*
 *************************************************************************
 * int
 * Flash_Write
 *
 * Parameters:
 *
 *  	Flash 		flash	-- flash to write
 *	u_int		sector -- starting offset, in sectors
 * 	u_int		count -- # of sectors to write
 *	void		*buffer -- buffer from which flash is written
 *
 * Returns:
 *	0 on success, 1 otherwise and errno is set.
 *
 *
 * Flash_Write writes "count" sectors from "buffer" to "flash" 
 * starting at sector "sector". 
 *
 *************************************************************************
 */

int	Flash_Write(Flash flash, u_int sector, u_int count, void *buffer);

/*
 *************************************************************************
 * int
 * Flash_Erase
 *
 * Parameters:
 *
 *  	Flash 		flash	-- flash to write
 *	u_int		block -- starting offset, in blocks
 * 	u_int		count -- # of blocks to erase
 *
 * Returns:
 *	0 on success, 1 otherwise and errno is set.
 *
 *
 * Flash_Erase erases "count" blocks in "flash" starting at block "block". 
 *
 *************************************************************************
 */

int	Flash_Erase(Flash flash, u_int block, u_int count);


/*
 *************************************************************************
 * int
 * Flash_GetWear
 *
 * Parameters:
 *
 *  	Flash 		flash	-- flash to write
 *	u_int		block -- starting offset, in blocks
 * 	u_int		*wear -- block's wear level
 *
 * Returns:
 *	0 on success, 1 otherwise and errno is set.
 *
 *
 * Flash_GetWear returns the wear level for block "block" in the address pointed to by "wear"
 *
 *************************************************************************
 */

int	Flash_GetWear(Flash flash, u_int block, u_int *wear);

/*
 *************************************************************************
 * int
 * Flash_Close
 *
 * Parameters:
 *
 *  	Flash 		flash	-- flash to close
 *
 * Returns:
 *	0 on success, 1 otherwise and errno is set.
 *
 *
 * Flash_Close closes the flash.
 *
 *************************************************************************
 */

int	Flash_Close(Flash flash);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
