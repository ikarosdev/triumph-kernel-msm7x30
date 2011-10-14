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
 *
 */

/*
 * this needs to be before <linux/kernel.h> is loaded,
 * and <linux/sched.h> loads <linux/kernel.h>
 */
#define DEBUG  1

#include <linux/earlysuspend.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>
#include <linux/mutex.h>

#include <asm/atomic.h>

#include <mach/msm_rpcrouter.h>
#include <mach/msm_hsusb.h>
#include <mach/fih_msm_battery.h>
#ifdef CONFIG_BATTERY_FIH_DS2784
#include <mach/ds2784.h>
#endif
#ifdef CONFIG_BATTERY_BQ275X0
#include <mach/bq275x0_battery.h>
#endif

#include <linux/delay.h>
#include <linux/kernel.h>
#include "../../arch/arm/mach-msm/smd_private.h"

#define BATTERY_RPC_PROG	0x30000089
#define BATTERY_RPC_VER_1_1	0x00010001
#define BATTERY_RPC_VER_2_1	0x00020001
#define BATTERY_RPC_VER_5_1     0x00050001

#define BATTERY_RPC_CB_PROG	(BATTERY_RPC_PROG | 0x01000000)

#define CHG_RPC_PROG		0x3000001a
#define CHG_RPC_VER_1_1		0x00010001
#define CHG_RPC_VER_1_3		0x00010003
#define CHG_RPC_VER_2_2		0x00020002

#define BATTERY_REGISTER_PROC                          	2
#define BATTERY_MODIFY_CLIENT_PROC                     	4
#define BATTERY_DEREGISTER_CLIENT_PROC			5
#define BATTERY_READ_MV_PROC 				12
#define BATTERY_ENABLE_DISABLE_FILTER_PROC 		14

#define VBATT_FILTER			2

#define BATTERY_CB_TYPE_PROC 		1
#define BATTERY_CB_ID_ALL_ACTIV       	1
#define BATTERY_CB_ID_LOW_VOL		2

#define BATTERY_LOW            	2800
#define BATTERY_HIGH            4200

#define ONCRPC_CHG_GET_GENERAL_STATUS_PROC 	12
#define ONCRPC_CHARGER_API_VERSIONS_PROC 	0xffffffff

#define BATT_RPC_TIMEOUT    5000	/* 5 sec */

#define INVALID_BATT_HANDLE    -1

#define RPC_TYPE_REQ     0
#define RPC_TYPE_REPLY   1
#define RPC_REQ_REPLY_COMMON_HEADER_SIZE   (3 * sizeof(uint32_t))


#if DEBUG
#define DBG_LIMIT(x...) do {if (printk_ratelimit()) pr_debug(x); } while (0)
#else
#define DBG_LIMIT(x...) do {} while (0)
#endif

//Slate Code Start
bool slate_counter_flag = false;
EXPORT_SYMBOL(slate_counter_flag);
//Slate Code End

struct rpc_reply_batt_chg_v1 {
	struct rpc_reply_hdr hdr;
	u32 	more_data;

	u32	charger_status;
	u32	charger_type;
	u32	battery_status;
	u32	battery_level;
	u32     battery_voltage;
	u32	battery_temp;
};

struct rpc_reply_batt_chg_v2 {
	struct rpc_reply_batt_chg_v1	v1;

	u32	is_charger_valid;
	u32	is_charging;
	u32	is_battery_valid;
	u32	ui_event;
};

union rpc_reply_batt_chg {
	struct rpc_reply_batt_chg_v1	v1;
	struct rpc_reply_batt_chg_v2	v2;
};

static union rpc_reply_batt_chg rep_batt_chg;

static DEFINE_MUTEX(charger_status_lock);

struct msm_battery_info {
	u32 chg_api_version;
	u32 batt_api_version;

	u32 avail_chg_sources;
	u32 current_chg_source;

    u32 batt_level;
	u32 batt_status;
	u32 charger_valid;

	u32 charger_type;

	s32 batt_handle;

	struct power_supply *msm_psy_ac;
	struct power_supply *msm_psy_usb;

	struct msm_rpc_client *batt_client;
	struct msm_rpc_endpoint *chg_ep;

	wait_queue_head_t wait_q;

	u32 vbatt_modify_reply_avail;

	struct early_suspend early_suspend;
    struct wake_lock msm_batt_wakelock;
    
    struct batt_info_interface* batt_info_if;
    bool OT_flag;
    bool over400;
    bool driver_ready;
    bool charging;
    bool full;
    bool presence;
};

static struct msm_battery_info msm_batt_info = {
	.batt_handle = INVALID_BATT_HANDLE,
    .charger_type = CHARGER_TYPE_NONE,
    .batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING,
    .batt_level = BATTERY_LEVEL_GOOD,
    .vbatt_modify_reply_avail = 0,
    .current_chg_source = 0,
    .OT_flag = false,
    .over400 = false,
    .driver_ready = false,
    .charging = false,
    .full = false,
    .presence = true,
};

