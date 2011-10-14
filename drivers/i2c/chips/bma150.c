#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/earlysuspend.h> 


#define DEBUG 0 //DIV5-BSP-CH-SF6-SENSOR-PORTING01++

//Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00+{
#define USE_SUSPEND 
#define I2C_RETRY_ALLOW_TIME 12
static atomic_t is_early_suspend;
static atomic_t i2c_failed_counter; 
//Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00+}

#if DEBUG
    #define DBG_MSG(msg, ...) printk("[BMA150_DBG] %s : " msg "\n", __FUNCTION__, ## __VA_ARGS__)
#else
    #define DBG_MSG(...) 
#endif

#define INF_MSG(msg, ...) printk("[BMA150_INF] %s : " msg "\n", __FUNCTION__, ## __VA_ARGS__)
#define ERR_MSG(msg, ...) printk("[BMA150_ERR] %s : " msg "\n", __FUNCTION__, ## __VA_ARGS__)

#define BMA150_I2C_SLAVE_ADDRESS         0x38

#define BMA150_MODE_NORMAL	0
#define BMA150_MODE_SLEEP	2

#define BMA150_CONF2_REG	0x15
#define BMA150_CTRL_REG		0x0a

#define WAKE_UP_REG		BMA150_CONF2_REG
#define WAKE_UP_POS		0
#define WAKE_UP_MSK		0x01

#define SLEEP_REG		BMA150_CTRL_REG
#define SLEEP_POS		0
#define SLEEP_MSK		0x01 

#define RETRY_TIME 3 //i2c transfer retry time

