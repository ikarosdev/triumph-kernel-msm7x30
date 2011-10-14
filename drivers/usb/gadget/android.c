/*
 * Gadget Driver for Android
 *
 * Copyright (C) 2008 Google, Inc.
 * Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/* #define DEBUG */
/* #define VERBOSE_DEBUG */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/utsname.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/debugfs.h>

#include <linux/usb/android_composite.h>
#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/moduleparam.h>
#include <linux/miscdevice.h>

#include "gadget_chips.h"

//Div2-5-3-Peripheral-LL-UsbPorting-00+{
#include "../../../arch/arm/mach-msm/smd_private.h"
#define USB_PID_FUNC_ALL 0xc000    //C000
#define USB_PID_FUNC_NODIAG 0xc001  //c001
#define USB_PID_FUNC_FTM 0xc003 //c003
#define USB_PID_FUNC_NODIAG_NOADB 0xc004 //c004
bool vbus_online = false;
static bool doRndis = false;
static bool enable_rndis_diag = false;
bool usb_check_rndis_switch(void);
void usb_switch_pid(int);
#define USBDBG(fmt, args...) \
    printk(KERN_INFO "android_usb:%s() " fmt "\n", __func__, ## args)
//Div2-5-3-Peripheral-LL-UsbPorting-00+}

/*
 * Kbuild is not very cooperative with respect to linking separately
 * compiled library objects into one module.  So for now we won't use
 * separate compilation ... ensuring init/exit sections work to shrink
 * the runtime footprint, and giving us at least some parts of what
 * a "gcc --combine ... part1.c part2.c part3.c ... " build would.
 */
#include "usbstring.c"
#include "config.c"
#include "epautoconf.c"
#include "composite.c"

MODULE_AUTHOR("Mike Lockwood");
MODULE_DESCRIPTION("Android Composite USB Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

static const char longname[] = "Gadget Android";

/* Default vendor and product IDs, overridden by platform data */
//Div6-D1-JL-UsbPidVid-00+{    
//#define VENDOR_ID		0x18D1
//#define PRODUCT_ID		0x0001
#define VENDOR_ID 0x0489
#define PRODUCT_ID 0xC000
//Div6-D1-JL-UsbPidVid-00+}

//FXP Dmitriy Berchanskiy USB FXPCAYM-100 {
/* support product ID */
static int product_id = 0;
static int android_set_pid(const char *val, struct kernel_param *kp);
static int android_get_pid(char *buf, struct kernel_param *kp);

module_param_call(product_id, android_set_pid, android_get_pid, &product_id, 0664);
MODULE_PARM_DESC(product_id,"USB device product id");
//FXP Dmitriy Berchanskiy USB FXPCAYM-100 }

struct android_dev {
	struct usb_composite_dev *cdev;
	struct usb_configuration *config;
	int num_products;
	struct android_usb_product *products;
	int num_functions;
	char **functions;

	int product_id;
	int version;
};

static struct android_dev *_android_dev;

#define MAX_STR_LEN 16
/* string IDs are assigned dynamically */

#define STRING_MANUFACTURER_IDX		0
#define STRING_PRODUCT_IDX		1
#define STRING_SERIAL_IDX		2

char serial_number[MAX_STR_LEN];
/* String Table */
static struct usb_string strings_dev[] = {
	/* These dummy values should be overridden by platform data */
	[STRING_MANUFACTURER_IDX].s = "Android",
	[STRING_PRODUCT_IDX].s = "Android",
	[STRING_SERIAL_IDX].s = "0123456789ABCDEF",
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

static struct usb_device_descriptor device_desc = {
	.bLength              = sizeof(device_desc),
	.bDescriptorType      = USB_DT_DEVICE,
	.bcdUSB               = __constant_cpu_to_le16(0x0200),
	.bDeviceClass         = USB_CLASS_PER_INTERFACE,
	.idVendor             = __constant_cpu_to_le16(VENDOR_ID),
	.idProduct            = __constant_cpu_to_le16(PRODUCT_ID),
	.bcdDevice            = __constant_cpu_to_le16(0xffff),
	.bNumConfigurations   = 1,
};

static struct usb_otg_descriptor otg_descriptor = {
	.bLength =		sizeof otg_descriptor,
	.bDescriptorType =	USB_DT_OTG,
	.bmAttributes =		USB_OTG_SRP | USB_OTG_HNP,
	.bcdOTG               = __constant_cpu_to_le16(0x0200),
};

static const struct usb_descriptor_header *otg_desc[] = {
	(struct usb_descriptor_header *) &otg_descriptor,
	NULL,
};

static struct list_head _functions = LIST_HEAD_INIT(_functions);
static int _registered_function_count = 0;

static void android_set_default_product(int product_id);

void android_usb_set_connected(int connected)
{
	if (_android_dev && _android_dev->cdev && _android_dev->cdev->gadget) {
		if (connected)
			usb_gadget_connect(_android_dev->cdev->gadget);
		else
			usb_gadget_disconnect(_android_dev->cdev->gadget);
	}
}

static struct android_usb_function *get_function(const char *name)
{
	struct android_usb_function	*f;
	list_for_each_entry(f, &_functions, list) {
		if (!strcmp(name, f->name))
			return f;
	}
	return 0;
}

//SW2-5-3-LL-Peripheral-Tethering_RNDIS-00+{
//avoid the enable RNDIS first offline state switch then 
//recover to original state update
bool usb_check_rndis_switch()
{
    bool ret = doRndis;
    printk("%s state=%d\n",__func__,ret);
    doRndis = false;
    return ret;
}
EXPORT_SYMBOL(usb_check_rndis_switch);
//SW2-5-3-LL-Peripheral-Tethering_RNDIS-00+}

static void bind_functions(struct android_dev *dev)
{
	struct android_usb_function	*f;
	char **functions = dev->functions;
	int i;

	for (i = 0; i < dev->num_functions; i++) {
		char *name = *functions++;
		f = get_function(name);
		if (f)
			f->bind_config(dev->config);
		else
			printk(KERN_ERR "function %s not found in bind_functions\n", name);
	}

	/*
	 * set_alt(), or next config->bind(), sets up
	 * ep->driver_data as needed.
	 */
	usb_ep_autoconfig_reset(dev->cdev->gadget);
}

static int __init android_bind_config(struct usb_configuration *c)
{
	struct android_dev *dev = _android_dev;

	printk(KERN_DEBUG "android_bind_config\n");
	dev->config = c;

	/* bind our functions if they have all registered */
	if (_registered_function_count == dev->num_functions)
		bind_functions(dev);

	return 0;
}

static int android_setup_config(struct usb_configuration *c,
		const struct usb_ctrlrequest *ctrl);

static struct usb_configuration android_config_driver = {
	.label		= "android",
	.bind		= android_bind_config,
	.setup		= android_setup_config,
	.bConfigurationValue = 1,
	.bMaxPower	= 0xFA, /* 500ma */
};

static int android_setup_config(struct usb_configuration *c,
		const struct usb_ctrlrequest *ctrl)
{
	int i;
	int ret = -EOPNOTSUPP;

	for (i = 0; i < android_config_driver.next_interface_id; i++) {
		if (android_config_driver.interface[i]->setup) {
			ret = android_config_driver.interface[i]->setup(
				android_config_driver.interface[i], ctrl);
			if (ret >= 0)
				return ret;
		}
	}
	return ret;
}

static int product_has_function(struct android_usb_product *p,
		struct usb_function *f)
{
	char **functions = p->functions;
	int count = p->num_functions;
	const char *name = f->name;
	int i;

	for (i = 0; i < count; i++) {
		if (!strcmp(name, *functions++))
			return 1;
	}
	return 0;
}

static int product_matches_functions(struct android_usb_product *p)
{
	struct usb_function		*f;
	list_for_each_entry(f, &android_config_driver.functions, list) {
		if (product_has_function(p, f) == !!f->hidden)
			return 0;
	}
	return 1;
}

static int get_product_id(struct android_dev *dev)
{
    struct android_usb_product *p = dev->products;
    int count = dev->num_products;
    int i;

    if (p) {
        for (i = 0; i < count; i++, p++) {
            if (product_matches_functions(p)){
                USBDBG("PID(%X)", p->product_id);
                return p->product_id;
            }
        }
    }
    USBDBG("PID(%X)", dev->product_id);
    /* use default product ID */
    return dev->product_id;
}

static int __init android_bind(struct usb_composite_dev *cdev)
{
	struct android_dev *dev = _android_dev;
	struct usb_gadget	*gadget = cdev->gadget;
	int			gcnum, id, product_id, ret;

	printk(KERN_INFO "android_bind\n");

	/* Allocate string descriptor numbers ... note that string
	 * contents can be overridden by the composite_dev glue.
	 */
	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_MANUFACTURER_IDX].id = id;
	device_desc.iManufacturer = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_PRODUCT_IDX].id = id;
	device_desc.iProduct = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_SERIAL_IDX].id = id;
	device_desc.iSerialNumber = id;

	if (gadget_is_otg(cdev->gadget))
		android_config_driver.descriptors = otg_desc;

	if (!usb_gadget_set_selfpowered(gadget))
		android_config_driver.bmAttributes |= USB_CONFIG_ATT_SELFPOWER;

	if (gadget->ops->wakeup)
		android_config_driver.bmAttributes |= USB_CONFIG_ATT_WAKEUP;

	/* register our configuration */
	ret = usb_add_config(cdev, &android_config_driver);
	if (ret) {
		printk(KERN_ERR "usb_add_config failed\n");
		return ret;
	}

	gcnum = usb_gadget_controller_number(gadget);
	if (gcnum >= 0)
		device_desc.bcdDevice = cpu_to_le16(0x0200 + gcnum);
	else {
		/* gadget zero is so simple (for now, no altsettings) that
		 * it SHOULD NOT have problems with bulk-capable hardware.
		 * so just warn about unrcognized controllers -- don't panic.
		 *
		 * things like configuration and altsetting numbering
		 * can need hardware-specific attention though.
		 */
		pr_warning("%s: controller '%s' not recognized\n",
			longname, gadget->name);
		device_desc.bcdDevice = __constant_cpu_to_le16(0x9999);
	}

	usb_gadget_set_selfpowered(gadget);
	dev->cdev = cdev;
    #if 0
    product_id = get_product_id(dev);
    #else
    product_id = dev->product_id;
    USBDBG("PID(%X)", dev->product_id);
    #endif
	device_desc.idProduct = __constant_cpu_to_le16(product_id);
	cdev->desc.idProduct = device_desc.idProduct;

	return 0;
}

