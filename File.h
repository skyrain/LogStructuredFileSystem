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

int File_Init(inode *inode, int type);

int File_Write(inode *inode, int offset, int length, void *buffer);

int File_Read(inode *inode, int offset, int length, void *buffer);

int File_Free(inode *inode);
