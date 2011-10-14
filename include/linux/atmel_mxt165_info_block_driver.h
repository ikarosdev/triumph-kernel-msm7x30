/*============================================================================
Filename : $Workfile: info_block_driver.h $
Project  :
Purpose  : Info block type definition for driver SW. Similar to info_block.h
   in standard firmware files, but with slight differences:
   1) info_id fields are not const (they can't be since they are not
      known at struct init time)
   2) info_block struct is defined (even if the compiler is not IAR),
      but the object table is pointer to array instead of array, since
      the size of that array is not known before the info block is
      read from touch chip.

   This file needs to be kept in sync with firmware info_block.h!

------------------------------------------------------------------------------
Version Number : $Revision: 4 $
Last Updated   : $Date: 2009-06-11 18:23:52+03:00 $
Updated By     : $Author: Iiro Valkonen $
------------------------------------------------------------------------------
Copyright (c) 2009 Quantum Research Group Ltd. All rights reserved.
------------------------------------------------------------------------------
This file contains trade secrets of Quantum Research Group Ltd. No part may be
reproduced or transmitted in any form by any means or for any purpose without
the express written permission of Quantum Research Group Ltd.
------------------------------------------------------------------------------
Revision History
----------------
$Log: /Library Code/Touch Project Framework/Tools/OBP Driver/OBP_driver/src/TOUCH_DRIVER/info_block_driver.h $

 Revision: 4   Date: 2009-06-11 15:23:52Z   User: Iiro Valkonen
 More clean-up, improvements on the message read etc.

 Revision: 3   Date: 2009-06-10 13:53:16Z   User: Iiro Valkonen
 Most of fixes from code review implemented + other modifications.

 Revision: 2   Date: 2009-06-03 13:09:57Z   User: Iiro Valkonen
 File name changes, code clean up etc.

 Revision: 1   Date: 2009-06-03 12:35:06Z   User: Iiro Valkonen

 ------------------------------------------------------------------------------
Known Issues
------------
------------------------------------------------------------------------------
To Do List
----------
============================================================================*/

/*! \file info_block_driver.h
 * \brief Header file defining datatypes & structs needed in driver.
 */

#ifndef INFO_BLOCK_DRIVER_H
#define INFO_BLOCK_DRIVER_H

/*----------------------------------------------------------------------------
  type definitions
----------------------------------------------------------------------------*/

/*! \brief Object table element struct. */
typedef struct _object_t
{
   uint8_t object_type;     /*!< Object type ID. */
   uint16_t i2c_address;    /*!< Start address of the obj config structure. */
   uint8_t size;            /*!< Byte length of the obj config structure -1.*/
   uint8_t instances;       /*!< Number of objects of this obj. type -1. */
   uint8_t num_report_ids;  /*!< The max number of touches in a screen,
                             *  max number of sliders in a slider array, etc.*/
} object_t;

/*! \brief Info ID struct. */
typedef struct _info_id_t
{
   uint8_t family_id;            /* address 0 */
   uint8_t variant_id;           /* address 1 */

   uint8_t version;              /* address 2 */
   uint8_t build;                /* address 3 */

   uint8_t matrix_x_size;        /* address 4 */
   uint8_t matrix_y_size;        /* address 5 */

   /*! Number of entries in the object table. The actual number of objects
    * can be different if any object has more than one instance. */
   uint8_t num_declared_objects; /* address 6 */
} info_id_t;

/*! \brief Info block struct holding ID and object table data and their CRC sum.
 *
 * Info block struct. Similar to one in info_block.h, but since
 * the size of object table is not known beforehand, it's pointer to an
 * array instead of an array. This is not defined in info_block.h unless
 * we are compiling with IAR AVR or AVR32 compiler (__ICCAVR__ or __ICCAVR32__
 * is defined). If this driver is compiled with those compilers, the
 * info_block.h needs to be edited to not include that struct definition.
 *
 * CRC is 24 bits, consisting of CRC and CRC_hi; CRC is the lower 16 bytes and
 * CRC_hi the upper 8.
 *
 */
typedef struct _info_block_t
{
    /*! Info ID struct. */
    info_id_t *info_id;

    /*! Pointer to an array of objects. */
    object_t *objects;

    /*! 24-bit CRC.  */
    uint32_t CRC;
} info_block_t;

#endif  /* #ifndef INFO_BLOCK_H */

