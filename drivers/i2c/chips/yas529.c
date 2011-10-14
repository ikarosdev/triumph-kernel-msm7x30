#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <mach/gpio.h>
#include <linux/earlysuspend.h> 

//Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00+{
#define USE_SUSPEND 
#define I2C_RETRY_ALLOW_TIME 12
static atomic_t is_early_suspend;
static atomic_t i2c_failed_counter; 
//Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00+}
#define DEBUG 0

#if DEBUG
    #define DBG_MSG(msg, ...) printk("[YAS529_DBG] %s : " msg "\n", __FUNCTION__, ## __VA_ARGS__)
#else
    #define DBG_MSG(...) 
#endif

#define INF_MSG(msg, ...) printk("[YAS529_INF] %s : " msg "\n", __FUNCTION__, ## __VA_ARGS__)
#define ERR_MSG(msg, ...) printk("[YAS529_ERR] %s : " msg "\n", __FUNCTION__, ## __VA_ARGS__)

#define POWER_OFF    0 
#define POWER_ON     1

#define RETRY_TIME   3 //retry time

//yas529 register definition
#define MS3CDRV_RESEL_INITCOIL 0xd0

#define MS3CDRV_CHIP_ID        0x40 //yas529 chip ID
#define YAS529_I2C_SLAVE_ADDRESS         0x2e

//Div2D5-OwenHuang-ALSPS-CM3623_FTM_Porting-01*{
#if defined(CONFIG_FIH_PROJECT_SF4V5)
#define YAS529_GPIO_RST_N                82 //for SF5 project
#elif defined(CONFIG_FIH_PROJECT_SF8)
#define YAS529_GPIO_RST_N                82 //for SF8 project
#else
#define YAS529_GPIO_RST_N                136 //for others
#endif
//Div2D5-OwenHuang-ALSPS-CM3623_FTM_Porting-01*}

static struct i2c_client *yas529_i2c_client = NULL;
static atomic_t dev_opened;

#ifdef CONFIG_HAS_EARLYSUSPEND
static struct early_suspend yas529_early_suspend;
static void yas529_early_suspend_func(struct early_suspend * h)
{
//Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00*{
#ifndef USE_SUSPEND
	INF_MSG("START");
	
	if (atomic_read(&dev_opened) > 0)
		gpio_set_value(YAS529_GPIO_RST_N , POWER_OFF);

	atomic_inc(&is_early_suspend);
#else
	
#endif
//Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00*}
}

static void yas529_late_resume_func(struct early_suspend *h)
{
//Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00*{
#ifndef USE_SUSPEND
	INF_MSG("START");
	
	if (atomic_read(&dev_opened) > 0)
		gpio_set_value(YAS529_GPIO_RST_N , POWER_ON);

	atomic_dec(&is_early_suspend);
	atomic_set(&i2c_failed_counter, 0); //reset failed 
#else
	atomic_set(&i2c_failed_counter, 0); //reset failed 
#endif
//Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00*}
}
#endif

static int yas529_i2c_tx(char *buf, int length)
{
	int ret;
	int retry;
	
	DBG_MSG("START");
	
	for(retry = 0; retry < RETRY_TIME; retry++)
	{
		ret = i2c_master_send(yas529_i2c_client, buf, length);
		if (ret == length)
		{
			DBG_MSG("TRANSFER SUCCESS");
			return ret; //successful
		}
	}

	atomic_inc(&i2c_failed_counter);//Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00+
	
	ERR_MSG("I2C transfer failed!");
	return (-EIO); //failed
}
static int yas529_i2c_rx(char *buf, int length)
{
	int ret;
	int retry;
	
	DBG_MSG("START");
	
	for(retry = 0; retry < RETRY_TIME; retry++)
	{
		ret = i2c_master_recv(yas529_i2c_client, buf, length);
		if (ret == length)
		{
			DBG_MSG("TRANSFER SUCCESS");
			return ret; //successful
		}
	}
	
	atomic_inc(&i2c_failed_counter); //Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00+
	
	ERR_MSG("I2C transfer failed!");
	return (-EIO); //failed
}

