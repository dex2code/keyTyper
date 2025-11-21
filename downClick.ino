void downClickDo() {
  pkey.ones--;
  if (pkey.ones < 0) {
    pkey.ones = 9;
    pkey.tens--;
    if (pkey.tens < 0) {
      pkey.tens = 9;
    }
  }

  pkeyChanges--;
}
