void updateOLED() {
  if (!isOledPresent) {
    return;
  }
  oled.clearDisplay();
  oled.setCursor(OLED_CURSOR_X, OLED_CURSOR_Y);
  oled.print(pkey.tens);
  oled.print(pkey.ones);
  oled.display();
}
