/*
 * dir.c
 * Created on: oct 17, 2013
 * This is the directory layer c code
 */

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include "dir.h"

Inode *ifile; //ifile stores an array of inodes
Inode *inode_ifile; // the inode of ifile;
int ifile_length; //number of files currently held in the ifile;

//Init the Directory layer, as a special file, it needs use
//File_Layer_Init as well, creat the root directory if not there.
//read from the ifile's inode
int Dir_Layer_Init(char *filename, u_int cachesize)
{
	int status = 0;

  	//inode_ifile = (Inode *)calloc(1, sizeof(Inode));
    // Init the File layer as well
	// int normalclose; save for the checkpoint roll forward---???--
    	status = 0;//File_Layer_Init(filename, inode_ifile, cachesize);
	printf("Dir Layer is Initing.\n");
	
	// Init the ifile by calling File_read, and return the ifile to
	// the memory and load it to the memory as well as set the
	ifile = (Inode *)malloc(inode_ifile->filesize);
	status = File_Read(inode_ifile, 0, inode_ifile->filesize, ifile);
	if(status)
	{
		printf("fail to load ifile into memory\n");
		return status;
	}

	ifile_length = inode_ifile->filesize / sizeof(Inode);

	if(ifile_length ==0)
	{
		printf("creat the root directory... \n");
		mode_t mode =S_IFDIR | 0755;
		status = Dir_mkdir("/", mode, getuid(), getgid());
		if(status)
		{
			printf("Make root fail!\n");
			return status;
		}
		else
		{
			printf("Root directory is successful built\n");
		}
	}
		
	return status;
}

// open a directory or file
int Dir_Open_File(const char *path, struct fuse_file_info *fi)
{
	Inode *myNode;
	int status;
	
	status = Get_Inode(path, &myNode);
	if(status) {printf("openning file fail\n"); return status;}

	fi->fh = myNode->ino;

	return status;
}

int Dir_GetAttr(const char *path, struct stat *stbuf)
{
	int status;
	Inode *myNode;

	status = Get_Inode(path, &myNode);
	if(status) {printf("Fail to getattr in get inode\n"); return status;}
	myNode->mode = S_IFDIR | 0755; 
	if(myNode->mode == 0){ printf("Invalid mode in get attr\n"); return -ENOENT;}

	status = GetAttr(myNode, stbuf);
	if(status) {printf("cannot get Getattr \n"); return status;}
}

//mkdir, make a directory
int Dir_mkdir(const char *dir_name, mode_t mode, uid_t uid, gid_t gid)
{
	//create a new directory
	int status;

	if(dir_name[0] != '/')
	{
		printf("It's not a directory name!");
		return -1;
	}
	
	//Make sure the mode show that this is a direcoty
	if(!S_ISDIR(mode)){mode = mode | S_IFDIR;}
	// this is for the directory other than Root
	DirEntry currentDir[2];
    //???
	Inode *dirNode = (Inode *)calloc(1,sizeof(Inode));
	Inode *parentDirNode; //= (Inode *)calloc(1,sizeof(Inode));
	struct fuse_file_info *fi = NULL;
	// Create a directory and get its inode to init
	// this will also add its info to the parent dir
	
	Dir_Create_File(dir_name, mode, uid, gid, fi);
	
	Get_Inode(dir_name, &dirNode);
   
    // ?????????? 
    // dirNode->ino = 0;
	
    // ADd . and .. to this directory
	strcpy(currentDir[1].filename, "..");
	// if this dir is the root dir then..
	if(strcmp(dir_name, "/") ==0 )
	{
		currentDir[1].inum = UNDEFINE_FILE;
	}
	else
	{      
		// No problem here, because Get Dir Inode is 
		// Getting the directory that contains the file/dir specified by path
		status = Get_Dir_Inode(dir_name, &parentDirNode, currentDir[0].filename);
		currentDir[1].inum = parentDirNode->ino;
	}

	// Current directory "."
	strcpy(currentDir[0].filename, ".");
	currentDir[0].inum = dirNode->ino;
	
	// create the new directory.
	status = File_Write(dirNode, 0, 2*sizeof(DirEntry), &currentDir);
	if(status)
	{
		printf("create with error");
		return status;
	}
	
	// flush the content to the file, 
	// Write the inode for the new file to the ifile
	//status = Flush_Ino(dirNode->ino);

	return status;
}

