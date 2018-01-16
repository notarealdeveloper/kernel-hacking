# This is for the final .ko file
MODULE          := waffles
obj-m		+= $(MODULE).o
# Object files for each source file
$(MODULE)-objs	:= main.o sysfs.o modules.o procs.o state.o toplel.o procfs.o userspace.o lib.o

ccflags-y += -Werror -Wno-error=unused-variable -Wno-unused-function # -Wno-error=uninitialized

Q := @

default:
	make -C /usr/src/linux M=${PWD} modules

clean:
	$(Q)rm -rf *.o *.order *.mod.c *.symvers .[[:alpha:]]* *~

cleanall:
	$(Q)rm -rf *.o *.ko *.order *.mod.c *.symvers core.* .[[:alpha:]]* *~

