obj-m +=xfs5152ce.o

KDIR:=/home/mfy/Linux-drive/Linux-core-source-code/linux-imx-rel_imx_4.1.15_2.1.0_ga/

PWD?=$(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules
