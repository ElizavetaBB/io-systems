obj-m+=lab1.o
all:
	${MAKE} -C /lib/modules/`uname -r`/build M=${PWD} modules

clean:
	${MAKE} -C /lib/modules/`uname -r`/build M=${PWD} clean

install:
	sudo insmod lab1.ko
remove:
	sudo rmmod lab1
