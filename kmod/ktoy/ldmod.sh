#!/bin/sh

module="ktoy"
device="ktoy"
mode="664"

/sbin/insmod ./$module.ko $* || exit 1

rm -f /dev/${device}0

major=$(awk "\$2==\"$module\" {print \$1}" /proc/devices)

# because I use class_create, udev will auto manage the device file
# mknod /dev/${device}0 c $major 0

group="staff"
grep -q '^staff:' /etc/group || group="wheel"

# the default device will be named /dev/ktoy
# chgrp $group /dev/${device}0
# chmod $mode /dev/${device}0
chgrp $group /dev/${device}
chmod $mode /dev/${device}
