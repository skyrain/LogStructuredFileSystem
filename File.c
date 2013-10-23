/*
 * File.c
 * Created on: oct 11, 2013
 * This is the file layer c code
 */

#include <string.h> 
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "File.h"
#include "log.h"
#include <errno.h>

#define FREE_BLOCK_NUM -1
#define READ_ERROR -2
// temp
#define BLOCK_SIZE bk_content_size

//extern Log_Read(u_int disk_seg, u_int disk_bk_no, u_int length, void *buffer); 

int File_Init(Inode *Ino, int type)
{	
	//Init file time and filesize
	time_t t;     
        time(&t);     
        Ino->filesize = 0;        
        Ino->access_Time = t;  
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
	int status = 0;

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
	int MaxBlocks = DIRECT_BK_NUM; // phase 2 : + BlockSize_byte/sizeof(Block_pointer);
	if(ReadEndBlock >= MaxBlocks)
		return READ_ERROR;
	
	// init temp buffer
	void *ReadBuffer = calloc(NumBlocks, BlockSize_byte); 
	void *ReadPointer = ReadBuffer;

	// read from log, set the disk addr to read from.
	Block_pointer blockPointer;
	LogAddress LogAddr;
	int ReadBlockNumber;
	for(ReadBlockNumber = ReadStartBlock; ReadBlockNumber <= ReadEndBlock; ReadBlockNumber++)
	{
		//get pointer to the first block;
		//read block by block to the ReadBuffer;
		//get one of the four direct block from the Inode. and cpy to the Block_pointer
		Get_Block_pointer(Ino, ReadBlockNumber, &blockPointer);
		LogAddr.seg_no = blockPointer.bk_no;
		LogAddr.bk_no = blockPointer.seg_no;
		if(blockPointer.seg_no != FREE_BLOCK_NUM && blockPointer.bk_no != FREE_BLOCK_NUM)
		{
			status = Log_Read(&LogAddr, BlockSize_byte, ReadPointer);// if read more than one segment, I cant read if the segment is the last segment in log
			if(status)
			{
				return status;
				printf("Log_Read fail\n");
			}

			printf("Reading the file %i: \n", Ino->ino);
		}
		else
		{
			printf("Cannot read %i because the block %i is not Init \n", Ino->ino, ReadBlockNumber);
		}
		// append the content to the init buffer
		ReadPointer += BlockSize_byte;
	}
	// fill in the final buffer which contain the result
	// , update inode and free the temp buffer.
	memcpy(buffer, ReadBuffer+InBlockStart,length);
	time(&Ino->access_Time);	
	free(ReadBuffer);
	
	return status;
}

