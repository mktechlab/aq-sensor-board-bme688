#include "config.h"
#include <M5Unified.h>
#include "FastLED.h"

#include <bsec2.h>
Bsec2 envSensor;
TwoWire Wire2 = TwoWire(2);

#if defined(UPLOAD_DATA_EN)
#include <WiFi.h>
#include "Ambient.h"

// for Ambient
WiFiClient client;
Ambient ambient;

const int WIFI_CON_WAIT_INTERVAL_MS	= 1000;
const int WIFI_CON_TIMEOUT_MS       = 60000;
enum WIFI_STS {
   WIFI_STS_INIT
  ,WIFI_STS_DISCONNECTED
  ,WIFI_STS_CONNECTED
  ,WIFI_STS_MAX
};
WIFI_STS wifiSts = WIFI_STS_INIT;
char WIFI_STS_STR[WIFI_STS_MAX][8] = {
   "W..."
  ,"W---"
  ,"WiFi"
};
#endif /* defined(UPLOAD_DATA_EN) */

const int CPU_FREQ_MHZ = 80;
const int BATT_LV_0    = 30;

int i2c_scl_pin = 26;
int i2c_sda_pin = 0;

int update_interval_sec = UPDATE_INTERVAL_S;
int upload_interval_sec = UPLOAD_INTERVAL_S;

enum D_STS {
   D_STS_NO_ERR
  ,D_STS_INITIALIZING
  ,D_STS_NO_DEVICE
  ,D_STS_NETWORK_AMB_ERR
  ,D_STS_READ_DATA_ERR
  ,D_STS_MEASURING
  ,D_STS_MAX
};
#if defined(LANG_JA_EN)
char D_STS_MSG[D_STS_MAX][64] = {
   ""
  ,"初期化中..."
  ,"ﾃﾞﾊﾞｲｽが見つかりません。"
  ,"ﾃﾞｰﾀの送信に失敗しました。"
  ,"ﾃﾞｰﾀの読み取りに失敗しました。"
  ,"測定中..."
};
#else
char D_STS_MSG[D_STS_MAX][32] = {
   "                      "
  ,"Initializing..."
  ,"Device not found."
  ,"Failed to send data."
  ,"Failed to read data."
  ,"Measuring..."
};
#endif /* LANG_JA_EN */

D_STS currentSts = D_STS_NO_ERR;
B_TYPE boardType = B_TYPE_NOT_SUPPORT;

enum AL_TYPE {
   AL_TYPE_NONE
  ,AL_TYPE_OVER
  ,AL_TYPE_RANGE
  ,AL_TYPE_MAX
};
const AL_TYPE valAlertType[D_TYPE_MAX] {
   AL_TYPE_OVER
  ,AL_TYPE_OVER
  ,AL_TYPE_OVER
  ,AL_TYPE_OVER
  ,AL_TYPE_RANGE
  ,AL_TYPE_RANGE
  ,AL_TYPE_RANGE
  ,AL_TYPE_RANGE
  ,AL_TYPE_NONE
};

float sensorData[D_TYPE_MAX];
char valStr[D_TYPE_MAX][10];
uint16_t valDispColor[D_TYPE_MAX];
char valBattVolPerStr[10];
char drawValBattStr[32];

int dispPage        = 0;
int ledPage         = 0;

bool bInitialUpdate = true;
bool bInitialUpload = true;
bool bDataReady     = false;
bool bNetworkReady  = false;
bool bDispEn        = false;
bool bDispLcdEn     = false;
bool bBattEquipped  = false;
bool bRGBLedEn      = false;

enum LCD_BRI_LV {
   LCD_BRI_LV_OFF
  ,LCD_BRI_LV_LOW
  ,LCD_BRI_LV_NRM
  ,LCD_BRI_LV_HIGH
  ,LCD_BRI_LV_NUM
};
uint8_t lcdBriLv[LCD_BRI_LV_NUM] = { 0, 64, 128, 194 };
uint8_t curLcdBri = LCD_BRI_LV_NRM;
enum LED_BRI_LV {
   LED_BRI_LV_OFF
  ,LED_BRI_LV_LOW
  ,LED_BRI_LV_NRM
  ,LED_BRI_LV_HIGH
  ,LED_BRI_LV_NUM
};
uint8_t ledBriLv[LED_BRI_LV_NUM] = { 0, 4, 8, 16 };
uint8_t curLedBri = LED_BRI_LV_NRM;
const uint8_t LED_DATA_PIN  = 27;
uint8_t ledTotalNum       = 1;
CRGB leds[25];
CRGB valLedColor[D_TYPE_MAX];
const lgfx::IFont* defaultFont;

