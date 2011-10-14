//define GPIO
struct ir_mcu {
	unsigned int mcu_rst_n;
	unsigned int mcu_buad;
	unsigned int mcu_level_shift; //Div2D5-OwenHuang-SF5_CIR_GPIOSetting-00+
};

//ioctl command set
#define MCU_RESET_CMD 		0x01
#define MCU_BAUD_9600 		0x04 //Div2D5-OwenHuang-SF5_IR_MCU_Settings-02*
#define MCU_BAUD_192000		0x03
#define MCU_LEVEL_ENABLE    0x05 //Div2D5-OwenHuang-SF5_IR_MCU_Settings-02+
#define MCU_LEVEL_DISABLE   0x06 //Div2D5-OwenHuang-SF5_IR_MCU_Settings-02+

//GPIO
#define MCU_IR_BAUD_RATE_PIN 21
#define MCU_IR_RESET_PIN     22
#define MCU_LEVEL_SHIFT_EN   18 //Div2D5-OwenHuang-SF5_IR_MCU_Settings-02+
#define MCU_LEVEL_SHIFT_EN_PCR 97 //Div2D5-OwenHuang-SF5_CIR_GPIOSetting-00+

//debug messages
#define DEBUG 0
#if DEBUG   
#define DBG_MSG(msg, ...) printk("[IR_MCU_DBG] %s : " msg "\n", __FUNCTION__, ## __VA_ARGS__)
#else    
#define DBG_MSG(...) 
#endif

#define ERR_MSG(msg, ...) printk("[IR_MCU_ERR] %s : " msg "\n", __FUNCTION__, ## __VA_ARGS__)
#define INF_MSG(msg, ...) printk("[IR_MCU_INF] %s : " msg "\n", __FUNCTION__, ## __VA_ARGS__)
