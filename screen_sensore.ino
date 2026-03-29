// screen_sensore.ino - Schermata sensore remoto
//
// Lista unificata. Colori e testo basati sullo stato reale:
//   Verde  CONNESSO  = paired + riceve dati       → UNPAIR
//   Grigio OFFLINE   = paired ma nessun dato       → ELIMINA
//   Arancio ATTESA   = pending pair o unpair        → ... (inattivo)
//   Rosso  TROVATO   = DISCOVERY ricevuto           → PAIR
//
// Tipi fuori da ifdef (Arduino IDE auto-genera prototipi prima dell'ifdef).

// ============================================
// ROW DATA TYPES
// ============================================

// enum RsensStatus : uint8_t { RS_RED, RS_ORANGE, RS_GREEN, RS_GRAY };
// enum RsensBtnType : uint8_t {
//   RS_BTN_PAIR, RS_BTN_UNPAIR, RS_BTN_ELIMINA,
//   RS_BTN_INACTIVE, RS_BTN_NONE
// };

struct RsensRowData {
  bool         active;
  char         label[22];    // "Name______ [XX:YY]" truncated
  RsensStatus  status;
  RsensBtnType btnType;
  int          discIndex;    // index in discovered list, -1 = paired/pending
};

#ifdef TANKA_SENSOR_REMOTE

// ============================================
// LAYOUT
// ============================================

const int rsens_title_y    = 2;
const int rsens_anim_y     = 20;
const int rsens_sep1_y     = 36;

const int rsens_row_y0     = 42;
const int rsens_row_h      = 34;
const int rsens_row_gap    = 2;
const int rsens_max_rows   = 4;

const int rsens_dot_x      = 8;
const int rsens_name_x     = 20;
const int rsens_status_x   = 155;
const int rsens_btn_x      = 240;
const int rsens_btn_w      = 72;
const int rsens_btn_h      = 26;

const int rsens_sep2_y     = 186;
const int rsens_msg_y      = 190;
const int rsens_home_x     = 100;
const int rsens_home_y     = 212;
const int rsens_home_w     = 120;
const int rsens_home_h     = 26;

// ============================================
// COLORI
// ============================================

const uint16_t rsens_bg        = TFT_BLACK;
const uint16_t rsens_title_col = TFT_CYAN;
const uint16_t rsens_row_bg    = 0x1082;

// ============================================
// ROW DATA INSTANCES
// ============================================

RsensRowData rsens_rows[4];
RsensRowData rsens_rows_prev[4];
int rsens_row_count = 0;
int rsens_row_count_prev = -1;

// ============================================
// STATE
// ============================================

int  rsens_pressed_row   = -1;
bool rsens_home_pressed  = false;
int  rsens_anim_dots     = 0;
unsigned long rsens_pair_completed_at = 0;
bool rsens_was_pair_pending = false;

// ============================================
// HELPERS
// ============================================

int rsens_row_y(int i) {
  return rsens_row_y0 + i * (rsens_row_h + rsens_row_gap);
}

void rsens_draw_btn(int x, int y, int w, int h, const char* text, uint16_t col) {
  tft.fillRoundRect(x, y, w, h, 4, col);
  tft.setTextColor(TFT_BLACK, col);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.setTextPadding(0);
  tft.drawString(text, x + w / 2, y + h / 2);
}

void rsens_make_label(char* buf, size_t bufSize,
                      const char* name, const uint8_t* mac) {
  char shortName[11];   // max 10 chars + null
  strncpy(shortName, name, 10);
  shortName[10] = '\0';
  snprintf(buf, bufSize, "%s [%02X:%02X]", shortName, mac[4], mac[5]);
}

void rsens_make_label_config(char* buf, size_t bufSize) {
  const char* name   = espnowConfig["device_name"] | "Sensore";
  const char* macStr = espnowConfig["mac"] | "";
  int macLen = strlen(macStr);
  char shortName[11];
  strncpy(shortName, name, 10);
  shortName[10] = '\0';
  if (macLen >= 5) {
    snprintf(buf, bufSize, "%s [%s]", shortName, &macStr[macLen - 5]);
  } else {
    snprintf(buf, bufSize, "%s [??:??]", shortName);
  }
}

uint16_t rsens_dot_color(RsensStatus s) {
  switch (s) {
    case RS_GREEN:  return TFT_GREEN;
    case RS_ORANGE: return TFT_ORANGE;
    case RS_GRAY:   return TFT_DARKGREY;
    default:        return TFT_RED;
  }
}