unsigned long timeUpdateMs = 0;
unsigned long timeUploadMs = 0;

// forward declaration
void drawTitle();
void drawSts(D_STS sts);
void updateStsWrite(D_STS sts);
#if defined(UPLOAD_DATA_EN)
void drawWiFiSts(WIFI_STS wifiSts);
void updateWiFiSts(WIFI_STS wifiSts);
#endif /* defined(UPLOAD_DATA_EN) */
void updateDisp();
void newDataCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec);
void initSensor();

void drawTitle() {
#if defined(LANG_JA_EN)
  const char* title = "空気質モニタ (BME688)";
#else
  const char* title = "AQ Monitor (BME688)";
#endif /* LANG_JA_EN */
  M5.Display.setFont(defaultFont);
  M5.Display.setTextSize(1.00);
  switch (boardType) {
    case B_TYPE_M5STACK:
      M5.Display.setTextSize(1.25);
      M5.Display.setCursor(M5.Display.width()/2 - M5.Display.textWidth(title)/2, 0);
      M5.Display.fillRect(0, 0, M5.Display.width(), 0, BLACK);
      break;
    case B_TYPE_M5STICKC:
#if defined(LANG_JA_EN)
      title = "空気質BME688";
#else
      title = "AQ Mon.BME688";
#endif /* LANG_JA_EN */
      M5.Display.setCursor(M5.Display.width()/2 - M5.Display.textWidth(title)/2, 0);
      M5.Display.fillRect(0, 0, M5.Display.width(), 0, BLACK);
      break;
    case B_TYPE_M5STICKC_PLUS:
#if defined(LANG_JA_EN)
      title = "空気質ﾓﾆﾀ(BME688)";
#else
      title = "AQ Mon.(BME688)";
#endif /* LANG_JA_EN */
      M5.Display.setCursor(M5.Display.width()/2 - M5.Display.textWidth(title)/2, 0);
      M5.Display.fillRect(0, 0, M5.Display.width(), 0, BLACK);
      break;
    case B_TYPE_M5STACK_COREINK:
      M5.Display.setCursor(M5.Display.width()/2 - M5.Display.textWidth(title)/2, 0);
      M5.Display.setTextColor(BLACK);
      M5.Display.fillRect(0, 0, M5.Display.width(), 0, WHITE);
      break;
    case B_TYPE_M5ATOM:
      // Not supported
      break;
    default:
      break;
  }
  M5.Display.print(title);
}

void drawSts(D_STS sts) {
  M5.Display.setFont(defaultFont);
  M5.Display.setTextSize(1.00);
  switch (boardType) {
    case B_TYPE_M5STACK:
      M5.Display.setTextSize(1.25);
      M5.Display.setCursor(0, 20);
      M5.Display.fillRect(0, 20, M5.Display.width(), 20, BLACK);
      break;
    case B_TYPE_M5STICKC:
      M5.Display.setCursor(0, 12);
      M5.Display.fillRect(0, 12, M5.Display.width(), 12, BLACK);
      break;
    case B_TYPE_M5STICKC_PLUS:
      M5.Display.setCursor(0, 20);
      M5.Display.fillRect(0, 20, M5.Display.width(), 20, BLACK);
      break;
    case B_TYPE_M5STACK_COREINK:
      M5.Display.setCursor(0, 16);
      M5.Display.setTextColor(BLACK);
      M5.Display.fillRect(0, 16, M5.Display.width(), 16, WHITE);
      break;
    case B_TYPE_M5ATOM:
      // Not supported
      break;
    default:
      break;
  }
  M5.Display.print(D_STS_MSG[sts]);
}

void updateStsWrite(D_STS sts) {
    M5.Display.startWrite();
    drawSts(sts);
    M5.Display.endWrite();
    M5.Display.display();
}

void UpdateLed(CRGB color, uint8_t led_num) {
  for(int i=0; i< led_num; i++) {
    leds[i] = color;
  }
  FastLED.show();
}

