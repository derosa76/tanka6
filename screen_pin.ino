// screen_pin.ino - Schermata tastierino PIN
//
// Tastierino numerico 3×4, PIN 4 cifre.
// SCOPRI: tieni premuto per rivelare le cifre.
// VAI: verifica PIN → se corretto, sblocca installatore.
// Pattern touch/release standard — flag su touch, azione su release.

// ============================================
// PIN VALUES
// ============================================

const char PIN_INSTALLATORE[] = "1234";
// TODO: PIN_SVILUPPATORE per livello debug

// ============================================
// LAYOUT
// ============================================

const int pin_title_y = 2;

// PIN display — 4 boxes
const int pin_disp_x0  = 103;
const int pin_disp_y   = 22;
const int pin_disp_w   = 24;
const int pin_disp_h   = 22;
const int pin_disp_gap = 6;

// Keypad 3×4
const int pin_key_x0   = 68;
const int pin_key_y0   = 50;
const int pin_key_w    = 56;
const int pin_key_h    = 30;
const int pin_key_xgap = 8;
const int pin_key_ygap = 4;
// Row y = pin_key_y0 + row * (pin_key_h + pin_key_ygap)
// Row 0: y=50, Row 1: y=84, Row 2: y=118, Row 3: y=152
// Bottom of keypad: 152+30 = 182

// Message area (PIN ERRATO / vuoto)
const int pin_msg_y = 186;

// Bottom buttons
const int pin_bottom_y     = 206;
const int pin_bottom_h     = 28;
const int pin_btn_home_x   = 8;
const int pin_btn_home_w   = 92;
const int pin_btn_scopri_x = 112;
const int pin_btn_scopri_w = 94;
const int pin_btn_vai_x    = 218;
const int pin_btn_vai_w    = 94;

// ============================================
// COLORI
// ============================================

const uint16_t pin_bg          = TFT_BLACK;
const uint16_t pin_title_col   = TFT_CYAN;
const uint16_t pin_key_bg      = 0x2945;       // dark blue-grey
const uint16_t pin_key_fg      = TFT_WHITE;
const uint16_t pin_key_spec_bg = 0x4208;       // lighter grey for < and C
const uint16_t pin_key_push    = TFT_RED;
const uint16_t pin_disp_bg     = TFT_NAVY;
const uint16_t pin_disp_border = TFT_WHITE;
const uint16_t pin_disp_fg     = TFT_WHITE;
const uint16_t pin_btn_col     = TFT_ORANGE;
const uint16_t pin_scopri_col  = TFT_CYAN;
const uint16_t pin_vai_active  = TFT_GREEN;
const uint16_t pin_vai_inactive = TFT_DARKGREY;

// ============================================
// STATE
// ============================================

char pin_digits[5];   // 4 digits + null
int  pin_pos = 0;     // digits entered (0-4)

int  pin_key_active = -1;        // pressed key index (0-11), -1=none
bool pin_scopri_pressed = false;
bool pin_btn_home_pressed = false;
bool pin_btn_vai_pressed = false;

// Keymap
const char pin_keymap[4][3] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'<','0','C'}
};

// ============================================
// DRAW HELPERS
// ============================================

void pin_draw_btn(int x, int y, int w, int h, const char* text, uint16_t col) {
  tft.fillRoundRect(x, y, w, h, 4, col);
  tft.setTextColor(TFT_BLACK, col);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.setTextPadding(0);
  tft.drawString(text, x + w / 2, y + h / 2);
}

void pin_draw_key(int row, int col, bool pressed) {
  int x = pin_key_x0 + col * (pin_key_w + pin_key_xgap);
  int y = pin_key_y0 + row * (pin_key_h + pin_key_ygap);
  char ch = pin_keymap[row][col];

  uint16_t bg;
  if (pressed) {
    bg = pin_key_push;
  } else if (ch == '<' || ch == 'C') {
    bg = pin_key_spec_bg;
  } else {
    bg = pin_key_bg;
  }

  tft.fillRoundRect(x, y, pin_key_w, pin_key_h, 4, bg);
  tft.drawRoundRect(x, y, pin_key_w, pin_key_h, 4, TFT_DARKGREY);
  tft.setTextColor(pin_key_fg, bg);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.setTextPadding(0);

  // Label
  char label[4];
  if (ch == '<') {
    strcpy(label, "<");   // backspace
  } else {
    label[0] = ch;
    label[1] = '\0';
  }
  tft.drawString(label, x + pin_key_w / 2, y + pin_key_h / 2);
}