const char* rsens_status_text(RsensStatus s) {
  switch (s) {
    case RS_GREEN:  return "CONNESSO";
    case RS_ORANGE: return "ATTESA";
    case RS_GRAY:   return "OFFLINE";
    default:        return "TROVATO";
  }
}

const char* rsens_btn_text(RsensBtnType t) {
  switch (t) {
    case RS_BTN_PAIR:    return "PAIR";
    case RS_BTN_UNPAIR:  return "UNPAIR";
    case RS_BTN_ELIMINA: return "ELIMINA";
    case RS_BTN_INACTIVE: return "...";
    default: return "";
  }
}

uint16_t rsens_btn_color(RsensBtnType t) {
  switch (t) {
    case RS_BTN_PAIR:    return TFT_GREEN;
    case RS_BTN_UNPAIR:  return TFT_ORANGE;
    case RS_BTN_ELIMINA: return TFT_RED;
    case RS_BTN_INACTIVE: return TFT_DARKGREY;
    default: return TFT_DARKGREY;
  }
}

// ============================================
// ROW DRAWING
// ============================================

void rsens_draw_row(int i) {
  const RsensRowData& r = rsens_rows[i];
  int y = rsens_row_y(i);

  if (!r.active) {
    tft.fillRect(4, y, 312, rsens_row_h, rsens_bg);
    return;
  }

  // Background
  tft.fillRect(4, y, 312, rsens_row_h, rsens_row_bg);
  int cy = y + rsens_row_h / 2;

  // Dot
  tft.fillCircle(rsens_dot_x, cy, 4, rsens_dot_color(r.status));

  // Name
  tft.setTextColor(TFT_WHITE, rsens_row_bg);
  tft.setTextFont(2);
  tft.setTextDatum(ML_DATUM);
  tft.setTextPadding(0);
  tft.drawString(r.label, rsens_name_x, cy);

  // Status text (colored like the dot)
  tft.setTextColor(rsens_dot_color(r.status), rsens_row_bg);
  tft.setTextDatum(ML_DATUM);
  tft.drawString(rsens_status_text(r.status), rsens_status_x, cy);

  // Button
  if (r.btnType != RS_BTN_NONE) {
    int btn_y = y + (rsens_row_h - rsens_btn_h) / 2;
    bool pressed = (rsens_pressed_row == i);
    uint16_t col = pressed ? 0x7800 : rsens_btn_color(r.btnType);
    rsens_draw_btn(rsens_btn_x, btn_y, rsens_btn_w, rsens_btn_h,
                   rsens_btn_text(r.btnType), col);
  }
}

void rsens_draw_empty() {
  int yc = rsens_row_y0 + (rsens_max_rows * (rsens_row_h + rsens_row_gap)) / 2;
  tft.setTextColor(TFT_DARKGREY, rsens_bg);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("Nessun sensore", 160, yc);
}

// ============================================
// ROW DATA BUILDING
// ============================================

void rsens_build_rows() {
  memset(rsens_rows, 0, sizeof(rsens_rows));
  int idx = 0;

  // --- 1) Paired sensor ---
  if (remoteInterface.isPaired()) {
    RsensRowData& r = rsens_rows[idx];
    r.active = true;
    rsens_make_label_config(r.label, sizeof(r.label));
    r.discIndex = -1;

    if (remoteInterface.isUnpairPending()) {
      r.status  = RS_ORANGE;
      r.btnType = RS_BTN_INACTIVE;
    } else if (rsens_pair_completed_at > 0 &&
               millis() - rsens_pair_completed_at < 30000) {
      // Appena paired — attesa primo DATA (~10s)
      r.status  = RS_ORANGE;
      r.btnType = RS_BTN_INACTIVE;
    } else if (sensor.hasReading() && !sensor.isStale()) {
      r.status  = RS_GREEN;
      r.btnType = RS_BTN_UNPAIR;
    } else {
      r.status  = RS_GRAY;
      r.btnType = RS_BTN_ELIMINA;
    }
    idx++;
  }
  // --- 2) Pending pair (not yet paired) ---
  else if (remoteInterface.isPairPending()) {
    RsensRowData& r = rsens_rows[idx];
    r.active = true;
    char shortName[11];
    strncpy(shortName, remoteInterface.getPendingPairDeviceName(), 10);
    shortName[10] = '\0';
    snprintf(r.label, sizeof(r.label), "%s [%02X:%02X]",
             shortName,
             remoteInterface.getPendingPairMac()[4],
             remoteInterface.getPendingPairMac()[5]);
    r.discIndex = -1;
    r.status  = RS_ORANGE;
    r.btnType = RS_BTN_INACTIVE;
    idx++;
  }

  // --- 3) Discovered sensors ---
  int discCount = remoteInterface.getDiscoveredCount();
  for (int i = 0; i < discCount && idx < rsens_max_rows; i++) {
    const DiscoveredSensor* s = remoteInterface.getDiscoveredSensor(i);
    if (!s) continue;
    if (millis() - s->lastSeen > 180000) continue;  // stale >3 min

    // Skip if matches pending pair MAC (shown as orange above)
    if (remoteInterface.isPairPending() &&
        memcmp(s->mac, remoteInterface.getPendingPairMac(), 6) == 0) continue;

    RsensRowData& r = rsens_rows[idx];
    r.active = true;
    rsens_make_label(r.label, sizeof(r.label), s->deviceName, s->mac);
    r.discIndex = i;
    r.status  = RS_RED;
    r.btnType = RS_BTN_PAIR;
    idx++;
  }

  rsens_row_count = idx;
  for (int i = idx; i < rsens_max_rows; i++) {
    rsens_rows[i].active = false;
  }
}

bool rsens_rows_changed() {
  if (rsens_row_count != rsens_row_count_prev) return true;
  return memcmp(rsens_rows, rsens_rows_prev, sizeof(rsens_rows)) != 0;
}

void rsens_save_cache() {
  memcpy(rsens_rows_prev, rsens_rows, sizeof(rsens_rows));
  rsens_row_count_prev = rsens_row_count;
}

void rsens_redraw_all() {
  tft.fillRect(0, rsens_row_y0, 320, rsens_sep2_y - rsens_row_y0, rsens_bg);
  if (rsens_row_count == 0) {
    rsens_draw_empty();
  } else {
    for (int i = 0; i < rsens_row_count; i++) {
      rsens_draw_row(i);
    }
  }
}

// ============================================
// ANIMATION
// ============================================

void rsens_draw_anim() {
  tft.fillRect(0, rsens_anim_y, 320, 14, rsens_bg);
  tft.setTextFont(2);
  tft.setTextDatum(TL_DATUM);
  tft.setTextPadding(0);
  if (remoteInterface.isListening()) {
    const char* dots[] = {"   ", ".  ", ".. ", "..."};
    char buf[20];
    snprintf(buf, sizeof(buf), "Ascolto%s", dots[rsens_anim_dots]);
    tft.setTextColor(TFT_YELLOW, rsens_bg);
    tft.drawString(buf, 12, rsens_anim_y);
  } else {
    tft.setTextColor(TFT_DARKGREY, rsens_bg);
    tft.drawString("Ascolto terminato", 12, rsens_anim_y);
  }
}

// ============================================
// MESSAGE
// ============================================

void rsens_show_msg(const char* msg, uint16_t col) {
  tft.fillRect(0, rsens_msg_y, 320, 18, rsens_bg);
  tft.setTextColor(col, rsens_bg);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.setTextPadding(0);
  tft.drawString(msg, 160, rsens_msg_y + 6);
}

// ============================================
// SCREEN INTERFACE
// ============================================

void rsens_Draw() {
  rsens_pressed_row = -1;
  rsens_home_pressed = false;
  rsens_anim_dots = 0;
  rsens_pair_completed_at = 0;
  rsens_was_pair_pending = false;
  rsens_row_count_prev = -1;
  memset(rsens_rows_prev, 0, sizeof(rsens_rows_prev));

  tft.fillScreen(rsens_bg);

  tft.setTextColor(rsens_title_col, rsens_bg);
  tft.setTextFont(2);
  tft.setTextDatum(TC_DATUM);
  tft.setTextPadding(0);
  tft.drawString("SENSORE REMOTO", 160, rsens_title_y);

  rsens_draw_anim();
  tft.drawFastHLine(10, rsens_sep1_y, 300, TFT_DARKGREY);
  tft.drawFastHLine(10, rsens_sep2_y, 300, TFT_DARKGREY);

  remoteInterface.startListeningForSensors(600000);

  rsens_build_rows();
  rsens_redraw_all();
  rsens_save_cache();

  rsens_draw_btn(rsens_home_x, rsens_home_y, rsens_home_w, rsens_home_h,
                 "HOME", TFT_ORANGE);
}

