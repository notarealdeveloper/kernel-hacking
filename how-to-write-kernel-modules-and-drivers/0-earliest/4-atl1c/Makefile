obj-$(CONFIG_ATL1C) += atl1c.o
atl1c-objs := atl1c_main.o atl1c_hw.o atl1c_ethtool.o

default:
	make -C /usr/src/linux M=${PWD} modules

clean:
	rm -rf *.o *.order *.mod.c *.symvers .[[:alpha:]]* *~

cleanall:
	rm -rf *.o *.ko *.order *.mod.c *.symvers core.* .[[:alpha:]]* *~