void pin_draw_display() {
  for (int i = 0; i < 4; i++) {
    int x = pin_disp_x0 + i * (pin_disp_w + pin_disp_gap);
    tft.fillRoundRect(x, pin_disp_y, pin_disp_w, pin_disp_h, 3, pin_disp_bg);
    tft.drawRoundRect(x, pin_disp_y, pin_disp_w, pin_disp_h, 3, pin_disp_border);

    if (i < pin_pos) {
      tft.setTextColor(pin_disp_fg, pin_disp_bg);
      tft.setTextFont(2);
      tft.setTextDatum(MC_DATUM);
      tft.setTextPadding(0);

      if (pin_scopri_pressed) {
        char buf[2] = {pin_digits[i], '\0'};
        tft.drawString(buf, x + pin_disp_w / 2, pin_disp_y + pin_disp_h / 2);
      } else {
        tft.drawString("*", x + pin_disp_w / 2, pin_disp_y + pin_disp_h / 2);
      }
    }
  }
}

void pin_draw_vai() {
  uint16_t col = (pin_pos == 4) ? pin_vai_active : pin_vai_inactive;
  if (pin_btn_vai_pressed && pin_pos == 4) col = pin_key_push;
  pin_draw_btn(pin_btn_vai_x, pin_bottom_y, pin_btn_vai_w, pin_bottom_h, "VAI", col);
}

void pin_clear_message() {
  tft.fillRect(0, pin_msg_y, 320, 16, pin_bg);
}

void pin_show_error() {
  pin_clear_message();
  tft.setTextColor(TFT_RED, pin_bg);
  tft.setTextFont(2);
  tft.setTextDatum(MC_DATUM);
  tft.setTextPadding(0);
  tft.drawString("PIN ERRATO", 160, pin_msg_y);
}

// ============================================
// SCREEN INTERFACE
// ============================================

void pin_Draw() {
  // Reset state
  pin_pos = 0;
  memset(pin_digits, 0, sizeof(pin_digits));
  pin_key_active = -1;
  pin_scopri_pressed = false;
  pin_btn_home_pressed = false;
  pin_btn_vai_pressed = false;

  tft.fillScreen(pin_bg);

  // Title
  tft.setTextColor(pin_title_col, pin_bg);
  tft.setTextFont(2);
  tft.setTextDatum(TC_DATUM);
  tft.setTextPadding(0);
  tft.drawString("INSERISCI PIN", 160, pin_title_y);

  // Separator
  tft.drawFastHLine(10, pin_disp_y - 4, 300, TFT_DARKGREY);

  // PIN display
  pin_draw_display();

  // Keypad
  for (int r = 0; r < 4; r++)
    for (int c = 0; c < 3; c++)
      pin_draw_key(r, c, false);

  // Separator sotto keypad
  tft.drawFastHLine(10, pin_msg_y - 2, 300, TFT_DARKGREY);

  // Bottom buttons
  pin_draw_btn(pin_btn_home_x, pin_bottom_y, pin_btn_home_w, pin_bottom_h,
               "HOME", pin_btn_col);
  pin_draw_btn(pin_btn_scopri_x, pin_bottom_y, pin_btn_scopri_w, pin_bottom_h,
               "SCOPRI", pin_scopri_col);
  pin_draw_vai();
}

void pin_Update() {
  // Niente da aggiornare dinamicamente
}

