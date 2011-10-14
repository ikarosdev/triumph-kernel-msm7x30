/* arch/arm/mach-msm/proc_comm.h
 *
 * Copyright (c) 2007-2009, Code Aurora Forum. All rights reserved.
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

#ifndef _ARCH_ARM_MACH_MSM_MSM_PROC_COMM_H_
#define _ARCH_ARM_MACH_MSM_MSM_PROC_COMM_H_

enum {
	PCOM_CMD_IDLE = 0x0,
	PCOM_CMD_DONE,
	PCOM_RESET_APPS,
	PCOM_RESET_CHIP,
	PCOM_CONFIG_NAND_MPU,
	PCOM_CONFIG_USB_CLKS,
	PCOM_GET_POWER_ON_STATUS,
	PCOM_GET_WAKE_UP_STATUS,
	PCOM_GET_BATT_LEVEL,
	PCOM_CHG_IS_CHARGING,
	PCOM_POWER_DOWN,
	PCOM_USB_PIN_CONFIG,
	PCOM_USB_PIN_SEL,
	PCOM_SET_RTC_ALARM,
	PCOM_NV_READ,
	PCOM_NV_WRITE,
	PCOM_GET_UUID_HIGH,
	PCOM_GET_UUID_LOW,
	PCOM_GET_HW_ENTROPY,
	PCOM_RPC_GPIO_TLMM_CONFIG_REMOTE,
	PCOM_CLKCTL_RPC_ENABLE,
	PCOM_CLKCTL_RPC_DISABLE,
	PCOM_CLKCTL_RPC_RESET,
	PCOM_CLKCTL_RPC_SET_FLAGS,
	PCOM_CLKCTL_RPC_SET_RATE,
	PCOM_CLKCTL_RPC_MIN_RATE,
	PCOM_CLKCTL_RPC_MAX_RATE,
	PCOM_CLKCTL_RPC_RATE,
	PCOM_CLKCTL_RPC_PLL_REQUEST,
	PCOM_CLKCTL_RPC_ENABLED,
	PCOM_VREG_SWITCH,
	PCOM_VREG_SET_LEVEL,
	PCOM_GPIO_TLMM_CONFIG_GROUP,
	PCOM_GPIO_TLMM_UNCONFIG_GROUP,
	PCOM_NV_WRITE_BYTES_4_7,
	PCOM_CONFIG_DISP,
	PCOM_GET_FTM_BOOT_COUNT,
	PCOM_RPC_GPIO_TLMM_CONFIG_EX,
	PCOM_PM_MPP_CONFIG,
	PCOM_GPIO_IN,
	PCOM_GPIO_OUT,
	PCOM_RESET_MODEM,
	PCOM_RESET_CHIP_IMM,
	PCOM_PM_VID_EN,
	PCOM_VREG_PULLDOWN,
	PCOM_GET_MODEM_VERSION,
	PCOM_CLK_REGIME_SEC_RESET,
	PCOM_CLK_REGIME_SEC_RESET_ASSERT,
	PCOM_CLK_REGIME_SEC_RESET_DEASSERT,
	PCOM_CLK_REGIME_SEC_PLL_REQUEST_WRP,
	PCOM_CLK_REGIME_SEC_ENABLE,
	PCOM_CLK_REGIME_SEC_DISABLE,
	PCOM_CLK_REGIME_SEC_IS_ON,
	PCOM_CLK_REGIME_SEC_SEL_CLK_INV,
	PCOM_CLK_REGIME_SEC_SEL_CLK_SRC,
	PCOM_CLK_REGIME_SEC_SEL_CLK_DIV,
	PCOM_CLK_REGIME_SEC_ICODEC_CLK_ENABLE,
	PCOM_CLK_REGIME_SEC_ICODEC_CLK_DISABLE,
	PCOM_CLK_REGIME_SEC_SEL_SPEED,
	PCOM_CLK_REGIME_SEC_CONFIG_GP_CLK_WRP,
	PCOM_CLK_REGIME_SEC_CONFIG_MDH_CLK_WRP,
	PCOM_CLK_REGIME_SEC_USB_XTAL_ON,
	PCOM_CLK_REGIME_SEC_USB_XTAL_OFF,
	PCOM_CLK_REGIME_SEC_SET_QDSP_DME_MODE,
	PCOM_CLK_REGIME_SEC_SWITCH_ADSP_CLK,
	PCOM_CLK_REGIME_SEC_GET_MAX_ADSP_CLK_KHZ,
	PCOM_CLK_REGIME_SEC_GET_I2C_CLK_KHZ,
	PCOM_CLK_REGIME_SEC_MSM_GET_CLK_FREQ_KHZ,
	PCOM_CLK_REGIME_SEC_SEL_VFE_SRC,
	PCOM_CLK_REGIME_SEC_MSM_SEL_CAMCLK,
	PCOM_CLK_REGIME_SEC_MSM_SEL_LCDCLK,
	PCOM_CLK_REGIME_SEC_VFE_RAIL_OFF,
	PCOM_CLK_REGIME_SEC_VFE_RAIL_ON,
	PCOM_CLK_REGIME_SEC_GRP_RAIL_OFF,
	PCOM_CLK_REGIME_SEC_GRP_RAIL_ON,
	PCOM_CLK_REGIME_SEC_VDC_RAIL_OFF,
	PCOM_CLK_REGIME_SEC_VDC_RAIL_ON,
	PCOM_CLK_REGIME_SEC_LCD_CTRL,
	PCOM_CLK_REGIME_SEC_REGISTER_FOR_CPU_RESOURCE,
	PCOM_CLK_REGIME_SEC_DEREGISTER_FOR_CPU_RESOURCE,
	PCOM_CLK_REGIME_SEC_RESOURCE_REQUEST_WRP,
	PCOM_CLK_REGIME_MSM_SEC_SEL_CLK_OWNER,
	PCOM_CLK_REGIME_SEC_DEVMAN_REQUEST_WRP,
	PCOM_GPIO_CONFIG,
	PCOM_GPIO_CONFIGURE_GROUP,
	PCOM_GPIO_TLMM_SET_PORT,
	PCOM_GPIO_TLMM_CONFIG_EX,
	PCOM_SET_FTM_BOOT_COUNT,
	PCOM_RESERVED0,
	PCOM_RESERVED1,
	PCOM_CUSTOMER_CMD1,
	PCOM_CUSTOMER_CMD2,
	PCOM_CUSTOMER_CMD3,
	PCOM_CLK_REGIME_ENTER_APPSBL_CHG_MODE,
	PCOM_CLK_REGIME_EXIT_APPSBL_CHG_MODE,
	PCOM_CLK_REGIME_SEC_RAIL_DISABLE,
	PCOM_CLK_REGIME_SEC_RAIL_ENABLE,
	PCOM_CLK_REGIME_SEC_RAIL_CONTROL,
	PCOM_SET_SW_WATCHDOG_STATE,
	PCOM_PM_MPP_CONFIG_DIGITAL_INPUT,
	PCOM_PM_MPP_CONFIG_I_SINK,
	PCOM_RESERVED_101,
	PCOM_MSM_HSUSB_PHY_RESET,
	PCOM_GET_BATT_MV_LEVEL,
	PCOM_CHG_USB_IS_PC_CONNECTED,
	PCOM_CHG_USB_IS_CHARGER_CONNECTED,
	PCOM_CHG_USB_IS_DISCONNECTED,
	PCOM_CHG_USB_IS_AVAILABLE,
	PCOM_CLK_REGIME_SEC_MSM_SEL_FREQ,
	PCOM_CLK_REGIME_SEC_SET_PCLK_AXI_POLICY,
	PCOM_CLKCTL_RPC_RESET_ASSERT,
	PCOM_CLKCTL_RPC_RESET_DEASSERT,
	PCOM_CLKCTL_RPC_RAIL_ON,
	PCOM_CLKCTL_RPC_RAIL_OFF,
	PCOM_CLKCTL_RPC_RAIL_ENABLE,
	PCOM_CLKCTL_RPC_RAIL_DISABLE,
	PCOM_CLKCTL_RPC_RAIL_CONTROL,
	PCOM_CLKCTL_RPC_MIN_MSMC1,
	PCOM_CLKCTL_RPC_SRC_REQUEST,
	PCOM_NPA_INIT,
	PCOM_NPA_ISSUE_REQUIRED_REQUEST,
};

enum {
	PCOM_OEM_FIRST_CMD = 0x10000000,
	PCOM_OEM_TEST_CMD = PCOM_OEM_FIRST_CMD,

	/* add OEM PROC COMM commands here */

	PCOM_OEM_LAST = PCOM_OEM_TEST_CMD,
};

