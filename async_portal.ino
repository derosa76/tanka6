// async_portal.ino - Gestione WiFi non bloccante per TANKA
// Con auto-detection e risoluzione collision subnet
// Usa config JSON (LittleFS) invece di Preferences
// Versione 2.1 — Aggiunto cleanup RTOS prima di ESP.restart()

// === CONFIGURAZIONE ===
String TANKA_AP_SSID;  // Generato da MAC
const char* TANKA_AP_PASS = "tanka1234";

// Subnet AP (dinamica, auto-gestita)
IPAddress TANKA_AP_IP;
IPAddress TANKA_AP_GATEWAY;
IPAddress TANKA_AP_SUBNET(255, 255, 255, 0);

// === OGGETTI GLOBALI ===
AsyncWebServer asyncServer(80);
AsyncCaptivePortal portal;

// === VARIABILI STATO ===
bool forcePortal = false;
unsigned long lastPortalCheck = 0;
unsigned long wifiConnectStart = 0;
bool wifiConnecting = false;
unsigned long lastCollisionCheck = 0;
bool staWasConnected = false;

// ============================================
// HELPER: REBOOT PULITO (con cleanup RTOS tasks)
// ============================================

void safe_restart() {
  Serial.println("[SYSTEM] Cleanup RTOS tasks prima del reboot...");
  config.commit();
  #ifdef TANKA_SENSOR_REMOTE
  espnowConfig.commit();
  #endif
  shelly.stop();
  mdns.stop();
  delay(1000);
  ESP.restart();
}

// ============================================
// GESTIONE SUBNET AP - AUTO-DETECTION COLLISION
// ============================================

uint8_t get_ap_third_octet() {
  if (config["ap_subnet_third"].isNull()) {
    config["ap_subnet_third"] = 42;
    return 42;
  }
  return config["ap_subnet_third"].as<uint8_t>();
}

void save_ap_third_octet(uint8_t value) {
  config["ap_subnet_third"] = value;
  Serial.printf("[SUBNET] Salvato in config: 192.168.%d.x\n", value);
}

uint8_t generate_non_colliding_subnet(uint8_t sta_third_octet) {
  uint8_t candidates[] = {42, 123, 88, 99, 77, 200, 150, 100, 55, 66};
  const int num_candidates = sizeof(candidates) / sizeof(candidates[0]);
  
  for (int i = 0; i < num_candidates; i++) {
    if (candidates[i] != sta_third_octet) {
      return candidates[i];
    }
  }
  
  uint8_t random_subnet;
  do {
    random_subnet = random(10, 251);
  } while (random_subnet == sta_third_octet);
  
  return random_subnet;
}

bool check_and_resolve_subnet_collision() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  
  IPAddress sta_ip = WiFi.localIP();
  uint8_t sta_third = sta_ip[2];
  uint8_t ap_third = get_ap_third_octet();
  
  Serial.printf("[SUBNET] STA: 192.168.%d.x (IP: %s)\n", sta_third, sta_ip.toString().c_str());
  Serial.printf("[SUBNET] AP:  192.168.%d.x (config)\n", ap_third);
  
  if (sta_third == ap_third) {
    Serial.println("[SUBNET] !!! COLLISION RILEVATA !!!");
    uint8_t new_ap_third = generate_non_colliding_subnet(sta_third);
    Serial.printf("[SUBNET] Nuova subnet AP: 192.168.%d.x\n", new_ap_third);
    save_ap_third_octet(new_ap_third);
    return true;
  }
  
  Serial.println("[SUBNET] OK, nessuna collision");
  return false;
}

void configure_ap_ip() {
  uint8_t third_octet = get_ap_third_octet();
  TANKA_AP_IP = IPAddress(192, 168, third_octet, 1);
  TANKA_AP_GATEWAY = IPAddress(192, 168, third_octet, 1);
  Serial.printf("[SUBNET] AP configurato: 192.168.%d.1\n", third_octet);
}

// ============================================
// HELPER FUNZIONI
// ============================================

String genera_AP_name() {
  uint64_t chipid = ESP.getEfuseMac();
  uint16_t chip = (uint16_t)(chipid >> 32);
  char nome[16];
  sprintf(nome, "TANKA_%04X", chip);
  return String(nome);
}

void onWiFiCredentialsSaved(String ssid, String password) {
  Serial.println("\n=== NUOVE CREDENZIALI SALVATE ===");
  Serial.println("SSID: " + ssid);
  Serial.println("================================\n");
  
  WiFi.disconnect();
  delay(100);
  WiFi.begin(ssid.c_str(), password.c_str());
  
  wifiConnectStart = millis();
  wifiConnecting = true;
  forcePortal = false;
}

// ============================================
// SETUP - SEQUENZA CORRETTA
// ============================================

