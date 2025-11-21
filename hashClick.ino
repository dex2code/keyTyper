void hashClickDo() {
  if (pkeyChanges != 0) {
    Serial.print((String) F("A new PKey value \"") + pkey.tens + pkey.ones + F("\" has been provided. Trying to save ..."));

    while (!eeprom_is_ready()) {
      Serial.print(F("."));
      delay(100);
    }
    EEPROM.put(EEPROM_START_ADDR, pkey);

    oled.clearDisplay();
    oled.setCursor(36, 16);
    oled.print(F("OK"));
    oled.display();
    delay(2000);

    Serial.println(F(" Done!"));

    pkeyChanges = 0;
  } else {
    oled.invertDisplay(true);
    delay(KEYBOARD_PUT_DELAY_MS);

    Keyboard.print(PKEY_HEAD);

    oled.invertDisplay(false);
    delay(KEYBOARD_PUT_DELAY_MS);

    oled.invertDisplay(true);
    delay(KEYBOARD_PUT_DELAY_MS);

    Keyboard.print(pkey.tens);

    oled.invertDisplay(false);
    delay(KEYBOARD_PUT_DELAY_MS);

    oled.invertDisplay(true);
    delay(KEYBOARD_PUT_DELAY_MS);

    Keyboard.print(pkey.ones);

    oled.invertDisplay(false);
    delay(KEYBOARD_PUT_DELAY_MS);

    oled.invertDisplay(true);
    delay(KEYBOARD_PUT_DELAY_MS);

    Keyboard.print(PKEY_TAIL);

    oled.invertDisplay(false);
    delay(KEYBOARD_PUT_DELAY_MS);
  }
}