void InitM5() {
  currentSts = D_STS_INITIALIZING;
  timeUpdateMs = millis();
  timeUploadMs = millis();
  
  auto cfg = M5.config();
  M5.begin(cfg);

  if (update_interval_sec < 3) {
    update_interval_sec = 3;
  }
  if (upload_interval_sec < 30) {
    upload_interval_sec = 30;
  }
#if defined(LANG_JA_EN)
  defaultFont = &fonts::lgfxJapanGothic_16;
#else
  defaultFont = &fonts::AsciiFont8x16;
#endif /* LANG_JA_EN */
  switch (M5.getBoard()) {
    case m5::board_t::board_M5Stack:
      boardType = B_TYPE_M5STACK;
      i2c_scl_pin = 22;
      i2c_sda_pin = 21;
      bDispEn = true;
      bDispLcdEn = true;
      bBattEquipped = true;
      break;
    case m5::board_t::board_M5StickC:
      boardType = B_TYPE_M5STICKC;
      i2c_scl_pin = 26;
      i2c_sda_pin = 0;
      bDispEn = true;
      bDispLcdEn = true;
      bBattEquipped = true;
#if defined(LANG_JA_EN)
      defaultFont = &fonts::lgfxJapanGothic_12;
#else
      defaultFont = &fonts::Font0;
#endif /* LANG_JA_EN */
      break;
    case m5::board_t::board_M5StickCPlus:
      boardType = B_TYPE_M5STICKC_PLUS;
      i2c_scl_pin = 26;
      i2c_sda_pin = 0;
      bDispEn = true;
      bDispLcdEn = true;
      bBattEquipped = true;
      break;
    case m5::board_t::board_M5StackCoreInk:
      boardType = B_TYPE_M5STACK_COREINK;
      i2c_scl_pin = 26;
      i2c_sda_pin = 25;
      if (update_interval_sec < 60) {
        update_interval_sec = 60;
      }
      bDispEn = true;
      bBattEquipped = true;
      break;
    case m5::board_t::board_M5Atom:
      boardType = B_TYPE_M5ATOM;
      i2c_scl_pin = 22;
      i2c_sda_pin = 19;
      bRGBLedEn = true;
      ledTotalNum   = 25;
      break;
    default:
      boardType = B_TYPE_NOT_SUPPORT;
      break;
  }
  setCpuFrequencyMhz(CPU_FREQ_MHZ);
  
  if (bDispLcdEn) {
    M5.Display.setBrightness(lcdBriLv[curLcdBri]);
  }
  if (bDispEn) {
    M5.Display.startWrite();
    M5.Display.clear();
    drawTitle();
    drawSts(currentSts);
    M5.Display.endWrite();
    M5.Display.display();
  }
  if (bRGBLedEn) {
    FastLED.addLeds<WS2812,LED_DATA_PIN,GRB>(leds, ledTotalNum).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(ledBriLv[curLedBri]);
  }
}

#if defined(UPLOAD_DATA_EN)
void NetworkTask(void * pvParameters) {
  Serial.print("NetworkTask started.");
  ambient.begin(AMB_CH_ID, AMB_WR_KEY, &client); // Initialize Ambient channel
  updateWiFiSts(wifiSts);
  while(1){
    if (WiFi.status() != WL_CONNECTED) {
      wifiSts = WIFI_STS_DISCONNECTED;
      updateWiFiSts(wifiSts);
      bNetworkReady = false;
      while (!bNetworkReady) {
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        auto last = millis();
        while (WiFi.status() != WL_CONNECTED && last + WIFI_CON_TIMEOUT_MS > millis()) {
          delay(WIFI_CON_WAIT_INTERVAL_MS);
          Serial.print(".");
        }
        if (WiFi.status() == WL_CONNECTED) {
          Serial.println();
          Serial.print("WiFi connected\r\nIP address: ");
          Serial.println(WiFi.localIP());
          wifiSts = WIFI_STS_CONNECTED;
          updateWiFiSts(wifiSts);
          bNetworkReady = true;
        } else {
          Serial.println("Retry WiFi connect.");
          WiFi.disconnect(true,true);
          WiFi.reconnect();
        }
      }
    }
    delay(WIFI_CON_WAIT_INTERVAL_MS);
  }
}
#endif /* defined(UPLOAD_DATA_EN) */