#define BMA150_SET_BITSLICE(regvar, bitname, val) \
    (regvar & ~bitname##_MSK) | ((val<<bitname##_POS)&bitname##_MSK)

static struct i2c_client *bma150_i2c_client = NULL;
static atomic_t dev_opened;

static int enter_mode(unsigned char mode);


//Div6D1-OH-GSensor-APP_Calibration-00+{
static int bma150_calibration_status = 0;
static ssize_t show_gsensorcalibration_state(struct device *dev, struct device_attribute *attr, char *buf)
{
    DBG_MSG("START");
	
    return sprintf(buf, "%d\n", bma150_calibration_status);
}

static ssize_t set_gsensorcalibration_state(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    DBG_MSG("START");
    
    sscanf(buf, "%d\n", &bma150_calibration_status);
    
    return sizeof(bma150_calibration_status);
}
static DEVICE_ATTR(gsensorcalibration_state, 0666, show_gsensorcalibration_state, set_gsensorcalibration_state);
//Div6D1-OH-GSensor-APP_Calibration-00+}


#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend bma150_early_suspend;
static void bma150_early_suspend_func(struct early_suspend * h)
{
//Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00*{
#ifndef USE_SUSPEND
    INF_MSG("START");
    
    if (atomic_read(&dev_opened) > 0)
    {
        if (enter_mode(BMA150_MODE_SLEEP))
        {
            ERR_MSG("Gsensor Enter sleep mode fail");
        }
    }
    atomic_inc(&is_early_suspend);
#else
	
#endif
//Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00*}
}

static void bma150_late_resume_func(struct early_suspend *h)
{
//Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00*{
#ifndef USE_SUSPEND
    INF_MSG("START");
    
    if (atomic_read(&dev_opened) > 0)
    {
        if (enter_mode(BMA150_MODE_NORMAL))
        {
            ERR_MSG("Gsensor Enter sleep mode fail");
        }
    }
    atomic_dec(&is_early_suspend);
    atomic_set(&i2c_failed_counter, 0); //reset failed //owenhuang
#else
    atomic_set(&i2c_failed_counter, 0); //reset failed //owenhuang
#endif
//Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00*}
}
#endif

static int bma150_i2c_tx(char *buf, int length)
{
    int ret;
    int retry;
    
    DBG_MSG("START");
    
    for (retry = 0; retry < RETRY_TIME; retry++)
    {
        ret = i2c_master_send(bma150_i2c_client, buf, length);
        if (ret == length)
        {
            DBG_MSG("i2c transfer success");	
            return ret;
        }
    }
    
    atomic_inc(&i2c_failed_counter); //Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00+
    
    ERR_MSG("i2c transfer failed");
    return (-EIO);
}

static int bma150_i2c_rx(char *buf, int length)
{
    int ret;
    int retry;
    
    DBG_MSG("START");
    
    for (retry = 0; retry < RETRY_TIME; retry++)
    {
        ret = i2c_master_recv(bma150_i2c_client, buf, length);
        if (ret == length)
        {
            DBG_MSG("i2c transfer success");	
            return ret;
        }
    }
    
    atomic_inc(&i2c_failed_counter); //Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00+
    
    ERR_MSG("i2c transfer failed");
    return (-EIO);
}



// ************************************************************************
// Gsensor Code -- BMA150
// ************************************************************************
static int bma150_read_reg(struct i2c_client *clnt, unsigned char reg, unsigned char *data, unsigned char count)
{
    unsigned char tmp[10];
	
    if (10 < count)
        return -1;
	
    tmp[0] = reg;
    if (bma150_i2c_tx(tmp, 1) < 0)
    {
        ERR_MSG("Set REGISTER address error");
        return -EIO;
    }

    if (bma150_i2c_rx(tmp, count) < 0)
    {
        ERR_MSG("Read REGISTER content error");
        return -EIO;
    }

    strncpy(data, tmp, count);

    return 0;
}

static int bma150_write_reg(struct i2c_client *clnt, unsigned char reg, unsigned char *data, unsigned char count)
{
    unsigned char tmp[2];

    while(count)
    {
        tmp[0] = reg++;
        tmp[1] = *(data++);

        if (bma150_i2c_tx(tmp, 2) < 0)
        {
            ERR_MSG("ERROR");
            return -EIO;
        }

        count--;
    }

    return 0;
}

static int enter_mode(unsigned char mode)
{
    unsigned char data1, data2;
    int ret;

    DBG_MSG("Mode = %d, (0 = Normal, 2 = Sleep)", mode);

    if((mode==BMA150_MODE_NORMAL) || (mode==BMA150_MODE_SLEEP)) 
    {
        ret = bma150_read_reg(bma150_i2c_client, WAKE_UP_REG, &data1, 1);
        if (ret)
        {
            ERR_MSG("Read WAKE UP REG fail, ret = %d", ret);
            return ret;
        }

        DBG_MSG("WAKE_UP_REG (0x%x)", data1);
        data1 = BMA150_SET_BITSLICE(data1, WAKE_UP, mode);

        ret = bma150_read_reg(bma150_i2c_client, SLEEP_REG, &data2, 1);
        if (ret)
        {
            ERR_MSG("Read SLEEP REG fail, ret = %d", ret);
            return ret;
        }

        DBG_MSG("SLEEP_REG (0x%x)", data2);		
        data2 = BMA150_SET_BITSLICE(data2, SLEEP, (mode>>1));

        ret = bma150_write_reg(bma150_i2c_client, WAKE_UP_REG, &data1, 1);
        if (ret)
        {
            ERR_MSG("Write WAKE UP REG fail, ret = %d", ret);
            return ret;
        }

        ret = bma150_write_reg(bma150_i2c_client, SLEEP_REG, &data2, 1);
        if (ret)
        {
            ERR_MSG("Write SLEEP REG fail, ret = %d", ret);
            return ret;
        }
    }

    return 0;
}

// ************************************************************************
// BOSCH BMA150 Ioctl interface for MS3C HAL Layer
// ************************************************************************
static int bma150_open(struct inode *inode, struct file *file)
{
    INF_MSG("START");

    if (atomic_read(&dev_opened) == 0)
    {
        if (enter_mode(BMA150_MODE_NORMAL)) 
        {
            ERR_MSG("Gsensor Enter normal mode fail");
            return -EIO;
        }
    }

    atomic_inc(&dev_opened);
    return 0;
}

static int bma150_release(struct inode *inode, struct file *file)
{
    INF_MSG("START");

    if (atomic_read(&dev_opened) == 0)
        return 0;

    atomic_dec(&dev_opened);

    if (atomic_read(&dev_opened) == 0)
    {
        if(enter_mode(BMA150_MODE_SLEEP)) 
        {
            ERR_MSG("Gsensor Enter sleep mode fail");
            return -EIO;
        }
    }

    return 0;
}

static int bma150_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
        default:
            DBG_MSG("BMA Ioctl value is undefine");
            break;
    }

    return 0;
}

