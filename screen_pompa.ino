// screen_pompa.ino - Schermata LOGICA POMPA
// Modo (CAMBIA cicla), soglie H/L (<<,<,>,>>), feedback colori

// ============================================
// LAYOUT
// ============================================

const int pmp_title_y = 8;
const int pmp_row_y0 = 45;
const int pmp_row_h = 42;
const int pmp_label_x = 10;
const int pmp_value_x = 100;
const int pmp_value_w = 100;

// 4 pulsantini per riga H/L
const int pmp_sb_x0 = 210;     // start x primo pulsantino
const int pmp_sb_w = 24;       // larghezza pulsantino
const int pmp_sb_gap = 2;      // gap tra pulsantini
const int pmp_sb_h = 30;
// pulsante CAMBIA per riga modo
const int pmp_cambia_x = 210;
const int pmp_cambia_w = 102;  // da 210 a 312

const int pmp_status_y = 175;

const int pmp_bottom_y = 208;
const int pmp_btn_bottom_w = 120;
const int pmp_btn_bottom_h = 30;
const int pmp_btn_home_x = 15;
const int pmp_btn_salva_x = 185;

// ============================================
// COLORI
// ============================================

const uint16_t pmp_bg = TFT_BLACK;
const uint16_t pmp_title_col = TFT_CYAN;
const uint16_t pmp_label_col = TFT_WHITE;
const uint16_t pmp_val_unchanged = TFT_GREEN;     // conforme a config
const uint16_t pmp_val_changed = TFT_ORANGE;      // modificato
const uint16_t pmp_value_bg = TFT_NAVY;
const uint16_t pmp_btn_col = TFT_ORANGE;
const uint16_t pmp_btn_push = TFT_RED;
const uint16_t pmp_salva_active = TFT_GREEN;
const uint16_t pmp_salva_inactive = TFT_DARKGREY;
const uint16_t pmp_on_col = TFT_GREEN;
const uint16_t pmp_off_col = TFT_DARKGREY;

// ============================================
// VARIABILI TEMPORANEE
// ============================================

int pompa_temp_mode = 0;
float pompa_temp_H = 80;
float pompa_temp_L = 20;

// Valori salvati in config (per confronto colori)
int pompa_saved_mode = 0;
float pompa_saved_H = 80;
float pompa_saved_L = 20;

// Stato pulsanti (pressed tracking)
bool pmp_cambia_pressed = false;
bool pmp_sb_pressed[2][4] = {{false}};  // [riga 0=H,1=L][btn 0-3]
bool pmp_btn_home = false;
bool pmp_btn_salva = false;

// ============================================
// HELPER
// ============================================

const char* pmp_mode_str(int m) {
  switch(m) {
    case 1: return "RIEMPI";
    case 2: return "SVUOTA";
    default: return "OFF";
  }
}

bool pmp_mode_changed()  { return pompa_temp_mode != pompa_saved_mode; }
bool pmp_H_changed()     { return pompa_temp_H != pompa_saved_H; }
bool pmp_L_changed()     { return pompa_temp_L != pompa_saved_L; }
bool pmp_any_changed()   { return pmp_mode_changed() || pmp_H_changed() || pmp_L_changed(); }

int pmp_sb_x(int btn) {
  return pmp_sb_x0 + btn * (pmp_sb_w + pmp_sb_gap);
}

// ============================================
// DISEGNO
// ============================================

void pmp_draw_btn(int x, int y, int w, int h, const char* text, uint16_t col) {
  tft.fillRoundRect(x, y, w, h, 4, col);
  tft.setTextColor(TFT_BLACK, col);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(text, x + w / 2, y + h / 2);
}

void pmp_draw_value_box(int row, const char* text, bool changed) {
  int y = pmp_row_y0 + row * pmp_row_h + (pmp_row_h - 24) / 2;
  tft.fillRoundRect(pmp_value_x, y, pmp_value_w, 24, 4, pmp_value_bg);
  tft.drawRoundRect(pmp_value_x, y, pmp_value_w, 24, 4, TFT_WHITE);
  tft.setTextColor(changed ? pmp_val_changed : pmp_val_unchanged, pmp_value_bg);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(text, pmp_value_x + pmp_value_w / 2, y + 12);
}

void pmp_draw_row0() {
  pmp_draw_value_box(0, pmp_mode_str(pompa_temp_mode), pmp_mode_changed());
  int by = pmp_row_y0 + (pmp_row_h - pmp_sb_h) / 2;
  pmp_draw_btn(pmp_cambia_x, by, pmp_cambia_w, pmp_sb_h, "CAMBIA",
               pmp_cambia_pressed ? pmp_btn_push : pmp_btn_col);
}

void pmp_draw_row_hl(int row, float val, bool changed, int pressedBtn) {
  char buf[8];
  snprintf(buf, sizeof(buf), "%.0f%%", val);
  pmp_draw_value_box(row, buf, changed);
  int by = pmp_row_y0 + row * pmp_row_h + (pmp_row_h - pmp_sb_h) / 2;
  const char* labels[] = {"<<", "<", ">", ">>"};
  int ri = row - 1;  // indice in pmp_sb_pressed: 0=H, 1=L
  for (int i = 0; i < 4; i++) {
    pmp_draw_btn(pmp_sb_x(i), by, pmp_sb_w, pmp_sb_h, labels[i],
                 pmp_sb_pressed[ri][i] ? pmp_btn_push : pmp_btn_col);
  }
}

void pmp_draw_row1() { pmp_draw_row_hl(1, pompa_temp_H, pmp_H_changed(), -1); }
void pmp_draw_row2() { pmp_draw_row_hl(2, pompa_temp_L, pmp_L_changed(), -1); }

void pmp_draw_salva() {
  bool active = pmp_any_changed();
  uint16_t col = active ? pmp_salva_active : pmp_salva_inactive;
  if (pmp_btn_salva && active) col = pmp_btn_push;
  pmp_draw_btn(pmp_btn_salva_x, pmp_bottom_y, pmp_btn_bottom_w, pmp_btn_bottom_h, "SALVA", col);
}

void pmp_draw_status() {
  tft.fillRect(0, pmp_status_y, 320, 28, pmp_bg);
  tft.setTextFont(2);
  tft.setTextDatum(ML_DATUM);

  tft.setTextColor(pump_running ? pmp_on_col : pmp_off_col, pmp_bg);
  tft.drawString(pump_running ? "POMPA: ON" : "POMPA: OFF", 10, pmp_status_y + 14);

  tft.setTextColor(TFT_WHITE, pmp_bg);
  if (!config["calib_max_level"].isNull() && !config["calib_min_level"].isNull()) {
    float liv = sensor.getLevel(config["calib_max_level"].as<float>(), config["calib_min_level"].as<float>());
    if (!isnan(liv)) {
      char buf[20]; snprintf(buf, sizeof(buf), "Livello: %.0f%%", liv);
      tft.drawString(buf, 180, pmp_status_y + 14);
    } else {
      tft.drawString("ERR CAL", 180, pmp_status_y + 14);
    }
  } else {
    tft.drawString("Non calibrato", 180, pmp_status_y + 14);
  }
}

// ============================================
// SCREEN FUNCTIONS
// ============================================

void pompa_Draw() {
  // Carica valori da config
  pompa_temp_mode = config["pump_mode"] | 0;
  pompa_temp_H = config["pump_H"] | 80.0f;
  pompa_temp_L = config["pump_L"] | 20.0f;
  pompa_saved_mode = pompa_temp_mode;
  pompa_saved_H = pompa_temp_H;
  pompa_saved_L = pompa_temp_L;

  tft.fillScreen(pmp_bg);

  // Titolo
  tft.setTextColor(pmp_title_col, pmp_bg);
  tft.setTextFont(2);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("LOGICA POMPA", 160, pmp_title_y);

  tft.drawFastHLine(10, pmp_row_y0 - 5, 300, TFT_DARKGREY);

  // Labels
  const char* labels[] = {"MODO", "ALTO", "BASSO"};
  for (int i = 0; i < 3; i++) {
    int y = pmp_row_y0 + i * pmp_row_h + pmp_row_h / 2;
    tft.setTextColor(pmp_label_col, pmp_bg);
    tft.setTextFont(2);
    tft.setTextDatum(ML_DATUM);
    tft.setTextPadding(0);
    tft.drawString(labels[i], pmp_label_x, y);
  }

  pmp_draw_row0();
  pmp_draw_row1();
  pmp_draw_row2();

  tft.drawFastHLine(10, pmp_status_y - 5, 300, TFT_DARKGREY);
  pmp_draw_status();

  pmp_draw_btn(pmp_btn_home_x, pmp_bottom_y, pmp_btn_bottom_w, pmp_btn_bottom_h, "HOME", pmp_btn_col);
  pmp_draw_salva();
}

