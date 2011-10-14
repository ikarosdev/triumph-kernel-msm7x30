#
# Makefile for miscellaneous I2C chip drivers.
#
# Do not add new drivers to this directory! It is DEPRECATED.
#
# Device drivers are better grouped according to the functionality they
# implement rather than to the bus they are connected to. In particular:
# * Hardware monitoring chip drivers go to drivers/hwmon
# * RTC chip drivers go to drivers/rtc
# * I/O expander drivers go to drivers/gpio
#

obj-$(CONFIG_SENSORS_AKM8973)	+= akm8973.o
obj-$(CONFIG_SENSORS_AKM8976)	+= akm8976.o
obj-$(CONFIG_DS1682)		+= ds1682.o
obj-$(CONFIG_SENSORS_PCA963X)	+= pca963x.o
obj-$(CONFIG_SENSORS_TSL2550)	+= tsl2550.o
obj-$(CONFIG_SENSORS_MT9T013)	+= mt9t013.o
obj-$(CONFIG_VP_A1026)		+= a1026.o
obj-$(CONFIG_SENSORS_YAS529)    += ecompass.o
obj-$(CONFIG_SENSORS_LTR502ALS)	+= ltr502als.o
obj-$(CONFIG_DS2482)		+= ds2482.o
ifeq ($(CONFIG_I2C_DEBUG_CHIP),y)
EXTRA_CFLAGS += -DDEBUG
endif

#Div2D5-OwenHuang-FTM-YAS529_Self-Test-00+{
obj-$(CONFIG_SENSORS_NK_YAS529) += yas529.o
obj-$(CONFIG_SENSORS_BMA150)    += bma150.o
#Div2D5-OwenHuang-FTM-YAS529_Self-Test-00+}

#Div2D1-OH-eCompass-AKM8975C_Porting-00+{
obj-$(CONFIG_SENSORS_AKM8975C) += akm8975.o
#Div2D1-OH-eCompass-AKM8975C_Porting-00+}

#Div2D5-OwenHuang-SF8_Sensor_Porting-00-{
#obj-$(CONFIG_SENSORS_CM3623) += cm3623.o
#Div2D5-OwenHuang-SF8_Sensor_Porting-00-}