// When create a file or dir in the directory given by path.
int Dir_Create_File(const char *path, mode_t mode, uid_t uid, gid_t gid, struct fuse_file_info *fi){
	// Assume this file does not exist already

	int inum, status;
	time_t t;
	time(&t); // Store current time in t

	if (path[0] != '/'){
		printf("ERROR: invalid path: \n");
		return -EBADF;
	}
	//path++;

	// Check if file exists unless root
	if (strcmp(path, "/") != 0){
		Inode *someNode = (Inode *)calloc(1,sizeof(Inode));
		status = Get_Inode(path, &someNode);
		if (status == 0){
			printf( "File already exists \n");
			return -EEXIST;
		}
	}

	// Get a new inum for this file - just add it to the end and expand the ifile
	// ---???---
	inum = Get_New_Ino();

	if (inum < 0){
		printf("ERROR can't find a good inum.\n");
		return -ENOSPC;
	}

	status = File_Init(&ifile[inum], S_IFREG);
	ifile[inum].ino = inum;
	ifile[inum].mode =  mode;
	ifile[inum].num_links = 1;
	ifile[inum].userID = uid;
	ifile[inum].groupID = gid;

	printf( "Just created a file:\n");

	// add this file to the appropriate directory ---???---
	status = Add_File_To_Directory(path, inum);
	if( status )
	{
		printf("ERROR when add file to Dir:\n");
		return status;
	}

	// Write the inode for the new file to the disk ---???---
	//status = Flush_Ino( inum );
	if( status )
	{
		printf("ERROR when flush the inum:\n");
		return status;
	}

	return status;
}

int Dir_Read_File(const char *path, char *buf, size_t size, off_t offset,
        			struct fuse_file_info *fi){
	// path: path of file to read
	// buf: buffer in which to place file contents
	// size: how much data would you like to read?
	// offset: at what offset would you like to start reading?
	// returns number of bytes read
	Inode *myNode = (Inode *)calloc(1,sizeof(Inode));
	int status;

	status = Validate_Inum(fi->fh, (char *)path);
	if( status = 0 ){
		// Bad Inum, parse the path to get the inode, so myNode is the return Node
		status = Get_Inode(path, &myNode);
		if( status )
		{
			printf("ERROR in dir read file");
			return status;
		}
	}else{
		// Valid inum, get the inode
		status = Get_Inode_From_Inum(fi->fh, &myNode);
		if( status )
		{
			printf("ERROR in dir read file");
			return status;
		}
	}

//	debug_print("Printing Inode\n");
//	printInode(myNode);

	status = File_Read( myNode, offset, size, buf );
	if( status )
	{
		printf("ERROR in dir read file");
		return status;
	}

	status = Flush_Ino( myNode->ino );
	if( status )
	{
		printf("ERROR in dir read file");
		return status;
	}

	return size;
}

int Dir_Read_Dir(const char *path, void *buf, fuse_fill_dir_t filler,
        off_t offset, struct fuse_file_info *fi){
	// Lists all the files in the directory specified by path

	int status, i, numfiles;
	Inode *dirNode;

	if (ifile_length == 0){
		return 0;
	}

	// First, read the directory file from disk
	status = Validate_Inum(fi->fh, (char *) path);
	if( status ){
		// Bad Inum, parse the path to get the inode
		status = Get_Inode(path, &dirNode);
		if( status ) {printf("ERROR in DirReadDir GetInode\n"); return status;}
	}else{
		// Valid inum, get the inode
		status = Get_Inode_From_Inum(fi->fh, &dirNode);
		if( status ) {printf("ERROR in DirReadDir GetIFromInum\n"); return status;}
	}
	DirEntry *dir  = Get_Dir(dirNode, &numfiles);

	printf("Number of files in directory: %i\n", numfiles-2);

	struct stat *stbuf;
	stbuf = (struct stat *) malloc(sizeof(struct stat));
	char name[FILE_NAME_LENGTH];

	for(i = 0; i < numfiles; i++){
		strcpy(name, dir[i].filename);

		// fill information into stbuf
		if (dir[i].inum != UNDEFINE_FILE){
			// check to be sure this entry exists:  .. will not have a valid inum
			status = GetAttr(&ifile[dir[i].inum], stbuf);
			if( status )
			{printf("ERROR in DirReadDir FillAttr\n"); return status;}
		}else{
			memset(stbuf, 0, sizeof(struct stat));
		}

		// let fuse put stbuf information into buf
		printf("filename '%s', inum %i\n", name, dir[i].inum);
		status = filler(buf, name, stbuf, offset);
		if( status )
		{printf("ERROR in DirReadDir Filler"); return status;}
	}

	time( &inode_ifile->access_Time );
	free(stbuf);
	return 0;
}


