/*
 * BQ275x0 dfi programming driver
 */
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/skbuff.h>
#include <linux/module.h>
#include <linux/param.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/rtc.h>
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>
#include <mach/bq275x0_battery.h>
#include "../../arch/arm/mach-msm/smd_private.h"
#if defined(CONFIG_FIH_PROJECT_FB0)
#include "bqfs/FB0_BQFS.h"
#include "bqfs/FB3_BQFS.h"
#include "bqfs/FD1_BQFS.h"
#elif defined(CONFIG_FIH_PROJECT_SF4Y6)
#include "bqfs/SF6_BQFS.h"
#elif defined(CONFIG_FIH_PROJECT_SF4V5)
#include "bqfs/SF5_BQFS.h"
#elif defined(CONFIG_FIH_PROJECT_SF8)
#include "bqfs/SF8_BQFS.h"
#endif

#if defined(CONFIG_FIH_POWER_LOG)
#include "linux/pmlog.h"
#endif

#define ERASE_RETRIES           3
#define FAILED_MARK             0xFFFFFFFF

static struct i2c_client *bq275x0_client;
static struct proc_dir_entry *dfi_upgrade_proc_entry;

static int g_error_type = 0;
static int counter = 0;
static struct wake_lock bqfs_wakelock;
static struct delayed_work bqfs_upgrade;

extern int bq275x0_battery_fw_version(void);
extern int bq275x0_battery_device_type(void);
extern int bq275x0_battery_enter_rom_mode(void);
extern int bq275x0_battery_IT_enable(void);
extern int bq275x0_battery_reset(void);
extern int bq275x0_battery_sealed(void);
extern void bq275x0_battery_exit_rommode(void);

extern int msmrtc_timeremote_read_time(struct device *dev, struct rtc_time *tm);
extern void bq275x0_battery_ignore_battery_detection(bool ignore);

enum {
    ERR_SUCCESSFUL,
    ERR_READ_DFI,
    ERR_ENTER_ROM_MODE,
    ERR_READ_IF,
    ERR_ERASE_IF,
    ERR_MASS_ERASE,
    ERR_PROGRAM_DFI,
    ERR_CHECKSUM,
    ERR_PROGRAM_IF,
    ERR_IT_ENABLE,
    ERR_RESET,
    ERR_SEALED,
    ERR_DEVICE_TYPE_FW_VER,
    ERR_UNKNOWN_CMD,
};

static int bq275x0_RomMode_read(u8 cmd, u8 *data, int length)
{
    int ret;
    struct i2c_msg msgs[] = {
        [0] = {
            .addr   = bq275x0_client->addr,
            .flags  = 0,
            .buf    = (void *)&cmd,
            .len    = 1
        },
        [1] = {
            .addr   = bq275x0_client->addr,
            .flags  = I2C_M_RD,
            .buf    = (void *)data,
            .len    = length
        }
    };

    ret = (i2c_transfer(bq275x0_client->adapter, msgs, 2) < 0) ? -1 : 0;
    return ret;
}

static int bq275x0_RomMode_write(u8 *cmd, int length)
{
    int ret;
    struct i2c_msg msgs[] = {
        [0] = {
            .addr   = bq275x0_client->addr,
            .flags  = 0,
            .buf    = (void *)cmd,
            .len    = length
        },
    };

    ret = (i2c_transfer(bq275x0_client->adapter, msgs, 1) < 0) ? -1 : 0;
    mdelay(1);
    return ret;  
}

static int bq275x0_RomMode_erase_first_two_rows(void)
{
    u8 cmd[3];
    u8 buf;
    u8 retries = 0;
    
    do {
        retries++;
        
        cmd[0] = 0x00;
        cmd[1] = 0x03;
        bq275x0_RomMode_write(cmd, 2);
        cmd[0] = 0x64;
        cmd[1] = 0x03;  /* 0x64 */
        cmd[2] = 0x00;  /* 0x65 */
        bq275x0_RomMode_write(cmd, sizeof(cmd));
        mdelay(20);
        
        bq275x0_RomMode_read(0x66, &buf, sizeof(buf));
        if (buf == 0x00)
            return 0;
            
        if (retries > ERASE_RETRIES)
            break;
    } while (1);
    
    return -1;
}

static int bq275x0_RomMode_mass_erase(void)
{
    u8 cmd[3];
    u8 buf;
    u8 retries = 0;

    do {
        retries++;
        
        cmd[0] = 0x00;
        cmd[1] = 0x0C;
        bq275x0_RomMode_write(cmd, 2);
        cmd[0] = 0x04;
        cmd[1] = 0x83;  /* 0x04 */
        cmd[2] = 0xDE;  /* 0x05 */
        bq275x0_RomMode_write(cmd, sizeof(cmd));
        cmd[0] = 0x64;
        cmd[1] = 0x6D;  /* 0x64 */
        cmd[2] = 0x01;  /* 0x65 */
        bq275x0_RomMode_write(cmd, sizeof(cmd));
        mdelay(200);
    
        bq275x0_RomMode_read(0x66, &buf, sizeof(buf));
        if (buf == 0x00)
            return 0;
            
        if (retries > ERASE_RETRIES)
            break;
    } while (1);
    
    return -1;
}

static int BQFS_TOTAL_LINES = 0;
static unsigned char* bqfs_data;
static unsigned long bqfs_data_size = 0;
static void bq275x0_RomMode_get_bqfs(void)
{
#if defined(CONFIG_FIH_PROJECT_FB0)
    int product_id  = fih_get_product_id();
    if (product_id == Product_FB3) {
        bqfs_data = FB3_BQFS;
        bqfs_data_size = sizeof(FB3_BQFS);
        BQFS_TOTAL_LINES = FB3_BQFS_TOTAL_LINES;
    } else if (product_id == Product_FD1) {
        bqfs_data = FD1_BQFS;
        bqfs_data_size = sizeof(FD1_BQFS);
        BQFS_TOTAL_LINES = FD1_BQFS_TOTAL_LINES;
	}else {
        bqfs_data = FB0_BQFS;
        bqfs_data_size = sizeof(FB0_BQFS);
        BQFS_TOTAL_LINES = FB0_BQFS_TOTAL_LINES;
    }
#elif defined(CONFIG_FIH_PROJECT_SF4Y6)
    bqfs_data = SF6_BQFS;
    bqfs_data_size = sizeof(SF6_BQFS);
    BQFS_TOTAL_LINES = SF6_BQFS_TOTAL_LINES;
#elif defined(CONFIG_FIH_PROJECT_SF4V5)
    bqfs_data = SF5_BQFS;
    bqfs_data_size = sizeof(SF5_BQFS);
    BQFS_TOTAL_LINES = SF5_BQFS_TOTAL_LINES;
#elif defined(CONFIG_FIH_PROJECT_SF8)
    bqfs_data = SF8_BQFS;
    bqfs_data_size = sizeof(SF8_BQFS);
    BQFS_TOTAL_LINES = SF8_BQFS_TOTAL_LINES;
#endif    
}

void bq275x0_RomMode_get_bqfs_version(char* prj_name, int *version, u8* date)
{
    char BQFS_DATE[9];
    
#if defined(CONFIG_FIH_PROJECT_FB0)
    int product_id  = fih_get_product_id();
    if (product_id == Product_FB3) {
        strcpy(prj_name, FB3_BQFS_PROJECT_NAME);
        strcpy(BQFS_DATE, FB3_BQFS_DATE);
        *version = FB3_BQFS_VERSION;
    } else if (product_id == Product_FD1) {
        strcpy(prj_name, FD1_BQFS_PROJECT_NAME);
        strcpy(BQFS_DATE, FD1_BQFS_DATE);
        *version = FD1_BQFS_VERSION;		
	} else {
        strcpy(prj_name, FB0_BQFS_PROJECT_NAME);
        strcpy(BQFS_DATE, FB0_BQFS_DATE);
        *version = FB0_BQFS_VERSION;
    }
#elif defined(CONFIG_FIH_PROJECT_SF4Y6)
    strcpy(prj_name, SF6_BQFS_PROJECT_NAME);
    strcpy(BQFS_DATE, SF6_BQFS_DATE);
    *version = SF6_BQFS_VERSION;
#elif defined(CONFIG_FIH_PROJECT_SF4V5)
    strcpy(prj_name, SF5_BQFS_PROJECT_NAME);
    strcpy(BQFS_DATE, SF5_BQFS_DATE);
    *version = SF5_BQFS_VERSION;
#elif defined(CONFIG_FIH_PROJECT_SF8)
    strcpy(prj_name, SF8_BQFS_PROJECT_NAME);
    strcpy(BQFS_DATE, SF8_BQFS_DATE);
    *version = SF8_BQFS_VERSION;
#endif

    if (date != NULL)
        sscanf(BQFS_DATE, "%02x%02x%02x%02x",
                            (unsigned int*)&date[0],
                            (unsigned int*)&date[1],
                            (unsigned int*)&date[2],
                            (unsigned int*)&date[3]);
}
EXPORT_SYMBOL(bq275x0_RomMode_get_bqfs_version);

#define WRITE_BLOCK_SIZE 32
static int bq275x0_RomMode_program_bqfs(void)
{
    u8 cmd[33];
    u8 buf[96];
    u8 reg;
    int len = 0;
    int i = 0;
    //int j = 0;
    unsigned long idx = 0;
    unsigned long size = bqfs_data_size;
    union {
        u16 s;
        struct {
            u8 low;
            u8 high;
        } u;
    } delay;
    
    counter = 0;
    while (idx < size) {
        counter++;
        if (counter == 1 || counter % 200 == 0) {
            dev_info(&bq275x0_client->dev,"[%d].\n", counter);
#if defined(CONFIG_FIH_POWER_LOG)
            pmlog("BQFS Line[%d]\n", counter);
#endif
        }
        switch (bqfs_data[idx++]) {
        case 'W':
            //dev_info(&bq275x0_client->dev, "[%d] W ", counter);
            len = bqfs_data[idx++];
            reg = bqfs_data[idx++];
            memcpy(buf, &bqfs_data[idx], len - 1);
            idx = idx + (len - 1);
                
            for (i = 0; i < (len - 1) / WRITE_BLOCK_SIZE; i ++) {
                cmd[0] = reg + (i * WRITE_BLOCK_SIZE);
                //printk(KERN_INFO "reg: %02X\n", cmd[0]);
                memcpy(&cmd[1], &buf[i * WRITE_BLOCK_SIZE], WRITE_BLOCK_SIZE);
                if (bq275x0_RomMode_write(cmd, WRITE_BLOCK_SIZE + 1)) {
                    dev_err(&bq275x0_client->dev,"[%d] Write FAILED\n", counter);
#if defined(CONFIG_FIH_POWER_LOG)
                    pmlog("BQFS Line[%d] W Failed\n", counter);
#endif
                    return -1;
                }
                //for (j = 1; j <  WRITE_BLOCK_SIZE + 1; j ++)
                //    printk(KERN_INFO "[%d] %02X\n", j + i * WRITE_BLOCK_SIZE, cmd[j]);
            }
            
            if ((len - 1) % WRITE_BLOCK_SIZE) {
                cmd[0] = reg + (i * WRITE_BLOCK_SIZE);
                //printk(KERN_INFO "reg: %02X\n", cmd[0]);
                memcpy(&cmd[1], &buf[i * WRITE_BLOCK_SIZE], (len - 1) % WRITE_BLOCK_SIZE);
                if (bq275x0_RomMode_write(cmd, (len - 1) % WRITE_BLOCK_SIZE + 1)) {
                    dev_err(&bq275x0_client->dev,"[%d] Write FAILED\n", counter);
#if defined(CONFIG_FIH_POWER_LOG)
                    pmlog("BQFS Line[%d] W Failed\n", counter);
#endif
                    return -1;
                }
                //for (j = 1; j < (len - 1) % WRITE_BLOCK_SIZE + 1; j ++)
                    //printk(KERN_INFO "[%d] %02X\n", j + i * WRITE_BLOCK_SIZE, cmd[j]);
            }
            
            /*for (i = 0; i < (len - 1); i++) {
                cmd[0] = reg + i;
                cmd[1] = bqfs_data[idx++];
                if (bq275x0_RomMode_write(cmd, 2)) {
                    dev_err(&bq275x0_client->dev,"[%d] Write FAILED\n", counter);
#if defined(CONFIG_FIH_POWER_LOG)
                    pmlog("BQFS Line[%d] W Failed\n", counter);
#endif
                    return -1;
                }
                //dev_info(&bq275x0_client->dev, "[%d]reg: %02x data: %02x\n", i, cmd[0], cmd[1]);
            }*/
            break;
        case 'R':
            break;
        case 'C':
            //dev_info(&bq275x0_client->dev, "[%d] C ", counter);
			mdelay(16);
            len = bqfs_data[idx++];
            bq275x0_RomMode_read(bqfs_data[idx++], cmd, len - 1);
            //dev_info(&bq275x0_client->dev, "Reg: %02x\n", bqfs_data[idx]);
            for (i = 0; i < (len - 1); i++) {
                if (cmd[i] != bqfs_data[idx++]) {
                    dev_err(&bq275x0_client->dev, "[%d] Checksum FAILED\n", counter);
#if defined(CONFIG_FIH_POWER_LOG)
                    pmlog("BQFS Line[%d] C Failed\n", counter);
#endif 
                    return -1;
                } /*else {
                    dev_info(&bq275x0_client->dev, "[%d]data: %02x\n", i, bqfs_data[idx]);
                }*/
            }
            break;
        case 'X':
            //dev_info(&bq275x0_client->dev, "%d X ", counter);
            delay.u.low = bqfs_data[idx++];
            delay.u.high = bqfs_data[idx++];
            mdelay(delay.s);
            //dev_info(&bq275x0_client->dev, "delay %d ms\n", delay.s);
            break;
        }
    }
    
    dev_info(&bq275x0_client->dev,"[%d].\n", counter);
#if defined(CONFIG_FIH_POWER_LOG)
    pmlog("BQFS Line[%d]\n", counter);
#endif
    
    return 0;
}

