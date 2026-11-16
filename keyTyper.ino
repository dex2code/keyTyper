#include <EEPROM.h>
#include <Adafruit_SSD1306.h>
#include <Keyboard.h>
#include <Mouse.h>


#define SERIAL_SPEED 9600

#define EEPROM_START_ADDR 0
#define EEPROM_MAGIC 0x5A

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define OLED_SLEEP_AFTER_MS 30000
#define OLED_INIT_ATTEMPTS 10
#define OLED_CURSOR_X 36
#define OLED_CURSOR_Y 16

#define K1_UP_PIN 18
#define K2_DOWN_PIN 19
#define K3_HASH_PIN 20
#define K4_STAR_PIN 21

#define OLED_FLICKER_DELAY_MS 50

#define MOUSE_MOVE_PX 6

// Static head and tail of the password, typed around the two-digit key:
// HASH sends PKEY_HEAD + <tens><ones> + PKEY_TAIL. Fill them in before
// compiling; keep empty if the password is just the two digits.
#define PKEY_HEAD "my"
#define PKEY_TAIL "pass"

#define DEBOUNCE_MS 35

#define TIMER_DELAY_MS 200


Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


struct PKey {
  uint8_t magic = EEPROM_MAGIC;  // stored on every save; tells real data from fresh/corrupted EEPROM
  int8_t tens = 0;
  int8_t ones = 0;
};
PKey pkey;

// True when the key differs from the value stored in EEPROM.
bool pkeyDirty = false;

uint32_t tmrLast = 0;

bool isMouseOn = false;
uint8_t mouseStep = 0;

bool isOledOn = true;
// OLED detected at startup. When false (init failed), the device keeps working
// without the display: button actions still run, all OLED calls are skipped.
bool isOledPresent = true;

uint32_t lastActionAt = 0;


// ---- Buttons ----

void upClickAction();
void downClickAction();
void hashClickAction();
void starClickAction();

// Button names live in PROGMEM: F() expands to a statement expression,
// which is not allowed in static initializers.
const char btnNameUp[] PROGMEM = "UP";
const char btnNameDown[] PROGMEM = "DOWN";
const char btnNameHash[] PROGMEM = "HASH";
const char btnNameStar[] PROGMEM = "STAR";

struct Button {
  uint8_t pin;
  const char *name;  // PROGMEM string, printed via a cast to __FlashStringHelper
  // When true, the action is skipped if the screen is asleep: the press only
  // wakes the display. STAR is exempt and works silently from sleep.
  bool needsScreen;
  bool pressedFlag;
  uint32_t lastChangeAt;  // per-button debounce timestamp
  void (*action)();
};

Button buttons[] = {
  { K1_UP_PIN,   btnNameUp,   true,  false, 0, upClickAction   },
  { K2_DOWN_PIN, btnNameDown, true,  false, 0, downClickAction },
  { K3_HASH_PIN, btnNameHash, true,  false, 0, hashClickAction },
  { K4_STAR_PIN, btnNameStar, false, false, 0, starClickAction },
};
const uint8_t BUTTONS_COUNT = sizeof(buttons) / sizeof(buttons[0]);


void setup() {
  // BUILTIN LED PREPARING
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // BUTTONS PREPARING
  // K1..K4: active-low buttons with EXTERNAL pull-up resistors on the board (plain INPUT).
  pinMode(K1_UP_PIN, INPUT);
  pinMode(K2_DOWN_PIN, INPUT);
  pinMode(K3_HASH_PIN, INPUT);
  pinMode(K4_STAR_PIN, INPUT);
  delay(1000);

  Keyboard.begin();
  delay(1000);
  Mouse.begin();
  delay(1000);

  Serial.begin(SERIAL_SPEED);
  delay(1000);

  Serial.println();
  Serial.println(F(":: KeyPass Tool ::"));
  Serial.println(F("Compiled at: " __TIMESTAMP__));

  Serial.print(F("Trying to get EEPROM data ..."));
  while (!eeprom_is_ready()) {
    Serial.print(F("."));
    delay(100);
  }
  EEPROM.get(EEPROM_START_ADDR, pkey);
  if (pkey.magic != EEPROM_MAGIC) {
    // Fresh EEPROM or data from an older firmware layout: start from defaults.
    pkey.magic = EEPROM_MAGIC;
    pkey.tens = 0;
    pkey.ones = 0;
    Serial.println(F(" nothing saved yet, starting from 00."));
  } else {
    Serial.println(F(" saved key loaded."));  // the key value itself is not logged on purpose
  }
  // Sanity clamps in case of EEPROM corruption.
  if (pkey.tens > 9 || pkey.tens < 0) {
    pkey.tens = 0;
  }
  if (pkey.ones > 9 || pkey.ones < 0) {
    pkey.ones = 0;
  }

  // OLED DISPLAY PREPARING
  // Bound each blocking I2C phase: a physically jammed bus (shorted SDA/SCL,
  // a slave holding SDA low) would otherwise hang the Wire library forever.
  // An absent display still fails fast via NAK; this covers the stuck-bus case.
  Wire.setWireTimeout(5000, true);  // 5 ms per phase, reset TWI on timeout
  Serial.print(F("Starting OLED ..."));
  bool oledReady = false;
  for (uint8_t attempt = 0; attempt < OLED_INIT_ATTEMPTS && !oledReady; attempt++) {
    oledReady = oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
    if (!oledReady) {
      delay(100);
      Serial.print(F("."));
    }
  }
  if (oledReady) {
    Serial.println(F(" Ok!"));
    oled.clearDisplay();
    oled.setTextColor(WHITE);
    oled.setTextSize(5);
    updateOLED();
  } else {
    // No display on the bus: keep going without it. isOledOn stays false so
    // none of the sleep/wake or draw code ever touches the OLED.
    isOledPresent = false;
    isOledOn = false;
    Serial.println(F(" Failed! Continuing without OLED."));
  }

  lastActionAt = millis();

  Serial.print(F("Entering main loop at ["));
  Serial.print(lastActionAt);
  Serial.println(F("]..."));
}


