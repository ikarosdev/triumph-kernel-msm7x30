# Configuration
#ifdef SUBDIRS
#include $(SUBDIRS)/../.config
#else
#include ../.config
#endif

include ./.config

KERNELDIR = /home/OMAP_Android_2.2_20100901/kernel/android-2.6.29
KVER=$(word 3,$(shell grep ^PATCHLEVEL $(KERNELDIR)/Makefile))

ifeq ("$(KVER)", "4")
BUILD_VER=V2.4
CFLAGS=
else
ifeq ("$(KVER)", "6")
BUILD_VER=V2.6
else
BUILD_VER=VOthers
endif
endif

HIF := sdio

GDM_CFLAGS :=

CONFIG_GDM_QOS=n
CONFIG_AGGREGATE_RECV_PKT=n
CONFIG_GDM_USB_PM=n
CONFIG_K_MODE=n

ifeq ("$(CONFIG_ENABLE_BW_SWITCHING_FOR_KT)", "y")
CONFIG_K_MODE=y
endif

ifeq ("$(CONFIG_GDM_QOS)", "y")
GDM_CFLAGS +=-DCONFIG_GDM_QOS
endif

ifeq ("$(CONFIG_AGGREGATE_RECV_PKT)", "y")
GDM_CFLAGS +=-DCONFIG_AGGREGATE_RECV_PKT
endif

ifeq ("$(CONFIG_GDM_USB_PM)", "y")
GDM_CFLAGS += -DCONFIG_GDM_USB_PM
endif

ifeq ("$(CONFIG_K_MODE)", "y")
GDM_CFLAGS += -DCONFIG_K_MODE
endif

ifeq ("$(BUILD_VER)", "V2.4")
CFLAGS+=$(GDM_CFLAGS)
else
EXTRA_CFLAGS+=$(GDM_CFLAGS)
endif

all: $(BUILD_VER)

.PHONY: V2.4 V2.6 VOthers clean

# Support for Kernel 2.4

OBJS = gdmwm.o
SRCS = gdm_usb.c gdm_wimax.c usb_boot.c netlink_k.c
ifeq ("$(CONFIG_GDM_QOS)", "y")
SRCS += gdm_qos.c
endif

V2.4: $(OBJS)

$(OBJS): CFLAGS += -D__KERNEL__ -DMODULE -I$(KERNELDIR)/include -O3
$(OBJS):	$(SRCS:.c=.o)
	$(LD) -r $^ -o $@

# Support for Kernel 2.6

obj-m := gdmwm.o
gdmwm-objs := gdm_wimax.o netlink_k.o

ifeq ("$(HIF)", "sdio")
gdmwm-objs += gdm_sdio.o sdio_boot.o
else
gdmwm-objs += gdm_usb.o usb_boot.o
endif

ifeq ("$(CONFIG_GDM_QOS)", "y")
gdmwm-objs += gdm_qos.o
endif

V2.6: 
	make -C $(KERNELDIR) SUBDIRS=$(shell pwd) modules

# Not supported Kernel versions

VOthers:
	@echo "This drivers only supports for Kernel v2.4 and v2.6."

clean:
	rm -rf *.o .*.cmd *.ko *.mod.c .tmp_versions built_in.o \
	Module.symvers Module.markers modules.order