int Get_Inode(const char *path, Inode **returnNode){
	// Get the inode for the path If the path is invalid, throw an error
	// DirNode is inode ** so it to pass back an address of an inode
	//returnNode = (Inode **)calloc(1,sizeof(Inode *));
	int i;
	char filename[FILE_NAME_LENGTH]; 
	const char *subpath; 	// will point to the "part of the path" that has not yet been processed
	char *breakpath; // holds the "location" of the next '/' in the subpath
	// Check to make sure this is a valid path
	if (path[0] != '/'){
		printf("ERROR: invalid path \n");
		return -ENOENT;
	}else if (strcmp(path, "/") == 0){
		// This is root!
		*returnNode = &ifile[ROOT_INUM];
		return 0;
	}

	subpath = &path[1]; // start the subpath after the first '/'
	breakpath = (char *) subpath;

	// If there aren't any inodes in the ifile, return an error
	if (ifile_length == 0){
		printf("ERROR: No inodes in ifile.\n");
		return -ENOENT;
	}

	// First, read the directory file from disk
	//int inum = ROOT_INUM;
	// ---???--- phase 2: search sub directory
	int numfiles;
	Inode *dirNode;// = (Inode *)calloc(1,sizeof(Inode));
	dirNode = &ifile[ROOT_INUM];
	DirEntry *dir  = Get_Dir(dirNode, &numfiles);
	if (dir == NULL){
		printf("ERROR fail to get dir\n");
		return -ENOENT;
	}
	Inode *myNode = dirNode; // so that it return the right thing for root


	while(breakpath){ //strlen returns the number of chars before /0
		// determine where the next '/' is in the subpath
		// strchr return the pointer of location in subpath where the "/" show up in the first time
		breakpath = strchr( subpath,  '/');

		if (breakpath != NULL){
		// if there is '/' in subpath, pull out chars before as the next
		// dirname to find that mean this is a directory
			if (breakpath - subpath > FILE_NAME_LENGTH){
				printf("ERROR: invalid subpath:\n");
				return -ENOENT;
			}
			strncpy(filename, subpath, breakpath - subpath);
			filename[breakpath - subpath] = '\0';
			// look up new dirname in current dir, set dirNode and dir to that dir
		}else{
		// else use remaining chars as the filename, find the file, and
		// return it inode
			if (strlen(subpath) > FILE_NAME_LENGTH){
				printf("ERROR: invalid subpath: \n");
				return -ENOENT;
			}
			strcpy(filename, subpath);
		}
		// find filename in current directory
		i = 0;
		while (i < numfiles && strcmp(dir[i].filename, filename) != 0){
			i++;
		}
		if (i < numfiles && dir[i].inum >= 0 && dir[i].inum < ifile_length){
			// file found!
			myNode = &ifile[dir[i].inum];
			if (breakpath)
			{
				// expect this to be a directory
				if S_ISDIR(myNode->mode)
				{
					// this is a directory
					dirNode = myNode;
					dir  = Get_Dir(dirNode, &numfiles);
					if (dir == NULL)
					{
						printf("ERROR\n");
						return -ENOENT;
					}
				}
				else
				{
					printf("File found is not a directory.\n");
					return -ENOENT;
				}
			}
		}
		else
		{
			return -ENOENT;
		}
		// Update subpath to be break path + 1 (move past the '/')
		if(breakpath != NULL)
		{
			subpath = &breakpath[1];
		}
	}

	// returnNode has been initiated.
	*returnNode = myNode;

	time( &inode_ifile->access_Time );

	return 0;
}

// write the corresponding ifile[inum] to inode_ifile or add to it
int Flush_Ino(int inum)
{
	if(inum >= ifile_length)
		return -ENOENT;
   
    void * bf = calloc(1, sizeof(Inode));
    memcpy(bf, &ifile[inum], sizeof(Inode));
    return File_Write(inode_ifile, inum*sizeof(Inode), sizeof(Inode), bf);
}

