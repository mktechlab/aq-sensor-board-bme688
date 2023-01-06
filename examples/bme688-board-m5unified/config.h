#ifndef CONFIG_H
#define CONFIG_H

//#define UPLOAD_DATA_EN    // Enable upload data to Ambient

#if defined(UPLOAD_DATA_EN)
// WiFi Access Point
const char* WIFI_SSID   = "********";
const char* WIFI_PASS   = "********";
unsigned int AMB_CH_ID  = 0; // Channel ID for Ambient
const char* AMB_WR_KEY  = "********"; // Write Key to Channel
#endif /* defined(UPLOAD_DATA_EN) */

const int UPDATE_INTERVAL_S = 10; // Update interval seconds (default:10, range:3-3600)
const int UPLOAD_INTERVAL_S = 60; // Upload interval seconds (default:60, range:30-3600)

enum D_TYPE {
   D_TYPE_IAQ       // Index of Air Quality (recommended for mobile devices)
  ,D_TYPE_sIAQ      // Static IAQ (recommended for stationay devices)
  ,D_TYPE_eCO2      // Estimation of the CO2
  ,D_TYPE_bVOC      // Conversion into breath-VOC equivalents
  ,D_TYPE_RAW_TEMP  // Raw temperature
  ,D_TYPE_AMB_TEMP  // Ambient temperature (compensating the influence of device heatsources)
  ,D_TYPE_RAW_HUMI  // Raw relative humidity
  ,D_TYPE_AMB_HUMI  // Ambient relative humidity (compensating the influence of device heatsources) 
  ,D_TYPE_RAW_PRES  // Raw pressure
  ,D_TYPE_MAX
};
// up to 4 characters
char D_TYPE_LABEL_STR[D_TYPE_MAX][8] = {
   "IAQ :"
  ,"sIAQ:"
  ,"eCO2:"
  ,"bVOC:"
  ,"rTMP:"
  ,"TEMP:"
  ,"rHMD:"
  ,"HUMI:"
  ,"PRES:"
};
// up to 3 characters
char D_TYPE_UNIT_STR[D_TYPE_MAX][8] = {
   ""
  ,""
  ,"ppm"
  ,"ppm"
  ,"`C"
  ,"`C"
  ,"%RH"
  ,"%RH"
  ,"hPa"
};

const int DISP_PAGE_NUM = 3;
const int DISP_VAL_NUM = 3;
const int DISP_VAL[DISP_PAGE_NUM][DISP_VAL_NUM] = {
  { D_TYPE_IAQ ,  D_TYPE_AMB_TEMP,  D_TYPE_AMB_HUMI } // Page 1
 ,{ D_TYPE_sIAQ,  D_TYPE_AMB_TEMP,  D_TYPE_AMB_HUMI } // Page 2
 ,{ D_TYPE_eCO2,  D_TYPE_bVOC,      D_TYPE_RAW_PRES } // Page 3
};
const int LED_PAGE_NUM = 6;
const int LED_VAL[LED_PAGE_NUM] = {
  D_TYPE_IAQ, D_TYPE_sIAQ, D_TYPE_AMB_TEMP, D_TYPE_AMB_HUMI, D_TYPE_eCO2, D_TYPE_bVOC
};

enum AL_LV {
   AL_LV_1
  ,AL_LV_2
  ,AL_LV_NUM
};
enum AL_RANGE {
   AL_RANGE_MIN
  ,AL_RANGE_MAX
  ,AL_RANGE_NUM
};

enum B_TYPE {
   B_TYPE_NOT_SUPPORT
  ,B_TYPE_M5STACK         // M5Stack
  ,B_TYPE_M5STICKC        // M5StickC
  ,B_TYPE_M5STICKC_PLUS   // M5StickC Plus
  ,B_TYPE_M5STACK_COREINK // M5Stack CoreInk
  ,B_TYPE_M5ATOM          // M5Atom Lite/Matrix
  ,B_TYPE_MAX
};

const float OFS_VAL[B_TYPE_MAX][D_TYPE_MAX] {
// {  IAQ, sIAQ, eCO2, bVOC, rTMP, TEMP, rHMD, HUMI, PRES }
   {  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0,  0.0 }
  ,{  0.0,  0.0,  0.0,  0.0, -3.0, -3.0,  9.0,  9.0,  0.0 } // M5Stack
  ,{  0.0,  0.0,  0.0,  0.0, -4.2, -4.2, 13.0, 13.0,  0.0 } // M5StickC
  ,{  0.0,  0.0,  0.0,  0.0, -4.2, -4.2, 13.0, 13.0,  0.0 } // M5StickC Plus
  ,{  0.0,  0.0,  0.0,  0.0, -1.9, -1.9,  9.0,  9.0,  0.0 } // M5Stack CoreInk
  ,{  0.0,  0.0,  0.0,  0.0, -2.4, -2.4,  9.4,  9.4,  0.0 } // M5Atom Lite/Matrix
};

const float AL_SET_RANGE[D_TYPE_MAX][AL_LV_NUM][AL_RANGE_NUM] {
// {{LV1_MIN, LV1_MAX}, {LV2_MIN, LV2_MAX}}
   {{    0.0,   150.0}, {  150.0,   250.0}} // IAQ
  ,{{    0.0,   150.0}, {  150.0,   250.0}} // sIAQ
  ,{{    0.0,  1000.0}, { 1000.0,  2000.0}} // eCO2
  ,{{    0.0,     1.4}, {    1.4,    40.0}} // bVOC
  ,{{   22.0,    26.0}, {   16.0,    30.0}} // Raw temperature
  ,{{   22.0,    26.0}, {   16.0,    30.0}} // Ambient temperature
  ,{{   40.0,    50.0}, {   20.0,    65.0}} // Raw relative humidity
  ,{{   40.0,    50.0}, {   20.0,    65.0}} // Ambient relative humidity
  ,{{    0.0,     0.0}, {    0.0,     0.0}} // Raw pressure
};

#endif /* CONFIG_H */
