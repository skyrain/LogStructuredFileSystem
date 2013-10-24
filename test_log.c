#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "flash.h"
#include "log.h"

int main()
{

    u_int sec_num = 1024;
    u_int seg_size = 64;
    u_int seg_num = 16;
    Flash_Flags flags = FLASH_SILENT;
    //blocks : # of blocks in the flash
    u_int tmp = sec_num / FLASH_SECTORS_PER_BLOCK;
    u_int * blocks = &tmp;
    Flash   flash = Flash_Open("disk", flags, blocks);

    //------read super seg into memory-------------------
    void * buffer = calloc(1, seg_size * FLASH_SECTOR_SIZE);
    Flash_Read(flash, 0, seg_size, buffer);
    //----------for global variables in log.h-----------------------
    Inode * seefile = (Inode *)(buffer + sizeof(Super_seg) + (seg_num - 1) * sizeof(Seg_usage_table) + sizeof(Checkpoint));
   
}
