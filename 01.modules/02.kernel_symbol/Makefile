ifneq ($(KERNELRELEASE), )

ifeq ($(MODULE), symbol)
obj-m := symbol.o
else
obj-m := helloworld.o
endif

else

KDIR := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(shell pwd) modules

clean:
	rm *.o *.ko *.mod.c *.order *.symvers

endif