int File_Write(Inode *Ino, int offset, int length, void *buffer)
{
	// length to be written in bytes
	
	int status = 0;
	int BlockSize_byte = BLOCK_SIZE;

	// next define where start to write, if offset out of range
	// write at the last block of the file
	int fileEndBlock = Ino->filesize / BlockSize_byte;
	if(Ino->filesize % BlockSize_byte == 0 )
		fileEndBlock --;

	int writeStartBlock;
	if(Ino->filesize < offset)
		writeStartBlock = fileEndBlock;
	else
		writeStartBlock = offset / BlockSize_byte;

	// next, define which block end the write and from which byte
	// start to write at the writeStartBlock 
	int innerBlockStartOffset = offset % BlockSize_byte;
	int writeEndBlock = (offset + length) / BlockSize_byte;

	if((offset + length) % BlockSize_byte == 0)
		writeEndBlock--;

	// deal with some write error such as out of range.
	// numBlocks is how many blocks going to write
	int numBlocks = writeEndBlock - writeStartBlock + 1;
	int maxNumBlocks = DIRECT_BK_NUM; // phase 2 + indirect
	if(writeEndBlock >= maxNumBlocks)
		return -EFBIG;

	void *writeBuffer = calloc(numBlocks, BlockSize_byte);
	
	// start to copy part of blocks of data into the temp buffer, at first we need to 
	// read some orginal data from the disk. start at writeStartBlock to readEndBlock
	LogAddress firstBlockAddr;
	LogAddress lastBlockAddr;
	Block_pointer blockPointer;
	if( fileEndBlock >= writeStartBlock)
	{
		int readEndBlock;
		if(fileEndBlock > writeEndBlock)
			readEndBlock = writeEndBlock;
		else
			readEndBlock = fileEndBlock;
		
		//set a pointer to the first block to write
		//Block_pointer blockPointer;
		Get_Block_pointer(Ino, writeStartBlock, &blockPointer);
		firstBlockAddr.seg_no = blockPointer.seg_no;
		firstBlockAddr.bk_no = blockPointer.bk_no;
		// read the block to write buffer
		status = Log_Read(&firstBlockAddr, BlockSize_byte, writeBuffer);
		if(status)
		{
			printf("error, fail to read the first block the file in log\n");
			return status;
		}

		if(numBlocks > 1)
		{
			// set a pointer to the last block
			Get_Block_pointer(Ino, readEndBlock, &blockPointer);
			lastBlockAddr.seg_no = blockPointer.seg_no;
			lastBlockAddr.bk_no = blockPointer.bk_no;
			status = Log_Read(&lastBlockAddr, BlockSize_byte, writeBuffer + (numBlocks-1)*BlockSize_byte);
			if(status)
			{
				printf("error, fail to read the file's last block in log \n");
				return status;
			}
		}
	}
	
	// overwrite the file,  write start at innerBlockStartOffset, with length from the buffer
	memcpy(writeBuffer + innerBlockStartOffset, buffer, length);

	void *writePointer = calloc(1, BlockSize_byte);

	// Log_Write of block one by one and buffer one by one.
	int writeBlock;
	LogAddress AddrWrite;
	LogAddress *tailaddr;
	//Block_pointer blockPointer;
	for(writeBlock = 0; writeBlock < numBlocks; writeBlock ++ )
	{ 	
		// write the orginal file which file block number is AddrWrite.bk_no
		// to the tail of the segment
		Get_Block_pointer(Ino, writeBlock, &blockPointer);
		AddrWrite.seg_no = blockPointer.seg_no;
		AddrWrite.bk_no = blockPointer.bk_no;
		tailaddr = tail_log_addr;
		memcpy(writePointer, writeBuffer + BlockSize_byte*writeBlock, BlockSize_byte);
		status  = Log_Write(Ino->ino, AddrWrite.bk_no, BlockSize_byte, writePointer, tailaddr);
		if(status)
		{
			printf("fail to write on the log, Log_Write\n");
			return status;
		}
	}

/*--------------------------------------------change seg usage table
	Block_pointer *blockBuffer = NULL;
	int i;
	for(i = 0; i < numBlocks; i++)
	{
		int fileBlockNum = writeStartBlock + i;
		if(fileBlockNum < DIRECT_BK_NUM)
		{
			if(Ino->direct_bk[fileBlockNum].seg_no != FREE_BLOCK_NUM)
				//update segment usage table ---???---
		}
		// else read from the indirect block , phase 2 ---???---
	}
------------------------------------------------------------------*/

	//update inode, File layer tought we  write something on the disk, so it should update
	// the disk locations into inode ---???---
	if(length + offset > Ino->filesize)
	{
		Ino->filesize = offset + length;
	}
	
	int i;
	for(i=0; i < numBlocks; i++)
	{
		int fileBlockNum = writeStartBlock + i;

		if( fileBlockNum < DIRECT_BK_NUM)
		{
			Ino->direct_bk[fileBlockNum].seg_no = tailaddr->seg_no;
			Ino->direct_bk[fileBlockNum].bk_no = tailaddr->bk_no;
		}
		// else , indirect block phase 2 ---???---
		
	}

	time_t t;
	time(&t);
	Ino->access_Time = t;
	Ino->change_Time = t;

	free(writeBuffer);

	return status;

}
	
int File_Free(Inode *Ino)
{
	printf("Freeing the file %i\n", Ino->ino);

	int status = 0;

	//status = File_Drop(Ino, 0);
	if(status)
	{
		printf("ERROR: when dropping the file");
		return status;
	}

	memset(Ino, 0, sizeof(Inode));

	return 0;

}

/*
int File_Drop(Inode *Ino, int offset)
{
	Block_pointer *BlockBuffer = NULL;
	Block_pointer *BlockPointer;
	int BlockSize_byte = BLOCK_SIZE;
	int FileBlockNumber;
	int FirstBlockDrop;
	int PartialBlockLeft;

	int status = 0;

	if(offset > Ino->filesize)
	{
		printf("Drop beyond the range of file! \n");
		return status;
	}

	double BlockNum = offset / BlockSize_byte;
	FirstBlockDrop = ceil(BlockNum);
	PartialBlockLeft = floor(BlockNum);


	//decide which file are going to drop.
	if(PartialBlockLeft * BlockSize_byte != offset)
	{

	}
}
*/


// Init the file layer, ready to read and write the file from the info 
// of ifile.
int File_Layer_Init(char *filename, Inode **ifile, u_int cachesize)
{
	// filename is the flash file that are going to open, these argument are passed by user, layer to layer.
	// then we need to read the ifile from the checkpoint to the
	// memory, please return like:*ifile = &(Log.checkPoint.ifile)
	// in phase 2, we need add more argument for lfs [option], 
	// such as cachesize and the interval of checkpoint time.
	return Log_Init(filename, ifile, cachesize /* other arguement in phase 2*/);
}

//get one of the four direct block from the Inode. and cpy to the Block_pointer.
void Get_Block_pointer(Inode *Ino, int BlockNumber, Block_pointer *Block_pointer)
{
	// beyond the current file
	if(BlockNumber * BLOCK_SIZE > Ino->filesize  )
	{
		Block_pointer->seg_no = FREE_BLOCK_NUM;
		Block_pointer->bk_no = FREE_BLOCK_NUM;
	}

	if(BlockNumber < DIRECT_BK_NUM)
	{ 
		memcpy(Block_pointer, Ino->direct_bk + BlockNumber, sizeof(Block_pointer));
	}
	else
	{
		printf("BlockNumber out of DIRECT_BK_NUM\n");
		return;
	}
}
