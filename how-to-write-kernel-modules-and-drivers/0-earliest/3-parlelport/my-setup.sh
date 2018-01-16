#!/bin/bash

[[ $(whoami) != 'root' ]] && echo "Gotta be root, fool." && exit 1

module=parlelport
[[ -e /dev/$module ]] || (mknod /dev/$module c 61 0 && chmod 666 /dev/$module)


if [[ -n $(lsmod | grep $module) ]]; then rmmod $module.ko; fi
make &&
dmesg -C &&
insmod $module.ko &&
make clean

echo -e '\nLooks like everything worked! :)\n'

# To check that it's effectively reserving the input/output port addresses 0x378
# cat /proc/ioports | grep parlelport

# This should turn on LED zero and six, leaving all of the others off.
# To turn on the LEDs and check that the system is working, execute:
# echo -n A > /dev/parlelport

# You can check the state of the parallel port issuing the command:
# cat /dev/parlelport
