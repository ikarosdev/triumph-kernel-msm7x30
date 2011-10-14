#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <mach/gpio.h>
#include <mach/vreg.h>

#include <linux/atmel_mxt165_info_block_driver.h>
#include <linux/atmel_mxt165_std_objects_driver.h>
#include <linux/atmel_mxt165_touch_driver.h>
#include <linux/atmel_mxt165.h>

info_block_t *info_block;
static uint16_t g_I2CAddress;
static uint16_t g_RegAddr[DEFAULT_REG_NUM];
static uint8_t g_MsgData[8];
static uint8_t g_ReportID[256];

static int mxt_debug_level;
module_param_named(
    debug_level, mxt_debug_level, int, S_IRUGO | S_IWUSR | S_IWGRP
);

static int cap_touch_fw_version;
module_param_named(
    fw_version, cap_touch_fw_version, int, S_IRUGO | S_IWUSR | S_IWGRP
);

// Return 1: Face Down.
// Return 0: Face Up.
// Return -1: Failed.
int MXT_FaceDetection(void)
{
    if (atmel_mxt165 == NULL)
    {
        TCH_ERR("atmel_mxt165 is a null pointer.");
        return -1;
    }
    else
    {
        TCH_DBG(DB_LEVEL1, "facedetect = %d", atmel_mxt165->facedetect);
        return atmel_mxt165->facedetect;
    }
}

int Touch_FaceDetection(void)
{
    return MXT_FaceDetection();
}
EXPORT_SYMBOL(Touch_FaceDetection);

uint8_t read_mem(uint16_t start, uint8_t size, uint8_t *mem)
{
    struct i2c_adapter *adap = atmel_mxt165->client->adapter;
    struct i2c_msg msgs[]= {
        [0] = {
                .addr = atmel_mxt165->client->addr,
                .flags = 0,
                .len = 2,
                .buf = (uint8_t*)&start,
        },

        [1] = {
                .addr = atmel_mxt165->client->addr,
                .flags = 1,
                .len = size,
                .buf = mem,
        },
    };

    return (i2c_transfer(adap, msgs, 2) < 0) ? -EIO : 0;
}

uint8_t write_mem(uint16_t start, uint8_t size, uint8_t *mem)
{
    uint8_t data[256];  // max tranfer data size = 256-2 (16 bit reg address)
    uint8_t status = 0;

    struct i2c_adapter *adap = atmel_mxt165->client->adapter;
    struct i2c_msg msgs[]= {
        [0] = {
            .addr = atmel_mxt165->client->addr,
            .flags = 0,
            .len = size+2,
            .buf = data,
        }
    };

    if (size <= 0)
        return status;

    data[0] = LSB(start);
    data[1] = MSB(start);

    memcpy(data+2, mem, size);
    TCH_DBG(DB_LEVEL2, "write mem, start = 0x%x, size = %d", start, size);

    status = (i2c_transfer(adap, msgs, 1) < 0) ? -EIO : 0;
    if (status != 0)
        TCH_ERR("I2C transfer failed.");

    return status;
}

// brief Reads the id part of info block.
// Reads the id part of the info block (7 bytes) from touch IC to info_block struct.
uint8_t read_id_block(info_id_t *id)
{
    uint8_t status = 0;
    uint8_t data[7];

    status = read_mem(0, 7, (void *) data);
    if (status != READ_MEM_OK)
    {
        TCH_ERR("Read id information failed, status = %d", status);
        return status;
    }
    id->family_id            = data[0];
    id->variant_id           = data[1];
    id->version              = data[2];
    id->build                = data[3];
    id->matrix_x_size        = data[4];
    id->matrix_y_size        = data[5];
    id->num_declared_objects = data[6];

    TCH_DBG(DB_LEVEL1, "Done.");
    return status;
}

// brief Reads the object table of info block.
// Reads the object table part of the info block from touch IC to info_block struct.
uint8_t read_object_table(object_t *obj, unsigned int num)
{
    uint8_t status = 0, iLoop = 0, jLoop = 0, id = 0;
    uint8_t data[DEFAULT_REG_NUM][6];

    if (num > DEFAULT_REG_NUM)
    {
        TCH_ERR("Please increase default reg number, REG_NUM > %d",DEFAULT_REG_NUM);
        return READ_MEM_FAILED;
    }

    status = read_mem(7, num*6, (void *) data);
    if (status != READ_MEM_OK)
    {
        TCH_ERR("Read object table information failed, status = %d", status);
        return status;
    }

    atmel_mxt165->first_finger_id = -1;
    for (iLoop=0; iLoop<num; iLoop++)
    {
        obj->object_type = data[iLoop][0];
        obj->i2c_address = ((uint16_t) data[iLoop][2] << 8) | data[iLoop][1];
        obj->size        = data[iLoop][3];
        obj->instances   = data[iLoop][4];
        obj->num_report_ids = data[iLoop][5];

        // Save the address to global variable
        if (obj->object_type < DEFAULT_REG_NUM)
        {
            g_RegAddr[obj->object_type] = obj->i2c_address;
        }

        // Save the Report ID information
        for (jLoop=0; jLoop<obj->num_report_ids; jLoop++)
        {
           id++;
           g_ReportID[id] = obj->object_type;

           // Save the first touch ID
           if ((obj->object_type == TOUCH_MULTITOUCHSCREEN_T9) && (atmel_mxt165->first_finger_id == -1))
               atmel_mxt165->first_finger_id = id;
        }

        obj++;
    }

    TCH_DBG(DB_LEVEL1, "Done.");
    return status;
}

uint32_t MXT_CRCSoft24(uint32_t crc, uint8_t FirstByte, uint8_t SecondByte)
{
    uint32_t crcPoly;
    uint32_t Result;
    uint16_t WData;

    crcPoly = 0x80001b;

    WData = (uint16_t) ((uint16_t)(SecondByte << 8) | FirstByte);

    Result = ((crc << 1) ^ ((uint32_t) WData));
    if (Result & 0x1000000)
    {
        Result ^= crcPoly;
    }

    return Result;
}

