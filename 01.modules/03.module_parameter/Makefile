ifneq ($(KERNELRELEASE), )

obj-m := module_parameter.o

else

KDIR := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(shell pwd) modules

clean:
	rm *.o *.ko *.mod.c *.order *.symvers

endif