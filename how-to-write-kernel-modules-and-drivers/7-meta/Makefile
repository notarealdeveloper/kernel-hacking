# This is for the final .ko file
obj-m		+= waffles.o
# Object files for each source file
waffles-objs	:= main.o sysfs.o modules.o procs.o state.o toplel.o

ccflags-y += -Werror -Wno-error=unused-variable -Wno-unused-function

default:
	make -C /usr/src/linux M=${PWD} modules

clean:
	rm -rf *.o *.order *.mod.c *.symvers .[[:alpha:]]* *~

cleanall:
	rm -rf *.o *.ko *.order *.mod.c *.symvers core.* .[[:alpha:]]* *~

