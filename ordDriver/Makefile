BUILD_DIR:=/lib/modules/$(shell uname -r)/build
PWD:=$(shell pwd)
obj-m := sortlist.o
all : 
	$(MAKE) -C $(BUILD_DIR) M=$(PWD) modules
clean:
	$(MAKE) -C $(BUILD_DIR) M=$(PWD) clean