static struct usb_composite_driver android_usb_driver = {
	.name		= "android_usb",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.bind		= android_bind,
	.enable_function = android_enable_function,
};

void android_register_function(struct android_usb_function *f)
{
	struct android_dev *dev = _android_dev;

	printk(KERN_INFO "android_register_function %s\n", f->name);
	list_add_tail(&f->list, &_functions);
	_registered_function_count++;

	/* bind our functions if they have all registered
	 * and the main driver has bound.
	 */
	if (dev->config && _registered_function_count == dev->num_functions) {
		bind_functions(dev);
		android_set_default_product(dev->product_id);
	}
}

/**
 * android_set_function_mask() - enables functions based on selected pid.
 * @up: selected product id pointer
 *
 * This function enables functions related with selected product id.
 */
static void android_set_function_mask(struct android_usb_product *up)
{
    int index;
    struct usb_function *func;

    list_for_each_entry(func, &android_config_driver.functions, list) {
        /* adb function enable/disable handled separetely */
        if (!strcmp(func->name, "adb"))
            continue;
        func->hidden = 1;
        for (index = 0; index < up->num_functions; index++) {
            if (!strcmp(up->functions[index], func->name)){
                USBDBG("func->name = %s enable", func->name);//Div2-5-3-Peripheral-LL-UsbPorting-00+
                func->hidden = 0;
            }
        }
    }
}

