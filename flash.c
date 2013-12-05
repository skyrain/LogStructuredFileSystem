#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "flash.h"

#define assume(expr) {                                                  \
    if (!(expr)) {                                                      \
        printf("%s:%d: failed assumption `%s'\n", __FILE__, __LINE__,   \
            #expr);                                                     \
        abort();                                                        \
    }                                                                   \
}

#define FLASH_READ 0
#define FLASH_WRITE 1
#define FLASH_ERASE 2

typedef struct FlashHeader {
    char        magic[8];          // "FLASH"
    u_int       wearLimit;
    u_int       blocks;
    u_int       stateOffset;      // Offset of sector state array, in bytes.
    u_int       wearOffset;       // Offset of wear level array, in bytes.
    u_int       blockOffset;      // Offset of data blocks, in bytes.
} FlashHeader;


typedef struct FlashInfo {
    char	*file;
    Flash_Flags	flags;
    int		fd;
    FlashHeader hdr;
    long long	readOps;
    long long	readSectors;
    long long	writeOps;
    long long	writeSectors;
    long long   eraseOps;
    long long   eraseBlocks;
} FlashInfo;


#define FLASH_STATE_EMPTY 0
#define FLASH_STATE_FULL 1

int
Flash_Create(
    char        *file,
    u_int       wearLimit,
    u_int       blocks)
{
    int         rc;
    FlashHeader hdr;
    int         fd = -1;
    u_int       sectors;
    int         amount;
    off_t       len;

    fd = open(file, O_CREAT|O_RDWR|O_TRUNC, 0666);
    if (fd < 0) {
	rc = 1;
	goto done;
    }
    strcpy(hdr.magic, "FLASH");
    if (wearLimit > 100000) {
        errno = EINVAL;
        rc = 1;
        goto done;
    }
    hdr.wearLimit = wearLimit;
    if (blocks > 1000000) {
        errno = EINVAL;
        rc = 1;
        goto done;
    }
    hdr.blocks = blocks;
    sectors = blocks * 16;
    hdr.stateOffset = sizeof(FlashHeader);
    hdr.wearOffset = hdr.stateOffset + sectors;
    hdr.blockOffset = hdr.wearOffset + (blocks * sizeof(u_int));
    amount = write(fd, &hdr, sizeof(hdr));
    if (amount != sizeof(hdr)) {
        rc = 1;
        if (errno == 0) {
            errno = EIO;
        }
        goto done;
    }
    len = hdr.blockOffset + (hdr.blocks * FLASH_BLOCK_SIZE);
    rc = ftruncate(fd, len);
    if (rc) {
        goto done;
    }
    rc = 0;
done:
    if (fd >= 0) {
        close(fd);
    }
    return rc;
}

int
Flash_GetWear(
    Flash       flashHandle,
    u_int       block,
    u_int       *wear)

{
    FlashInfo	*flash = (FlashInfo *) flashHandle;
    u_int       buffer;
    off_t       offset;
    off_t       result;
    int         rc;
    int         amount;

    offset = flash->hdr.wearOffset + (block * sizeof(u_int));
    result = lseek(flash->fd, offset, SEEK_SET);
    if (result != offset) {
        rc = 1;
        goto done;
    }
    amount = read(flash->fd, &buffer, sizeof(buffer));
    if (amount != sizeof(buffer)) {
        rc = 1;
        if (errno == 0) {
            errno = EIO;
        }
        goto done;
    }
    rc = 0;
    *wear = buffer;
done:
    return rc;
}


int
FlashSetWear(
    Flash       flashHandle,
    int         block,
    u_int       wear)

