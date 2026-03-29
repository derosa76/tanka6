// screen_menu_installatore.ino - Menu installatore
//
// Menu con accesso a schermate di configurazione.
// Accessibile solo dopo PIN valido o se sessione installatore ancora attiva.
// Voci: Calibrazione, Logica Pompa, Sensore Remoto (se TANKA_SENSOR_REMOTE).

// ============================================
// LAYOUT
// ============================================

const int minst_title_y = 8;

// Menu buttons — centrati, verticali
const int minst_btn_x = 30;
const int minst_btn_w = 260;
const int minst_btn_h = 36;
const int minst_btn_gap = 8;
const int minst_btn_y0 = 38;

#ifdef TANKA_SENSOR_REMOTE
const int minst_num_btns = 4;
#else
const int minst_num_btns = 3;
#endif

const int minst_bottom_y = 208;
const int minst_btn_home_x = 100;
const int minst_btn_home_w = 120;
const int minst_btn_home_h = 28;

// ============================================
// COLORI
// ============================================

const uint16_t minst_bg        = TFT_BLACK;
const uint16_t minst_title_col = TFT_CYAN;
const uint16_t minst_btn_col   = TFT_ORANGE;
const uint16_t minst_btn_push  = TFT_RED;

// ============================================
// STATE
// ============================================

int  minst_btn_pressed = -1;    // menu button index, -1=none
bool minst_home_pressed = false;

// ============================================
// LABELS & TARGETS
// ============================================

#ifdef TANKA_SENSOR_REMOTE
const char* minst_labels[] = {
  "CALIBRAZIONE",
  "LOGICA POMPA",
  "LOGICA POMPA AVANZATA",
  "SENSORE REMOTO"
};
const ScreenID minst_targets[] = {
  SCREEN_CALIBRAZIONE,
  SCREEN_POMPA,
  SCREEN_POMPA_ADV,
  SCREEN_SENSORE
};
#else
const char* minst_labels[] = {
  "CALIBRAZIONE",
  "LOGICA POMPA",
  "LOGICA POMPA AVANZATA"
};
const ScreenID minst_targets[] = {
  SCREEN_CALIBRAZIONE,
  SCREEN_POMPA,
  SCREEN_POMPA_ADV
};
#endif

// ============================================
// DRAW HELPERS
// ============================================

void minst_draw_btn(int x, int y, int w, int h, const char* text, uint16_t col) {
  tft.fillRoundRect(x, y, w, h, 5, col);
  tft.setTextColor(TFT_BLACK, col);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.setTextPadding(0);
  tft.drawString(text, x + w / 2, y + h / 2);
}

int minst_btn_y(int idx) {
  return minst_btn_y0 + idx * (minst_btn_h + minst_btn_gap);
}

// ============================================
// SCREEN INTERFACE
// ============================================

void minst_Draw() {
  minst_btn_pressed = -1;
  minst_home_pressed = false;

  tft.fillScreen(minst_bg);

  // Title
  tft.setTextColor(minst_title_col, minst_bg);
  tft.setTextFont(2);
  tft.setTextDatum(TC_DATUM);
  tft.setTextPadding(0);
  tft.drawString("INSTALLATORE", 160, minst_title_y);

  // Separator
  tft.drawFastHLine(10, minst_btn_y0 - 8, 300, TFT_DARKGREY);

  // Menu buttons
  for (int i = 0; i < minst_num_btns; i++) {
    minst_draw_btn(minst_btn_x, minst_btn_y(i), minst_btn_w, minst_btn_h,
                   minst_labels[i], minst_btn_col);
  }

  // Separator
  tft.drawFastHLine(10, minst_bottom_y - 8, 300, TFT_DARKGREY);

  // HOME
  minst_draw_btn(minst_btn_home_x, minst_bottom_y, minst_btn_home_w, minst_btn_home_h,
                 "HOME", minst_btn_col);
}

void minst_Update() {
  // Niente da aggiornare dinamicamente
}

void minst_Touch(int tx, int ty) {
  // --- Menu buttons ---
  for (int i = 0; i < minst_num_btns; i++) {
    int by = minst_btn_y(i);
    if (tx >= minst_btn_x && tx <= minst_btn_x + minst_btn_w &&
        ty >= by && ty <= by + minst_btn_h) {
      minst_btn_pressed = i;
      minst_draw_btn(minst_btn_x, by, minst_btn_w, minst_btn_h,
                     minst_labels[i], minst_btn_push);
      return;
    }
  }

  // --- HOME ---
  if (tx >= minst_btn_home_x && tx <= minst_btn_home_x + minst_btn_home_w &&
      ty >= minst_bottom_y && ty <= minst_bottom_y + minst_btn_home_h) {
    minst_home_pressed = true;
    minst_draw_btn(minst_btn_home_x, minst_bottom_y, minst_btn_home_w, minst_btn_home_h,
                   "HOME", minst_btn_push);
  }
}

void minst_Release() {
  // --- Menu button ---
  if (minst_btn_pressed >= 0) {
    int idx = minst_btn_pressed;
    minst_btn_pressed = -1;
    minst_draw_btn(minst_btn_x, minst_btn_y(idx), minst_btn_w, minst_btn_h,
                   minst_labels[idx], minst_btn_col);
    setScreen(minst_targets[idx]);
  }

  // --- HOME ---
  if (minst_home_pressed) {
    minst_home_pressed = false;
    minst_draw_btn(minst_btn_home_x, minst_bottom_y, minst_btn_home_w, minst_btn_home_h,
                   "HOME", minst_btn_col);
    setScreen(SCREEN_MAIN);
  }
}