uint32_t MXT_CheckCRC(void)
{
    uint8_t crc_data[256];
    uint8_t iLoop = 0;
    uint32_t CRC = 0;
    object_t *obj_index;

    crc_data[0] = info_block->info_id->family_id;
    crc_data[1] = info_block->info_id->variant_id;
    crc_data[2] = info_block->info_id->version;
    crc_data[3] = info_block->info_id->build;
    crc_data[4] = info_block->info_id->matrix_x_size;
    crc_data[5] = info_block->info_id->matrix_y_size;
    crc_data[6] = info_block->info_id->num_declared_objects;

    obj_index = info_block->objects;
    for (iLoop=0; iLoop<info_block->info_id->num_declared_objects; iLoop++)
    {
        crc_data[iLoop*6+7+0] = obj_index->object_type;
        crc_data[iLoop*6+7+1] = obj_index->i2c_address & 0xFF;
        crc_data[iLoop*6+7+2] = obj_index->i2c_address >> 8;
        crc_data[iLoop*6+7+3] = obj_index->size;
        crc_data[iLoop*6+7+4] = obj_index->instances;
        crc_data[iLoop*6+7+5] = obj_index->num_report_ids;

        obj_index++;
    }
    crc_data[iLoop*6+7] = 0x0;

    for (iLoop=0; iLoop<(info_block->info_id->num_declared_objects*3+4); iLoop++)
    {
        CRC = MXT_CRCSoft24(CRC, crc_data[iLoop*2], crc_data[iLoop*2+1]);
    }
    CRC = CRC & 0x00FFFFFF;

    TCH_DBG(DB_LEVEL1, "Done. CRC = 0x%x", CRC);
    return CRC;
}

void MXT_GetConfigStatus(void)
{
    uint16_t addr;
    uint8_t data = 0x01; // Nonzero value
    uint8_t msg_data[8];
    uint8_t i;

    addr = g_RegAddr[GEN_COMMANDPROCESSOR_T6] + 3;  // Cal Report all field address.
                                                    // T6 address + 3
    write_mem(addr, 1, &data);  // write one nonzero value;
    mdelay(5);

    addr = g_RegAddr[GEN_MESSAGEPROCESSOR_T5];
    while (1)
    {
        read_mem(addr, 8, msg_data);
        TCH_DBG(DB_LEVEL1, "Message Address = 0x%x", addr);

        for (i=0; i<8; i++)
        {
            TCH_DBG(DB_LEVEL1, "Message Data = 0x%x", msg_data[i]);
        }

        if ((g_ReportID[msg_data[0]] == GEN_COMMANDPROCESSOR_T6) ||
            (g_ReportID[msg_data[0]] == 0xFF) ||
            (g_ReportID[msg_data[0]] == 0x00))
            break;
    }

    TCH_DBG(DB_LEVEL1, "Done.");
}

void MXT_Calibrate(void)
{
    uint16_t addr = 0;
    uint8_t data = 0x55; // Nonzero value

    addr = g_RegAddr[GEN_COMMANDPROCESSOR_T6] + 2;  // Call the calibrate field, command processor + 2
    write_mem(addr, 1, &data);  // Write one nonzero value.

    TCH_DBG(DB_LEVEL1, "Done.");
}

