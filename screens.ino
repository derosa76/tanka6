// screens.ino - gestione schermate
//X = 320 pixel (larghezza, parte lunga orizzontale)
//Y = 240 pixel (altezza, parte corta verticale)

// Tipi per le funzioni schermata
typedef void (*ScreenDrawFunc)();
typedef void (*ScreenUpdateFunc)();
typedef void (*ScreenTouchFunc)(int x, int y);
typedef void (*ScreenReleaseFunc)();

// Struttura schermata
struct Screen {
  ScreenDrawFunc draw;      // disegna UI (chiamata una volta all'ingresso)
  ScreenUpdateFunc update;  // aggiorna dati (chiamata nel loop)
  ScreenTouchFunc onTouch;  // gestisce touch
  ScreenReleaseFunc onRelease;
};


// Array di schermate
Screen screens[SCREEN_COUNT];
ScreenID currentScreen = SCREEN_MAIN;
bool screenChanged = true;  // flag per ridisegno

// Cambio schermata
void setScreen(ScreenID id) {
  if (id != currentScreen) {
    currentScreen = id;
    screenChanged = true;
  }
}

// Forza ridisegno della schermata corrente (usato da toggleRotation)
void forceScreenRedraw() {
  screenChanged = true;
}

// Setup schermate (chiamare in setup())
void setupScreens() {
  screens[SCREEN_MAIN]         = {main_Draw, main_Update, main_Touch, main_Release};
  screens[SCREEN_HOME_IDLE]    = {idle_Draw, idle_Update, idle_Touch, idle_Release};
  screens[SCREEN_CALIBRAZIONE] = {cal_Draw, cal_Update, cal_Touch, cal_Release};
  screens[SCREEN_POMPA]        = {pompa_Draw, pompa_Update, pompa_Touch, pompa_Release};
  screens[SCREEN_PIN]          = {pin_Draw, pin_Update, pin_Touch, pin_Release};
  screens[SCREEN_MENU_INST]    = {minst_Draw, minst_Update, minst_Touch, minst_Release};
  screens[SCREEN_SENSORE]      = {rsens_Draw, rsens_Update, rsens_Touch, rsens_Release};
}

// Loop schermate (chiamare in loop())
void loopScreens() {
  // Ridisegna solo se cambiata
  if (screenChanged) {
    screens[currentScreen].draw();
    screenChanged = false;
  }

  // Scadenza passiva sessione installatore
  if (menu_installatore_allowed && millis() - last_touch_time >= MENU_INST_TIMEOUT) {
    menu_installatore_allowed = false;
    Serial.println("[PIN] Sessione installatore scaduta");
  }


  // Idle timeout centralizzato
  if (currentScreen != SCREEN_HOME_IDLE) {
    unsigned long timeout = (currentScreen == SCREEN_MAIN) ? IDLE_TIMEOUT_HOME : IDLE_TIMEOUT_SUB;
    if (millis() - last_touch_time >= timeout) {
      setScreen(SCREEN_HOME_IDLE);
      // Forza draw immediato (setScreen ha settato screenChanged)
      screens[currentScreen].draw();
      screenChanged = false;
      return;
    }
  }

  // Update sempre
  screens[currentScreen].update();
}

// Gestione touch (chiamare quando c'è un touch)
void handleScreenTouch(int x, int y) {
  if (screens[currentScreen].onTouch) {
    screens[currentScreen].onTouch(x, y);
  }
}

//gestione del release
void handleScreenRelease() {
  if (screens[currentScreen].onRelease) {
    screens[currentScreen].onRelease();
  }
}
