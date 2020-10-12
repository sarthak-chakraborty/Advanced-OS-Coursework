# /*
# PART B: Assignment 1 Makefile
# ------------------------------------------
# Sankalp R. 16CS30031
# Sarthak Charkraborty 16CS30044
# ------------------------------------------
# Kernel Version used : 5.4.0-48-generic
# System : Ubuntu 18.04 LTS
# */
obj-m+=lkm_moduleA1.o
all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules MODULE_FORCE_UNLOAD=yes

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean


