/* Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef DIAGCHAR_SHARED
#define DIAGCHAR_SHARED

#define MSG_MASKS_TYPE			1
#define LOG_MASKS_TYPE			2
#define EVENT_MASKS_TYPE		4
#define PKT_TYPE			8
#define DEINIT_TYPE			16
#define MEMORY_DEVICE_LOG_TYPE		32
#define USB_MODE			1
#define MEMORY_DEVICE_MODE		2
#define NO_LOGGING_MODE			3
#define MAX_SYNC_OBJ_NAME_SIZE		32

/* different values that go in for diag_data_type */
#define DATA_TYPE_EVENT         	0
#define DATA_TYPE_F3            	1
#define DATA_TYPE_LOG           	2
#define DATA_TYPE_RESPONSE      	3

/* Different IOCTL values */
#define DIAG_IOCTL_COMMAND_REG  	0
#define DIAG_IOCTL_SWITCH_LOGGING	7
#define DIAG_IOCTL_GET_DELAYED_RSP_ID 	8
#define DIAG_IOCTL_LSM_DEINIT		9
//Div2D5-LC-BSP-Porting_OTA_SDDownload-00 +[
#define DIAG_IOCTL_WRITE_BUFFER 10
#define DIAG_IOCTL_READ_BUFFER  11
#define DIAG_IOCTL_PASS_FIRMWARE_LIST  12
#define DIAG_IOCTL_GET_PART_TABLE_FROM_SMEM  13
//Div2D5-LC-BSP-Porting_OTA_SDDownload-00 +]

//Mig-Added for Slate Application support
#define DIAG_IOCTL_GET_RSSI                 14
#define DIAG_IOCTL_GET_STATE_AND_CONN_INFO  15
//Shashi added for Slate command support FXPCAYM81
#define DIAG_IOCTL_GET_KEY_EVENT_MASK  16
#define DIAG_IOCTL_GET_PEN_EVENT_MASK  17
//Shashi added for Slate command support FXPCAYM81 ends here

// Ketan-Added for FTM Application Support - FXPCAYM-87
#define DIAG_IOCTL_GET_SEARCHER_DUMP        18
#define DIAG_IOCTL_READ_DMSS_STATUS         19
#define DIAG_IOCTL_SEND_DMSS_STATUS         20
#define DIAG_IOCTL_RESTORE_LOGGING_MASKS    21

struct bindpkt_params {
	uint16_t cmd_code;
	uint16_t subsys_id;
	uint16_t cmd_code_lo;
	uint16_t cmd_code_hi;
	uint16_t proc_id;
	uint32_t event_id;
	uint32_t log_code;
	uint32_t client_id;
};

struct bindpkt_params_per_process {
	/* Name of the synchronization object associated with this process */
	char sync_obj_name[MAX_SYNC_OBJ_NAME_SIZE];
	uint32_t count;	/* Number of entries in this bind */
	struct bindpkt_params *params; /* first bind params */
};

struct diagpkt_delay_params{
	void *rsp_ptr;
	int size;
	int *num_bytes_ptr;
};

//Div2D5-LC-BSP-Porting_OTA_SDDownload-00 +[
struct diagpkt_ioctl_param
{
		uint8_t * pPacket;
		uint16_t Len;
};

#define DL_FILENAME_LEN 32

#pragma pack(push)
#pragma pack(8)
typedef struct file_dest_info
{
   char filename[26];
   int  in_use;
   int  offset;
   uint position;
} CMM_info;

typedef struct dl_list
{
uint32_t magic_num;               
uint32_t iFLAG;// update flag
uint32_t dl_flag;                /*bit 0  (1)be set-> dont' switch to backup even EFS parti diff*/
                             /*bit 1  (2)be set-> combined image mode*/
                             /*bit 2  (4)be set-> need backup and restore NV*/
                             /*bit 3  (8)be set-> multi port download*/                             
                             /*bit 4 (16)be set-> eMMC download*/
                             /*bit 5 (32)be set-> Emgerency download mode*/
                             /*bit 6 (64)be set-> erase user data partition*/
char pCOMBINED_IMAGE[32];
int aAMSS[3];
int aAPPSBOOT[3];
int aANDROID_BOOT[3];
int aANDROID_SYSTEM[3];
int aANDROID_SPLASH[3];
int aANDROID_RECOVERY[3];
int aANDROID_HIDDEN[3];
int aANDROID_USR_DATA[3];
int aANDROID_FTM[3];
int aOSBL[3];
int aDBL[3];
int aDSP1[3];
int aCUSTOMER_NV[3];
int aPARTITION_BIN[3];
int aLOADPT[3];
int aANDROID_CDA[3];
int  aRESERVED1[3];
int  aRESERVED2[3];
int  aRESERVED3[3];
int  aRESERVED4[3];
int  aRESERVED5[3];
CMM_info fdinfo[17];
uint32_t checksum;  
}FirmwareList;
#pragma pack(pop)
//Div2D5-LC-BSP-Porting_OTA_SDDownload-00 +]

#endif
