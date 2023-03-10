#
# Makefile for the kernel math emulation routines
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

AR	=ar
AS	=as
LD	=ld
LDFLAGS	=-s -x
CC	=gcc -nostdinc -I../../include
CPP	=cpp -nostdinc -I../../include

.c.s:
	$(CC) $(CFLAGS) $(MATH_EMULATION) \
	-S -o $*.s $<
.s.o:
	$(AS) -c -o $*.o $<
.c.o:
	$(CC) $(CFLAGS) $(MATH_EMULATION) \
	-c -o $*.o $<

OBJS  = emulate.o error.o convert.o ea.o get_put.o \
	add.o mul.o div.o compare.o

math.a: $(OBJS)
	$(AR) rcs math.a $(OBJS)
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
add.s add.o : add.c ../../include/linux/math_emu.h ../../include/linux/sched.h \
  ../../include/linux/head.h ../../include/linux/fs.h \
  ../../include/sys/types.h ../../include/linux/mm.h \
  ../../include/linux/kernel.h ../../include/signal.h \
  ../../include/sys/param.h ../../include/sys/time.h ../../include/time.h \
  ../../include/sys/resource.h 
compare.s compare.o : compare.c ../../include/linux/math_emu.h \
  ../../include/linux/sched.h ../../include/linux/head.h \
  ../../include/linux/fs.h ../../include/sys/types.h ../../include/linux/mm.h \
  ../../include/linux/kernel.h ../../include/signal.h \
  ../../include/sys/param.h ../../include/sys/time.h ../../include/time.h \
  ../../include/sys/resource.h 
convert.s convert.o : convert.c ../../include/linux/math_emu.h \
  ../../include/linux/sched.h ../../include/linux/head.h \
  ../../include/linux/fs.h ../../include/sys/types.h ../../include/linux/mm.h \
  ../../include/linux/kernel.h ../../include/signal.h \
  ../../include/sys/param.h ../../include/sys/time.h ../../include/time.h \
  ../../include/sys/resource.h 
div.s div.o : div.c ../../include/linux/math_emu.h ../../include/linux/sched.h \
  ../../include/linux/head.h ../../include/linux/fs.h \
  ../../include/sys/types.h ../../include/linux/mm.h \
  ../../include/linux/kernel.h ../../include/signal.h \
  ../../include/sys/param.h ../../include/sys/time.h ../../include/time.h \
  ../../include/sys/resource.h 
ea.s ea.o : ea.c ../../include/stddef.h ../../include/linux/math_emu.h \
  ../../include/linux/sched.h ../../include/linux/head.h \
  ../../include/linux/fs.h ../../include/sys/types.h ../../include/linux/mm.h \
  ../../include/linux/kernel.h ../../include/signal.h \
  ../../include/sys/param.h ../../include/sys/time.h ../../include/time.h \
  ../../include/sys/resource.h ../../include/asm/segment.h 
error.s error.o : error.c ../../include/signal.h ../../include/sys/types.h \
  ../../include/linux/sched.h ../../include/linux/head.h \
  ../../include/linux/fs.h ../../include/linux/mm.h \
  ../../include/linux/kernel.h ../../include/sys/param.h \
  ../../include/sys/time.h ../../include/time.h ../../include/sys/resource.h 
get_put.s get_put.o : get_put.c ../../include/signal.h ../../include/sys/types.h \
  ../../include/linux/math_emu.h ../../include/linux/sched.h \
  ../../include/linux/head.h ../../include/linux/fs.h \
  ../../include/linux/mm.h ../../include/linux/kernel.h \
  ../../include/sys/param.h ../../include/sys/time.h ../../include/time.h \
  ../../include/sys/resource.h ../../include/asm/segment.h 
emulate.s emulate.o : emulate.c ../../include/linux/config.h 
mul.s mul.o : mul.c ../../include/linux/math_emu.h ../../include/linux/sched.h \
  ../../include/linux/head.h ../../include/linux/fs.h \
  ../../include/sys/types.h ../../include/linux/mm.h \
  ../../include/linux/kernel.h ../../include/signal.h \
  ../../include/sys/param.h ../../include/sys/time.h ../../include/time.h \
  ../../include/sys/resource.h 