/**
 * android_set_defaut_product() - selects default product id and enables
 * required functions
 * @product_id: default product id
 *
 * This function selects default product id using pdata information and
 * enables functions for same.
*/
static void android_set_default_product(int pid)
{
	struct android_dev *dev = _android_dev;
	struct android_usb_product *up = dev->products;
	int index;

	for (index = 0; index < dev->num_products; index++, up++) {
		if (pid == up->product_id)
			break;
	}
	android_set_function_mask(up);
}

/**
 * android_config_functions() - selects product id based on function need
 * to be enabled / disabled.
 * @f: usb function
 * @enable : function needs to be enable or disable
 *
 * This function selects product id having required function at first index.
 * TODO : Search of function in product id can be extended for all index.
 * RNDIS function enable/disable uses this.
*/
#ifdef CONFIG_USB_ANDROID_RNDIS
static void android_config_functions(struct usb_function *f, int enable)
{
    struct android_dev *dev = _android_dev;
    struct android_usb_product *up = dev->products;
    int index;
    char **functions;

    /* Searches for product id having function at first index */
    if (enable) {
        for (index = 0; index < dev->num_products; index++, up++) {
            functions = up->functions;
            //if (!strcmp(*functions, f->name))
            //Div2-5-3-Peripheral-LL-UsbPorting-00*{
            if(enable_rndis_diag) {
                if (!strcmp("rndis", f->name) && (up->product_id == 0xc007))
                    break;
            } else {
                if (!strcmp(*functions, f->name))
                    break;
            }
            //Div2-5-3-Peripheral-LL-UsbPorting-00*}
            
        }
        android_set_function_mask(up);
    } else
        android_set_default_product(dev->product_id);
}
#endif