static ssize_t bma150_read(struct file *fp, char __user *buf, size_t count, loff_t *pos)
{
    int ret = 0;
    char tmp[64];

//Div6D1-OH-GSensor-APP_Calibration-00+{
#ifndef USE_SUSPEND
    if (atomic_read(&is_early_suspend) == 0 && atomic_read(&i2c_failed_counter) < I2C_RETRY_ALLOW_TIME)
    {
        ret = bma150_i2c_rx(tmp, count);
        if (ret < 0)
        {
            ERR_MSG("i2c recv failed");
            return -EIO;
        }
        
        if (copy_to_user(buf, tmp, ret))
        {
            ERR_MSG("copy data to user space fail");
            ret = -EFAULT;
        }
        
        DBG_MSG("read result: (%d, %d)", count, ret);
    }
#else
    if (atomic_read(&i2c_failed_counter) < I2C_RETRY_ALLOW_TIME)
    {
        ret = bma150_i2c_rx(tmp, count);
        if (ret < 0)
        {
            ERR_MSG("i2c recv failed");
            return -EIO;
        }
        
        if (copy_to_user(buf, tmp, ret))
        {
            ERR_MSG("copy data to user space fail");
            ret = -EFAULT;
        }
    
        DBG_MSG("read result: (%d, %d)", count, ret);
    }
#endif
    else
    {	
        ERR_MSG("Enter early suspend or i2c transfer error counter bigger than %d", I2C_RETRY_ALLOW_TIME);
        ret = count;
        memset(tmp, 0, sizeof(tmp));
        if (copy_to_user(buf, tmp, ret))
        {
            ERR_MSG("copy data to user space fail");
            ret = -EFAULT;
        }
    }
	
    return ret;
//Div6D1-OH-GSensor-APP_Calibration-00+}
}
                     
static ssize_t bma150_write(struct file *fp, const char __user *buf, size_t count, loff_t *pos)
{
    int ret = 0;
    char tmp[64];

//Div6D1-OH-GSensor-APP_Calibration-00+{
#ifndef USE_SUSPEND
    if (atomic_read(&is_early_suspend) == 0 && atomic_read(&i2c_failed_counter) < I2C_RETRY_ALLOW_TIME)
    {
        if (copy_from_user(tmp, buf, count))
        {
            ERR_MSG("copy data from user space fail");
            return -EFAULT;
        }
        
        ret = bma150_i2c_tx(tmp, count);
        if (ret < 0)
        {
            ERR_MSG("i2c send failed");
            return -EIO;
        }
    
        DBG_MSG("write result: (%d, %d)", count, ret);
    }
#else
    if (atomic_read(&i2c_failed_counter) < I2C_RETRY_ALLOW_TIME)
    {
        if (copy_from_user(tmp, buf, count))
        {
            ERR_MSG("copy data from user space fail");
            return -EFAULT;
        }
        
        ret = bma150_i2c_tx(tmp, count);
        if (ret < 0)
        {
            ERR_MSG("i2c send failed");
            return -EIO;
        }
        
    DBG_MSG("write result: (%d, %d)", count, ret);
    }
#endif
    else
    {
        ERR_MSG("Enter early suspend or i2c transfer error counter bigger than %d", I2C_RETRY_ALLOW_TIME);
        ret = count;
    }

    return ret;
//Div6D1-OH-GSensor-APP_Calibration-00+}
} 
 
