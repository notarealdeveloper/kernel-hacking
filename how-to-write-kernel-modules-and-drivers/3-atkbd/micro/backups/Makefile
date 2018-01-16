obj-$(CONFIG_KEYBOARD_ATKBD)		+= atkbd.o

# ccflags-y += -Werror

default:
	make -C /usr/src/linux M=${PWD} modules

clean:
	rm -rf *.o *.order *.mod.c *.symvers .[[:alpha:]]* *~

cleanall:
	rm -rf *.o *.ko *.order *.mod.c *.symvers core.* .[[:alpha:]]* *~