enum {
	PCOM_INVALID_STATUS = 0x0,
	PCOM_READY,
	PCOM_CMD_RUNNING,
	PCOM_CMD_SUCCESS,
	PCOM_CMD_FAIL,
	PCOM_CMD_FAIL_FALSE_RETURNED,
	PCOM_CMD_FAIL_CMD_OUT_OF_BOUNDS_SERVER,
	PCOM_CMD_FAIL_CMD_OUT_OF_BOUNDS_CLIENT,
	PCOM_CMD_FAIL_CMD_UNREGISTERED,
	PCOM_CMD_FAIL_CMD_LOCKED,
	PCOM_CMD_FAIL_SERVER_NOT_YET_READY,
	PCOM_CMD_FAIL_BAD_DESTINATION,
	PCOM_CMD_FAIL_SERVER_RESET,
	PCOM_CMD_FAIL_SMSM_NOT_INIT,
	PCOM_CMD_FAIL_PROC_COMM_BUSY,
	PCOM_CMD_FAIL_PROC_COMM_NOT_INIT,
};

void msm_proc_comm_reset_modem_now(void);
int msm_proc_comm(unsigned cmd, unsigned *data1, unsigned *data2);
//Div2-SW2-BSP,JOE HSU,+++
int msm_proc_comm_oem(unsigned cmd, unsigned *data1, unsigned *data2, unsigned *cmd_parameter);
#define SMEM_OEM_CMD_BUF_SIZE  32

