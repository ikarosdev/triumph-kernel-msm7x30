/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
/*
 * WLAN Power Switch Module
 * controls power to external WLAN device
 * with interface to power management device
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>

#define DEBUG_OUTPUT 1

#ifdef DEBUG_OUTPUT
#define DBG_MSG(arg, ...) printk("[BCM4329_WLAN_DBG] %s: "arg"\r\n", __FUNCTION__, ## __VA_ARGS__);
#else
#define DBG_MSG(arg, ...)
#endif

#define INF_MSG(arg, ...) printk("[BCM4329_WLAN_INF] %s: "arg"\r\n", __FUNCTION__, ## __VA_ARGS__);
#define ERR_MSG(arg, ...) printk("[BCM4329_WLAN_ERR] %s: "arg"\r\n", __FUNCTION__, ## __VA_ARGS__);

static int bcm4329_wlan_toggle_radio(void *data, bool blocked)
{
	int ret;
	int (*power_control)(int enable);

	power_control = data;
	ret = (*power_control)(!blocked);
	return ret;
}

static const struct rfkill_ops bcm4329_wlan_power_rfkill_ops = {
	.set_block = bcm4329_wlan_toggle_radio,
};

static int bcm4329_wlan_power_rfkill_probe(struct platform_device *pdev)
{
	struct rfkill *rfkill;
	int ret;

	rfkill = rfkill_alloc("bcm4329_wifi_power", &pdev->dev, RFKILL_TYPE_WLAN,
			      &bcm4329_wlan_power_rfkill_ops,
			      pdev->dev.platform_data);

	if (!rfkill) {
		ERR_MSG("rfkill allocate failed");
		return -ENOMEM;
	}

	/* force Bluetooth off during init to allow for user control */
	rfkill_init_sw_state(rfkill, 1);

	ret = rfkill_register(rfkill);
	if (ret) {
		ERR_MSG("rfkill register failed=%d", ret);
		rfkill_destroy(rfkill);
		return ret;
	}

	platform_set_drvdata(pdev, rfkill);

	return 0;
}

static void bcm4329_wlan_power_rfkill_remove(struct platform_device *pdev)
{
	struct rfkill *rfkill;

	DBG_MSG("START");

	rfkill = platform_get_drvdata(pdev);
	if (rfkill)
		rfkill_unregister(rfkill);
	rfkill_destroy(rfkill);
	platform_set_drvdata(pdev, NULL);
}

static int __init bcm4329_wlan_power_probe(struct platform_device *pdev)
{
	int ret = 0;

	DBG_MSG("START");

	if (!pdev->dev.platform_data) {
		ERR_MSG("platform data not initialized");
		return -ENOSYS;
	}

	ret = bcm4329_wlan_power_rfkill_probe(pdev);

	return ret;
}

static int __devexit bcm4329_wlan_power_remove(struct platform_device *pdev)
{
	DBG_MSG("START");

	bcm4329_wlan_power_rfkill_remove(pdev);

	return 0;
}

static struct platform_driver bcm4329_wlan_power_driver = {
	.probe = bcm4329_wlan_power_probe,
	.remove = __devexit_p(bcm4329_wlan_power_remove),
	.driver = {
		.name = "bcm4329_wifi_power",
		.owner = THIS_MODULE,
	},
};

static int __init bcm4329_wlan_power_init(void)
{
	int ret;

	INF_MSG("START");

	ret = platform_driver_register(&bcm4329_wlan_power_driver);
	return ret;
}

static void __exit bcm4329_wlan_power_exit(void)
{
	INF_MSG("START");

	platform_driver_unregister(&bcm4329_wlan_power_driver);
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("BCM4329 WLAN power control driver");
MODULE_VERSION("1.00");

module_init(bcm4329_wlan_power_init);
module_exit(bcm4329_wlan_power_exit);
