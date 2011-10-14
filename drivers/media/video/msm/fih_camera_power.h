#include <mach/gpio.h>
#include <mach/vreg.h>

extern int fih_cam_output_gpio_control(unsigned gpio, const char *sensor, int value);
extern int fih_cam_vreg_control(const char * id, unsigned mv, unsigned enable);
extern int fih_cam_vreg_pull_down_switch(const char * id, unsigned enable);