static enum power_supply_property msm_power_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *msm_power_supplied_to[] = {
	"battery",
};

static int msm_power_get_property(struct power_supply *psy,
				  enum power_supply_property psp,
				  union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		if (psy->type == POWER_SUPPLY_TYPE_MAINS) {
			val->intval = msm_batt_info.current_chg_source & AC_CHG
			    ? 1 : 0;
		}
		if (psy->type == POWER_SUPPLY_TYPE_USB) {
			val->intval = msm_batt_info.current_chg_source & USB_CHG
			    ? 1 : 0;
		}
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static struct power_supply msm_psy_ac = {
	.name = "ac",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.supplied_to = msm_power_supplied_to,
	.num_supplicants = ARRAY_SIZE(msm_power_supplied_to),
	.properties = msm_power_props,
	.num_properties = ARRAY_SIZE(msm_power_props),
	.get_property = msm_power_get_property,
};

static struct power_supply msm_psy_usb = {
	.name = "usb",
	.type = POWER_SUPPLY_TYPE_USB,
	.supplied_to = msm_power_supplied_to,
	.num_supplicants = ARRAY_SIZE(msm_power_supplied_to),
	.properties = msm_power_props,
	.num_properties = ARRAY_SIZE(msm_power_props),
	.get_property = msm_power_get_property,
};

#define be32_to_cpu_self(v) (v = be32_to_cpu(v))

//static void msm_batt_status_update_work(struct work_struct *work);
//static DECLARE_DELAYED_WORK(status_update_work, msm_batt_status_update_work);

static int msm_batt_get_batt_chg_status(void)
{
	int rc ;
	struct rpc_req_batt_chg {
		struct rpc_request_hdr hdr;
		u32 more_data;
	} req_batt_chg;
	struct rpc_reply_batt_chg_v1 *v1p;

	req_batt_chg.more_data = cpu_to_be32(1);

	memset(&rep_batt_chg, 0, sizeof(rep_batt_chg));

	v1p = &rep_batt_chg.v1;
	rc = msm_rpc_call_reply(msm_batt_info.chg_ep,
				ONCRPC_CHG_GET_GENERAL_STATUS_PROC,
				&req_batt_chg, sizeof(req_batt_chg),
				&rep_batt_chg, sizeof(rep_batt_chg),
				msecs_to_jiffies(BATT_RPC_TIMEOUT));
	if (rc < 0) {
		pr_err("%s: ERROR. msm_rpc_call_reply failed! proc=%d rc=%d\n",
		       __func__, ONCRPC_CHG_GET_GENERAL_STATUS_PROC, rc);
		return rc;
	} else if (be32_to_cpu(v1p->more_data)) {
        //be32_to_cpu_self(v1p->charger_status);
        //be32_to_cpu_self(v1p->charger_type);
        //be32_to_cpu_self(v1p->battery_status);
        be32_to_cpu_self(v1p->battery_level);
        //be32_to_cpu_self(v1p->battery_voltage);
        //be32_to_cpu_self(v1p->battery_temp);
	} else {
		pr_err("%s: No battery/charger data in RPC reply\n", __func__);
		return -EIO;
	}

	return 0;
}

extern int proc_comm_set_chg_voltage(u32 vmaxsel, u32 vbatdet, u32 vrechg);
static void msm_batt_update_psy_status(void)
{
	int product_id = fih_get_product_id();
	
    if (msm_batt_get_batt_chg_status())
        return;
        
    msm_batt_info.batt_level = rep_batt_chg.v1.battery_level;
    DBG_LIMIT("BATT: battery_level %d\n", msm_batt_info.batt_level);
    
	if (product_id == Product_FB3) {
		if (msm_batt_info.current_chg_source & (AC_CHG | USB_CHG)) {
			if (msm_batt_info.over400) {
				DBG_LIMIT("[CALLBACK]BATT: proc_comm_set_chg_voltage 4100, 4050, 3800\n");
				proc_comm_set_chg_voltage(4100, 4050, 3800);
			} else {
				DBG_LIMIT("[CALLBACK]BATT: proc_comm_set_chg_voltage 4200, 4150, 3900\n");
				proc_comm_set_chg_voltage(4200, 4150, 3900);
			}
		}
	}
}

extern int hsusb_chg_notify_over_tempearture(bool OT_flag);
static int discharging_counter = 0;
static bool disable_discharging_montior = false;
void msm_batt_disable_discharging_monitor(bool disable)
{
    disable_discharging_montior = disable;
}
EXPORT_SYMBOL(msm_batt_disable_discharging_monitor);

int msm_batt_get_batt_level(void)
{
    if (msm_batt_info.batt_level == BATTERY_LEVEL_FULL) {
        DBG_LIMIT("BATT: Set 100");
        return 100;
    } else {
        return 99;
    }
}
EXPORT_SYMBOL(msm_batt_get_batt_level);

static void msm_batt_update_batt_status(void)
{
    static bool OT_flag = false;
    
    DBG_LIMIT("BATT: Discharging Counter %d\n", discharging_counter);
    
    if (disable_discharging_montior)
        discharging_counter = 31;
		
    if (msm_batt_info.current_chg_source & (AC_CHG | USB_CHG)) 
    {
        if (!msm_batt_info.presence) 
        {
            discharging_counter = 0;
            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;            
        } else if (msm_batt_info.OT_flag) {
            OT_flag = true;
            discharging_counter = 0;
            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_DISCHARGING;
        } else if (msm_batt_info.full || msm_batt_info.batt_level == BATTERY_LEVEL_FULL) {
            discharging_counter = 0;
            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_FULL;
        } 
	else if (!msm_batt_info.charging && !OT_flag) 
	{
            if (msm_batt_info.batt_level == BATTERY_LEVEL_FULL) 
	    {
                discharging_counter = 0;
                msm_batt_info.batt_status = POWER_SUPPLY_STATUS_FULL;
            } 
	    else if (discharging_counter > 5 || slate_counter_flag ) {
                discharging_counter = 0;
                msm_batt_info.batt_status = POWER_SUPPLY_STATUS_DISCHARGING;
				slate_counter_flag = false;
            } 
	    else if (msm_batt_info.batt_status != POWER_SUPPLY_STATUS_DISCHARGING) 
	    {
                discharging_counter++;
                if (msm_batt_info.batt_status != POWER_SUPPLY_STATUS_DISCHARGING)
                    msm_batt_info.batt_status = POWER_SUPPLY_STATUS_CHARGING;
            } 
	    else 
	    {
                msm_batt_info.batt_status = POWER_SUPPLY_STATUS_DISCHARGING;            
            }
        }
	else 
	{
            OT_flag = false;
            discharging_counter = 0;
            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_CHARGING;
        }
    }// (msm_batt_info.current_chg_source & (AC_CHG | USB_CHG)) ends
    else {
        OT_flag = false;
        discharging_counter = 0;
        msm_batt_info.batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
    }
}

void msm_batt_update_charger_type(enum chg_type charger_type)
{
    if (msm_batt_info.driver_ready)
        wake_lock(&msm_batt_info.msm_batt_wakelock);
    
    mutex_lock(&charger_status_lock);
    switch (charger_type) {
    case USB_CHG_TYPE__SDP:
        DBG_LIMIT("BATT: USB charger plugged in\n");   
        msm_batt_info.current_chg_source = USB_CHG;
        break;
    case USB_CHG_TYPE__CARKIT:
        DBG_LIMIT("BATT: CAR KIT plugged in\n");
        break;
    case USB_CHG_TYPE__WALLCHARGER:
        DBG_LIMIT("BATT: AC Wall changer plugged in\n");
        msm_batt_info.current_chg_source = AC_CHG;
        break;
    case USB_CHG_TYPE__INVALID:
        if (msm_batt_info.current_chg_source & AC_CHG)
            DBG_LIMIT("BATT: AC Wall charger removed\n");
        else if (msm_batt_info.current_chg_source & USB_CHG)
            DBG_LIMIT("BATT: USB charger removed\n");
        else
            DBG_LIMIT("BATT: No charger present\n");
            
        msm_batt_info.current_chg_source = 0;
        break;
    }
    
    if (msm_batt_info.current_chg_source & (AC_CHG | USB_CHG)) {
        if (msm_batt_info.OT_flag)
            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_DISCHARGING;
        else if (msm_batt_info.full)
            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_FULL;
        else
            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_CHARGING;
    } else
        msm_batt_info.batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;

    mutex_unlock(&charger_status_lock);

    if (msm_batt_info.driver_ready) {
        power_supply_changed(msm_batt_info.msm_psy_ac);
        wake_unlock(&msm_batt_info.msm_batt_wakelock);
        wake_lock_timeout(&msm_batt_info.msm_batt_wakelock, 10 * HZ);
    }
}
EXPORT_SYMBOL(msm_batt_update_charger_type);

void msm_batt_notify_over_400(bool over400)
{
    DBG_LIMIT("BATT: %s\n", over400 ? "over400" : "below400");
    mutex_lock(&charger_status_lock);
    msm_batt_info.over400 = over400;
    if (msm_batt_info.current_chg_source & (AC_CHG | USB_CHG)) {
		if (msm_batt_info.over400) {
			DBG_LIMIT("BATT: proc_comm_set_chg_voltage 4100, 4050, 3800\n");
			proc_comm_set_chg_voltage(4100, 4050, 3800);
		} else {
			DBG_LIMIT("BATT: proc_comm_set_chg_voltage 4200, 4150, 3900\n");
			proc_comm_set_chg_voltage(4200, 4150, 3900);
		}
		msm_batt_update_batt_status();
	}
    mutex_unlock(&charger_status_lock);
}
EXPORT_SYMBOL(msm_batt_notify_over_400);

void msm_batt_notify_over_temperature(bool over_temperature)
{
    DBG_LIMIT("BATT: %s\n", over_temperature ? "Over Temperature" : "Normal Temperature");
    mutex_lock(&charger_status_lock);
    msm_batt_info.OT_flag = over_temperature;
    hsusb_chg_notify_over_tempearture(over_temperature);
    msm_batt_update_batt_status();
    mutex_unlock(&charger_status_lock);
}
EXPORT_SYMBOL(msm_batt_notify_over_temperature);

void msm_batt_notify_battery_full(bool full)
{
    DBG_LIMIT("BATT: %s\n", full ? "Full" : "Not Full");
    mutex_lock(&charger_status_lock);
    msm_batt_info.full = full;
    msm_batt_update_batt_status();
    mutex_unlock(&charger_status_lock);
}
EXPORT_SYMBOL(msm_batt_notify_battery_full);

void msm_batt_notify_charging_state(bool charging)
{
    DBG_LIMIT("BATT: %s\n", charging ? "Charging" : "Discharing");
    mutex_lock(&charger_status_lock);
    msm_batt_info.charging = charging;
    msm_batt_update_batt_status();
    mutex_unlock(&charger_status_lock);
}
EXPORT_SYMBOL(msm_batt_notify_charging_state);

void msm_batt_notify_battery_presence(bool presence)
{
    DBG_LIMIT("BATT: %s\n", presence ? "Presence" : "No Presence");
    mutex_lock(&charger_status_lock);
    msm_batt_info.presence = presence;
    msm_batt_update_batt_status();
    mutex_unlock(&charger_status_lock);
}
EXPORT_SYMBOL(msm_batt_notify_battery_presence);

u32 msm_batt_get_batt_status(void)
{
    return msm_batt_info.batt_status;
}
EXPORT_SYMBOL(msm_batt_get_batt_status);

u32 msm_batt_get_chg_source(void)
{
    return msm_batt_info.current_chg_source;
}
EXPORT_SYMBOL(msm_batt_get_chg_source);

int msm_batt_info_not_support(void)
{
    return 0xFF;
}
EXPORT_SYMBOL(msm_batt_info_not_support);

#ifdef CONFIG_HAS_EARLYSUSPEND
void msm_batt_early_suspend(struct early_suspend *h)
{
    /*int rc;

    pr_debug("%s: enter\n", __func__);

    pr_debug("%s: exit\n", __func__);*/
}

void msm_batt_late_resume(struct early_suspend *h)
{
    /*int rc;

    pr_debug("%s: enter\n", __func__);

    pr_debug("%s: exit\n", __func__);*/
}
#endif

struct msm_batt_vbatt_filter_req {
	u32 batt_handle;
	u32 enable_filter;
	u32 vbatt_filter;
};

struct msm_batt_vbatt_filter_rep {
	u32 result;
};

static int msm_batt_filter_arg_func(struct msm_rpc_client *batt_client,

		void *buf, void *data)
{
	struct msm_batt_vbatt_filter_req *vbatt_filter_req =
		(struct msm_batt_vbatt_filter_req *)data;
	u32 *req = (u32 *)buf;
	int size = 0;

	*req = cpu_to_be32(vbatt_filter_req->batt_handle);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(vbatt_filter_req->enable_filter);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(vbatt_filter_req->vbatt_filter);
	size += sizeof(u32);
	return size;
}

static int msm_batt_filter_ret_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{

	struct msm_batt_vbatt_filter_rep *data_ptr, *buf_ptr;

	data_ptr = (struct msm_batt_vbatt_filter_rep *)data;
	buf_ptr = (struct msm_batt_vbatt_filter_rep *)buf;

	data_ptr->result = be32_to_cpu(buf_ptr->result);
	return 0;
}

static int msm_batt_enable_filter(u32 vbatt_filter)
{
	int rc;
	struct  msm_batt_vbatt_filter_req  vbatt_filter_req;
	struct  msm_batt_vbatt_filter_rep  vbatt_filter_rep;

	vbatt_filter_req.batt_handle = msm_batt_info.batt_handle;
	vbatt_filter_req.enable_filter = 1;
	vbatt_filter_req.vbatt_filter = vbatt_filter;

	rc = msm_rpc_client_req(msm_batt_info.batt_client,
			BATTERY_ENABLE_DISABLE_FILTER_PROC,
			msm_batt_filter_arg_func, &vbatt_filter_req,
			msm_batt_filter_ret_func, &vbatt_filter_rep,
			msecs_to_jiffies(BATT_RPC_TIMEOUT));

	if (rc < 0) {
		pr_err("%s: FAIL: enable vbatt filter. rc=%d\n",
		       __func__, rc);
		return rc;
	}

	if (vbatt_filter_rep.result != BATTERY_DEREGISTRATION_SUCCESSFUL) {
		pr_err("%s: FAIL: enable vbatt filter: result=%d\n",
		       __func__, vbatt_filter_rep.result);
		return -EIO;
	}

	pr_debug("%s: enable vbatt filter: OK\n", __func__);
	return rc;
}

struct batt_client_registration_req {
	/* The voltage at which callback (CB) should be called. */
	u32 desired_batt_voltage;

	/* The direction when the CB should be called. */
	u32 voltage_direction;

	/* The registered callback to be called when voltage and
	 * direction specs are met. */
	u32 batt_cb_id;

	/* The call back data */
	u32 cb_data;
	u32 more_data;
	u32 batt_error;
};

struct batt_client_registration_rep {
	u32 batt_handle;
};

static int msm_batt_register_arg_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct batt_client_registration_req *batt_reg_req =
		(struct batt_client_registration_req *)data;
	u32 *req = (u32 *)buf;
	int size = 0;


	*req = cpu_to_be32(batt_reg_req->desired_batt_voltage);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(batt_reg_req->voltage_direction);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(batt_reg_req->batt_cb_id);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(batt_reg_req->cb_data);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(batt_reg_req->more_data);
	size += sizeof(u32);
	req++;

	*req = cpu_to_be32(batt_reg_req->batt_error);
	size += sizeof(u32);

	return size;
}

