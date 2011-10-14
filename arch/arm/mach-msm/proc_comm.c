/* arch/arm/mach-msm/proc_comm.c
 *
 * Copyright (C) 2007-2008 Google, Inc.
 * Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 * Author: Brian Swetland <swetland@google.com>
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

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <mach/msm_iomap.h>
#include <mach/system.h>

#include "proc_comm.h"
#include "smd_private.h"

#if defined(CONFIG_ARCH_MSM7X30)
#define MSM_TRIG_A2M_PC_INT (writel(1 << 6, MSM_GCC_BASE + 0x8))
#elif defined(CONFIG_ARCH_MSM8X60)
#define MSM_TRIG_A2M_PC_INT (writel(1 << 5, MSM_GCC_BASE + 0x8))
#else
#define MSM_TRIG_A2M_PC_INT (writel(1, MSM_CSR_BASE + 0x400 + (6) * 4))
#endif

static inline void notify_other_proc_comm(void)
{
	MSM_TRIG_A2M_PC_INT;
}

#define APP_COMMAND 0x00
#define APP_STATUS  0x04
#define APP_DATA1   0x08
#define APP_DATA2   0x0C

#define MDM_COMMAND 0x10
#define MDM_STATUS  0x14
#define MDM_DATA1   0x18
#define MDM_DATA2   0x1C

static DEFINE_SPINLOCK(proc_comm_lock);

/* Poll for a state change, checking for possible
 * modem crashes along the way (so we don't wait
 * forever while the ARM9 is blowing up.
 *
 * Return an error in the event of a modem crash and
 * restart so the msm_proc_comm() routine can restart
 * the operation from the beginning.
 */
static int proc_comm_wait_for(unsigned addr, unsigned value)
{
	while (1) {
		if (readl(addr) == value)
			return 0;

		if (smsm_check_for_modem_crash())
			return -EAGAIN;

		udelay(5);
	}
}

void msm_proc_comm_reset_modem_now(void)
{
	unsigned base = (unsigned)MSM_SHARED_RAM_BASE;
	unsigned long flags;

	spin_lock_irqsave(&proc_comm_lock, flags);

again:
	if (proc_comm_wait_for(base + MDM_STATUS, PCOM_READY))
		goto again;

	writel(PCOM_RESET_MODEM, base + APP_COMMAND);
	writel(0, base + APP_DATA1);
	writel(0, base + APP_DATA2);

	spin_unlock_irqrestore(&proc_comm_lock, flags);

	notify_other_proc_comm();

	return;
}
EXPORT_SYMBOL(msm_proc_comm_reset_modem_now);

int msm_proc_comm(unsigned cmd, unsigned *data1, unsigned *data2)
{
	unsigned base = (unsigned)MSM_SHARED_RAM_BASE;
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&proc_comm_lock, flags);

again:
	if (proc_comm_wait_for(base + MDM_STATUS, PCOM_READY))
		goto again;

	writel(cmd, base + APP_COMMAND);
	writel(data1 ? *data1 : 0, base + APP_DATA1);
	writel(data2 ? *data2 : 0, base + APP_DATA2);

	notify_other_proc_comm();

	if (proc_comm_wait_for(base + APP_COMMAND, PCOM_CMD_DONE))
		goto again;

	if (readl(base + APP_STATUS) == PCOM_CMD_SUCCESS) {
		if (data1)
			*data1 = readl(base + APP_DATA1);
		if (data2)
			*data2 = readl(base + APP_DATA2);
		ret = 0;
	} else {
		ret = -EIO;
	}

	writel(PCOM_CMD_IDLE, base + APP_COMMAND);

	spin_unlock_irqrestore(&proc_comm_lock, flags);
	return ret;
}
EXPORT_SYMBOL(msm_proc_comm);

//SW2-5-1-MP-DbgCfgTool-00+[
int msm_proc_comm_oem_n(unsigned cmd, unsigned *data1, unsigned *data2, unsigned *cmd_parameter, int u32_para_size)
{
	unsigned base = (unsigned)MSM_SHARED_RAM_BASE;
	unsigned long flags;
	int ret;
	size_t sizeA, sizeB;
	smem_oem_cmd_data *cmd_buf;
       void* test;

	/* get share memory command address dynamically */
	int size;
	sizeA=40;
	sizeB=64;
	cmd_buf = smem_get_entry(SMEM_OEM_CMD_DATA, &size);

	if( (cmd_buf == 0) || (size < u32_para_size) )
	{
		printk(KERN_ERR "[SMEM_PROC_COMM] %s() LINE:%d, Can't get shared memory entry.(size %d,u32_para_size %d)\n", __func__, __LINE__, size, u32_para_size); 
		return -EINVAL;
	}

	test= (unsigned*)&cmd_buf->cmd_data.cmd_parameter[0];

	if( (cmd_parameter == NULL) || (u32_para_size == 0) )
	{
		printk(KERN_ERR "[SMEM_PROC_COMM] %s() LINE:%d, ERROR: u32_para_size %d.\n", __func__, __LINE__, u32_para_size); 
		return -EINVAL;
	}

	spin_lock_irqsave(&proc_comm_lock, flags);

again:
	if (proc_comm_wait_for(base + MDM_STATUS, PCOM_READY))
		goto again;

	writel(cmd, base + APP_COMMAND);
	writel(data1 ? *data1 : 0, base + APP_DATA1);
	writel(data2 ? *data2 : 0, base + APP_DATA2);

	// Set the parameter of OEM_CMD1
	cmd_buf->cmd_data.check_flag = smem_oem_locked_flag;
	memcpy(cmd_buf->cmd_data.cmd_parameter,cmd_parameter,sizeof(unsigned)*u32_para_size);
	notify_other_proc_comm();

	if (proc_comm_wait_for(base + APP_COMMAND, PCOM_CMD_DONE))
	{
		goto again;
	}
	
	writel(PCOM_CMD_IDLE, base + APP_COMMAND);

	/* read response value, Hanson Lin */
	while(!(cmd_buf->return_data.check_flag & smem_oem_unlocked_flag))
	{
		//waiting
	}

	/* Div6-D1-SY-FIHDBG-00*{ 
	 * Due to review return value in AMSS, we decide to modify the mask 
	 * from 0x1111 to 0xFFFF to identify the correct error type.
	 * Notice: Need to review the "check_flag" usage in both mARM and aARM.
	 */	
	ret = (cmd_buf->return_data.check_flag & 0xFFFF);
	/* Div6-D1-SY-FIHDBG-00*} */

	if(!ret)
	{
		*data2 = cmd_buf->return_data.return_value[0];

		/* Copy the returned value back to user "cmd_parameter" */
		memcpy(cmd_parameter, cmd_buf->return_data.return_value, sizeof(unsigned) * u32_para_size);
	}
	spin_unlock_irqrestore(&proc_comm_lock, flags);
	return ret;
}
EXPORT_SYMBOL(msm_proc_comm_oem_n);
//SW2-5-1-MP-DbgCfgTool-00+]

