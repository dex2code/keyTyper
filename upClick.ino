void upClickDo() {
  pkey.ones++;
  if (pkey.ones > 9) {
    pkey.ones = 0;
    pkey.tens++;
    if (pkey.tens > 9) {
      pkey.tens = 0;
    }
  }

  pkeyChanges++;
}
