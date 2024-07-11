# Comment/uncomment the following line to disable/enable debugging
#DEBUG = y


# Add your debugging flag (or not) to CFLAGS
ifeq ($(DEBUG),y)
  DEBFLAGS = -O -g -DSCULL_DEBUG # "-O" is needed to expand inlines
else
  DEBFLAGS = -O2
endif

LDDINC=$(PWD)/../include

EXTRA_CFLAGS += $(DEBFLAGS)
EXTRA_CFLAGS += -I$(LDDINC)

ifneq ($(KERNELRELEASE),)
# call from kernel build system

fake-objs := \
	fake_drv.o \
	fake_gem.o

obj-m	:= fake.o

else

KERNELDIR ?= /lib/modules/$(shell uname -r)/build
PWD       := $(shell pwd)

modules:
	@echo "kernel release $(KERNELRELEASE)"
	@echo "kernel dir $(KERNELDIR)"
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

endif


test:
	gcc -o accel_test test.c -I. `pkg-config --cflags --libs libdrm` -Wall -O0 -g
	./accel_test

clean:
	rm -rf accel_test *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.mod modules.order *.symvers

.phony: test

depend .depend dep:
	$(CC) $(EXTRA_CFLAGS) -M *.c > .depend


ifeq (.depend,$(wildcard .depend))
include .depend
endif
