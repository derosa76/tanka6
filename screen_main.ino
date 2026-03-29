// screen_main.ino - Schermata principale TANKA
//
// Anti-flicker: cache con ridisegno selettivo + full refresh ogni 10s.
// Livello con 1 decimale (92.3%).
// Idle timeout gestito centralmente da screens.ino.

// ============================================
// LAYOUT CONSTANTS
// ============================================

const int mn_bar_h = 20;
const uint16_t mn_bar_bg = 0x18E3;

const int mn_level_y0 = 22;
const int mn_level_y1 = 127;

const int mn_info_y = 133;
const int mn_pump_y = 153;

const int mn_btn_y = 182;
const int mn_btn_h = 32;
const int mn_btn_w = 120;
const int mn_btn_inst_x = 100;   // centrato: (320-120)/2
const uint16_t mn_btn_col = TFT_ORANGE;
const uint16_t mn_btn_push = TFT_RED;

// Rotation buttons — top-right + bottom-left
// (same physical device corner after 180° rotation)
const int mn_rot_w  = 24;
const int mn_rot_h  = 18;
const int mn_rot_tr_x = 294;   // top-right
const int mn_rot_tr_y = 1;
const int mn_rot_bl_x = 2;     // bottom-left
const int mn_rot_bl_y = 221;

// ============================================
// CACHE
// ============================================

struct {
  int level_tenths;     // 923 = 92.3% (-9999 = invalidato)
  bool level_valid;
  bool level_nan;
  bool pump_on;
  int pump_mode;
  float pump_H;
  float pump_L;
  int pump_mode_ps;     // cache separata per riga pompa/sensore
  float sensor_mm;
  float sensor_rate;
  bool wifi_sta;
  int ap_clients;
  bool shelly_online;
  int rssi;
  bool sensor_stale;       // status bar
  bool level_stale;        // level zone
  bool ps_stale;           // pump/sensor zone
  unsigned long last_full_refresh;
} mn_cache;

bool mn_btn_inst_pressed = false;
bool mn_btn_rot_pressed = false;

void mn_cache_invalidate() {
  mn_cache.level_tenths = -9999;
  mn_cache.level_valid = false;
  mn_cache.level_nan = false;
  mn_cache.pump_on = false;
  mn_cache.pump_mode = -1;
  mn_cache.pump_H = -1;
  mn_cache.pump_L = -1;
  mn_cache.pump_mode_ps = -1;
  mn_cache.sensor_mm = -1;
  mn_cache.sensor_rate = -9999;
  mn_cache.wifi_sta = false;
  mn_cache.ap_clients = -1;
  mn_cache.shelly_online = false;
  mn_cache.rssi = 1;
  mn_cache.sensor_stale = false;
  mn_cache.level_stale = false;
  mn_cache.ps_stale = false;
  mn_cache.last_full_refresh = 0;
}

// ============================================
// DRAW HELPERS
// ============================================

void mn_draw_btn(int x, int y, int w, int h, const char* text, uint16_t col) {
  tft.fillRoundRect(x, y, w, h, 5, col);
  tft.setTextColor(TFT_BLACK, col);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.setTextPadding(0);
  tft.drawString(text, x + w / 2, y + h / 2);
}

void mn_draw_dot(int x, int y, bool ok) {
  tft.fillCircle(x, y, 3, ok ? TFT_GREEN : TFT_RED);
}

void mn_draw_rotate_btn(int x, int y) {
  uint16_t bg = 0x2945;   // dark grey-blue
  uint16_t fg = TFT_LIGHTGREY;
  tft.fillRoundRect(x, y, mn_rot_w, mn_rot_h, 3, bg);
  tft.drawRoundRect(x, y, mn_rot_w, mn_rot_h, 3, TFT_DARKGREY);
  int cx = x + mn_rot_w / 2;
  int cy = y + mn_rot_h / 2;
  // Cerchio (anello di rotazione) con gap in alto
  tft.drawCircle(cx, cy, 5, fg);
  // Cancella gap in alto per creare arco aperto
  tft.fillRect(cx - 1, cy - 7, 6, 3, bg);
  // Freccia triangolare al bordo destro del gap (direzione oraria)
  tft.fillTriangle(cx + 2, cy - 7,    // punta in alto
                   cx + 5, cy - 4,    // basso-destra
                   cx + 1, cy - 3,    // basso-sinistra
                   fg);
}

