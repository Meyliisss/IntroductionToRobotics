# Homework #5 - Stop the Counter!

For this homework I built a reaction game using an ESP32, a 74HC595 shift register, a four-digit seven-segment display, and a joystick. At the start of each round the display shows a target. The player then uses the joystick to adjust a number and tries to get as close to the target as possible before the round ends.

## Circuit Design and Connections

The 74HC595 controls the display segments, while four ESP32 GPIO pins select which digit is active. The joystick provides two analog axes and a push button.

| Component / signal | ESP32 connection | Purpose |
|---|---:|---|
| 74HC595 DS (data) | GPIO23 | Sends segment bits to the shift register |
| 74HC595 SHCP (clock) | GPIO18 | Clocks each data bit into the register |
| 74HC595 STCP (latch) | GPIO22 | Applies the segment pattern to the outputs |
| Display digits D1-D4 | GPIO25, 26, 27, 33 | Selects the active display digit |
| Joystick VRx | GPIO34 | Left/right actions |
| Joystick VRy | GPIO35 | Adjusts the displayed number |
| Joystick SW | GPIO32 | Starts, stops, or resets a round |

The 74HC595 and joystick are powered from 3.3 V. Each display segment has a current-limiting resistor; the schematic shows 500-1000 Ω resistors. All components are connected to the same circuit ground.

![Homework 5 circuit schematic](photos/circuit%20schematic.jpg)

![Assembled Homework 5 display and joystick](photos/digitdisplay%20game.jpg)

## Game and Controls

At startup, the program chooses a target from 1 to 9999 and displays it. The game has four states: `IDLE`, `COUNTDOWN`, `PLAYING`, and `STOPPED`.

| Action | Result |
|---|---|
| Press the joystick button while the target is displayed | Start the 3-2-1 countdown |
| Push the joystick vertically during the round | Change the displayed number up or down |
| Press the joystick button during the round | Stop and score the round |
| Let 20 seconds pass | Stop the round automatically |
| Hold the joystick to the left after stopping | Temporarily display the absolute error |
| Release the joystick after showing the error | Show the stopped number again |
| Flick the joystick to the right after stopping | Replay with the same target |
| Press the button after stopping | Start a new round with a new target |

The joystick's Y-axis has a dead zone so small movements do not change the number. A light push changes the value by 1, a stronger push by 10, and a full push by 100. The repeat interval also changes with the joystick magnitude to make large adjustments quicker. The value wraps around between 0 and 9999.

## Implementation

### Target, timing, and state transitions

`newTarget()` chooses a new number at the start and after a reset. The target stays the same when replaying a round.

```cpp
void newTarget() {
  targetNumber = 1 + esp_random() % MAX_VALUE;
  Serial.print("New target: ");
  Serial.println(targetNumber);
}
```

Pressing the button in `IDLE` starts a three-second 3-2-1 countdown. In `PLAYING`, the joystick adjusts the player's number and the button or 20-second timer ends the round. The state and elapsed time are checked in `updateGame()` on each pass through `loop()`.

When stopped, the program freezes the player's number, calculates the absolute error, and prints the target, stopped value, error, and score to the Serial Monitor. The score starts at 100 and decreases by one point for each 10 units of error, to a minimum of zero.

```cpp
errorValue = abs((int)playerNumber - (int)targetNumber);
int score = 100 - (int)min<uint16_t>(100, errorValue / 10);
```

### Seven-segment display and bitwise control

Each digit is stored as a compact `uint8_t` segment pattern. `setNumber()` extracts decimal digits and blanks unused leading positions, so 42 is shown as `42`, not `0042`. Zero remains visible as one digit. During the game, the decimal point is added to the last digit with a bitwise OR:

```cpp
uint8_t pattern = displayBuf[current];
if (dpOn && current == 3) pattern |= DP_BIT;
writeReg(pattern);
```

`writeReg()` sends the segment pattern to the 74HC595 one bit at a time, most-significant bit first, using the data, clock, and latch pins. The display is multiplexed: `refreshDisplay()` disables all digits, updates the segment pattern, and enables the next digit. Each digit is refreshed every 2000 microseconds, resulting in a full four-digit refresh about 125 times per second.

### Non-blocking behavior and input handling

All timing uses elapsed-time checks with `millis()` or `micros()`. There are no `delay()` calls or busy-wait loops. The main loop processes the game and refreshes the display continuously:

```cpp
void loop() {
  updateGame();
  refreshDisplay();
}
```

The joystick button uses a 30 ms stability check to debounce presses. The joystick's right flick is edge-triggered and must return below a re-arm threshold before it can trigger again. These checks prevent accidental repeated actions while keeping display refresh active.

### Implementation note

The current implementation includes a three-second 3-2-1 start countdown. During the round, however, the player adjusts a number with the joystick; the program does not automatically decrement a starting value that is higher than the target as described in the assignment brief.

## Pictures and Video

[Watch the working game demonstration](photos/working%20game.mp4)

## Conclusion

This project combines shift-register output, multiplexed seven-segment display control, analog joystick input, debounced button handling, and timed game states. Segment patterns and the decimal point are controlled with byte-sized bit patterns and bitwise operations. The main loop keeps the display refreshed while it processes input and checks the game timer without blocking.