static int msm_batt_register_ret_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct batt_client_registration_rep *data_ptr, *buf_ptr;

	data_ptr = (struct batt_client_registration_rep *)data;
	buf_ptr = (struct batt_client_registration_rep *)buf;

	data_ptr->batt_handle = be32_to_cpu(buf_ptr->batt_handle);

	return 0;
}

static int msm_batt_register(u32 desired_batt_voltage,
			     u32 voltage_direction, u32 batt_cb_id, u32 cb_data)
{
	struct batt_client_registration_req batt_reg_req;
	struct batt_client_registration_rep batt_reg_rep;
	int rc;

	batt_reg_req.desired_batt_voltage = desired_batt_voltage;
	batt_reg_req.voltage_direction = voltage_direction;
	batt_reg_req.batt_cb_id = batt_cb_id;
	batt_reg_req.cb_data = cb_data;
	batt_reg_req.more_data = 1;
	batt_reg_req.batt_error = 0;

	rc = msm_rpc_client_req(msm_batt_info.batt_client,
			BATTERY_REGISTER_PROC,
			msm_batt_register_arg_func, &batt_reg_req,
			msm_batt_register_ret_func, &batt_reg_rep,
			msecs_to_jiffies(BATT_RPC_TIMEOUT));

	if (rc < 0) {
		pr_err("%s: FAIL: vbatt register. rc=%d\n", __func__, rc);
		return rc;
	}

	msm_batt_info.batt_handle = batt_reg_rep.batt_handle;

	pr_debug("%s: got handle = %d\n", __func__, msm_batt_info.batt_handle);

	return 0;
}

struct batt_client_deregister_req {
	u32 batt_handle;
};

struct batt_client_deregister_rep {
	u32 batt_error;
};

static int msm_batt_deregister_arg_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct batt_client_deregister_req *deregister_req =
		(struct  batt_client_deregister_req *)data;
	u32 *req = (u32 *)buf;
	int size = 0;

	*req = cpu_to_be32(deregister_req->batt_handle);
	size += sizeof(u32);

	return size;
}

static int msm_batt_deregister_ret_func(struct msm_rpc_client *batt_client,
				       void *buf, void *data)
{
	struct batt_client_deregister_rep *data_ptr, *buf_ptr;

	data_ptr = (struct batt_client_deregister_rep *)data;
	buf_ptr = (struct batt_client_deregister_rep *)buf;

	data_ptr->batt_error = be32_to_cpu(buf_ptr->batt_error);

	return 0;
}

static int msm_batt_deregister(u32 batt_handle)
{
	int rc;
	struct batt_client_deregister_req req;
	struct batt_client_deregister_rep rep;

	req.batt_handle = batt_handle;

	rc = msm_rpc_client_req(msm_batt_info.batt_client,
			BATTERY_DEREGISTER_CLIENT_PROC,
			msm_batt_deregister_arg_func, &req,
			msm_batt_deregister_ret_func, &rep,
			msecs_to_jiffies(BATT_RPC_TIMEOUT));

	if (rc < 0) {
		pr_err("%s: FAIL: vbatt deregister. rc=%d\n", __func__, rc);
		return rc;
	}

	if (rep.batt_error != BATTERY_DEREGISTRATION_SUCCESSFUL) {
		pr_err("%s: vbatt deregistration FAIL. error=%d, handle=%d\n",
		       __func__, rep.batt_error, batt_handle);
		return -EIO;
	}

	return 0;
}

static int msm_batt_cleanup(void)
{
    int rc = 0;
    
    msm_batt_info.driver_ready = false;

	if (msm_batt_info.batt_handle != INVALID_BATT_HANDLE) {

		rc = msm_batt_deregister(msm_batt_info.batt_handle);
		if (rc < 0)
			pr_err("%s: FAIL: msm_batt_deregister. rc=%d\n",
			       __func__, rc);
	}

	msm_batt_info.batt_handle = INVALID_BATT_HANDLE;

	if (msm_batt_info.batt_client)
		msm_rpc_unregister_client(msm_batt_info.batt_client);

	if (msm_batt_info.msm_psy_ac)
		power_supply_unregister(msm_batt_info.msm_psy_ac);

	if (msm_batt_info.msm_psy_usb)
		power_supply_unregister(msm_batt_info.msm_psy_usb);

	if (msm_batt_info.chg_ep) {
		rc = msm_rpc_close(msm_batt_info.chg_ep);
		if (rc < 0) {
			pr_err("%s: FAIL. msm_rpc_close(chg_ep). rc=%d\n",
			       __func__, rc);
		}
	}
    
    wake_lock_destroy(&msm_batt_info.msm_batt_wakelock);

#ifdef CONFIG_HAS_EARLYSUSPEND
	if (msm_batt_info.early_suspend.suspend == msm_batt_early_suspend)
		unregister_early_suspend(&msm_batt_info.early_suspend);
#endif

	return rc;
}

