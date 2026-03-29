unsigned long last_touch_run_time=0;

void touchscreen_setup() {
  touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  ts.begin(touchSPI);
  ts.setRotation(1);
}


/*
int last_touch_x = -1;
int last_touch_y = -1;

void touchscreen_run(unsigned long interval_ms) {
  if ((millis()-last_touch)>=interval_ms) {
    if (ts.touched()) {
      TS_Point p = ts.getPoint();
      
      // Usa la calibrazione se disponibile
      int16_t touch_x, touch_y;
      getTouchCalibrated(touch_x, touch_y);

      
      
      if (touch_x >= 0) {  // Touch valido
        handleScreenTouch(touch_x, touch_y); //funzione in screens.ino()
        

        Serial.println("touch on: "+String(p.x)+" ; "+String(p.y)+" -> pixel: "+String(touch_x)+","+String(touch_y));
        
        // CANCELLA il cerchio precedente PRIMA di disegnare quello nuovo
        if (last_touch_x >= 0 && last_touch_y >= 0) {
          tft.fillCircle(last_touch_x, last_touch_y, 10, TFT_BLACK);
        }
        
        // Disegna cerchio rosso nella nuova posizione
        tft.fillCircle(touch_x, touch_y, 8, TFT_RED);
        tft.drawCircle(touch_x, touch_y, 9, TFT_WHITE);  // Bordo bianco per visibilità
        
        // Salva nuova posizione
        last_touch_x = touch_x;
        last_touch_y = touch_y;
        
      }
    }
    
    else {
      // Dito/pennino sollevato - elimina il cerchio rosso
      if (last_touch_x >= 0 && last_touch_y >= 0) {
        // Cancella ridisegnando in nero
        tft.fillCircle(last_touch_x, last_touch_y, 10, TFT_BLACK);
        
        Serial.println("touch released");
        
        // Reset posizione
        last_touch_x = -1;
        last_touch_y = -1;
      }
    }
    last_touch=millis();
  }
  
}
*/

bool was_touched = false; 

void touchscreen_run(unsigned long interval_ms) {
  if ((millis()-last_touch_run_time)>=interval_ms) {
    if (ts.touched()) {
      TS_Point p = ts.getPoint();
      int16_t touch_x, touch_y; 
      getTouchCalibrated(touch_x, touch_y);// Usa la calibrazione se disponibile
      if (touch_x >= 0) handleScreenTouch(touch_x, touch_y); // Se touch valido -->funzione in screens.ino()
      was_touched = true;
      last_touch_time = millis();  // Reset idle timer globale
    }
    else if (was_touched) {
      // Appena rilasciato!
      handleScreenRelease();  // ← Nuova funzione
      was_touched = false;
    }

    last_touch_run_time=millis();
  }
}





