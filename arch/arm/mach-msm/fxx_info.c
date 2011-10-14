#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/compile.h>
#include "smd_private.h"

#include "proc_comm.h"  //SW252-rexer-upld-00+

static int proc_calc_metrics(char *page, char **start, off_t off,
				 int count, int *eof, int len)
{
	if (len <= off+count) *eof = 1;
	*start = page + off;
	len -= off;
	if (len>count) len = count;
	if (len<0) len = 0;
	return len;
}

static int band_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{
	int len;
	int pi = fih_get_band_id();
	char ver[40];

  switch (pi)
  {
    case FIH_BAND_W1245:
      strcpy( ver, "GSM_BAND_1234, WCDMA_BAND_1245\n");
      break;
    case FIH_BAND_W1248:
      strcpy( ver, "GSM_BAND_1234, WCDMA_BAND_1248\n");
      break;
    case FIH_BAND_W125:
      strcpy( ver, "GSM_BAND_1234, WCDMA_BAND_125\n");
      break;
    case FIH_BAND_W128:
      strcpy( ver, "GSM_BAND_1234, WCDMA_BAND_128\n");
      break;
    case FIH_BAND_W25:
      strcpy( ver, "GSM_BAND_1234, WCDMA_BAND_25\n");
      break;
    case FIH_BAND_W18:
      strcpy( ver, "GSM_BAND_1234, WCDMA_BAND_18\n");
      break;
    case FIH_BAND_W15:
      strcpy( ver, "GSM_BAND_1234, WCDMA_BAND_15\n");
      break;
    case FIH_BAND_C01:
      strcpy( ver, "CDMA_BAND_01\n");
      break;
    case FIH_BAND_C0:
      strcpy( ver, "CDMA_BAND_0\n");
      break;
    case FIH_BAND_C1:
      strcpy( ver, "CDMA_BAND_1\n");
      break;
    case FIH_BAND_C01_AWS:
      strcpy( ver, "CDMA_BAND_01F\n");
      break;
    case FIH_BAND_W1245_C01:
      strcpy( ver, "GSM_BAND_1234, CDMA_BAND_01, WCDMA_BAND_1245\n");
      break;
    case FIH_BAND_W1245_G_850_1800_1900:
      strcpy( ver, "GSM_BAND_134, WCDMA_BAND_1245\n");
      break;
    case FIH_BAND_W1248_G_900_1800_1900:
      strcpy(ver, "GSM_BAND_234, WCDMA_BAND_1248\n");
      break;
    case FIH_BAND_W125_G_850_1800_1900:
      strcpy( ver, "GSM_BAND_134, WCDMA_BAND_125\n");
      break;
    case FIH_BAND_W128_G_900_1800_1900:
      strcpy( ver, "GSM_BAND_234, WCDMA_BAND_128\n");
      break;
    case FIH_BAND_W25_G_850_1800_1900:
      strcpy( ver, "GSM_BAND_134, WCDMA_BAND_25\n");
      break;
    case FIH_BAND_W18_G_900_1800_1900:
      strcpy( ver, "GSM_BAND_234, WCDMA_BAND_18\n");
      break;
    case FIH_BAND_W1_XI_G_900_1800_1900:
      strcpy( ver, "GSM_BAND_234, WCDMA_BAND_1B\n");
      break;
    case FIH_BAND_W15_G_850_1800_1900:
      strcpy( ver, "GSM_BAND_134, WCDMA_BAND_15\n");
      break;
    default:
      strcpy( ver, "Unkonwn RF band id\n");
      break;
    }
	len = snprintf(page, PAGE_SIZE, "%s\n",
		ver);
		
	return proc_calc_metrics(page, start, off, count, eof, len);
}

static int device_model_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{
	int len;
	int pi = fih_get_product_id();
	char ver[24];
		
	switch (pi){
	case Product_FB0:
		strcpy(ver, "FB0");
		break;
	case Product_FD1:
		strcpy(ver, "FD1");
		break;
	case Product_FB3:
		strcpy(ver, "FB3");
		break;		
	case Product_FB1:
		strcpy(ver, "FB1");
		break;			
	case Product_SF3:
		strcpy(ver, "SF3"); //SW2-5-1-MP-Device_Model-00*
		break;
	case Product_SF5:
		strcpy(ver, "SF5"); //SW2-5-1-MP-Device_Model-00*
		break;
	case Product_SF6:
		strcpy(ver, "SF6"); //SW2-5-1-MP-Device_Model-00*
		break;
  //Div252-AC-HARDWARE_ID_02+{
	case Product_SF8:
		strcpy(ver, "SF8"); //SW2-5-1-MP-Device_Model-00*
		break;
  //Div252-AC-HARDWARE_ID_02+}
  case Product_SFH:
      strcpy(ver, "SFH");
      break;
  case Product_SH8:
      strcpy(ver, "SH8");
      break;
  case Product_SFC:
      strcpy(ver, "SFC");
      break;
	default:
		strcpy(ver, "Unkonwn Device Model");
		break;
	}

	len = snprintf(page, PAGE_SIZE, "%s\n",
		ver);
		
	return proc_calc_metrics(page, start, off, count, eof, len);	
}