void CheckBsecSts(Bsec2 bsec) {
  if (bsec.status < BSEC_OK) {
    Serial.println("BSEC error code : " + String(bsec.status));
    currentSts = D_STS_READ_DATA_ERR;
  } else if (bsec.status > BSEC_OK) {
    Serial.println("BSEC warning code : " + String(bsec.status));
    currentSts = D_STS_READ_DATA_ERR;
  }
  if (bsec.sensor.status < BME68X_OK) {
    Serial.println("BME68X error code : " + String(bsec.sensor.status));
    currentSts = D_STS_READ_DATA_ERR;
  } else if (bsec.sensor.status > BME68X_OK) {
    Serial.println("BME68X warning code : " + String(bsec.sensor.status));
    currentSts = D_STS_READ_DATA_ERR;
  }
}

void UpdateAlertSts() {
  for (int i = 0; i < D_TYPE_MAX; i++) {
    uint8_t col_r, col_g, col_b;
    if (String(valStr[i]).startsWith("-")) {
      // DARKGREY
      col_r = 0x80;
      col_g = 0x80;
      col_b = 0x80;
    } else if (AL_TYPE_OVER == valAlertType[i]) {
      if (sensorData[i] < AL_SET_RANGE[i][AL_LV_1][AL_RANGE_MAX]) {
        col_r = 0x00;
        col_g = 0xff;
        col_b = 0x00;
      } else if (sensorData[i] < AL_SET_RANGE[i][AL_LV_2][AL_RANGE_MAX]) {
        col_r = 0xff;
        col_g = 0xb4;
        col_b = 0x00;
      } else {
        col_r = 0xff;
        col_g = 0x00;
        col_b = 0x00;
      }
    } else if (AL_TYPE_RANGE == valAlertType[i]) {
      if (AL_SET_RANGE[i][AL_LV_1][AL_RANGE_MIN] <= sensorData[i] && sensorData[i] < AL_SET_RANGE[i][AL_LV_1][AL_RANGE_MAX]) {
        col_r = 0x00;
        col_g = 0xff;
        col_b = 0x00;
      } else if (AL_SET_RANGE[i][AL_LV_2][AL_RANGE_MIN] <= sensorData[i] && sensorData[i] < AL_SET_RANGE[i][AL_LV_1][AL_RANGE_MIN]
              || AL_SET_RANGE[i][AL_LV_1][AL_RANGE_MAX] <= sensorData[i] && sensorData[i] < AL_SET_RANGE[i][AL_LV_2][AL_RANGE_MAX]) {
        col_r = 0xff;
        col_g = 0xb4;
        col_b = 0x00;
      } else {
        col_r = 0xff;
        col_g = 0x00;
        col_b = 0x00;
      }
    } else {
      // WHITE
      col_r = 0xff;
      col_g = 0xff;
      col_b = 0xff;
    }
    valLedColor[i] = col_r<<16|col_g<<8|col_b;
    valDispColor[i] = M5.Display.color565(col_r,col_g,col_b);
  }
}

#if defined(UPLOAD_DATA_EN)
void drawWiFiSts(WIFI_STS wifiSts) {
  M5.Display.setFont(defaultFont);
  M5.Display.setTextSize(1.00);
  if (bDispEn) {
    switch (boardType) {
      case B_TYPE_M5STACK:
        M5.Display.setTextSize(1.25);
        M5.Display.setTextColor(WHITE);
        M5.Display.fillRect(16, 208, M5.Display.textWidth(WIFI_STS_STR[wifiSts]), M5.Display.fontHeight(), BLACK);
        M5.Display.setCursor(16, 208);
        M5.Display.print(WIFI_STS_STR[wifiSts]);
        break;
      case B_TYPE_M5STICKC:
        M5.Display.setTextColor(WHITE);
        M5.Display.fillRect(0, 148, M5.Display.textWidth(WIFI_STS_STR[wifiSts]), M5.Display.fontHeight(), BLACK);
        M5.Display.setCursor(0, 148);
        M5.Display.print(WIFI_STS_STR[wifiSts]);
        break;
      case B_TYPE_M5STICKC_PLUS:
        M5.Display.setTextColor(WHITE);
        M5.Display.fillRect(0, 208, M5.Display.textWidth(WIFI_STS_STR[wifiSts]), M5.Display.fontHeight(), BLACK);
        M5.Display.setCursor(0, 208);
        M5.Display.print(WIFI_STS_STR[wifiSts]);
        break;
      case B_TYPE_M5STACK_COREINK:
        M5.Display.fillRect(8, 184, M5.Display.textWidth(WIFI_STS_STR[wifiSts]), M5.Display.fontHeight(), WHITE);
        M5.Display.setCursor(8, 184);
        M5.Display.print(WIFI_STS_STR[wifiSts]);
        break;
      case B_TYPE_M5ATOM:
        break;
      default:
        break;
    }
  }
}
void updateWiFiSts(WIFI_STS wifiSts) {
  if (bDispLcdEn) {
    M5.Display.startWrite();
    drawWiFiSts(wifiSts);
    M5.Display.endWrite();
    M5.Display.display();
  }
}
#endif /* defined(UPLOAD_DATA_EN) */

