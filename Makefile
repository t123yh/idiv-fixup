obj-m += idiv-fixup-mod.o 
idiv-fixup-mod-objs += insn-eval.o insn.o inat.o idiv-fixup.o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

# Set the path to the Kernel build utils.
KBUILD=/lib/modules/$(shell uname -r)/build/
 
default:
	$(MAKE) -C $(KBUILD) M=$(PWD) modules

clean:
	$(MAKE) -C $(KBUILD) M=$(PWD) clean

menuconfig:
	$(MAKE) -C $(KBUILD) M=$(PWD) menuconfig
