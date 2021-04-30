obj-m+=lab2.o
all:
	${MAKE} -C /lib/modules/`uname -r`/build M=${PWD} modules
clean:
	${MAKE} -C /lib/modules/`uname -r`/build M=${PWD} clean
install:
	sudo insmod lab2.ko
remove:
	sudo rmmod lab2.ko
