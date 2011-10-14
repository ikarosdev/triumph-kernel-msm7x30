#include <media/msm_camera.h>
#include <mach/board.h>
#include <linux/string.h>

#define TRUE 1
#define FALSE 0

#define DONT_CARE 0

struct sensor_parameters {
    unsigned int *autoexposuretbl;
    unsigned int *effectstbl;
    unsigned int *wbtbl;
    unsigned int *antibandingtbl;
    unsigned int *flashtbl;
    unsigned int *focustbl;
    unsigned int *ISOtbl;
    unsigned int *lensshadetbl;
    unsigned int *scenemodetbl;
    unsigned int *continuous_aftbl;
    unsigned int *touchafaectbl;
    unsigned int *frame_rate_modestbl;
    int8_t autoexposuretbl_size;
    int8_t effectstbl_size;
    int8_t wbtbl_size;
    int8_t antibandingtbl_size;
    int8_t flashtbl_size;
    int8_t focustbl_size;
    int8_t ISOtbl_size;
    int8_t lensshadetbl_size;
    int8_t scenemodetbl_size;
    int8_t continuous_aftbl_size;
    int8_t touchafaectbl_size;
    int8_t frame_rate_modestbl_size;
    int8_t  max_brightness;
    int8_t  max_contrast;
    int8_t  max_saturation;
    int8_t  max_sharpness;
    int8_t  min_brightness;
    int8_t  min_contrast;
    int8_t  min_saturation;
    int8_t  min_sharpness;
};

extern void sensor_init_parameters(const struct msm_camera_sensor_info *data, struct sensor_parameters const *para_tbls);



