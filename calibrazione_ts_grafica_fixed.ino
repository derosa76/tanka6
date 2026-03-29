
void calibrazione_ts_setup(bool forza_calibrazione){
  if (forza_calibrazione){
    Serial.println("Calibrazione forzata da codice...");
    calibrateTouch();
  }
  else{
    // Prova a caricare calibrazione
    if (!loadCalibration()) {
      Serial.println("Calibrazione non trovata, avvio procedura...");
      calibrateTouch();
    } else {
      Serial.println("Calibrazione touchscreen caricata con successo");
    }
  }
}


// ============================================
// STRUTTURA DATI CALIBRAZIONE
// ============================================

struct TouchCalibration {
  int16_t x_min;
  int16_t x_max;
  int16_t y_min;
  int16_t y_max;
  bool valid;
};

TouchCalibration touchCal;

// Salva calibrazione nel config principale
bool saveCalibration() {
  config["ts_x_min"] = touchCal.x_min;
  config["ts_x_max"] = touchCal.x_max;
  config["ts_y_min"] = touchCal.y_min;
  config["ts_y_max"] = touchCal.y_max;
  config["ts_valid"] = touchCal.valid;
  config.commit();  // scrivi subito — calibrazione è critica

  Serial.println("Calibrazione salvata in config");
  return true;
}

// Carica calibrazione dal config principale
bool loadCalibration() {
  if (config["ts_valid"].isNull()) {
    Serial.println("Calibrazione touchscreen non presente in config");
    return false;
  }

  touchCal.x_min = config["ts_x_min"] | 0;
  touchCal.x_max = config["ts_x_max"] | 0;
  touchCal.y_min = config["ts_y_min"] | 0;
  touchCal.y_max = config["ts_y_max"] | 0;
  touchCal.valid = config["ts_valid"] | false;

  Serial.println("=== Calibrazione touchscreen caricata ===");
  Serial.printf("X: %d - %d\n", touchCal.x_min, touchCal.x_max);
  Serial.printf("Y: %d - %d\n", touchCal.y_min, touchCal.y_max);

  return touchCal.valid;
}

// ============================================
// PROCEDURA CALIBRAZIONE
// ============================================

