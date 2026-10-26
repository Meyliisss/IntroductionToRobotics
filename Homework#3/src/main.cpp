#include <Arduino.h>
#include "pitches.h"
// On our development board, the built-in LED is connected to GPIO2.
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// ---------- Pins ----------
const int POT_PIN = 34;     // Potentiometer middle leg (ADC1, input-only pin)
const int FAN_LED_PIN = 18; // External LED (through 220 Ohm resistor)
const int RGB_RED_PIN = 25;   // RGB LED red leg   (through 220 Ohm resistor)
const int RGB_GREEN_PIN = 26; // RGB LED green leg (through 220 Ohm resistor)
const int RGB_BLUE_PIN = 27;  // RGB LED blue leg  (through 220 Ohm resistor)
const int buzzerPin = 23;
const int buzzerTone = 1000; // Frequency of the buzzer tone in Hz

// Notes in the melody (0 represents a rest).
const int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3,
  NOTE_G3, 0, NOTE_B3, NOTE_C4
};
// Note durations: 4 = quarter note, 8 = eighth note, etc.
const int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};
const int noteCount = sizeof(melody) / sizeof(melody[0]);


int state = 0; //0-blue, 1-yellow, 2-red
int prMillis = 0;
bool ledState = LOW;
bool buzzed = false;

// Melody player state (non-blocking, so loop() keeps running while it plays).
bool melodyPlaying = false;
int noteIndex = 0;
unsigned long noteStartMillis = 0;

void playCurrentNote() {
  noteStartMillis = millis();
  // A frequency of 0 represents a rest.
  if (melody[noteIndex] > 0) {
    tone(buzzerPin, melody[noteIndex], 1000 / noteDurations[noteIndex]);
  }
}

void startMelody() {
  melodyPlaying = true;
  noteIndex = 0;
  playCurrentNote();
}

// Call every loop(): moves on to the next note once the current one
// (plus a 30% pause) is over.
void updateMelody() {
  if (!melodyPlaying) return;
  int noteDuration = 1000 / noteDurations[noteIndex];
  if (millis() - noteStartMillis >= noteDuration * 1.30) {
    noTone(buzzerPin);
    noteIndex++;
    if (noteIndex >= noteCount) {
      melodyPlaying = false;
    } else {
      playCurrentNote();
    }
  }
}


void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  

  analogReadResolution(12);
  

  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
  pinMode(19, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  digitalWrite(19, HIGH);

  
}

void loop() {
  int currentMillis = millis();
  int potValue = analogRead(POT_PIN);
  int fanLedBrightness = map(potValue, 0, 4095, 0, 255);
  if(currentMillis - prMillis >= 1000){
    prMillis = currentMillis;
    ledState = !ledState;
    digitalWrite(LED_BUILTIN, ledState);
    Serial.printf("Potentiometer value: %d\n", potValue);
    Serial.printf("Fan LED brightness: %d\n", fanLedBrightness);
    if(state == 0){
      Serial.println("RGB LED color: Blue");
    } else if(state == 1){
      Serial.println("RGB LED color: Yellow");
    } else {
      Serial.println("RGB LED color: Red");
    }
  }
 
  if (potValue < 1365) {
    digitalWrite(RGB_RED_PIN, LOW);
    digitalWrite(RGB_GREEN_PIN, LOW);
    digitalWrite(RGB_BLUE_PIN, HIGH);
    state = 0;
  } else if (potValue < 2730 && potValue >= 1365) {
    digitalWrite(RGB_RED_PIN, HIGH);
    digitalWrite(RGB_GREEN_PIN, HIGH);
    digitalWrite(RGB_BLUE_PIN, LOW);
    state = 1;
  } else {
    digitalWrite(RGB_RED_PIN, HIGH);
    digitalWrite(RGB_GREEN_PIN, LOW);
    digitalWrite(RGB_BLUE_PIN, LOW);
    state = 2;

    
  }
  analogWrite(FAN_LED_PIN, fanLedBrightness);
  if (potValue > 2730 && !buzzed) {
    startMelody();
    buzzed = true;
  }
  updateMelody();
  if (potValue <= 2600) {
    buzzed = false;
  }
  
}
