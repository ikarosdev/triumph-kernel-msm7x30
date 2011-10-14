#ifndef _FIH_HW_INFO_H
#define _FIH_HW_INFO_H

extern unsigned int fih_get_product_id(void);

//SW252-rexer-hwid-01+{
typedef enum 
{
  // FB series ++
  Product_FB0 = 1,
  Product_FB1,
  Product_FB3,
  Product_FBX_MAX = 20,
  // FB series --
  // FD series ++
  Product_FD1 = 21,
  Product_FDX_MAX = 40,
  // FD series --
  // SF series ++
  Product_SF3 = 41,
  Product_SF5,
  Product_SF6,
  Product_SF8,
  Product_SFH,
  Product_SH8,
  Product_SFC,
  Product_SFX_MAX = 60,
  // SF series --
  Product_ID_MAX = 255,
  Product_ID_EXTEND_TO_DWORD = 0xFFFFFFFF, //SW252-rexer-smem-00
}fih_virtual_project_id_type;

typedef enum 
{
  Product_EVB   = 1,
  Product_PR1   = 10,
  Product_PR1p5 = 15,
  Product_PR2   = 20,
  Product_PR2p5 = 25, 
  Product_PR230 = 30,
  Product_PR231 = 31, 
  Product_PR232 = 32,
  Product_PR3   = 40,
  Product_PR4   = 50,
  Product_PR5   = 60,
  Product_PHASE_MAX   = 255,
  Product_PHASE_EXTEND_TO_DWORD = 0xFFFFFFFF, //SW252-rexer-smem-00
}fih_virtual_phase_id_type;

typedef enum 
{
  // GSM Quad-band + WCDMA
  FIH_BAND_W1245 = 1,
  FIH_BAND_W1248,
  FIH_BAND_W125,
  FIH_BAND_W128,
  FIH_BAND_W25,
  FIH_BAND_W18,
  FIH_BAND_W15,

  // CDMA (and EVDO) only
  FIH_BAND_C01 = 21,
  FIH_BAND_C0,
  FIH_BAND_C1,
  FIH_BAND_C01_AWS,

  // Multimode + Quad-band
  FIH_BAND_W1245_C01 = 41,
  
  // GSM Tri-band + WCDMA
  FIH_BAND_W1245_G_850_1800_1900 = 61,
  FIH_BAND_W1248_G_900_1800_1900,
  FIH_BAND_W125_G_850_1800_1900,
  FIH_BAND_W128_G_900_1800_1900,
  FIH_BAND_W25_G_850_1800_1900,
  FIH_BAND_W18_G_900_1800_1900,
  FIH_BAND_W15_G_850_1800_1900,
  FIH_BAND_W1_XI_G_900_1800_1900,

  FIH_BAND_MAX = 255,
  FIH_BAND_EXTEND_TO_DWORD = 0xFFFFFFFF, //SW252-rexer-smem-00
}fih_virtual_band_id_type;

typedef struct {
  fih_virtual_project_id_type   virtual_project_id;
  fih_virtual_phase_id_type     virtual_phase_id;
  fih_virtual_band_id_type      virtual_band_id;
}fih_hw_info_type;

static __inline bool IS_SF8_SERIES_PRJ(void)
{
  unsigned int is_sf8 =fih_get_product_id();	
  if((is_sf8==Product_SF8)||(is_sf8==Product_SFH)||(is_sf8==Product_SH8)||(is_sf8==Product_SFC))
  {
    return true;
  }
	return false;
}
//SW252-rexer-hwid-01+}

#endif