void updateDisp() {
  if (bDispEn) {
    M5.Display.startWrite();
    switch (boardType) {
      case B_TYPE_M5STACK:
        M5.Display.fillRect(0, 20, M5.Display.width(), M5.Display.height()-20, BLACK);
        for (int i = 0; i < DISP_VAL_NUM; i++) {
          M5.Display.setTextSize(1.25);
          M5.Display.setFont(defaultFont);
          M5.Display.setTextColor(WHITE);
          M5.Display.setCursor(16, 58*i+40);
          M5.Display.print(D_TYPE_LABEL_STR[DISP_VAL[dispPage][i]]);
          M5.Display.setCursor(M5.Display.width()-46, 58*i+64);
          M5.Display.print(D_TYPE_UNIT_STR[DISP_VAL[dispPage][i]]);
          M5.Display.setTextSize(1.25);
          M5.Display.setFont(&fonts::Font6);
          M5.Display.setTextColor(valDispColor[DISP_VAL[dispPage][i]]);
          M5.Display.setCursor(M5.Display.width()-54-M5.Display.textWidth(valStr[DISP_VAL[dispPage][i]]), 58*i+36);
          M5.Display.setTextDatum(TR_DATUM);
          M5.Display.print(valStr[DISP_VAL[dispPage][i]]);
          M5.Display.setTextDatum(TL_DATUM);
        }
        M5.Display.setTextSize(1.25);
        M5.Display.setFont(defaultFont);
        M5.Display.setTextColor(WHITE);
        M5.Display.setCursor(M5.Display.width()-M5.Display.textWidth(drawValBattStr)-16, 208);
        M5.Display.print(drawValBattStr);
        break;
      case B_TYPE_M5STICKC:
        M5.Display.fillRect(0, 12, M5.Display.width(), M5.Display.height()-12, BLACK);
        for (int i = 0; i < DISP_VAL_NUM; i++) {
          M5.Display.setTextSize(1.0);
          M5.Display.setFont(defaultFont);
          M5.Display.setTextColor(WHITE);
          M5.Display.setCursor(0, 36*i+30);
          M5.Display.print(D_TYPE_LABEL_STR[DISP_VAL[dispPage][i]]);
          M5.Display.setCursor(M5.Display.width()-18, 36*i+50);
          M5.Display.print(D_TYPE_UNIT_STR[DISP_VAL[dispPage][i]]);
          M5.Display.setTextSize(0.8);
          M5.Display.setFont(&fonts::Font4);
          M5.Display.setTextColor(valDispColor[DISP_VAL[dispPage][i]]);
          M5.Display.setCursor(M5.Display.width()-20-M5.Display.textWidth(valStr[DISP_VAL[dispPage][i]]), 36*i+44);
          M5.Display.setTextDatum(TR_DATUM);
          M5.Display.print(valStr[DISP_VAL[dispPage][i]]);
          M5.Display.setTextDatum(TL_DATUM);
        }
        M5.Display.setTextSize(1.0);
        M5.Display.setFont(defaultFont);
        M5.Display.setTextColor(WHITE);
        M5.Display.setCursor(M5.Display.width()-M5.Display.textWidth(drawValBattStr), 148);
        M5.Display.print(drawValBattStr);
        break;
      case B_TYPE_M5STICKC_PLUS:
        M5.Display.fillRect(0, 20, M5.Display.width(), M5.Display.height()-20, BLACK);
        for (int i = 0; i < DISP_VAL_NUM; i++) {
          M5.Display.setTextSize(1.0);
          M5.Display.setFont(defaultFont);
          M5.Display.setTextColor(WHITE);
          M5.Display.setCursor(0, 50*i+40);
          M5.Display.print(D_TYPE_LABEL_STR[DISP_VAL[dispPage][i]]);
          M5.Display.setCursor(M5.Display.width()-24, 50*i+74);
          M5.Display.print(D_TYPE_UNIT_STR[DISP_VAL[dispPage][i]]);
          M5.Display.setTextSize(1.4);
          M5.Display.setFont(&fonts::Font4);
          M5.Display.setTextColor(valDispColor[DISP_VAL[dispPage][i]]);
          M5.Display.setCursor(M5.Display.width()-28-M5.Display.textWidth(valStr[DISP_VAL[dispPage][i]]), 50*i+60);
          M5.Display.setTextDatum(TR_DATUM);
          M5.Display.print(valStr[DISP_VAL[dispPage][i]]);
          M5.Display.setTextDatum(TL_DATUM);
        }
        M5.Display.setTextSize(1.0);
        M5.Display.setFont(defaultFont);
        M5.Display.setTextColor(WHITE);
        M5.Display.setCursor(M5.Display.width()-M5.Display.textWidth(drawValBattStr), 208);
        M5.Display.print(drawValBattStr);
        break;
      case B_TYPE_M5STACK_COREINK:
        M5.Display.clear();
        drawTitle();
        for (int i = 0; i < DISP_VAL_NUM; i++) {
          M5.Display.setTextSize(1.0);
          M5.Display.setFont(defaultFont);
          M5.Display.setCursor(8,48*i+32);
          M5.Display.print(D_TYPE_LABEL_STR[DISP_VAL[dispPage][i]]);
          M5.Display.setCursor(M5.Display.width()-32,48*i+70);
          M5.Display.print(D_TYPE_UNIT_STR[DISP_VAL[dispPage][i]]);
          M5.Display.setTextSize(1.0);
          M5.Display.setFont(&fonts::Font6);
          M5.Display.setCursor(M5.Display.width()-34-M5.Display.textWidth(valStr[DISP_VAL[dispPage][i]]),48*i+46);
          M5.Display.setTextDatum(TR_DATUM);
          M5.Display.print(valStr[DISP_VAL[dispPage][i]]);
          M5.Display.setTextDatum(TL_DATUM);
        }
        M5.Display.setTextSize(1.0);
        M5.Display.setFont(defaultFont);
        M5.Display.setCursor(M5.Display.width()-M5.Display.textWidth(drawValBattStr)-8,184);
        M5.Display.print(drawValBattStr);
        drawSts(currentSts);
        break;
      case B_TYPE_M5ATOM:
        break;
      default:
        break;
    }
#if defined(UPLOAD_DATA_EN)
    if (bDispEn) {
      drawWiFiSts(wifiSts);
  }
#endif /* defined(UPLOAD_DATA_EN) */
    M5.Display.endWrite();
    M5.Display.display();
  }
  if (bRGBLedEn) {
    UpdateLed(valLedColor[ledPage], ledTotalNum);
  }
}

