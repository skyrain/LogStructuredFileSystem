/*
 * File.c
 * Author: Weng
 * Created on: oct 11, 2013
 * This is the file layer c code
 */

#include <string.h>
#include "File.h"
#include "log.h"

#define FREE_BLOCK_NUM -1

int File_Init(Inode *Ino, int type)
{	
	//Init file time and filesize
	time_t t;     
        time(&t);     
        Ino->filesize = 0;        
        Ino->accese_Time = t;  
        Ino->change_Time = t;  
        Ino->create_Time = t;  
        Ino->modify_Time = t;  
 	
	int i;	
	//Init all direct blocks and indirect blocks.
        for(i = 0; i<DIRECT_BK_NUM; i++)
        {
		Ino->direct_bk[i].seg_no = FREE_BLOCK_NUM;
		Ino->direct_bk[i].bk_no = FREE_BLOCK_NUM;
	}
	
	//some problem with the indirect and direct block structure
	Ino->indirect_bk.bk.seg_no = FREE_BLOCK_NUM;
	Ino->indirect_bk.bk.bk_no = FREE_BLOCK_NUM;

 	return 0;

}

int File_Read(Inode *Ino, int offset, int length, void *buffer)
{
	// Ino is the file to read from
	// offset : where to start reading in bytes
	// length : in bytes
	// buffer is where to fill the data in
	int status;

	if (length == 0)
		return status;
	if(Ino->filesize < offset)
		return status;

	// where the file end in block.
	int BlockSize_byte; // !!! undone
	int FileEndBlock = Ino->filesize / BlockSize_byte;
	if(Ino->filesize % BlockSize_byte ==0)
		FileEndBlock--;

	// if the read request beyond the one block size then keep synchronize.
	int ReadEndBlock;
	int FileEndBlock = Ino->filesize / BlockSize_byte; // !!! need to get the block size
	if(Ino->filesize < offset + length)
	{
		ReadEndBlock = FileEndBlock;
		length = Ino->filesize - offset;
	}
	else
	{
		ReadEndBlock = (offset + length) / BlockSize_byte; // where the read end in block
		if((offset + length) % BlockSize_byte == 0) // if just at the end of a block
			ReadEndBlock--;
	}
	
	int ReadStartBlock = offset / BlockSize_byte;
}

int File_Write(Inode *Ino, int offset, int length, void *buffer){}

int File_Free(Inode *Ino){}

int File_Layer_Init(char *filename, Inode **ifile, int *closestatus)
{
	return Log_Init(filename, ifile, closestatus);
}