int msm_batt_get_charger_api_version(void)
{
	int rc ;
	struct rpc_reply_hdr *reply;

	struct rpc_req_chg_api_ver {
		struct rpc_request_hdr hdr;
		u32 more_data;
	} req_chg_api_ver;

	struct rpc_rep_chg_api_ver {
		struct rpc_reply_hdr hdr;
		u32 num_of_chg_api_versions;
		u32 *chg_api_versions;
	};

	u32 num_of_versions;

	struct rpc_rep_chg_api_ver *rep_chg_api_ver;


	req_chg_api_ver.more_data = cpu_to_be32(1);

	msm_rpc_setup_req(&req_chg_api_ver.hdr, CHG_RPC_PROG, CHG_RPC_VER_1_1,
			  ONCRPC_CHARGER_API_VERSIONS_PROC);

	rc = msm_rpc_write(msm_batt_info.chg_ep, &req_chg_api_ver,
			sizeof(req_chg_api_ver));
	if (rc < 0) {
		pr_err("%s: FAIL: msm_rpc_write. proc=0x%08x, rc=%d\n",
		       __func__, ONCRPC_CHARGER_API_VERSIONS_PROC, rc);
		return rc;
	}

	for (;;) {
		rc = msm_rpc_read(msm_batt_info.chg_ep, (void *) &reply, -1,
				BATT_RPC_TIMEOUT);
		if (rc < 0)
			return rc;
		if (rc < RPC_REQ_REPLY_COMMON_HEADER_SIZE) {
			pr_err("%s: LENGTH ERR: msm_rpc_read. rc=%d (<%d)\n",
			       __func__, rc, RPC_REQ_REPLY_COMMON_HEADER_SIZE);

			rc = -EIO;
			break;
		}
		/* we should not get RPC REQ or call packets -- ignore them */
		if (reply->type == RPC_TYPE_REQ) {
			pr_err("%s: TYPE ERR: type=%d (!=%d)\n",
			       __func__, reply->type, RPC_TYPE_REQ);
			kfree(reply);
			continue;
		}

		/* If an earlier call timed out, we could get the (no
		 * longer wanted) reply for it.	 Ignore replies that
		 * we don't expect
		 */
		if (reply->xid != req_chg_api_ver.hdr.xid) {
			pr_err("%s: XID ERR: xid=%d (!=%d)\n", __func__,
			       reply->xid, req_chg_api_ver.hdr.xid);
			kfree(reply);
			continue;
		}
		if (reply->reply_stat != RPCMSG_REPLYSTAT_ACCEPTED) {
			rc = -EPERM;
			break;
		}
		if (reply->data.acc_hdr.accept_stat !=
				RPC_ACCEPTSTAT_SUCCESS) {
			rc = -EINVAL;
			break;
		}

		rep_chg_api_ver = (struct rpc_rep_chg_api_ver *)reply;

		num_of_versions =
			be32_to_cpu(rep_chg_api_ver->num_of_chg_api_versions);

		rep_chg_api_ver->chg_api_versions =  (u32 *)
			((u8 *) reply + sizeof(struct rpc_reply_hdr) +
			sizeof(rep_chg_api_ver->num_of_chg_api_versions));

		rc = be32_to_cpu(
			rep_chg_api_ver->chg_api_versions[num_of_versions - 1]);

		pr_debug("%s: num_of_chg_api_versions = %u. "
			"The chg api version = 0x%08x\n", __func__,
			num_of_versions, rc);
		break;
	}
	kfree(reply);
	return rc;
}

