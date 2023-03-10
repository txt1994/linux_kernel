#
# Makefile for the linux filesystem.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...

AR	=ar
AS	=as
LD	=ld
CC	=gcc -nostdinc -I../include
CPP	=cpp -nostdinc -I../include

.c.s:
	$(CC) $(CFLAGS) \
	-S -o $*.s $<
.c.o:
	$(CC) $(CFLAGS) \
	-c -o $*.o $<
.s.o:
	$(AS) -o $*.o $<

OBJS=	open.o read_write.o inode.o file_table.o buffer.o super.o \
	block_dev.o char_dev.o stat.o exec.o pipe.o namei.o \
	fcntl.o ioctl.o select.o

fs.o: $(OBJS)
	$(LD) -r -o fs.o $(OBJS)

clean:
	rm -f core *.o *.a tmp_make
	for i in *.c;do rm -f `basename $$i .c`.s;done
	cd minix; make clean

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do $(CPP) -M $$i;done) >> tmp_make
	cp tmp_make Makefile
	cd minix; make dep

### Dependencies:
block_dev.o : block_dev.c ../include/errno.h ../include/linux/sched.h \
  ../include/linux/head.h ../include/linux/fs.h ../include/sys/types.h \
  ../include/sys/dirent.h ../include/limits.h ../include/linux/mm.h \
  ../include/linux/kernel.h ../include/signal.h ../include/sys/param.h \
  ../include/sys/time.h ../include/time.h ../include/sys/resource.h \
  ../include/asm/segment.h ../include/asm/system.h 
buffer.o : buffer.c ../include/stdarg.h ../include/linux/config.h \
  ../include/linux/sched.h ../include/linux/head.h ../include/linux/fs.h \
  ../include/sys/types.h ../include/sys/dirent.h ../include/limits.h \
  ../include/linux/mm.h ../include/linux/kernel.h ../include/signal.h \
  ../include/sys/param.h ../include/sys/time.h ../include/time.h \
  ../include/sys/resource.h ../include/asm/system.h ../include/asm/io.h 
char_dev.o : char_dev.c ../include/errno.h ../include/sys/types.h \
  ../include/linux/sched.h ../include/linux/head.h ../include/linux/fs.h \
  ../include/sys/dirent.h ../include/limits.h ../include/linux/mm.h \
  ../include/linux/kernel.h ../include/signal.h ../include/sys/param.h \
  ../include/sys/time.h ../include/time.h ../include/sys/resource.h \
  ../include/asm/segment.h ../include/asm/io.h 
exec.o : exec.c ../include/signal.h ../include/sys/types.h \
  ../include/errno.h ../include/string.h ../include/sys/stat.h \
  ../include/a.out.h ../include/linux/fs.h ../include/sys/dirent.h \
  ../include/limits.h ../include/linux/sched.h ../include/linux/head.h \
  ../include/linux/mm.h ../include/linux/kernel.h ../include/sys/param.h \
  ../include/sys/time.h ../include/time.h ../include/sys/resource.h \
  ../include/asm/segment.h 
fcntl.o : fcntl.c ../include/string.h ../include/errno.h \
  ../include/linux/sched.h ../include/linux/head.h ../include/linux/fs.h \
  ../include/sys/types.h ../include/sys/dirent.h ../include/limits.h \
  ../include/linux/mm.h ../include/linux/kernel.h ../include/signal.h \
  ../include/sys/param.h ../include/sys/time.h ../include/time.h \
  ../include/sys/resource.h ../include/asm/segment.h ../include/fcntl.h \
  ../include/sys/stat.h 
file_table.o : file_table.c ../include/linux/fs.h ../include/sys/types.h \
  ../include/sys/dirent.h ../include/limits.h 
inode.o : inode.c ../include/string.h ../include/sys/stat.h \
  ../include/sys/types.h ../include/linux/sched.h ../include/linux/head.h \
  ../include/linux/fs.h ../include/sys/dirent.h ../include/limits.h \
  ../include/linux/mm.h ../include/linux/kernel.h ../include/signal.h \
  ../include/sys/param.h ../include/sys/time.h ../include/time.h \
  ../include/sys/resource.h ../include/asm/system.h 
