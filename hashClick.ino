void hashClickDo() {
  if (pkeyDirty) {
    Serial.println(F("A new PKey value has been provided. Trying to save ..."));

    while (!eeprom_is_ready()) {
      Serial.print(F("."));
      delay(100);
    }
    EEPROM.put(EEPROM_START_ADDR, pkey);

    if (isOledPresent) {
      oled.clearDisplay();
      oled.setCursor(OLED_CURSOR_X, OLED_CURSOR_Y);
      oled.print(F("OK"));
      oled.display();
      delay(2000);
      // no redraw here: hashClickAction() brings the key value back on screen
    }

    Serial.println(F(" Done!"));

    pkeyDirty = false;
  } else {
    // Type the full password in four parts: static head (PKEY_HEAD), the two
    // digits, static tail (PKEY_TAIL). Flicker the screen while each part goes
    // out; empty head/tail (the defaults) send nothing but still flicker.
    for (uint8_t part = 0; part < 4; part++) {
      if (isOledPresent) {
        oled.invertDisplay(true);
        delay(OLED_FLICKER_DELAY_MS);
      }

      switch (part) {
        case 0:
          Keyboard.print(PKEY_HEAD);
          break;

        case 1:
          Keyboard.print(pkey.tens);
          break;

        case 2:
          Keyboard.print(pkey.ones);
          break;

        case 3:
          Keyboard.print(PKEY_TAIL);
          break;
      }

      if (isOledPresent) {
        oled.invertDisplay(false);
        delay(OLED_FLICKER_DELAY_MS);
      }
    }
  }
}
