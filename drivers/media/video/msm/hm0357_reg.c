/*
 *     hm0357_reg.c - Camera Sensor Config
 *
 *     Copyright (C) 2010 Kent Kwan <kentkwan@fihspec.com>
 *     Copyright (C) 2008 FIH CO., Inc.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; version 2 of the License.
 */

#include "hm0357.h"

struct hm0357_i2c_reg_conf const hm0357_init_tbl[] =
{
	//initial settings!++
	{0x0022, 0x0000, BYTE_LEN, 0},// Reset                                                                                               
	{0x0020, 0x0008, BYTE_LEN, 0},// Reset         //20100901	W 30 08->0x68	//­×§ï¤~¯à«G                                                   
	{0x0023, 0x00FF, BYTE_LEN, 0},// 30 0023 5A 2 1;W 30 0023 7F 2 1	
	{0x008F, 0x0000, BYTE_LEN, 0},// 30 008F FF,	                                                                                         
	{0x0004, 0x0011, BYTE_LEN, 0},//
	{0x0006, 0x0002, BYTE_LEN, 0},//
	{0x000F, 0x0000, BYTE_LEN, 0},//                                                                                                    
	{0x0012, 0x000C, BYTE_LEN, 0},//                                                                                                    
	{0x0013, 0x0001, BYTE_LEN, 0},//                                                                                                    
	{0x0015, 0x0002, BYTE_LEN, 0},//                                                                                                    
	{0x0016, 0x0001, BYTE_LEN, 0},//                                                                                                    
	{0x0025, 0x0000, BYTE_LEN, 0},//ISP=24M ,0x0000.
	{0x0027, 0x0030, BYTE_LEN, 0},//YCbYCr                                                                                              
	{0x0040, 0x000F, BYTE_LEN, 0},//BLC Ph1                                                                                             
	{0x0053, 0x000F, BYTE_LEN, 0},//BLC Ph2                                                                                             
	{0x0075, 0x0040, BYTE_LEN, 0},//Auto Negative Pump                                                                                  
	{0x0041, 0x0002, BYTE_LEN, 0},//                                                                                                    
	{0x0045, 0x00CA, BYTE_LEN, 0},//                                                                                                    
	{0x0046, 0x004F, BYTE_LEN, 0},//                                                                                                    
	{0x004A, 0x000A, BYTE_LEN, 0},//                                                                                                    
	{0x004B, 0x0072, BYTE_LEN, 0},//                                                                                                    
	{0x004D, 0x00BF, BYTE_LEN, 0},//                                                                                                    
	{0x004E, 0x0030, BYTE_LEN, 0},//                                                                                         
	{0x0055, 0x0010, BYTE_LEN, 0},//                                                                                                    
	{0x0053, 0x0000, BYTE_LEN, 0},//                                                                                                    
	{0x0070, 0x0044, BYTE_LEN, 0},//                                                                                                    
	{0x0071, 0x00B9, BYTE_LEN, 0},//                                                                                                    
	{0x0072, 0x0055, BYTE_LEN, 0},//                                                                                                    
	{0x0073, 0x0010, BYTE_LEN, 0},//                                                                                                    
	{0x0081, 0x00D2, BYTE_LEN, 0},//                                                                                                    
	{0x0082, 0x00A6, BYTE_LEN, 0},//                                                                                                    
	{0x0083, 0x0070, BYTE_LEN, 0},//                                                                                                    
	{0x0085, 0x0011, BYTE_LEN, 0},//                                                                                                    
	{0x0088, 0x00E1, BYTE_LEN, 0},//                                                                                                    
	{0x008A, 0x002D, BYTE_LEN, 0},//                                                                                                    
	{0x008D, 0x0020, BYTE_LEN, 0},//                                                                                                    
	{0x0090, 0x0000, BYTE_LEN, 0},//                                                                                                    
	{0x0091, 0x0010, BYTE_LEN, 0},//                                                                                                    
	{0x0092, 0x0011, BYTE_LEN, 0},//                                                                                                    
	{0x0093, 0x0012, BYTE_LEN, 0},//                                                                                                    
	{0x0094, 0x0013, BYTE_LEN, 0},//                                                                                                    
	{0x0095, 0x0017, BYTE_LEN, 0},//                                                                                                    
	{0x00A0, 0x00C0, BYTE_LEN, 0},//                                                                                                    
	{0x011F, 0x0044, BYTE_LEN, 0},//                                                                                                    
	{0x0121, 0x0080, BYTE_LEN, 0},//                                                                                                    
	{0x0122, 0x006B, BYTE_LEN, 0},//SIM_BPC,LSC_ON,A6_ON                                                                                
	{0x0123, 0x00A5, BYTE_LEN, 0},//                                                                                                    
	{0x0124, 0x0052, BYTE_LEN, 0},//CT
	{0x0125, 0x00DF, BYTE_LEN, 0},//Windowing
	{0x0126, 0x0071, BYTE_LEN, 0},//70=EV_SEL 3x5,60=EV_SEL 3x3                                                                         
	{0x0140, 0x0014, BYTE_LEN, 0},//BPC                                                                                                 
	{0x0141, 0x000A, BYTE_LEN, 0},//                                                                                                    
	{0x0142, 0x0014, BYTE_LEN, 0},//                                                                                                    
	{0x0143, 0x000A, BYTE_LEN, 0},//                                                                                                    
	{0x0144, 0x000C, BYTE_LEN, 0},//                                                                                                
	{0x0145, 0x000C, BYTE_LEN, 0},// 
	{0x0146, 0x0065, BYTE_LEN, 0},//New BPC
	{0x0147, 0x000A, BYTE_LEN, 0},//                                                                                                   
	{0x0148, 0x0080, BYTE_LEN, 0},//
	{0x0149, 0x000C, BYTE_LEN, 0},//                                                                                                   
	{0x014A, 0x0080, BYTE_LEN, 0},//                                                                                                   
	{0x014B, 0x0080, BYTE_LEN, 0},//                                                                                                   
	{0x014C, 0x0080, BYTE_LEN, 0},//                                                                                                   
	{0x014D, 0x002E, BYTE_LEN, 0},//                                                                                                   
	{0x014E, 0x0005, BYTE_LEN, 0},//                                                                                                   
	{0x014F, 0x0005, BYTE_LEN, 0},//                                                                                                   
	{0x0150, 0x000D, BYTE_LEN, 0},//                                                                                                   
	{0x0155, 0x0000, BYTE_LEN, 0},//                                                                                                   
	{0x0156, 0x000A, BYTE_LEN, 0},//                                                                                                   
	{0x0157, 0x000A, BYTE_LEN, 0},//                                                                                                   
	{0x0158, 0x000A, BYTE_LEN, 0},//                                                                                                   
	{0x0159, 0x000A, BYTE_LEN, 0},//                                                                                                   
	{0x0160, 0x0014, BYTE_LEN, 0},//SIM_BPC_th                                                                                          
	{0x0161, 0x0014, BYTE_LEN, 0},//SIM_BPC_th Alpha                                                                                    
	{0x0162, 0x000A, BYTE_LEN, 0},//SIM_BPC_Directional                                                                                 
	{0x0163, 0x000A, BYTE_LEN, 0},//SIM_BPC_Directional Alpha                                                                           
	{0x01B0, 0x0033, BYTE_LEN, 0},//G1G2 Balance                                                                                        
	{0x01B1, 0x0010, BYTE_LEN, 0},//                                                                                                    
	{0x01B2, 0x0010, BYTE_LEN, 0},//                                                                                                    
	{0x01B3, 0x0030, BYTE_LEN, 0},//                                                                                                    
	{0x01B4, 0x0018, BYTE_LEN, 0},//                                                                                                    
	{0x01D8, 0x0020, BYTE_LEN, 0},//Bayer Denoise
	{0x01DE, 0x0050, BYTE_LEN, 0},//
	{0x01E4, 0x000A, BYTE_LEN, 0},//
	{0x01E5, 0x0010, BYTE_LEN, 0},//
	{0x0220, 0x0000, BYTE_LEN, 0},//LSC                                                                                                  
	{0x0221, 0x00B2, BYTE_LEN, 0},//
	{0x0222, 0x0000, BYTE_LEN, 0},//                                                                                                    
	{0x0223, 0x0080, BYTE_LEN, 0},//                                                                                                    
	{0x0224, 0x0080, BYTE_LEN, 0},//
	{0x0225, 0x0000, BYTE_LEN, 0},//                                                                                                    
	{0x0226, 0x0090, BYTE_LEN, 0},//
	{0x0227, 0x0080, BYTE_LEN, 0},//
	{0x0229, 0x0090, BYTE_LEN, 0},//
	{0x022A, 0x0080, BYTE_LEN, 0},//                                                                                                       
	{0x022B, 0x0000, BYTE_LEN, 0},//                                                                                                       
	{0x022C, 0x0096, BYTE_LEN, 0},//
	{0x022D, 0x0011, BYTE_LEN, 0},//                                                                                                       
	{0x022E, 0x0010, BYTE_LEN, 0},//                                                                                                       
	{0x022F, 0x0011, BYTE_LEN, 0},//                                                                                                       
	{0x0230, 0x0010, BYTE_LEN, 0},//                                                                                                       
	{0x0231, 0x0011, BYTE_LEN, 0},//                                                                                                       
	{0x0233, 0x0011, BYTE_LEN, 0},//                                                                                                       
	{0x0234, 0x0010, BYTE_LEN, 0},//                                                                                                       
	{0x0235, 0x0046, BYTE_LEN, 0},//                                                                                                       
	{0x0236, 0x0001, BYTE_LEN, 0},//                                                                                                       
	{0x0237, 0x0046, BYTE_LEN, 0},//                                                                                                       
	{0x0238, 0x0001, BYTE_LEN, 0},//                                                                                                       
	{0x023B, 0x0046, BYTE_LEN, 0},//                                                                                                       
	{0x023C, 0x0001, BYTE_LEN, 0},//                                                                                                       
	{0x023D, 0x00F8, BYTE_LEN, 0},//                                                                                                       
	{0x023E, 0x0000, BYTE_LEN, 0},//                                                                                                       
	{0x023F, 0x00F8, BYTE_LEN, 0},//                                                                                                       
	{0x0240, 0x0000, BYTE_LEN, 0},//                                                                                                       
	{0x0243, 0x00F8, BYTE_LEN, 0},//                                                                                                       
	{0x0244, 0x0000, BYTE_LEN, 0},//                                                                                                       
	{0x0250, 0x0003, BYTE_LEN, 0},//                                                                                                       
	{0x0251, 0x000D, BYTE_LEN, 0},//                                                                                                       
	{0x0252, 0x000A, BYTE_LEN, 0},//
	{0x0280, 0x000B, BYTE_LEN, 0},//Gamma                                                                                                
	{0x0282, 0x0015, BYTE_LEN, 0},//                                                                                                       
	{0x0284, 0x002A, BYTE_LEN, 0},//                                                                                                       
	{0x0286, 0x004C, BYTE_LEN, 0},//                                                                                                       
	{0x0288, 0x005A, BYTE_LEN, 0},//                                                                                                       
	{0x028A, 0x0067, BYTE_LEN, 0},//                                                                                                       
	{0x028C, 0x0073, BYTE_LEN, 0},//                                                                                                       
	{0x028E, 0x007D, BYTE_LEN, 0},//                                                                                                       
	{0x0290, 0x0086, BYTE_LEN, 0},//                                                                                                       
	{0x0292, 0x008E, BYTE_LEN, 0},//                                                                                                       
	{0x0294, 0x009E, BYTE_LEN, 0},//                                                                                                       
	{0x0296, 0x00AC, BYTE_LEN, 0},//                                                                                                       
	{0x0298, 0x00C0, BYTE_LEN, 0},//                                                                                                       
	{0x029A, 0x00D2, BYTE_LEN, 0},//                                                                                                       
	{0x029C, 0x00E2, BYTE_LEN, 0},//                                                                                                       
	{0x029E, 0x0027, BYTE_LEN, 0},//                                                                        
	{0x02A0, 0x0006, BYTE_LEN, 0},//
	{0x02C0, 0x00CC, BYTE_LEN, 0},//D CCM                                                                                                        
	{0x02C1, 0x0001, BYTE_LEN, 0},//                                                                                                       
	{0x02C2, 0x00BB, BYTE_LEN, 0},//                                                                                                       
	{0x02C3, 0x0004, BYTE_LEN, 0},//                                                                                                       
	{0x02C4, 0x0011, BYTE_LEN, 0},//                                                                                                       
	{0x02C5, 0x0004,  BYTE_LEN, 0},//                                                                                                       
	{0x02C6, 0x002E, BYTE_LEN, 0},//                                                                                                       
	{0x02C7, 0x0004, BYTE_LEN, 0},//                                                                                                       
	{0x02C8, 0x006C, BYTE_LEN, 0},//                                                                                                       
	{0x02C9, 0x0001, BYTE_LEN, 0},//                                                                                                       
	{0x02CA, 0x003E, BYTE_LEN, 0},//                                                                                                       
	{0x02CB, 0x0004, BYTE_LEN, 0},//                                                                                                       
	{0x02CC, 0x0004, BYTE_LEN, 0},//                                                                                                       
	{0x02CD, 0x0004, BYTE_LEN, 0},//                                                                                                       
	{0x02CE, 0x00BE, BYTE_LEN, 0},//                                                                                                       
	{0x02CF, 0x0004, BYTE_LEN, 0},//                                                                                                       
	{0x02E0, 0x0004, BYTE_LEN, 0},//CCM bu Alpha
	{0x02D0, 0x00C2, BYTE_LEN, 0},//                                                                                                       
	{0x02D1, 0x0001, BYTE_LEN, 0},//                                                                                                     
	{0x02F0, 0x0074, BYTE_LEN, 0},//CCM DIFF
	{0x02F1, 0x0004, BYTE_LEN, 0},//                                                                                                       
	{0x02F2, 0x00E3, BYTE_LEN, 0},//
	{0x02F3, 0x0000, BYTE_LEN, 0},//                                                                                                       
	{0x02F4, 0x006F, BYTE_LEN, 0},//
	{0x02F5, 0x0004, BYTE_LEN, 0},//                                                                                                       
	{0x02F6, 0x002F, BYTE_LEN, 0},//
	{0x02F7, 0x0004, BYTE_LEN, 0},//                                                                                                       
	{0x02F8, 0x0042, BYTE_LEN, 0},//
	{0x02F9, 0x0004, BYTE_LEN, 0},//                                                                                                       
	{0x02FA, 0x0071, BYTE_LEN, 0},//
	{0x02FB, 0x0000, BYTE_LEN, 0},//                                                                                                       
	{0x02FC, 0x0038, BYTE_LEN, 0},//
	{0x02FD, 0x0004, BYTE_LEN, 0},//                                                                                                       
	{0x02FE, 0x0078, BYTE_LEN, 0},//
	{0x02FF, 0x0004, BYTE_LEN, 0},//                                                                                                       
	{0x0300, 0x00B0, BYTE_LEN, 0},//
	{0x0301, 0x0000, BYTE_LEN, 0},//                                                                                               
	{0x0328, 0x0000, BYTE_LEN, 0},//Win Pick P Min                                                                                      
	{0x0329, 0x0004, BYTE_LEN, 0},//                                                                                                    
	{0x032D, 0x0066, BYTE_LEN, 0},//Initial Channel Gain                                                                                
	{0x032E, 0x0001, BYTE_LEN, 0},//                                                                                                    
	{0x032F, 0x0000, BYTE_LEN, 0},//                                                                                                    
	{0x0330, 0x0001, BYTE_LEN, 0},//                                                                                                    
	{0x0331, 0x0066, BYTE_LEN, 0},//                                                                                                    
	{0x0332, 0x0001, BYTE_LEN, 0},//                                                                                                    
	{0x0333, 0x0000, BYTE_LEN, 0},//AWB channel offset                                                                                  
	{0x0334, 0x0000, BYTE_LEN, 0},//                                                                                                       
	{0x0335, 0x0000, BYTE_LEN, 0},//                                                                                                       
	{0x033E, 0x0000, BYTE_LEN, 0},//                                                                                                       
	{0x033F, 0x0000, BYTE_LEN, 0},//                                                                                           
	{0x0340, 0x0020, BYTE_LEN, 0},//AWB
	{0x0341, 0x0044, BYTE_LEN, 0},// 
	{0x0342, 0x0049, BYTE_LEN, 0},// 
	{0x0343, 0x0030, BYTE_LEN, 0},// 
	{0x0344, 0x008D, BYTE_LEN, 0},// 
	{0x0345, 0x0040, BYTE_LEN, 0},// 
	{0x0346, 0x006C, BYTE_LEN, 0},// 
	{0x0347, 0x004D, BYTE_LEN, 0},// 
	{0x0348, 0x0068, BYTE_LEN, 0},// 
	{0x0349, 0x006B, BYTE_LEN, 0},//
	{0x034A, 0x005A, BYTE_LEN, 0},// 
	{0x034B, 0x0076, BYTE_LEN, 0},// 
	{0x034C, 0x004B, BYTE_LEN, 0},// 
	{0x0350, 0x0090, BYTE_LEN, 0},//                                                                                                        
	{0x0351, 0x0090, BYTE_LEN, 0},//                                                                                                        
	{0x0352, 0x0018, BYTE_LEN, 0},//                                                                                                        
	{0x0353, 0x0018, BYTE_LEN, 0},//                                                                                                        
	{0x0354, 0x0080, BYTE_LEN, 0},//R Max
	{0x0355, 0x0045, BYTE_LEN, 0},//G Max                                                                                               
	{0x0356, 0x0090, BYTE_LEN, 0},//B Max
	{0x0357, 0x00D0, BYTE_LEN, 0},//R+B Max                                                                                             
	{0x0358, 0x0005, BYTE_LEN, 0},//R comp Max                                                                                          
	{0x035A, 0x0005, BYTE_LEN, 0},//                                                                                                    
	{0x035B, 0x00A0, BYTE_LEN, 0},//R+B Min                                                                                             
	{0x0381, 0x004C, BYTE_LEN, 0},//
	{0x0382, 0x0034, BYTE_LEN, 0},//
	{0x0383, 0x0020, BYTE_LEN, 0},//                                                                                                     
	{0x038A, 0x0080, BYTE_LEN, 0},//                                                                                                     
	{0x038B, 0x0010, BYTE_LEN, 0},//                                                                                                     
	{0x038C, 0x00C1, BYTE_LEN, 0},//
	{0x038E, 0x0044, BYTE_LEN, 0},//
	{0x038F, 0x0003, BYTE_LEN, 0},//AE Max exposure
	{0x0390, 0x00E0, BYTE_LEN, 0},//AE Max exposure
	{0x0391, 0x0008, BYTE_LEN, 0},//AE Min exposure for Garmin-asus LV16
	{0x0393, 0x0080, BYTE_LEN, 0},//                                                                                                     
	{0x0395, 0x0012, BYTE_LEN, 0},//                                                                                                     
	{0x0398, 0x0002, BYTE_LEN, 0},//Frame rate control                                                                                  
	{0x0399, 0x00E9, BYTE_LEN, 0},//	                                                                                                    
	{0x039A, 0x0004, BYTE_LEN, 0},//	                                                                                                    
	{0x039B, 0x0013, BYTE_LEN, 0},//	                                                                                                    
	{0x039C, 0x0004, BYTE_LEN, 0},//	                                                                                                    
	{0x039D, 0x00A8, BYTE_LEN, 0},//	                                                                                                    
	{0x039E, 0x0005, BYTE_LEN, 0},//	                                                                                                    
	{0x039F, 0x00D2, BYTE_LEN, 0},//	                                                                                                    
	{0x03A0, 0x0007, BYTE_LEN, 0},//	                                                                                                    
	{0x03A1, 0x0044, BYTE_LEN, 0},//	                                                                                                    
	{0x03A6, 0x001A, BYTE_LEN, 0},//	                                                                                                    
	{0x03A7, 0x0020, BYTE_LEN, 0},//	                                                                                                    
	{0x03A8, 0x0036, BYTE_LEN, 0},//	                                                                                                    
	{0x03A9, 0x0040, BYTE_LEN, 0},//	                                                                                                    
	{0x03AE, 0x0030, BYTE_LEN, 0},//	                                                                                                    
	{0x03AF, 0x0028, BYTE_LEN, 0},//	                                                                                                    
	{0x03B0, 0x001A, BYTE_LEN, 0},//	                                                                                                    
	{0x03B1, 0x0010, BYTE_LEN, 0},//	  
	{0x03BB, 0x005F, BYTE_LEN, 0},//AE Winding
	{0x03BC, 0x00FF, BYTE_LEN, 0},//AE Winding
	{0x03BD, 0x0000, BYTE_LEN, 0},//AE Winding
	{0x03BE, 0x0000, BYTE_LEN, 0},//AE Winding                                                                                          
	{0x03BF, 0x001D, BYTE_LEN, 0},//                                                                                                    
	{0x03C0, 0x002E, BYTE_LEN, 0},//                                                                                                    
	{0x03C3, 0x000F, BYTE_LEN, 0},//                                                                                                    
	{0x03D0, 0x00E0, BYTE_LEN, 0},//                                                                                                    
	{0x0420, 0x0083, BYTE_LEN, 0},//BLC Offset
	{0x0421, 0x0000, BYTE_LEN, 0},//                                                                                                        
	{0x0422, 0x0000, BYTE_LEN, 0},//                                                                                                        
	{0x0423, 0x0084, BYTE_LEN, 0},//
	{0x0430, 0x0000, BYTE_LEN, 0},//ABLC                                                                                                
	{0x0431, 0x0050, BYTE_LEN, 0},//Max
	{0x0432, 0x0030, BYTE_LEN, 0},//ABLC                                                                                                 
	{0x0433, 0x0030, BYTE_LEN, 0},//                                                                                                        
	{0x0434, 0x0000, BYTE_LEN, 0},//                                                                                                        
	{0x0435, 0x0040, BYTE_LEN, 0},//                                                                                                        
	{0x0436, 0x0000, BYTE_LEN, 0},//                                                                                                        
	{0x0450, 0x00FF, BYTE_LEN, 0},//                                                                                                        
	{0x0451, 0x00FF, BYTE_LEN, 0},//                                                                                                        
	{0x0452, 0x00A0, BYTE_LEN, 0},//
	{0x0453, 0x0070, BYTE_LEN, 0},//                                                                                                        
	{0x0454, 0x0000, BYTE_LEN, 0},//                                                                                                        
	{0x045A, 0x0000, BYTE_LEN, 0},//                                                                                                        
	{0x045B, 0x0040, BYTE_LEN, 0},//CT
	{0x045C, 0x0001, BYTE_LEN, 0},//CT
	{0x045D, 0x0000, BYTE_LEN, 0},//CT
	{0x0465, 0x0002, BYTE_LEN, 0},//                                                                                                        
	{0x0466, 0x0004, BYTE_LEN, 0},//                                                                                                        
	{0x047A, 0x0000, BYTE_LEN, 0},//ODEL Rayer denoise
	{0x047B, 0x0000, BYTE_LEN, 0},//ODEL Y denoise
	{0x0480, 0x0046, BYTE_LEN, 0},//Sat                                                                                                  
	{0x0481, 0x0006, BYTE_LEN, 0},//Sat by Alpha                                                                                         
	{0x04B0, 0x0040, BYTE_LEN, 0},//Contrast                                                                                             
	{0x04B1, 0x0000, BYTE_LEN, 0},//Contrast                                                                                             
	{0x04B3, 0x007F, BYTE_LEN, 0},//Contrast                                                                                             
	{0x04B4, 0x0000, BYTE_LEN, 0},//Contrast                                                                                             
	{0x04B6, 0x0020, BYTE_LEN, 0},//Contrast                                                                                             
	{0x04B9, 0x0040, BYTE_LEN, 0},//Contrast                                                                                             
	{0x0540, 0x0000, BYTE_LEN, 0},//60Hz Flicker step                                                                                    
	{0x0541, 0x007C, BYTE_LEN, 0},//60Hz Flicker step                                                                                    
	{0x0542, 0x0000, BYTE_LEN, 0},//50Hz Flicker step                                                                                    
	{0x0543, 0x0095, BYTE_LEN, 0},//50Hz Flicker step                                                                                    
	{0x0580, 0x0001, BYTE_LEN, 0},// Y Blurring                                                                                          
	{0x0581, 0x0004, BYTE_LEN, 0},// Y Blurring Alpha                                                                                    
	{0x0590, 0x0020, BYTE_LEN, 0},// UV denoise                                                                                          
	{0x0591, 0x0030, BYTE_LEN, 0},// UV denoise Alpha                                                                                    
	{0x0594, 0x0010, BYTE_LEN, 0},//UV Gray TH
	{0x0595, 0x0020, BYTE_LEN, 0},//UV Gray TH Alpha
	{0x05A0, 0x000A, BYTE_LEN, 0},//Sharpness Curve Edge THL 
	{0x05A1, 0x000C, BYTE_LEN, 0},//Edge THL Alpha                                                                                      
	{0x05A2, 0x0050, BYTE_LEN, 0},//Edge THH                                                                                            
	{0x05A3, 0x0060, BYTE_LEN, 0},//Edge THH Alpha                                                                                      
	{0x05B0, 0x0028, BYTE_LEN, 0},//Edge strength
	{0x05B1, 0x0006, BYTE_LEN, 0},//
	{0x05D0, 0x0008, BYTE_LEN, 0},//F2B Start Mean
	{0x05D1, 0x0007, BYTE_LEN, 0},//F2B                                                                                                                                                                              
	{0x05E4, 0x0004, BYTE_LEN, 0},//Windowing                                                                                           
	{0x05E5, 0x0000, BYTE_LEN, 0},//                                                                                                     
	{0x05E6, 0x0083, BYTE_LEN, 0},//                                                                                                     
	{0x05E7, 0x0002, BYTE_LEN, 0},//                                                                                                     
	{0x05E8, 0x0004, BYTE_LEN, 0},//                                                                                                     
	{0x05E9, 0x0000, BYTE_LEN, 0},//                                                                                                     
	{0x05EA, 0x00E3, BYTE_LEN, 0},//                                                                                                     
	{0x05EB, 0x0001, BYTE_LEN, 0},//

