# Makefile for the USB input drivers

usbhid-y		:= hid-core.o
obj-$(CONFIG_USB_HID)	+= usbhid.o

# ccflags-y += -Werror

default:
	make -C /usr/src/linux M=${PWD} modules

clean:
	rm -rf *.o *.order *.mod.c *.symvers .[[:alpha:]]* *~

cleanall:
	rm -rf *.o *.ko *.order *.mod.c *.symvers core.* .[[:alpha:]]* *~