static int baseband_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{
	int len;

	int pp = fih_get_product_phase();
	char ver[24];
	
	switch (pp){
	case Product_PR1:
		strcpy(ver, "PR1");
		break; 
	case Product_PR2:
		strcpy(ver, "PR2");
		break; 
	case Product_PR2p5:
		strcpy(ver, "PR2p5");
		break; 
	case Product_PR230:
		strcpy(ver, "PR230");
		break; 
	case Product_PR231:
		if (fih_get_product_id() == Product_FD1) //Div2-SW2-BSP, JOE HSU,FD1 PR235 = FB0 PR231
				strcpy(ver, "PR235");
			else
		    strcpy(ver, "PR231");
		break; 
	case Product_PR232:
		strcpy(ver, "PR232");
		break; 
	case Product_PR3:
		strcpy(ver, "PR3");
		break;
  //Div252-AC-HARDWARE_ID_01+{
	case Product_PR1p5:
		strcpy(ver, "PR15");
		break; 
  //Div252-AC-HARDWARE_ID_01+}
	case Product_PR4:
		strcpy(ver, "PR4");
		break;
	case Product_PR5:
		strcpy(ver, "PR5");
		break;  
	case Product_EVB:
		strcpy(ver, "EVB");
		break; 
	default:
		strcpy(ver, "Unkonwn Baseband version");
		break;
	}

	len = snprintf(page, PAGE_SIZE, "%s\n",
		ver);

	return proc_calc_metrics(page, start, off, count, eof, len);
}

//Div2-SW2-BSP,JOE HSU,get_oem_info ,get memory info ,+++
static int emmc_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{
	int len;
        char *pe;
	char ver[24];
	
         pe = fih_get_emmc_info();

	if (strcmp(pe ,"SEM02") == 0)
     	    strcpy(ver, "SanDisk_2G");
  else if (strcmp(pe ,"M2G1D") == 0)
     	      strcpy(ver, "Samsung_2G");
  else if (strcmp(pe ,"MMC02") == 0)
     	      strcpy(ver, "Kingston_2G");     	      
	else if (strcmp(pe ,"SEM04") == 0)
		strcpy(ver, "SanDisk_4G");      
  else         	
	    strcpy(ver, "Unkonwn eMMC");


	len = snprintf(page, PAGE_SIZE, "%s\n",
		ver);
		
	return proc_calc_metrics(page, start, off, count, eof, len);	
}

static int dram_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{
	int len;
	int pd = fih_get_dram_info();
	char ver[24];
	
 	
	switch (pd){
	case 0:
		strcpy(ver, "DDR_1G");
		break; 
	case 1:
		strcpy(ver, "DDR_2G");
		break; 
	case 2:
		strcpy(ver, "DDR_3G");
		break; 
	case 3:
		strcpy(ver, "DDR_4G");
		break; 
	default:
		strcpy(ver, "Unkonwn DRAM");
		break;
	}

	len = snprintf(page, PAGE_SIZE, "%s\n",
		ver);
		
	return proc_calc_metrics(page, start, off, count, eof, len);	
}