static int bq275x0_RomMode_read_erase_if(void)
{
    int ret = 0;
    
    if (bq275x0_battery_enter_rom_mode()) {
        dev_err(&bq275x0_client->dev, "Entering rom mode was failed\n");
        ret = -ERR_ENTER_ROM_MODE; /* Rom Mode is locked, or IC is damaged */
    }
    mdelay(2); /* Gauge need time to enter ROM mode. */

    if (bq275x0_RomMode_erase_first_two_rows()) {
        dev_err(&bq275x0_client->dev, "Erasing IF first two rows was failed\n");
        if (ret)
            return ret;
 
        return -ERR_ERASE_IF;
    }
    
    dev_info(&bq275x0_client->dev, "Start to Writing Image\n");
    return 0;
}

static int bq275x0_RomMode_writing_image(void)
{
    if (bq275x0_RomMode_mass_erase()) {
        dev_err(&bq275x0_client->dev, "Mass Erase procedure was failed\n");
        return -ERR_MASS_ERASE;
    }

    if (bq275x0_RomMode_program_bqfs()) {
        dev_err(&bq275x0_client->dev, "BQFS programming was failed\n");
        return -ERR_PROGRAM_DFI;
    }

    dev_info(&bq275x0_client->dev, "End of Writing Image\n");
    return 0;
}

enum {
    PROGRAM_READ_ERASE_IF,
    PROGRAM_WRITING_DFI,
    PROGRAM_IT_ENABLE,
    PROGRAM_RESET,
    PROGRAM_SEALED,
};

static ssize_t bq275x0_RomMode_programming_store(struct device *dev, 
        struct device_attribute *attr, const char *buf, size_t count)
{  
    int step    = 0;
    int cmd     = 0;
    int ret     = 0;
    
    /*
     * cmd:     Assign first command.
     * step:    Assign how many commands we want to execute since first
     *          command.
     * 
     * If PROGRAM_WRITING_DFI command is failed, IF rows should not be 
     * reporgrammed. If IF rows are empty, the IC will always enter ROM 
     * Mode.
     * 
     * PROGRAM_RESET command is strongly suggested to execute after
     * PROGRAM_IT_ENABLE command is executed.
     * 
     * PROGRAM_SEALED command need other command to execute it. This way
     * is to prevent from entering SEALED mode unexpectedly.
     */
    wake_lock(&bqfs_wakelock);
    sscanf(buf, "%d %d", &cmd, &step);
    switch(cmd) {
    case PROGRAM_READ_ERASE_IF: /* step 4 */
        if (step)
            step--;
        else
            break;
    
        dev_info(&bq275x0_client->dev, "%s: step %d\n", __func__, step);
    
        if ((g_error_type = bq275x0_RomMode_read_erase_if())) {
            bq275x0_battery_exit_rommode();
#if defined(CONFIG_FIH_POWER_LOG)
            pmlog("ERASE IF[FAILED]\n");
#endif
            break;
        }
#if defined(CONFIG_FIH_POWER_LOG)
        pmlog("ERASE IF[SUCCESSFUL]\n");
#endif
    case PROGRAM_WRITING_DFI:   /* step 3 */
        if (step) {
            step--;
        } else 
            break;
            
        dev_info(&bq275x0_client->dev, "%s: step %d\n", __func__, step);

        if ((g_error_type = bq275x0_RomMode_writing_image())) {
            bq275x0_battery_exit_rommode();
#if defined(CONFIG_FIH_POWER_LOG)
            pmlog("WRITING BQFS[FAILED]\n");
#endif
            break;
        }
#if defined(CONFIG_FIH_POWER_LOG)
        pmlog("WRITING BQFS[SUCCESSFUL]\n");
#endif
    case PROGRAM_IT_ENABLE:     /* step 2 */
        if (step > 0) {
            if (step != 1)
                step--;
        } else
            break;
            
        dev_info(&bq275x0_client->dev, "%s: step %d\n", __func__, step);
        if ((ret = bq275x0_battery_IT_enable())) {
            if (ret)
                g_error_type = -ERR_IT_ENABLE;
            bq275x0_battery_exit_rommode();
#if defined(CONFIG_FIH_POWER_LOG)
            pmlog("IT ENABLE[FAILED]\n");
#endif
            break;
        }
#if defined(CONFIG_FIH_POWER_LOG)
        pmlog("IT ENABLE[SUCCESSFUL]\n");
#endif

        counter = BQFS_TOTAL_LINES + 1;
    case PROGRAM_RESET:         /* step 1 */
        if (step)
            step--;
        else
            break;
            
        dev_info(&bq275x0_client->dev, "%s: step %d\n", __func__, step);

        if ((ret = bq275x0_battery_reset())) {
            if (ret)
                g_error_type = -ERR_RESET;
            bq275x0_battery_exit_rommode();
#if defined(CONFIG_FIH_POWER_LOG)
            pmlog("RESET[FAILED]\n");
#endif
            break;
        }
        
        bq275x0_battery_exit_rommode();
#if defined(CONFIG_FIH_POWER_LOG)
        pmlog("RESET[SUCCESSFUL]\n");
#endif

        counter = BQFS_TOTAL_LINES + 2;
        break;
    case PROGRAM_SEALED:
        /*dev_info(&bq275x0_client->dev, "%s: step %d\n", __func__, step);
        if (bq275x0_battery_sealed()) {
            g_error_type = -ERR_SEALED;
        }*/
        break;
    default:
        g_error_type = -ERR_UNKNOWN_CMD;
    };
    wake_unlock(&bqfs_wakelock);

    return count;
}

static ssize_t bq275x0_RomMode_programming_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
    return 0;
}
static DEVICE_ATTR(programming, 0644, bq275x0_RomMode_programming_show, bq275x0_RomMode_programming_store);

static char cmd[256];
static int
proc_calc_metrics(char *page, char **start, off_t off,
                 int count, int *eof, int len)
{
    if (len <= off+count) *eof = 1;
    *start = page + off;
    len -= off;
    if (len > count) len = count;
    if (len < 0) len = 0;
    return len;
}