//Div2-SW2-BSP,JOE HSU,+++
int msm_proc_comm_oem(unsigned cmd, unsigned *data1, unsigned *data2, unsigned *cmd_parameter)
{
	unsigned base = (unsigned)MSM_SHARED_RAM_BASE;
	unsigned long flags;
	unsigned int size;
	int ret;
	
	smem_oem_cmd_data *cmd_buf;

//Div2-SW2-BSP, JOE HSU,rename smem_mem_type
        cmd_buf = (smem_oem_cmd_data *)smem_get_entry(SMEM_OEM_CMD_DATA, &size);

	spin_lock_irqsave(&proc_comm_lock, flags);

again:
	if (proc_comm_wait_for(base + MDM_STATUS, PCOM_READY))
		goto again;

	writel(cmd, base + APP_COMMAND);
	writel(data1 ? *data1 : 0, base + APP_DATA1);
	writel(data2 ? *data2 : 0, base + APP_DATA2);
	
	// Set the parameter of OEM_CMD1
	cmd_buf->cmd_data.check_flag = smem_oem_locked_flag;
	cmd_buf->cmd_data.cmd_parameter[0] = cmd_parameter[0];
	cmd_buf->cmd_data.cmd_parameter[1] = cmd_parameter[1];

	notify_other_proc_comm();

	if (proc_comm_wait_for(base + APP_COMMAND, PCOM_CMD_DONE))
		goto again;

	if (readl(base + APP_STATUS) == PCOM_CMD_SUCCESS) {
		if (data1)
			*data1 = readl(base + APP_DATA1);
		if (data2)
			*data2 = readl(base + APP_DATA2);
		ret = 0;
	} else {
		ret = -EIO;
	}

	writel(PCOM_CMD_IDLE, base + APP_COMMAND);
	
	while(!(cmd_buf->return_data.check_flag & smem_oem_unlocked_flag))
    	{
                //waiting
    	}
    
	ret = (cmd_buf->return_data.check_flag & 0x1111);
    	if(!ret)
    	{
            //Div6-D1-JL-FixReadNV-00+{
            //Default code
            //*data2 = cmd_buf->return_data.return_value[0];
            // Add for Software Upgrading Tools
            if(*data1==SMEM_PROC_COMM_OEM_NV_READ)
            {
                printk(KERN_ERR "[SMEM_PROC_COMM] USBDBG %s() LINE:%d, \n", __func__, __LINE__); 
                memcpy(data2,&cmd_buf->return_data.return_value[1],128);
            }
            else
            {
                *data2 = cmd_buf->return_data.return_value[0];
            }
            //Div6-D1-JL-FixReadNV-00+}
        }

	spin_unlock_irqrestore(&proc_comm_lock, flags);
	return ret;
}
EXPORT_SYMBOL(msm_proc_comm_oem);
//Div2-SW2-BSP,JOE HSU,---

//SW2-5-1-MP-DbgCfgTool-00+[
/*--------------------------------------------------------------------------*
 * Function    : fih_read_fihdbg_config_nv
 *
 * Description :
 *     Read fih debug configuration settings from nv item (NV_FIHDBG_I).
 *
 * Parameters  :
 *     String of 16 bytes fih debug configuration setting array in 
 *     unsigned char.
 *
 * Return value: Integer
 *     Zero     - Successful
 *     Not zero - Fail
 *--------------------------------------------------------------------------*/
int fih_read_fihdbg_config_nv( unsigned char* fih_debug )
{
    unsigned smem_response;
    uint32_t oem_cmd = SMEM_PROC_COMM_OEM_NV_READ;
    unsigned int cmd_para[FIH_DEBUG_CMD_DATA_SIZE];
    int ret = 0;

    printk(KERN_INFO "[SMEM_PROC_COMM] %s() LINE:%d \n", __func__, __LINE__);
    if ( fih_debug == NULL )
    {
        printk(KERN_ERR "[SMEM_PROC_COMM] %s() LINE:%d, ERROR: fih_debug is NULL.\n", __func__, __LINE__); 
        ret = -EINVAL;
    }
    else
    {
        cmd_para[0] = NV_FIHDBG_I;
        ret = msm_proc_comm_oem_n(PCOM_CUSTOMER_CMD1, &oem_cmd, &smem_response, cmd_para, FIH_DEBUG_CMD_DATA_SIZE);
        if(ret == 0)
        {
            memcpy( fih_debug, &cmd_para[1], FIH_DEBUG_CFG_LEN );
        }
        else
        {
            printk(KERN_ERR "[SMEM_PROC_COMM] %s() LINE:%d, ERROR: ret %d, cmd_para[0] %d.\n", __func__, __LINE__, ret, cmd_para[0]); 
        }
    }
    return ret;
}
EXPORT_SYMBOL(fih_read_fihdbg_config_nv);
 
/*--------------------------------------------------------------------------*
 * Function    : fih_write_fihdbg_config_nv
 *
 * Description :
 *     Write fih debug configuration settings into nv item (NV_FIHDBG_I).
 *
 * Parameters  :
 *     String of 16 bytes fih debug configuration setting array in 
 *     unsigned char.
 *
 * Return value: Integer
 *     Zero     - Successful
 *     Not zero - Fail
 *--------------------------------------------------------------------------*/
int fih_write_fihdbg_config_nv( unsigned char* fih_debug )
{
    unsigned smem_response;
    uint32_t oem_cmd = SMEM_PROC_COMM_OEM_NV_WRITE;
    unsigned int cmd_para[FIH_DEBUG_CMD_DATA_SIZE];
    int ret = 0;

    printk(KERN_INFO "[SMEM_PROC_COMM] %s() LINE:%d \n", __func__, __LINE__);
    if ( fih_debug == NULL )
    {
        printk(KERN_ERR "[SMEM_PROC_COMM] %s() LINE:%d, ERROR: fih_debug is NULL.\n", __func__, __LINE__); 
        ret = -EINVAL;
    }
    else
    {
        cmd_para[0] = NV_FIHDBG_I;
        memcpy( &cmd_para[1], fih_debug, FIH_DEBUG_CFG_LEN );
        ret = msm_proc_comm_oem_n(PCOM_CUSTOMER_CMD1, &oem_cmd, &smem_response, cmd_para, FIH_DEBUG_CMD_DATA_SIZE);
        if(ret != 0)
        {
            printk(KERN_ERR "[SMEM_PROC_COMM] %s() LINE:%d, ERROR: ret %d, cmd_para[0] %d.\n", __func__, __LINE__, ret, cmd_para[0]); 
        }
    }
    return ret;
}
EXPORT_SYMBOL(fih_write_fihdbg_config_nv);
//SW2-5-1-MP-DbgCfgTool-00+]

int msm_proc_comm_oem_multi(unsigned cmd, unsigned *data1, unsigned *data2, unsigned *cmd_parameter, int number)
{
    unsigned base = (unsigned)MSM_SHARED_RAM_BASE;
    unsigned long flags;
    unsigned int size;
    int ret, index;

    smem_oem_cmd_data *cmd_buf;
//Div2-SW2-BSP, JOE HSU,rename smem_mem_type    
    cmd_buf = (smem_oem_cmd_data *)smem_get_entry(SMEM_OEM_CMD_DATA, &size);
    
    printk(KERN_INFO "%s: 0x%08x\n", __func__, (unsigned)cmd_buf);

    spin_lock_irqsave(&proc_comm_lock, flags);

again:
    if (proc_comm_wait_for(base + MDM_STATUS, PCOM_READY))
        goto again;

    writel(cmd, base + APP_COMMAND);
    writel(data1 ? *data1 : 0, base + APP_DATA1);
    writel(data2 ? *data2 : 0, base + APP_DATA2);
                        
    // Set the parameter of OEM_CMD1
    cmd_buf->cmd_data.check_flag = smem_oem_locked_flag;
    for( index = 0 ; index < number ; index++)
        cmd_buf->cmd_data.cmd_parameter[index] = cmd_parameter[index];
                        
    notify_other_proc_comm();
                        
    if (proc_comm_wait_for(base + APP_COMMAND, PCOM_CMD_DONE))
        goto again;
                        
    if (readl(base + APP_STATUS) == PCOM_CMD_SUCCESS) {
        if (data1)
            *data1 = readl(base + APP_DATA1);
        if (data2)
            *data2 = readl(base + APP_DATA2);
        
        ret = 0;
    } else {
        ret = -EIO;
    }
                        
    for (index = 0; index < number; index++) {
        cmd_parameter[index] = cmd_buf->return_data.return_value[index];
    }

    writel(PCOM_CMD_IDLE, base + APP_COMMAND);
                        
    while(!(cmd_buf->return_data.check_flag & smem_oem_unlocked_flag)) {
        //waiting
        mdelay(100);
        printk(KERN_INFO "%s: wait...... 0x%04x\n", __func__, cmd_buf->return_data.check_flag);
    }

    ret = (cmd_buf->return_data.check_flag & 0x1111);
    if(!ret) {
        *data2 = cmd_buf->return_data.return_value[0];
    }

    spin_unlock_irqrestore(&proc_comm_lock, flags);
    return ret;
}
EXPORT_SYMBOL(msm_proc_comm_oem_multi);