void loop() {

  delay(1);

  if (isOledPresent && isOledOn && millis() - lastActionAt >= OLED_SLEEP_AFTER_MS) {
    Serial.print(F("### Activity timeout at ["));
    Serial.print(millis());
    Serial.println(F("]. Turning OLED Off."));
    oled.ssd1306_command(SSD1306_DISPLAYOFF);
    isOledOn = false;
  }
  if (isOledPresent && !isOledOn && millis() - lastActionAt < OLED_SLEEP_AFTER_MS) {
    Serial.print(F("### Activity detected at ["));
    Serial.print(millis());
    Serial.println(F("]. Turning OLED On."));
    oled.ssd1306_command(SSD1306_DISPLAYON);
    isOledOn = true;
  }

  // Button scanning: one code path for edge detection and debounce on every button.
  for (uint8_t i = 0; i < BUTTONS_COUNT; i++) {
    Button &btn = buttons[i];
    bool pressed = !digitalRead(btn.pin);  // active-low

    if (btn.pressedFlag && !pressed && millis() - btn.lastChangeAt > DEBOUNCE_MS) {
      btn.pressedFlag = false;
      btn.lastChangeAt = millis();
      if (btn.needsScreen || isOledOn) {
        lastActionAt = millis();
      }
    } else if (!btn.pressedFlag && pressed && millis() - btn.lastChangeAt > DEBOUNCE_MS) {
      btn.pressedFlag = true;
      btn.lastChangeAt = millis();
      if (btn.needsScreen || isOledOn) {
        lastActionAt = millis();
      }

      bool wakeUpPress = btn.needsScreen && isOledPresent && !isOledOn;

      Serial.print(F("--> "));
      Serial.print(reinterpret_cast<const __FlashStringHelper *>(btn.name));
      Serial.print(F(" key pressed at ["));
      Serial.print(millis());
      if (wakeUpPress) {
        // First press after the screen went to sleep only wakes the display.
        Serial.println(F("] (screen asleep, wake-up only)"));
      } else {
        Serial.println(F("]"));
        btn.action();
      }
    }
  }

  if (millis() - tmrLast >= TIMER_DELAY_MS) {
    tmrLast = millis();

    if (isMouseOn) {
      switch (mouseStep) {
        case 0:  // Move right
          Mouse.move(MOUSE_MOVE_PX, 0);
          break;

        case 1:  // Move down
          Mouse.move(0, MOUSE_MOVE_PX);
          break;

        case 2:  // Move left
          Mouse.move(-MOUSE_MOVE_PX, 0);
          break;

        case 3:  // Move up
          Mouse.move(0, -MOUSE_MOVE_PX);
          break;
      }
      mouseStep = (mouseStep + 1) % 4;
    }
  }

}


// ---- Button actions (referenced by the buttons[] table above) ----

void upClickAction() {
  upClickDo();
  updateOLED();
}

void downClickAction() {
  downClickDo();
  updateOLED();
}

void hashClickAction() {
  hashClickDo();
  updateOLED();
}

void starClickAction() {
  isMouseOn = !isMouseOn;
  Serial.print(F("    mouse jitter "));
  Serial.println(isMouseOn ? F("ON") : F("OFF"));
}