//SW2-5-1-MP-DbgCfgTool-00+[
int msm_proc_comm_oem_n(unsigned cmd, unsigned *data1, unsigned *data2, unsigned *cmd_parameter, int para_size);
#define NV_FIHDBG_I    51001
/*--------------------------------------------------------------------------* 
 * Size of smem_oem_cmd_data to carry both return value and FIH debug 
 * configurations.
 *
 * FIHDBG:(FIH_DEBUG_CMD_DATA_SIZE:5 int)
 *
 *   |-- 1 unsigned int return value  --|-- 4 unsigned int(128 bit) configurations --|
 *
 *   a) SMEM command's return value:
 *       true  : 1
 *       false : 0
 *   b) configurations:
 *       Magic number : 0xFFFF FFFF FFFF FFFF FFFF FFFF FFFF FFFF is default value.
 *       enable       : bit(n) = 1
 *       disable      : bit(n) = 0
 *--------------------------------------------------------------------------*/
#define FIH_DEBUG_CMD_DATA_SIZE  5
#define FIH_DEBUG_CFG_LEN  4 * sizeof(unsigned int)

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
int fih_read_fihdbg_config_nv( unsigned char* );

int fih_read_fihversion_nv( unsigned char* );     //Div2D5-LC-BSP-Porting_OTA_SDDownload-00 +

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
int fih_write_fihdbg_config_nv( unsigned char* );
//SW2-5-1-MP-DbgCfgTool-00+]

typedef union
{
	struct t_cmd_data
	{
		unsigned check_flag;
		unsigned cmd_parameter[SMEM_OEM_CMD_BUF_SIZE];
	}cmd_data;
	
	struct t_return_data
	{
	unsigned check_flag;
	unsigned return_value[SMEM_OEM_CMD_BUF_SIZE];
	}return_data;
  
} smem_oem_cmd_data;

// used for checking the cmd_buff
#define smem_oem_locked_flag		0x10000000
#define smem_oem_unlocked_flag	0x20000000

typedef enum
{
  SMEM_PROC_COMM_OEM_CHG_MODE = 0,
  SMEM_PROC_COMM_OEM_SET_PREF,
  SMEM_PROC_COMM_OEM_DIAL_EMERGENCY,
  SMEM_PROC_COMM_OEM_GET_CARD_MODE,
  SMEM_PROC_COMM_OEM_NV_READ,
  SMEM_PROC_COMM_OEM_NV_WRITE = 5,
  SMEM_PROC_COMM_OEM_CONFIG_CHG_CURRENT,
  SMEM_PROC_COMM_OEM_CONFIG_COIN_CELL,
  SMEM_PROC_COMM_OEM_RESET_CHIP_EBOOT,
  SMEM_PROC_COMM_OEM_POWER_OFF,
  SMEM_PROC_COMM_OEM_PWRKEY_DETECT = 10,
  SMEM_PROC_COMM_OEM_PM_LOW_CURRENT_LED_SET_EXT_SIGNAL,
  SMEM_PROC_COMM_OEM_PM_LOW_CURRENT_SET_CURRENT,
  SMEM_PROC_COMM_OEM_BACKUP_UNIQUE_NV,
  SMEM_PROC_COMM_OEM_HW_RESET,
  SMEM_PROC_COMM_OEM_BACKUP_NV_FLAG = 15,
  SMEM_PROC_COMM_OEM_HOST_CHECK_TO_UPDATE_DL_FLAG,
  SMEM_PROC_COMM_OEM_FUSE_BOOT,
  SMEM_PROC_COMM_OEM_ALLOC_SD_DL_INFO,
  SMEM_PROC_COMM_OEM_PMIC_UNLOCK,
  SMEM_PROC_COMM_OEM_HW_RESET_TO_FTM,
/* FIH; Tiger; 2009/12/10 { */
/* add TCP filter command */
  SMEM_PROC_COMM_OEM_UPDATE_TCP_FILTER,
/* } FIH; Tiger; 2009/12/10 */
  SMEM_PROC_COMM_OEM_SET_CHG_VAL,
  SMEM_PROC_COMM_OEM_NUM_CMDS
} smem_proc_comm_oem_cmd_type;
//Div2-SW2-BSP,JOE HSU,---

