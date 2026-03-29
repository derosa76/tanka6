//per wifi, wifimanager e ota
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include <AsyncCaptivePortal.h>

// Librerie Shelly
#include <AsyncMDNS.h>
#include <ShellyController.h>

//codice spostato qui da screen_sensore.ino per problemi di compilazione
enum RsensStatus : uint8_t { RS_RED, RS_ORANGE, RS_GREEN, RS_GRAY };
enum RsensBtnType : uint8_t {
  RS_BTN_PAIR, RS_BTN_UNPAIR, RS_BTN_ELIMINA,
  RS_BTN_INACTIVE, RS_BTN_NONE
};


// ============================================
// SENSOR SOURCE — scelta di fabbrica (compile-time)
// Decommentare UNA delle due:
// ============================================
//#define TANKA_SENSOR_LOCAL    // Sensore cablato (HCSR04 / A02YYUW)
#define TANKA_SENSOR_REMOTE   // Sensore wireless via ESP-NOW

// ============================================
// SENSORE
// ============================================
#include <PersistentJson.h>
#include <SensorController_ema_and_trendline.h>
SensorController_ema_and_trendline sensor;

#ifdef TANKA_SENSOR_REMOTE
  #include "TankaRemoteNodeInterface.h"
  #include "SensorController_Wrapper_ESPNOW.h"
  PersistentJson espnowConfig("/espnow.json");
  TankaRemoteNodeInterface remoteInterface(&espnowConfig.doc());
  SensorController_Wrapper_ESPNOW espnowDriver(&remoteInterface);
#endif

// ============================================

AsyncMDNS mdns;
ShellyController shelly;

//per file system
#include <FS.h>
#include <LittleFS.h>

//per il CYD - display
#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

// Smooth fonts Poppins Bold (PROGMEM, anti-aliased)
#include "PoppinsBold80.h"
#include "PoppinsBold48.h"

extern int font_size;
extern unsigned long last_display;

//per il CYD - touchscreen
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#define TOUCH_CS 33
#define TOUCH_IRQ 36
#define TOUCH_CLK 25
#define TOUCH_MOSI 32
#define TOUCH_MISO 39
SPIClass touchSPI = SPIClass(VSPI);
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);
extern unsigned long last_touch;

PersistentJson config("/config.json");

// ============================================
// SISTEMA SCHERMATE
// ============================================
enum ScreenID {
  SCREEN_MAIN,
  SCREEN_HOME_IDLE,
  SCREEN_CALIBRAZIONE,
  SCREEN_POMPA,
  SCREEN_PIN,
  SCREEN_MENU_INST,
  SCREEN_SENSORE,
  SCREEN_POMPA_ADV,
  SCREEN_COUNT
};
void setScreen(ScreenID id);
void setupScreens();
void loopScreens();
void handleScreenTouch(int x, int y);

unsigned long last_touch_time = 0;
const unsigned long IDLE_TIMEOUT_HOME = 30000;
const unsigned long IDLE_TIMEOUT_SUB  = 60000;

// Installer unlock — sblocco persistente per 10 min dall'ultimo touch
bool menu_installatore_allowed = false;
const unsigned long MENU_INST_TIMEOUT = 600000;  // 10 minuti

bool isInstallerUnlocked() {
  if (!menu_installatore_allowed) return false;
  if (millis() - last_touch_time >= MENU_INST_TIMEOUT) {
    menu_installatore_allowed = false;
    return false;
  }
  return true;
}
// ============================================


