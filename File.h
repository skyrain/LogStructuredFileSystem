/*
 * File.h
 * Created on: Oct 11, 2013
 * This is the file layer header
 */

//#include <sys/types.h>
#include "log.h"
#include <sys/types.h>

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

int File_Layer_Init(char *filename, Inode **ifile, u_int cachesize);

void Get_Block_pointer(Inode *inode, int ReadBlockNumber, Block_pointer *blockpointer);
