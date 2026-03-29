void littleFS_setup(){
  // PRIMA prova senza formattare
  if (!LittleFS.begin(false)) {  // false = non formattare
    Serial.println("LittleFS non montato");
    
    // Controlla se ci sono file da salvare
    // (in questo caso non possiamo perché non si monta)
    Serial.println("Formatting LittleFS...");
    
    // SOLO ORA formatta
    if (!LittleFS.begin(true)) {  // true = formatta se necessario
      Serial.println("Errore anche dopo format!");
      return;
    }
    Serial.println("LittleFS formattato e montato");
  } else {
    Serial.println("LittleFS montato correttamente");
  }
  
  // Info spazio
  size_t totalBytes = LittleFS.totalBytes();
  size_t usedBytes = LittleFS.usedBytes();
  Serial.printf("Spazio totale: %d bytes\n", totalBytes);
  Serial.printf("Spazio usato: %d bytes\n", usedBytes);
  Serial.printf("Spazio libero: %d bytes\n", totalBytes - usedBytes);





}


// bool writeJson(String filename, JsonDocument& doc) {
//   fs::File file = LittleFS.open(filename, "w");
//   if (!file) {
//     Serial.println("ERRORE: Impossibile aprire " + filename + " per scrittura");
//     return false;
//   }
  
//   size_t bytesWritten = serializeJson(doc, file);
//   file.close();
  
//   if (bytesWritten == 0) {
//     Serial.println("ERRORE: Serializzazione JSON fallita");
//     return false;
//   }
  
//   Serial.printf("File %s salvato (%d bytes)\n", filename.c_str(), bytesWritten);
//   return true;
// }

// bool readJson(String filename, JsonDocument& doc) {
//   if (!LittleFS.exists(filename)) {
//     Serial.println("File " + filename + " non trovato");
//     return false;
//   }
  
//   fs::File file = LittleFS.open(filename, "r");
//   if (!file) {
//     Serial.println("ERRORE: Impossibile aprire " + filename + " per lettura");
//     return false;
//   }
  
//   DeserializationError error = deserializeJson(doc, file);
//   file.close();
  
//   if (error) {
//     Serial.print("ERRORE parsing JSON: ");
//     Serial.println(error.c_str());
//     return false;
//   }
  
//   //Serial.printf("File %s letto\n", filename.c_str());
//   return true;
// }

// bool syncJson(String filename, JsonDocument& doc) {
//   // Carica il JSON dal file
//   StaticJsonDocument<1000> fileDoc; 
  
//   if (!readJson(filename, fileDoc)) {
//     // File non esiste o errore → salva direttamente
//     Serial.println("File non esiste o corrotto, creo nuovo file");
//     return writeJson(filename, doc);
//   }
  
//   // Confronta serializzando entrambi
//   String memoryJson;
//   String fileJson;
  
//   serializeJson(doc, memoryJson);
//   serializeJson(fileDoc, fileJson);
  
//   if (memoryJson == fileJson) {
//     //Serial.println("File " + filename + " già aggiornato, nessuna modifica");
//     return true;  // Nessuna modifica necessaria
//   }
  
//   // Sono diversi → sovrascrivi
//   Serial.println("File " + filename + " diverso, aggiorno...");
//   return writeJson(filename, doc);
// }


// unsigned long last_sync_time=0;
// bool runSynksyncJson(String filename, JsonDocument& doc,int sync_interval){
//   if (millis()-last_sync_time>sync_interval){
//     last_sync_time = millis();
//     return syncJson(filename, doc);
//   }
//   return false;
// }