//Div2-5-3-Peripheral-LL-UsbPorting-00+
void usb_switch_pid(int pid)
{
    struct android_dev *dev = _android_dev;
    struct android_usb_product *up = dev->products;
    int index;
    struct usb_function *func;

    for (index = 0; index < dev->num_products; index++, up++) {
        if (pid == up->product_id)
            break;
    }

    list_for_each_entry(func, &android_config_driver.functions, list) {
        // FXPCAYM-213: Start - ADB should be controlled by USB debugging settings
        if (!strcmp(func->name, "adb"))
            continue;
        // FXPCAYM-213: End
        func->hidden = 1;
        for (index = 0; index < up->num_functions; index++) {
            if (!strcmp(up->functions[index], func->name)){
                USBDBG("func->name = %s enable", func->name);
                func->hidden = 0;
            }
        }
    }

    device_desc.idProduct = __constant_cpu_to_le16(pid);
    if (dev->cdev)
        dev->cdev->desc.idProduct = device_desc.idProduct;

    /* force reenumeration */
    if (dev->cdev && dev->cdev->gadget &&
            dev->cdev->gadget->speed != USB_SPEED_UNKNOWN) {
        usb_gadget_disconnect(dev->cdev->gadget);
        msleep(10);
        usb_gadget_connect(dev->cdev->gadget);
    }

}
EXPORT_SYMBOL(usb_switch_pid);
//Div2-5-3-Peripheral-LL-UsbPorting-00+}

void android_enable_function(struct usb_function *f, int enable)
{
	struct android_dev *dev = _android_dev;
	int disable = !enable;
	int product_id;

	printk(KERN_INFO "%s: function name = %s, enable = %d.\n", __func__, f->name, enable);

	if (!!f->hidden != disable) {
		f->hidden = disable;

#ifdef CONFIG_USB_ANDROID_RNDIS
		if (!strcmp(f->name, "rndis")) {

			/* We need to specify the COMM class in the device descriptor
			 * if we are using RNDIS.
			 */
			if (enable) {
#ifdef CONFIG_USB_ANDROID_RNDIS_WCEIS
				dev->cdev->desc.bDeviceClass = USB_CLASS_MISC;
				dev->cdev->desc.bDeviceSubClass      = 0x02;
				dev->cdev->desc.bDeviceProtocol      = 0x01;
#else
				dev->cdev->desc.bDeviceClass = USB_CLASS_COMM;
#endif
				doRndis = true;//SW2-5-3-LL-Peripheral-Tethering_RNDIS-00+
			} else {
				dev->cdev->desc.bDeviceClass = USB_CLASS_PER_INTERFACE;
				dev->cdev->desc.bDeviceSubClass      = 0;
				dev->cdev->desc.bDeviceProtocol      = 0;
				doRndis = false;//SW2-5-3-LL-Peripheral-Tethering_RNDIS-00+
			}

			android_config_functions(f, enable);
		}
#endif

		product_id = get_product_id(dev);
		device_desc.idProduct = __constant_cpu_to_le16(product_id);
		if (dev->cdev)
			dev->cdev->desc.idProduct = device_desc.idProduct;

		/* force reenumeration */
		if (dev->cdev && dev->cdev->gadget &&
				dev->cdev->gadget->speed != USB_SPEED_UNKNOWN) {
			usb_gadget_disconnect(dev->cdev->gadget);
			msleep(10);
			usb_gadget_connect(dev->cdev->gadget);
		}
	}
}

#ifdef CONFIG_DEBUG_FS
static int android_debugfs_open(struct inode *inode, struct file *file)
{
    file->private_data = inode->i_private;
    return 0;
}

