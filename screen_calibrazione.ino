// screen_calibrazione.ino - Calibrazione livello cisterna
// USA cattura distanza EMA, <<< << < > >> >>> per regolazione fine
// Stessi colori pompa: verde=conforme, arancio=modificato, SALVA grigio/verde

// ============================================
// LAYOUT
// ============================================

const int cal_title_y = 8;
const int cal_row_y0 = 48;
const int cal_row_h = 44;
const int cal_label_x = 4;

// Value box
const int cal_value_x = 50;
const int cal_value_w = 68;

// Pulsante USA
const int cal_usa_x = 122;
const int cal_usa_w = 34;

// 6 pulsanti regolazione: <<<, <<, <, >, >>, >>>
const int cal_adj_x0 = 162;
const int cal_adj_w = 24;
const int cal_adj_gap = 2;
const int cal_adj_h = 30;

// Status
const int cal_status_y = 145;

// Bottom
const int cal_bottom_y = 208;
const int cal_btn_bottom_w = 120;
const int cal_btn_bottom_h = 30;
const int cal_btn_home_x = 15;
const int cal_btn_salva_x = 185;

// ============================================
// COLORI
// ============================================

const uint16_t cal_bg = TFT_BLACK;
const uint16_t cal_title_col = TFT_CYAN;
const uint16_t cal_label_col = TFT_WHITE;
const uint16_t cal_val_unchanged = TFT_GREEN;
const uint16_t cal_val_changed = TFT_ORANGE;
const uint16_t cal_value_bg = TFT_NAVY;
const uint16_t cal_btn_col = TFT_ORANGE;
const uint16_t cal_btn_usa_col = 0x07E0;  // verde brillante
const uint16_t cal_btn_push = TFT_RED;
const uint16_t cal_salva_active = TFT_GREEN;
const uint16_t cal_salva_inactive = TFT_DARKGREY;

// ============================================
// VARIABILI TEMPORANEE
// ============================================

float cal_temp_pieno = 0;
float cal_temp_vuoto = 0;
float cal_saved_pieno = 0;
float cal_saved_vuoto = 0;

// Pressed tracking
bool cal_usa_pressed[2] = {false};       // [0=pieno, 1=vuoto]
bool cal_adj_pressed[2][6] = {{false}};  // [riga][btn]
bool cal_btn_home = false;
bool cal_btn_salva = false;

// ============================================
// HELPER
// ============================================

bool cal_pieno_changed() { return cal_temp_pieno != cal_saved_pieno; }
bool cal_vuoto_changed() { return cal_temp_vuoto != cal_saved_vuoto; }
bool cal_any_changed()   { return cal_pieno_changed() || cal_vuoto_changed(); }

int cal_adj_x(int btn) {
  return cal_adj_x0 + btn * (cal_adj_w + cal_adj_gap);
}

float cal_clamp(float v) {
  if (v < 30) return 30;
  if (v > 4500) return 4500;
  return v;
}

// ============================================
// DISEGNO
// ============================================

void cal_draw_btn(int x, int y, int w, int h, const char* text, uint16_t col) {
  tft.fillRoundRect(x, y, w, h, 4, col);
  tft.setTextColor(TFT_BLACK, col);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(text, x + w / 2, y + h / 2);
}

void cal_draw_value_box(int row, float val, bool changed) {
  int y = cal_row_y0 + row * cal_row_h + (cal_row_h - 24) / 2;
  tft.fillRoundRect(cal_value_x, y, cal_value_w, 24, 4, cal_value_bg);
  tft.drawRoundRect(cal_value_x, y, cal_value_w, 24, 4, TFT_WHITE);
  tft.setTextColor(changed ? cal_val_changed : cal_val_unchanged, cal_value_bg);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  char buf[10];
  snprintf(buf, sizeof(buf), "%.1f", val);
  tft.drawString(buf, cal_value_x + cal_value_w / 2, y + 12);
}

