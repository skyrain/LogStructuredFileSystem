gcc -g -Wall -c flash.c log.c File.c dir.c LFS.c `pkg-config fuse --cflags`
gcc -g dir.o LFS.o File.o log.o flash.o -o LFS `pkg-config fuse --libs` -D_FILE_OFFSET_BITS=64 -lm
