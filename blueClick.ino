void blueClickDo() {
  Keyboard.press(KEY_LEFT_CTRL);
  Keyboard.press(KEY_LEFT_GUI);
  Keyboard.press('q');
  delay(100);
  Keyboard.releaseAll();
}