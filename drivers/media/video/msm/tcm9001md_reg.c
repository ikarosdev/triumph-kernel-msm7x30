/*
 *     tcm9001md_reg.c - Camera Sensor Config
 *
 *     Copyright (C) 2010 Kent Kwan <kentkwan@fihspec.com>
 *     Copyright (C) 2008 FIH CO., Inc.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; version 2 of the License.
 */

#include "tcm9001md.h"

#define PLK_BY_PASS //Div2-SW6-MM-MC-Camera-Modify_TCM9001MD_Init_Table-00+

#define Ini101221V2 1 //Div2-SW6-MM-MC-Modify_TCM9001MD_Init_Table-01+

#if 0//Div2-SW6-MM-MC-Modify_TCM9001MD_Init_Table-01-
struct tcm9001md_i2c_reg_conf const fc_init_tbl[] = {
{ 0x00, 0x48, FC_BYTE_LEN, 0 },
{ 0x01, 0x10, FC_BYTE_LEN, 0 },
{ 0x02, 0xD8, FC_BYTE_LEN, 0 },//alcint_sekiout[7:0];
{ 0x03, 0x00, FC_BYTE_LEN, 0 },//alc_ac5060out/alc_les_modeout[2:0]/*/*/alcint_sekiout[9:;
{ 0x04, 0x20, FC_BYTE_LEN, 0 },//alc_agout[7:0];
{ 0x05, 0x00, FC_BYTE_LEN, 0 },//alc_agout[11:8];
{ 0x06, 0x2D, FC_BYTE_LEN, 0 },//alc_dgout[7:0];
{ 0x07, 0x9D, FC_BYTE_LEN, 0 },//alc_esout[7:0];
{ 0x08, 0x80, FC_BYTE_LEN, 0 },//alc_okout//alc_esout[13:8];
{ 0x09, 0x45, FC_BYTE_LEN, 0 },//awb_uiout[7:0];
{ 0x0A, 0x17, FC_BYTE_LEN, 0 },//0x56
{ 0x0B, 0x05, FC_BYTE_LEN, 0 },//awb_uiout[23:16];
{ 0x0C, 0x00, FC_BYTE_LEN, 0 },//awb_uiout[29:24];
{ 0x0D, 0x39, FC_BYTE_LEN, 0 },//awb_viout[7:0];
{ 0x0E, 0x56, FC_BYTE_LEN, 0 },//awb_viout[15:8];
{ 0x0F, 0xFC, FC_BYTE_LEN, 0 },//awb_viout[2316];
{ 0x10, 0x3F, FC_BYTE_LEN, 0 },//awb_viout[29:24];
{ 0x11, 0xA1, FC_BYTE_LEN, 0 },//awb_pixout[7:0];
{ 0x12, 0x28, FC_BYTE_LEN, 0 },//awb_pixout[15:8];
{ 0x13, 0x03, FC_BYTE_LEN, 0 },//awb_pixout[18:16];
{ 0x14, 0x85, FC_BYTE_LEN, 0 },//awb_rgout[7:0];
{ 0x15, 0x80, FC_BYTE_LEN, 0 },//awb_ggout[7:0];
{ 0x16, 0x6B, FC_BYTE_LEN, 0 },//awb_bgout[7:0];
{ 0x17, 0x00, FC_BYTE_LEN, 0 },//
{ 0x18, 0x9C, FC_BYTE_LEN, 0 },//LSTB
{ 0x19, 0x04, FC_BYTE_LEN, 0 },//TSEL[2:0];
{ 0x1A, 0x90, FC_BYTE_LEN, 0 },//
//Div2-SW6-MM-MC-Camera-Modify_TCM9001MD_Init_Table-00*{
#ifdef PLK_BY_PASS
{ 0x1B, 0x01, FC_BYTE_LEN, 0 },//CKREF_DIV[4:0];
#else
{ 0x1B, 0x00, FC_BYTE_LEN, 0 },//CKREF_DIV[4:0];
#endif
{ 0x1C, 0x00, FC_BYTE_LEN, 0 },//CKVAR_SS0DIV[7:0];
#ifdef PLK_BY_PASS
{ 0x1D, 0x10, FC_BYTE_LEN, 0 },//SPCK_SEL/*/EXTCLK_THROUGH/*/*/*/CKVAR_SS0DIV[8];
#else
{ 0x1D, 0x00, FC_BYTE_LEN, 0 },//SPCK_SEL/*/EXTCLK_THROUGH/*/*/*/CKVAR_SS0DIV[8];
#endif
{ 0x1E, 0x8E, FC_BYTE_LEN, 0 },//CKVAR_SS1DIV[7:0]; // 0x8E
{ 0x1F, 0x00, FC_BYTE_LEN, 0 },//MRCK_DIV[3:0]/*/*/*/CKVAR_SS1DIV[8];
#ifdef PLK_BY_PASS
{ 0x20, 0xE0, FC_BYTE_LEN, 0 },//VCO_DIV[1:0]/*/CLK_SEL[1:0]/AMON0SEL[1:0]/*;
#else
{ 0x20, 0xC0, FC_BYTE_LEN, 0 },//VCO_DIV[1:0]/*/CLK_SEL[1:0]/AMON0SEL[1:0]/*;
#endif
//Div2-SW6-MM-MC-Camera-Modify_TCM9001MD_Init_Table-00*}
{ 0x21, 0x0B, FC_BYTE_LEN, 0 },//
{ 0x22, 0x07, FC_BYTE_LEN, 0 },//TBINV/RLINV//WIN_MODE//HV_INTERMIT[2:0];
{ 0x23, 0x96, FC_BYTE_LEN, 0 },//H_COUNT[7:0];
{ 0x24, 0x00, FC_BYTE_LEN, 0 },//
{ 0x25, 0x42, FC_BYTE_LEN, 0 },//V_COUNT[7:0];
{ 0x26, 0x00, FC_BYTE_LEN, 0 },//V_COUNT[10:8];
{ 0x27, 0x00, FC_BYTE_LEN, 0 },//
{ 0x28, 0x00, FC_BYTE_LEN, 0 },//
{ 0x29, 0x83, FC_BYTE_LEN, 0 },//
{ 0x2A, 0x84, FC_BYTE_LEN, 0 },//
{ 0x2B, 0xAE, FC_BYTE_LEN, 0 },//
{ 0x2C, 0x21, FC_BYTE_LEN, 0 },//
{ 0x2D, 0x00, FC_BYTE_LEN, 0 },//
{ 0x2E, 0x04, FC_BYTE_LEN, 0 },//
{ 0x2F, 0x7D, FC_BYTE_LEN, 0 },//
{ 0x30, 0x19, FC_BYTE_LEN, 0 },//
{ 0x31, 0x88, FC_BYTE_LEN, 0 },//
{ 0x32, 0x88, FC_BYTE_LEN, 0 },//
{ 0x33, 0x09, FC_BYTE_LEN, 0 },//
{ 0x34, 0x6C, FC_BYTE_LEN, 0 },//
{ 0x35, 0x00, FC_BYTE_LEN, 0 },//
{ 0x36, 0x90, FC_BYTE_LEN, 0 },//
{ 0x37, 0x22, FC_BYTE_LEN, 0 },//
{ 0x38, 0x0B, FC_BYTE_LEN, 0 },//
{ 0x39, 0xAA, FC_BYTE_LEN, 0 },//
{ 0x3A, 0x0A, FC_BYTE_LEN, 0 },//
{ 0x3B, 0x84, FC_BYTE_LEN, 0 },//
{ 0x3C, 0x03, FC_BYTE_LEN, 0 },//
{ 0x3D, 0x10, FC_BYTE_LEN, 0 },//
{ 0x3E, 0x4C, FC_BYTE_LEN, 0 },//
{ 0x3F, 0x1D, FC_BYTE_LEN, 0 },//
{ 0x40, 0x34, FC_BYTE_LEN, 0 },//
{ 0x41, 0x05, FC_BYTE_LEN, 0 },//
{ 0x42, 0x12, FC_BYTE_LEN, 0 },//
{ 0x43, 0xB0, FC_BYTE_LEN, 0 },//
{ 0x44, 0x3F, FC_BYTE_LEN, 0 },//
{ 0x45, 0xFF, FC_BYTE_LEN, 0 },//
{ 0x46, 0x44, FC_BYTE_LEN, 0 },//
{ 0x47, 0x44, FC_BYTE_LEN, 0 },//
{ 0x48, 0x00, FC_BYTE_LEN, 0 },//
{ 0x49, 0xE8, FC_BYTE_LEN, 0 },//
{ 0x4A, 0x00, FC_BYTE_LEN, 0 },//
{ 0x4B, 0x9F, FC_BYTE_LEN, 0 },//
{ 0x4C, 0x9B, FC_BYTE_LEN, 0 },//
{ 0x4D, 0x2B, FC_BYTE_LEN, 0 },//
{ 0x4E, 0x53, FC_BYTE_LEN, 0 },//
{ 0x4F, 0x50, FC_BYTE_LEN, 0 },//
{ 0x50, 0x0E, FC_BYTE_LEN, 0 },//
{ 0x51, 0x00, FC_BYTE_LEN, 0 },//
{ 0x52, 0x00, FC_BYTE_LEN, 0 },//
{ 0x53, 0x04, FC_BYTE_LEN, 0 },//TP_MODE[4:0]/TPG_DR_SEL/TPG_CBLK_SW/TPG_LINE_SW;
{ 0x54, 0x08, FC_BYTE_LEN, 0 },//
{ 0x55, 0x14, FC_BYTE_LEN, 0 },//
{ 0x56, 0x84, FC_BYTE_LEN, 0 },//
{ 0x57, 0x30, FC_BYTE_LEN, 0 },//
{ 0x58, 0x80, FC_BYTE_LEN, 0 },//
{ 0x59, 0x80, FC_BYTE_LEN, 0 },//
{ 0x5A, 0x00, FC_BYTE_LEN, 0 },//
{ 0x5B, 0x06, FC_BYTE_LEN, 0 },//
{ 0x5C, 0xF0, FC_BYTE_LEN, 0 },//
{ 0x5D, 0x00, FC_BYTE_LEN, 0 },//
{ 0x5E, 0x00, FC_BYTE_LEN, 0 },//
{ 0x5F, 0xB0, FC_BYTE_LEN, 0 },//
{ 0x60, 0x00, FC_BYTE_LEN, 0 },//
{ 0x61, 0x1B, FC_BYTE_LEN, 0 },//
{ 0x62, 0x4F, FC_BYTE_LEN, 0 },//HYUKO_START[7:0];
{ 0x63, 0x04, FC_BYTE_LEN, 0 },//VYUKO_START[7:0];
{ 0x64, 0x10, FC_BYTE_LEN, 0 },//
{ 0x65, 0x20, FC_BYTE_LEN, 0 },//
{ 0x66, 0x30, FC_BYTE_LEN, 0 },//
{ 0x67, 0x28, FC_BYTE_LEN, 0 },//
{ 0x68, 0x66, FC_BYTE_LEN, 0 },//
{ 0x69, 0xC0, FC_BYTE_LEN, 0 },//
{ 0x6A, 0x30, FC_BYTE_LEN, 0 },//
{ 0x6B, 0x30, FC_BYTE_LEN, 0 },//
{ 0x6C, 0x3F, FC_BYTE_LEN, 0 },//
{ 0x6D, 0xBF, FC_BYTE_LEN, 0 },//
{ 0x6E, 0xAB, FC_BYTE_LEN, 0 },//
{ 0x6F, 0x30, FC_BYTE_LEN, 0 },//
{ 0x70, 0x80, FC_BYTE_LEN, 0 },//AGMIN_BLACK_ADJ[7:0];
{ 0x71, 0x90, FC_BYTE_LEN, 0 },//AGMAX_BLACK_ADJ[7:0];
{ 0x72, 0x00, FC_BYTE_LEN, 0 },//IDR_SET[7:0];
{ 0x73, 0x28, FC_BYTE_LEN, 0 },//PWB_RG[7:0];
{ 0x74, 0x04, FC_BYTE_LEN, 0 },//PWB_GRG[7:0];
{ 0x75, 0x04, FC_BYTE_LEN, 0 },//PWB_GBG[7:0];
{ 0x76, 0x58, FC_BYTE_LEN, 0 },//PWB_BG[7:0];
{ 0x77, 0x00, FC_BYTE_LEN, 0 },//
{ 0x78, 0x80, FC_BYTE_LEN, 0 },//LSSC_SW
{ 0x79, 0x52, FC_BYTE_LEN, 0 },//
{ 0x7A, 0x4F, FC_BYTE_LEN, 0 },//
{ 0x7B, 0xA6, FC_BYTE_LEN, 0 },//LSSC_LEFT_RG[7:0];
{ 0x7C, 0x62, FC_BYTE_LEN, 0 },//LSSC_LEFT_GG[7:0];
{ 0x7D, 0x56, FC_BYTE_LEN, 0 },//LSSC_LEFT_BG[7:0];
{ 0x7E, 0xE9, FC_BYTE_LEN, 0 },//LSSC_RIGHT_RG[7:0];
{ 0x7F, 0x87, FC_BYTE_LEN, 0 },//LSSC_RIGHT_GG[7:0];
{ 0x80, 0x70, FC_BYTE_LEN, 0 },//LSSC_RIGHT_BG[7:0];
{ 0x81, 0xA5, FC_BYTE_LEN, 0 },//LSSC_TOP_RG[7:0];
{ 0x82, 0x77, FC_BYTE_LEN, 0 },//LSSC_TOP_GG[7:0];
{ 0x83, 0x73, FC_BYTE_LEN, 0 },//LSSC_TOP_BG[7:0];
{ 0x84, 0xA1, FC_BYTE_LEN, 0 },//LSSC_BOTTOM_RG[7:0];
{ 0x85, 0x6C, FC_BYTE_LEN, 0 },//LSSC_BOTTOM_GG[7:0];
{ 0x86, 0x66, FC_BYTE_LEN, 0 },//LSSC_BOTTOM_BG[7:0];
{ 0x87, 0x01, FC_BYTE_LEN, 0 },//
{ 0x88, 0x00, FC_BYTE_LEN, 0 },//
{ 0x89, 0x00, FC_BYTE_LEN, 0 },//
{ 0x8A, 0x40, FC_BYTE_LEN, 0 },//
{ 0x8B, 0x09, FC_BYTE_LEN, 0 },//
{ 0x8C, 0xE0, FC_BYTE_LEN, 0 },//
{ 0x8D, 0xC0, FC_BYTE_LEN, 0 },//
{ 0x8E, 0x80, FC_BYTE_LEN, 0 },//
{ 0x8F, 0xC0, FC_BYTE_LEN, 0 },//
{ 0x90, 0x80, FC_BYTE_LEN, 0 },//
{ 0x91, 0x80, FC_BYTE_LEN, 0 },//ANR_SW/*/*/*/TEST_ANR/*/*/*;
{ 0x92, 0x80, FC_BYTE_LEN, 0 },//AGMIN_ANR_WIDTH[7:0];
{ 0x93, 0x80, FC_BYTE_LEN, 0 },//AGMAX_ANR_WIDTH[7:0];
{ 0x94, 0x40, FC_BYTE_LEN, 0 },//AGMIN_ANR_MP[7:0];
{ 0x95, 0x40, FC_BYTE_LEN, 0 },//AGMAX_ANR_MP[7:0];
{ 0x96, 0x80, FC_BYTE_LEN, 0 },//DTL_SW
{ 0x97, 0x20, FC_BYTE_LEN, 0 },//AGMIN_HDTL_NC[7:0];
{ 0x98, 0x68, FC_BYTE_LEN, 0 },//AGMIN_VDTL_NC[7:0];
{ 0x99, 0xF8, FC_BYTE_LEN, 0 },//AGMAX_HDTL_NC[7:0];
{ 0x9A, 0xF8, FC_BYTE_LEN, 0 },//AGMAX_VDTL_NC[7:0];
{ 0x9B, 0x80, FC_BYTE_LEN, 0 },//AGMIN_HDTL_MG[7:0];
{ 0x9C, 0x20, FC_BYTE_LEN, 0 },//AGMIN_HDTL_PG[7:0];
{ 0x9D, 0x40, FC_BYTE_LEN, 0 },//AGMIN_VDTL_MG[7:0];
{ 0x9E, 0x10, FC_BYTE_LEN, 0 },//AGMIN_VDTL_PG[7:0];
{ 0x9F, 0x80, FC_BYTE_LEN, 0 },//AGMAX_HDTL_MG[7:0];
{ 0xA0, 0x20, FC_BYTE_LEN, 0 },//AGMAX_HDTL_PG[7:0];
{ 0xA1, 0x40, FC_BYTE_LEN, 0 },//AGMAX_VDTL_MG[7:0];
{ 0xA2, 0x10, FC_BYTE_LEN, 0 },//AGMAX_VDTL_PG[7:0];
{ 0xA3, 0x80, FC_BYTE_LEN, 0 },//
{ 0xA4, 0x82, FC_BYTE_LEN, 0 },//HCBC_SW
{ 0xA5, 0x38, FC_BYTE_LEN, 0 },//AGMIN_HCBC_G[7:0];
{ 0xA6, 0x38, FC_BYTE_LEN, 0 },//AGMAX_HCBC_G[7:0];
{ 0xA7, 0x84, FC_BYTE_LEN, 0 },//
{ 0xA8, 0x84, FC_BYTE_LEN, 0 },//
{ 0xA9, 0x84, FC_BYTE_LEN, 0 },//
{ 0xAA, 0x09, FC_BYTE_LEN, 0 },//LMCC_BMG_SEL/LMCC_BMR_SEL/*/LMCC_GMB_SEL/LMCC_GMR_SEL/*/;
{ 0xAB, 0x04, FC_BYTE_LEN, 0 },//LMCC_RMG_G[7:0];
{ 0xAC, 0x10, FC_BYTE_LEN, 0 },//LMCC_RMB_G[7:0];
{ 0xAD, 0x04, FC_BYTE_LEN, 0 },//LMCC_GMR_G[7:0];
{ 0xAE, 0x08, FC_BYTE_LEN, 0 },//LMCC_GMB_G[7:0];
{ 0xAF, 0x40, FC_BYTE_LEN, 0 },//LMCC_BMR_G[7:0];
{ 0xB0, 0x40, FC_BYTE_LEN, 0 },//LMCC_BMG_G[7:0];
{ 0xB1, 0x40, FC_BYTE_LEN, 0 },//GAM_SW[1:0]/*/CGC_DISP/TEST_AWBDISP/*/YUVM_AWBDISP_SW/YU;
{ 0xB2, 0x4D, FC_BYTE_LEN, 0 },//*/R_MATRIX[6:0];
{ 0xB3, 0x1C, FC_BYTE_LEN, 0 },//*/B_MATRIX[6:0];
{ 0xB4, 0xC8, FC_BYTE_LEN, 0 },//UVG_SEL/BRIGHT_SEL/*/TEST_YUVM_PE/NEG_YMIN_SW/PIC_EFFECT;
{ 0xB5, 0x48, FC_BYTE_LEN, 0 },//CONTRAST[7:0];
{ 0xB6, 0x72, FC_BYTE_LEN, 0 },//BRIGHT[7:0];
{ 0xB7, 0x00, FC_BYTE_LEN, 0 },//Y_MIN[7:0];
{ 0xB8, 0xFF, FC_BYTE_LEN, 0 },//Y_MAX[7:0];
{ 0xB9, 0x4C, FC_BYTE_LEN, 0 },//U_GAIN[7:0];
{ 0xBA, 0x78, FC_BYTE_LEN, 0 },//V_GAIN[7:0];
{ 0xBB, 0x78, FC_BYTE_LEN, 0 },//SEPIA_US[7:0];
{ 0xBC, 0x90, FC_BYTE_LEN, 0 },//SEPIA_VS[7:0];
{ 0xBD, 0x03, FC_BYTE_LEN, 0 },//U_CORING[7:0];
{ 0xBE, 0x03, FC_BYTE_LEN, 0 },//V_CORING[7:0];
{ 0xBF, 0xC0, FC_BYTE_LEN, 0 },//
{ 0xC0, 0x00, FC_BYTE_LEN, 0 },//
{ 0xC1, 0x00, FC_BYTE_LEN, 0 },//
{ 0xC2, 0x80, FC_BYTE_LEN, 0 },//ALC_SW/ALC_LOCK
{ 0xC3, 0x14, FC_BYTE_LEN, 0 },//MES[7:0];
{ 0xC4, 0x03, FC_BYTE_LEN, 0 },//MES[13:8];
{ 0xC5, 0x00, FC_BYTE_LEN, 0 },//MDG[7:0];
{ 0xC6, 0x74, FC_BYTE_LEN, 0 },//MAG[7:0];
{ 0xC7, 0x80, FC_BYTE_LEN, 0 },//AGCONT_SEL[1:0]/*/*/MAG[11:8];
{ 0xC8, 0x20, FC_BYTE_LEN, 0 },//AG_MIN[7:0];
{ 0xC9, 0x06, FC_BYTE_LEN, 0 },//AG_MAX[7:0];
{ 0xCA, 0x23, FC_BYTE_LEN, 0 },//AUTO_LES_SW/AUTO_LES_MODE[2:0]/ALC_WEIGHT[1:0]/FLCKADJ[1;
{ 0xCB, 0xC2, FC_BYTE_LEN, 0 },//ALC_LV[7:0];
{ 0xCC, 0x10, FC_BYTE_LEN, 0 },//*/UPDN_MODE[2:0]/ALC_LV[9:8];
{ 0xCD, 0x0A, FC_BYTE_LEN, 0 },//ALC_LVW[7:0];
{ 0xCE, 0x93, FC_BYTE_LEN, 0 },//L64P600S[7:0];
{ 0xCF, 0x06, FC_BYTE_LEN, 0 },//*/ALC_VWAIT[2:0]/L64P600S[11:8];
{ 0xD0, 0x80, FC_BYTE_LEN, 0 },//UPDN_SPD[7:0];
{ 0xD1, 0x20, FC_BYTE_LEN, 0 },//
{ 0xD2, 0x80, FC_BYTE_LEN, 0 },//NEAR_SPD[7:0];
{ 0xD3, 0x30, FC_BYTE_LEN, 0 },//
{ 0xD4, 0x8A, FC_BYTE_LEN, 0 },//AC5060/*/ALC_SAFETY[5:0];
{ 0xD5, 0x02, FC_BYTE_LEN, 0 },//*/*/*/*/*/ALC_SAFETY2[2:0];
{ 0xD6, 0x4F, FC_BYTE_LEN, 0 },//
{ 0xD7, 0x08, FC_BYTE_LEN, 0 },//
{ 0xD8, 0x00, FC_BYTE_LEN, 0 },//
{ 0xD9, 0xFF, FC_BYTE_LEN, 0 },//
{ 0xDA, 0x01, FC_BYTE_LEN, 0 },//
{ 0xDB, 0x00, FC_BYTE_LEN, 0 },//
{ 0xDC, 0x14, FC_BYTE_LEN, 0 },//
{ 0xDD, 0x00, FC_BYTE_LEN, 0 },//
{ 0xDE, 0x80, FC_BYTE_LEN, 0 },//AWB_SW/AWB_LOCK/*/AWB_TEST/*/*/HEXG_SLOPE_SEL/COLGATE_SE;
{ 0xDF, 0x80, FC_BYTE_LEN, 0 },//WB_MRG[7:0];
{ 0xE0, 0x80, FC_BYTE_LEN, 0 },//WB_MGG[7:0];
{ 0xE1, 0x80, FC_BYTE_LEN, 0 },//WB_MBG[7:0];
{ 0xE2, 0x22, FC_BYTE_LEN, 0 },//WB_RBMIN[7:0];
{ 0xE3, 0xF8, FC_BYTE_LEN, 0 },//WB_RBMAX[7:0];
{ 0xE4, 0x90, FC_BYTE_LEN, 0 },//HEXA_SW/*/COLGATE_RANGE[1:0]/*/*/*/COLGATE_OPEN;
{ 0xE5, 0x3C, FC_BYTE_LEN, 0 },//*/RYCUTM[6:0];
{ 0xE6, 0x54, FC_BYTE_LEN, 0 },//*/RYCUTP[6:0];
{ 0xE7, 0x28, FC_BYTE_LEN, 0 },//*/BYCUTM[6:0];
{ 0xE8, 0x50, FC_BYTE_LEN, 0 },//*/BYCUTP[6:0];
{ 0xE9, 0xE4, FC_BYTE_LEN, 0 },//RBCUTL[7:0];
{ 0xEA, 0x3C, FC_BYTE_LEN, 0 },//RBCUTH[7:0];
{ 0xEB, 0x81, FC_BYTE_LEN, 0 },//SQ1_SW/SQ1_POL/*/*/*/*/*/YGATE_SW;
{ 0xEC, 0x37, FC_BYTE_LEN, 0 },//RYCUT1L[7:0];
{ 0xED, 0x5A, FC_BYTE_LEN, 0 },//RYCUT1H[7:0];
{ 0xEE, 0xDE, FC_BYTE_LEN, 0 },//BYCUT1L[7:0];
{ 0xEF, 0x08, FC_BYTE_LEN, 0 },//BYCUT1H[7:0];
{ 0xF0, 0x18, FC_BYTE_LEN, 0 },//YGATE_L[7:0];
{ 0xF1, 0xFE, FC_BYTE_LEN, 0 },//YGATE_H[7:0];
{ 0xF2, 0x00, FC_BYTE_LEN, 0 },//*/*/IPIX_DISP_SW/*/*/*/*/;
{ 0xF3, 0x02, FC_BYTE_LEN, 0 },//
{ 0xF4, 0x02, FC_BYTE_LEN, 0 },//
{ 0xF5, 0x04, FC_BYTE_LEN, 0 },//AWB_WAIT[7:0];
{ 0xF6, 0x00, FC_BYTE_LEN, 0 },//AWB_SPDDLY[7:0];
{ 0xF7, 0x20, FC_BYTE_LEN, 0 },//*/*/AWB_SPD[5:0];
{ 0xF8, 0x86, FC_BYTE_LEN, 0 },//
{ 0xF9, 0x00, FC_BYTE_LEN, 0 },//
{ 0xFA, 0x41, FC_BYTE_LEN, 0 },//MR_HBLK_START[7:0];
{ 0xFB, 0x50, FC_BYTE_LEN, 0 },//*/MR_HBLK_WIDTH[6:0];
{ 0xFC, 0x0C, FC_BYTE_LEN, 0 },//MR_VBLK_START[7:0];
{ 0xFD, 0x3C, FC_BYTE_LEN, 0 },//*/*/MR_VBLK_WIDTH[5:0];
{ 0xFE, 0x50, FC_BYTE_LEN, 0 },//PIC_FORMAT[3:0]/PINTEST_SEL[3:0];
{ 0xFF, 0x85, FC_BYTE_LEN, 0 },//SLEEP/*/PARALLEL_OUTSW[1:0]/DCLK_POL/DOUT_CBLK_SW/*/AL;
};
#endif

