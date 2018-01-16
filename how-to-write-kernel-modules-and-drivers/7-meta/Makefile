# This is for the final .ko file
obj-m		+= waffles.o
# Object files for each source file
waffles-objs	:= main.o sysfs.o modules.o procs.o state.o toplel.o procfs.o

ccflags-y += -Werror -Wno-error=unused-variable -Wno-unused-function

Q := @

default:
	make -C /usr/src/linux M=${PWD} modules

clean:
	$(Q)rm -rf *.o *.order *.mod.c *.symvers .[[:alpha:]]* *~

cleanall:
	$(Q)rm -rf *.o *.ko *.order *.mod.c *.symvers core.* .[[:alpha:]]* *~

