#!/bin/bash

testmod () {
    # Using this module:
    echo "Press enter to test the module!" && read &&
    echo 'boop beep boop bwarp!' > /dev/memory &&
    cat /dev/memory
}

[[ $(whoami) != 'root' ]] && echo "Gotta be root, fool." && exit 1

# As root
module=memory

[[ -e /dev/$module ]] || (mknod /dev/$module c 60 0 && chmod 666 /dev/$module)

if [[ -n $(lsmod | grep $module) ]]; then rmmod $module.ko; fi
make &&
dmesg -C &&
insmod $module.ko &&
make clean

echo -e '\nLooks like everything worked! :)\n'
# testmod # If you want to use it

