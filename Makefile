VERSION = 2

MNTDIR = mnt
VDISK = vdisk
BUILD_TYPE ?= debug

CC = gcc

ifeq ($(BUILD_TYPE), release)
CFLAGS = -Wall -std=gnu11 -O2
else
CFLAGS = -Wall -std=gnu11 -Og -g -fsanitize=address -fsanitize=undefined -fsanitize=leak
endif

OBJS = disk.o fs_opt.o fs.c logger.o

all: fuse

debug: cleand init fuse umount
	./fuse -s -f $(MNTDIR)

mount: cleand init fuse umount
	./fuse -s $(MNTDIR)

umount:
	-fusermount -zu $(MNTDIR)

mount_noinit: fuse umount
	./fuse --noinit -s $(MNTDIR)

debug_noinit: fuse umount
	./fuse --noinit -s -f $(MNTDIR)

disk.o: disk.c disk.h

fs_opt.o: fs_opt.c fs_opt.h

logger.o: logger.c logger.h

fuse: $(OBJS)
	$(CC) $(CFLAGS) -o fuse $(OBJS) -DFUSE_USE_VERSION=29 -D_FILE_OFFSET_BITS=64 -lfuse

init:
	mkdir -p $(VDISK)
	echo $(abspath $(VDISK)) > fuse~
	mkdir -p $(MNTDIR)

cleand:
	rm -rf *~
	-umount -l $(MNTDIR)
	rm -rf $(VDISK) $(MNTDIR)

clean: cleand
	rm -rf *.o fuse
