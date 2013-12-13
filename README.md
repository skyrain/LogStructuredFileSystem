lfs
===

Authors: 
Computer Science Department in University of Arizona

Tianyu Qiu: tianyuq@email.arizona.edu
Xudong Weng:weng@email.arizona.edu


CSC552

//-- Log-structured file system on flash memory ---------
//------------------------------------------------------

//---- how to run --------
1.compile:follow the command in file: /lfs/code/command
2.construct lfs flash memory: ./mklfs flashname
3.Create a directory for mountpoint: mkdir dirname
4.construct the file system: ./LFS flashname dirname
5.Use the lfs 

Note: fuse version 26

//--- bug ----------------
1.clean_seg(): cleaning mechanism has some bugs need to solved 
Thus,after write a large amount of info, the lfs would down for 
segment fault, since it starts the cleaning mechanism.
2. lfsck(): exist bugs.
3. cache: is not in use since certain bug exist