{
    FlashInfo	*flash = (FlashInfo *) flashHandle;
    off_t       offset;
    off_t       result;
    int         rc;
    int         amount;

    offset = flash->hdr.wearOffset + (block * sizeof(u_int));
    result = lseek(flash->fd, offset, SEEK_SET);
    if (result != offset) {
        rc = 1;
        goto done;
    }
    amount = write(flash->fd, &wear, sizeof(wear));
    if (amount != sizeof(wear)) {
        rc = 1;
        if (errno == 0) {
            errno = EIO;
        }
        goto done;
    }
    rc = 0;
done:
    return rc;
}

int
FlashGetState(
    Flash       flashHandle,
    int         sector,
    u_char      *state)

{
    FlashInfo	*flash = (FlashInfo *) flashHandle;
    u_char      buffer;
    off_t       offset;
    off_t       result;
    int         rc;
    int         amount;

    offset = flash->hdr.stateOffset + (sector * sizeof(u_char));
    result = lseek(flash->fd, offset, SEEK_SET);
    if (result != offset) {
        rc = 1;
        goto done;
    }
    amount = read(flash->fd, &buffer, sizeof(buffer));
    if (amount != sizeof(buffer)) {
        rc = 1;
        if (errno == 0) {
            errno = EIO;
        }
        goto done;
    }
    rc = 0;
    *state = buffer;
done:
    return rc;
}

int
FlashSetState(
    Flash       flashHandle,
    int         sector,
    u_char      state)

{
    FlashInfo	*flash = (FlashInfo *) flashHandle;
    off_t       offset;
    off_t       result;
    int         rc;
    int         amount;

    offset = flash->hdr.stateOffset + (sector * sizeof(u_char));
    result = lseek(flash->fd, offset, SEEK_SET);
    if (result != offset) {
        rc = 1;
        goto done;
    }
    amount = write(flash->fd, &state, sizeof(state));
    if (amount != sizeof(state)) {
        rc = 1;
        if (errno == 0) {
            errno = EIO;
        }
        goto done;
    }
    rc = 0;
done:
    return rc;
}

Flash
Flash_Open(
    char	*file,
    Flash_Flags	flags,
    u_int	*blocks)
{
    FlashInfo	*flash;
    int		rc;
    int         amount;

    flash = (FlashInfo *) malloc(sizeof(FlashInfo));
    memset(flash, 0, sizeof(*flash));
    assume(flash != NULL);
    flash->file = file;
    flash->flags = flags;
    flash->fd = open(file, O_RDWR);
    
    if (flash->fd < 0) {
	rc = 1;
	goto done;
    }
    amount = read(flash->fd, &flash->hdr, sizeof(flash->hdr));
    if (amount != sizeof(flash->hdr)) {
        rc = 1;
        if (errno == 0) {
            errno = EIO;
        }
        goto done;
    }
    if (strcmp(flash->hdr.magic, "FLASH")) {
        rc = 1;
        errno = EIO;
        goto done;
    }
    *blocks = flash->hdr.blocks;
    rc = 0;
done:
    if (rc) {
	free((void *) flash);
	flash = NULL;
    }
    return flash;
}

static int
FlashIO(
    FlashInfo	*flash,
    int		type,
    u_int	offset,
    u_int	count,
    void	*buffer)
{
    off_t	seekOffset;
    off_t	resultOffset;
    int		rc;
    ssize_t	amount;

    if (flash == NULL) {
	rc = 1;
	errno = EINVAL;
	goto done;
    }
    if ((offset + count) > flash->hdr.blocks * FLASH_SECTORS_PER_BLOCK) {
        rc = 1;
        errno = EINVAL;
        goto done;
    }
    seekOffset = flash->hdr.blockOffset + (offset * FLASH_SECTOR_SIZE);
    resultOffset = lseek(flash->fd, seekOffset, SEEK_SET);
    if (resultOffset != seekOffset) {
	rc = 1;
	goto done;
    }
    switch (type) {
	case FLASH_READ: 
	    amount = read(flash->fd, buffer, count * FLASH_SECTOR_SIZE);
	    break;
	case FLASH_WRITE: 
	    amount = write(flash->fd, buffer, count * FLASH_SECTOR_SIZE);
	    break;
	default:
	    fprintf(stderr, "Internal error in FlashIO\n");
	    rc = 1;
	    goto done;
    }
    if (amount != (count * FLASH_SECTOR_SIZE)) {
	rc = 1;
	if (errno == 0) {
	    errno = EIO;
	}
	goto done;
    }
    if ((flash->flags & FLASH_ASYNC) == 0) {
	struct timespec req;
	req.tv_sec = 0;
	req.tv_nsec = 10000000;
	rc = nanosleep(&req, NULL);
	if (rc) {
	    rc = 1;
	    goto done;
	}
    }
    rc = 0;
done:
    return rc;
}