int32_t proc_comm_phone_online(void)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_CHG_MODE;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[2];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = 5;
    
    printk(KERN_INFO "%s before call msm_proc_comm_oem_multi\n", __func__);
    
    msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1,
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    1);
    
    return 0;
}
EXPORT_SYMBOL(proc_comm_phone_online);

int proc_comm_phone_setpref(void)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_SET_PREF;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[2];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = 13;
    
    printk(KERN_INFO "%s before call msm_proc_comm_oem_multi\n", __func__);
    msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1,
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    1);

    return 0;
}
EXPORT_SYMBOL(proc_comm_phone_setpref);

int proc_comm_phone_dialemergency(void)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_DIAL_EMERGENCY;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[2];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = 5;
    
    printk(KERN_INFO "%s before call msm_proc_comm_oem_multi\n", __func__);
    msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1,
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    0);

    return 0;
}
EXPORT_SYMBOL(proc_comm_phone_dialemergency);

int32_t proc_comm_phone_getsimstatus(void)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_GET_CARD_MODE;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[2];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = 5;
    
    printk(KERN_INFO "%s before call msm_proc_comm_oem_multi\n", __func__);
    msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1,
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    1);

    return data[0];
}
EXPORT_SYMBOL(proc_comm_phone_getsimstatus);

void proc_comm_ftm_product_id_write(unsigned* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_WRITE;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[11];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)50001;
    
    memcpy(&data[1], buf, 40);
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    11)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
    
    printk(KERN_INFO "proc_comm_ftm_product_id_write() '%x %x %x %x %x'\n", data[1], data[2], data[3], data[4], data[5]);
}
EXPORT_SYMBOL(proc_comm_ftm_product_id_write);

void proc_comm_ftm_product_id_read(unsigned* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_READ;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[11];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)50001;
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    11)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
    
    memcpy(buf, &data[1], 40);
    
    printk(KERN_INFO "proc_comm_ftm_product_id_read()\n");
}
EXPORT_SYMBOL(proc_comm_ftm_product_id_read);

void proc_comm_ftm_imei_write(unsigned* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_WRITE;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[10];
    
    //
    char data1[15] = {0};
    memcpy(data1, buf, 15);
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)550;	// NV ID IMEI
    //
    data[1] = 0x8;
    data[2] = ((data1[0] << 4) & 0xf0) | 0xa;
    
    data[3] = ((data1[2] << 4) & 0xf0) | (data1[1] & 0xf);
    data[4] = ((data1[4] << 4) & 0xf0) | (data1[3] & 0xf);
    data[5] = ((data1[6] << 4) & 0xf0) | (data1[5] & 0xf);
    data[6] = ((data1[8] << 4) & 0xf0) | (data1[7] & 0xf);
    data[7] = ((data1[10] << 4) & 0xf0) | (data1[9] & 0xf);
    data[8] = ((data1[12] << 4) & 0xf0) | (data1[11] & 0xf);
    data[9] = ((data1[14] << 4) & 0xf0) | (data1[13] & 0xf);
    
    printk(KERN_INFO "proc_comm_ftm_imei_write() '0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x'\n", data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9]);
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    10)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
}
EXPORT_SYMBOL(proc_comm_ftm_imei_write);

void proc_comm_ftm_imei_read(unsigned* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_READ;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[10];
    char data1[15] = {0};
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)550;	// NV ID IMEI
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    10)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
    
    data1[0] = (data[2] & 0xf0) >> 4;
    data1[1] = data[3] & 0xf;
    data1[2] = (data[3] & 0xf0) >> 4;
    data1[3] = data[4] & 0xf;
    data1[4] = (data[4] & 0xf0) >> 4;
    data1[5] = data[5] & 0xf;
    data1[6] = (data[5] & 0xf0) >> 4;
    data1[7] = data[6] & 0xf;
    data1[8] = (data[6] & 0xf0) >> 4;
    data1[9] = data[7] & 0xf;
    data1[10] = (data[7] & 0xf0) >> 4;
    data1[11] = data[8] & 0xf;
    data1[12] = (data[8] & 0xf0) >> 4;
    data1[13] = data[9] & 0xf;
    data1[14] = (data[9] & 0xf0) >> 4;
    
    memcpy(buf, data1, 15);
    
    printk(KERN_INFO "proc_comm_ftm_imei_read() IMEI = '%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d'\n", data1[0], data1[1], data1[2], data1[3], data1[4], data1[5], data1[6], data1[7], data1[8], data1[9], data1[10], data1[11], data1[12], data1[13], data1[14]);
}
EXPORT_SYMBOL(proc_comm_ftm_imei_read);

//FIH, godfrey, 20100505++
void proc_comm_ftm_bdaddr_write(char* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_WRITE;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    int32_t data[3];

	printk(KERN_INFO "proc_comm_ftm_bdaddr_write() '0x%x:%x:%x:%x:%x:%x'\n", buf[5], buf[4], buf[3], buf[2], buf[1], buf[0]);
   
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)447;	// NV ID IMEI NV_BD_ADDR_I
	memcpy((void *)&data[1],buf,6);
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    3)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
}
EXPORT_SYMBOL(proc_comm_ftm_bdaddr_write);

void proc_comm_ftm_bdaddr_read(char * buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_READ;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    int32_t data[3];

    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)447;	// NV ID IMEI NV_BD_ADDR_I
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    3)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
    
    memcpy(buf, &data[1], sizeof(char)*6);
    
    printk(KERN_INFO "proc_comm_ftm_bdaddr_read() '0x%x:%x:%x:%x:%x:%x'\n", buf[5], buf[4],buf[3], buf[2],buf[1], buf[0]);
}
EXPORT_SYMBOL(proc_comm_ftm_bdaddr_read);
//FIH, godfrey, 20100505--

//Div6-PT2-Peripheral-CD-WIFI_FTM_TEST-02+[
void proc_comm_ftm_wlanaddr_write(char* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_WRITE;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    int32_t data[3];

	printk(KERN_INFO "proc_comm_ftm_wlanaddr_write() '0x%x:%x:%x:%x:%x:%x'\n", buf[5], buf[4], buf[3], buf[2], buf[1], buf[0]);
   
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)4678;	// NV_WLAN_MAC_ADDRESS_I
	memcpy((void *)&data[1],buf,6);
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    3)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
}
EXPORT_SYMBOL(proc_comm_ftm_wlanaddr_write);
//Div2-SW6-Conn-JC-WiFi-GetMacFromNV-01+{
int proc_comm_ftm_wlanaddr_read(char * buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_READ;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    int32_t data[3] = {0};
    int32_t ret = 0;

    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)4678;	// NV_WLAN_MAC_ADDRESS_I

    ret = msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    3);
     if (ret != 0) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
	  return ret;
    }
    
    memcpy(buf, &data[1], sizeof(char)*6);
    if ((data[1] == 0) && (data[2] == 0)) ret = -1;
    printk(KERN_INFO "proc_comm_ftm_wlanaddr_read() '0x%x:%x:%x:%x:%x:%x'\n", buf[5], buf[4],buf[3], buf[2],buf[1], buf[0]);
    return ret;
}
//Div2-SW6-JC-Conn-WiFi-GetMacFromNV-01+}
EXPORT_SYMBOL(proc_comm_ftm_wlanaddr_read);
//Div6-PT2-Peripheral-CD-WIFI_FTM_TEST-02+]

/* FIHTDC, Audi, FTM Charging And Backup Battery Test{ */
int proc_comm_config_chg_current(bool on, int curr)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_CONFIG_CHG_CURRENT;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[2];
    int32_t ret = 0;
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)on;
    data[1] = curr;
    
    ret = msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    2);
    if (ret != 0){
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_CONFIG_CHG_CURRENT FAILED!!\n",
                __func__);
    }
    
    return ret;
}
EXPORT_SYMBOL(proc_comm_config_chg_current);

