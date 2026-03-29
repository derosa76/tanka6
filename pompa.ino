// pompa.ino - Logica controllo pompa via Shelly (1 canale)
//
// Modo 0: Disabilitato
// Modo 1: Riempimento (livello < L → ON, livello > H → OFF)
// Modo 2: Svuotamento  (livello > H → ON, livello < L → OFF)
//
// L'isteresi è data dal gap H−L. Nessun parametro extra.
// Retry/keepalive/auto-off gestiti da ShellyController.

bool pump_running = false;
unsigned long pompa_last_run = 0;

void pompa_run() {
  if (millis() - pompa_last_run < 1000) return;
  pompa_last_run = millis();

  int mode = config["pump_mode"] | 0;

  // Modo disabilitato: spegni se era accesa
  if (mode == 0) {
    if (pump_running) {
      shelly.off(0);
      pump_running = false;
      Serial.println("[POMPA] Disabilitata, spengo");
    }
    return;
  }

  // Nessuna lettura o sensore stale: spegni per sicurezza
  if (!sensor.hasReading() || sensor.isStale()) {
    if (pump_running) {
      shelly.off(0);
      pump_running = false;
      Serial.println("[POMPA] Sensore non disponibile, spengo per sicurezza");
    }
    return;
  }

  // Calibrazione disponibile?
  if (config["calib_max_level"].isNull() || config["calib_min_level"].isNull()) {
    if (pump_running) {
      shelly.off(0);
      pump_running = false;
      Serial.println("[POMPA] Non calibrato, spengo per sicurezza");
    }
    return;
  }

  float distFull  = config["calib_max_level"].as<float>();
  float distEmpty = config["calib_min_level"].as<float>();
  float livello = sensor.getLevel(distFull, distEmpty);
  if (isnan(livello)) return;  // calibrazione invertita

  float H = config["pump_H"] | 80.0f;
  float L = config["pump_L"] | 20.0f;

  if (mode == 1) {  // Riempimento
    if (!pump_running && livello < L) {
      shelly.on(0);
      pump_running = true;
      Serial.printf("[POMPA] ON (riempimento, livello %.1f%% < L=%.0f%%)\n", livello, L);
    }
    else if (pump_running && livello > H) {
      shelly.off(0);
      pump_running = false;
      Serial.printf("[POMPA] OFF (riempimento, livello %.1f%% > H=%.0f%%)\n", livello, H);
    }
  }
  else if (mode == 2) {  // Svuotamento
    if (!pump_running && livello > H) {
      shelly.on(0);
      pump_running = true;
      Serial.printf("[POMPA] ON (svuotamento, livello %.1f%% > H=%.0f%%)\n", livello, H);
    }
    else if (pump_running && livello < L) {
      shelly.off(0);
      pump_running = false;
      Serial.printf("[POMPA] OFF (svuotamento, livello %.1f%% < L=%.0f%%)\n", livello, L);
    }
  }
}
