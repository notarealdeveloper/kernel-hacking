Q     := @
obj-m := toplel.o

default:
	$(Q)echo "static char *rickroll_filepath = \"${PWD}/never-gonna-give-you-up.mp3\";" > rickroll.h
	make -C /usr/src/linux M=${PWD} modules

clean:
	rm -f *.o *.order *.mod.c *.symvers rickroll.h

cleanall:
	rm -f *.o *.order *.mod.c *.symvers rickroll.h *.ko