void rsens_Update() {
  static uint32_t lastUpd = 0;
  if (millis() - lastUpd < 500) return;
  lastUpd = millis();

  // Keep-alive
  if (remoteInterface.isListening()) {
    last_touch_time = millis();
  }

  // Drive listener
  remoteInterface.runListeningForSensors();

  // Detect pair completion
  bool pend = remoteInterface.isPairPending();
  if (rsens_was_pair_pending && !pend && remoteInterface.isPaired()) {
    rsens_pair_completed_at = millis();
    rsens_show_msg("Accoppiato!", TFT_GREEN);
  }
  rsens_was_pair_pending = pend;

  // Handle unpair completion
  if (remoteInterface.unpairDone()) {
    remoteInterface.unpair();
    remoteInterface.clearUnpairDone();
    rsens_show_msg("Disaccoppiato!", TFT_GREEN);
  }

  // Animate
  rsens_anim_dots = (rsens_anim_dots + 1) % 4;
  rsens_draw_anim();

  // Rebuild and redraw if changed
  rsens_build_rows();
  if (rsens_rows_changed()) {
    rsens_redraw_all();
    rsens_save_cache();
  }
}

void rsens_Touch(int tx, int ty) {
  // HOME
  if (tx >= rsens_home_x && tx < rsens_home_x + rsens_home_w &&
      ty >= rsens_home_y && ty < rsens_home_y + rsens_home_h) {
    rsens_home_pressed = true;
    rsens_draw_btn(rsens_home_x, rsens_home_y, rsens_home_w, rsens_home_h,
                   "HOME", 0x7800);
    return;
  }

  // Row buttons
  for (int i = 0; i < rsens_row_count; i++) {
    const RsensRowData& r = rsens_rows[i];
    if (!r.active || r.btnType == RS_BTN_NONE || r.btnType == RS_BTN_INACTIVE)
      continue;

    int ry = rsens_row_y(i);
    int by = ry + (rsens_row_h - rsens_btn_h) / 2;

    if (tx >= rsens_btn_x && tx < rsens_btn_x + rsens_btn_w &&
        ty >= by && ty < by + rsens_btn_h) {
      rsens_pressed_row = i;
      rsens_draw_btn(rsens_btn_x, by, rsens_btn_w, rsens_btn_h,
                     rsens_btn_text(r.btnType), 0x7800);
      return;
    }
  }
}

void rsens_Release() {
  // HOME
  if (rsens_home_pressed) {
    rsens_home_pressed = false;
    remoteInterface.stopListening();
    rsens_draw_btn(rsens_home_x, rsens_home_y, rsens_home_w, rsens_home_h,
                   "HOME", TFT_ORANGE);
    setScreen(SCREEN_MAIN);
    return;
  }

  // Row button
  if (rsens_pressed_row >= 0) {
    int idx = rsens_pressed_row;
    rsens_pressed_row = -1;
    const RsensRowData& r = rsens_rows[idx];

    if (r.btnType == RS_BTN_PAIR && r.discIndex >= 0) {
      // New pair (auto-substitutes if currently paired)
      if (remoteInterface.requestPairWith(r.discIndex)) {
        rsens_show_msg("Attesa sensore...", TFT_ORANGE);
      } else {
        rsens_show_msg("Errore!", TFT_RED);
      }
    }
    else if (r.btnType == RS_BTN_UNPAIR && r.discIndex == -1) {
      // Deferred unpair — waits for sensor contact
      remoteInterface.requestUnpair(TANKA_UNPAIR_TIMEOUT_MS);
      rsens_show_msg("Attesa sensore...", TFT_ORANGE);
    }
    else if (r.btnType == RS_BTN_ELIMINA && r.discIndex == -1) {
      // Local-only delete — instant, no sensor communication
      remoteInterface.unpair();
      // main loop handles commit via configChanged()
      rsens_show_msg("Eliminato", TFT_DARKGREY);
    }

    // Immediate redraw
    rsens_build_rows();
    rsens_redraw_all();
    rsens_save_cache();
    return;
  }
}

#else
void rsens_Draw()            {}
void rsens_Update()          {}
void rsens_Touch(int, int)   {}
void rsens_Release()         {}
#endif