static ssize_t android_debugfs_serialno_write(struct file *file, const char
            __user *buf,	size_t count, loff_t *ppos)
{
    char str_buf[MAX_STR_LEN];

    if (count > MAX_STR_LEN)
        return -EFAULT;

    if (copy_from_user(str_buf, buf, count))
        return -EFAULT;

    memcpy(serial_number, str_buf, count);

    if (serial_number[count - 1] == '\n')
        serial_number[count - 1] = '\0';

    strings_dev[STRING_SERIAL_IDX].s = serial_number;

    return count;
}
static ssize_t android_debugfs_rndis_diag_write(struct file *file, const char
            __user *buf,	size_t count, loff_t *ppos)
{
    char str_buf[1];

    if (copy_from_user(str_buf, buf, sizeof(str_buf)))
        return -EFAULT;
    enable_rndis_diag = (str_buf[0] == '1');
    USBDBG("diag %s",enable_rndis_diag?"enable":"disable");
    return count;
}
static ssize_t android_debugfs_pid_write(struct file *file, const char
            __user *buf,	size_t count, loff_t *ppos)
{
    char str_buf[5];

    if (copy_from_user(str_buf, buf, sizeof(str_buf)))
        return -EFAULT;
    str_buf[4] = '\0';
    USBDBG("pid(%s)",str_buf);
    if(!strcmp("c000", str_buf)) {
        usb_switch_pid(0xc000);
    } else if(!strcmp("c001", str_buf)) {
        usb_switch_pid(0xc001);
    } else if(!strcmp("c002", str_buf)) {
        usb_switch_pid(0xc002);
    } else if(!strcmp("c003", str_buf)) {
        usb_switch_pid(0xc003);
    } else if(!strcmp("c004", str_buf)) {
        usb_switch_pid(0xc004);
    } else if(!strcmp("c007", str_buf)) {
        usb_switch_pid(0xc007);
    } else if(!strcmp("c008", str_buf)) {
        usb_switch_pid(0xc008);
    }
    return count;
}

const struct file_operations android_fops = {
    .open = android_debugfs_open,
    .write = android_debugfs_serialno_write,
};
const struct file_operations android_rndis_diag_fops = {
    .open = android_debugfs_open,
    .write = android_debugfs_rndis_diag_write,
};
const struct file_operations android_pid_fops = {
    .open = android_debugfs_open,
    .write = android_debugfs_pid_write,
};

struct dentry *android_debug_root;
struct dentry *android_debug_serialno;
struct dentry *android_debug_rndis_diag;
struct dentry *android_debug_pid;

static int android_debugfs_init(struct android_dev *dev)
{
    android_debug_root = debugfs_create_dir("android", NULL);
    if (!android_debug_root)
        return -ENOENT;

    android_debug_serialno = debugfs_create_file("serial_number", 0200,
            android_debug_root, dev,
            &android_fops);
    if (!android_debug_serialno) {
        goto err;
    }
    android_debug_rndis_diag = debugfs_create_file("tethering_diag", 0200,
            android_debug_root, dev,
            &android_rndis_diag_fops);
    if (!android_debug_rndis_diag) {
        goto err;
    }
    android_debug_pid = debugfs_create_file("pid", 0200,
            android_debug_root, dev,
            &android_pid_fops);
    if (!android_debug_pid) {
        goto err;
    }

    return 0;
err:
    if(android_debug_serialno)
        debugfs_remove(android_debug_serialno);
    if(android_debug_rndis_diag)
        debugfs_remove(android_debug_rndis_diag);
    if(android_debug_pid)
        debugfs_remove(android_debug_pid);
    if(android_debug_root)
        debugfs_remove(android_debug_root);
    return -ENOENT;
}