int
Flash_Read(
    Flash	flashHandle,
    u_int	sector,
    u_int	count,
    void	*buffer)
{
    FlashInfo	*flash = (FlashInfo *) flashHandle;
    int		rc;

    rc = FlashIO(flash, FLASH_READ, sector, count, buffer);
    if (rc == 0) {
	flash->readOps++;
	flash->readSectors += count;
    }
    return rc;
}

int
Flash_Write(
    Flash	flashHandle,
    u_int	sector,
    u_int	count,
    void	*buffer)
{
    FlashInfo	*flash = (FlashInfo *) flashHandle;
    int		rc;
    int         i;
    u_char      state;

    for (i = sector; i < sector + count; i++) {
        rc = FlashGetState(flash, i, &state);
        if (rc) {
            goto done;
        }
        if (state == FLASH_STATE_FULL) {
            rc = 1;
            errno = EIO;
            goto done;
        }
    }
    rc = FlashIO(flash, FLASH_WRITE, sector, count, buffer);
    if (rc) {
        goto done;
    }
    for (i = sector; i < sector + count; i++) {
        rc = FlashSetState(flash, i, FLASH_STATE_FULL);
        if (rc) {
            goto done;
        }
    }
    rc = 0;
    flash->writeOps++;
    flash->writeSectors += count;
done:
    return rc;
}


int
Flash_Erase(
    Flash	flashHandle,
    u_int	block,
    u_int	count)
{
    FlashInfo	*flash = (FlashInfo *) flashHandle;
    int		rc;
    int         i,j;
    u_int       wear;
    u_int       sector;

    for (i = block; i < block + count; i++) {
        rc = Flash_GetWear(flash, i, &wear);
        if (rc) {
            goto done;
        }
        if (wear >= flash->hdr.wearLimit) {
            rc = 1;
            errno = EIO;
            goto done;
        }
        wear++;
        rc = FlashSetWear(flash, i, wear);
        if (rc) {
            goto done;
        }
        for (j = 0; j < FLASH_SECTORS_PER_BLOCK; j++) {
            sector = (i * FLASH_SECTORS_PER_BLOCK) + j;
            rc = FlashSetState(flash, sector, FLASH_STATE_EMPTY);
            if (rc) {
                goto done;
            }
        }
    }
    rc = 0;
    flash->eraseOps++;
    flash->eraseBlocks += count;
done:
    return rc;
}

int
Flash_Close(
    Flash	flashHandle)
{
    FlashInfo	*flash = (FlashInfo *) flashHandle;
    int		rc;

    if (flash == NULL) {
	rc = 1;
	errno = EINVAL;
	goto done;
    }
    rc = close(flash->fd);
    if (rc) {
	rc = 1;
	goto done;
    }
    if ((flash->flags & FLASH_SILENT) == 0) {
	fprintf(stderr, "Flash read ops: %lld\n", flash->readOps);
	fprintf(stderr, "Flash read sectors: %lld\n", flash->readSectors);
	fprintf(stderr, "Flash write ops: %lld\n", flash->writeOps);
	fprintf(stderr, "Flash write sectors: %lld\n", flash->writeSectors);
    }
    free((void *) flash);
    rc = 0;
done:
    return rc;
}







