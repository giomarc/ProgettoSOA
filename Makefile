MODNAME = service_module

.PHONY: all clean unload load


all:
	make -C  /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

uninstall:
	rm /lib/modules/$(shell uname -r)/extra/$(MODNAME).ko
	rm /lib/modules/$(shell uname -r)/$(MODNAME).ko
	
unload:
	echo "$(MODNAME).ko Removing..."
	sudo rmmod $(MODNAME).ko

load:
	echo "$(MODNAME).ko Loading..."
	sudo insmod $(MODNAME).ko

EXTRA_CFLAGS = -Wall

obj-m += $(MODNAME).o

$(MODNAME)-y += main_module.o
	$(MODNAME)-y += ./lib/vtpmo.o ./lib/sys_call_table_discovery.o ./lib/tag_descriptor_queue.o

ccflags-y :=  -I$(PWD)/include