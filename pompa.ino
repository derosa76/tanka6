// pompa.ino - Logica controllo pompa via Shelly (1 canale)
//
// Modo 0: Disabilitato
// Modo 1: Riempimento (livello < L → ON, livello > H → OFF)
// Modo 2: Svuotamento  (livello > H → ON, livello < L → OFF)
//
// FSM avanzata (quando pump_check_delay > 0):
//   IDLE        — pompa spenta, attesa soglie H/L
//   STARTING    — pompa accesa, countdown verifica portata
//   RUNNING     — pompa accesa, portata verificata OK
//   RETRY_WAIT  — portata insufficiente, countdown retry
//
// In RUNNING il watchdog usa lo stesso pump_check_delay:
// se per check_delay secondi la derivata è sotto soglia → RETRY_WAIT.
//
// Se pump_check_delay == 0: FSM disabilitata, comportamento legacy H/L puro.

// ============================================
// FSM STATE
// ============================================

enum PumpFsmState : uint8_t {
  PUMP_IDLE,
  PUMP_STARTING,
  PUMP_RUNNING,
  PUMP_RETRY_WAIT
};

// Prototipo esplicito — necessario perché l'IDE genera i prototipi automatici
// prima di processare gli enum, causando "not declared in this scope".
static void pump_fsm_enter(PumpFsmState s);

PumpFsmState pump_fsm_state = PUMP_IDLE;
bool         pump_running   = false;   // esposto globalmente per display

static unsigned long pump_fsm_timer = 0;  // timestamp ingresso stato corrente

// ============================================
// HELPER: verifica portata
// ============================================

static bool pump_flow_ok(int mode) {
  float rate = sensor.getDerivative();
  if (isnan(rate)) return false;
  float threshold = config["pump_flow_threshold"] | 5.0f;
  if (mode == 1) return rate < -threshold;   // Riempimento: livello sale → distanza cala
  else           return rate >  threshold;   // Svuotamento: livello scende → distanza cresce
}

// ============================================
// HELPER: tempo trascorso in stato corrente (ms)
// ============================================

static unsigned long pump_fsm_elapsed() {
  return millis() - pump_fsm_timer;
}

static void pump_fsm_enter(PumpFsmState s) {
  pump_fsm_state = s;
  pump_fsm_timer = millis();
}

// ============================================
// MAIN LOOP
// ============================================

unsigned long pompa_last_run = 0;

