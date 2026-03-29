
void display_setup() {
  tft.init();
  int rot = config["display_rotation"] | 1;
  if (rot != 1 && rot != 3) rot = 1;  // sanitize: solo 1 o 3
  tft.setRotation(rot);
}

void toggleRotation() {
  int rot = config["display_rotation"] | 1;
  rot = (rot == 1) ? 3 : 1;
  config["display_rotation"] = rot;
  config.commit();           // critico, scrivi subito
  tft.setRotation(rot);
  forceScreenRedraw();       // definita in screens.ino
  Serial.printf("[DISPLAY] Rotazione → %d\n", rot);
}
