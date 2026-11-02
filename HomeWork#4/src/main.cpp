#include <Arduino.h>

const int buttonPin = 21; 
const int ledPin = 18;        
const int builtinLedPin = 2;  

enum LampMode {
  MODE_OFF,
  MODE_DIM,
  MODE_FULL,
  MODE_BLINK,
  MODE_BREATHE
};
const int NUM_MODES = 5;

LampMode currentMode = MODE_OFF;

const unsigned long debounceDelay = 50;  
int lastReading = HIGH;
int buttonState = HIGH;
unsigned long lastDebounceTime = 0;


volatile bool buttonInterrupt = false;
bool checkingButton = false;

//longpress
const unsigned long longPressTime = 1000;
unsigned long pressStartTime = 0;
bool longPressHandled = false;

//blink
const unsigned long blinkInterval = 500;
unsigned long lastBlinkTime = 0;
bool blinkOn = false;

//bonus: breathe
const unsigned long breatheInterval = 10;
unsigned long lastBreatheTime = 0;
int breatheBrightness = 0;
int breatheStep = 3;

// heartbeat
const unsigned long heartbeatInterval = 500;
unsigned long lastHeartbeatTime = 0;
int heartbeatState = LOW;

// ISR
void IRAM_ATTR handleButtonInterrupt() {
  buttonInterrupt = true;
}

void setup() {
  Serial.begin(115200);

  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(ledPin, OUTPUT);
  pinMode(builtinLedPin, OUTPUT);

  analogWrite(ledPin, 0);

  
  attachInterrupt(digitalPinToInterrupt(buttonPin), handleButtonInterrupt, FALLING);

  Serial.println("Smart Lamp Controller");
  Serial.println("Mode: OFF");
}

void loop() {
  unsigned long now = millis();


  bool shortPress = false;
  bool longPress = false;


  if (buttonInterrupt) {
    buttonInterrupt = false;

    if (!checkingButton) {
      checkingButton = true;
      lastReading = LOW;
      lastDebounceTime = now;
    }
  }


  if (checkingButton) {
    int reading = digitalRead(buttonPin);


    if (reading != lastReading) {
      lastDebounceTime = now;
    }


    if ((now - lastDebounceTime) > debounceDelay) {
      if (reading != buttonState) {
        buttonState = reading;

        if (buttonState == LOW) {


          pressStartTime = now;
          longPressHandled = false;
        } else {


          if (!longPressHandled) {
            shortPress = true;
          }
          checkingButton = false;
        }
      } else if (buttonState == HIGH) {

        checkingButton = false;
      }
    }

    lastReading = reading;


    if (buttonState == LOW && !longPressHandled) {
      unsigned long pressDuration = now - pressStartTime;
      if (pressDuration >= longPressTime) {
        longPress = true;
        longPressHandled = true;
      }
    }
  }


  if (longPress) {
    currentMode = MODE_OFF;
    Serial.println("Long press -> Mode: OFF");
  }

  if (shortPress) {
    currentMode = (LampMode)((currentMode + 1) % NUM_MODES);

    Serial.print("Mode: ");
    if (currentMode == MODE_OFF) {
      Serial.println("OFF");
    } else if (currentMode == MODE_DIM) {
      Serial.println("DIM");
    } else if (currentMode == MODE_FULL) {
      Serial.println("FULL");
    } else if (currentMode == MODE_BLINK) {
      Serial.println("BLINK");

      blinkOn = true;
      lastBlinkTime = now;
    } else if (currentMode == MODE_BREATHE) {
      Serial.println("BREATHE");

      breatheBrightness = 0;
      breatheStep = 3;
      lastBreatheTime = now;
    }
  }


  if (currentMode == MODE_OFF) {
    analogWrite(ledPin, 0);
  } else if (currentMode == MODE_DIM) {
    analogWrite(ledPin, 64);
  } else if (currentMode == MODE_FULL) {
    analogWrite(ledPin, 255);
  } else if (currentMode == MODE_BLINK) {

    if (now - lastBlinkTime >= blinkInterval) {
      lastBlinkTime = now;
      blinkOn = !blinkOn;
    }

    if (blinkOn) {
      analogWrite(ledPin, 255);
    } else {
      analogWrite(ledPin, 0);
    }
  } else if (currentMode == MODE_BREATHE) {

    if (now - lastBreatheTime >= breatheInterval) {
      lastBreatheTime = now;
      breatheBrightness = breatheBrightness + breatheStep;


      if (breatheBrightness >= 255) {
        breatheBrightness = 255;
        breatheStep = -breatheStep;
      } else if (breatheBrightness <= 0) {
        breatheBrightness = 0;
        breatheStep = -breatheStep;
      }
    }

    analogWrite(ledPin, breatheBrightness);
  }


  if (now - lastHeartbeatTime >= heartbeatInterval) {
    lastHeartbeatTime = now;
    heartbeatState = !heartbeatState;
    digitalWrite(builtinLedPin, heartbeatState);
  }
}
