# Homework #4

For this homework I built a Smart Lamp Controller using an ESP32 and PlatformIO (Arduino framework). A single push button cycles the lamp through its operating modes, and an external LED acts as the lamp. The ESP32's built-in LED works as an independent heartbeat. The button is read with a GPIO interrupt combined with software debouncing, and both short and long presses are recognised.

## Circuit Design and Connections

| Component | ESP32 connection | Purpose |
|---|---:|---|
| Push button | GPIO21 (other side to GND) | Mode control, uses `INPUT_PULLUP` |
| Lamp LED (+ 220 Ω resistor) | GPIO18 | Lamp output, brightness set with PWM |
| Built-in LED | GPIO2 | System heartbeat |

The button connects GPIO21 to GND, so the internal pull-up resistor keeps the pin HIGH and a press pulls it LOW (no external resistor needed). The lamp LED is connected through a 220 Ω current-limiting resistor to GND. All components share a common ground.

![Homework 4 circuit schematic](HomeWork%234/photos/schematic%20circuit.png)

![Assembled Homework 4 circuit](HomeWork%234/photos/circuit.jpg)

## Solution

The program is split into small independent parts, and each one is timed with `millis()` so that `loop()` never blocks.

**Checkpoint 1 - Operating modes.** The modes are stored in an `enum` (`OFF`, `DIM`, `FULL`, `BLINK`, plus the bonus `BREATHE`). Each debounced short press moves to the next mode: OFF → DIM → FULL → BLINK → BREATHE → OFF. The lamp brightness is set with `analogWrite()`: 0 for OFF, 64 (about 25 %) for DIM and 255 for FULL.

**Checkpoint 2 - Independent operations.** In BLINK mode the lamp toggles every 500 ms. The program stores the time of the last toggle and compares it with `millis()` instead of using `delay()`. The built-in LED on GPIO2 is a separate heartbeat that toggles every 500 ms on its own timer, so it keeps running in every mode and shows that the main loop is never blocked.

**Checkpoint 3 - Short and long presses.** When a press is validated, the program stores the time it started. If the button is released before 1000 ms it counts as a short press and the mode changes. If the button is still held after 1000 ms, a long press is triggered and the lamp goes to OFF immediately. A flag (`longPressHandled`) makes sure the long press is not also treated as a short press when the button is released.

**Checkpoint 4 - Interrupt-assisted button handling.** The button pin has a `FALLING` edge interrupt. The ISR only sets a `volatile bool` flag, so it stays as short as possible (marked `IRAM_ATTR` for the ESP32). The application logic is outside the ISR: when `loop()` sees the flag it starts a debounce check. During this check the pin is polled, and any change of the reading restarts the 50 ms timer. The new state is accepted only after the reading has been stable for 50 ms, which filters out contact bounce. When the button is released and stable, checking stops and the system waits for the next interrupt.

## Design Decisions

- **Debouncing:** time-based, 50 ms stable-reading rule (the technique from Lab 3, Section 3). Polling is only done while a press is being validated.
- **Interrupts:** the ISR only sets a flag; it does not print, debounce or change modes. This avoids long ISRs and race problems.
- **Non-blocking timing:** blink, breathe, heartbeat and long-press detection all use `millis()` differences, so they run at the same time without `delay()`.
- **Bonus:** a fifth `BREATHE` mode fades the lamp up and down smoothly (step of 3 every 10 ms).

The Serial Monitor output below shows the mode changes on short presses and the `Long press -> Mode: OFF` message when the button is held.

![Homework 4 serial monitor output](HomeWork%234/photos/terminal.jpg)

## Pictures and Videos

[Watch the demonstration video (all four checkpoints)](HomeWork%234/photos/working%20circuit.mp4)

## Conclusion

This project combines debounced input, a short ISR, short/long press detection and several independent timed behaviours in one ESP32 program. Using an interrupt only to signal a possible press, and validating it in the main loop with `millis()`, gives reliable button handling while the blinking and the heartbeat continue to run without blocking.