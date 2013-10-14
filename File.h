/*
 * File.h
 * Author: Xudong Weng
 * Created on: Oct 11, 2013
 * This is the file layer header
 */

#include <sys/types.h>
#include "log.h"

/*
 * inode is the inode of the file to be accessed, 
 * offset is the starting offset of the I/O in bytes,
 * length is the length of the I/O in bytes.
 */

//temp fun
//int Log_Init(char *filename, Inode **ifile, int *closestatus);

int File_Init(Inode *inode, int type);

int File_Write(Inode *inode, int offset, int length, void *buffer);

int File_Read(Inode *inode, int offset, int length, void *buffer);

int File_Free(Inode *inode);

int File_Layer_Init(char *filename, Inode **ifile, int *closestatus);

void Get_Block(Inode *inode, int ReadBlockNumber, Block_pointer *blockpointer);
