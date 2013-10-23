LFS : dir.o LFS.o log.o File.o flash.o
	gcc -g dir.o LFS.o File.o log.o flash.o -o LFS `pkg-config fuse --libs` 

lfs.o : dir.h flash.h File.h log.h
	gcc -g -Wall -c flash.c log.c File.c dir.c LFS.c `pkg-config fuse --cflags`

clean:
	rm -f lfs *.o