int proc_comm_config_coin_cell(int vset, int *voltage, int status)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_CONFIG_COIN_CELL;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[2];
    int32_t ret = 0;

    memset((void*)data, 0x00, sizeof(data));
    data[0] = vset;
    //status = 0:off / 1:on
    data[1] = status;
    
    ret = msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1,
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    2); 
    if (ret != 0){
        printk(KERN_ERR 
                "%s: SMEM_PROC_COMM_OEM_CONFIG_COIN_CELL FAILED!!\n",
                __func__);
    }
    
    *voltage = data[0];
    
    return ret; //mV
}
EXPORT_SYMBOL(proc_comm_config_coin_cell);
/* } FIHTDC, Audi, FTM Charging And Backup Battery Test */

/* FIHTDC, Div2-SW2-BSP CHHsieh, NVBackup { */
int proc_comm_ftm_backup_unique_nv(void)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_BACKUP_UNIQUE_NV;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[2];
    int32_t ret = 0;
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = 1;
    
    ret = msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    0);
    if (ret != 0){
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_BACKUP_UNIQUE_NV FAILED!!\n",
                __func__);
                
    	return ret;
    }
    
    printk(KERN_INFO "proc_comm_ftm_backup_unique_nv()\n");
    
    return ret;
}
EXPORT_SYMBOL(proc_comm_ftm_backup_unique_nv);

void proc_comm_ftm_hw_reset(void)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_HW_RESET;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[2];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = 1;
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    0)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_HW_RESET FAILED!!\n",
                __func__);
    }
    
    printk(KERN_INFO "proc_comm_ftm_hw_reset()\n");
}
EXPORT_SYMBOL(proc_comm_ftm_hw_reset);

int proc_comm_ftm_nv_flag(void)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_BACKUP_NV_FLAG;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[2];
    int32_t ret = 0;
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = 1;
    
    ret = msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    0);
    if (ret != 0){
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_BACKUP_NV_FLAG FAILED!!\n",
                __func__);
                
    	return ret;
    }
    
    printk(KERN_INFO "proc_comm_ftm_nv_flag()\n");
    
    return ret;
}
EXPORT_SYMBOL(proc_comm_ftm_nv_flag);
/* } FIHTDC, Div2-SW2-BSP CHHsieh, NVBackup */

int proc_comm_touch_id_read(void)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_READ;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[2];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)3314;
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    2)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
    
    printk(KERN_INFO "%s: result is %d\n", __func__, data[1]);

    return data[1];
}
EXPORT_SYMBOL(proc_comm_touch_id_read);

//+++ FIH; Louis; 2010/11/9
void proc_comm_compass_param_read(int *buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_READ;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[7];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)50032;
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    7)) {
        printk(KERN_ERR "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n", __func__);
    }
    
	//printk(KERN_INFO "%s: result is %d, %d, %d, %d, %d, %d\n", __func__, data[1], data[2], data[3], data[4], data[5], data[6]);

	memcpy(buf, &data[1], sizeof(int)*6);
    
    //printk(KERN_INFO "%s: result is %d, %d, %d, %d, %d, %d\n", __func__, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
}
EXPORT_SYMBOL(proc_comm_compass_param_read);

void proc_comm_compass_param_write(int* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_WRITE;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
	int32_t data[7];

	//printk(KERN_INFO "%s1: result is %d, %d, %d, %d, %d, %d\n", __func__, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
   
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)50032;
	memcpy((void *)&data[1], buf, sizeof(int)*6);

	//printk(KERN_INFO "%s2: result is %d, %d, %d, %d, %d, %d\n", __func__, data[1], data[2], data[3], data[4], data[5], data[6]);
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    7)) {
        printk(KERN_ERR "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n", __func__);
    }
}
EXPORT_SYMBOL(proc_comm_compass_param_write);

// ---------------------------------------------------------------------------
//
// ---------------------------------------------------------------------------
int proc_comm_fuse_boot_set(void)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_FUSE_BOOT;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[2];
    int32_t ret = 0;
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = 1;
    
    ret = msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    0);
    if (ret != 0){
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FUSE_BOOT FAILED!!\n",
                __func__);
                
    	return ret;
    }
    
    printk(KERN_INFO "proc_comm_fuse_boot_set()\n");
    
    return ret;
}
EXPORT_SYMBOL(proc_comm_fuse_boot_set);

int proc_comm_fuse_boot_get(void)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_FUSE_BOOT;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[2];
    int32_t ret = 0;
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = 0;
    
    ret = msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    0);
    if (ret != 0){
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FUSE_BOOT FAILED!!\n",
                __func__);
                
    	return ret;
    }
    
    printk(KERN_INFO "proc_comm_fuse_boot_get()\n");
    
    return ret;
}
EXPORT_SYMBOL(proc_comm_fuse_boot_get);

//Div2D5-LC-BSP-Porting_OTA_SDDownload-00 +[
int proc_comm_alloc_sd_dl_smem(int action_id)
{
	unsigned smem_response = 0;
	uint32_t oem_cmd = SMEM_PROC_COMM_OEM_ALLOC_SD_DL_INFO;
    unsigned int cmd_parameter = action_id;
	/* cmd_parameter setting */

    return msm_proc_comm_oem(PCOM_CUSTOMER_CMD1, &oem_cmd, &smem_response, &cmd_parameter );
}
EXPORT_SYMBOL(proc_comm_alloc_sd_dl_smem);

int fih_write_nv4719( unsigned int* fih_debug )
{
    unsigned smem_response;
    uint32_t oem_cmd = SMEM_PROC_COMM_OEM_NV_WRITE;
    unsigned int cmd_para[4];
    int ret = 0;

    printk(KERN_INFO "[SMEM_PROC_COMM] %s() LINE:%d \n", __func__, __LINE__);
    if ( fih_debug == NULL )
    {
        printk(KERN_ERR "[SMEM_PROC_COMM] %s() LINE:%d, ERROR: fih_debug is NULL.\n", __func__, __LINE__); 
        ret = -EINVAL;
    }
    else
    {
        cmd_para[0] = fih_debug[0];
        cmd_para[1] = fih_debug[1];
        ret = msm_proc_comm_oem_n(PCOM_CUSTOMER_CMD1, &oem_cmd, &smem_response, cmd_para, 2);
        if(ret != 0)
        {
            printk(KERN_ERR "[SMEM_PROC_COMM] %s() LINE:%d, ERROR: ret %d, cmd_para[0] %d.\n", __func__, __LINE__, ret, cmd_para[0]); 
        }
    }
    return ret;
}
EXPORT_SYMBOL(fih_write_nv4719);
//Div2D5-LC-BSP-Porting_OTA_SDDownload-00 +]

/* FIHTDC, CHHsieh, PMIC Unlock { */
int proc_comm_ftm_pmic_unlock(void)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_PMIC_UNLOCK;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[2];
    int32_t ret = 0;
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = 1;
    
    ret = msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    0);
    if (ret != 0){
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_PMIC_UNLOCK FAILED!!\n",
                __func__);
                
    	return ret;
    }
    
    printk(KERN_INFO "proc_comm_ftm_pmic_unlock()\n");
    
    return ret;
}
EXPORT_SYMBOL(proc_comm_ftm_pmic_unlock);
/* } FIHTDC, CHHsieh, PMIC Unlock */

/* FIHTDC, CHHsieh, MEID Get/Set { */
typedef struct SHA1Context
{
    unsigned Message_Digest[5]; /* Message Digest (output)          */

    unsigned Length_Low;        /* Message length in bits           */
    unsigned Length_High;       /* Message length in bits           */

    unsigned char Message_Block[64]; /* 512-bit message blocks      */
    int Message_Block_Index;    /* Index into message block array   */

    int Computed;               /* Is the digest computed?          */
    int Corrupted;              /* Is the message digest corruped?  */
} SHA1Context;

#define SHA1CircularShift(bits,word) \
                ((((word) << (bits)) & 0xFFFFFFFF) | \
                ((word) >> (32-(bits))))