int Add_File_To_Directory(const char *path, int inum)
{
	//Add the file of this path to the directory.
	int status;
	Inode *dirNode = (Inode *)calloc(1,sizeof(Inode));
	DirEntry currentFile;
	status = Get_Dir_Inode(path, &dirNode, currentFile.filename);
	if(status)
	{
		printf("fail to get a inode for this dir\n");
		return status;
	}

	currentFile.inum = inum;
	status =  Write_file(dirNode, (const char *)&currentFile, sizeof(DirEntry), dirNode->filesize);

	// check whether write the file correctly, by size
	if(status != sizeof(DirEntry))
	{
		printf("error! when to add the file to dir");
		if(status < 0)
		{
			printf("error! when to add the file to dir\n"); 
			return status;
		}
		else
		{
			return -1;
		}
	}
	
	return 0;
}

int Dir_Write_File(const char *path, const char *buf, size_t size, off_t offset,
			struct fuse_file_info *fi){
	// path: path of file to write
	// buf: buffer from which to weite file contents
	// size: how much data would you like to write?
	// offset: at what offset would you like to start writing?
	// returns the number of bytes written
	Inode *myNode = (Inode *)calloc(1,sizeof(Inode));
	int status;

//	status = _Get_Inode(path, &myNode);
//	if( status ){
//		return status;
//	}

	status = Validate_Inum(fi->fh, (char *) path);
	if( status ){
		// Bad Inum, parse the path to get the inode
		status = Get_Inode(path, &myNode);
		if( status )
		{
			printf("Write file failure \n");
			return status;
		}
	}else{
		// Valid inum, get the inode
		status = Get_Inode_From_Inum(fi->fh, &myNode);
		if( status )
		{
			printf("Write file failure \n");
			return status;
		}
	}

	status = Write_file(myNode, buf, size, offset);
	if( status != size )
	{
		printf("Write file failure \n");
		return status;
	}

	return size;
}

// delete some part of the file, shrink the size of file ---???---
// phase 2, to handle the indirect block
int Dir_Truncate_File(const char *path, off_t offset)
{
	Inode *myNode;
	int status;
	
	status = Get_Inode(path, &myNode);
	if(status){ printf("delete file in Dir\n"); return status;}

	if(offset < myNode->filesize)
	{
		status = File_Truncate(myNode, offset);
	}
	else if(offset > myNode->filesize)
	{
		char buf = '\0';
		status = File_Write(myNode, offset -1, 1, &buf);
		
		if(status)
		{
			printf("fail to truncate the file\n");
			return status;
		}
	}

	return status;
}

// Delete the file specified by path. only file not dir ---???--- phase 2
int Dir_Delete_File(const char *path)
{
	Inode *fileNode, *dirNode;
	char filename[FILE_NAME_LENGTH];
	int status;

	status = Get_Inode(path, &fileNode);
	if(status) { return status;}	
	status = Get_Dir_Inode(path, &dirNode, filename);
	if(status) { return status;}
	
	// only delete the file from the LFS
	if(fileNode->num_links == 1 )
	{
		status = File_Free(fileNode);
		if(status) { printf(" error when use file free in dir delete\n"); return status;}

	}
	else
	{
		printf("error in dir delete file \n");
		return -ENOENT;
	}

	return 0;
}

void Dir_Layer_Destroy()
{
	File_Layer_Destroy();
	free(ifile);
}

int Write_file(Inode *myNode, const char *buf, size_t size, off_t offset)
{
	// path: path of the file to write
	// buf: buffer from which to write file contents
	// size: how much data would like to write
	// offset: start point
	//
	// return the number of bytes written
	
	int status;
	status = File_Write(myNode, offset, size, buf);
	status = Flush_Ino(myNode->ino);
	if(status)
	{
		printf("Error for write\n");
		return status;
	}

	return size;
}

