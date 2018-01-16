obj-m += tutorial.o
ccflags-y += -Werror -Wno-error=unused-variable -Wno-error=unused-function
Q := @

default:
	make -C /lib/modules/$(shell uname -r)/build M=${PWD} 

clean:
	$(Q)rm -rf *.o *.order *.mod.c *.symvers .[[:alpha:]]*

cleanall:
	$(Q)rm -rf *.o *.order *.mod.c *.symvers .[[:alpha:]]* *.ko
