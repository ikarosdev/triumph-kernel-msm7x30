/*============================================================================
Filename : $Workfile: touch_driver.h $
Project  : OBP Driver
Purpose  : C driver for OBP based touch chips.
------------------------------------------------------------------------------
Version Number : $Revision: 7 $
Last Updated   : $Date: 2009-06-12 18:13:54+03:00 $
Updated By     : $Author: Iiro Valkonen $
------------------------------------------------------------------------------
Copyright (c) 2009 QRG Ltd. All rights reserved.
------------------------------------------------------------------------------
This file contains trade secrets of QRG Ltd. No part may be reproduced or
transmitted in any form by any means or for any purpose without the express
written permission of QRG Ltd.
------------------------------------------------------------------------------
Revision History
----------------
$Log: /Library Code/Touch Project Framework/Tools/OBP Driver/OBP_driver/src/TOUCH_DRIVER/touch_driver.h $

 Revision: 7   Date: 2009-06-12 15:13:54Z   User: Iiro Valkonen
 V0.2 with fixed address pointer update.

 Revision: 6   Date: 2009-06-12 14:48:07Z   User: Iiro Valkonen
 Version 0.2.

 Revision: 5   Date: 2009-06-11 15:23:52Z   User: Iiro Valkonen
 More clean-up, improvements on the message read etc.

 Revision: 4   Date: 2009-06-10 13:53:16Z   User: Iiro Valkonen
 Most of fixes from code review implemented + other modifications.

 Revision: 3   Date: 2009-06-03 14:49:54Z   User: Iiro Valkonen
 CTE object support added.


------------------------------------------------------------------------------
Known Issues
------------
------------------------------------------------------------------------------
To Do List
----------
============================================================================*/

/*! \file touch_driver.h
 * \brief Header file for Atmel OBP touch driver.
 */

#ifndef GENERAL_H_
#define GENERAL_H_

#include "qt602240_info_block_driver.h"
#include "qt602240_std_objects_driver.h"

/*----------------------------------------------------------------------------
  Constant definitions.
----------------------------------------------------------------------------*/

/*! \brief Touch driver version number. */
#define TOUCH_DRIVER_VERSION	0x02u

#define I2C_ADDR_0         0x10u
#define I2C_ADDR_1         0x20u
#define I2C_ADDR_2         0x30u
#define I2C_ADDR_3         0x40u
#define I2C_ADDR_4         0x11u
#define I2C_ADDR_5         0x54u
#define I2C_ADDR_6         0x4Au
#define I2C_ADDR_7         0x4Bu

#define NUM_OF_I2C_ADDR    8

/* \brief Defines CHANGE line active mode. */
#define CHANGELINE_ASSERTED 0


/* This sets the I2C frequency to 400kHz (it's a feature in I2C driver that the
 * actual value needs to be double that). */
/*! \brief I2C bus speed. */
#define I2C_SPEED                   800000u

#define CONNECT_OK                  1u
#define CONNECT_ERROR               2u

#define READ_MEM_OK                 0//1u
#define READ_MEM_FAILED             1//2u

#define MESSAGE_READ_OK             1u
#define MESSAGE_READ_FAILED         2u

#define WRITE_MEM_OK                1u
#define WRITE_MEM_FAILED            2u

#define CFG_WRITE_OK                1u
#define CFG_WRITE_FAILED            2u

#define I2C_INIT_OK                 1u
#define I2C_INIT_FAIL               2u

#define CRC_CALCULATION_OK          1u
#define CRC_CALCULATION_FAILED      2u

#define ID_MAPPING_OK               1u
#define ID_MAPPING_FAILED           2u

#define ID_DATA_OK                  1u
#define ID_DATA_NOT_AVAILABLE       2u

enum driver_setup_t {DRIVER_SETUP_OK, DRIVER_SETUP_INCOMPLETE};

/*! \brief Returned by get_object_address() if object is not found. */
#define OBJECT_NOT_FOUND   0u

/* Array of I2C addresses where we are trying to find the chip. */
extern uint8_t i2c_addresses[];

/*! Address where object table starts at touch IC memory map. */
#define OBJECT_TABLE_START_ADDRESS      7U

/*! Size of one object table element in touch IC memory map. */
#define OBJECT_TABLE_ELEMENT_SIZE     	6U

/*! Offset to RESET register from the beginning of command processor. */
#define RESET_OFFSET		            0u

/*! Offset to BACKUP register from the beginning of command processor. */
#define BACKUP_OFFSET		1u

/*! Offset to CALIBRATE register from the beginning of command processor. */
#define CALIBRATE_OFFSET	2u

/*! Offset to REPORTALL register from the beginning of command processor. */
#define REPORTATLL_OFFSET	3u

/*! Offset to DEBUG_CTRL register from the beginning of command processor. */
#define DEBUG_CTRL_OFFSET	4u



/*----------------------------------------------------------------------------
  Function prototypes.
----------------------------------------------------------------------------*/


/* Initializes the touch driver: tries to connect to given address,
 * sets the message handler pointer, reads the info block and object
 * table, sets the message processor address etc. */

// uint8_t init_touch_driver(uint8_t I2C_address, void (*handler)());

/* Closes the touch driver. */
// uint8_t close_touch_driver();

/* Reads a message from the message processor, calls application
 * provided handler routine. */
uint8_t get_message(void);

/* Resets the chip. */
uint8_t reset_chip(void);

/* Calibrates the chip. */
uint8_t calibrate_chip(void);

/* Backups the configuration settings to non-volatile memory. */
uint8_t backup_config(void);

/* Gets the touch chip variant id. */
uint8_t get_variant_id(uint8_t *variant);

/* Gets the touch chip family id. */
uint8_t get_family_id(uint8_t *family_id);

/* Gets the touch build number. */
uint8_t get_build_number(uint8_t *build);

/* Gets the touch chip version number. */
uint8_t get_version(uint8_t *version);

/* Writes power config. */
uint8_t write_power_config(gen_powerconfig_t7_config_t power_config);

/* Writes acquisition config. */
uint8_t write_acquisition_config(gen_acquisitionconfig_t8_config_t acq_config);

/* Writes multitouchscreen config. */
uint8_t write_multitouchscreen_config(uint8_t screen_number,
                                      touch_multitouchscreen_t9_config_t cfg);

/* Writes key array config. */
uint8_t write_keyarray_config(uint8_t key_array_number,
                              touch_keyarray_t15_config_t cfg);

/* Writes linearization config. */
/*
uint8_t write_linearization_config(uint8_t instance,
                                   proci_linearizationtable_t17_config_t cfg);
*/

/* Writes one touch gesture config. */
uint8_t write_onetouchgesture_config(uint8_t instance,
   proci_onetouchgestureprocessor_t24_config_t cfg);

/* Writes two touch gesture config. */
uint8_t write_twotouchgesture_config(uint8_t instance,
   proci_twotouchgestureprocessor_t27_config_t cfg);

/* Writes grip suppression config. */
uint8_t write_gripsuppression_config(uint8_t instance,
   proci_gripfacesuppression_t20_config_t cfg);

/* Writes CTE config. */
uint8_t write_CTE_config(spt_cteconfig_t28_config_t cfg);

/* Maps object type/instance to report id. */
uint8_t type_to_report_id(uint8_t object_type, uint8_t instance);

/* Maps report id to object type/instance. */
uint8_t report_id_to_type(uint8_t report_id, uint8_t *instance);

/* Returns the maximum report id in use on this touch chip. */
// uint8_t get_max_report_id();

/* Returns object address in memory map. */
uint16_t get_object_address(uint8_t object_type, uint8_t instance);

/* Calculates the infoblock CRC. */
uint8_t calculate_infoblock_crc(uint32_t *crc_pointer);

/* Returns the stored infoblock CRC from touch chip. */
uint32_t get_stored_infoblock_crc(void);


/*
 * Prototypes for functions that are platform specific and need to be defined
 * in the platform specific files.
 */

/* Initializes the pin monitoring the changeline. */
// void init_changeline();

/* Initializes the I2C interface. */
uint8_t init_I2C(uint8_t);

/*----------------------------------------------------------------------------
  Type definitions.
----------------------------------------------------------------------------*/




/*! \brief Struct holding the object type / instance info.
 *
 * Struct holding the object type / instance info. An array of these maps
 * report id's to object type / instance (array index = report id).  Note
 * that the report ID number 0 is reserved.
 *
 */


typedef struct
{
   uint8_t object_type; 	/*!< Object type. */
   uint8_t instance;   	    /*!< Instance number. */
} report_id_map_t;

/*----------------------------------------------------------------------------
  Global variables.
----------------------------------------------------------------------------*/

/*! \brief Flag indicating whether there's a message read pending.
 *
 * When the CHANGE line is asserted during the message read, the state change
 * is possibly missed by the host monitoring the CHANGE line. The host
 * should check this flag after a read operation, and if set, do a read.
 *
 */

volatile uint8_t read_pending;

/*! Message processor address. */
extern uint16_t message_processor_address;

#endif /* GENERAL_H_ */