static int msm_batt_cb_func(struct msm_rpc_client *client,
			    void *buffer, int in_size)
{
	int rc = 0;
	struct rpc_request_hdr *req;
	u32 procedure;
	u32 accept_status;

	req = (struct rpc_request_hdr *)buffer;
	procedure = be32_to_cpu(req->procedure);

	switch (procedure) {
	case BATTERY_CB_TYPE_PROC:
		accept_status = RPC_ACCEPTSTAT_SUCCESS;
		break;

	default:
		accept_status = RPC_ACCEPTSTAT_PROC_UNAVAIL;
		pr_err("%s: ERROR. procedure (%d) not supported\n",
		       __func__, procedure);
		break;
	}

	msm_rpc_start_accepted_reply(msm_batt_info.batt_client,
			be32_to_cpu(req->xid), accept_status);

	rc = msm_rpc_send_accepted_reply(msm_batt_info.batt_client, 0);
	if (rc)
		pr_err("%s: FAIL: sending reply. rc=%d\n", __func__, rc);

    if (accept_status == RPC_ACCEPTSTAT_SUCCESS) {
	    mutex_lock(&charger_status_lock);
        msm_batt_update_psy_status();
        mutex_unlock(&charger_status_lock);
    }

	return rc;
}

struct batt_info_interface* get_batt_info_if(void)
{
    return msm_batt_info.batt_info_if;
}
EXPORT_SYMBOL(get_batt_info_if);

struct msm_batt_get_volt_ret_data {
    u32 battery_voltage;
};

static int msm_batt_get_volt_ret_func(struct msm_rpc_client *batt_client,
                       void *buf, void *data)
{
    struct msm_batt_get_volt_ret_data *data_ptr, *buf_ptr;

    data_ptr = (struct msm_batt_get_volt_ret_data *)data;
    buf_ptr = (struct msm_batt_get_volt_ret_data *)buf;

    data_ptr->battery_voltage = be32_to_cpu(buf_ptr->battery_voltage);

    return 0;
}

u32 msm_batt_get_vbatt_voltage(void)
{
    int rc;

    struct msm_batt_get_volt_ret_data rep;

    rc = msm_rpc_client_req(msm_batt_info.batt_client,
            BATTERY_READ_MV_PROC,
            NULL, NULL,
            msm_batt_get_volt_ret_func, &rep,
            msecs_to_jiffies(BATT_RPC_TIMEOUT));

    if (rc < 0) {
        pr_err("%s: FAIL: vbatt get volt. rc=%d\n", __func__, rc);
        return 0;
    }

    return rep.battery_voltage;
}
EXPORT_SYMBOL(msm_batt_get_vbatt_voltage);

static int __devinit msm_batt_probe(struct platform_device *pdev)
{
	int rc;
	struct msm_psy_batt_pdata *pdata = pdev->dev.platform_data;

	if (pdev->id != -1) {
		dev_err(&pdev->dev,
			"%s: MSM chipsets Can only support one"
			" battery ", __func__);
		return -EINVAL;
	}

	if (pdata->avail_chg_sources & AC_CHG) {
		rc = power_supply_register(&pdev->dev, &msm_psy_ac);
		if (rc < 0) {
			dev_err(&pdev->dev,
				"%s: power_supply_register failed"
				" rc = %d\n", __func__, rc);
			msm_batt_cleanup();
			return rc;
		}
		msm_batt_info.msm_psy_ac = &msm_psy_ac;
		msm_batt_info.avail_chg_sources |= AC_CHG;
	}

	if (pdata->avail_chg_sources & USB_CHG) {
		rc = power_supply_register(&pdev->dev, &msm_psy_usb);
		if (rc < 0) {
			dev_err(&pdev->dev,
				"%s: power_supply_register failed"
				" rc = %d\n", __func__, rc);
			msm_batt_cleanup();
			return rc;
		}
		msm_batt_info.msm_psy_usb = &msm_psy_usb;
		msm_batt_info.avail_chg_sources |= USB_CHG;
	}

	if (!msm_batt_info.msm_psy_ac && !msm_batt_info.msm_psy_usb) {

		dev_err(&pdev->dev,
			"%s: No external Power supply(AC or USB)"
			"is avilable\n", __func__);
		msm_batt_cleanup();
		return -ENODEV;
	}

    rc = msm_batt_register(BATTERY_LOW, BATTERY_ALL_ACTIVITY,
                   BATTERY_CB_ID_ALL_ACTIV, BATTERY_ALL_ACTIVITY);
	if (rc < 0) {
		dev_err(&pdev->dev,
			"%s: msm_batt_register failed rc = %d\n", __func__, rc);
		msm_batt_cleanup();
		return rc;
	}

	rc =  msm_batt_enable_filter(VBATT_FILTER);

	if (rc < 0) {
		dev_err(&pdev->dev,
			"%s: msm_batt_enable_filter failed rc = %d\n",
			__func__, rc);
		msm_batt_cleanup();
		return rc;
	}
    
    msm_batt_info.batt_info_if = &(pdata->batt_info_if);
    
    wake_lock_init(&msm_batt_info.msm_batt_wakelock, WAKE_LOCK_SUSPEND, "msm_batt");

#ifdef CONFIG_HAS_EARLYSUSPEND
	msm_batt_info.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	msm_batt_info.early_suspend.suspend = msm_batt_early_suspend;
	msm_batt_info.early_suspend.resume = msm_batt_late_resume;
	register_early_suspend(&msm_batt_info.early_suspend);
#endif
    
    msm_batt_info.driver_ready = true;

	return 0;
}

