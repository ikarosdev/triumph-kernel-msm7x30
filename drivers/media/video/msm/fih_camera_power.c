#include "fih_camera_power.h"

int fih_cam_output_gpio_control(unsigned gpio, const char *sensor, int value)
{
    int rc = 0;
    
    if(gpio==0xffff)
        return 0;
    
    rc = gpio_request(gpio, sensor);
    if (!rc) 
    {
        rc = gpio_direction_output(gpio, value);
        if (!rc)
        {
            if (value)
                printk(KERN_INFO "%s: %s: Pull gpio %d High.\n", __func__, sensor, gpio);
            else
                printk(KERN_INFO "%s: %s: Pull gpio %d Low.\n", __func__, sensor, gpio);
        }
        else
        {
            if (value)
                printk(KERN_ERR "%s: %s: ERR: Pull gpio %d High failed !\n", __func__, sensor, gpio);
            else
                printk(KERN_ERR "%s: %s: ERR: Pull gpio %d Low failed !\n", __func__, sensor, gpio);
        }
    }
    else 
    {
        printk(KERN_ERR "%s: %s: ERR: Request gpio %d failed !\n", __func__, sensor, gpio);
        return rc;
    }	
    gpio_free(gpio);
    return rc;
        
}

int fih_cam_vreg_control(const char * id, unsigned mv, unsigned enable)
{
    int rc = 0;
    struct vreg *vreg_camera = NULL;

    vreg_camera = vreg_get(NULL, id);
    if (!vreg_camera)
    {
        printk(KERN_ERR "%s: ERR: vreg_get(%s) failed !\n", __func__, id);
        return -EINVAL;
    }
    
    if (enable)
    {
        rc = vreg_set_level(vreg_camera, mv);
        if (rc)
        {
            printk(KERN_ERR "%s: ERR: Vreg %s set level to %d mV failed !\n", __func__, id, mv);
            goto fih_cam_fail;
        }
        
        rc = vreg_enable(vreg_camera);
        if (rc)
        {
            printk(KERN_ERR "%s: ERR: Vreg %s enable failed !\n", __func__, id);
            goto fih_cam_fail;
        }

        printk(KERN_INFO "%s: Enable Vreg %s (%d mV).\n", __func__, id, mv);
    }
    else
    {
        rc = vreg_disable(vreg_camera);
        if (rc)
        {
            printk(KERN_ERR "%s: ERR: Vreg %s disable failed !\n", __func__, id);
            goto fih_cam_fail;
        }
        printk(KERN_INFO "%s: Disable Vreg %s .\n", __func__, id);
    }
    
fih_cam_fail:
    
    return rc;
}

int fih_cam_vreg_pull_down_switch(const char * id, unsigned enable)
{
    int rc = 0;
    struct vreg *vreg_camera = NULL;

    vreg_camera = vreg_get(NULL, id);
    if (!vreg_camera)
    {
        printk(KERN_ERR "%s: ERR: vreg_get(%s) failed !\n", __func__, id);
        return -EINVAL;
    }

    rc = vreg_pull_down_switch(vreg_camera, enable);
    if (rc)
    {
        printk(KERN_ERR "%s: ERR: vreg_pull_down_switch(%s ) failed !\n", __func__, id);
        goto fih_cam_fail;
    }

    if (enable)
        printk(KERN_INFO "%s: Enable vreg_pull_down_switch(%s).\n", __func__, id);
    else
        printk(KERN_INFO "%s: Disable vreg_pull_down_switch(%s).\n", __func__, id);

fih_cam_fail:
    
    return rc;
}

