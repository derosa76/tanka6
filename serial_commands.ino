// serial_commands.ino - Comandi seriale: h=help, s=status, r=reboot

void serial_cmd_help() {
  Serial.println();
  Serial.println("=== TANKA 4.7 ===");
  Serial.println("  h = Help");
  Serial.println("  m = mostra misure raw, ema, estrapol. lin mm, rate mm/min ");
  Serial.println("  s = Status completo");
  Serial.println("  r = Reboot");
  Serial.println("=================");
  Serial.println();
}

void serial_cmd_measure(){
  Serial.print("raw, EMA(mm), rate(mm/min): ");
  Serial.print(sensor.getRawDistance(), 1); 
  Serial.print("\t");
  Serial.print(sensor.getDistance(), 1);
  Serial.print("\t");
  Serial.println(sensor.getDerivative(), 1);
}


void serial_cmd_status() {
  Serial.println();
  Serial.println("=== STATUS TANKA ===");
  
  // --- WiFi STA ---
  Serial.println();
  Serial.println("--- WiFi STA ---");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("  SSID:    %s\n", WiFi.SSID().c_str());
    Serial.printf("  IP:      %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("  RSSI:    %d dBm\n", WiFi.RSSI());
  } else {
    Serial.println("  Non connesso");
  }
  
  // --- WiFi AP ---
  Serial.println();
  Serial.println("--- WiFi AP ---");
  Serial.printf("  SSID:    %s\n", TANKA_AP_SSID.c_str());
  Serial.printf("  IP:      %s\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("  Clients: %d\n", WiFi.softAPgetStationNum());
  
  // --- Shelly ---
  Serial.println();
  Serial.println("--- Shelly ---");
  if (shelly.isOnline()) {
    Serial.printf("  IP:       %s\n", shelly.getIP());
    Serial.printf("  Stato:    %s\n", shelly.isOn(0) ? "ON" : "OFF");
    Serial.printf("  Allineato:%s\n", shelly.isAligned(0) ? " SI" : " NO (pending)");
    if (shelly.getPower(0) > 0 || shelly.getEnergy(0) > 0) {
      Serial.printf("  Potenza:  %.1f W\n", shelly.getPower(0));
      Serial.printf("  Energia:  %.1f Wh\n", shelly.getEnergy(0));
    }
  } else {
    Serial.println("  Offline");
  }
  
  // --- Pompa ---
  Serial.println();
  Serial.println("--- Pompa ---");
  int mode = config["pump_mode"] | 0;
  const char* mode_str = mode == 0 ? "OFF" : (mode == 1 ? "RIEMPIMENTO" : "SVUOTAMENTO");
  Serial.printf("  Modo:    %s\n", mode_str);
  Serial.printf("  H:       %.0f%%\n", config["pump_H"].as<float>());
  Serial.printf("  L:       %.0f%%\n", config["pump_L"].as<float>());
  Serial.printf("  Running: %s\n", pump_running ? "SI" : "NO");
  
  // --- Sensore ---
  Serial.println();
  Serial.println("--- Sensore ---");
  if (!config["calib_max_level"].isNull() && !config["calib_min_level"].isNull()) {
    float distFull  = config["calib_max_level"].as<float>();
    float distEmpty = config["calib_min_level"].as<float>();
    float livello = sensor.getLevel(distFull, distEmpty);
    if (!isnan(livello)) {
      Serial.printf("  Livello:   %.1f%%\n", livello);
    } else {
      Serial.println("  Livello:   calibrazione invertita");
    }
    Serial.println("  Calibrato: SI");
  } else {
    Serial.println("  Livello:   non calibrato");
    Serial.println("  Calibrato: NO");
  }
  Serial.printf("  Distanza:  %.1f mm (EMA)\n", sensor.getDistance());
  Serial.printf("  Raw:       %.0f mm\n", sensor.getRawDistance());
  Serial.printf("  Stale:     %s\n", sensor.isStale() ? "SI" : "NO");
  Serial.printf("  Letture:   %lu\n", sensor.getReadingCount());
  
  // --- Sistema ---
  Serial.println();
  Serial.println("--- Sistema ---");
  Serial.printf("  Heap:    %d KB\n", ESP.getFreeHeap() / 1024);
  unsigned long up = millis() / 1000;
  Serial.printf("  Uptime:  %luh %lum %lus\n", up / 3600, (up % 3600) / 60, up % 60);
  if (shelly.isOnline()) {
    Serial.printf("  RTOS cmd stack: %u bytes\n", shelly.getStackFree(0));
    Serial.printf("  RTOS mon stack: %u bytes\n", shelly.getStackFree(1));
  }
  
  Serial.println();
  Serial.println("====================");
  Serial.println();
}

void serial_cmd_run() {
  if (!Serial.available()) return;
  
  char cmd = Serial.read();
  
  switch (cmd) {
    case 'h': serial_cmd_help(); break;
    case 'm': serial_cmd_measure();break;
    case 's': serial_cmd_status(); break;
    case 'r': Serial.println("Reboot..."); Serial.flush(); safe_restart(); break;
    case '\n': case '\r': break;
    default:
      Serial.printf("'%c' non valido. h=help\n", cmd);
      break;
  }
}