static int __devexit msm_batt_remove(struct platform_device *pdev)
{
	int rc;
	rc = msm_batt_cleanup();

	if (rc < 0) {
		dev_err(&pdev->dev,
			"%s: msm_batt_cleanup  failed rc=%d\n", __func__, rc);
		return rc;
	}
	return 0;
}

static struct platform_driver msm_batt_driver = {
	.probe = msm_batt_probe,
	.remove = __devexit_p(msm_batt_remove),
	.driver = {
		   .name = "msm-battery",
		   .owner = THIS_MODULE,
		   },
};

static int __devinit msm_batt_init_rpc(void)
{
	int rc;

	msm_batt_info.chg_ep =
		msm_rpc_connect_compatible(CHG_RPC_PROG, CHG_RPC_VER_2_2, 0);

	if (msm_batt_info.chg_ep == NULL) {
		pr_err("%s: rpc connect CHG_RPC_PROG = NULL\n", __func__);
		return -ENODEV;
	} else if (IS_ERR(msm_batt_info.chg_ep)) {
		msm_batt_info.chg_ep = msm_rpc_connect_compatible(
				CHG_RPC_PROG, CHG_RPC_VER_1_1, 0);
		msm_batt_info.chg_api_version =  CHG_RPC_VER_1_1;
	} else
		msm_batt_info.chg_api_version =  CHG_RPC_VER_2_2;

	if (IS_ERR(msm_batt_info.chg_ep)) {
		rc = PTR_ERR(msm_batt_info.chg_ep);
		pr_err("%s: FAIL: rpc connect for CHG_RPC_PROG. rc=%d\n",
		       __func__, rc);
		msm_batt_info.chg_ep = NULL;
		return rc;
	}

	/* Get the real 1.x version */
	if (msm_batt_info.chg_api_version == CHG_RPC_VER_1_1)
		msm_batt_info.chg_api_version =
			msm_batt_get_charger_api_version();

	/* Fall back to 1.1 for default */
	if (msm_batt_info.chg_api_version < 0)
		msm_batt_info.chg_api_version = CHG_RPC_VER_1_1;

	msm_batt_info.batt_client =
		msm_rpc_register_client("battery", BATTERY_RPC_PROG,
					BATTERY_RPC_VER_2_1,
					1, msm_batt_cb_func);

	if (msm_batt_info.batt_client == NULL) {
		pr_err("%s: FAIL: rpc_register_client. batt_client=NULL\n",
		       __func__);
		return -ENODEV;
	} else if (IS_ERR(msm_batt_info.batt_client)) {
		msm_batt_info.batt_client =
			msm_rpc_register_client("battery", BATTERY_RPC_PROG,
						BATTERY_RPC_VER_1_1,
						1, msm_batt_cb_func);
		msm_batt_info.batt_api_version =  BATTERY_RPC_VER_1_1;
	} else
		msm_batt_info.batt_api_version =  BATTERY_RPC_VER_2_1;

	if (IS_ERR(msm_batt_info.batt_client)) {
		msm_batt_info.batt_client =
			msm_rpc_register_client("battery", BATTERY_RPC_PROG,
						BATTERY_RPC_VER_5_1,
						1, msm_batt_cb_func);
		msm_batt_info.batt_api_version =  BATTERY_RPC_VER_5_1;
	}
	if (IS_ERR(msm_batt_info.batt_client)) {
		rc = PTR_ERR(msm_batt_info.batt_client);
		pr_err("%s: ERROR: rpc_register_client: rc = %d\n ",
		       __func__, rc);
		msm_batt_info.batt_client = NULL;
		return rc;
	}

	rc = platform_driver_register(&msm_batt_driver);

	if (rc < 0)
		pr_err("%s: FAIL: platform_driver_register. rc = %d\n",
		       __func__, rc);

	return rc;
}

static int __init msm_batt_init(void)
{
	int rc;

	pr_debug("%s: enter\n", __func__);

	rc = msm_batt_init_rpc();

	if (rc < 0) {
		pr_err("%s: FAIL: msm_batt_init_rpc.  rc=%d\n", __func__, rc);
		msm_batt_cleanup();
		return rc;
	}

	pr_info("%s: Charger/Battery = 0x%08x/0x%08x (RPC version)\n",
		__func__, msm_batt_info.chg_api_version,
		msm_batt_info.batt_api_version);

	return 0;
}

static void __exit msm_batt_exit(void)
{
	platform_driver_unregister(&msm_batt_driver);
}

module_init(msm_batt_init);
module_exit(msm_batt_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Kiran Kandi, Qualcomm Innovation Center, Inc.");
MODULE_DESCRIPTION("Battery driver for Qualcomm MSM chipsets.");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:msm_battery");