void SHA1ProcessMessageBlock(SHA1Context *);
void SHA1PadMessage(SHA1Context *);

void SHA1Reset(SHA1Context *context)
{
    context->Length_Low             = 0;
    context->Length_High            = 0;
    context->Message_Block_Index    = 0;

    context->Message_Digest[0]      = 0x67452301;
    context->Message_Digest[1]      = 0xEFCDAB89;
    context->Message_Digest[2]      = 0x98BADCFE;
    context->Message_Digest[3]      = 0x10325476;
    context->Message_Digest[4]      = 0xC3D2E1F0;

    context->Computed   = 0;
    context->Corrupted  = 0;
}

int SHA1Result(SHA1Context *context)
{

    if (context->Corrupted)
    {
        return 0;
    }

    if (!context->Computed)
    {
        SHA1PadMessage(context);
        context->Computed = 1;
    }

    return 1;
}

void SHA1Input(     SHA1Context         *context,
                    const unsigned char *message_array,
                    unsigned            length)
{
    if (!length)
    {
        return;
    }

    if (context->Computed || context->Corrupted)
    {
        context->Corrupted = 1;
        return;
    }

    while(length-- && !context->Corrupted)
    {
        context->Message_Block[context->Message_Block_Index++] =
                                                (*message_array & 0xFF);

        context->Length_Low += 8;
        /* Force it to 32 bits */
        context->Length_Low &= 0xFFFFFFFF;
        if (context->Length_Low == 0)
        {
            context->Length_High++;
            /* Force it to 32 bits */
            context->Length_High &= 0xFFFFFFFF;
            if (context->Length_High == 0)
            {
                /* Message is too long */
                context->Corrupted = 1;
            }
        }

        if (context->Message_Block_Index == 64)
        {
            SHA1ProcessMessageBlock(context);
        }

        message_array++;
    }
}

void SHA1ProcessMessageBlock(SHA1Context *context)
{
    const unsigned K[] =            /* Constants defined in SHA-1   */      
    {
        0x5A827999,
        0x6ED9EBA1,
        0x8F1BBCDC,
        0xCA62C1D6
    };
    int         t;                  /* Loop counter                 */
    unsigned    temp;               /* Temporary word value         */
    unsigned    W[80];              /* Word sequence                */
    unsigned    A, B, C, D, E;      /* Word buffers                 */

    /*
     *  Initialize the first 16 words in the array W
     */
    for(t = 0; t < 16; t++)
    {
        W[t] = ((unsigned) context->Message_Block[t * 4]) << 24;
        W[t] |= ((unsigned) context->Message_Block[t * 4 + 1]) << 16;
        W[t] |= ((unsigned) context->Message_Block[t * 4 + 2]) << 8;
        W[t] |= ((unsigned) context->Message_Block[t * 4 + 3]);
    }

    for(t = 16; t < 80; t++)
    {
       W[t] = SHA1CircularShift(1,W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16]);
    }

    A = context->Message_Digest[0];
    B = context->Message_Digest[1];
    C = context->Message_Digest[2];
    D = context->Message_Digest[3];
    E = context->Message_Digest[4];

    for(t = 0; t < 20; t++)
    {
        temp =  SHA1CircularShift(5,A) +
                ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 20; t < 40; t++)
    {
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[1];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 40; t < 60; t++)
    {
        temp = SHA1CircularShift(5,A) +
               ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    for(t = 60; t < 80; t++)
    {
        temp = SHA1CircularShift(5,A) + (B ^ C ^ D) + E + W[t] + K[3];
        temp &= 0xFFFFFFFF;
        E = D;
        D = C;
        C = SHA1CircularShift(30,B);
        B = A;
        A = temp;
    }

    context->Message_Digest[0] =
                        (context->Message_Digest[0] + A) & 0xFFFFFFFF;
    context->Message_Digest[1] =
                        (context->Message_Digest[1] + B) & 0xFFFFFFFF;
    context->Message_Digest[2] =
                        (context->Message_Digest[2] + C) & 0xFFFFFFFF;
    context->Message_Digest[3] =
                        (context->Message_Digest[3] + D) & 0xFFFFFFFF;
    context->Message_Digest[4] =
                        (context->Message_Digest[4] + E) & 0xFFFFFFFF;

    context->Message_Block_Index = 0;
}

void SHA1PadMessage(SHA1Context *context)
{
    /*
     *  Check to see if the current message block is too small to hold
     *  the initial padding bits and length.  If so, we will pad the
     *  block, process it, and then continue padding into a second
     *  block.
     */
    if (context->Message_Block_Index > 55)
    {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while(context->Message_Block_Index < 64)
        {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }

        SHA1ProcessMessageBlock(context);

        while(context->Message_Block_Index < 56)
        {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }
    }
    else
    {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while(context->Message_Block_Index < 56)
        {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }
    }

    /*
     *  Store the message length as the last 8 octets
     */
    context->Message_Block[56] = (context->Length_High >> 24) & 0xFF;
    context->Message_Block[57] = (context->Length_High >> 16) & 0xFF;
    context->Message_Block[58] = (context->Length_High >> 8) & 0xFF;
    context->Message_Block[59] = (context->Length_High) & 0xFF;
    context->Message_Block[60] = (context->Length_Low >> 24) & 0xFF;
    context->Message_Block[61] = (context->Length_Low >> 16) & 0xFF;
    context->Message_Block[62] = (context->Length_Low >> 8) & 0xFF;
    context->Message_Block[63] = (context->Length_Low) & 0xFF;

    SHA1ProcessMessageBlock(context);
}

int proc_comm_ftm_meid_write(char* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_WRITE;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[4];
    int32_t ret = 0;
    char data1[7] = {0};
    SHA1Context sha;
    
    memcpy(data1, buf, 7);
    
    memset((void*)data, 0x00000000, sizeof(data));
    data[0] = (uint32_t)1943;	// NV ID IMEI
    //
    data[1] = ( ((data1[3] << 24) & 0xff000000) | ((data1[4] << 16) & 0xff0000) |
    			((data1[5] <<  8) & 0xff00)     |  (data1[6]        & 0xff) );
    data[2] = ( ((data1[0] << 16) & 0xff0000)   | ((data1[1] <<  8) & 0xff00) |
    			 (data1[2]        & 0xff) );
    
    
    SHA1Reset(&sha);
    SHA1Input(&sha, (const unsigned char *) buf, 7);

    if (!SHA1Result(&sha))
    {
        printk(KERN_ERR "%s: ERROR-- could not compute message digest\n", __func__);
    	
    	return 1;
    }
    else
    {
        data[3] = ( 0x80000000 | ( sha.Message_Digest[4] & 0x00ffffff ) );
    }
    
    printk(KERN_INFO "proc_comm_ftm_meid_write() '0x%08x, %08x'\n",
    											data[2], data[3]);
    
    if (ret != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1,
    								&smem_proc_comm_oem_data1,
    								&smem_proc_comm_oem_data2,
    								data,
    								4)) {
        printk(KERN_ERR
        		"%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    	
    	return ret;
    }
    
    printk(KERN_INFO "proc_comm_ftm_meid_write() success\n");
    
    return ret;
}
EXPORT_SYMBOL(proc_comm_ftm_meid_write);

