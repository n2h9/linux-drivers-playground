obj-m := n2_i2c_bmp280.o

RASPBERRY_SSH_PATH ?= 192.168.178.28:/home/n2/linux_drivers
KERNEL_DIR ?= $(PWD)/../../../kernel/raspberrypi


# Target architecture and cross-compiler
ARCH ?= arm64
CROSS_COMPILE ?= aarch64-linux-gnu-
EXTRA_CFLAGS += -DDEBUG

all:
	make -C $(KERNEL_DIR) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		EXTRA_CFLAGS=${EXTRA_CFLAGS} \
		M=$(PWD) modules

clean:
	make -C $(KERNEL_DIR) \
		ARCH=$(ARCH) \
		CROSS_COMPILE=$(CROSS_COMPILE) \
		M=$(PWD) clean

deploy:
	scp *.ko $(RASPBERRY_SSH_PATH)

