#!/bin/sh

module="ktoy"
device="ktoy"

rm -f /dev/${device}

/sbin/rmmod $module $* || exit 1