//Div2-SW6-MM-MC-Modify_TCM9001MD_Init_Table-01+{
#ifdef Ini101221V2
struct tcm9001md_i2c_reg_conf const fc_init_tbl[] = {
{ 0x00, 0x48, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x01, 0x10, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x02, 0xDC, FC_BYTE_LEN, 0 },// alcint_sekiout[7:0];
{ 0x03, 0x80, FC_BYTE_LEN, 0 },// alc_ac5060out/alc_les_modeout[2:0]/*/*/alcint_sekiout[9:;
{ 0x04, 0x20, FC_BYTE_LEN, 0 },// alc_agout[7:0];
{ 0x05, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/alc_agout[11:8];
{ 0x06, 0x00, FC_BYTE_LEN, 0 },// alc_dgout[7:0];
{ 0x07, 0x10, FC_BYTE_LEN, 0 },// alc_esout[7:0];
{ 0x08, 0x80, FC_BYTE_LEN, 0 },// alc_okout//alc_esout[13:8];
{ 0x09, 0xBA, FC_BYTE_LEN, 0 },// awb_uiout[7:0];
{ 0x0A, 0x7B, FC_BYTE_LEN, 0 },// awb_uiout[15:8];
{ 0x0B, 0xFD, FC_BYTE_LEN, 0 },// awb_uiout[23:16];
{ 0x0C, 0x3F, FC_BYTE_LEN, 0 },// */*/awb_uiout[29:24];
{ 0x0D, 0x33, FC_BYTE_LEN, 0 },// awb_viout[7:0];
{ 0x0E, 0x39, FC_BYTE_LEN, 0 },// awb_viout[15:8];
{ 0x0F, 0xFE, FC_BYTE_LEN, 0 },// awb_viout[2316];
{ 0x10, 0x3F, FC_BYTE_LEN, 0 },// */*/awb_viout[29:24];
{ 0x11, 0xCA, FC_BYTE_LEN, 0 },// awb_pixout[7:0];
{ 0x12, 0xAF, FC_BYTE_LEN, 0 },// awb_pixout[15:8];
{ 0x13, 0x04, FC_BYTE_LEN, 0 },// */*/*/*/*/awb_pixout[18:16];
{ 0x14, 0x77, FC_BYTE_LEN, 0 },// awb_rgout[7:0];
{ 0x15, 0x80, FC_BYTE_LEN, 0 },// awb_ggout[7:0];
{ 0x16, 0x79, FC_BYTE_LEN, 0 },// awb_bgout[7:0];
{ 0x17, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x18, 0x9C, FC_BYTE_LEN, 0 },// LSTB/*/*/*/*/*/*/*;
{ 0x19, 0x04, FC_BYTE_LEN, 0 },// */*/*/*/*/TSEL[2:0];
{ 0x1A, 0x90, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
#ifdef PLK_BY_PASS
{ 0x1B, 0x01, FC_BYTE_LEN, 0 },//CKREF_DIV[4:0];
#else
{ 0x1B, 0x00, FC_BYTE_LEN, 0 },// */*/*/CKREF_DIV[4:0];
#endif
{ 0x1C, 0x00, FC_BYTE_LEN, 0 },// CKVAR_SS0DIV[7:0];
#ifdef PLK_BY_PASS
{ 0x1D, 0x10, FC_BYTE_LEN, 0 },//SPCK_SEL/*/EXTCLK_THROUGH/*/*/*/CKVAR_SS0DIV[8];
#else
{ 0x1D, 0x00, FC_BYTE_LEN, 0 },// */SPCK_SEL/*/EXTCLK_THROUGH/*/*/*/CKVAR_SS0DIV[8];
#endif
{ 0x1E, 0x8E, FC_BYTE_LEN, 0 },// CKVAR_SS1DIV[7:0];
{ 0x1F, 0x00, FC_BYTE_LEN, 0 },// MRCK_DIV[3:0]/*/*/*/CKVAR_SS1DIV[8];
#ifdef PLK_BY_PASS
{ 0x20, 0xE0, FC_BYTE_LEN, 0 },//VCO_DIV[1:0]/*/CLK_SEL[1:0]/AMON0SEL[1:0]/*;
#else
{ 0x20, 0xC0, FC_BYTE_LEN, 0 },// VCO_DIV[1:0]/*/CLK_SEL[1:0]/AMON0SEL[1:0]/*;
#endif
{ 0x21, 0x0B, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x22, 0x47, FC_BYTE_LEN, 0 },// TBINV/RLINV//WIN_MODE//HV_INTERMIT[2:0];//Div2-SW6-MM-MC-EnableHorizontalFlip-00+
{ 0x23, 0x96, FC_BYTE_LEN, 0 },// H_COUNT[7:0];
{ 0x24, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x25, 0x42, FC_BYTE_LEN, 0 },// V_COUNT[7:0];
{ 0x26, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/V_COUNT[10:8];
{ 0x27, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x28, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x29, 0x83, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x2A, 0x84, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x2B, 0xAE, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x2C, 0x21, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x2D, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x2E, 0x04, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x2F, 0x7D, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x30, 0x19, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x31, 0x88, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x32, 0x88, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x33, 0x09, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x34, 0x6C, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x35, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x36, 0x90, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x37, 0x22, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x38, 0x0B, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x39, 0xAA, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x3A, 0x0A, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x3B, 0x84, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x3C, 0x03, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x3D, 0x10, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x3E, 0x4C, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x3F, 0x1D, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x40, 0x34, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x41, 0x05, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x42, 0x12, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x43, 0xB0, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x44, 0x3F, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x45, 0xFF, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x46, 0x44, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x47, 0x44, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x48, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x49, 0xE8, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x4A, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x4B, 0x9F, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x4C, 0x9B, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x4D, 0x2B, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x4E, 0x53, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x4F, 0x50, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x50, 0x0E, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x51, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x52, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x53, 0x04, FC_BYTE_LEN, 0 },// TP_MODE[4:0]/TPG_DR_SEL/TPG_CBLK_SW/TPG_LINE_SW;
{ 0x54, 0x08, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x55, 0x14, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x56, 0x84, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x57, 0x30, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x58, 0x80, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x59, 0x80, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x5A, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x5B, 0x06, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x5C, 0xF0, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x5D, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x5E, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x5F, 0xB0, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x60, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x61, 0x1B, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x62, 0x4F, FC_BYTE_LEN, 0 },// HYUKO_START[7:0];
{ 0x63, 0x04, FC_BYTE_LEN, 0 },// VYUKO_START[7:0];
{ 0x64, 0x10, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x65, 0x20, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x66, 0x30, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x67, 0x28, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x68, 0x66, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x69, 0xC0, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x6A, 0x30, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x6B, 0x30, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x6C, 0x3F, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x6D, 0xBF, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x6E, 0xAB, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x6F, 0x30, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x70, 0x80, FC_BYTE_LEN, 0 },// AGMIN_BLACK_ADJ[7:0];
{ 0x71, 0x90, FC_BYTE_LEN, 0 },// AGMAX_BLACK_ADJ[7:0];
{ 0x72, 0x00, FC_BYTE_LEN, 0 },// IDR_SET[7:0];
{ 0x73, 0x28, FC_BYTE_LEN, 0 },// PWB_RG[7:0];
{ 0x74, 0x00, FC_BYTE_LEN, 0 },// PWB_GRG[7:0];
{ 0x75, 0x00, FC_BYTE_LEN, 0 },// PWB_GBG[7:0];
{ 0x76, 0x58, FC_BYTE_LEN, 0 },// PWB_BG[7:0];
{ 0x77, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x78, 0x80, FC_BYTE_LEN, 0 },// LSSC_SW/*/*/*/*/*/*/*/;
{ 0x79, 0x52, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x7A, 0x4F, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x7B, 0xCD, FC_BYTE_LEN, 0 },// LSSC_LEFT_RG[7:0];
{ 0x7C, 0x86, FC_BYTE_LEN, 0 },// LSSC_LEFT_GG[7:0];
{ 0x7D, 0x6A, FC_BYTE_LEN, 0 },// LSSC_LEFT_BG[7:0];
{ 0x7E, 0xDA, FC_BYTE_LEN, 0 },// LSSC_RIGHT_RG[7:0];
{ 0x7F, 0x8E, FC_BYTE_LEN, 0 },// LSSC_RIGHT_GG[7:0];
{ 0x80, 0x6B, FC_BYTE_LEN, 0 },// LSSC_RIGHT_BG[7:0];
{ 0x81, 0xCE, FC_BYTE_LEN, 0 },// LSSC_TOP_RG[7:0];
{ 0x82, 0x9D, FC_BYTE_LEN, 0 },// LSSC_TOP_GG[7:0];
{ 0x83, 0x8F, FC_BYTE_LEN, 0 },// LSSC_TOP_BG[7:0];
{ 0x84, 0xB9, FC_BYTE_LEN, 0 },// LSSC_BOTTOM_RG[7:0];
{ 0x85, 0x8F, FC_BYTE_LEN, 0 },// LSSC_BOTTOM_GG[7:0];
{ 0x86, 0x76, FC_BYTE_LEN, 0 },// LSSC_BOTTOM_BG[7:0];
{ 0x87, 0x01, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x88, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x89, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x8A, 0x40, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x8B, 0x09, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x8C, 0xE0, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x8D, 0x20, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x8E, 0x20, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x8F, 0x20, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x90, 0x20, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0x91, 0x80, FC_BYTE_LEN, 0 },// ANR_SW/*/*/*/TEST_ANR/*/*/*;
{ 0x92, 0xDF, FC_BYTE_LEN, 0 },// AGMIN_ANR_WIDTH[7:0];
{ 0x93, 0xDF, FC_BYTE_LEN, 0 },// AGMAX_ANR_WIDTH[7:0];
{ 0x94, 0x40, FC_BYTE_LEN, 0 },// AGMIN_ANR_MP[7:0];
{ 0x95, 0x40, FC_BYTE_LEN, 0 },// AGMAX_ANR_MP[7:0];
{ 0x96, 0x80, FC_BYTE_LEN, 0 },// DTL_SW/*/*/*/*/*/*/*/;
{ 0x97, 0x20, FC_BYTE_LEN, 0 },// AGMIN_HDTL_NC[7:0];
{ 0x98, 0x68, FC_BYTE_LEN, 0 },// AGMIN_VDTL_NC[7:0];
{ 0x99, 0xFF, FC_BYTE_LEN, 0 },// AGMAX_HDTL_NC[7:0];
{ 0x9A, 0xFF, FC_BYTE_LEN, 0 },// AGMAX_VDTL_NC[7:0];
{ 0x9B, 0xD9, FC_BYTE_LEN, 0 },// AGMIN_HDTL_MG[7:0];
{ 0x9C, 0x68, FC_BYTE_LEN, 0 },// AGMIN_HDTL_PG[7:0];
{ 0x9D, 0x90, FC_BYTE_LEN, 0 },// AGMIN_VDTL_MG[7:0];
{ 0x9E, 0x68, FC_BYTE_LEN, 0 },// AGMIN_VDTL_PG[7:0];
{ 0x9F, 0x00, FC_BYTE_LEN, 0 },// AGMAX_HDTL_MG[7:0];
{ 0xA0, 0x00, FC_BYTE_LEN, 0 },// AGMAX_HDTL_PG[7:0];
{ 0xA1, 0x00, FC_BYTE_LEN, 0 },// AGMAX_VDTL_MG[7:0];
{ 0xA2, 0x00, FC_BYTE_LEN, 0 },// AGMAX_VDTL_PG[7:0];
{ 0xA3, 0x80, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xA4, 0x82, FC_BYTE_LEN, 0 },// HCBC_SW/*/*/*/*/*/*/*/;
{ 0xA5, 0x38, FC_BYTE_LEN, 0 },// AGMIN_HCBC_G[7:0];
{ 0xA6, 0x18, FC_BYTE_LEN, 0 },// AGMAX_HCBC_G[7:0];
{ 0xA7, 0x98, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xA8, 0x98, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xA9, 0x98, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xAA, 0x10, FC_BYTE_LEN, 0 },// LMCC_BMG_SEL/LMCC_BMR_SEL/*/LMCC_GMB_SEL/LMCC_GMR_SEL/*/;
{ 0xAB, 0x5B, FC_BYTE_LEN, 0 },// LMCC_RMG_G[7:0];
{ 0xAC, 0x00, FC_BYTE_LEN, 0 },// LMCC_RMB_G[7:0];
{ 0xAD, 0x00, FC_BYTE_LEN, 0 },// LMCC_GMR_G[7:0];
{ 0xAE, 0x00, FC_BYTE_LEN, 0 },// LMCC_GMB_G[7:0];
{ 0xAF, 0x00, FC_BYTE_LEN, 0 },// LMCC_BMR_G[7:0];
{ 0xB0, 0x48, FC_BYTE_LEN, 0 },// LMCC_BMG_G[7:0];
{ 0xB1, 0xC0, FC_BYTE_LEN, 0 },// GAM_SW[1:0]/*/CGC_DISP/TEST_AWBDISP/*/YUVM_AWBDISP_SW/YU;
{ 0xB2, 0x4D, FC_BYTE_LEN, 0 },// */R_MATRIX[6:0];
{ 0xB3, 0x10, FC_BYTE_LEN, 0 },// */B_MATRIX[6:0];
{ 0xB4, 0xC8, FC_BYTE_LEN, 0 },// UVG_SEL/BRIGHT_SEL/*/TEST_YUVM_PE/NEG_YMIN_SW/PIC_EFFECT;
{ 0xB5, 0x5C, FC_BYTE_LEN, 0 },// CONTRAST[7:0];
{ 0xB6, 0x57, FC_BYTE_LEN, 0 },// BRIGHT[7:0];
{ 0xB7, 0x00, FC_BYTE_LEN, 0 },// Y_MIN[7:0];
{ 0xB8, 0xFF, FC_BYTE_LEN, 0 },// Y_MAX[7:0];
{ 0xB9, 0x72, FC_BYTE_LEN, 0 },// U_GAIN[7:0];
{ 0xBA, 0x75, FC_BYTE_LEN, 0 },// V_GAIN[7:0];
{ 0xBB, 0x78, FC_BYTE_LEN, 0 },// SEPIA_US[7:0];
{ 0xBC, 0x90, FC_BYTE_LEN, 0 },// SEPIA_VS[7:0];
{ 0xBD, 0x04, FC_BYTE_LEN, 0 },// U_CORING[7:0];
{ 0xBE, 0x04, FC_BYTE_LEN, 0 },// V_CORING[7:0];
{ 0xBF, 0xC0, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xC0, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xC1, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xC2, 0x80, FC_BYTE_LEN, 0 },// ALC_SW/ALC_LOCK/*/*/*/*/*/*/;
{ 0xC3, 0x14, FC_BYTE_LEN, 0 },// MES[7:0];
{ 0xC4, 0x03, FC_BYTE_LEN, 0 },// */*/MES[13:8];
{ 0xC5, 0x00, FC_BYTE_LEN, 0 },// MDG[7:0];
{ 0xC6, 0x74, FC_BYTE_LEN, 0 },// MAG[7:0];
{ 0xC7, 0x80, FC_BYTE_LEN, 0 },// AGCONT_SEL[1:0]/*/*/MAG[11:8];
{ 0xC8, 0x30, FC_BYTE_LEN, 0 },// AG_MIN[7:0];
{ 0xC9, 0x06, FC_BYTE_LEN, 0 },// AG_MAX[7:0];
{ 0xCA, 0xAF, FC_BYTE_LEN, 0 },// AUTO_LES_SW/AUTO_LES_MODE[2:0]/ALC_WEIGHT[1:0]/FLCKADJ[1;
{ 0xCB, 0xD2, FC_BYTE_LEN, 0 },// ALC_LV[7:0];
{ 0xCC, 0x10, FC_BYTE_LEN, 0 },// */UPDN_MODE[2:0]/ALC_LV[9:8];
{ 0xCD, 0x0A, FC_BYTE_LEN, 0 },// ALC_LVW[7:0];
{ 0xCE, 0x93, FC_BYTE_LEN, 0 },// L64P600S[7:0];
{ 0xCF, 0x06, FC_BYTE_LEN, 0 },// */ALC_VWAIT[2:0]/L64P600S[11:8];
{ 0xD0, 0x80, FC_BYTE_LEN, 0 },// UPDN_SPD[7:0];
{ 0xD1, 0x20, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xD2, 0x80, FC_BYTE_LEN, 0 },// NEAR_SPD[7:0];
{ 0xD3, 0x30, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xD4, 0x8A, FC_BYTE_LEN, 0 },// AC5060/*/ALC_SAFETY[5:0];
{ 0xD5, 0x02, FC_BYTE_LEN, 0 },// */*/*/*/*/ALC_SAFETY2[2:0];
{ 0xD6, 0x4F, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xD7, 0x08, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xD8, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xD9, 0xFF, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xDA, 0x01, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xDB, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xDC, 0x14, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xDD, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xDE, 0x80, FC_BYTE_LEN, 0 },// AWB_SW/AWB_LOCK/*/AWB_TEST/*/*/HEXG_SLOPE_SEL/COLGATE_SE;
{ 0xDF, 0x80, FC_BYTE_LEN, 0 },// WB_MRG[7:0];
{ 0xE0, 0x80, FC_BYTE_LEN, 0 },// WB_MGG[7:0];
{ 0xE1, 0x80, FC_BYTE_LEN, 0 },// WB_MBG[7:0];
{ 0xE2, 0x22, FC_BYTE_LEN, 0 },// WB_RBMIN[7:0];
{ 0xE3, 0xF8, FC_BYTE_LEN, 0 },// WB_RBMAX[7:0];
{ 0xE4, 0x80, FC_BYTE_LEN, 0 },// HEXA_SW/*/COLGATE_RANGE[1:0]/*/*/*/COLGATE_OPEN;
{ 0xE5, 0x2C, FC_BYTE_LEN, 0 },// */RYCUTM[6:0];
{ 0xE6, 0x54, FC_BYTE_LEN, 0 },// */RYCUTP[6:0];
{ 0xE7, 0x28, FC_BYTE_LEN, 0 },// */BYCUTM[6:0];
{ 0xE8, 0x39, FC_BYTE_LEN, 0 },// */BYCUTP[6:0];
{ 0xE9, 0xE4, FC_BYTE_LEN, 0 },// RBCUTL[7:0];
{ 0xEA, 0x3C, FC_BYTE_LEN, 0 },// RBCUTH[7:0];
{ 0xEB, 0x81, FC_BYTE_LEN, 0 },// SQ1_SW/SQ1_POL/*/*/*/*/*/YGATE_SW;
{ 0xEC, 0x37, FC_BYTE_LEN, 0 },// RYCUT1L[7:0];
{ 0xED, 0x5A, FC_BYTE_LEN, 0 },// RYCUT1H[7:0];
{ 0xEE, 0xDE, FC_BYTE_LEN, 0 },// BYCUT1L[7:0];
{ 0xEF, 0x08, FC_BYTE_LEN, 0 },// BYCUT1H[7:0];
{ 0xF0, 0x18, FC_BYTE_LEN, 0 },// YGATE_L[7:0];
{ 0xF1, 0xFE, FC_BYTE_LEN, 0 },// YGATE_H[7:0];
{ 0xF2, 0x00, FC_BYTE_LEN, 0 },// */*/IPIX_DISP_SW/*/*/*/*/;
{ 0xF3, 0x02, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xF4, 0x02, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xF5, 0x04, FC_BYTE_LEN, 0 },// AWB_WAIT[7:0];
{ 0xF6, 0x00, FC_BYTE_LEN, 0 },// AWB_SPDDLY[7:0];
{ 0xF7, 0x20, FC_BYTE_LEN, 0 },// */*/AWB_SPD[5:0];
{ 0xF8, 0x86, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xF9, 0x00, FC_BYTE_LEN, 0 },// */*/*/*/*/*/*/*;
{ 0xFA, 0x41, FC_BYTE_LEN, 0 },// MR_HBLK_START[7:0];
{ 0xFB, 0x50, FC_BYTE_LEN, 0 },// */MR_HBLK_WIDTH[6:0];
{ 0xFC, 0x0C, FC_BYTE_LEN, 0 },// MR_VBLK_START[7:0];
{ 0xFD, 0x3C, FC_BYTE_LEN, 0 },// */*/MR_VBLK_WIDTH[5:0];
{ 0xFE, 0x50, FC_BYTE_LEN, 0 },// PIC_FORMAT[3:0]/PINTEST_SEL[3:0];
{ 0xFF, 0x85, FC_BYTE_LEN, 0 },// SLEEP/*/PARALLEL_OUTSW[1:0]/DCLK_POL/DOUT_CBLK_SW/*/AL;
};
#endif
//Div2-SW6-MM-MC-Modify_TCM9001MD_Init_Table-01+}

struct tcm9001md_reg tcm9001md_regs = {
	.inittbl = fc_init_tbl,
	.inittbl_size = ARRAY_SIZE(fc_init_tbl),
};