void setup() {
  Serial.begin(115200);
  littleFS_setup();

  

  // ============================================
  // CONFIG — load + migration
  // ============================================
  if (config.begin()) {
    Serial.println("Config caricato da file");

    // Migrazione: aggiungi nuovi parametri se mancanti
    if (config["ap_subnet_third"].isNull()) {
      config["ap_subnet_third"] = 42;
      Serial.println("  + Aggiunto ap_subnet_third = 42");
    }
    if (config["pump_keepalive_interval"].isNull()) {
      config["pump_keepalive_interval"] = 60;
      Serial.println("  + Aggiunto pump_keepalive_interval = 60s");
    }
    if (config["pump_auto_off_timeout"].isNull()) {
      config["pump_auto_off_timeout"] = 90;
      Serial.println("  + Aggiunto pump_auto_off_timeout = 90s");
    }
    if (config["pump_mode"].isNull()) {
      config["pump_mode"] = 0;
      Serial.println("  + Aggiunto pump_mode = 0");
    }
    if (config["pump_H"].isNull()) {
      config["pump_H"] = 80.0;
      Serial.println("  + Aggiunto pump_H = 80%");
    }
    if (config["pump_L"].isNull()) {
      config["pump_L"] = 20.0;
      Serial.println("  + Aggiunto pump_L = 20%");
    }
    if (config["display_rotation"].isNull()) {
      config["display_rotation"] = 1;
      Serial.println("  + Aggiunto display_rotation = 1");
    }
    if (config["pump_check_delay"].isNull()) {
      config["pump_check_delay"] = 120;
      Serial.println("  + Aggiunto pump_check_delay = 120s");
    }
    if (config["pump_flow_threshold"].isNull()) {
      config["pump_flow_threshold"] = 5.0f;
      Serial.println("  + Aggiunto pump_flow_threshold = 5.0 mm/min");
    }
    if (config["pump_retry_wait"].isNull()) {
      config["pump_retry_wait"] = 1800;
      Serial.println("  + Aggiunto pump_retry_wait = 1800s");
    }
  } else {
    Serial.println("Config non trovato, creo default...");
    config["ap_subnet_third"] = 42;
    config["pump_keepalive_interval"] = 60;
    config["pump_auto_off_timeout"] = 90;
    config["pump_mode"] = 0;
    config["pump_H"] = 80.0;
    config["pump_L"] = 20.0;
    config["dist_min"] = 700.0;
    config["dist_max"] = 4000.0;
    config["calib_min_level"] = nullptr;
    config["calib_max_level"] = nullptr;
    config["display_rotation"]    = 1;
    config["pump_check_delay"]    = 120;
    config["pump_flow_threshold"] = 5.0f;
    config["pump_retry_wait"]     = 1800;
  }

  async_portal_setup();
  ota_setup();

  // ============================================
  // SETUP SENSORE
  // ============================================

#ifdef TANKA_SENSOR_LOCAL
  sensor.begin(35, 27);
  sensor.setDamping(10000);
  sensor.setDerivativeWindow(120000);
  sensor.setOffset(18.7);
  sensor.setStaleTimeout(5000);
#endif

#ifdef TANKA_SENSOR_REMOTE
  if (!espnowConfig.begin()) {
    Serial.println("ESP-NOW config non trovato, creo default...");
    espnowConfig["paired"]  = false;
    espnowConfig["mac"]     = "";
    espnowConfig["lmk"]     = "";
    espnowConfig["ch"]      = 0;
    espnowConfig["sys_id"]  = 56950;
    espnowConfig["desired_sleep"] = 5000;
  }

  uint32_t systemID = espnowConfig["sys_id"] | 56950UL;
  bool espnowOk = remoteInterface.begin(systemID);
  Serial.printf("[ESPNOW] begin: %s\n", espnowOk ? "OK" : "FAILED");

  //set dello sleep desired del sensore
  uint32_t desiredSleep = espnowConfig["desired_sleep"] | 5000UL; //fallback 2s nel caso non configurato  nel persistentjson
  remoteInterface.setDesiredSleep(desiredSleep);

  if (remoteInterface.isPaired()) {
    Serial.println("[ESPNOW] Gia' paired, pronto.");
  } else {
    Serial.println("[ESPNOW] Non paired. Usare menu Installatore > Sensore Remoto.");
  }

  sensor.begin(&espnowDriver);
  sensor.setDamping(10000);
  sensor.setDerivativeWindow(120000);
  sensor.setOffset(18.7);
  sensor.setStaleTimeout(30000);
#endif

  // ============================================

  display_setup();
  touchscreen_setup();
  calibrazione_ts_setup(false);

  setupScreens();
  setScreen(SCREEN_MAIN);
  last_touch_time = millis();

  // === ShellyController + AsyncMDNS ===
  mdns.begin("tanka");

  unsigned long keepalive_s = config["pump_keepalive_interval"] | 60UL;
  float auto_off_s = config["pump_auto_off_timeout"] | 90.0f;
  shelly.setAutoOffTimeout(auto_off_s);
  shelly.setKeepaliveInterval(keepalive_s * 1000);

  shelly.begin(mdns, "shelly*", 1,
               WiFi.softAPIP(), WiFi.softAPSubnetMask());
}

void loop() {
  ArduinoOTA.handle();
  async_portal_run();
  sensor.run();

#ifdef TANKA_SENSOR_REMOTE
  // Fast poll: quando c'è un pending pair, runListeningForSensors()
  // deve girare ogni loop (~5ms) per rispondere al sensore entro
  // la sua finestra DISCOVERY di 300ms. La schermata lo chiama solo
  // ogni 500ms — troppo lento.
  if (remoteInterface.isPairPending()) {
    remoteInterface.runListeningForSensors();
  }

  if (remoteInterface.configChanged()) {
    espnowConfig.commit();
    remoteInterface.clearConfigChanged();
    Serial.println("[ESPNOW] Config salvato.");
  }
  espnowConfig.run();
#endif

  touchscreen_run(5);
  loopScreens();

  config.run();

  serial_cmd_run();

  pompa_run();
}