int proc_comm_ftm_meid_read(unsigned* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_READ;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[3];
    int32_t ret = 0;
    
    char data1[14] = {0};
    

    memset((void*)data, 0x00000000, sizeof(data));
    data[0] = (uint32_t)1943;	// NV ID MEID
    
    if (ret != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    3)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    	
    	return ret;
    }
    
    printk(KERN_ERR " meid = %08x   %08x",data[1],data[2]);
    
    data1[ 6] = ( data[1] & 0xf0000000 ) >> 28;
    data1[ 7] = ( data[1] & 0xf000000 )  >> 24;
    data1[ 8] = ( data[1] & 0xf00000 )   >> 20;
    data1[ 9] = ( data[1] & 0xf0000 )    >> 16;
    data1[10] = ( data[1] & 0xf000 )     >> 12;
    data1[11] = ( data[1] & 0xf00 )      >>  8;
    data1[12] = ( data[1] & 0xf0 )       >>  4;
    data1[13] = ( data[1] & 0xf );
    data1[ 0] = ( data[2] & 0xf00000 ) >> 20;
    data1[ 1] = ( data[2] & 0xf0000 )  >> 16;
    data1[ 2] = ( data[2] & 0xf000 )   >> 12;
    data1[ 3] = ( data[2] & 0xf00 )    >>  8;
    data1[ 4] = ( data[2] & 0xf0 )     >>  4;
    data1[ 5] = ( data[2] & 0xf );
    
    memcpy(buf, data1, 14);
    
    printk(KERN_INFO "proc_comm_ftm_meid_read() MEID = '0x%x,%x,%x,%x,%x,%x,%x,%x, %x, %x, %x, %x, %x, %x'\n",
    										data1[ 0], data1[ 1], data1[ 2], data1[ 3], data1[ 4],
    										data1[ 5], data1[ 6], data1[ 7], data1[ 8], data1[ 9],
    										data1[10], data1[11], data1[12], data1[13]);
    										
    return ret;
}
EXPORT_SYMBOL(proc_comm_ftm_meid_read);
/* } FIHTDC, CHHsieh, MEID Get/Set */

/* FIHTDC, CHHsieh, FD1 NV programming in factory { */
int proc_comm_ftm_esn_read(unsigned* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_READ;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[2];
    int32_t ret = 0;
    
    char data1[8] = {0};
    

    memset((void*)data, 0x00000000, sizeof(data));
    data[0] = (uint32_t)0;	// NV ID MEID
    
    if (ret != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    3)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    	
    	return ret;
    }
    
    printk(KERN_ERR " esn = %08x",data[1]);
    
    data1[ 0] = ( data[1] & 0xf0000000 ) >> 28;
    data1[ 1] = ( data[1] & 0xf000000 )  >> 24;
    data1[ 2] = ( data[1] & 0xf00000 )   >> 20;
    data1[ 3] = ( data[1] & 0xf0000 )    >> 16;
    data1[ 4] = ( data[1] & 0xf000 )     >> 12;
    data1[ 5] = ( data[1] & 0xf00 )      >>  8;
    data1[ 6] = ( data[1] & 0xf0 )       >>  4;
    data1[ 7] = ( data[1] & 0xf );
    
    memcpy(buf, data1, 8);
    
    printk(KERN_INFO "proc_comm_ftm_esn_read() ESN = '0x%x,%x,%x,%x,%x,%x,%x,%x\n",
    										data1[0], data1[1], data1[2], data1[3],
    										data1[4], data1[5], data1[6], data1[7]);
    										
    return ret;
}
EXPORT_SYMBOL(proc_comm_ftm_esn_read);

int proc_comm_ftm_akey1_write(unsigned* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_WRITE;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[4];
    int32_t ret = 0;
    
    char data1[16] = {0};
    
    memcpy(data1, buf, 16);
    
    memset((void*)data, 0x00000000, sizeof(data));
    data[0] = (uint32_t)25;	// NV ID IMEI
    
    data[1] = 0x0;
    data[2] = ( ((data1[ 8] << 28) & 0xf0000000) | ((data1[ 9] << 24) & 0xf000000) |
    			((data1[10] << 20) & 0xf00000)	 | ((data1[11] << 16) & 0xf0000) |
    			((data1[12] << 12) & 0xf000)     | ((data1[13] <<  8) & 0xf00) |
    			((data1[14] <<  4) & 0xf0)       |  (data1[15]        & 0xf) );
    data[3] = ( ((data1[ 0] << 28) & 0xf0000000) | ((data1[ 1] << 24) & 0xf000000) |
    			((data1[ 2] << 20) & 0xf00000)	 | ((data1[ 3] << 16) & 0xf0000) |
    			((data1[ 4] << 12) & 0xf000)     | ((data1[ 5] <<  8) & 0xf00) |
    			((data1[ 6] <<  4) & 0xf0)       |  (data1[ 7]        & 0xf) );
    
    printk(KERN_INFO "proc_comm_ftm_akey1_write() '0x%08x, %08x'\n",
    											data[2], data[3]);
    
    if (ret != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1,
    								&smem_proc_comm_oem_data1,
    								&smem_proc_comm_oem_data2,
    								data,
    								4)) {
        printk(KERN_ERR
        		"%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    	
    	return ret;
    }
    
    printk(KERN_INFO "proc_comm_ftm_akey1_write() success\n");
    
    return ret;
}
EXPORT_SYMBOL(proc_comm_ftm_akey1_write);

int proc_comm_ftm_akey2_write(unsigned* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_WRITE;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[4];
    int32_t ret = 0;
    
    char data1[16] = {0};
    
    memcpy(data1, buf, 16);
    
    memset((void*)data, 0x00000000, sizeof(data));
    data[0] = (uint32_t)25;	// NV ID IMEI
    
    data[1] = 0x1;
    data[2] = ( ((data1[ 8] << 28) & 0xf0000000) | ((data1[ 9] << 24) & 0xf000000) |
    			((data1[10] << 20) & 0xf00000)	 | ((data1[11] << 16) & 0xf0000) |
    			((data1[12] << 12) & 0xf000)     | ((data1[13] <<  8) & 0xf00) |
    			((data1[14] <<  4) & 0xf0)       |  (data1[15]        & 0xf) );
    data[3] = ( ((data1[ 0] << 28) & 0xf0000000) | ((data1[ 1] << 24) & 0xf000000) |
    			((data1[ 2] << 20) & 0xf00000)	 | ((data1[ 3] << 16) & 0xf0000) |
    			((data1[ 4] << 12) & 0xf000)     | ((data1[ 5] <<  8) & 0xf00) |
    			((data1[ 6] <<  4) & 0xf0)       |  (data1[ 7]        & 0xf) );
    
    printk(KERN_INFO "proc_comm_ftm_akey2_write() '0x%08x, %08x'\n",
    											data[2], data[3]);
    
    if (ret != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1,
    								&smem_proc_comm_oem_data1,
    								&smem_proc_comm_oem_data2,
    								data,
    								4)) {
        printk(KERN_ERR
        		"%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    	
    	return ret;
    }
    
    printk(KERN_INFO "proc_comm_ftm_akey2_write() success\n");
    
    return ret;
}
EXPORT_SYMBOL(proc_comm_ftm_akey2_write);

int proc_comm_ftm_akey1_read(char* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_READ;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[4];
    int32_t ret = 0;
    
    char data1[16] = {0};
    

    memset((void*)data, 0x00000000, sizeof(data));
    data[0] = (uint32_t)25;	// NV ID IMEI
    
    data[1] = 0x0;
    
    if (ret != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    4)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    	
    	return ret;
    }
    
    printk(KERN_ERR " Akey1 = %08x, %08x, %08x, %08x", data[0], data[1], data[2], data[3]);
    
    data1[ 8] = ( data[2] & 0xf0000000 ) >> 28;
    data1[ 9] = ( data[2] & 0xf000000 )  >> 24;
    data1[10] = ( data[2] & 0xf00000 )   >> 20;
    data1[11] = ( data[2] & 0xf0000 )    >> 16;
    data1[12] = ( data[2] & 0xf000 )     >> 12;
    data1[13] = ( data[2] & 0xf00 )      >>  8;
    data1[14] = ( data[2] & 0xf0 )       >>  4;
    data1[15] = ( data[2] & 0xf );
    data1[ 0] = ( data[3] & 0xf0000000 ) >> 28;
    data1[ 1] = ( data[3] & 0xf000000 )  >> 24;
    data1[ 2] = ( data[3] & 0xf00000 )   >> 20;
    data1[ 3] = ( data[3] & 0xf0000 )    >> 16;
    data1[ 4] = ( data[3] & 0xf000 )     >> 12;
    data1[ 5] = ( data[3] & 0xf00 )      >>  8;
    data1[ 6] = ( data[3] & 0xf0 )       >>  4;
    data1[ 7] = ( data[3] & 0xf );
    
    memcpy(buf, data1, 16);
    
    printk(KERN_INFO "proc_comm_ftm_akey1_read() SPC = '0x%x,%x,%x,%x,%x,%x\n",
    										data1[0], data1[1], data1[2],
    										data1[3], data1[4], data1[5]);
    										
    return ret;
}
EXPORT_SYMBOL(proc_comm_ftm_akey1_read);

