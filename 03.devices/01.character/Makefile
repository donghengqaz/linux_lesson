ifneq ($(KERNELRELEASE), )

EXTRA_CFLAGS += -I$(src)/../../04.virtualization/02.module
obj-m := character.o

else

EXTRA_CFLAGS += -I$(src)/../../04.virtualization/02.module
KDIR := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(shell pwd) modules

clean:
	rm *.o *.ko *.mod.c *.order *.symvers .tmp_versions .*.o.cmd .*.ko.cmd -rf

endif