//Div2-SW2-BSP, HenryMCWang, get power on cause, +++
static int poweroncause_read_proc(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{
	int len;
	char ver[24];
		
	sprintf(ver, "0x%x", fih_get_poweroncause_info());
	
	len = snprintf(page, PAGE_SIZE, "%s\n",
		ver);
		
	return proc_calc_metrics(page, start, off, count, eof, len);	
}

//Div2-SW2-BSP,JOE HSU,get_oem_info ,get memory info ,+++

//Div2D5-OwenHuang-FB0_Sensor_Proc_Read-00+{
#define BIT_ECOMPASS  1 << 0
#define BIT_GSENSOR   1 << 1
#define BIT_LIGHT     1 << 2
#define BIT_PROXIMITY 1 << 3
#define BIT_GYROSCOPE 1 << 4
#define DEFAULT_SUPPORT_SENSOR (BIT_ECOMPASS | BIT_GSENSOR | BIT_LIGHT | BIT_PROXIMITY)
#define DEFAULT_SF6_SUPPORT_SENSOR (BIT_ECOMPASS | BIT_GSENSOR | BIT_LIGHT ) //DIV5-BSP-CH-SF6-SENSOR-PORTING03++
static int support_sensor_read(char *page, char **start, off_t off,
				 int count, int *eof, void *data)
{
	int len;
	int project_id = fih_get_product_id();
	int phase_id = fih_get_product_phase(); //Div2D5-OwenHuang-FB0_Sensor_Proc_Read-01+
	int project_sensor = 0;
		
	switch (project_id){
	case Product_FB0:
		project_sensor = DEFAULT_SUPPORT_SENSOR;
		break;
	case Product_FD1:
		project_sensor = DEFAULT_SUPPORT_SENSOR;
		break;
	case Product_SF3:
		project_sensor = DEFAULT_SUPPORT_SENSOR;
		break;
	case Product_SF5:
		//Div2D5-OwenHuang-FB0_Sensor_Proc_Read-01+{
		if (phase_id == Product_PR1)
			project_sensor = BIT_ECOMPASS | BIT_GSENSOR | BIT_LIGHT; //not support proximity sensor at PR1 stage
		else
			project_sensor = DEFAULT_SUPPORT_SENSOR;
		//Div2D5-OwenHuang-FB0_Sensor_Proc_Read-01+}
		break;
	case Product_SF6:
		project_sensor = DEFAULT_SUPPORT_SENSOR; //DIV5-BSP-CH-SF6-SENSOR-PORTING04++
		break;
	case Product_SF8:
		project_sensor = DEFAULT_SUPPORT_SENSOR;
		break;
	default:
		project_sensor = DEFAULT_SUPPORT_SENSOR;
		break;
	}

	len = snprintf(page, PAGE_SIZE, "%d\n", project_sensor);
		
	return proc_calc_metrics(page, start, off, count, eof, len);
}
//Div2D5-OwenHuang-FB0_Sensor_Proc_Read-00+}

//SW252-rexer-upld-00+[
#ifdef CONFIG_FIH_PROJECT_SF4V5
static int wifi_mac_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
  int len;
  char wifi_mac[6];

  proc_comm_ftm_wlanaddr_read(&wifi_mac[0]);

  len = snprintf(page, PAGE_SIZE, "%x:%x:%x:%x:%x:%x\n", wifi_mac[5],wifi_mac[4],wifi_mac[3],wifi_mac[2],wifi_mac[1],wifi_mac[0]);

  return proc_calc_metrics(page, start, off, count, eof, len);	
}

static int product_id_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
  int len;
  char product_id[40];

  proc_comm_ftm_product_id_read((unsigned*)product_id);

  len = snprintf(page, PAGE_SIZE, "%s\n", (char*)product_id);

  return proc_calc_metrics(page, start, off, count, eof, len);	
}
#endif
//SW252-rexer-upld-00+]


static struct {
		char *name;
		int (*read_proc)(char*,char**,off_t,int,int*,void*);
} *p, fxx_info[] = {
	{"devmodel",	device_model_read_proc},
	{"baseband",	baseband_read_proc},
	{"bandinfo",	band_read_proc},
    {"emmcinfo",	emmc_read_proc},
    {"draminfo",	dram_read_proc},
    {"poweroncause",	poweroncause_read_proc},
	{"sensors", support_sensor_read}, //Div2D5-OwenHuang-FB0_Sensor_Proc_Read-00+
 #ifdef CONFIG_FIH_PROJECT_SF4V5
    //SW252-rexer-upld-00+[
    {"wifi_mac", wifi_mac_read},
    {"product_id",product_id_read},
    //SW252-rexer-upld-00+]
 #endif	
	{NULL,},
};

void fxx_info_init(void)
{	
	for (p = fxx_info; p->name; p++)
		create_proc_read_entry(p->name, 0, NULL, p->read_proc, NULL);
		
}
EXPORT_SYMBOL(fxx_info_init);

void fxx_info_remove(void)
{
	for (p = fxx_info; p->name; p++)
		remove_proc_entry(p->name, NULL);
}
EXPORT_SYMBOL(fxx_info_remove);