static int
bq275x0_dfi_upgrade_proc_read(char *page, char **start, off_t off,
              int count, int *eof, void *data)
{
    int len;
    
    //dev_info(&bq275x0_client->dev, "procfile_read (/proc/dfi_upgrade) called\n");
    //len = snprintf(page, PAGE_SIZE, "cmd: %s", cmd);
    if (g_error_type < 0)
        len = snprintf(page, PAGE_SIZE, "%d", g_error_type);
    else {
        len = snprintf(page, PAGE_SIZE, "%d", counter);
        if (counter == BQFS_TOTAL_LINES)
            counter = 0;
    }
        
    return proc_calc_metrics(page, start, off, count, eof, len);
}

static int
bq275x0_dfi_upgrade_proc_write(struct file *file, const char __user *buffer,
               unsigned long count, void *data)
{
    dev_info(&bq275x0_client->dev, "procfile_write (/proc/dfi_upgrade) called\n");
    
    if (copy_from_user(cmd, buffer, count)) {
        return -EFAULT;
    } else {
        if (!strncmp(cmd, "flash22685511@FIHLX\n", count)) {
#if defined(CONFIG_FIH_POWER_LOG)
            pmlog("[START]Flash BQFS.\n");
#endif
            bq275x0_RomMode_get_bqfs();
            bq275x0_battery_ignore_battery_detection(true);
            g_error_type = 0;
            counter = BQFS_TOTAL_LINES;
            schedule_delayed_work(&bqfs_upgrade, msecs_to_jiffies(30));
        }
    }
        
    return count;
}

static void bq275x0_RomMode_upgrade(struct work_struct *work)
{
        bq275x0_RomMode_programming_store(NULL, NULL, "0 4", 4);
        bq275x0_battery_ignore_battery_detection(false);
#if defined(CONFIG_FIH_POWER_LOG)
        pmlog("[END]Flash BQFS.\n");
#endif
}

static int bq275x0_RomMode_probe(struct i2c_client *client,
                 const struct i2c_device_id *id)
{
    int retval = 0;

    bq275x0_client = client;
    retval = device_create_file(&client->dev, &dev_attr_programming);
    if (retval) {
        dev_err(&client->dev,
               "%s: dev_attr_test failed\n", __func__);
    }

    wake_lock_init(&bqfs_wakelock, WAKE_LOCK_SUSPEND, "bq275x0_bqfs");
    INIT_DELAYED_WORK(&bqfs_upgrade, bq275x0_RomMode_upgrade);
    dfi_upgrade_proc_entry = create_proc_entry("dfi_upgrade", 0666, NULL);
    dfi_upgrade_proc_entry->read_proc   = bq275x0_dfi_upgrade_proc_read;
    dfi_upgrade_proc_entry->write_proc  = bq275x0_dfi_upgrade_proc_write;

    dev_info(&client->dev, "%s finished\n", __func__);

    return retval;
}

static int bq275x0_RomMode_remove(struct i2c_client *client)
{
    return 0;
}

/*
 * Module stuff
 */
static const struct i2c_device_id bq275x0_RomMode_id[] = {
    { "bq275x0-RomMode", 0 },
    {},
};

static struct i2c_driver bq275x0_RomMode_driver = {
    .driver = {
        .name = "bq275x0-RomMode",
    },
    .probe = bq275x0_RomMode_probe,
    .remove = bq275x0_RomMode_remove,
    .id_table = bq275x0_RomMode_id,
};

static int __init bq275x0_RomMode_init(void)
{
    int ret;

    ret = i2c_add_driver(&bq275x0_RomMode_driver);
    if (ret)
        printk(KERN_ERR "Unable to register bq275x0 RomMode driver\n");

    return ret;
}
module_init(bq275x0_RomMode_init);

static void __exit bq275x0_RomMode_exit(void)
{
    i2c_del_driver(&bq275x0_RomMode_driver);
}
module_exit(bq275x0_RomMode_exit);

MODULE_AUTHOR("Audi PC Huang <AudiPCHuang@fihtdc.com>");
MODULE_DESCRIPTION("bq275x0 dfi programming driver");
MODULE_LICENSE("GPL");