void newDataCallback(const bme68xData data, const bsecOutputs outputs, Bsec2 bsec)
{
  uint8_t accuracy = 0;
  
  if (!outputs.nOutputs) {
    currentSts = D_STS_READ_DATA_ERR;
    return;
  }
  currentSts = D_STS_NO_ERR;

  bDataReady = true;
  for (int i = 0; i < outputs.nOutputs; i++) {
    const bsecData output  = outputs.output[i];
    switch (output.sensor_id) {
      case BSEC_OUTPUT_IAQ:
        accuracy = (int)output.accuracy;
        sensorData[D_TYPE_IAQ] = output.signal + OFS_VAL[boardType][D_TYPE_IAQ];
        if (accuracy >= 1 ) {
            dtostrf(sensorData[D_TYPE_IAQ],5,0,valStr[D_TYPE_IAQ]);
        } else {
            sprintf(valStr[D_TYPE_IAQ],"-");
        }
        break;
      case BSEC_OUTPUT_STATIC_IAQ:
        accuracy = (int)output.accuracy;
        sensorData[D_TYPE_sIAQ] = output.signal + OFS_VAL[boardType][D_TYPE_sIAQ];
        if (accuracy >= 1 ) {
            dtostrf(sensorData[D_TYPE_sIAQ],5,0,valStr[D_TYPE_sIAQ]);
        } else {
            sprintf(valStr[D_TYPE_sIAQ],"-");
        }
        break;
      case BSEC_OUTPUT_CO2_EQUIVALENT:
        accuracy = (int)output.accuracy;
        sensorData[D_TYPE_eCO2] = output.signal + OFS_VAL[boardType][D_TYPE_eCO2];
        if (accuracy >= 1 ) {
            dtostrf(sensorData[D_TYPE_eCO2],5,0,valStr[D_TYPE_eCO2]);
        } else {
            sprintf(valStr[D_TYPE_eCO2],"-");
        }
        break;
      case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
        accuracy = (int)output.accuracy;
        sensorData[D_TYPE_bVOC] = output.signal + OFS_VAL[boardType][D_TYPE_bVOC];
        if (accuracy >= 1 ) {
            dtostrf(sensorData[D_TYPE_bVOC],5,1,valStr[D_TYPE_bVOC]);
        } else {
            sprintf(valStr[D_TYPE_bVOC],"-");
        }
        break;
      case BSEC_OUTPUT_RAW_TEMPERATURE:
        sensorData[D_TYPE_RAW_TEMP] = output.signal + OFS_VAL[boardType][D_TYPE_RAW_TEMP];
        dtostrf(sensorData[D_TYPE_RAW_TEMP],5,1,valStr[D_TYPE_RAW_TEMP]);
        break;
      case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
        sensorData[D_TYPE_AMB_TEMP] = output.signal + OFS_VAL[boardType][D_TYPE_AMB_TEMP];
        dtostrf(sensorData[D_TYPE_AMB_TEMP],5,1,valStr[D_TYPE_AMB_TEMP]);
        break;
      case BSEC_OUTPUT_RAW_HUMIDITY:
        sensorData[D_TYPE_RAW_HUMI] = output.signal + OFS_VAL[boardType][D_TYPE_RAW_HUMI];
        dtostrf(sensorData[D_TYPE_RAW_HUMI],5,1,valStr[D_TYPE_RAW_HUMI]);
        break;
      case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
        sensorData[D_TYPE_AMB_HUMI] = output.signal + OFS_VAL[boardType][D_TYPE_AMB_HUMI];
        dtostrf(sensorData[D_TYPE_AMB_HUMI],5,1,valStr[D_TYPE_AMB_HUMI]);
        break;
      case BSEC_OUTPUT_RAW_PRESSURE:
        sensorData[D_TYPE_RAW_PRES] = output.signal/100.0 + OFS_VAL[boardType][D_TYPE_RAW_PRES];
        dtostrf(sensorData[D_TYPE_RAW_PRES],5,1,valStr[D_TYPE_RAW_PRES]);
        break;
      case BSEC_OUTPUT_RAW_GAS:
        break;
      case BSEC_OUTPUT_STABILIZATION_STATUS:
        break;
      case BSEC_OUTPUT_RUN_IN_STATUS:
        break;
      default:
        break;
    }
  }

  // Read battery level
  if (bBattEquipped) {
    int battery = M5.Power.getBatteryLevel()-BATT_LV_0;
    if (battery < 0) {
      battery = 0;
    }
    battery = battery*100/(100-BATT_LV_0);
    sprintf(valBattVolPerStr,"%d",battery);
    sprintf(drawValBattStr,"B:%3d%%",battery);
  }
  
  // serial print val
  for (int i = 0;i < D_TYPE_MAX; i++) {
    Serial.print(D_TYPE_LABEL_STR[i]);
    Serial.print(valStr[i]);
    Serial.print(D_TYPE_UNIT_STR[i]);
    Serial.print(", ");
  }
  if (bBattEquipped) {
    Serial.print(drawValBattStr);
  }
  Serial.println();

  // Update sensor value 
  UpdateAlertSts();
  if (bInitialUpdate || (update_interval_sec * 1000 < millis() - timeUpdateMs)) {
    bInitialUpdate = false;
    updateDisp();
    timeUpdateMs = millis();
  }
}