ioctl.o : ioctl.c ../include/string.h ../include/errno.h \
  ../include/sys/stat.h ../include/sys/types.h ../include/linux/sched.h \
  ../include/linux/head.h ../include/linux/fs.h ../include/sys/dirent.h \
  ../include/limits.h ../include/linux/mm.h ../include/linux/kernel.h \
  ../include/signal.h ../include/sys/param.h ../include/sys/time.h \
  ../include/time.h ../include/sys/resource.h 
namei.o : namei.c ../include/linux/sched.h ../include/linux/head.h \
  ../include/linux/fs.h ../include/sys/types.h ../include/sys/dirent.h \
  ../include/limits.h ../include/linux/mm.h ../include/linux/kernel.h \
  ../include/signal.h ../include/sys/param.h ../include/sys/time.h \
  ../include/time.h ../include/sys/resource.h ../include/asm/segment.h \
  ../include/string.h ../include/fcntl.h ../include/errno.h \
  ../include/const.h ../include/sys/stat.h 
open.o : open.c ../include/string.h ../include/errno.h ../include/fcntl.h \
  ../include/sys/types.h ../include/utime.h ../include/sys/stat.h \
  ../include/linux/sched.h ../include/linux/head.h ../include/linux/fs.h \
  ../include/sys/dirent.h ../include/limits.h ../include/linux/mm.h \
  ../include/linux/kernel.h ../include/signal.h ../include/sys/param.h \
  ../include/sys/time.h ../include/time.h ../include/sys/resource.h \
  ../include/asm/segment.h 
pipe.o : pipe.c ../include/signal.h ../include/sys/types.h \
  ../include/errno.h ../include/termios.h ../include/fcntl.h \
  ../include/linux/sched.h ../include/linux/head.h ../include/linux/fs.h \
  ../include/sys/dirent.h ../include/limits.h ../include/linux/mm.h \
  ../include/linux/kernel.h ../include/sys/param.h ../include/sys/time.h \
  ../include/time.h ../include/sys/resource.h ../include/asm/segment.h 
read_write.o : read_write.c ../include/errno.h ../include/sys/types.h \
  ../include/sys/stat.h ../include/sys/dirent.h ../include/limits.h \
  ../include/linux/kernel.h ../include/linux/sched.h ../include/linux/head.h \
  ../include/linux/fs.h ../include/linux/mm.h ../include/signal.h \
  ../include/sys/param.h ../include/sys/time.h ../include/time.h \
  ../include/sys/resource.h ../include/linux/minix_fs.h \
  ../include/asm/segment.h 
select.o : select.c ../include/linux/fs.h ../include/sys/types.h \
  ../include/sys/dirent.h ../include/limits.h ../include/linux/kernel.h \
  ../include/linux/tty.h ../include/termios.h ../include/linux/sched.h \
  ../include/linux/head.h ../include/linux/mm.h ../include/signal.h \
  ../include/sys/param.h ../include/sys/time.h ../include/time.h \
  ../include/sys/resource.h ../include/asm/segment.h ../include/asm/system.h \
  ../include/sys/stat.h ../include/string.h ../include/const.h \
  ../include/errno.h 
stat.o : stat.c ../include/errno.h ../include/sys/stat.h \
  ../include/sys/types.h ../include/linux/fs.h ../include/sys/dirent.h \
  ../include/limits.h ../include/linux/sched.h ../include/linux/head.h \
  ../include/linux/mm.h ../include/linux/kernel.h ../include/signal.h \
  ../include/sys/param.h ../include/sys/time.h ../include/time.h \
  ../include/sys/resource.h ../include/asm/segment.h 
super.o : super.c ../include/linux/config.h ../include/linux/sched.h \
  ../include/linux/head.h ../include/linux/fs.h ../include/sys/types.h \
  ../include/sys/dirent.h ../include/limits.h ../include/linux/mm.h \
  ../include/linux/kernel.h ../include/signal.h ../include/sys/param.h \
  ../include/sys/time.h ../include/time.h ../include/sys/resource.h \
  ../include/linux/minix_fs.h ../include/asm/system.h ../include/errno.h \
  ../include/sys/stat.h 
