#
# Makefile for some libs needed in the kernel.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

AR	=ar
AS	=as
LD	=ld
LDFLAGS	=-s -x
CC	=gcc -nostdinc -I../include
CPP	=gcc -E -nostdinc -I../include

.c.s:
	$(CC) $(CFLAGS) \
	-S -o $*.s $<
.s.o:
	$(AS) -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) \
	-c -o $*.o $<

OBJS  = ctype.o _exit.o open.o close.o errno.o write.o dup.o setsid.o \
	execve.o wait.o string.o malloc.o itimer.o

lib.a: $(OBJS)
	$(AR) rcs lib.a $(OBJS)
	sync

clean:
	rm -f core *.o *.a tmp_make
	for i in *.c;do rm -f `basename $$i .c`.s;done

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	(for i in *.c;do echo -n `echo $$i | sed 's,\.c,\.s,'`" "; \
		$(CPP) -M $$i;done) >> tmp_make
	cp tmp_make Makefile

### Dependencies:
_exit.s _exit.o : _exit.c ../include/linux/unistd.h 
close.s close.o : close.c ../include/linux/unistd.h 
ctype.s ctype.o : ctype.c ../include/linux/ctype.h 
dup.s dup.o : dup.c ../include/linux/unistd.h 
errno.s errno.o : errno.c 
execve.s execve.o : execve.c ../include/linux/unistd.h 
itimer.s itimer.o : itimer.c ../include/linux/unistd.h ../include/sys/time.h ../include/time.h \
  ../include/sys/types.h 
malloc.s malloc.o : malloc.c ../include/linux/kernel.h ../include/linux/mm.h ../include/linux/fs.h \
  ../include/sys/types.h ../include/sys/dirent.h ../include/limits.h ../include/sys/vfs.h \
  ../include/signal.h ../include/asm/system.h 
open.s open.o : open.c ../include/linux/unistd.h ../include/stdarg.h 
setsid.s setsid.o : setsid.c ../include/sys/types.h ../include/linux/unistd.h 
string.s string.o : string.c ../include/linux/string.h 
wait.s wait.o : wait.c ../include/linux/unistd.h ../include/sys/wait.h ../include/sys/types.h 
write.s write.o : write.c ../include/linux/unistd.h ../include/sys/types.h 
