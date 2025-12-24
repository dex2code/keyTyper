void hashClickDo() {
  Keyboard.press(KEY_LEFT_CTRL);
  delay(10);
  Keyboard.press(KEY_LEFT_GUI);
  delay(10);
  Keyboard.press('q');
  delay(10);
  Keyboard.releaseAll();
  delay(10);
}