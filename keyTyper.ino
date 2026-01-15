#include <EEPROM.h>
#include <Adafruit_SSD1306.h>
#include <Keyboard.h>
#include <Mouse.h>


#define SERIAL_SPEED 9600

#define EEPROM_START_ADDR 0

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
#define OLED_SLEEP_AFTER_MS 30000

#define K1_UP_PIN 18
#define K2_DOWN_PIN 19
#define K3_HASH_PIN 20
#define K4_STAR_PIN 21
#define K5_BLUE_PIN 14

#define KEYBOARD_PUT_DELAY_MS 50

#define MOUSE_MOVE_PX 6

#define PKEY_HEAD ""
#define PKEY_TAIL ""

#define DEBOUNCE_MS 35

#define TIMER_DELAY 200


Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);


struct PKey {
  int8_t tens = 0;
  int8_t ones = 0;
};
PKey pkey;

int8_t pkeyChanges = 0;

uint32_t tmrLast = 0;

bool isMouseOn = false;
int8_t mouseStep = 0;

bool isOledOn = true;

bool k1_up_isPressedFlag = false;
bool k2_down_isPressedFlag = false;
bool k3_hash_isPressedFlag = false;
bool k4_star_isPressedFlag = false;
//bool k5_blue_isPressedFlag = false;

uint32_t lastActionAt = 0;


void setup() {
  // BUILTIN LED PREPARING
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  //BUTTONS PREPARING
  pinMode(K1_UP_PIN, INPUT);
  pinMode(K2_DOWN_PIN, INPUT);
  pinMode(K3_HASH_PIN, INPUT);
  pinMode(K4_STAR_PIN, INPUT);
  pinMode(K5_BLUE_PIN, INPUT_PULLUP);
  delay(1000);

  Keyboard.begin();
  delay(1000);
  Mouse.begin();
  delay(1000);

  Serial.begin(SERIAL_SPEED);
  delay(1000);

  Serial.println();
  Serial.println(F(":: KeyPass Tool ::"));
  Serial.println((String) F("Compiled at: ") + __TIMESTAMP__ + F(""));

  Serial.print(F("Trying to get EEPROM data ..."));
  while (!eeprom_is_ready()) {
    Serial.print(F("."));
    delay(100);
  }
  EEPROM.get(EEPROM_START_ADDR, pkey);
  if (pkey.tens > 9 || pkey.tens < 0) {
    pkey.tens = 0;
  }
  if (pkey.ones > 9 || pkey.ones < 0) {
    pkey.ones = 0;
  }
  Serial.println((String) F(" got \"") + pkey.tens + pkey.ones + F("\" value."));

  //OLED DISPLAY PREPARING
  Serial.print(F("Starting OLED ..."));
  while (!oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    delay(100);
    Serial.print(F("."));
  }
  Serial.println(F(" Ok!"));
  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.setTextSize(5);
  updateOLED();

  lastActionAt = millis();

  Serial.println((String) F("Entering main loop at [") + lastActionAt + F("]..."));
}


void loop() {

  delay(1);

  if (isOledOn && millis() - lastActionAt >= OLED_SLEEP_AFTER_MS) {
    Serial.println((String) F("### Activity timeout at [") + lastActionAt + F("]. Turning OLED Off."));
    oled.ssd1306_command(SSD1306_DISPLAYOFF);
    isOledOn = false;
  }
  if (!isOledOn && millis() - lastActionAt < OLED_SLEEP_AFTER_MS) {
    Serial.println((String) F("### Activity detected at [") + lastActionAt + F("]. Turning OLED On."));
    oled.ssd1306_command(SSD1306_DISPLAYON);
    isOledOn = true;
  }

  if (k1_up_isPressedFlag && !k1_up_isPressedState() && (lastActionAt - millis()) > DEBOUNCE_MS) {
    k1_up_isPressedFlag = false;
    lastActionAt = millis();
  }
  if (k2_down_isPressedFlag && !k2_down_isPressedState() && (lastActionAt - millis()) > DEBOUNCE_MS) {
    k2_down_isPressedFlag = false;
    lastActionAt = millis();
  }
  if (k3_hash_isPressedFlag && !k3_hash_isPressedState() && (lastActionAt - millis()) > DEBOUNCE_MS) {
    k3_hash_isPressedFlag = false;
    lastActionAt = millis();
  }
  if (k4_star_isPressedFlag && !k4_star_isPressedState() && (lastActionAt - millis()) > DEBOUNCE_MS) {
    k4_star_isPressedFlag = false;
    lastActionAt = millis();
  }

/*
  if (k5_blue_isPressedFlag && !k5_blue_isPressedState() && (lastActionAt - millis()) > DEBOUNCE_MS) {
    k5_blue_isPressedFlag = false;
    lastActionAt = millis();
  }
*/

  if (!k1_up_isPressedFlag && k1_up_isPressedState() && (lastActionAt - millis()) > DEBOUNCE_MS) {
    k1_up_isPressedFlag = true;
    lastActionAt = millis();
    Serial.println((String) F("--> UP key pressed at [") + lastActionAt + F("]"));
    if (isOledOn) {
      upClickDo();
      updateOLED();
    }
  }
  if (!k2_down_isPressedFlag && k2_down_isPressedState() && (lastActionAt - millis()) > DEBOUNCE_MS) {
    k2_down_isPressedFlag = true;
    lastActionAt = millis();
    Serial.println((String) F("--> DOWN key pressed at [") + lastActionAt + F("]"));
    if (isOledOn) {
      downClickDo();
      updateOLED();
    }
  }
  if (!k3_hash_isPressedFlag && k3_hash_isPressedState() && (lastActionAt - millis()) > DEBOUNCE_MS) {
    k3_hash_isPressedFlag = true;
    lastActionAt = millis();
    Serial.println((String) F("--> HASH key pressed at [") + lastActionAt + F("]"));
    if (isOledOn) {
      hashClickDo();
      updateOLED();
    }
  }
  if (!k4_star_isPressedFlag && k4_star_isPressedState() && (lastActionAt - millis()) > DEBOUNCE_MS) {
    k4_star_isPressedFlag = true;
    lastActionAt = millis();
    Serial.println((String) F("--> STAR key pressed at [") + lastActionAt + F("]"));
    isMouseOn = !isMouseOn;
  }

/*
  if (!k5_blue_isPressedFlag && k5_blue_isPressedState() && (lastActionAt - millis()) > DEBOUNCE_MS) {
    k5_blue_isPressedFlag = true;
    lastActionAt = millis();
    Serial.println((String) F("--> BLUE key pressed at [") + lastActionAt + F("]"));
    blueClickDo();
  }
*/

  if (millis() - tmrLast >= TIMER_DELAY) {
    tmrLast = millis();

    if (isMouseOn) {
      switch (mouseStep) {
        case 0:  // Move right
          Mouse.move(MOUSE_MOVE_PX, 0);
          mouseStep++;
          break;

        case 1:  // Move down
          Mouse.move(0, MOUSE_MOVE_PX);
          mouseStep++;
          break;

        case 2:  // Move Left
          Mouse.move(-MOUSE_MOVE_PX, 0);
          mouseStep++;
          break;

        case 3:  // Move up
          Mouse.move(0, -MOUSE_MOVE_PX);
          mouseStep++;
          break;

        default:
          mouseStep = 0;
          break;
      }
    }
  }

}