/* FIHTDC, Div2-SW2-BSP CHHsieh { */
int proc_comm_phone_online(void);
int proc_comm_phone_setpref(void);
int proc_comm_phone_dialemergency(void);
int proc_comm_phone_getsimstatus(void);
void proc_comm_ftm_product_id_write(unsigned* pid);
void proc_comm_ftm_product_id_read(unsigned* pid);
void proc_comm_ftm_imei_write(unsigned* pid);
void proc_comm_ftm_imei_read(unsigned* pid);
void proc_comm_ftm_bdaddr_write(char* buf);
void proc_comm_ftm_bdaddr_read(char* buf);
void proc_comm_ftm_wlanaddr_write(char* buf);
int proc_comm_ftm_wlanaddr_read(char * buf);
int proc_comm_ftm_backup_unique_nv(void);
void proc_comm_ftm_hw_reset(void);
int proc_comm_ftm_nv_flag(void);
/* } FIHTDC, Div2-SW2-BSP CHHsieh */

int proc_comm_touch_id_read(void);
//+++ FIH; Louis; 2010/11/9
void proc_comm_compass_param_read(int* buf);
void proc_comm_compass_param_write(int* buf);
int proc_comm_fuse_boot_set(void);
int proc_comm_fuse_boot_get(void);

//Div2D5-LC-BSP-Porting_OTA_SDDownload-00 +[
int proc_comm_read_nv(unsigned *cmd_parameter);
int proc_comm_write_nv(unsigned *cmd_parameter);
int fih_write_nv4719( unsigned int* fih_debug );
//Div2D5-LC-BSP-Porting_OTA_SDDownload-00 +]
/* FIHTDC, CHHsieh, PMIC Unlock { */
int proc_comm_ftm_pmic_unlock(void);
/* } FIHTDC, CHHsieh, PMIC Unlock */
/* FIHTDC, CHHsieh, MEID Get/Set { */
int proc_comm_ftm_meid_write(char* buf);
int proc_comm_ftm_meid_read(unsigned* buf);
/* } FIHTDC, CHHsieh, MEID Get/Set */
/* FIHTDC, CHHsieh, FD1 NV programming in factory { */
int proc_comm_ftm_esn_read(unsigned* buf);
int proc_comm_ftm_akey1_write(unsigned* buf);
int proc_comm_ftm_akey2_write(unsigned* buf);
int proc_comm_ftm_akey1_read(char* buf);
int proc_comm_ftm_spc_read(char* buf);
int proc_comm_ftm_spc_write(unsigned* buf);
void proc_comm_ftm_customer_pid_write(unsigned* buf);
void proc_comm_ftm_customer_pid_read(unsigned* buf);
void proc_comm_ftm_customer_pid2_write(unsigned* buf);
void proc_comm_ftm_customer_pid2_read(unsigned* buf);
void proc_comm_ftm_customer_swid_write(unsigned* buf);
void proc_comm_ftm_customer_swid_read(unsigned* buf);
void proc_comm_ftm_dom_write(unsigned* buf);
void proc_comm_ftm_dom_read(unsigned* buf);
/* } FIHTDC, CHHsieh, FD1 NV programming in factory */
/* FIHTDC, CHHsieh, FB3 CA/WCA Time Set/Get { */
void proc_comm_ftm_ca_time_write(char* buf);
void proc_comm_ftm_ca_time_read(char* buf);
void proc_comm_ftm_wca_time_write(char* buf);
void proc_comm_ftm_wca_time_read(char* buf);
/* } FIHTDC, CHHsieh, FB3 CA/WCA Time Set/Get */
void proc_comm_hw_reset_to_ftm(void);
/* FIHTDC, CHHsieh, FB0 FactoryInfo Set/Get { */
int proc_comm_ftm_factory_info_write(char* buf);
int proc_comm_ftm_factory_info_read(char* buf);
/* } FIHTDC, CHHsieh, FB0 FactoryInfo Set/Get */
/* FIHTDC, Peter, 2011/3/18 { */
void proc_comm_version_info_read(char* buf);
/* } FIHTDC, Peter, 2011/3/18 */
int proc_comm_ftm_change_modem_lpm(void);

/* FIH, Tiger, 2009/12/10 { */
#ifdef CONFIG_FIH_FXX
#define CLEAR_TABLE			0
#define ADD_DEST_PORT		1
#define DELETE_DEST_PORT	2
#define UPDATE_COMPLETE		3

extern int msm_proc_comm_oem_tcp_filter(void *cmd_data, unsigned cmd_size);
#endif 
/* } FIH; Tiger; 2009/12/10 */

#endif