	//{0x0028, 0x0084, BYTE_LEN, 0},	//test pattern 
	//Pink Issue	20101115 by Harris
	{0x0090, 0x0000, BYTE_LEN, 0},//Analog Table
	{0x0091, 0x0001, BYTE_LEN, 0},//
	{0x0092, 0x0002, BYTE_LEN, 0},//
	{0x0093, 0x0003, BYTE_LEN, 0},//
	{0x0094, 0x0007, BYTE_LEN, 0},//
	{0x0095, 0x0008, BYTE_LEN, 0},//
	{0x00A0, 0x00C4, BYTE_LEN, 0},//
	{0x01E5, 0x000A, BYTE_LEN, 0},//Bayer De-Noise A0 Strength
	{0x03A6, 0x000D, BYTE_LEN, 0},//AE Frame Rate Control
	{0x03A7, 0x000A, BYTE_LEN, 0},//
	{0x03A8, 0x0023, BYTE_LEN, 0},//
	{0x03A9, 0x0033, BYTE_LEN, 0},//
	{0x03AE, 0x0020, BYTE_LEN, 0},//
	{0x03AF, 0x0017, BYTE_LEN, 0},//
	{0x03B0, 0x0007, BYTE_LEN, 0},//
	{0x03B1, 0x0008, BYTE_LEN, 0},//
	{0x0000, 0x0001, BYTE_LEN, 0},//                                                                                                    
	{0x0100, 0x0001, BYTE_LEN, 0},//                                                                                                    
	{0x0101, 0x0001, BYTE_LEN, 0},//                                                                                                    
	{0x0005, 0x0001, BYTE_LEN, 0},//Turn on rolling shutter	
};

struct hm0357_reg hm0357_regs = {
	.inittbl = hm0357_init_tbl,
	.inittbl_size = ARRAY_SIZE(hm0357_init_tbl),
};