void pompa_Update() {
  static unsigned long last_status = 0;
  if (millis() - last_status > 1000) {
    last_status = millis();
    pmp_draw_status();
  }
}

void pompa_Touch(int tx, int ty) {
  // --- Row 0: CAMBIA ---
  int by0 = pmp_row_y0 + (pmp_row_h - pmp_sb_h) / 2;
  if (ty >= by0 && ty <= by0 + pmp_sb_h &&
      tx >= pmp_cambia_x && tx <= pmp_cambia_x + pmp_cambia_w) {
    pmp_cambia_pressed = true;
    pmp_draw_row0();
  }

  // --- Row 1 (H) e Row 2 (L): 4 pulsantini ---
  for (int row = 1; row <= 2; row++) {
    int by = pmp_row_y0 + row * pmp_row_h + (pmp_row_h - pmp_sb_h) / 2;
    if (ty >= by && ty <= by + pmp_sb_h) {
      int ri = row - 1;
      for (int i = 0; i < 4; i++) {
        int bx = pmp_sb_x(i);
        if (tx >= bx && tx <= bx + pmp_sb_w) {
          pmp_sb_pressed[ri][i] = true;
          if (row == 1) pmp_draw_row1();
          else pmp_draw_row2();
          break;
        }
      }
    }
  }

  // --- HOME ---
  if (tx >= pmp_btn_home_x && tx <= pmp_btn_home_x + pmp_btn_bottom_w &&
      ty >= pmp_bottom_y && ty <= pmp_bottom_y + pmp_btn_bottom_h) {
    pmp_btn_home = true;
    pmp_draw_btn(pmp_btn_home_x, pmp_bottom_y, pmp_btn_bottom_w, pmp_btn_bottom_h, "HOME", pmp_btn_push);
  }

  // --- SALVA (solo se attivo) ---
  if (pmp_any_changed() &&
      tx >= pmp_btn_salva_x && tx <= pmp_btn_salva_x + pmp_btn_bottom_w &&
      ty >= pmp_bottom_y && ty <= pmp_bottom_y + pmp_btn_bottom_h) {
    pmp_btn_salva = true;
    pmp_draw_salva();
  }
}

void pompa_Release() {
  // --- CAMBIA modo ---
  if (pmp_cambia_pressed) {
    pmp_cambia_pressed = false;
    pompa_temp_mode = (pompa_temp_mode + 1) % 3;
    pmp_draw_row0();
    pmp_draw_salva();
  }

  // --- Pulsantini H/L ---
  const float steps[] = {-5, -1, 1, 5};
  for (int row = 0; row < 2; row++) {
    for (int i = 0; i < 4; i++) {
      if (pmp_sb_pressed[row][i]) {
        pmp_sb_pressed[row][i] = false;
        float* val = (row == 0) ? &pompa_temp_H : &pompa_temp_L;
        *val += steps[i];
        if (*val < 0) *val = 0;
        if (*val > 100) *val = 100;
        if (row == 0) pmp_draw_row1();
        else pmp_draw_row2();
        pmp_draw_salva();
      }
    }
  }

  // --- HOME ---
  if (pmp_btn_home) {
    pmp_btn_home = false;
    pmp_draw_btn(pmp_btn_home_x, pmp_bottom_y, pmp_btn_bottom_w, pmp_btn_bottom_h, "HOME", pmp_btn_col);
    setScreen(SCREEN_MAIN);
  }

  // --- SALVA ---
  if (pmp_btn_salva) {
    pmp_btn_salva = false;

    // Validazione: H deve essere > L
    if (pompa_temp_mode != 0 && pompa_temp_H <= pompa_temp_L) {
      tft.fillRect(0, pmp_status_y, 320, 28, pmp_bg);
      tft.setTextColor(TFT_RED, pmp_bg);
      tft.setTextFont(2);
      tft.setTextDatum(MC_DATUM);
      tft.drawString("ERRORE: ALTO deve essere > BASSO", 160, pmp_status_y + 14);
      Serial.println("[POMPA] Errore: H <= L, salvataggio rifiutato");
      return;
    }

    config["pump_mode"] = pompa_temp_mode;
    config["pump_H"] = pompa_temp_H;
    config["pump_L"] = pompa_temp_L;
    //writeJson(config_filename, config);
    Serial.printf("[POMPA] Salvato: mode=%d H=%.0f L=%.0f\n", pompa_temp_mode, pompa_temp_H, pompa_temp_L);

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Riavvio...", 160, 120);
    delay(500);
    safe_restart();
  }
}