int proc_comm_ftm_spc_read(char* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_READ;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[7];
    int32_t ret = 0;
    
    char data1[6] = {0};
    

    memset((void*)data, 0x00000000, sizeof(data));
    data[0] = (uint32_t)85;	// NV ID MEID
    
    if (ret != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    7)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    	
    	return ret;
    }
    
    printk(KERN_ERR " SPC = %08x, %08x",data[1],data[2]);
    
    data1[ 0] = data[1] & 0xff;
    data1[ 1] = data[2] & 0xff;
    data1[ 2] = data[3] & 0xff;
    data1[ 3] = data[4] & 0xff;
    data1[ 4] = data[5] & 0xff;
    data1[ 5] = data[6] & 0xff;
    
    memcpy(buf, data1, 8);
    
    printk(KERN_INFO "proc_comm_ftm_spc_read() SPC = '0x%x,%x,%x,%x,%x,%x\n",
    										data1[0], data1[1], data1[2],
    										data1[3], data1[4], data1[5]);
    										
    return ret;
}
EXPORT_SYMBOL(proc_comm_ftm_spc_read);

int proc_comm_ftm_spc_write(unsigned* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_WRITE;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[7];
    int32_t ret = 0;
    
    char data1[6] = {0};
    
    memcpy(data1, buf, 6);
    
    memset((void*)data, 0x00000000, sizeof(data));
    data[0] = (uint32_t)85;	// NV ID IMEI
    
    
    data[1] = ( ( ( 0x3  <<  4) & 0xf0 ) | ( data1[0] & 0xf ) );
    data[2] = ( ( ( 0x3  <<  4) & 0xf0 ) | ( data1[1] & 0xf ) );
    data[3] = ( ( ( 0x3  <<  4) & 0xf0 ) | ( data1[2] & 0xf ) );
    data[4] = ( ( ( 0x3  <<  4) & 0xf0 ) | ( data1[3] & 0xf ) );
    data[5] = ( ( ( 0x3  <<  4) & 0xf0 ) | ( data1[4] & 0xf ) );
    data[6] = ( ( ( 0x3  <<  4) & 0xf0 ) | ( data1[5] & 0xf ) );
    
    printk(KERN_INFO "proc_comm_ftm_spc_write(): %02x, %02x, %02x, %02x, %02x, %02x",
    								data[1],data[2],data[3],data[4],data[5],data[6]);
    
    if (ret != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1,
    								&smem_proc_comm_oem_data1,
    								&smem_proc_comm_oem_data2,
    								data,
    								7)) {
        printk(KERN_ERR
        		"%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    	
    	return ret;
    }
    
    printk(KERN_INFO "proc_comm_ftm_spc_write() success\n");
    
    return ret;
}
EXPORT_SYMBOL(proc_comm_ftm_spc_write);

void proc_comm_ftm_customer_pid_write(unsigned* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_WRITE;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[10];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)52001;
    data[1] = 0x0;
    
    memcpy(&data[2], buf, 32);
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    10)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
    
    printk(KERN_INFO "proc_comm_ftm_customer_pid_write() '%x %x %x'\n", data[1], data[2], data[3]);
}
EXPORT_SYMBOL(proc_comm_ftm_customer_pid_write);

void proc_comm_ftm_customer_pid_read(unsigned* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_READ;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[9];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)52001;
    data[1] = 0x0;
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    9)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
    
    memcpy(buf, &data[1], 32);
    
    printk(KERN_INFO "proc_comm_ftm_customer_pid_read()\n");
}
EXPORT_SYMBOL(proc_comm_ftm_customer_pid_read);

void proc_comm_ftm_customer_pid2_write(unsigned* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_WRITE;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[10];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)52001;
    data[1] = 0x1;
    
    memcpy(&data[2], buf, 32);
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    10)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
    
    printk(KERN_INFO "proc_comm_ftm_customer_pid2_write() '%x %x %x'\n", data[1], data[2], data[3]);
}
EXPORT_SYMBOL(proc_comm_ftm_customer_pid2_write);

void proc_comm_ftm_customer_pid2_read(unsigned* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_READ;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[9];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)52001;
    data[1] = 0x1;
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    9)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
    
    memcpy(buf, &data[1], 32);
    
    printk(KERN_INFO "proc_comm_ftm_customer_pid2_read()\n");
}
EXPORT_SYMBOL(proc_comm_ftm_customer_pid2_read);

void proc_comm_ftm_customer_swid_write(unsigned* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_WRITE;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[9];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)52001;
    data[1] = 0x2;
    
    memcpy(&data[2], buf, 32);
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    10)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
    
    printk(KERN_INFO "proc_comm_ftm_customer_swid_write() '%x %x %x'\n", data[1], data[2], data[3]);
}
EXPORT_SYMBOL(proc_comm_ftm_customer_swid_write);

void proc_comm_ftm_customer_swid_read(unsigned* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_READ;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[9];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)52001;
    data[1] = 0x2;
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    9)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
    
    memcpy(buf, &data[1], 32);
    
    printk(KERN_INFO "proc_comm_ftm_customer_swid_read()\n");
}
EXPORT_SYMBOL(proc_comm_ftm_customer_swid_read);

void proc_comm_ftm_dom_write(unsigned* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_WRITE;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[6];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)52001;
    data[1] = 0x3;
    
    memcpy(&data[2], buf, 14);
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    6)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
    
    printk(KERN_INFO "proc_comm_ftm_dom_write() '%x %x %x'\n", data[1], data[2], data[3]);
}
EXPORT_SYMBOL(proc_comm_ftm_dom_write);

void proc_comm_ftm_dom_read(unsigned* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_READ;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[5];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)52001;
    data[1] = 0x3;
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    5)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
    
    memcpy(buf, &data[1], 14);
    
    printk(KERN_INFO "proc_comm_ftm_dom_read()\n");
}
EXPORT_SYMBOL(proc_comm_ftm_dom_read);
/* } FIHTDC, CHHsieh, FD1 NV programming in factory */

/* FIHTDC, CHHsieh, FB3 CA/WCA Time Set/Get { */
void proc_comm_ftm_ca_time_write(char* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_WRITE;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[11];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)2499;
    
    memcpy(&data[1], buf, 14);
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    11)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
    
    printk(KERN_INFO "proc_comm_ftm_ca_time_write() '%x %x %x %x %x'\n", data[1], data[2], data[3], data[4], data[5]);
}
EXPORT_SYMBOL(proc_comm_ftm_ca_time_write);

void proc_comm_ftm_ca_time_read(char* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_READ;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[3];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)2499;
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    3)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
    
    printk(KERN_ERR " ca time = %08x, %08x",data[1],data[2]);
    
    memcpy(buf, &data[1], sizeof(char)*7);
    
    printk(KERN_INFO "proc_comm_ftm_ca_time_read() '0x%x:%x:%x:%x:%x:%x'\n", buf[5], buf[4],buf[3], buf[2],buf[1], buf[0]);
}
EXPORT_SYMBOL(proc_comm_ftm_ca_time_read);

void proc_comm_ftm_wca_time_write(char* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_WRITE;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[3];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)2500;
    
    memcpy(&data[1], buf, 7);
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    3)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
    
    printk(KERN_INFO "proc_comm_ftm_wca_time_write() '%x, %x'\n", data[1], data[2]);
}
EXPORT_SYMBOL(proc_comm_ftm_wca_time_write);