void async_portal_setup() {
  TANKA_AP_SSID = genera_AP_name();
  Serial.println("\n=== AsyncCaptivePortal Setup ===");

  // FASE 1: Configura IP AP
  configure_ap_ip();
  
  // FASE 2: Imposta modalità AP+STA PRIMA di tutto
  WiFi.mode(WIFI_AP_STA);
  delay(200);
  
  // FASE 3: Configura e avvia AP
  WiFi.softAPConfig(TANKA_AP_IP, TANKA_AP_GATEWAY, TANKA_AP_SUBNET);
  WiFi.softAP(TANKA_AP_SSID.c_str(), TANKA_AP_PASS);
  delay(500);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  
  Serial.println("\n=== Access Point TANKA ===");
  Serial.println("SSID:     " + TANKA_AP_SSID);
  Serial.println("Password: " + String(TANKA_AP_PASS));
  Serial.println("IP AP:    " + WiFi.softAPIP().toString());
  Serial.println("==========================\n");
  
  // FASE 4: Inizializza portal PRIMA di leggere credenziali
  portal.begin(&asyncServer);
  portal.onCredentialsSaved(onWiFiCredentialsSaved);
  
  // FASE 5: Connetti STA (se credenziali salvate)
  bool sta_connected = false;
  if (portal.hasCredentials()) {
    String ssid = portal.getSSID();
    String pass = portal.getPassword();
    
    Serial.println("Credenziali trovate, connessione STA...");
    Serial.println("  SSID: " + ssid);
    
    WiFi.begin(ssid.c_str(), pass.c_str());
    wifiConnectStart = millis();
    
    int timeout = 15000;
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
      delay(100);
      timeout -= 100;
      if (timeout % 2000 == 0) {
        Serial.print(".");
      }
    }
    Serial.println();
    
    sta_connected = (WiFi.status() == WL_CONNECTED);
    
    if (sta_connected) {
      Serial.println("STA connesso!");
      Serial.println("  IP STA: " + WiFi.localIP().toString());
      Serial.println("  RSSI:   " + String(WiFi.RSSI()) + " dBm");
      wifiConnecting = false;
      staWasConnected = true;
      
      if (check_and_resolve_subnet_collision()) {
        Serial.println("[SUBNET] Collision! Riavvio...");
        safe_restart();
      }
    } else {
      Serial.println("STA timeout, continuo senza");
      Serial.printf("  WiFi.status() = %d\n", WiFi.status());
      wifiConnecting = true;
    }
  } else {
    Serial.println("Nessuna credenziale salvata");
    forcePortal = true;
  }
  
  WiFi.setAutoReconnect(true);
  
  asyncServer.begin();
  Serial.println("AsyncWebServer avviato");
  
  if (!portal.hasCredentials()) {
    forcePortal = true;
    Serial.println("Portal attivo per configurazione WiFi");
  }
  
  Serial.println("=================================\n");
}

// ============================================
// RUN (chiamare nel loop)
// ============================================

void async_portal_run() {
  bool staIsConnected = (WiFi.status() == WL_CONNECTED);
  
  // === CHECK SU RICONNESSIONE STA ===
  if (staIsConnected && !staWasConnected) {
    Serial.println("[WIFI] STA connesso!");
    Serial.println("  IP: " + WiFi.localIP().toString());
    Serial.println("  RSSI: " + String(WiFi.RSSI()) + " dBm");
    
    if (check_and_resolve_subnet_collision()) {
      Serial.println("[SUBNET] Collision rilevata! Riavvio...");
      safe_restart();  // Cleanup RTOS tasks prima del reboot
    }
    wifiConnecting = false;
  }
  staWasConnected = staIsConnected;
  
  // === CHECK PERIODICO COLLISION (ogni 60 secondi) ===
  if (millis() - lastCollisionCheck > 60000) {
    lastCollisionCheck = millis();
    if (staIsConnected && check_and_resolve_subnet_collision()) {
      Serial.println("[SUBNET] Collision! Riavvio...");
      safe_restart();  // Cleanup RTOS tasks prima del reboot
    }
  }
  
  // === CHECK PERIODICO PORTAL (ogni 2 secondi) ===
  if (millis() - lastPortalCheck < 2000) return;
  lastPortalCheck = millis();
  
  // === LOGICA ATTIVAZIONE PORTAL ===
  bool shouldEnablePortal = false;
  
  if (forcePortal) {
    shouldEnablePortal = true;
  }
  else if (!staIsConnected && portal.hasCredentials()) {
    if (wifiConnecting) {
      if (millis() - wifiConnectStart > 15000) {
        Serial.println("Timeout connessione WiFi -> Attivo portal");
        shouldEnablePortal = true;
        wifiConnecting = false;
      }
    } else {
      String ssid = portal.getSSID();
      String pass = portal.getPassword();
      WiFi.begin(ssid.c_str(), pass.c_str());
      wifiConnectStart = millis();
      wifiConnecting = true;
    }
  }
  else if (!portal.hasCredentials()) {
    shouldEnablePortal = true;
  }
  
  portal.handle(shouldEnablePortal);
  
  // === STATUS REPORT (ogni 20 secondi) ===
  static int checkCounter = 0;
  if (++checkCounter % 10 == 0) {
    if (portal.isActive()) {
      Serial.println("Portal attivo: http://" + WiFi.softAPIP().toString());
    }
  }
}

// ============================================
// UTILITY PUBBLICHE
// ============================================

void async_portal_force_open() {
  forcePortal = true;
  Serial.println("Portal forzato manualmente");
}

bool async_portal_is_connected() {
  return WiFi.status() == WL_CONNECTED;
}

String async_portal_get_sta_ip() {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }
  return "Non connesso";
}

String async_portal_get_ap_ip() {
  return WiFi.softAPIP().toString();
}

uint8_t async_portal_get_ap_subnet() {
  return get_ap_third_octet();
}

void async_portal_force_subnet_change(uint8_t new_third_octet) {
  save_ap_third_octet(new_third_octet);
  Serial.printf("[DEBUG] Subnet AP forzata a 192.168.%d.x\n", new_third_octet);
  Serial.println("[DEBUG] Riavvio...");
  safe_restart();  // Cleanup RTOS tasks prima del reboot
}

#include <esp_wifi.h>
