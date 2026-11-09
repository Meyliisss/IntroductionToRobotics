#include <Arduino.h>

// Pins
const uint8_t dataPin  = 23;  // DS
const uint8_t clockPin = 18;  // SHCP
const uint8_t latchPin = 22;  // STCP

const uint8_t digitPins[4] = {25, 26, 27, 33};  // D1 (left) - D4 (right)

const uint8_t joyXPin   = 34;  // VRx
const uint8_t joyYPin   = 35;  // VRy
const uint8_t buttonPin = 32;  // SW

// Display type
const bool COMMON_ANODE = false;
const uint8_t DIGIT_ON  = COMMON_ANODE ? HIGH : LOW;
const uint8_t DIGIT_OFF = COMMON_ANODE ? LOW : HIGH;

// Bits
const uint8_t DP_BIT = 0b00000001;
const uint8_t digitEncoding[10] = {
  0b11111100, // 0
  0b01100000, // 1
  0b11011010, // 2
  0b11110010, // 3
  0b01100110, // 4
  0b10110110, // 5
  0b10111110, // 6
  0b11100000, // 7
  0b11111110, // 8
  0b11110110  // 9
};

// Game settings 
const uint16_t MAX_VALUE         = 9999;
const uint32_t COUNTDOWN_STEP_MS = 1000;   // countdown
const uint32_t GAME_TIME_MS      = 20000;  // time limit to stop on the target

// Joystick
const int JOY_CENTER   = 2048;
const int JOY_DEADZONE = 400;
const int JOY_MAX_MAG  = 2048 - JOY_DEADZONE;
const int JOY_HIGH     = 3500;  // "pushed right" threshold
const int JOY_LOW      = 600;   // "pushed left" threshold
const int JOY_REARM    = 2800;  // must come back below this before another flick

// Display (multiplexing)
const uint32_t REFRESH_US = 2000;  // each digit lit for 2 ms -> 125 Hz full frame

uint8_t displayBuf[4] = {0, 0, 0, 0};  // segment pattern of every digit
bool    dpOn = false;                  // decimal point on the last digit

void writeReg(uint8_t logicalPattern) {
  uint8_t pattern = COMMON_ANODE ? static_cast<uint8_t>(~logicalPattern) : logicalPattern;
  digitalWrite(latchPin, LOW);
  for (int i = 7; i >= 0; i--) {              // MSB first
    digitalWrite(clockPin, LOW);
    digitalWrite(dataPin, (pattern >> i) & 0x01);
    digitalWrite(clockPin, HIGH);
  }
  digitalWrite(clockPin, LOW);
  digitalWrite(latchPin, HIGH);
}

// Non-blocking multiplexing: call as often as possible. Every REFRESH_US it
// switches to the next digit (blank all -> load segments -> enable one digit).
void refreshDisplay() {
  static uint8_t current = 0;
  static uint32_t lastUs = 0;
  uint32_t now = micros();
  if (now - lastUs < REFRESH_US) return;
  lastUs = now;

  for (uint8_t i = 0; i < 4; i++) digitalWrite(digitPins[i], DIGIT_OFF);  // no ghosting

  uint8_t pattern = displayBuf[current];
  if (dpOn && current == 3) pattern |= DP_BIT;
  writeReg(pattern);

  digitalWrite(digitPins[current], DIGIT_ON);
  current = (current + 1) & 0x03;
}

// Fills the buffer with the number, leading zeros omitted (0 still shows "0").
void setNumber(uint16_t value) {
  for (int i = 3; i >= 0; i--) {
    if (i == 3 || value > 0) {
      displayBuf[i] = digitEncoding[value % 10];
      value /= 10;
    } else {
      displayBuf[i] = 0;  // blank
    }
  }
}

// Inputs
// Debounced press of the joystick button (returns true once per press).
bool buttonPressed() {
  static bool lastRaw = HIGH, stable = HIGH;
  static uint32_t changedAt = 0;
  bool raw = digitalRead(buttonPin);
  if (raw != lastRaw) { lastRaw = raw; changedAt = millis(); }
  if (millis() - changedAt > 30 && raw != stable) {
    stable = raw;
    if (stable == LOW) return true;
  }
  return false;
}

// Joystick pushed right: true once per flick.
bool joyRightFlick() {
  static bool armed = true;
  int x = analogRead(joyXPin);
  if (armed && x > JOY_HIGH) { armed = false; return true; }
  if (x < JOY_REARM) armed = true;
  return false;
}

// Game
enum GameState { IDLE, COUNTDOWN, PLAYING, STOPPED };
GameState state = IDLE;

