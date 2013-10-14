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
#define READ_ERROR -2

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
	//Ino->indirect_bk.seg_no = FREE_BLOCK_NUM; phase 2
	//Ino->indirect_bk.bk_no = FREE_BLOCK_NUM; phase 2

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
	int BlockSize_byte = BLOCK_SIZE;
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
	printf("ReadEndBlock: %i\n", ReadEndBlock);

	// how many blocks a the read request need and where to start in a block
	int ReadStartBlock = offset / BlockSize_byte;
	int InBlockStart = offset % BlockSize_byte;
	int NumBlocks = ReadEndBlock - ReadStartBlock + 1;
	printf("NumBlocks: %i\n", NumBlocks);

	// a read request cannot over the max blocks number a Inode has
	int MaxBlocks = DIRECT_BK_NUM + BlockSize_byte/sizeof(Block_pointer);
	if(ReadEndBlock >= MaxBlocks)
		return READ_ERROR;
	
	// init temp buffer
	void *ReadBuffer = calloc(NumBlocks, BlockSize_byte); 
	void *ReadPointer = ReadBuffer;

	// read from log !!! Log_Read undone!
	Block_pointer BlockPointer;
	int ReadBlockNumber;
	for(ReadBlockNumber = ReadStartBlock; ReadBlockNumber <= ReadEndBlock; ReadBlockNumber++)
	{
		//get pointer to the first block;
		//read block by block to the ReadBuffer;
		Get_Block_pointer(inode, ReadBlockNumber, &Block_pointer);
		if(Block_pointer.seg_no != FREE_BLOCK_NUM && Block_pointer.bk_no != FREE_BLOCK_NUM)
		{
			status = Log_Read(Block_pointer.seg_no,,Block_pointer.bk_no, ReadPointer);
			if(status)
			{
				return status;
				print("Log_Read fail\n");
			}

			printf("Reading the file %i: \n", Ino->ino);
		}
		else
		{
			printf("Cannot read %i because the block %i is not Init \n", Ino->ino, ReadBlockNumber)
		}

		ReadPointer += BlockSize_byte;
	}
	// fill in the buffer, update inode and free the temp buffer.
	memcpy(buffer, ReadBuffer+InBlockStart,length);
	time(&Ino->accese_Time);	
	free(ReadBuffer);
	
	return status;
}

int File_Write(Inode *Ino, int offset, int length, void *buffer){}
	
int File_Free(Inode *Ino){}

int File_Layer_Init(char *filename, Inode **ifile, int *closestatus)
{
	return Log_Init(filename, ifile, closestatus);
}

void Get_Block_pointer(Inode *Ino, int BlockNumber, Block_pointer Block_pointer)
{
	// beyond the current file
	if(BlockNumber * BLOCK_SIZE > Ino.filesize )
	{
		Block_pointer->seg_no = FREE_BLOCK_NUM;
		Block_pointer->bk_no = FREE_BLOCK_NUM;
	}

	if(BlockNumber < DIRECT_BK_NUM)
	{ 
		memcpy(Block_pointer, Ino->Direct_bk + BlockNumber, sizeof(Block_pointer));
	}
	else
	{
		printf("BlockNumber out of DIRECT_BK_NUM\n");
		return;
	}
}