void MXT_InitConfig(void)
{
    uint8_t v16_T7[]  = { 32, 10, 50 };
    uint8_t v16_T8[]  = { 6, 0, 5, 5, 0, 0, 15, 30 };
    uint8_t v16_T9[]  = { 143, 0, 0, 15, 11, 0, 48, 60, 3, 3, 0, 3, 1, 0, 2, 5, 5, 5, 0x0, 0x0, 0x0, 0x0, 0, 0, 0, 0, 0, 0, 0, 0, 18 };
    uint8_t v16_T15[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uint8_t v16_T18[] = { 0, 0, };
    uint8_t v16_T19[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uint8_t v16_T20[] = { 0, 100, 100, 100, 100, 0, 0, 30, 20, 4, 15, 5 };
    uint8_t v16_T22[] = { 7, 0, 0, 0x0, 0x0, 0x0, 0x0, 0, 30, 0, 0, 0, 10, 15, 20, 30, 0 };
    uint8_t v16_T23[] = { 0, 0, 0, 0, 0, 0, 0, 0x0, 0x0, 0, 0, 0x0, 0x0 };
    uint8_t v16_T24[] = { 0, 0, 0x0, 0x0, 0, 0, 0, 0, 0, 0, 0, 0x0, 0, 0x0, 0, 0x0, 0, 0x0, 0 };
    uint8_t v16_T25[] = { 0, 0, 0xF8, 0x2A, 0x70, 0x17, 0x28, 0x23, 0x88, 0x13, 0x0, 0x0, 0x0, 0x0 };
    uint8_t v16_T27[] = { 0, 0, 0, 0, 0, 0x0, 0x0 };
    uint8_t v16_T28[] = { 0, 0, 0, 16, 16, 30 };

    uint8_t iLoop = 0, MaxOBJSize = 0;
    uint16_t addr;
    object_t *obj_index;

    MaxOBJSize = info_block->info_id->num_declared_objects;
    obj_index = info_block->objects;
    cap_touch_fw_version = info_block->info_id->version;

//    if (info_block->info_id->version == 0x16)
    {
        TCH_MSG("Use 1.6 firmware config.");
        for (iLoop=0; iLoop<MaxOBJSize; iLoop++)
        {
            addr = obj_index->i2c_address;
            switch(obj_index->object_type)
            {
            case GEN_POWERCONFIG_T7 :
                write_mem(addr, sizeof(v16_T7), v16_T7);
                break;
            case GEN_ACQUISITIONCONFIG_T8 :
                write_mem(addr, sizeof(v16_T8), v16_T8);
                break;
            case TOUCH_MULTITOUCHSCREEN_T9 :
                write_mem(addr, sizeof(v16_T9), v16_T9);
                break;
            case TOUCH_KEYARRAY_T15 :
                write_mem(addr, sizeof(v16_T15), v16_T15);
                break;
            case SPT_COMCONFIG_T18 :
                write_mem(addr, sizeof(v16_T18), v16_T18);
                break;
            case SPT_GPIOPWM_T19 :
                write_mem(addr, sizeof(v16_T19), v16_T19);
                break;
            case PROCI_GRIPFACESUPPRESSION_T20 :
                write_mem(addr, sizeof(v16_T20), v16_T20);
                break;
            case PROCG_NOISESUPPRESSION_T22 :
                write_mem(addr, sizeof(v16_T22), v16_T22);
                break;
            case TOUCH_PROXIMITY_T23 :
                write_mem(addr, sizeof(v16_T23), v16_T23);
                break;
            case PROCI_ONETOUCHGESTUREPROCESSOR_T24 :
                write_mem(addr, sizeof(v16_T24), v16_T24);
                break;
            case SPT_SELFTEST_T25 :
                write_mem(addr, sizeof(v16_T25), v16_T25);
                break;
            case PROCI_TWOTOUCHGESTUREPROCESSOR_T27 :
                write_mem(addr, sizeof(v16_T27), v16_T27);
                break;
            case SPT_CTECONFIG_T28 :
                write_mem(addr, sizeof(v16_T28), v16_T28);
                break;
            default:
                break;
            }
            obj_index++;
        }
    }

    MXT_Calibrate();
    TCH_DBG(DB_LEVEL1, "Done.");
}

static void atmel_mxt165_reset(void)
{   // Active RESET pin to wakeup ts
    int i;

    if (gpio_get_value(GPIO_TP_RST_N) == LOW)
        gpio_set_value(GPIO_TP_RST_N, HIGH);
    gpio_set_value(GPIO_TP_RST_N, LOW);
    mdelay(10);
    gpio_set_value(GPIO_TP_RST_N, HIGH);
    for (i=0; i<10; i++)
    {
        mdelay(20);
        if (gpio_get_value(GPIO_TP_INT_N) == LOW)
            return ;
    }

    TCH_ERR("Failed.");
}

uint8_t atmel_mxt165_check(uint8_t I2C_address)
{
    uint8_t iLoop = 0, status = 0;
    info_id_t *id;
    object_t *obj;
    uint8_t crc24[3];

    // Read the info block data.
    id = kmalloc(sizeof(info_id_t), GFP_KERNEL);
    if (id == NULL)
    {
        return DRIVER_SETUP_INCOMPLETE;
    }

    if (read_id_block(id) != READ_MEM_OK)
    {
        TCH_ERR("read_id_block(id) != READ_MEM_OK");
        kfree(id);
        return DRIVER_SETUP_INCOMPLETE;
    }
    info_block->info_id = id;

    TCH_MSG("Family ID       = 0x%x", info_block->info_id->family_id);
    TCH_MSG("Variant ID      = 0x%x", info_block->info_id->variant_id);
    TCH_MSG("Version         = 0x%x", info_block->info_id->version);
    TCH_MSG("Build           = 0x%x", info_block->info_id->build);
    TCH_MSG("Matrix X size   = 0x%x", info_block->info_id->matrix_x_size);
    TCH_MSG("Matrix Y size   = 0x%x", info_block->info_id->matrix_y_size);
    TCH_MSG("Number of Table = 0x%x", info_block->info_id->num_declared_objects);

    // Read the Object Table data.
    obj = kmalloc(sizeof(object_t) * info_block->info_id->num_declared_objects, GFP_KERNEL);
    if (obj == NULL)
    {
        goto ERR_1;
    }

    if (read_object_table(obj, id->num_declared_objects) != READ_MEM_OK)
    {
        TCH_ERR("read_object_table(obj) != READ_MEM_OK");
        goto ERR_2;
    }
    info_block->objects = obj;

    for (iLoop=0; iLoop<info_block->info_id->num_declared_objects; iLoop++)
    {
        TCH_DBG(DB_LEVEL2, "type       = 0x%x", obj->object_type);
        TCH_DBG(DB_LEVEL2, "address    = 0x%x", obj->i2c_address);
        TCH_DBG(DB_LEVEL2, "size       = 0x%x", obj->size);
        TCH_DBG(DB_LEVEL2, "instances  = 0x%x", obj->instances);
        TCH_DBG(DB_LEVEL2, "report ids = 0x%x", obj->num_report_ids);
        TCH_DBG(DB_LEVEL2, "************************************************************");

        obj++;
    }

    // Read the Checksum info
    status = read_mem(info_block->info_id->num_declared_objects*6+7, 3, crc24);
    if (status != READ_MEM_OK)
    {
        TCH_ERR("Read CRC information failed, status = 0x%x", status);
        goto ERR_2;
    }
    info_block->CRC = (crc24[2] << 16) | (crc24[1] << 8) | crc24[0];
    TCH_DBG(DB_LEVEL1, "Internal CRC = 0x%x", info_block->CRC);

    if (info_block->CRC != MXT_CheckCRC())
    {
        TCH_ERR("Checksum failed.");
        goto ERR_2;
    }

    MXT_GetConfigStatus();
    MXT_InitConfig();

    TCH_DBG(DB_LEVEL1, "Done.");
    return(DRIVER_SETUP_OK);

ERR_2:
   kfree(obj);
ERR_1:
   kfree(id);
   TCH_ERR("Failed.");
   return DRIVER_SETUP_INCOMPLETE;
}

static void atmel_mxt165_report(struct atmel_mxt165_info *ts, int i, int x, int y, int pressure)
{
    if (pressure == NO_TOUCH)
    {   // Finger Up.
        if (y >= TS_MAX_Y)
        {   //Key Area
            if (ts->points[i].first_area == PRESS_KEY1_AREA)
            {
                input_report_key(ts->keyevent_input, KEY_HOME, 0);
                TCH_MSG("POS%d Report HOME key Up.", i+1);
            }
            else if (ts->points[i].first_area == PRESS_KEY2_AREA)
            {
                input_report_key(ts->keyevent_input, KEY_MENU, 0);
                TCH_MSG("POS%d Report MENU key Up.", i+1);
            }
            else if (ts->points[i].first_area == PRESS_KEY3_AREA)
            {
                input_report_key(ts->keyevent_input, KEY_BACK, 0);
                TCH_MSG("POS%d Report BACK key Up.", i+1);
            }
            else if (ts->points[i].first_area == PRESS_KEY4_AREA)
            {
                input_report_key(ts->keyevent_input, KEY_SEARCH, 0);
                TCH_MSG("POS%d Report SEARCH key Up.", i+1);
            }
        }
        if (ts->points[i].first_area == PRESS_TOUCH_AREA)
        {   // Report touch pen up event only if first tap in touch area.
            if (i == 0)
            {
#ifdef ATMEL_MXT165_MT
                input_report_abs(ts->touch_input, ABS_MT_TOUCH_MAJOR, 0);
                input_report_abs(ts->touch_input, ABS_MT_POSITION_X, x);
                input_report_abs(ts->touch_input, ABS_MT_POSITION_Y, y);
#else
                input_report_abs(ts->touch_input, ABS_X, x);
                input_report_abs(ts->touch_input, ABS_Y, y);
                input_report_abs(ts->touch_input, ABS_PRESSURE, pressure);
                input_report_key(ts->touch_input, BTN_TOUCH, 0);
#endif
            }
            if (i == 1)
            {
#ifdef ATMEL_MXT165_MT
                input_report_abs(ts->touch_input, ABS_MT_TOUCH_MAJOR, 0);
                input_report_abs(ts->touch_input, ABS_MT_POSITION_X, x);
                input_report_abs(ts->touch_input, ABS_MT_POSITION_Y, y);
#else
                input_report_abs(ts->touch_input, ABS_HAT0X, x);
                input_report_abs(ts->touch_input, ABS_HAT0Y, y);
                input_report_abs(ts->touch_input, ABS_PRESSURE, pressure);
                input_report_key(ts->touch_input, BTN_2, 0);
#endif
            }
#ifdef ATMEL_MXT165_MT
            input_mt_sync(ts->touch_input);
#endif
        }
        ts->points[i].num = 0;
        ts->points[i].first_area = NO_TOUCH;
        ts->points[i].last_area = NO_TOUCH;
    }
    else
    {   // Finger Down.
        if (y < TS_MAX_Y)
        {   //Touch Area
            if (ts->points[i].num == 0)
                ts->points[i].first_area = PRESS_TOUCH_AREA;
            if (ts->points[i].first_area == PRESS_KEY1_AREA)
            {   // Flick from home key area to touch area.
                input_report_key(ts->keyevent_input, KEY_HOME, 0);
                TCH_MSG("POS%d Report HOME key Up.", i+1);
                ts->points[i].first_area = NO_TOUCH;
            }
            else if (ts->points[i].first_area == PRESS_KEY2_AREA)
            {   // Flick from menu key area to touch area.
                input_report_key(ts->keyevent_input, KEY_MENU, 0);
                TCH_MSG("POS%d Report MENU key Up.", i+1);
                ts->points[i].first_area = NO_TOUCH;
            }
            else if (ts->points[i].first_area == PRESS_KEY3_AREA)
            {   // Flick from back key area to touch area.
                input_report_key(ts->keyevent_input, KEY_BACK, 0);
                TCH_MSG("POS%d Report BACK key Up.", i+1);
                ts->points[i].first_area = NO_TOUCH;
            }
            else if (ts->points[i].first_area == PRESS_KEY4_AREA)
            {   // Flick from search key area to touch area.
                input_report_key(ts->keyevent_input, KEY_SEARCH, 0);
                TCH_MSG("POS%d Report SEARCH key Up.", i+1);
                ts->points[i].first_area = NO_TOUCH;
            }
            else if (ts->points[i].first_area == PRESS_TOUCH_AREA)
            {
                if (i == 0)
                {
#ifdef ATMEL_MXT165_MT
                    input_report_abs(ts->touch_input, ABS_MT_TOUCH_MAJOR, 255);
                    input_report_abs(ts->touch_input, ABS_MT_POSITION_X, x);
                    input_report_abs(ts->touch_input, ABS_MT_POSITION_Y, y);
#else
                    input_report_abs(ts->touch_input, ABS_X, x);
                    input_report_abs(ts->touch_input, ABS_Y, y);
                    input_report_abs(ts->touch_input, ABS_PRESSURE, pressure);
                    input_report_key(ts->touch_input, BTN_TOUCH, 1);
#endif
                }
                if (i == 1)
                {
#ifdef ATMEL_MXT165_MT
                    input_report_abs(ts->touch_input, ABS_MT_TOUCH_MAJOR, 255);
                    input_report_abs(ts->touch_input, ABS_MT_POSITION_X, x);
                    input_report_abs(ts->touch_input, ABS_MT_POSITION_Y, y);
#else
                    input_report_abs(ts->touch_input, ABS_HAT0X, x);
                    input_report_abs(ts->touch_input, ABS_HAT0Y, y);
                    input_report_abs(ts->touch_input, ABS_PRESSURE, pressure);
                    input_report_key(ts->touch_input, BTN_2, 1);
#endif
                }
#ifdef ATMEL_MXT165_MT
                input_mt_sync(ts->touch_input);
#endif
            }
            ts->points[i].last_area = PRESS_TOUCH_AREA;
        }
        else
        {   //Key Area
            if (x < TS_KEY1_X)
            {
                if (ts->points[i].num == 0)
                {
                    input_report_key(ts->keyevent_input, KEY_HOME, 1);
                    TCH_MSG("POS%d Report HOME key Down.", i+1);
                    ts->points[i].first_area = PRESS_KEY1_AREA;
                }
                ts->points[i].last_area = PRESS_KEY1_AREA;
            }
            else if (x < TS_KEY2_X)
            {
                if (ts->points[i].num == 0)
                {
                    input_report_key(ts->keyevent_input, KEY_MENU, 1);
                    TCH_MSG("POS%d Report MENU key Down.", i+1);
                    ts->points[i].first_area = PRESS_KEY2_AREA;
                }
                ts->points[i].last_area = PRESS_KEY2_AREA;
            }
            else if (x < TS_KEY3_X)
            {
                if (ts->points[i].num == 0)
                {
                    input_report_key(ts->keyevent_input, KEY_BACK, 1);
                    TCH_MSG("POS%d Report BACK key Down.", i+1);
                    ts->points[i].first_area = PRESS_KEY3_AREA;
                }
                ts->points[i].last_area = PRESS_KEY3_AREA;
            }
            else
            {
                if (ts->points[i].num == 0)
                {
                    input_report_key(ts->keyevent_input, KEY_SEARCH, 1);
                    TCH_MSG("POS%d Report SEARCH key Down.", i+1);
                    ts->points[i].first_area = PRESS_KEY4_AREA;
                }
                ts->points[i].last_area = PRESS_KEY4_AREA;
            }
        }
        ts->points[i].num++;
    }
    //input_sync(ts->touch_input);
}

static void atmel_mxt165_isr_workqueue(struct work_struct *work)
{
    struct atmel_mxt165_info *ts = container_of(work, struct atmel_mxt165_info, wqueue);

    uint16_t Buf_X;
    uint16_t Buf_Y;
    uint8_t count = 3, MSG_TYPE = 0;
    int i,first_touch_id = atmel_mxt165->first_finger_id;

    g_I2CAddress = g_RegAddr[GEN_MESSAGEPROCESSOR_T5];

    while (!ts->suspend_state)	// Read data until chip queue no data.
    {
        // If read/write I2C data faield, re-action 3 times.
        while (read_mem(g_I2CAddress , 8, (void *) g_MsgData ) != 0)
        {
            TCH_ERR("Read data failed, Re-read.");
            mdelay(3);

            count--;
            if (count == 0)
            {
                TCH_MSG("Can't Read/write data, reset chip.");
                TCH_MSG("GPIO_TP_RST_N = %d", gpio_get_value(GPIO_TP_RST_N));
                TCH_MSG("GPIO_TP_INT_N = %d", gpio_get_value(GPIO_TP_INT_N));
                atmel_mxt165_reset();  // Re-try 3 times, can't read/write data, reset atmel_mxt165 chip
                MXT_GetConfigStatus();
                MXT_InitConfig();
                return ;
            }
        }

        MSG_TYPE = g_ReportID[g_MsgData[0]];
        if ((MSG_TYPE == 0xFF) || (MSG_TYPE == 0x00))   // No data, return
        {
            TCH_DBG(DB_LEVEL2, "No data, Leave ISR.");
            TCH_DBG(DB_LEVEL2, "GPIO_TP_RST_N = %d", gpio_get_value(GPIO_TP_RST_N));
            TCH_DBG(DB_LEVEL2, "GPIO_TP_INT_N = %d", gpio_get_value(GPIO_TP_INT_N));
            break;
        }
        else if (MSG_TYPE == GEN_COMMANDPROCESSOR_T6)
        {
            if (g_MsgData[1] & 0x04)
                TCH_ERR("I2C-Compatible Checksum Errors.");
            if (g_MsgData[1] & 0x08)
                TCH_ERR("Configuration Errors.");
            if (g_MsgData[1] & 0x10)
                TCH_MSG("Calibrating.");
            if (g_MsgData[1] & 0x20)
                TCH_ERR("Signal Errors.");
            if (g_MsgData[1] & 0x40)
                TCH_ERR("Overflow Errors.");
            if (g_MsgData[1] & 0x80)
                TCH_MSG("Reseting.");
            if (g_MsgData[1] & 0x04 || ((g_MsgData[1] & 0x10) && !gpio_get_value(GPIO_TP_INT_N)) || g_MsgData[1] & 0x20)
            {
                TCH_MSG("Prepare to reset touch IC. Please wait a moment...");
                atmel_mxt165_reset();
                MXT_GetConfigStatus();
                MXT_InitConfig();
                TCH_MSG("Finish to reset touch IC.");
                TCH_MSG("GPIO_TP_RST_N = %d", gpio_get_value(GPIO_TP_RST_N));
                TCH_MSG("GPIO_TP_INT_N = %d", gpio_get_value(GPIO_TP_INT_N));
                return ;
            }
        }
        else if ((MSG_TYPE == PROCI_GRIPFACESUPPRESSION_T20))
        {
            if (g_MsgData[1] & 0x01)
            {
                TCH_MSG("Face Down, Status = 0x%x, ID = %d", g_MsgData[1], MSG_TYPE);
                ts->facedetect = TRUE;
                continue;
            }
            else if (ts->facedetect)
            {
                TCH_MSG("Face Up, Status = 0x%x, ID = %d", g_MsgData[1], MSG_TYPE);
                ts->facedetect = FALSE;
                for (i=0; i<TS_MAX_POINTS; i++)
                {
                    if (ts->points[i].last_area > NO_TOUCH)
                    {   // Report a touch pen up event when touch detects finger down events in face detection(Face Up).
                        atmel_mxt165_report(ts, i, ts->points[i].x, ts->points[i].y, NO_TOUCH);
                        TCH_MSG("POS%d Up, Status = 0x%x, X = %d, Y = %d, ID = %d", i+1, g_MsgData[1], ts->points[i].x, ts->points[i].y, MSG_TYPE);
                        input_sync(ts->touch_input);
                    }
                }
            }
        }
        else if (MSG_TYPE != TOUCH_MULTITOUCHSCREEN_T9)
        {
            TCH_DBG(DB_LEVEL1, "Get other report id = %d", MSG_TYPE);
            for (i=0; i<8; i++)
                TCH_DBG(DB_LEVEL1, "Report T%d[%d] = %d", MSG_TYPE, i ,g_MsgData[i]);
            continue;
        }

        Buf_X = (g_MsgData[2] << 2) | (g_MsgData[4] >> 6);
        Buf_Y = (g_MsgData[3] << 2) | ((g_MsgData[4] >> 2) & 0x03);

        for (i=0; i<TS_MAX_POINTS; i++)
        {
            if (g_MsgData[0] == first_touch_id + i)
            {
                ts->points[i].x = Buf_X;
                ts->points[i].y = Buf_Y;

                if (g_MsgData[1] & 0xC0)
                {	// Finger Down.
                    if (i == 0)
                    {	// POS1
                        atmel_mxt165_report(ts, i, ts->points[i].x, ts->points[i].y, 255);
                        TCH_DBG(DB_LEVEL1, "POS%d Down %d, Status = 0x%x, X = %d, Y = %d, ID = %d", g_MsgData[0]-1, ts->points[i].num, g_MsgData[1], Buf_X, Buf_Y, MSG_TYPE);
#ifdef ATMEL_MXT165_MT
                        if (ts->points[i+1].last_area > NO_TOUCH)
                        {
                            atmel_mxt165_report(ts, i+1, ts->points[i+1].x, ts->points[i+1].y, 255);
                            TCH_DBG(DB_LEVEL1, "POS2 Down %d, X = %d, Y = %d", ts->points[i+1].num, ts->points[i+1].x, ts->points[i+1].y);
                        }
#endif
                        input_sync(ts->touch_input);
                    }
                    else if (i == 1)
                    {	// POS2
#ifdef ATMEL_MXT165_MT
                        if (ts->points[i-1].last_area > NO_TOUCH)
                        {
                            atmel_mxt165_report(ts, i-1, ts->points[i-1].x, ts->points[i-1].y, 255);
                            TCH_DBG(DB_LEVEL1, "POS1 Down %d, X = %d, Y = %d", ts->points[i-1].num, ts->points[i-1].x, ts->points[i-1].y);
                        }
#endif
                        atmel_mxt165_report(ts, i, ts->points[i].x, ts->points[i].y, 255);
                        TCH_DBG(DB_LEVEL1, "POS%d Down %d, Status = 0x%x, X = %d, Y = %d, ID = %d", g_MsgData[0]-1, ts->points[i].num, g_MsgData[1], Buf_X, Buf_Y, MSG_TYPE);
                        input_sync(ts->touch_input);
                    }
                    //input_sync(ts->touch_input);
                }
                else if ((g_MsgData[1] & 0x20) == 0x20)
                {	// Finger Up.
                    if (ts->points[i].last_area > NO_TOUCH)
                    {
                        if (i == 0)
                        {	// POS1
                            atmel_mxt165_report(ts, i, ts->points[i].x, ts->points[i].y, NO_TOUCH);
                            TCH_MSG("POS%d Up, Status = 0x%x, X = %d, Y = %d, ID = %d", g_MsgData[0]-1, g_MsgData[1], Buf_X, Buf_Y, MSG_TYPE);
#ifdef ATMEL_MXT165_MT
                            if (ts->points[i+1].last_area > NO_TOUCH)
                            {
                                atmel_mxt165_report(ts, i+1, ts->points[i+1].x, ts->points[i+1].y, 255);
                                TCH_DBG(DB_LEVEL1, "POS2 Down %d, X = %d, Y = %d", ts->points[i+1].num, ts->points[i+1].x, ts->points[i+1].y);
                            }
#endif
                            input_sync(ts->touch_input);
#ifdef ATMEL_MXT165_MT
                            if (ts->points[i+1].last_area > NO_TOUCH)
                            {
                                atmel_mxt165_report(ts, i+1, ts->points[i+1].x, ts->points[i+1].y, 255);
                                TCH_DBG(DB_LEVEL1, "POS2 Down %d, X = %d, Y = %d", ts->points[i+1].num, ts->points[i+1].x, ts->points[i+1].y);
                            }
#endif
                        }
                        else if (i == 1)
                        {	// POS2
#ifdef ATMEL_MXT165_MT
                            if (ts->points[i-1].last_area > NO_TOUCH)
                            {
                                atmel_mxt165_report(ts, i-1, ts->points[i-1].x, ts->points[i-1].y, 255);
                                TCH_DBG(DB_LEVEL1, "POS1 Down %d, X = %d, Y = %d", ts->points[i-1].num, ts->points[i-1].x, ts->points[i-1].y);
                            }
#endif
                            atmel_mxt165_report(ts, i, ts->points[i].x, ts->points[i].y, NO_TOUCH);
                            TCH_MSG("POS%d Up, Status = 0x%x, X = %d, Y = %d, ID = %d", g_MsgData[0]-1, g_MsgData[1], Buf_X, Buf_Y, MSG_TYPE);
                            input_sync(ts->touch_input);
#ifdef ATMEL_MXT165_MT
                            if (ts->points[i-1].last_area > NO_TOUCH)
                            {
                                atmel_mxt165_report(ts, i-1, ts->points[i-1].x, ts->points[i-1].y, 255);
                                TCH_DBG(DB_LEVEL1, "POS1 Down %d, X = %d, Y = %d", ts->points[i-1].num, ts->points[i-1].x, ts->points[i-1].y);
                            }
#endif
                        }
                        input_sync(ts->touch_input);
                    }
                }
            }
        }

    }

}

static irqreturn_t atmel_mxt165_isr(int irq, void * handle)
{
    struct atmel_mxt165_info *ts = handle;

    if (!ts->suspend_state)
        schedule_work(&ts->wqueue);

    return IRQ_HANDLED;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
void atmel_mxt165_early_suspend(struct early_suspend *h)
{
    uint16_t power_cfg_address = 0;
    uint8_t data[3];
    uint8_t rc = 0;

    //cancel_work_sync(&atmel_mxt165->wqueue);
    power_cfg_address = g_RegAddr[GEN_POWERCONFIG_T7];
    read_mem(power_cfg_address, 3, (void *) atmel_mxt165->T7);

    data[0] = 0;
    data[1] = 0;
    data[2] = 0;

    //MXT_GetConfigStatus();  // Clear all data, avoid intrrupt no resume.

    rc = write_mem(power_cfg_address, 3, (void *) data);
    if (rc != 0)
        TCH_ERR("Driver can't enter deep sleep mode [%d].", rc);
    else
        TCH_MSG("Enter deep sleep mode.");

    disable_irq(atmel_mxt165->irq);
    atmel_mxt165->suspend_state = TRUE;
    //gpio_tlmm_config(GPIO_CFG(GPIO_TP_INT_N, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    gpio_tlmm_config(GPIO_CFG(GPIO_TP_RST_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    gpio_set_value(GPIO_TP_RST_N, HIGH);

    TCH_MSG("GPIO_TP_RST_N = %d", gpio_get_value(GPIO_TP_RST_N));
    TCH_MSG("GPIO_TP_INT_N = %d", gpio_get_value(GPIO_TP_INT_N));
    TCH_DBG(DB_LEVEL1, "Done.");
}

void atmel_mxt165_late_resume(struct early_suspend *h)
{
    uint16_t power_cfg_address = 0;
    uint8_t i, rc = 0;

    power_cfg_address = g_RegAddr[GEN_POWERCONFIG_T7];
    //gpio_tlmm_config(GPIO_CFG(GPIO_TP_INT_N, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    gpio_tlmm_config(GPIO_CFG(GPIO_TP_RST_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    gpio_set_value(GPIO_TP_RST_N, HIGH);
    atmel_mxt165->suspend_state = FALSE;
    enable_irq(atmel_mxt165->irq);

    for (i=0; i<TS_MAX_POINTS; i++)
    {
        if (atmel_mxt165->points[i].last_area > NO_TOUCH)
            {
                TCH_MSG("POS%d Up, X = %d, Y = %d", i+1, atmel_mxt165->points[i].x, atmel_mxt165->points[i].y);
                atmel_mxt165_report(atmel_mxt165, i, atmel_mxt165->points[i].x, atmel_mxt165->points[i].y, NO_TOUCH);
                input_sync(atmel_mxt165->touch_input);
            }
    }
    rc = write_mem(power_cfg_address, 3, (void *) atmel_mxt165->T7);
    if (rc != 0)
        TCH_ERR("Driver can't return from deep sleep mode [%d].", rc);
    else
        TCH_MSG("Return from sleep mode.");

    //atmel_mxt165_reset();
    MXT_GetConfigStatus();
    //MXT_InitConfig();
    MXT_Calibrate();

    TCH_MSG("GPIO_TP_RST_N = %d", gpio_get_value(GPIO_TP_RST_N));
    TCH_MSG("GPIO_TP_INT_N = %d", gpio_get_value(GPIO_TP_INT_N));
    TCH_DBG(DB_LEVEL1, "Done.");
}
#endif

static int atmel_mxt165_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct input_dev *touch_input;
    struct input_dev *keyevent_input;
    struct vreg *device_vreg;
    int i, ret = 0;

    // I2C Check
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        TCH_ERR("Check I2C functionality failed.");
        ret = -ENODEV;
        goto err_check_functionality_failed;
    }

    // Allocate Memory
    atmel_mxt165 = kzalloc(sizeof(struct atmel_mxt165_info), GFP_KERNEL);
    if (atmel_mxt165 == NULL)
    {
        TCH_ERR("Allocate atmel_mxt165 kernel memory failed.");
        ret = -ENOMEM;
        goto err_alloc_atmel_mxt165_failed;
    }
    info_block = kzalloc(sizeof(info_block_t), GFP_KERNEL);
    if (info_block == NULL)
    {
        TCH_ERR("Allocate info_block kernel memory failed.");
        ret = -ENOMEM;
        goto err_alloc_info_block_failed;
    }

    // Power ON
    device_vreg = vreg_get(0, TOUCH_DEVICE_VREG);
    if (!device_vreg) {
        TCH_ERR("Get vreg failed.");
        ret = -EIO;
        goto err_power_failed;
    }
    vreg_set_level(device_vreg, 3000);
    TCH_DBG(0, "Power status = %d", vreg_enable(device_vreg));

    // Config GPIO
    gpio_tlmm_config(GPIO_CFG(GPIO_TP_INT_N, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    gpio_tlmm_config(GPIO_CFG(GPIO_TP_RST_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
    gpio_set_value(GPIO_TP_RST_N, HIGH);
    atmel_mxt165_reset();

    // Confirm Touch Chip
    atmel_mxt165->client = client;
    i2c_set_clientdata(client, atmel_mxt165);
    atmel_mxt165->client->addr = TOUCH_DEVICE_I2C_ADDRESS;
    for (i=0; i<5; i++)
    {
        if (atmel_mxt165_check(atmel_mxt165->client->addr) == DRIVER_SETUP_INCOMPLETE)
            TCH_ERR("Check touch IC failed %d times.", i+1);
        else
            break;
    }
    if (i==5)
        goto err_check_touch_IC_failed;

    // Allocate Input Device
    touch_input = input_allocate_device();
    if (touch_input == NULL)
    {
        TCH_ERR("Allocate touch input device failed.");
        ret = -ENODEV;
        goto err_touch_input_dev_alloc_failed;
    }
    keyevent_input = input_allocate_device();
    if (keyevent_input == NULL)
    {
        TCH_ERR("Allocate key input device failed.");
        ret = -ENODEV;
        goto err_key_input_dev_alloc_failed;
    }

    touch_input->name  = "atmel_mxt165_touch";
    touch_input->phys  = "atmel_mxt165/input0";
    set_bit(EV_ABS, touch_input->evbit);
    set_bit(EV_SYN, touch_input->evbit);
#ifndef ATMEL_MXT165_MT
    set_bit(EV_KEY, touch_input->evbit);
    set_bit(BTN_TOUCH, touch_input->keybit);
    set_bit(BTN_2, touch_input->keybit);
    input_set_abs_params(touch_input, ABS_X, TS_MIN_X, TS_MAX_X, 0, 0);
    input_set_abs_params(touch_input, ABS_Y, TS_MIN_Y, TS_MAX_Y, 0, 0);
    input_set_abs_params(touch_input, ABS_HAT0X, TS_MIN_X, TS_MAX_X, 0, 0);
    input_set_abs_params(touch_input, ABS_HAT0Y, TS_MIN_Y, TS_MAX_Y, 0, 0);
    input_set_abs_params(touch_input, ABS_PRESSURE, 0, 255, 0, 0);
#else
    input_set_abs_params(touch_input, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(touch_input, ABS_MT_POSITION_X, TS_MIN_X, TS_MAX_X, 0, 0);
    input_set_abs_params(touch_input, ABS_MT_POSITION_Y, TS_MIN_Y, TS_MAX_Y, 0, 0);
#endif

    keyevent_input->name  = "atmel_mxt165_key";
    keyevent_input->phys  = "atmel_mxt165/input1";
    set_bit(EV_KEY, keyevent_input->evbit);
    set_bit(KEY_HOME, keyevent_input->keybit);
    set_bit(KEY_MENU, keyevent_input->keybit);
    set_bit(KEY_BACK, keyevent_input->keybit);
    set_bit(KEY_SEARCH, keyevent_input->keybit);

    // Register Input Device
    atmel_mxt165->touch_input = touch_input;
    if (input_register_device(atmel_mxt165->touch_input))
    {
        TCH_ERR("Register touch input device failed.");
        ret = -ENODEV;
        goto err_touch_input_register_device_failed;
    }
    atmel_mxt165->keyevent_input = keyevent_input;
    if (input_register_device(atmel_mxt165->keyevent_input))
    {
        TCH_ERR("Register key input device failed.");
        ret = -ENODEV;
        goto err_key_input_register_device_failed;
    }

    // Init Work Queue and Register IRQ
    INIT_WORK(&atmel_mxt165->wqueue, atmel_mxt165_isr_workqueue);
    atmel_mxt165->irq = MSM_GPIO_TO_INT(GPIO_TP_INT_N);
    if (request_irq(atmel_mxt165->irq, atmel_mxt165_isr, IRQF_TRIGGER_FALLING, client->dev.driver->name, atmel_mxt165))
    {
        TCH_ERR("Request IRQ failed.");
        ret = -EBUSY;
        goto err_request_irq_failed;
    }

    memset(atmel_mxt165->points, 0, sizeof(struct point_info));
    atmel_mxt165->suspend_state = FALSE;
    atmel_mxt165->facedetect = FALSE;
    atmel_mxt165->T7[0] = 32;
    atmel_mxt165->T7[1] = 10;
    atmel_mxt165->T7[2] = 50;
#ifdef CONFIG_HAS_EARLYSUSPEND
    // Register early_suspend
    atmel_mxt165->es.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    atmel_mxt165->es.suspend = atmel_mxt165_early_suspend;
    atmel_mxt165->es.resume = atmel_mxt165_late_resume;

    register_early_suspend(&atmel_mxt165->es);
#endif
    if (gpio_get_value(GPIO_TP_INT_N) == LOW)
        atmel_mxt165_isr_workqueue(&atmel_mxt165->wqueue);

    TCH_DBG(0, "Done.");
    return 0;

    free_irq(atmel_mxt165->irq, atmel_mxt165);
err_request_irq_failed:
    cancel_work_sync(&atmel_mxt165->wqueue);
    kfree(&atmel_mxt165->wqueue);
    input_unregister_device(atmel_mxt165->keyevent_input);
err_key_input_register_device_failed:
    input_unregister_device(atmel_mxt165->touch_input);
err_touch_input_register_device_failed:
    input_free_device(keyevent_input);
err_key_input_dev_alloc_failed:
    input_free_device(touch_input);
err_touch_input_dev_alloc_failed:
err_check_touch_IC_failed:
    dev_set_drvdata(&client->dev, 0);
    vreg_disable(device_vreg);
err_power_failed:
    kfree(info_block);
err_alloc_info_block_failed:
    kfree(atmel_mxt165);
err_alloc_atmel_mxt165_failed:
err_check_functionality_failed:
    TCH_ERR("Failed.");
    return ret;
}

static int atmel_mxt165_remove(struct i2c_client * client)
{
    free_irq(atmel_mxt165->irq, atmel_mxt165);

    cancel_work_sync(&atmel_mxt165->wqueue);
    kfree(&atmel_mxt165->wqueue);

#ifdef CONFIG_HAS_EARLYSUSPEND
    unregister_early_suspend(&atmel_mxt165->es);
#endif

    input_unregister_device(atmel_mxt165->touch_input);
    input_unregister_device(atmel_mxt165->keyevent_input);

    kfree(atmel_mxt165->touch_input);
    kfree(atmel_mxt165->keyevent_input);
    input_free_device(atmel_mxt165->touch_input);
    input_free_device(atmel_mxt165->keyevent_input);

    kfree(atmel_mxt165);
    kfree(info_block->info_id);
    kfree(info_block);

    dev_set_drvdata(&client->dev, 0);
    TCH_DBG(DB_LEVEL1, "Done.");
    return 0;
}

static int atmel_mxt165_suspend(struct i2c_client *client, pm_message_t state)
{
    TCH_DBG(DB_LEVEL1, "Done.");
    return 0;
}

static int atmel_mxt165_resume(struct i2c_client *client)
{
    TCH_DBG(DB_LEVEL1, "Done.");
    return 0;
}

static const struct i2c_device_id atmel_mxt165_id[] = {
    { TOUCH_DEVICE_NAME, TOUCH_DEVICE_I2C_ADDRESS },
    { }
};
MODULE_DEVICE_TABLE(i2c, atmel_mxt165_id);

static struct i2c_driver atmel_mxt165_i2c_driver = {
   .driver = {
      .name   = TOUCH_DEVICE_NAME,
   },
   .id_table   = atmel_mxt165_id,
   .probe      = atmel_mxt165_probe,
   .remove     = atmel_mxt165_remove,
   .suspend    = atmel_mxt165_suspend,
   .resume     = atmel_mxt165_resume,
};

static int __init atmel_mxt165_init( void )
{
    TCH_DBG(DB_LEVEL1, "Done.");
    return i2c_add_driver(&atmel_mxt165_i2c_driver);
}

static void __exit atmel_mxt165_exit( void )
{
    TCH_DBG(DB_LEVEL1, "Done.");
    i2c_del_driver(&atmel_mxt165_i2c_driver);
}

module_init(atmel_mxt165_init);
module_exit(atmel_mxt165_exit);

MODULE_DESCRIPTION("ATMEL MXT165 Touchscreen Driver");
MODULE_AUTHOR("FIH Div2 Dep5 Peripheral Team");
MODULE_VERSION("0.2.4");
MODULE_LICENSE("GPL");
