obj-m := memory.o

default:
	make -C /usr/src/linux M=${PWD} modules

clean:
	rm -rf *.o *.order *.mod.c *.symvers

cleanall:
	rm -rf *.o *.ko *.order *.mod.c *.symvers core.*