static void android_debugfs_cleanup(void)
{
       debugfs_remove(android_debug_serialno);
       debugfs_remove(android_debug_root);
}
#endif
static int __init android_probe(struct platform_device *pdev)
{
    struct android_usb_platform_data *pdata = pdev->dev.platform_data;
    struct android_dev *dev = _android_dev;
    int result;
    //Div2-5-3-Peripheral-LL-UsbPorting-00+{
    unsigned int info_size;
    struct smem_host_oem_info *smem_Usb_type_info = NULL;
    //Div2-5-3-Peripheral-LL-UsbPorting-00+}

    printk(KERN_INFO "android_probe pdata: %p\n", pdata);

    pm_runtime_set_active(&pdev->dev);
    pm_runtime_enable(&pdev->dev);

    result = pm_runtime_get(&pdev->dev);
    if (result < 0) {
        dev_err(&pdev->dev,
            "Runtime PM: Unable to wake up the device, rc = %d\n",
            result);
        return result;
    }

    if (pdata) {
        dev->products = pdata->products;
        dev->num_products = pdata->num_products;
        dev->functions = pdata->functions;
        dev->num_functions = pdata->num_functions;
        if (pdata->vendor_id)
            device_desc.idVendor =
                __constant_cpu_to_le16(pdata->vendor_id);
        if (pdata->product_id) {
            dev->product_id = pdata->product_id;
            device_desc.idProduct =
                __constant_cpu_to_le16(pdata->product_id);
        }
        if (pdata->version)
            dev->version = pdata->version;

        if (pdata->product_name)
            strings_dev[STRING_PRODUCT_IDX].s = pdata->product_name;
        if (pdata->manufacturer_name)
            strings_dev[STRING_MANUFACTURER_IDX].s =
                pdata->manufacturer_name;
        if (pdata->serial_number)
            strings_dev[STRING_SERIAL_IDX].s = pdata->serial_number;
        else
            strings_dev[STRING_SERIAL_IDX].s = 0;
    }

    //Div2-5-3-Peripheral-LL-UsbPorting-00+{
    smem_Usb_type_info = smem_get_entry(SMEM_ID_VENDOR2, &info_size);
    if(smem_Usb_type_info) {
        dev->product_id = smem_Usb_type_info->host_usb_id;
        device_desc.idProduct = __constant_cpu_to_le16(dev->product_id);
        USBDBG("ready to set USB PID(%X)", dev->product_id);
    } else {
        dev->product_id = USB_PID_FUNC_ALL;
        device_desc.idProduct = __constant_cpu_to_le16(dev->product_id);
        USBDBG("can't get USB PID from smem, set default PID(%X)", dev->product_id);
    }

#ifdef CONFIG_FIH_FTM
    dev->product_id = USB_PID_FUNC_FTM;
    device_desc.idProduct = __constant_cpu_to_le16(dev->product_id);
    strings_dev[STRING_SERIAL_IDX].s = 0;
    USBDBG("ready to set USB PID(%X) for FTM", dev->product_id);
#endif
    //Div2-5-3-Peripheral-LL-UsbPorting-00+}
#ifdef CONFIG_DEBUG_FS
    result = android_debugfs_init(dev);
    if (result)
        pr_info("%s: android_debugfs_init failed\n", __func__);
#endif
    return usb_composite_register(&android_usb_driver);
}

//Div6-D1-JL-PidSwitching-00+}

//Fih DB ##Port switching{
static int android_set_pid(const char *val, struct kernel_param *kp)
{
  struct android_dev *dev = _android_dev;
  int ret = 0;
  unsigned long tmp;

  ret = strict_strtoul(val, 16, &tmp);
  if( dev->product_id != (unsigned int)tmp )
  {
     dev->product_id = (unsigned int)tmp;
     usb_switch_pid( (unsigned int)tmp);
  }
  return ret;
}

static int android_get_pid(char *buffer, struct kernel_param *pm)
{
   struct android_dev *dev = _android_dev;
   int ret;
   ret = sprintf(buffer,"%x", dev->product_id );
   return  ret; 
}

//Fih DB ##Port switching}


static int andr_runtime_suspend(struct device *dev)
{
	dev_dbg(dev, "pm_runtime: suspending...\n");
	return 0;
}

static int andr_runtime_resume(struct device *dev)
{
	dev_dbg(dev, "pm_runtime: resuming...\n");
	return 0;
}

static struct dev_pm_ops andr_dev_pm_ops = {
	.runtime_suspend = andr_runtime_suspend,
	.runtime_resume = andr_runtime_resume,
};

static struct platform_driver android_platform_driver = {
	.driver = { .name = "android_usb", .pm = &andr_dev_pm_ops},
	.probe = android_probe,
};

static int __init init(void)
{
    struct android_dev *dev;

    printk(KERN_INFO "android init\n");

    dev = kzalloc(sizeof(*dev), GFP_KERNEL);
    if (!dev)
        return -ENOMEM;

    /* set default values, which should be overridden by platform data */
    dev->product_id = PRODUCT_ID;
    _android_dev = dev;


    return platform_driver_register(&android_platform_driver);
}
module_init(init);

static void __exit cleanup(void)
{
#ifdef CONFIG_DEBUG_FS
	android_debugfs_cleanup();
#endif
	usb_composite_unregister(&android_usb_driver);
	platform_driver_unregister(&android_platform_driver);
	kfree(_android_dev);
	_android_dev = NULL;
}
module_exit(cleanup);
