ifneq ($(KERNELRELEASE), )

obj-m := vhw.o

else

KDIR := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(shell pwd) modules

clean:
	rm *.o *.ko *.mod.c *.order *.symvers .tmp_versions .*.o.cmd .*.ko.cmd -rf

endif