static struct file_operations bma150_fops = {
    .owner    = THIS_MODULE,
    .open     = bma150_open,
    .release  = bma150_release,
    .ioctl    = bma150_ioctl,
    .read     = bma150_read,
    .write    = bma150_write,
};

static struct miscdevice bma150_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "bma150",
    .fops = &bma150_fops,
};

static int bma150_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;
	
    DBG_MSG("START");

    bma150_i2c_client = client;

    //Div6D1-OH-GSensor-APP_Calibration-00+{
    if (device_create_file(&client->dev, &dev_attr_gsensorcalibration_state))
    {
        ERR_MSG("device_create_file!");
        return -EFAULT;
    }
    //Div6D1-OH-GSensor-APP_Calibration-00+}
	
    if (enter_mode(BMA150_MODE_SLEEP))
    {
        ERR_MSG("Enter Sleep mode fail");
        return -EFAULT;
    }

    ret = misc_register(&bma150_device);
    if (ret) 
    {
        ERR_MSG("BOSCH BMA misc register failed");
        return ret;
    }

    //initial dev_opened counter
    atomic_set(&dev_opened, 0);
    atomic_set(&is_early_suspend, 0); //Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00+
    atomic_set(&i2c_failed_counter, 0); //Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00+

#ifdef CONFIG_HAS_EARLYSUSPEND  
    bma150_early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 10;
    bma150_early_suspend.suspend = bma150_early_suspend_func;
    bma150_early_suspend.resume = bma150_late_resume_func;
    register_early_suspend(&bma150_early_suspend);
#endif

    return 0;
}

static int bma150_remove(struct i2c_client *client)
{
    DBG_MSG("START");

    misc_deregister(&bma150_device);		
    bma150_i2c_client = 0;
    unregister_early_suspend(&bma150_early_suspend);
    device_remove_file(&client->dev, &dev_attr_gsensorcalibration_state); //Div6D1-OH-GSensor-APP_Calibration-00+

    return 0;
}

static int bma150_suspend(struct i2c_client *client, pm_message_t mesg)
{
    INF_MSG("START");

    //Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00+{
#ifdef USE_SUSPEND
    if (atomic_read(&dev_opened) > 0)
    {
        if (enter_mode(BMA150_MODE_SLEEP))
        {
            ERR_MSG("Gsensor Enter sleep mode fail");
        }
    }
#endif
    //Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00+}

    return 0;
}

static int bma150_resume(struct i2c_client *client)
{
    INF_MSG("START");

    //Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00+{
#ifdef USE_SUSPEND
    if (atomic_read(&dev_opened) > 0)
    {
        if (enter_mode(BMA150_MODE_NORMAL))
        {
            ERR_MSG("Gsensor Enter sleep mode fail");
        }
    }
#endif
    //Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00+}
	
    return 0;
}

static const struct i2c_device_id bma150_id[] = {
    { "BMA150", 0 },
    { }
};

static struct i2c_driver bma150_driver = {
    .probe    = bma150_probe,
    .remove   = bma150_remove,
    .suspend  = bma150_suspend,
    .resume   = bma150_resume,
    .id_table = bma150_id,
    .driver   = {
                    .name = "BMA150",
                },
};

static int __init bma150_init(void)
{
    int ret;

    INF_MSG("START");

    ret = i2c_add_driver(&bma150_driver);
    if (ret)
    {
        ERR_MSG("BMA150 driver init fail");
        return ret;
    }

    INF_MSG("FIH Gsensor driver init success");
	
    return ret;
}

static void __exit bma150_exit(void)
{
    i2c_del_driver(&bma150_driver);
}

module_init(bma150_init);
module_exit(bma150_exit);

MODULE_DESCRIPTION("BOSCH BMA150 driver");
MODULE_LICENSE("GPL");
