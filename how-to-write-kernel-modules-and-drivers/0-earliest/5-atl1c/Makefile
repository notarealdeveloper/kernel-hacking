obj-$(CONFIG_ATL1C) += atl1c.o
atl1c-objs := atl1c_main.o atl1c_hw.o atl1c_ethtool.o

ccflags-y += -Werror -Wno-error=unused-function -Wunused-function -Wunused-variable #-Wno-error=unused-variable -Wno-error=uninitialized

Q := @

default:
	make -C /usr/src/linux M=${PWD} modules

clean:
	$(Q)rm -rf *.o *.order *.mod.c *.symvers .[[:alpha:]]* *~

cleanall:
	$(Q)rm -rf *.o *.ko *.order *.mod.c *.symvers core.* .[[:alpha:]]* *~