// ============================================
// STATUS BAR
// ============================================

void mn_update_status_bar(bool force) {
  bool wifi_sta = (WiFi.status() == WL_CONNECTED);
  int ap_clients = WiFi.softAPgetStationNum();
  bool shelly_on = shelly.isOnline();
  bool stale = sensor.isStale();
  int rssi = wifi_sta ? WiFi.RSSI() : 0;

  if (!force &&
      wifi_sta == mn_cache.wifi_sta &&
      ap_clients == mn_cache.ap_clients &&
      shelly_on == mn_cache.shelly_online &&
      stale == mn_cache.sensor_stale &&
      rssi == mn_cache.rssi) return;

  // Riempie solo fino al bottone rotazione top-right (preserva il bottone)
  tft.fillRect(0, 0, mn_rot_tr_x, mn_bar_h, mn_bar_bg);
  tft.setTextFont(1);
  tft.setTextDatum(TL_DATUM);
  tft.setTextPadding(0);
  tft.setTextColor(TFT_LIGHTGREY, mn_bar_bg);

  int x = 3, ty = 6;

  tft.drawString("WiFi Cl:", x, ty);
  x += 50;
  mn_draw_dot(x, 10, wifi_sta);
  x += 9;

  tft.drawString("Access P:", x, ty);
  x += 56;
  mn_draw_dot(x, 10, true);
  x += 7;
  char apBuf[6];
  snprintf(apBuf, sizeof(apBuf), "(%d)", ap_clients);
  tft.drawString(apBuf, x, ty);
  x += 24;

  tft.drawString("Quadro P:", x, ty);
  x += 56;
  mn_draw_dot(x, 10, shelly_on);
  x += 9;

  tft.drawString("Sens:", x, ty);
  x += 32;
  mn_draw_dot(x, 10, sensor.hasReading() && !stale);
  x += 9;

  if (wifi_sta) {
    char rBuf[10];
    snprintf(rBuf, sizeof(rBuf), "%ddB", rssi);
    tft.drawString(rBuf, x, ty);
  }

  mn_cache.wifi_sta = wifi_sta;
  mn_cache.ap_clients = ap_clients;
  mn_cache.shelly_online = shelly_on;
  mn_cache.sensor_stale = stale;
  mn_cache.rssi = rssi;
}

// ============================================
// LIVELLO % con decimale
// ============================================

void mn_update_level(bool force) {
  bool stale = sensor.isStale();
  bool valid = !config["calib_max_level"].isNull() && !config["calib_min_level"].isNull();
  int tenths = 0;
  bool isNan = false;

  if (valid && !stale) {
    float lev = sensor.getLevel(config["calib_max_level"].as<float>(),
                                config["calib_min_level"].as<float>());
    if (isnan(lev)) {
      isNan = true;
    } else {
      tenths = (int)roundf(lev * 10.0f);
    }
  }

  if (!force &&
      valid == mn_cache.level_valid &&
      isNan == mn_cache.level_nan &&
      stale == mn_cache.level_stale &&
      tenths == mn_cache.level_tenths) return;

  tft.fillRect(0, mn_level_y0, 320, mn_level_y1 - mn_level_y0, TFT_BLACK);
  int centerY = mn_level_y0 + (mn_level_y1 - mn_level_y0) / 2;

  if (!valid) {
    tft.setTextFont(4);
    tft.setTextDatum(MC_DATUM);
    tft.setTextPadding(0);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("NON CALIBRATO", 160, centerY);
  } else if (stale || !sensor.hasReading()) {
    tft.loadFont(PoppinsBold48);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("---", 160, centerY);
    tft.unloadFont();
  } else if (isNan) {
    tft.setTextFont(4);
    tft.setTextDatum(MC_DATUM);
    tft.setTextPadding(0);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("ERR CAL", 160, centerY);
  } else {
    // Smooth font Poppins Bold 48px — "92.3%" come stringa unica
    float levelF = tenths / 10.0f;
    char levelStr[12];
    snprintf(levelStr, sizeof(levelStr), "%.1f%%", levelF);

    tft.loadFont(PoppinsBold48);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(levelStr, 160, centerY);
    tft.unloadFont();
  }

  mn_cache.level_valid = valid;
  mn_cache.level_nan = isNan;
  mn_cache.level_tenths = tenths;
  mn_cache.level_stale = stale;
}