void cal_draw_row(int row, float val, bool changed) {
  cal_draw_value_box(row, val, changed);
  int by = cal_row_y0 + row * cal_row_h + (cal_row_h - cal_adj_h) / 2;

  // USA
  cal_draw_btn(cal_usa_x, by, cal_usa_w, cal_adj_h, "USA",
               cal_usa_pressed[row] ? cal_btn_push : cal_btn_usa_col);

  // 6 pulsanti
  const char* labels[] = {"<<<", "<<", "<", ">", ">>", ">>>"};
  for (int i = 0; i < 6; i++) {
    cal_draw_btn(cal_adj_x(i), by, cal_adj_w, cal_adj_h, labels[i],
                 cal_adj_pressed[row][i] ? cal_btn_push : cal_btn_col);
  }
}

void cal_draw_salva() {
  bool active = cal_any_changed();
  uint16_t col = active ? cal_salva_active : cal_salva_inactive;
  if (cal_btn_salva && active) col = cal_btn_push;
  cal_draw_btn(cal_btn_salva_x, cal_bottom_y, cal_btn_bottom_w, cal_btn_bottom_h, "SALVA", col);
}

void cal_draw_status() {
  tft.fillRect(0, cal_status_y, 320, 50, cal_bg);
  tft.setTextFont(2);
  tft.setTextColor(TFT_WHITE, cal_bg);
  tft.setTextDatum(ML_DATUM);

  // Distanza EMA attuale
  char buf[30];
  snprintf(buf, sizeof(buf), "Sensore: %.1f mm", sensor.getDistance());
  tft.drawString(buf, 10, cal_status_y + 10);

  // Livello calcolato con valori TEMP (preview live)
  if (cal_temp_pieno > 0 && cal_temp_vuoto > 0 && cal_temp_pieno < cal_temp_vuoto) {
    float lev = sensor.getLevel(cal_temp_pieno, cal_temp_vuoto);
    if (!isnan(lev)) {
      snprintf(buf, sizeof(buf), "Livello: %.1f%%", lev);
      tft.drawString(buf, 10, cal_status_y + 30);
    }
  } else if (cal_temp_pieno >= cal_temp_vuoto && cal_temp_pieno > 0) {
    tft.setTextColor(TFT_RED, cal_bg);
    tft.drawString("PIENO deve essere < VUOTO", 10, cal_status_y + 30);
  } else {
    tft.drawString("Livello: ---", 10, cal_status_y + 30);
  }
}

// ============================================
// SCREEN FUNCTIONS
// ============================================

void cal_Draw() {
  // Carica valori da config (0 se non esistono)
  cal_temp_pieno = config["calib_max_level"] | 0.0f;
  cal_temp_vuoto = config["calib_min_level"] | 0.0f;
  cal_saved_pieno = cal_temp_pieno;
  cal_saved_vuoto = cal_temp_vuoto;

  tft.fillScreen(cal_bg);

  // Titolo
  tft.setTextColor(cal_title_col, cal_bg);
  tft.setTextFont(2);
  tft.setTextDatum(TC_DATUM);
  tft.drawString("CALIBRAZIONE", 160, cal_title_y);

  tft.drawFastHLine(10, cal_row_y0 - 5, 300, TFT_DARKGREY);

  // Labels
  const char* labels[] = {"PIENO", "VUOTO"};
  for (int i = 0; i < 2; i++) {
    int y = cal_row_y0 + i * cal_row_h + cal_row_h / 2;
    tft.setTextColor(cal_label_col, cal_bg);
    tft.setTextFont(2);
    tft.setTextDatum(ML_DATUM);
    tft.setTextPadding(0);
    tft.drawString(labels[i], cal_label_x, y);
  }

  cal_draw_row(0, cal_temp_pieno, cal_pieno_changed());
  cal_draw_row(1, cal_temp_vuoto, cal_vuoto_changed());

  tft.drawFastHLine(10, cal_status_y - 5, 300, TFT_DARKGREY);
  cal_draw_status();

  tft.drawFastHLine(10, cal_bottom_y - 5, 300, TFT_DARKGREY);
  cal_draw_btn(cal_btn_home_x, cal_bottom_y, cal_btn_bottom_w, cal_btn_bottom_h, "HOME", cal_btn_col);
  cal_draw_salva();
}

void cal_Update() {
  static unsigned long last_status = 0;
  if (millis() - last_status > 500) {
    last_status = millis();
    cal_draw_status();
  }
}

void cal_Touch(int tx, int ty) {
  for (int row = 0; row < 2; row++) {
    int by = cal_row_y0 + row * cal_row_h + (cal_row_h - cal_adj_h) / 2;
    if (ty < by || ty > by + cal_adj_h) continue;

    // USA
    if (tx >= cal_usa_x && tx <= cal_usa_x + cal_usa_w) {
      cal_usa_pressed[row] = true;
      cal_draw_row(row, row == 0 ? cal_temp_pieno : cal_temp_vuoto,
                   row == 0 ? cal_pieno_changed() : cal_vuoto_changed());
    }

    // 6 adj
    for (int i = 0; i < 6; i++) {
      int bx = cal_adj_x(i);
      if (tx >= bx && tx <= bx + cal_adj_w) {
        cal_adj_pressed[row][i] = true;
        cal_draw_row(row, row == 0 ? cal_temp_pieno : cal_temp_vuoto,
                     row == 0 ? cal_pieno_changed() : cal_vuoto_changed());
        break;
      }
    }
  }

  // HOME
  if (tx >= cal_btn_home_x && tx <= cal_btn_home_x + cal_btn_bottom_w &&
      ty >= cal_bottom_y && ty <= cal_bottom_y + cal_btn_bottom_h) {
    cal_btn_home = true;
    cal_draw_btn(cal_btn_home_x, cal_bottom_y, cal_btn_bottom_w, cal_btn_bottom_h, "HOME", cal_btn_push);
  }

  // SALVA
  if (cal_any_changed() &&
      tx >= cal_btn_salva_x && tx <= cal_btn_salva_x + cal_btn_bottom_w &&
      ty >= cal_bottom_y && ty <= cal_bottom_y + cal_btn_bottom_h) {
    cal_btn_salva = true;
    cal_draw_salva();
  }
}

void cal_Release() {
  const float steps[] = {-100, -10, -1, 1, 10, 100};

  for (int row = 0; row < 2; row++) {
    float* val = (row == 0) ? &cal_temp_pieno : &cal_temp_vuoto;

    // USA
    if (cal_usa_pressed[row]) {
      cal_usa_pressed[row] = false;
      *val = sensor.getDistance();
      cal_draw_row(row, *val, row == 0 ? cal_pieno_changed() : cal_vuoto_changed());
      cal_draw_salva();
    }

    // 6 adj
    for (int i = 0; i < 6; i++) {
      if (cal_adj_pressed[row][i]) {
        cal_adj_pressed[row][i] = false;
        *val = cal_clamp(*val + steps[i]);
        cal_draw_row(row, *val, row == 0 ? cal_pieno_changed() : cal_vuoto_changed());
        cal_draw_salva();
      }
    }
  }

  // HOME
  if (cal_btn_home) {
    cal_btn_home = false;
    cal_draw_btn(cal_btn_home_x, cal_bottom_y, cal_btn_bottom_w, cal_btn_bottom_h, "HOME", cal_btn_col);
    setScreen(SCREEN_MAIN);
  }

  // SALVA
  if (cal_btn_salva) {
    cal_btn_salva = false;

    // Validazione
    if (cal_temp_pieno >= cal_temp_vuoto) {
      tft.fillRect(0, cal_status_y, 320, 50, cal_bg);
      tft.setTextColor(TFT_RED, cal_bg);
      tft.setTextFont(2);
      tft.setTextDatum(MC_DATUM);
      tft.drawString("ERRORE: PIENO deve essere < VUOTO", 160, cal_status_y + 20);
      Serial.println("[CAL] Errore: pieno >= vuoto, rifiutato");
      return;
    }

    config["calib_max_level"] = cal_temp_pieno;
    config["calib_min_level"] = cal_temp_vuoto;
    //writeJson(config_filename, config);
    Serial.printf("[CAL] Salvato: pieno=%.1f vuoto=%.1f\n", cal_temp_pieno, cal_temp_vuoto);

    // Aggiorna saved per feedback colori (torna tutto verde)
    cal_saved_pieno = cal_temp_pieno;
    cal_saved_vuoto = cal_temp_vuoto;
    cal_draw_row(0, cal_temp_pieno, false);
    cal_draw_row(1, cal_temp_vuoto, false);
    cal_draw_salva();
    cal_draw_status();
  }
}
