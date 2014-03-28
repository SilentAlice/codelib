obj-m   := ktoy.o

# KERNELDIR ?= /lib/modules/$(shell uname -r)
KERNELDIR ?= /usr/src/linux-headers-$(shell uname -r)/
PWD       := $(shell pwd)

EXTRA_CFLAGS += -I$(PWD)

all:
	$(MAKE) -C $(KERNELDIR) M=$(PWD)

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions
