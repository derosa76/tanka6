// screen_pompa_adv.ino - Schermata LOGICA POMPA AVANZATA
//
// Tre parametri FSM:
//   pump_check_delay    — secondi attesa/watchdog portata  (10..600, step 10/60)
//   pump_flow_threshold — soglia portata mm/min            (0.5..20, step 0.5/2)
//   pump_retry_wait     — secondi attesa tra retry         (60..28800, step 60/300, display minuti)
//
// Pattern identico a screen_pompa: value box colorato + pulsantini <<,<,>,>>
// SALVA attivo solo se modifiche pendenti → safe_restart()
// HOME → torna a SCREEN_MENU_INST

// ============================================
// LAYOUT
// ============================================

const int padv_title_y  = 8;
const int padv_row_y0   = 42;
const int padv_row_h    = 42;

// Colonna 1 — Label (font 1, fino a x=100)
const int padv_label_x  = 10;

// Colonna 2 — Value box (identico a screen_pompa)
const int padv_value_x  = 100;
const int padv_value_w  = 100;

// Colonna 3 — 4 step buttons (identico a screen_pompa)
const int padv_sb_x0    = 210;
const int padv_sb_w     = 24;
const int padv_sb_gap   = 2;
const int padv_sb_h     = 30;

const int padv_status_y = 172;   // riga stato FSM read-only

const int padv_bottom_y   = 208;
const int padv_btn_w      = 120;
const int padv_btn_h      = 30;
const int padv_btn_home_x = 15;
const int padv_btn_salva_x = 185;

// ============================================
// COLORI (riusa constanti di screen_pompa)
// ============================================

// Usiamo le stesse costanti pmp_* dove possibile per coerenza visiva.
// Nuovi alias locali per chiarezza:
#define padv_bg           pmp_bg
#define padv_title_col    pmp_title_col
#define padv_label_col    pmp_label_col
#define padv_val_ok       pmp_val_unchanged
#define padv_val_chg      pmp_val_changed
#define padv_value_bg     pmp_value_bg
#define padv_btn_col      pmp_btn_col
#define padv_btn_push     pmp_btn_push
#define padv_salva_active pmp_salva_active
#define padv_salva_inact  pmp_salva_inactive

// ============================================
// VARIABILI TEMPORANEE
// ============================================

int   padv_temp_delay;      // pump_check_delay (s)
float padv_temp_threshold;  // pump_flow_threshold (mm/min)
int   padv_temp_retry;      // pump_retry_wait (s)

int   padv_saved_delay;
float padv_saved_threshold;
int   padv_saved_retry;

// ============================================
// STATO PULSANTI
// ============================================

bool padv_sb_pressed[3][4];  // [riga 0-2][btn 0-3]
bool padv_btn_home   = false;
bool padv_btn_salva  = false;

// ============================================
// HELPERS CHANGED
// ============================================

bool padv_delay_changed()     { return padv_temp_delay     != padv_saved_delay; }
bool padv_thresh_changed()    { return padv_temp_threshold != padv_saved_threshold; }
bool padv_retry_changed()     { return padv_temp_retry     != padv_saved_retry; }
bool padv_any_changed()       {
  return padv_delay_changed() || padv_thresh_changed() || padv_retry_changed();
}

int padv_sb_x(int btn) {
  return padv_sb_x0 + btn * (padv_sb_w + padv_sb_gap);
}

// ============================================
// DRAW HELPERS
// ============================================

void padv_draw_btn(int x, int y, int w, int h, const char* text, uint16_t col) {
  tft.fillRoundRect(x, y, w, h, 4, col);
  tft.setTextColor(TFT_BLACK, col);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.setTextPadding(0);
  tft.drawString(text, x + w / 2, y + h / 2);
}

void padv_draw_value_box(int row, const char* text, bool changed) {
  int y = padv_row_y0 + row * padv_row_h + (padv_row_h - 24) / 2;
  tft.fillRoundRect(padv_value_x, y, padv_value_w, 24, 4, padv_value_bg);
  tft.drawRoundRect(padv_value_x, y, padv_value_w, 24, 4, TFT_WHITE);
  tft.setTextColor(changed ? padv_val_chg : padv_val_ok, padv_value_bg);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.setTextPadding(0);
  tft.drawString(text, padv_value_x + padv_value_w / 2, y + 12);
}

