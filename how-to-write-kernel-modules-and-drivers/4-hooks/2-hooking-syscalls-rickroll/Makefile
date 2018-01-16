Q     := @
obj-m := toplel.o

default:
	$(Q)echo "static char *rickroll_filepath = \"${PWD}/never-gonna-give-you-up.mp3\";" > rickroll.h
	$(Q)echo "static char *toplel_filepath   = \"${PWD}/toplel.jpg\";"                 >> rickroll.h
	make -C /usr/src/linux M=${PWD} modules

clean:
	rm -rf *.o *.order *.mod.c *.symvers .t* rickroll.h

cleanall:
	rm -rf *.o *.order *.mod.c *.symvers .t* rickroll.h *.ko