void calibrateTouch() {
  const int TOUCHES_PER_POINT = 4;
  const int PROXIMITY_THRESHOLD = 50;  // Aumentato per essere più tollerante
  
  Serial.println("\n=== INIZIO CALIBRAZIONE TOUCHSCREEN ===");
  
  // ==========================================
  // RESET COMPLETO DELLO STATO GRAFICO
  // ==========================================
  tft.setFreeFont(nullptr);      // Rimuovi font custom
  tft.setTextFont(1);            // Font built-in piccolo di default
  tft.setTextSize(1);            // Size normale
  tft.setTextPadding(0);         // CRITICO: rimuovi padding!
  tft.setTextDatum(TL_DATUM);    // Top-left alignment di default
  tft.setTextWrap(false);        // No word wrap
  
  // 4 punti ben distanziati dai bordi
  struct CalPoint {
    int16_t screen_x;
    int16_t screen_y;
    int32_t raw_x_sum;
    int32_t raw_y_sum;
    int count;
  };
  
  // Coordinate ottimizzate (schermo 320x240)
  CalPoint points[4] = {
    {40, 40, 0, 0, 0},       // 0: Alto-sinistra (40px margine)
    {280, 40, 0, 0, 0},      // 1: Alto-destra (40px margine)
    {280, 200, 0, 0, 0},     // 2: Basso-destra (40px margine)
    {40, 200, 0, 0, 0}       // 3: Basso-sinistra (40px margine)
  };
  
  Serial.println("Coordinate cerchi:");
  for (int i = 0; i < 4; i++) {
    Serial.printf("  Punto %d: (%d, %d)\n", i, points[i].screen_x, points[i].screen_y);
  }
  
  int last_touched = -1;
  bool calibration_complete = false;
  
  // ==========================================
  // SCHERMATA INIZIALE
  // ==========================================
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextSize(1);
  tft.setTextDatum(MC_DATUM);
  
  tft.drawString("CALIBRAZIONE TOUCH", 160, 60);
  tft.setTextFont(1);
  tft.drawString("Tocca ogni cerchio 4 volte", 160, 100);
  tft.drawString("Punti diversi consecutivamente", 160, 120);
  
  delay(3000);
  
  // ==========================================
  // LOOP PRINCIPALE CALIBRAZIONE
  // ==========================================
  while (!calibration_complete) {
    
    // Pulisci schermo
    tft.fillScreen(TFT_BLACK);
    
    // ==========================================
    // DISEGNA I 4 CERCHI + CONTATORI
    // ==========================================
    for (int i = 0; i < 4; i++) {
      uint16_t color = (points[i].count >= TOUCHES_PER_POINT) ? TFT_GREEN : TFT_RED;
      
      // Cerchio esterno (raggio 18px)
      tft.fillCircle(points[i].screen_x, points[i].screen_y, 18, color);
      
      // Cerchio medio bianco (contrasto)
      tft.drawCircle(points[i].screen_x, points[i].screen_y, 12, TFT_WHITE);
      tft.drawCircle(points[i].screen_x, points[i].screen_y, 13, TFT_WHITE);
      
      // Punto centrale (mirino)
      tft.fillCircle(points[i].screen_x, points[i].screen_y, 3, TFT_WHITE);
      
      // ==========================================
      // CONTATORE - Posizione calcolata per non sovrapporsi
      // ==========================================
      tft.setTextPadding(0);  // IMPORTANTE: nessun padding!
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.setTextFont(4);
      tft.setTextSize(1);
      tft.setTextDatum(MC_DATUM);
      
      char counter[5];
      sprintf(counter, "%d", points[i].count);
      
      // Posiziona contatore SOPRA o SOTTO il cerchio
      int counter_x = points[i].screen_x;
      int counter_y;
      
      if (i == 0 || i == 1) {
        // Punti in alto → contatore SOTTO
        counter_y = points[i].screen_y + 35;
      } else {
        // Punti in basso → contatore SOPRA
        counter_y = points[i].screen_y - 35;
      }
      
      tft.drawString(counter, counter_x, counter_y);
    }
    
    // ==========================================
    // MESSAGGIO CENTRALE
    // ==========================================
    tft.setTextPadding(0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    
    if (last_touched >= 0) {
      tft.fillRect(0, 110, 320, 30, TFT_NAVY);  // Box blu scuro
      tft.setTextColor(TFT_YELLOW, TFT_NAVY);
      tft.drawString("Tocca un ALTRO punto!", 160, 120);
    } else {
      tft.setTextColor(TFT_CYAN, TFT_BLACK);
      tft.drawString("Tocca un cerchio rosso", 160, 120);
    }
    
    // ==========================================
    // ATTENDI TOCCO
    // ==========================================
    bool valid_touch = false;
    
    while (!valid_touch) {
      if (ts.tirqTouched() && ts.touched()) {
        TS_Point p = ts.getPoint();
        
        // Converti raw in pixel (usa valori default approssimativi)
        int touch_x = map(p.x, 200, 3700, 0, 319);
        int touch_y = map(p.y, 240, 3800, 0, 239);
        
        // Mirror per rotazione 180°
        if ((config["display_rotation"] | 1) == 3) {
          touch_x = 319 - touch_x;
          touch_y = 239 - touch_y;
        }
        
        Serial.printf("Touch raw: (%d, %d) -> pixel: (%d, %d)\n", p.x, p.y, touch_x, touch_y);
        
        // Trova il punto più vicino
        int closest_point = -1;
        int min_distance = 9999;
        
        for (int i = 0; i < 4; i++) {
          int dx = touch_x - points[i].screen_x;
          int dy = touch_y - points[i].screen_y;
          int distance = sqrt(dx*dx + dy*dy);
          
          Serial.printf("  Distanza da punto %d: %d px\n", i, distance);
          
          if (distance < min_distance && distance < PROXIMITY_THRESHOLD) {
            min_distance = distance;
            closest_point = i;
          }
        }
        
        // ==========================================
        // GESTIONE TOCCO
        // ==========================================
        if (closest_point >= 0 && closest_point != last_touched) {
          // Tocco valido su punto diverso dall'ultimo
          
          if (points[closest_point].count < TOUCHES_PER_POINT) {
            // Accetta il tocco
            points[closest_point].raw_x_sum += p.x;
            points[closest_point].raw_y_sum += p.y;
            points[closest_point].count++;
            
            Serial.printf("✓ Punto %d: touch %d/%d registrato\n", 
                         closest_point, points[closest_point].count, TOUCHES_PER_POINT);
            
            last_touched = closest_point;
            valid_touch = true;
            
            // Feedback visivo CYAN
            tft.fillCircle(points[closest_point].screen_x, 
                          points[closest_point].screen_y, 18, TFT_CYAN);
            delay(200);
          }
          
        } else if (closest_point == last_touched) {
          // Stesso punto - ERRORE
          Serial.printf("✗ Errore: stesso punto %d toccato di nuovo\n", closest_point);
          
          tft.fillScreen(TFT_RED);
          tft.setTextColor(TFT_WHITE, TFT_RED);
          tft.setTextFont(2);
          tft.setTextDatum(MC_DATUM);
          tft.drawString("TOCCA UN PUNTO DIVERSO!", 160, 120);
          delay(700);
          
          valid_touch = true;  // Esci per ridisegnare
          
        } else {
          // Tocco fuori da tutti i punti
          Serial.println("✗ Tocco troppo lontano da tutti i punti");
          delay(100);
        }
        
        // Attendi rilascio
        while (ts.touched()) {
          delay(10);
        }
        delay(100);  // Debounce
      }
      delay(10);
    }
    
    // ==========================================
    // VERIFICA COMPLETAMENTO
    // ==========================================
    calibration_complete = true;
    for (int i = 0; i < 4; i++) {
      if (points[i].count < TOUCHES_PER_POINT) {
        calibration_complete = false;
        break;
      }
    }
    
    // Permetti di rifare qualsiasi punto dopo che tutti ne hanno almeno 1
    bool all_started = true;
    for (int i = 0; i < 4; i++) {
      if (points[i].count == 0) {
        all_started = false;
        break;
      }
    }
    if (all_started) {
      last_touched = -1;
    }
  }
  
  // ==========================================
  // CALCOLO CALIBRAZIONE FINALE CON ESTRAPOLAZIONE
  // ==========================================
  int16_t raw_x_avg[4], raw_y_avg[4];
  
  Serial.println("\n=== Medie calcolate ===");
  for (int i = 0; i < 4; i++) {
    raw_x_avg[i] = points[i].raw_x_sum / points[i].count;
    raw_y_avg[i] = points[i].raw_y_sum / points[i].count;
    Serial.printf("Punto %d (pixel %d,%d): raw_x=%d, raw_y=%d\n", 
                  i, points[i].screen_x, points[i].screen_y, raw_x_avg[i], raw_y_avg[i]);
  }
  
  // Calcola valori raw MEDI per ogni lato
  float raw_x_left = (raw_x_avg[0] + raw_x_avg[3]) / 2.0;   // Media punti sinistri (0,3)
  float raw_x_right = (raw_x_avg[1] + raw_x_avg[2]) / 2.0;  // Media punti destri (1,2)
  float raw_y_top = (raw_y_avg[0] + raw_y_avg[1]) / 2.0;    // Media punti alti (0,1)
  float raw_y_bottom = (raw_y_avg[2] + raw_y_avg[3]) / 2.0; // Media punti bassi (2,3)
  
  Serial.println("\n=== Valori raw medi per lato ===");
  Serial.printf("raw_x_left (pixel 40): %.1f\n", raw_x_left);
  Serial.printf("raw_x_right (pixel 280): %.1f\n", raw_x_right);
  Serial.printf("raw_y_top (pixel 40): %.1f\n", raw_y_top);
  Serial.printf("raw_y_bottom (pixel 200): %.1f\n", raw_y_bottom);
  
  // ESTRAPOLAZIONE LINEARE per trovare raw ai bordi reali (0,0) e (319,239)
  // Formula: raw = raw_left + (pixel - pixel_left) * (raw_right - raw_left) / (pixel_right - pixel_left)
  
  // Per X: punti calibrazione sono a pixel 40 e 280
  float slope_x = (raw_x_right - raw_x_left) / (280.0 - 40.0);
  touchCal.x_min = (int16_t)(raw_x_left - 40.0 * slope_x);      // Estrapola a pixel 0
  touchCal.x_max = (int16_t)(raw_x_right + 39.0 * slope_x);     // Estrapola a pixel 319
  
  // Per Y: punti calibrazione sono a pixel 40 e 200
  float slope_y = (raw_y_bottom - raw_y_top) / (200.0 - 40.0);
  touchCal.y_min = (int16_t)(raw_y_top - 40.0 * slope_y);       // Estrapola a pixel 0
  touchCal.y_max = (int16_t)(raw_y_bottom + 39.0 * slope_y);    // Estrapola a pixel 239
  
  touchCal.valid = true;
  
  Serial.println("\n=== CALIBRAZIONE FINALE (estrapolata) ===");
  Serial.printf("slope_x = %.2f raw/pixel\n", slope_x);
  Serial.printf("slope_y = %.2f raw/pixel\n", slope_y);
  Serial.printf("X range: %d (pixel 0) → %d (pixel 319)\n", touchCal.x_min, touchCal.x_max);
  Serial.printf("Y range: %d (pixel 0) → %d (pixel 239)\n", touchCal.y_min, touchCal.y_max);
  
  // ==========================================
  // SALVATAGGIO E CONFERMA
  // ==========================================
  if (saveCalibration()) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextDatum(MC_DATUM);
    tft.setTextPadding(0);
    
    tft.drawString("CALIBRAZIONE OK!", 160, 80);
    
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(1);
    char buf[50];
    sprintf(buf, "X: %d - %d", touchCal.x_min, touchCal.x_max);
    tft.drawString(buf, 160, 120);
    sprintf(buf, "Y: %d - %d", touchCal.y_min, touchCal.y_max);
    tft.drawString(buf, 160, 140);
    
    Serial.println("✓ Calibrazione salvata in config");
    
  } else {
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.setTextFont(2);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("ERRORE SALVATAGGIO!", 160, 100);
    tft.setTextFont(1);
    tft.drawString("Vedi Serial Monitor", 160, 130);
    
    Serial.println("✗ ERRORE salvataggio calibrazione");
  }
  
  delay(3000);
}

// ============================================
// FUNZIONE USO CALIBRAZIONE
// ============================================

void getTouchCalibrated(int16_t &x, int16_t &y) {
  if (!ts.touched()) {
    x = -1;
    y = -1;
    return;
  }
  
  TS_Point p = ts.getPoint();
  
  if (!touchCal.valid) {
    // Usa valori default se non calibrato
    x = map(p.x, 200, 3700, 0, 319);
    y = map(p.y, 240, 3800, 0, 239);
  } else {
    // Usa calibrazione salvata
    x = map(p.x, touchCal.x_min, touchCal.x_max, 0, 319);
    y = map(p.y, touchCal.y_min, touchCal.y_max, 0, 239);
  }
  
  // Clamp ai bordi
  x = constrain(x, 0, 319);
  y = constrain(y, 0, 239);

  // Specchia per rotazione 180° — ts.setRotation(1) è fisso,
  // la rotazione display 1↔3 richiede solo il mirror dei pixel finali.
  // I coefficienti di calibrazione restano identici.
  if ((config["display_rotation"] | 1) == 3) {
    x = 319 - x;
    y = 239 - y;
  }
}