void proc_comm_ftm_wca_time_read(char* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_READ;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[3];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)2500;
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    3)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
    
    printk(KERN_ERR " wca time = %08x, %08x",data[1],data[2]);
    
    memcpy(buf, &data[1], sizeof(char)*7);
    
    printk(KERN_INFO "proc_comm_ftm_wca_time_read() '0x%x:%x:%x:%x:%x:%x'\n", buf[5], buf[4],buf[3], buf[2],buf[1], buf[0]);
}
EXPORT_SYMBOL(proc_comm_ftm_wca_time_read);
/* } FIHTDC, CHHsieh, FB3 CA/WCA Time Set/Get */

void proc_comm_hw_reset_to_ftm(void)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_HW_RESET_TO_FTM;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[2];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = 1;
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    0)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_HW_RESET FAILED!!\n",
                __func__);
    }
    
    printk(KERN_INFO "proc_comm_hw_reset_to_ftm()\n");
}
EXPORT_SYMBOL(proc_comm_hw_reset_to_ftm);

/* FIHTDC, CHHsieh, FB0 FactoryInfo Set/Get { */
int proc_comm_ftm_factory_info_write(char* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_WRITE;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[32];
    int32_t ret = 0;
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)50033;
    
    memcpy(&data[1], buf, 124);
    
    printk(KERN_INFO "proc_comm_ftm_factory_info_write() '0x%08x, %08x, %08x'\n",
    											data[1], data[2], data[3]);
    printk(KERN_ERR "NV ID = %d", data[0]);
    
    ret = msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    32);
                                    
	if (ret != 0) {
		printk(KERN_ERR
				"%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
		return ret;
    }
    
    printk(KERN_INFO "proc_comm_ftm_factory_info_write() success\n");
    
    return ret;
}
EXPORT_SYMBOL(proc_comm_ftm_factory_info_write);

int proc_comm_ftm_factory_info_read(char* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_READ;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[32];
    int32_t ret = 0;
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)50033;
    
    printk(KERN_ERR "NV ID = %d", data[0]);
    
	ret = msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1,
									&smem_proc_comm_oem_data1,
									&smem_proc_comm_oem_data2,
									data,
									32);
									
	if (ret != 0) {
		printk(KERN_ERR
				"%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
				__func__);
		return ret;
    }
    
    printk(KERN_INFO "proc_comm_ftm_factory_info_read() success\n");
    //printk(KERN_ERR "factory info = %08x, %08x, %08x", data[1], data[2], data[3]);
    
    memcpy(buf, &data[1], sizeof(char)*124);
    
    printk(KERN_INFO "proc_comm_ftm_factory_info_read() '0x%x:%x:%x:%x:%x:%x'\n",
    										buf[0], buf[1],buf[2], buf[3],buf[4], buf[5]);
    
    return ret;
}
EXPORT_SYMBOL(proc_comm_ftm_factory_info_read);	// NV IDfactory info
/* } FIHTDC, CHHsieh, FB0 FactoryInfo Set/Get */

/* FIHTDC, Peter, 2011/3/18 { */
void proc_comm_version_info_read(char* buf)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_NV_READ;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[16];
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = (uint32_t)50030;
    
    if (0 != msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    16)) {
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_FIH FAILED!!\n",
                __func__);
    }
    
    memcpy(buf, &data[1], 64);    
}
EXPORT_SYMBOL(proc_comm_version_info_read);
/* } FIHTDC, Peter, 2011/3/18 */

/* FIH, Tiger, 2009/12/10 { */
#ifdef CONFIG_FIH_FXX
#define MSM_PM_DEBUG_FIH_MODULE (1U << 8)
extern int msm_pm_debug_mask;

int msm_proc_comm_oem_tcp_filter(void *cmd_data, unsigned cmd_size)
{
	unsigned cmd = PCOM_CUSTOMER_CMD1;
	unsigned oem_cmd = SMEM_PROC_COMM_OEM_UPDATE_TCP_FILTER;
	//unsigned oem_resp;
	unsigned *data1 = &oem_cmd;
	unsigned *data2 = NULL;
	unsigned base = (unsigned)MSM_SHARED_RAM_BASE;
	unsigned long flags;
	int ret;
	smem_oem_cmd_data *cmd_buf;

	void* ptr;
	int size;

#if 1
	if (msm_pm_debug_mask & MSM_PM_DEBUG_FIH_MODULE) {
		unsigned short *content = (unsigned short *)cmd_data;
		for(ret=0; ret<cmd_size/2; ret++) {
			printk(KERN_INFO "tcp filter [%d, %4x]\n", ret, *(content+ret));
		}
	}
#endif

	cmd_buf = smem_get_entry(SMEM_OEM_CMD_DATA, &size);

	ptr = (unsigned*)&cmd_buf->cmd_data.cmd_parameter[0];

	spin_lock_irqsave(&proc_comm_lock, flags);

again:
	if (proc_comm_wait_for(base + MDM_STATUS, PCOM_READY))
		goto again;

	writel(cmd, base + APP_COMMAND);
	writel(data1 ? *data1 : 0, base + APP_DATA1);
	writel(data2 ? *data2 : 0, base + APP_DATA2);
	cmd_buf->cmd_data.check_flag = smem_oem_locked_flag;
	{
		memcpy(ptr,(const void *)cmd_data,cmd_size);
	}
	notify_other_proc_comm();

	if (proc_comm_wait_for(base + APP_COMMAND, PCOM_CMD_DONE))
		goto again;

	writel(PCOM_CMD_IDLE, base + APP_COMMAND);

	/* read response value, Hanson Lin */
	ret = 50;
	while((!(cmd_buf->return_data.check_flag & smem_oem_unlocked_flag)) && (ret--))
	{
		//waiting
		mdelay(1);
	}
	ret = (ret) ? (cmd_buf->return_data.check_flag & 0x1111) : -1;

	spin_unlock_irqrestore(&proc_comm_lock, flags);

	if(ret) {
		printk(KERN_ERR "msm_proc_comm_oem_tcp_filter() returns %d\n", ret);
	}
	
	return ret;
}

EXPORT_SYMBOL(msm_proc_comm_oem_tcp_filter);
#endif
/* } FIH, Tiger, 2009/12/10 */

int proc_comm_set_chg_voltage(u32 vmaxsel, u32 vbatdet, u32 vrechg)
{
    uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
    uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_SET_CHG_VAL;
    uint32_t smem_proc_comm_oem_data2   = 0; //useless
    
    int32_t data[3];
    int32_t ret = 0;
    
    memset((void*)data, 0x00, sizeof(data));
    data[0] = vmaxsel;
    data[1] = vbatdet;
    data[2] = vrechg;
    
    ret = msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1, 
                                    &smem_proc_comm_oem_data1,
                                    &smem_proc_comm_oem_data2,
                                    data,
                                    3);
    if (ret != 0){
        printk(KERN_ERR
                "%s: SMEM_PROC_COMM_OEM_SET_CHG_VOL FAILED!!\n",
                __func__);
    }
    
    return ret;
}
EXPORT_SYMBOL(proc_comm_set_chg_voltage);

int proc_comm_ftm_change_modem_lpm(void)
{
	uint32_t smem_proc_comm_oem_cmd1    = PCOM_CUSTOMER_CMD1;
	uint32_t smem_proc_comm_oem_data1   = SMEM_PROC_COMM_OEM_CHG_MODE;
	uint32_t smem_proc_comm_oem_data2   = 0; //useless
	
	int32_t data[2];
	int		ret = 0;
	
	memset((void*)data, 0x00, sizeof(data));
	data[0] = 6;
	
	printk(KERN_INFO "%s before call msm_proc_comm_oem_multi\n", __func__);
	
    ret = msm_proc_comm_oem_multi(smem_proc_comm_oem_cmd1,
    								&smem_proc_comm_oem_data1,
    								&smem_proc_comm_oem_data2,
									data,
									1);
	if (ret != 1){
		printk(KERN_ERR
				"%s : %d : SMEM_PROC_COMM_OEM_CHG_MODE FAILED!!\n",
				__func__, ret);
		
		return ret;
	}
    
    printk(KERN_INFO "proc_comm_ftm_change_modem_lpm()\n");
    
    return ret;
}
EXPORT_SYMBOL(proc_comm_ftm_change_modem_lpm);