void padv_draw_stepbtns(int row, int pressedBtn) {
  int by = padv_row_y0 + row * padv_row_h + (padv_row_h - padv_sb_h) / 2;
  const char* labels[] = {"<<", "<", ">", ">>"};
  for (int i = 0; i < 4; i++) {
    padv_draw_btn(padv_sb_x(i), by, padv_sb_w, padv_sb_h, labels[i],
                  padv_sb_pressed[row][i] ? padv_btn_push : padv_btn_col);
  }
}

// ============================================
// RIGHE
// ============================================

void padv_draw_row0() {  // TEMPO VERIFICA LIVELLO
  char buf[12];
  snprintf(buf, sizeof(buf), "%ds", padv_temp_delay);
  padv_draw_value_box(0, buf, padv_delay_changed());
  padv_draw_stepbtns(0, -1);
}

void padv_draw_row1() {  // SOGLIA PORTATA
  char buf[12];
  snprintf(buf, sizeof(buf), "%.1f mm/min", padv_temp_threshold);
  padv_draw_value_box(1, buf, padv_thresh_changed());
  padv_draw_stepbtns(1, -1);
}

void padv_draw_row2() {  // ATTESA RETRY
  char buf[12];
  snprintf(buf, sizeof(buf), "%d min", padv_temp_retry / 60);
  padv_draw_value_box(2, buf, padv_retry_changed());
  padv_draw_stepbtns(2, -1);
}

void padv_draw_salva() {
  bool active = padv_any_changed();
  uint16_t col = active ? padv_salva_active : padv_salva_inact;
  if (padv_btn_salva && active) col = padv_btn_push;
  padv_draw_btn(padv_btn_salva_x, padv_bottom_y, padv_btn_w, padv_btn_h, "SALVA", col);
}

void padv_draw_status() {
  tft.fillRect(0, padv_status_y, 320, 30, padv_bg);
  tft.setTextFont(2);
  tft.setTextDatum(ML_DATUM);
  tft.setTextPadding(0);

  // Stato FSM corrente
  const char* stateStr;
  uint16_t stateCol;
  switch (pump_fsm_state) {
    case PUMP_IDLE:        stateStr = "FSM: IDLE";        stateCol = TFT_DARKGREY; break;
    case PUMP_STARTING:    stateStr = "FSM: AVVIO";       stateCol = TFT_YELLOW;   break;
    case PUMP_RUNNING:     stateStr = "FSM: IN FUNZIONE"; stateCol = TFT_GREEN;    break;
    case PUMP_RETRY_WAIT:  stateStr = "FSM: ATTESA RETRY";stateCol = TFT_ORANGE;   break;
    default:               stateStr = "FSM: ---";         stateCol = TFT_DARKGREY; break;
  }
  tft.setTextColor(stateCol, padv_bg);
  tft.drawString(stateStr, 10, padv_status_y + 15);
}

// ============================================
// SCREEN INTERFACE
// ============================================

void pompa_adv_Draw() {
  // Carica valori da config
  padv_temp_delay     = config["pump_check_delay"]     | 120;
  padv_temp_threshold = config["pump_flow_threshold"]  | 5.0f;
  padv_temp_retry     = config["pump_retry_wait"]      | 300;
  padv_saved_delay     = padv_temp_delay;
  padv_saved_threshold = padv_temp_threshold;
  padv_saved_retry     = padv_temp_retry;

  memset(padv_sb_pressed, false, sizeof(padv_sb_pressed));
  padv_btn_home  = false;
  padv_btn_salva = false;

  tft.fillScreen(padv_bg);

  // Titolo
  tft.setTextColor(padv_title_col, padv_bg);
  tft.setTextFont(2);
  tft.setTextDatum(TC_DATUM);
  tft.setTextPadding(0);
  tft.drawString("LOGICA POMPA AVANZATA", 160, padv_title_y);

  tft.drawFastHLine(10, padv_row_y0 - 5, 300, TFT_DARKGREY);

  // Labels su due righe dove necessario (font 1 = ~6px/char, colonna 90px)
  // Riga 1 centrata verticalmente nella metà superiore della row,
  // riga 2 nella metà inferiore.
  struct { const char* l1; const char* l2; } labels[] = {
    { "TEMPO VERIFICA", "PORTATA" },
    { "SOGLIA",         "PORTATA (mm/min)" },
    { "ATTESA",         "RETRY" }
  };
  for (int i = 0; i < 3; i++) {
    int row_cy = padv_row_y0 + i * padv_row_h + padv_row_h / 2;
    tft.setTextColor(padv_label_col, padv_bg);
    tft.setTextFont(1);
    tft.setTextDatum(ML_DATUM);
    tft.setTextPadding(0);
    tft.drawString(labels[i].l1, padv_label_x, row_cy - 5);
    tft.drawString(labels[i].l2, padv_label_x, row_cy + 5);
  }

  padv_draw_row0();
  padv_draw_row1();
  padv_draw_row2();

  tft.drawFastHLine(10, padv_status_y - 3, 300, TFT_DARKGREY);
  padv_draw_status();

  padv_draw_btn(padv_btn_home_x, padv_bottom_y, padv_btn_w, padv_btn_h, "INDIETRO", padv_btn_col);
  padv_draw_salva();
}

void pompa_adv_Update() {
  static unsigned long last_upd = 0;
  if (millis() - last_upd < 1000) return;
  last_upd = millis();
  padv_draw_status();
}

void pompa_adv_Touch(int tx, int ty) {
  // Step buttons — 3 righe × 4 bottoni
  for (int row = 0; row < 3; row++) {
    int by = padv_row_y0 + row * padv_row_h + (padv_row_h - padv_sb_h) / 2;
    if (ty < by || ty > by + padv_sb_h) continue;
    for (int i = 0; i < 4; i++) {
      int bx = padv_sb_x(i);
      if (tx >= bx && tx <= bx + padv_sb_w) {
        padv_sb_pressed[row][i] = true;
        if      (row == 0) padv_draw_row0();
        else if (row == 1) padv_draw_row1();
        else               padv_draw_row2();
        return;
      }
    }
  }

  // HOME
  if (tx >= padv_btn_home_x && tx <= padv_btn_home_x + padv_btn_w &&
      ty >= padv_bottom_y   && ty <= padv_bottom_y + padv_btn_h) {
    padv_btn_home = true;
    padv_draw_btn(padv_btn_home_x, padv_bottom_y, padv_btn_w, padv_btn_h, "INDIETRO", padv_btn_push);
  }

  // SALVA (solo se modifiche)
  if (padv_any_changed() &&
      tx >= padv_btn_salva_x && tx <= padv_btn_salva_x + padv_btn_w &&
      ty >= padv_bottom_y    && ty <= padv_bottom_y + padv_btn_h) {
    padv_btn_salva = true;
    padv_draw_salva();
  }
}

void pompa_adv_Release() {
  // Step buttons release
  // Steps: <<=-big, <=small, >=small, >>=big
  const int   delay_steps[]  = {-60, -10, 10, 60};
  const float thresh_steps[] = {-2.0f, -0.5f, 0.5f, 2.0f};
  const int   retry_steps[]  = {-300, -60, 60, 300};  // 5min/1min/1min/5min

  for (int row = 0; row < 3; row++) {
    for (int i = 0; i < 4; i++) {
      if (!padv_sb_pressed[row][i]) continue;
      padv_sb_pressed[row][i] = false;

      if (row == 0) {
        padv_temp_delay += delay_steps[i];
        padv_temp_delay = constrain(padv_temp_delay, 10, 600);
        padv_draw_row0();
      } else if (row == 1) {
        padv_temp_threshold += thresh_steps[i];
        padv_temp_threshold = constrain(padv_temp_threshold, 0.5f, 20.0f);
        padv_draw_row1();
      } else {
        padv_temp_retry += retry_steps[i];
        padv_temp_retry = constrain(padv_temp_retry, 60, 28800);
        padv_draw_row2();
      }
      padv_draw_salva();
      return;
    }
  }

  // HOME
  if (padv_btn_home) {
    padv_btn_home = false;
    padv_draw_btn(padv_btn_home_x, padv_bottom_y, padv_btn_w, padv_btn_h, "INDIETRO", padv_btn_col);
    setScreen(SCREEN_MENU_INST);
    return;
  }

  // SALVA
  if (padv_btn_salva) {
    padv_btn_salva = false;
    if (!padv_any_changed()) return;

    config["pump_check_delay"]    = padv_temp_delay;
    config["pump_flow_threshold"] = padv_temp_threshold;
    config["pump_retry_wait"]     = padv_temp_retry;

    Serial.printf("[POMPA ADV] Salvato: delay=%ds threshold=%.1fmm/min retry=%ds\n",
                  padv_temp_delay, padv_temp_threshold, padv_temp_retry);

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("Riavvio...", 160, 120);
    delay(500);
    safe_restart();
  }
}