void pompa_run() {
  if (millis() - pompa_last_run < 500) return;
  pompa_last_run = millis();

  int mode = config["pump_mode"] | 0;

  // --- Modo disabilitato ---
  if (mode == 0) {
    if (pump_running) {
      shelly.off(0);
      pump_running = false;
      Serial.println("[POMPA] Disabilitata, spengo");
    }
    pump_fsm_enter(PUMP_IDLE);
    return;
  }

  // --- Sensore non disponibile: spegni per sicurezza ---
  if (!sensor.hasReading() || sensor.isStale()) {
    if (pump_running) {
      shelly.off(0);
      pump_running = false;
      Serial.println("[POMPA] Sensore non disponibile, spengo per sicurezza");
    }
    pump_fsm_enter(PUMP_IDLE);
    return;
  }

  // --- Calibrazione mancante ---
  if (config["calib_max_level"].isNull() || config["calib_min_level"].isNull()) {
    if (pump_running) {
      shelly.off(0);
      pump_running = false;
      Serial.println("[POMPA] Non calibrato, spengo per sicurezza");
    }
    pump_fsm_enter(PUMP_IDLE);
    return;
  }

  float distFull  = config["calib_max_level"].as<float>();
  float distEmpty = config["calib_min_level"].as<float>();
  float livello   = sensor.getLevel(distFull, distEmpty);
  if (isnan(livello)) return;

  float H = config["pump_H"] | 80.0f;
  float L = config["pump_L"] | 20.0f;

  // Soglie attivazione/spegnimento in base al modo
  bool should_start = (mode == 1) ? (livello < L) : (livello > H);
  bool should_stop  = (mode == 1) ? (livello > H) : (livello < L);

  int check_delay_s = config["pump_check_delay"] | 0;

  // ============================================
  // FSM disabilitata (check_delay == 0): logica legacy
  // ============================================
  if (check_delay_s == 0) {
    if (!pump_running && should_start) {
      shelly.on(0);
      pump_running = true;
      pump_fsm_enter(PUMP_RUNNING);
      Serial.printf("[POMPA] ON (legacy, livello %.1f%%)\n", livello);
    } else if (pump_running && should_stop) {
      shelly.off(0);
      pump_running = false;
      pump_fsm_enter(PUMP_IDLE);
      Serial.printf("[POMPA] OFF (legacy, livello %.1f%%)\n", livello);
    }
    return;
  }

  // ============================================
  // FSM avanzata
  // ============================================
  unsigned long check_delay_ms = (unsigned long)check_delay_s * 1000UL;
  unsigned long retry_wait_ms  = (unsigned long)(config["pump_retry_wait"] | 300) * 1000UL;

  switch (pump_fsm_state) {

    // ------------------------------------------
    case PUMP_IDLE:
      if (should_start) {
        shelly.on(0);
        pump_running = true;
        pump_fsm_enter(PUMP_STARTING);
        Serial.printf("[POMPA] IDLE→STARTING (livello %.1f%%)\n", livello);
      }
      break;

    // ------------------------------------------
    case PUMP_STARTING:
      // Spegnimento per soglia raggiunta prima della verifica
      if (should_stop) {
        shelly.off(0);
        pump_running = false;
        pump_fsm_enter(PUMP_IDLE);
        Serial.println("[POMPA] STARTING→IDLE (soglia raggiunta)");
        break;
      }
      // Attesa countdown
      if (pump_fsm_elapsed() < check_delay_ms) break;

      // Verifica portata
      if (pump_flow_ok(mode)) {
        pump_fsm_enter(PUMP_RUNNING);
        Serial.printf("[POMPA] STARTING→RUNNING (portata ok, rate=%.1f mm/min)\n",
                      sensor.getDerivative());
      } else {
        shelly.off(0);
        pump_running = false;
        pump_fsm_enter(PUMP_RETRY_WAIT);
        Serial.printf("[POMPA] STARTING→RETRY_WAIT (portata insufficiente, rate=%.1f mm/min)\n",
                      sensor.getDerivative());
      }
      break;

    // ------------------------------------------
    case PUMP_RUNNING:
      // Spegnimento per soglia raggiunta
      if (should_stop) {
        shelly.off(0);
        pump_running = false;
        pump_fsm_enter(PUMP_IDLE);
        Serial.printf("[POMPA] RUNNING→IDLE (soglia raggiunta, livello %.1f%%)\n", livello);
        break;
      }
      // Watchdog stallo: ogni check_delay_ms verifica la portata
      // Usiamo pump_fsm_timer come riferimento dell'ultimo check OK
      if (pump_fsm_elapsed() >= check_delay_ms) {
        if (!pump_flow_ok(mode)) {
          shelly.off(0);
          pump_running = false;
          pump_fsm_enter(PUMP_RETRY_WAIT);
          Serial.printf("[POMPA] RUNNING→RETRY_WAIT (stallo portata, rate=%.1f mm/min)\n",
                        sensor.getDerivative());
        } else {
          // Portata ancora ok: resetta il timer watchdog
          pump_fsm_timer = millis();
        }
      }
      break;

    // ------------------------------------------
    case PUMP_RETRY_WAIT:
      if (pump_fsm_elapsed() >= retry_wait_ms) {
        // Riprova solo se la soglia richiede ancora la pompa
        if (should_start) {
          shelly.on(0);
          pump_running = true;
          pump_fsm_enter(PUMP_STARTING);
          Serial.println("[POMPA] RETRY_WAIT→STARTING");
        } else {
          pump_fsm_enter(PUMP_IDLE);
          Serial.println("[POMPA] RETRY_WAIT→IDLE (soglia non più attiva)");
        }
      }
      break;
  }
}
