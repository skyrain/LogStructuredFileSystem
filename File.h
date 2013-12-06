/*
 * File.h
 * Created on: Oct 11, 2013
 * This is the file layer header
 */
#ifndef _FILE_H
#define _FILE_H

//#include <sys/types.h>
#include "log.h"
#include <sys/types.h>

u_int BLOCK_SIZE;


/*
extern struct Block;
extern struct Seg_sum_entry;
extern struct Seg_sum_bk;
extern struct Begin_bk;
extern struct Seg;
extern struct LogAddress;
extern struct Seg_usage_table;
extern struct Block_pointer;
extern struct Inode;
extern struct checkpoint;
extern struct Super_seg;
extern struct Disk_cache;
*/

/*
 * inode is the inode of the file to be accessed, 
 * offset is the starting offset of the I/O in bytes,
 * length is the length of the I/O in bytes.
 */

//temp fun
//int Log_Init(char *filename, Inode **ifile, int *closestatus);

//store the inode 
typedef struct Inode_location
{
    Inode inode;
    //offset in "ifile",
    // there stores the inode for ino
    u_int offset; 
    struct Inode_location * next;

}Inode_location;

int File_Init(Inode *inode, int type);

int File_Write(Inode *inode, int offset, int length, void *buffer);

int File_Read(Inode *inode, int offset, int length, void *buffer);

int File_Free(Inode *inode);

int File_Layer_Init(char *filename, Inode *ifile, u_int cachesize);

int File_Truncate(Inode *myNode, off_t offset);

void File_Layer_Destroy();

void Get_Block_pointer(Inode *inode, int ReadBlockNumber, Block_pointer *blockpointer);

int Decrement_Seg_Usage(int segment, int block);

#endif
