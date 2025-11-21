void updateOLED() {
  oled.clearDisplay();
  oled.setCursor(36, 16);
  oled.print((String) "" + pkey.tens + "" + pkey.ones + "");
  oled.display();
}