static int yas529_chip_init(void)
{
	char buf[10];
	int ret;
	INF_MSG("START");
	 
	//TODO: test device on i2c bus
	buf[0] = MS3CDRV_RESEL_INITCOIL;
	ret = yas529_i2c_tx(buf, 1);
	if (ret < 0)
	{
		ERR_MSG("read chip id failed(1)");
		return -EIO;
	}

	ret = yas529_i2c_rx(buf, 2);
	if (ret < 0)
	{
		ERR_MSG("read chip id failed(2)");
		return -EIO;
	}

	if (buf[1] != MS3CDRV_CHIP_ID)
	{
		ERR_MSG("chip id is not correct");
		return -EFAULT;
	}
	 	
	return 0;
}

// ************************************************************************
// YAMAHA YAS529 Ioctl interface for MS3C HAL Layer
// ************************************************************************
static int yas529_open(struct inode *inode, struct file *file)
{
    INF_MSG("START");
		
	if (atomic_read(&dev_opened) == 0)
	{
		gpio_set_value(YAS529_GPIO_RST_N, POWER_ON);
	}
	atomic_inc(&dev_opened);
 
    return 0;
}

static int yas529_release(struct inode *inode, struct file *file)
{
    INF_MSG("START");

	if (atomic_read(&dev_opened) > 0) 
		atomic_dec(&dev_opened);
	
	//power off /dev/yas529
	if (atomic_read(&dev_opened) == 0)
	{
		gpio_set_value(YAS529_GPIO_RST_N, POWER_OFF); 
	}	
  
    return 0;
}

static int yas529_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) 
	{
        default:
        	DBG_MSG("YAS529 Ioctl value is undefine");
        	break;
    }
    return 0;
}

