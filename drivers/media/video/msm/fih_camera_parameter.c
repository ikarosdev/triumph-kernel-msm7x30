
#include "fih_camera_parameter.h"



uint32_t create_sensor_parameters(unsigned int *tbl,int8_t nums)
{
    int i;
    uint32_t para=0;
    //printk("create_sensor_parameters nums=%d\n",nums);
    for (i = 0; i < nums; i++)
    {
        para = para | (1<< tbl[i] );
    }
    //printk("create_sensor_parameters para=%d\n",para);
    return para;
}


void sensor_init_parameters(const struct msm_camera_sensor_info *data, struct sensor_parameters const *para_tbls)
{
    data->parameters_data->autoexposure=create_sensor_parameters(&para_tbls->autoexposuretbl[0],para_tbls->autoexposuretbl_size);
    data->parameters_data->effects=create_sensor_parameters(&para_tbls->effectstbl[0],para_tbls->effectstbl_size);
    data->parameters_data->wb=create_sensor_parameters(&para_tbls->wbtbl[0],para_tbls->wbtbl_size);
    data->parameters_data->antibanding=create_sensor_parameters(&para_tbls->antibandingtbl[0],para_tbls->antibandingtbl_size);
    data->parameters_data->flash=create_sensor_parameters(&para_tbls->flashtbl[0],para_tbls->flashtbl_size);
    data->parameters_data->focus=create_sensor_parameters(&para_tbls->focustbl[0],para_tbls->focustbl_size);
    data->parameters_data->ISO=create_sensor_parameters(&para_tbls->ISOtbl[0],para_tbls->ISOtbl_size);
    data->parameters_data->lensshade=create_sensor_parameters(&para_tbls->lensshadetbl[0],para_tbls->lensshadetbl_size);
    data->parameters_data->scenemode=create_sensor_parameters(&para_tbls->scenemodetbl[0],para_tbls->scenemodetbl_size);
    data->parameters_data->continuous_af=create_sensor_parameters(&para_tbls->continuous_aftbl[0],para_tbls->continuous_aftbl_size);
    data->parameters_data->touchafaec=create_sensor_parameters(&para_tbls->touchafaectbl[0],para_tbls->touchafaectbl_size);
    data->parameters_data->frame_rate_modes=create_sensor_parameters(&para_tbls->frame_rate_modestbl[0],para_tbls->frame_rate_modestbl_size);
    data->parameters_data->max_brightness=para_tbls->max_brightness;
    data->parameters_data->max_contrast=para_tbls->max_contrast;
    data->parameters_data->max_saturation=para_tbls->max_saturation;
    data->parameters_data->max_sharpness=para_tbls->max_sharpness;
    data->parameters_data->min_brightness=para_tbls->min_brightness;
    data->parameters_data->min_contrast=para_tbls->min_contrast;
    data->parameters_data->min_saturation=para_tbls->min_saturation;
    data->parameters_data->min_sharpness=para_tbls->min_sharpness;
}