int Get_Dir_Inode(const char *path, Inode **returnNode, char *filename){
    // Gets the directory that contains the file/dir specified by path
    returnNode = (Inode **)calloc(1,sizeof(Inode *));
    int status = 0;  
    char *breakpath; // holds the location of the last '/' in the subpath                              
    // find the last occurence of a /                                                                  
    breakpath = strrchr( path,  '/');

    if (breakpath == path){                                                                            
        // This is root   
        *returnNode = &ifile[ROOT_INUM];
        if (strlen(&path[0])> FILE_NAME_LENGTH){
            return -EBADF; // Error bad file descriptor
        }        
        strcpy(filename, &path[0]);                                                                
    }else if (breakpath){             
        // Not root, make _Get_Inode do its job                                                    
        char *dirpath;   
        dirpath = (char *) malloc(breakpath - path + 1);
        memset(dirpath, '\0', breakpath - path + 1); 
        strncpy(dirpath, path, breakpath-path);
        printf("path is '%s' and breakpath is '%s', and breakpath - path is %i\n",            
                path, breakpath, breakpath - path);
        printf("Getting inode for '%s'\n", dirpath);                                          
        status = Get_Inode((const char *) dirpath, returnNode);
        if( status )              
        {        
            printf("ERROR: .\n");                                  
            return status;    
        }        
        if (strlen(&breakpath[1])> FILE_NAME_LENGTH){
            return -EBADF; // Error bad file descriptor                                        
        }        
        strcpy(filename, &breakpath[1]);  
        free(dirpath);
    }else{           
        // path contains no '/' and is thus invalid
        return -ENOENT;   
    }


    return status;   
}

DirEntry *Get_Dir(Inode *dirNode, int *numfiles){
    // Return the directory file for the given directory inode
    int status;

	DirEntry *dir;
	dir = (DirEntry *) malloc(dirNode->filesize);

	status = File_Read( dirNode, 0, dirNode->filesize, dir );
	if( status ){
		printf("Error in _get_Dir.\n");
		return (DirEntry *) NULL;
	}

	*numfiles = dirNode->filesize / sizeof(DirEntry);
	return dir;
}

int Get_Inode_From_Inum(int inum, Inode **returnNode){
	// This function is provided as a courtesy to lfsck
	if (inum < ifile_length && inum >= 0){
		*returnNode = &ifile[inum];
		return 0;
	}
	return -ENOENT;
}

int Get_New_Ino(){
	// Used for creating a file.  Returns an unused inumber.
	int inum, status;

	// Check for an available inumber
	inum = 0;

	while(inum < ifile_length && ifile[inum].mode != 0){
		printf("Mode for inode %i is %i.\n", inum, ifile[inum].mode);
		inum++;
	}

	if(inum == ifile_length){
		// no existing inodes are available, expand ifile
		status = Expand_Ifile(inum + 1 - ifile_length);
		if( status )
		{
			printf("error in expand");
			return status;
		}
	}

	printf("Returning inum %i.  Ifile length is %i.\n", inum, ifile_length);
	return inum;
}

int Validate_Inum(int inum, char *path){
	// This functions takes a path of a file and an inum that is
	// presumed to be correct for that file and validates correctness
	// This is for use when the inum is set by Dir_Open_File then
	// passed through to other functions

	if (inum < 0){
		return -ENOENT;
	}
	if (inum > ifile_length){
		return -ENOENT;
	}
	if (inum == ROOT_INUM && strcmp(path, "/") != 0){
		return -ENOENT;
	}
	return 0;
}

int Expand_Ifile(int n){
	// Make the ifile hold n more inodes

	Inode *new_ifile = calloc( 1,(ifile_length + n)*sizeof(Inode) );
	memcpy(new_ifile, ifile, (ifile_length)*sizeof(Inode) );
	free(ifile);
	ifile = new_ifile;

	printf( "ifile_length= %i, new inodes= %i\n", ifile_length, n);
	ifile_length += n;
//	status = Flush_Ino( ifile_length - n, n );
//	printf( "Flush_Ino status: %i\n", status );
//	if( status ){
//		return status;
//	}

	return 0;
}

// Set Inode attr into buffer 
int GetAttr(Inode *myNode, struct stat *stbuf)
{
	memset(stbuf, '0', sizeof(struct stat));
	
	stbuf->st_mode = myNode->mode;
	stbuf->st_size = myNode->filesize;
	stbuf->st_gid = myNode->groupID;
	stbuf->st_uid = myNode->userID;
	stbuf->st_nlink = myNode->num_links;
	stbuf->st_ino = myNode->ino;

	stbuf->st_mtime = myNode->modify_Time;
	stbuf->st_atime = myNode->access_Time;
	stbuf->st_ctime = myNode->change_Time;

	return 0;

}
