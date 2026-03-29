// screen_home_idle.ino - Schermata IDLE (dopo 30s senza touch)
//
// Solo % grande con decimale + stato pompa.
// Tap ovunque → torna a HOME.
// Anti-flicker: cache + full refresh ogni 10s.

// ============================================
// CACHE
// ============================================

struct {
  int level_tenths;     // 923 = 92.3%  (-9999 = non disegnato)
  bool level_valid;
  bool level_nan;
  bool pump_on;
  int pump_mode;
  bool level_stale;
  unsigned long last_full_refresh;
} idle_cache;

bool idle_touched = false;

void idle_cache_invalidate() {
  idle_cache.level_tenths = -9999;
  idle_cache.level_valid = false;
  idle_cache.level_nan = false;
  idle_cache.pump_on = false;
  idle_cache.pump_mode = -1;
  idle_cache.level_stale = false;
  idle_cache.last_full_refresh = 0;
}

// ============================================
// DRAW
// ============================================

void idle_draw_level(bool force) {
  bool valid = !config["calib_max_level"].isNull() && !config["calib_min_level"].isNull();
  bool stale = sensor.isStale();
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
      valid == idle_cache.level_valid &&
      isNan == idle_cache.level_nan &&
      stale == idle_cache.level_stale &&
      tenths == idle_cache.level_tenths) return;

  // Pulisci zona livello (0-160)
  tft.fillRect(0, 0, 320, 160, TFT_BLACK);

  // Centro verticale nella zona 0-160 = y80
  int centerY = 80;

  if (!valid) {
    tft.setTextFont(4);
    tft.setTextDatum(MC_DATUM);
    tft.setTextPadding(0);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("NON CALIBRATO", 160, centerY);
  } else if (stale || !sensor.hasReading()) {
    tft.loadFont(PoppinsBold80);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("---", 160, centerY);
    tft.unloadFont();
  } else if (isNan) {
    tft.setTextFont(4);
    tft.setTextDatum(MC_DATUM);
    tft.setTextPadding(0);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("ERR SENSORE", 160, centerY);
  } else {
    // Smooth font Poppins Bold 80px — "92.3%" grande
    float levelF = tenths / 10.0f;
    char levelStr[12];
    snprintf(levelStr, sizeof(levelStr), "%.1f%%", levelF);

    tft.loadFont(PoppinsBold80);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(levelStr, 160, centerY);
    tft.unloadFont();
  }

  idle_cache.level_valid = valid;
  idle_cache.level_nan = isNan;
  idle_cache.level_tenths = tenths;
  idle_cache.level_stale = stale;
}

void idle_draw_pump(bool force) {
  int mode = config["pump_mode"] | 0;
  bool pOn = pump_running;

  if (!force &&
      pOn == idle_cache.pump_on &&
      mode == idle_cache.pump_mode) return;

  // Zona pompa: y 170-200, centrata
  tft.fillRect(0, 170, 320, 30, TFT_BLACK);
  tft.setTextFont(4);  // 26px, ben visibile
  tft.setTextDatum(MC_DATUM);
  tft.setTextPadding(0);

  if (mode == 0) {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("POMPA: ---", 160, 185);
  } else {
    uint16_t col = pOn ? TFT_GREEN : TFT_DARKGREY;
    // Quadrato 16x16 + testo
    int textW = tft.textWidth(pOn ? "POMPA ON" : "POMPA OFF");
    int totalW = 18 + textW;
    int startX = 160 - totalW / 2;

    tft.fillRect(startX, 177, 16, 16, col);

    tft.setTextColor(col, TFT_BLACK);
    tft.setTextDatum(ML_DATUM);
    tft.drawString(pOn ? "POMPA ON" : "POMPA OFF", startX + 20, 185);
  }

  idle_cache.pump_on = pOn;
  idle_cache.pump_mode = mode;
}

// ============================================
// SCREEN INTERFACE
// ============================================

void idle_Draw() {
  idle_cache_invalidate();
  idle_touched = false;
  tft.fillScreen(TFT_BLACK);

  idle_draw_level(true);
  idle_draw_pump(true);

  idle_cache.last_full_refresh = millis();
}

void idle_Update() {
  static uint32_t lastUpdate = 0;
  if (millis() - lastUpdate < 500) return;
  lastUpdate = millis();

  bool force = (millis() - idle_cache.last_full_refresh >= 10000);
  if (force) idle_cache.last_full_refresh = millis();

  idle_draw_level(force);
  idle_draw_pump(force);
}

void idle_Touch(int tx, int ty) {
  idle_touched = true;
}

void idle_Release() {
  if (idle_touched) {
    idle_touched = false;
    setScreen(SCREEN_MAIN);
  }
}
