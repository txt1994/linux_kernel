#
# Makefile for the linux networking.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definition is now in the main makefile...

# only these two lines should need to be changed to remove inet sockets.
# (and the tcp/tcpip.o in net.o)

SUBDIRS		=# tcp
SOCK_FLAGS	=# -DINET_SOCKETS

.c.o:
	$(CC) $(CFLAGS) $(SOCK_FLAGS) -c $<
.s.o:
	$(AS) -o $*.o $<
.c.s:
	$(CC) $(CFLAGS) -S $<

OBJS	=  socket.o unix.o 

net.o: $(OBJS) subdirs
	$(LD) -r -o net.o $(OBJS)# tcp/tcpip.o


subdirs: dummy
	for i in $(SUBDIRS); do (cd $$i && echo $$i && $(MAKE)) || exit; done

clean:
	rm -f core *.o *.a tmp_make
	for i in *.c;do rm -f `basename $$i .c`.s;done

dep:
	sed '/\#\#\# Dependencies/q' < Makefile > tmp_make
	for i in *.c;do $(CPP) -M $$i;done >> tmp_make
	cp tmp_make Makefile
	@for i in $(SUBDIRS); do (cd $$i && echo $$i && $(MAKE) dep) || exit; done

dummy:

### Dependencies:
socket.o : socket.c /usr/src/linux/include/linux/signal.h /usr/src/linux/include/linux/errno.h \
  /usr/src/linux/include/linux/sched.h /usr/src/linux/include/linux/head.h /usr/src/linux/include/linux/fs.h \
  /usr/src/linux/include/linux/limits.h /usr/src/linux/include/linux/wait.h /usr/src/linux/include/linux/types.h \
  /usr/src/linux/include/linux/dirent.h /usr/src/linux/include/linux/vfs.h /usr/src/linux/include/linux/minix_fs_i.h \
  /usr/src/linux/include/linux/ext_fs_i.h /usr/src/linux/include/linux/msdos_fs_i.h \
  /usr/src/linux/include/linux/minix_fs_sb.h /usr/src/linux/include/linux/ext_fs_sb.h \
  /usr/src/linux/include/linux/msdos_fs_sb.h /usr/src/linux/include/linux/mm.h \
  /usr/src/linux/include/linux/kernel.h /usr/src/linux/include/linux/time.h /usr/src/linux/include/linux/param.h \
  /usr/src/linux/include/linux/resource.h /usr/src/linux/include/linux/vm86.h \
  /usr/src/linux/include/linux/stat.h /usr/src/linux/include/linux/socket.h /usr/src/linux/include/linux/fcntl.h \
  /usr/src/linux/include/linux/termios.h /usr/src/linux/include/asm/system.h /usr/src/linux/include/asm/segment.h \
  kern_sock.h socketcall.h 
unix.o : unix.c /usr/src/linux/include/linux/signal.h /usr/src/linux/include/linux/sched.h \
  /usr/src/linux/include/linux/head.h /usr/src/linux/include/linux/fs.h /usr/src/linux/include/linux/limits.h \
  /usr/src/linux/include/linux/wait.h /usr/src/linux/include/linux/types.h /usr/src/linux/include/linux/dirent.h \
  /usr/src/linux/include/linux/vfs.h /usr/src/linux/include/linux/minix_fs_i.h \
  /usr/src/linux/include/linux/ext_fs_i.h /usr/src/linux/include/linux/msdos_fs_i.h \
  /usr/src/linux/include/linux/minix_fs_sb.h /usr/src/linux/include/linux/ext_fs_sb.h \
  /usr/src/linux/include/linux/msdos_fs_sb.h /usr/src/linux/include/linux/mm.h \
  /usr/src/linux/include/linux/kernel.h /usr/src/linux/include/linux/time.h /usr/src/linux/include/linux/param.h \
  /usr/src/linux/include/linux/resource.h /usr/src/linux/include/linux/vm86.h \
  /usr/src/linux/include/linux/errno.h /usr/src/linux/include/linux/string.h /usr/src/linux/include/linux/stat.h \
  /usr/src/linux/include/linux/socket.h /usr/src/linux/include/linux/un.h /usr/src/linux/include/linux/fcntl.h \
  /usr/src/linux/include/linux/termios.h /usr/src/linux/include/asm/system.h /usr/src/linux/include/asm/segment.h \
  kern_sock.h 
