# keyTyper

A pocket-sized USB HID gadget built on an ATmega32U4 board (SparkFun Pro Micro
class). It keeps a two-digit key on an OLED screen, lets you adjust and store the
key with four buttons, and types the full password into any computer as a hardware
keyboard. A bonus mouse-jitter mode nudges the cursor around so the host never
falls asleep.

The password consists of three parts: a static head, the two-digit key, and a
static tail. Out of the box the head and tail are empty, so HASH types just the
two digits — see [Password layout](#password-layout).

## Hardware

- **Board:** any ATmega32U4 board with native USB — Pro Micro, Leonardo, Micro.
  The sketch uses `Keyboard.h` / `Mouse.h`, which exist only on 32U4-class
  boards; it does **not** compile on Uno / Mega / Nano.
- **Display:** SSD1306 OLED, 128x64, I2C bus, address `0x3C`.
- **Buttons:** four momentary buttons, active-low (pressed = pin to GND), each
  with an external pull-up resistor. The fifth button of the original design
  (K5 "BLUE") was removed; pin 14 (MISO) stays reserved.

## Wiring

| Arduino pin | Pro Micro label | Connected to |
|-------------|-----------------|--------------|
| 18  | A0 | UP button    |
| 19  | A1 | DOWN button  |
| 20  | A2 | HASH button  |
| 21  | A3 | STAR button  |
| SDA | D2 | SSD1306 SDA  |
| SCL | D3 | SSD1306 SCL  |

## Controls

| Button | Action |
|--------|--------|
| UP   | Increment the key; the ones digit carries into the tens digit, both wrap 0↔9 |
| DOWN | Decrement the key |
| HASH | If the key was edited since the last save — save it to EEPROM (the screen shows "OK", nothing is typed). Otherwise type the full password over USB HID |
| STAR | Toggle the mouse jitter: the cursor draws a 6 px square, one step every 200 ms |

## Password layout

HASH types three parts back to back:

```
PKEY_HEAD + <tens><ones> + PKEY_TAIL
```

The head and tail are compile-time settings in `keyTyper.ino`:

```cpp
#define PKEY_HEAD ""
#define PKEY_TAIL ""
```

Fill them in before flashing if the password is longer than the two digits; keep
them empty to type only the key. The screen flickers as each part goes out.

## Behaviour notes

- The screen turns off after 30 seconds without button activity.
- While the screen is asleep, the first press of UP / DOWN / HASH only wakes the
  display; press again to act.
- STAR works while the screen is asleep and does not wake it — handy for turning
  the jitter off "blind".
- If the OLED is missing or fails at startup, the device keeps working without
  it: saving and typing are unaffected and all screen operations are skipped.
  A physically jammed I2C bus is handled too — each Wire transfer is bounded by
  a 5 ms timeout instead of hanging forever.
- Saving and typing briefly block the main loop (~2 s for the "OK" screen,
  ~0.4 s for typing): presses that fall entirely inside those windows are
  ignored, and the jitter pauses. If a press did not seem to register, press
  again.
- The key is stored to EEPROM only when HASH is pressed while the key differs
  from the saved value; typing then requires a second HASH press.
- The EEPROM record is versioned with a magic byte: on the first boot after
  upgrading from an older firmware the key resets to `00` once.
- The key value is never written to the serial log; it is only visible on
  screen.
- HASH types characters as a US QWERTY keyboard. On hosts with other layouts
  (e.g. AZERTY, where top-row digits require Shift) the typed result will be
  wrong — make sure the target hosts use a US-like layout.

## Dependencies & build

| Library | Tested version |
|---------|----------------|
| Keyboard | 1.0.6 |
| Mouse | 1.0.1 |
| Adafruit SSD1306 | 2.5.16 (pulls in Adafruit GFX 1.12.6 and Adafruit BusIO 1.17.4) |

Build and flash with arduino-cli:

```sh
arduino-cli compile --fqbn arduino:avr:leonardo --warnings all .
arduino-cli upload --fqbn arduino:avr:leonardo -p <PORT> .
```

For a SparkFun Pro Micro, install the SparkFun core (`arduino-cli core install
SparkFun:avr`) and use `--fqbn SparkFun:avr:promicro` instead.

Continuous integration compiles the sketch on every push and pull request with
the same library versions pinned — see `.github/workflows/compile.yml`.

## Serial console

The debug log runs at **9600 baud** and reports boot progress, every button
press (including wake-up presses that only wake the screen), saves and typing
sessions. The key value itself is never printed.

## Project layout

| File | Purpose |
|------|---------|
| `keyTyper.ino` | Setup, main loop, button table and debounce, OLED sleep/wake, mouse jitter, configuration defines |
| `hashClick.ino` | HASH handler: save to EEPROM or type the password |
| `upClick.ino` | Key increment with wrap-around |
| `downClick.ino` | Key decrement with wrap-around |
| `oled.ino` | Drawing the key on the OLED |
| `.github/workflows/compile.yml` | CI build |

## License

MIT — see [LICENSE](LICENSE).
