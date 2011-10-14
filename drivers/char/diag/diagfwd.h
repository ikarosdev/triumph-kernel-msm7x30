/* Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.
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

#ifndef DIAGFWD_H
#define DIAGFWD_H

//#define SLATE_DEBUG


#define DIAG_MASK_CMD_SAVE                           0
#define DIAG_MASK_CMD_RESTORE                        1
#define DIAG_MASK_CMD_ADD_GET_RSSI                   2
#define DIAG_MASK_CMD_ADD_GET_STATE_AND_CONN_ATT     3
#define DIAG_MASK_CMD_ADD_GET_SEARCHER_DUMP          4

/* Values for pending slate log commands */
#define DIAG_LOG_CMD_TYPE_NONE                    0
#define DIAG_LOG_CMD_TYPE_GET_RSSI                1
#define DIAG_LOG_CMD_TYPE_GET_STATE_AND_CONN_ATT  2
#define DIAG_LOG_CMD_TYPE_RESTORE_LOG_MASKS       3
#define DIAG_LOG_CMD_TYPE_GET_1x_SEARCHER_DUMP 	  4

/* Log Types */
#define DIAG_LOG_TYPE_RSSI                        0
#define DIAG_LOG_TYPE_STATE                       1
#define DIAG_LOG_TYPE_CONN_ATT                    2
//FXPCAYM-87
#define DIAG_LOG_TYPE_SEARCHER_AND_FINGER         3
#define DIAG_LOG_TYPE_INTERNAL_CORE_DUMP          4
#define DIAG_LOG_TYPE_SRCH_TNG_SEARCHER_DUMP      5


void diagfwd_init(void);
void diagfwd_exit(void);
void diag_process_hdlc(void *data, unsigned len);
void __diag_smd_send_req(void);
void __diag_smd_qdsp_send_req(void);
int diag_device_write(void *buf, int proc_num, struct diag_request *write_ptr);
int diagfwd_connect(void);
int diagfwd_disconnect(void);
int mask_request_validate(unsigned char mask_buf[]);

/* Routines added for SLATE support */
void diag_process_get_rssi_log(void);
void diag_process_get_stateAndConnInfo_log(void);
int  diag_log_is_enabled(unsigned char log_type);

/* State for diag forwarding */
extern int diag_debug_buf_idx;
extern unsigned char diag_debug_buf[1024];
#endif