uint16_t targetNumber = 0;
uint16_t playerNumber = 0;
uint16_t errorValue   = 0;
uint32_t stateStart   = 0;

void newTarget() {
  targetNumber = 1 + esp_random() % MAX_VALUE;
  Serial.print("New target: ");
  Serial.println(targetNumber);
}

void enterIdle() {
  newTarget();
  state = IDLE;
  dpOn = false;
  setNumber(targetNumber);
  Serial.println("Press the button to start.");
}

void enterCountdown() {
  playerNumber = 0;
  state = COUNTDOWN;
  stateStart = millis();
  dpOn = false;
  Serial.println("Get ready...");
}

void enterPlaying() {
  state = PLAYING;
  stateStart = millis();
  dpOn = true;  // DP = game running
  setNumber(playerNumber);
  Serial.println("GO! Scroll with the joystick (up/down), press the button to stop.");
}

void enterStopped() {
  state = STOPPED;
  dpOn = false;
  errorValue = abs((int)playerNumber - (int)targetNumber);
  int score = 100 - (int)min<uint16_t>(100, errorValue / 10);  // 0 error = 100 points
  setNumber(playerNumber);  // frozen
  Serial.print("Stopped at "); Serial.print(playerNumber);
  Serial.print(" | target "); Serial.print(targetNumber);
  Serial.print(" | absolute error "); Serial.print(errorValue);
  Serial.print(" | score "); Serial.println(score);
  Serial.println("Button = new target | joystick right = replay same target | hold joystick left = show error");
}

// Joystick Y scrolls the number; harder push = bigger step and faster repeat.
void updateScrolling() {
  static uint32_t lastStep = 0;
  int delta = (int)analogRead(joyYPin) - JOY_CENTER;
  if (abs(delta) < JOY_DEADZONE) return;

  int mag = min(abs(delta) - JOY_DEADZONE, JOY_MAX_MAG);   // 0 .. JOY_MAX_MAG
  int step;
  uint32_t interval;
  if (mag < JOY_MAX_MAG * 5 / 10)      { step = 1;   interval = map(mag, 0, JOY_MAX_MAG * 5 / 10, 200, 30); }
  else if (mag < JOY_MAX_MAG * 9 / 10) { step = 10;  interval = map(mag, JOY_MAX_MAG * 5 / 10, JOY_MAX_MAG * 9 / 10, 150, 40); }
  else                                 { step = 100; interval = 80; }

  if (millis() - lastStep < interval) return;
  lastStep = millis();

  // Swap the sign if "up" scrolls down on your joystick.
  int dir = (delta > 0) ? 1 : -1;
  int next = ((int)playerNumber + dir * step) % (MAX_VALUE + 1);
  if (next < 0) next += MAX_VALUE + 1;
  playerNumber = next;
  setNumber(playerNumber);
}

void updateGame() {
  switch (state) {
    case IDLE:
      if (buttonPressed()) enterCountdown();
      break;

    case COUNTDOWN: {
      uint32_t elapsed = millis() - stateStart;
      if (elapsed >= 3 * COUNTDOWN_STEP_MS) { enterPlaying(); break; }
      setNumber(3 - elapsed / COUNTDOWN_STEP_MS);
      break;
    }

    case PLAYING: {
      updateScrolling();
      static uint32_t lastPrint = 0;
      if (millis() - lastPrint >= 1000) {
        lastPrint = millis();
        Serial.print("Time left: ");
        Serial.println((GAME_TIME_MS - (millis() - stateStart)) / 1000 + 1);
      }
      if (buttonPressed() || millis() - stateStart >= GAME_TIME_MS) enterStopped();
      break;
    }

    case STOPPED:
      // Hold the joystick left to peek at the error, release to see your number again.
      setNumber(analogRead(joyXPin) < JOY_LOW ? errorValue : playerNumber);
      if (buttonPressed()) enterIdle();            // reset: new target
      else if (joyRightFlick()) enterCountdown();  // replay: same target
      break;
  }
}

// Arduino entry points
void setup() {
  Serial.begin(115200);

  pinMode(dataPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(latchPin, OUTPUT);
  for (uint8_t i = 0; i < 4; i++) {
    digitalWrite(digitPins[i], DIGIT_OFF);
    pinMode(digitPins[i], OUTPUT);
  }
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(joyXPin, INPUT);
  pinMode(joyYPin, INPUT);

  enterIdle();
}

void loop() {
  updateGame();
  refreshDisplay();  // no delay() anywhere: the display is multiplexed on every pass
}