// ============================================
// INFO LINE
// ============================================

void mn_update_info(bool force) {
  int mode = config["pump_mode"] | 0;
  float H = config["pump_H"] | 80.0f;
  float L = config["pump_L"] | 20.0f;

  if (!force &&
      mode == mn_cache.pump_mode &&
      H == mn_cache.pump_H &&
      L == mn_cache.pump_L) return;

  tft.fillRect(0, mn_info_y - 2, 320, 18, TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextDatum(TL_DATUM);
  tft.setTextPadding(0);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawString("LOGICA POMPA:", 15, mn_info_y);

  const char* modeStr = mode == 0 ? "OFF" : (mode == 1 ? "RIEMPI" : "SVUOTA");
  tft.setTextColor(mode == 0 ? TFT_DARKGREY : TFT_ORANGE, TFT_BLACK);
  tft.drawString(modeStr, 130, mn_info_y);

  if (mode != 0) {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    char hBuf[10], lBuf[10];
    snprintf(hBuf, sizeof(hBuf), "H:%.0f%%", H);
    snprintf(lBuf, sizeof(lBuf), "L:%.0f%%", L);
    tft.drawString(hBuf, 200, mn_info_y);
    tft.drawString(lBuf, 258, mn_info_y);
  }

  mn_cache.pump_mode = mode;
  mn_cache.pump_H = H;
  mn_cache.pump_L = L;
}

// ============================================
// PUMP STATUS + SENSOR
// ============================================

void mn_update_pump_sensor(bool force) {
  int mode = config["pump_mode"] | 0;
  bool pOn = pump_running;
  bool stale = sensor.isStale();
  float mm = roundf(sensor.getDistance() * 10.0f) / 10.0f;
  float rate = sensor.getDerivative();  // mm/min, NAN if not ready
  float rateR = isnan(rate) ? -9999.0f : roundf(rate * 10.0f) / 10.0f;

  if (!force &&
      pOn == mn_cache.pump_on &&
      mode == mn_cache.pump_mode_ps &&
      stale == mn_cache.ps_stale &&
      mm == mn_cache.sensor_mm &&
      rateR == mn_cache.sensor_rate) return;

  tft.fillRect(0, mn_pump_y - 2, 320, 18, TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextPadding(0);

  // --- Pump status (left side) ---
  if (mode == 0) {
    tft.fillRect(15, mn_pump_y + 1, 12, 12, TFT_DARKGREY);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("POMPA: ---", 32, mn_pump_y);
  } else {
    uint16_t col = pOn ? TFT_GREEN : TFT_DARKGREY;
    tft.fillRect(15, mn_pump_y + 1, 12, 12, col);
    tft.setTextDatum(TL_DATUM);
    tft.setTextColor(col, TFT_BLACK);
    tft.drawString(pOn ? "POMPA ON " : "POMPA OFF", 32, mn_pump_y);
  }

  // --- Distance + Trend (right side) ---
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setTextDatum(TR_DATUM);

  if (stale || !sensor.hasReading()) {
    tft.drawString("trend:---", 305, mn_pump_y);
    tft.drawString("---", 170, mn_pump_y);
  } else {
    // Trend (rightmost)
    char rateBuf[24];
    if (rateR != -9999.0f) {
      snprintf(rateBuf, sizeof(rateBuf), "trend:%+.1f mm/min", rateR);
    } else {
      snprintf(rateBuf, sizeof(rateBuf), "trend:n/a");
    }
    tft.drawString(rateBuf, 305, mn_pump_y);

    // Distance (left of trend)
    char sBuf[16];
    snprintf(sBuf, sizeof(sBuf), "%.1fmm", mm);
    tft.drawString(sBuf, 170, mn_pump_y);
  }

  mn_cache.pump_on = pOn;
  mn_cache.pump_mode_ps = mode;
  mn_cache.ps_stale = stale;
  mn_cache.sensor_mm = mm;
  mn_cache.sensor_rate = rateR;
}

// ============================================
// SCREEN INTERFACE
// ============================================

void main_Draw() {
  mn_cache_invalidate();  // resetta cache
  tft.fillScreen(TFT_BLACK);

  tft.drawFastHLine(0, mn_bar_h, 320, 0x2104);
  tft.drawFastHLine(10, mn_level_y1 + 1, 300, 0x2104);
  tft.drawFastHLine(10, 175, 300, 0x2104);

  mn_draw_btn(mn_btn_inst_x, mn_btn_y, mn_btn_w, mn_btn_h, "INSTALLATORE", mn_btn_col);

  mn_update_status_bar(true);
  mn_update_level(true);
  mn_update_info(true);
  mn_update_pump_sensor(true);

  // Bottoni rotazione — disegnati dopo la status bar per non essere sovrascritti
  mn_draw_rotate_btn(mn_rot_tr_x, mn_rot_tr_y);
  mn_draw_rotate_btn(mn_rot_bl_x, mn_rot_bl_y);

  mn_cache.last_full_refresh = millis();
}

void main_Update() {
  static uint32_t lastUpdate = 0;
  if (millis() - lastUpdate < 500) return;
  lastUpdate = millis();

  bool force = (millis() - mn_cache.last_full_refresh >= 10000);
  if (force) {
    tft.drawFastHLine(0, mn_bar_h, 320, 0x2104);
    tft.drawFastHLine(10, mn_level_y1 + 1, 300, 0x2104);
    tft.drawFastHLine(10, 175, 300, 0x2104);
    // Ridisegna i bottoni rotazione (il full refresh non li tocca,
    // ma per sicurezza li ridisegniamo — sono piccoli e veloci)
    mn_draw_rotate_btn(mn_rot_tr_x, mn_rot_tr_y);
    mn_draw_rotate_btn(mn_rot_bl_x, mn_rot_bl_y);
    mn_cache.last_full_refresh = millis();
  }

  mn_update_status_bar(force);
  mn_update_level(force);
  mn_update_info(force);
  mn_update_pump_sensor(force);
}

void main_Touch(int tx, int ty) {
  // --- Rotation buttons — flag su touch, azione su release ---
  if (tx >= mn_rot_tr_x && tx < mn_rot_tr_x + mn_rot_w &&
      ty >= mn_rot_tr_y && ty < mn_rot_tr_y + mn_rot_h) {
    mn_btn_rot_pressed = true;
    return;
  }
  if (tx >= mn_rot_bl_x && tx < mn_rot_bl_x + mn_rot_w &&
      ty >= mn_rot_bl_y && ty < mn_rot_bl_y + mn_rot_h) {
    mn_btn_rot_pressed = true;
    return;
  }

  // --- INSTALLATORE button ---
  if (ty >= mn_btn_y && ty <= mn_btn_y + mn_btn_h &&
      tx >= mn_btn_inst_x && tx <= mn_btn_inst_x + mn_btn_w) {
    mn_btn_inst_pressed = true;
    mn_draw_btn(mn_btn_inst_x, mn_btn_y, mn_btn_w, mn_btn_h, "INSTALLATORE", mn_btn_push);
  }
}

void main_Release() {
  if (mn_btn_rot_pressed) {
    mn_btn_rot_pressed = false;
    toggleRotation();
    return;  // toggleRotation ridisegna tutto — non serve proseguire
  }
  if (mn_btn_inst_pressed) {
    mn_btn_inst_pressed = false;
    mn_draw_btn(mn_btn_inst_x, mn_btn_y, mn_btn_w, mn_btn_h, "INSTALLATORE", mn_btn_col);
    if (isInstallerUnlocked()) {
      setScreen(SCREEN_MENU_INST);
    } else {
      setScreen(SCREEN_PIN);
    }
  }
}
