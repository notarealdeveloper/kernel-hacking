obj-m := procfs.o

default:
	make -C /usr/src/linux M=${PWD} modules

clean:
	rm -rf *.o *.ko *.order *.mod.c *.symvers