void initSensor() {
  // Initialize & setup sensor
  Wire2.begin(i2c_sda_pin,i2c_scl_pin);
  if (!envSensor.begin(BME68X_I2C_ADDR_LOW, Wire2)) {
    CheckBsecSts(envSensor);
    currentSts = D_STS_NO_DEVICE;
    updateStsWrite(currentSts);
    return;
  }
  bsecSensor sensorList[] = {
    BSEC_OUTPUT_IAQ,
    BSEC_OUTPUT_STATIC_IAQ,
    BSEC_OUTPUT_CO2_EQUIVALENT,
    BSEC_OUTPUT_BREATH_VOC_EQUIVALENT,
    BSEC_OUTPUT_RAW_TEMPERATURE,
    BSEC_OUTPUT_RAW_PRESSURE,
    BSEC_OUTPUT_RAW_HUMIDITY,
    BSEC_OUTPUT_RAW_GAS,
    BSEC_OUTPUT_STABILIZATION_STATUS,
    BSEC_OUTPUT_RUN_IN_STATUS,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
    BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY
  };
  if (!envSensor.updateSubscription(sensorList, ARRAY_LEN(sensorList), BSEC_SAMPLE_RATE_LP)) {
    CheckBsecSts(envSensor);
    currentSts = D_STS_NO_DEVICE;
    updateStsWrite(currentSts);
    return;
  }
  envSensor.attachCallback(newDataCallback);

  currentSts = D_STS_MEASURING;
  updateStsWrite(currentSts);
  Serial.println("Waiting for first measurement...");
}