void pin_Touch(int tx, int ty) {
  // --- Keypad 3×4 ---
  for (int row = 0; row < 4; row++) {
    int ky = pin_key_y0 + row * (pin_key_h + pin_key_ygap);
    if (ty < ky || ty >= ky + pin_key_h) continue;
    for (int col = 0; col < 3; col++) {
      int kx = pin_key_x0 + col * (pin_key_w + pin_key_xgap);
      if (tx >= kx && tx < kx + pin_key_w) {
        int idx = row * 3 + col;
        if (pin_key_active != idx) {
          // Rilascia eventuale key precedente (slide tra tasti)
          if (pin_key_active >= 0) {
            pin_draw_key(pin_key_active / 3, pin_key_active % 3, false);
          }
          pin_key_active = idx;
          pin_draw_key(row, col, true);
        }
        return;
      }
    }
  }

  // --- SCOPRI (hold to reveal) ---
  if (tx >= pin_btn_scopri_x && tx < pin_btn_scopri_x + pin_btn_scopri_w &&
      ty >= pin_bottom_y && ty < pin_bottom_y + pin_bottom_h) {
    if (!pin_scopri_pressed) {
      pin_scopri_pressed = true;
      pin_draw_btn(pin_btn_scopri_x, pin_bottom_y, pin_btn_scopri_w, pin_bottom_h,
                   "SCOPRI", pin_key_push);
      pin_draw_display();  // mostra cifre
    }
    return;
  }

  // --- HOME ---
  if (tx >= pin_btn_home_x && tx < pin_btn_home_x + pin_btn_home_w &&
      ty >= pin_bottom_y && ty < pin_bottom_y + pin_bottom_h) {
    pin_btn_home_pressed = true;
    pin_draw_btn(pin_btn_home_x, pin_bottom_y, pin_btn_home_w, pin_bottom_h,
                 "HOME", pin_key_push);
    return;
  }

  // --- VAI (solo se 4 cifre inserite) ---
  if (pin_pos == 4 &&
      tx >= pin_btn_vai_x && tx < pin_btn_vai_x + pin_btn_vai_w &&
      ty >= pin_bottom_y && ty < pin_bottom_y + pin_bottom_h) {
    pin_btn_vai_pressed = true;
    pin_draw_vai();
    return;
  }
}

void pin_Release() {
  // --- SCOPRI release → nascondi cifre ---
  if (pin_scopri_pressed) {
    pin_scopri_pressed = false;
    pin_draw_btn(pin_btn_scopri_x, pin_bottom_y, pin_btn_scopri_w, pin_bottom_h,
                 "SCOPRI", pin_scopri_col);
    pin_draw_display();  // torna ad asterischi
  }

  // --- Keypad release → processa tasto ---
  if (pin_key_active >= 0) {
    int row = pin_key_active / 3;
    int col = pin_key_active % 3;
    char ch = pin_keymap[row][col];
    pin_draw_key(row, col, false);
    pin_key_active = -1;

    if (ch >= '0' && ch <= '9' && pin_pos < 4) {
      pin_digits[pin_pos++] = ch;
      pin_clear_message();
      pin_draw_display();
      pin_draw_vai();
    } else if (ch == '<' && pin_pos > 0) {
      pin_pos--;
      pin_clear_message();
      pin_draw_display();
      pin_draw_vai();
    } else if (ch == 'C') {
      pin_pos = 0;
      pin_clear_message();
      pin_draw_display();
      pin_draw_vai();
    }
  }

  // --- HOME ---
  if (pin_btn_home_pressed) {
    pin_btn_home_pressed = false;
    pin_draw_btn(pin_btn_home_x, pin_bottom_y, pin_btn_home_w, pin_bottom_h,
                 "HOME", pin_btn_col);
    setScreen(SCREEN_MAIN);
  }

  // --- VAI → verifica PIN ---
  if (pin_btn_vai_pressed) {
    pin_btn_vai_pressed = false;
    pin_draw_vai();

    if (pin_pos == 4) {
      pin_digits[4] = '\0';
      if (strcmp(pin_digits, PIN_INSTALLATORE) == 0) {
        menu_installatore_allowed = true;
        Serial.println("[PIN] Installatore sbloccato");
        setScreen(SCREEN_MENU_INST);
      } else {
        Serial.println("[PIN] PIN errato");
        pin_show_error();
        pin_pos = 0;
        pin_draw_display();
        pin_draw_vai();
      }
    }
  }
}