static ssize_t yas529_read(struct file *fp, char __user *buf, size_t count, loff_t *pos)
{
    int ret = 0;
    char tmp[64];

//Div6D1-OH-GSensor-APP_Calibration-00+{
#ifndef USE_SUSPEND
	if (atomic_read(&is_early_suspend) == 0 && atomic_read(&i2c_failed_counter) < I2C_RETRY_ALLOW_TIME)
	{
		ret = yas529_i2c_rx(tmp, count);
		if (ret < 0)
		{
			ERR_MSG("read data failed, ret = %d", ret);
			return -EIO;
		}

    	if (copy_to_user(buf, tmp, ret))
    	{
    		ERR_MSG("copy data to user space failed!");
        	ret = -EFAULT;
    	}

    	DBG_MSG("result: (%d, %d)", count, ret);
	}
#else
	if (atomic_read(&i2c_failed_counter) < I2C_RETRY_ALLOW_TIME)
	{
		ret = yas529_i2c_rx(tmp, count);
		if (ret < 0)
		{
			ERR_MSG("read data failed, ret = %d", ret);
			return -EIO;
		}

    	if (copy_to_user(buf, tmp, ret))
    	{
    		ERR_MSG("copy data to user space failed!");
        	ret = -EFAULT;
    	}

    	DBG_MSG("result: (%d, %d)", count, ret);
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
//Div6D1-OH-GSensor-APP_Calibration-00+}	
    return ret;
}
                     
static ssize_t yas529_write(struct file *fp, const char __user *buf, size_t count, loff_t *pos)
{
    int ret = 0;
    char tmp[64];

//Div6D1-OH-GSensor-APP_Calibration-00+{
#ifndef USE_SUSPEND
	if (atomic_read(&is_early_suspend) == 0 && atomic_read(&i2c_failed_counter) < I2C_RETRY_ALLOW_TIME)
	{	
    	if (copy_from_user(tmp, buf, count))
    	{
    		ERR_MSG("copy data from user space failed!");
        	return -EFAULT;
    	}
	
    	ret = yas529_i2c_tx(tmp, count);
    	if (ret < 0)
    	{
    		ERR_MSG("write data failed, ret = %d", ret);
    		ret = -EIO;
    	}

    	DBG_MSG("write result: (%d, %d)", count, ret);
	}
#else
	if (atomic_read(&i2c_failed_counter) < I2C_RETRY_ALLOW_TIME)
	{	
    	if (copy_from_user(tmp, buf, count))
    	{
    		ERR_MSG("copy data from user space failed!");
        	return -EFAULT;
    	}
	
    	ret = yas529_i2c_tx(tmp, count);
    	if (ret < 0)
    	{
    		ERR_MSG("write data failed, ret = %d", ret);
    		ret = -EIO;
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
 
static struct file_operations yas529_fops = {
    .owner    = THIS_MODULE,
    .open     = yas529_open,
    .release  = yas529_release,
    .ioctl    = yas529_ioctl,
    .read     = yas529_read,
    .write    = yas529_write,
};

static struct miscdevice yas529_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "compass", 
    .fops = &yas529_fops,
};

// ************************************************************************
// Ecompass Code -- YAS529
// ************************************************************************
static int yas529_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	
	INF_MSG("START");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
	{
		ret = -ENODEV;
		ERR_MSG("i2c_check_functionality failed!");
		goto exit_check_i2c_functionality_failed;
	}
	
    yas529_i2c_client = client;

	gpio_tlmm_config(GPIO_CFG(YAS529_GPIO_RST_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	ret = gpio_request(YAS529_GPIO_RST_N, "YAS_reset_pin");
	if (ret)
	{
		ERR_MSG("ecompass reset pin request failed");
		goto exit_request_gpio_failed;
	}
	
    gpio_set_value(YAS529_GPIO_RST_N, POWER_ON);
	
    ret = yas529_chip_init();
    if (ret)
    {
    	ERR_MSG("test i2c exist failed!");
    	goto exit_yas529_init_failed;
    }

    gpio_set_value(YAS529_GPIO_RST_N, POWER_OFF);
            
    //register misc device "/dev/yas529"
    ret = misc_register(&yas529_device);
    if (ret) 
    {
        ERR_MSG("YAMAHA YAS529 misc register failed");
        goto exit_yas529_misc_register_failed;
    }
    
    //initial dev_opened counter
    atomic_set(&dev_opened, 0);
	atomic_set(&is_early_suspend, 0); //Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00+
	atomic_set(&i2c_failed_counter, 0); //Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00+
  
#ifdef CONFIG_HAS_EARLYSUSPEND  
    yas529_early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 10;
	yas529_early_suspend.suspend = yas529_early_suspend_func;
	yas529_early_suspend.resume = yas529_late_resume_func;
	register_early_suspend(&yas529_early_suspend);
#endif
    
    return 0;
   
exit_yas529_misc_register_failed:
exit_yas529_init_failed:
exit_request_gpio_failed:
exit_check_i2c_functionality_failed:
	return ret;
}

static int yas529_remove(struct i2c_client *client)
{
    DBG_MSG("START");  
    
	misc_deregister(&yas529_device);		
	yas529_i2c_client = 0;
	unregister_early_suspend(&yas529_early_suspend);
	
    gpio_set_value(YAS529_GPIO_RST_N, POWER_OFF);

    return 0;
}

static int yas529_suspend(struct i2c_client *client, pm_message_t mesg)
{
    INF_MSG("START");  

//Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00+{
#ifdef USE_SUSPEND
	if (atomic_read(&dev_opened) > 0)
		gpio_set_value(YAS529_GPIO_RST_N , POWER_OFF); 
#endif
//Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00+}
	
    return 0;
}

static int yas529_resume(struct i2c_client *client)
{
    INF_MSG("START");

//Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00+{
#ifdef USE_SUSPEND
	if (atomic_read(&dev_opened) > 0)
		gpio_set_value(YAS529_GPIO_RST_N , POWER_ON);
#endif
//Div6D1-OH-ECompass-EarlySuspend_To_Suspend-00+}

    return 0;
}

static const struct i2c_device_id yas529_id[] = {
    { "YAS529", 0 },
    { }
};

static struct i2c_driver yas529_driver = {
    .probe    = yas529_probe,
    .remove   = yas529_remove,
    .suspend  = yas529_suspend,
    .resume   = yas529_resume,
    .id_table = yas529_id,
    .driver = {
                  .name = "YAS529",
              },
};

static int __init yas529_init(void)
{
    int ret;

    INF_MSG("START");

    ret = i2c_add_driver(&yas529_driver);
    if (ret)
    {
        ERR_MSG("YAS529 driver init fail");
        goto exit_yas529_register_i2c_driver_failed;
    }

    INF_MSG("FIH ECOMPASS driver init success");
    return 0;

exit_yas529_register_i2c_driver_failed:
    return ret;
}

static void __exit yas529_exit(void)
{
    i2c_del_driver(&yas529_driver);
}

module_init(yas529_init);
module_exit(yas529_exit);

MODULE_DESCRIPTION("YAMAHA YAS529 driver");
MODULE_LICENSE("GPL");