void setup() {
  // Initialize M5
  InitM5();

#if defined(UPLOAD_DATA_EN)
  // Setup WiFi & Connect to Ambient
  // Create NetworkTask
  xTaskCreatePinnedToCore(
                  NetworkTask,
                  "nwtask",
                  4096,
                  NULL,
                  1,
                  NULL,
                  0);
#endif /* defined(UPLOAD_DATA_EN) */

  // Initialize & Setup
  initSensor();
}

void loop() {
  if (currentSts != D_STS_NO_DEVICE) {
    if (!envSensor.run()) {
      CheckBsecSts(envSensor);
    }
  }
#if defined(UPLOAD_DATA_EN)
  if ((bDataReady && bInitialUpload) || (upload_interval_sec * 1000 < millis() - timeUploadMs)) {
    if (bInitialUpload) {
      bInitialUpload = false;
      bDataReady = false;
    }
    // upload data to Ambient
    if (bNetworkReady && currentSts == D_STS_NO_ERR) {
      ambient.set(1, valStr[D_TYPE_AMB_TEMP]);
      ambient.set(2, valStr[D_TYPE_AMB_HUMI]);
      ambient.set(3, valStr[D_TYPE_RAW_PRES]);
      ambient.set(4, valStr[D_TYPE_IAQ]);
      ambient.set(5, valStr[D_TYPE_sIAQ]);
      ambient.set(6, valStr[D_TYPE_eCO2]);
      ambient.set(7, valStr[D_TYPE_bVOC]);
      if (bBattEquipped) {
        ambient.set(8, valBattVolPerStr);
      }
      if (ambient.send()) {
        Serial.println("Success to send data to Ambient.");
      } else {
        currentSts = D_STS_NETWORK_AMB_ERR;
        updateStsWrite(currentSts);
        Serial.println("Failed to send data to Ambient.");
      }
    }
    timeUploadMs = millis();
  }
#endif /* UPLOAD_DATA_EN */
  M5.update();
  if (M5.BtnA.wasClicked()) {
    if (bDispEn) {
      dispPage++;
      if (DISP_PAGE_NUM <= dispPage) {
        dispPage = 0;
      }
    }
    if (bRGBLedEn) {
      ledPage++;
      if (LED_PAGE_NUM <= ledPage) {
        UpdateLed(CRGB::Black, ledTotalNum);
        delay(200);
        ledPage = 0;
      }
    }
    updateDisp();
  } else if (M5.BtnA.wasHold()) {
    if (bDispLcdEn) {
      curLcdBri++;
      if (LCD_BRI_LV_NUM <= curLcdBri) {
        curLcdBri = LCD_BRI_LV_OFF;
      }
      M5.Display.setBrightness(lcdBriLv[curLcdBri]);
    }
    if (bRGBLedEn) {
      curLedBri++;
      if (LED_BRI_LV_NUM <= curLedBri) {
        curLedBri = LED_BRI_LV_OFF;
      }
      FastLED.setBrightness(ledBriLv[curLedBri]);
      UpdateLed(valLedColor[ledPage], ledTotalNum);
    }
  }
  delay(100);
}
