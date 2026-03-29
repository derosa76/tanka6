// void config_setup(){
//   if(readJson(config_filename, config)){
//     Serial.println("Config caricato da file");
    
//     // Migrazione: aggiungi nuovi parametri se mancanti
//     bool needs_save = false;
    
//     // Parametro subnet AP (aggiunto per auto-detection collision)
//     if (config["ap_subnet_third"].isNull()) {
//       config["ap_subnet_third"] = 42;
//       needs_save = true;
//       Serial.println("  + Aggiunto ap_subnet_third = 42");
//     }
    
//     // === PARAMETRI SAFETY POMPA ===
//     if (config["pump_keepalive_interval"].isNull()) {
//       config["pump_keepalive_interval"] = 60;
//       needs_save = true;
//       Serial.println("  + Aggiunto pump_keepalive_interval = 60s");
//     }
//     if (config["pump_auto_off_timeout"].isNull()) {
//       config["pump_auto_off_timeout"] = 90;
//       needs_save = true;
//       Serial.println("  + Aggiunto pump_auto_off_timeout = 90s");
//     }
    
//     // === PARAMETRI POMPA (nuova logica H/L) ===
//     if (config["pump_mode"].isNull()) {
//       config["pump_mode"] = 0;  // 0=off, 1=riempimento, 2=svuotamento
//       needs_save = true;
//       Serial.println("  + Aggiunto pump_mode = 0 (disabilitato)");
//     }
//     if (config["pump_H"].isNull()) {
//       config["pump_H"] = 80.0;
//       needs_save = true;
//       Serial.println("  + Aggiunto pump_H = 80%");
//     }
//     if (config["pump_L"].isNull()) {
//       config["pump_L"] = 20.0;
//       needs_save = true;
//       Serial.println("  + Aggiunto pump_L = 20%");
//     }
    
//     if (needs_save) {
//       writeJson(config_filename, config);
//       Serial.println("Config aggiornato con nuovi parametri");
//     }
    
//     return;
//   }
//   else{
//     Serial.println("Config non trovato, creo default...");
    
//     // === SUBNET AP ===
//     config["ap_subnet_third"] = 42;
    
//     // === PARAMETRI SAFETY POMPA ===
//     config["pump_keepalive_interval"] = 60;
//     config["pump_auto_off_timeout"] = 90;
    
//     // === PARAMETRI POMPA ===
//     config["pump_mode"] = 0;    // 0=off, 1=riempimento, 2=svuotamento
//     config["pump_H"] = 80.0;   // soglia alta %
//     config["pump_L"] = 20.0;   // soglia bassa %
    
//     // === PARAMETRI SENSORE ===
//     config["dist_min"] = 700.0;
//     config["dist_max"] = 4000.0;
//     config["calib_min_level"] = nullptr;
//     config["calib_max_level"] = nullptr;
    
//     writeJson(config_filename, config);
//     Serial.println("Config default creato");
//     return;
//   }
// }
