# Homework #3

For this homework I built a fan control and status panel using an ESP32. A potentiometer sets the requested speed, and an external LED represents the fan: its brightness is controlled with PWM. An RGB LED indicates the selected speed level, while a passive buzzer plays a short melody when the control enters the high-speed range. The ESP32's built-in LED blinks to show that the controller is running. The fan is represented by an LED, so no motor is connected.

## Circuit Design and Connections

The circuit uses a potentiometer as an analog input, three RGB LED channels as status outputs, a separate LED as the fan indicator, and a passive buzzer. The circuit diagram and breadboard layout are shown below.

| Component | ESP32 connection | Purpose |
|---|---:|---|
| Built-in LED | GPIO2 | Controller heartbeat |
| Potentiometer wiper | GPIO34 | Reads the requested speed |
| Fan indicator LED | GPIO18 | Brightness represents fan speed |
| RGB LED red channel | GPIO25 | High-speed status |
| RGB LED green channel | GPIO26 | Combined with red for medium speed |
| RGB LED blue channel | GPIO27 | Low-speed status |
| Passive buzzer | GPIO23 | Plays a melody on high-speed entry |

The potentiometer's outer pins connect to 3.3 V and GND, and its middle pin connects to GPIO34. The RGB LED is common-cathode, with its common pin connected to GND. Each external LED has its own 220 Ω current-limiting resistor. All components share a common ground. The potentiometer must use the ESP32's 3.3 V supply, not 5 V.

![Homework 3 circuit schematic](Homework%233/photos/schema%20circuit.jpg)

![Homework 3 breadboard circuit layout](Homework%233/photos/schema.jpg)

## Solution

The ESP32 reads the potentiometer using its 12-bit ADC, which produces a value from 0 to 4095. The program converts this reading into an 8-bit PWM value from 0 to 255:

PWM = ADC reading × 255 / 4095

For example, an ADC reading near the midpoint (about 2048) maps to a PWM value near 127. This PWM output on GPIO18 changes the fan LED's brightness: a higher reading makes it brighter and represents a higher fan speed.

The potentiometer range is divided into three status levels:

- **Low speed (0-1364):** the blue RGB channel is on.
- **Medium speed (1365-2729):** the red and green channels are on together, producing yellow.
- **High speed (2730-4095):** the red RGB channel is on.

The passive buzzer plays an eight-note melody when the potentiometer reading rises above 2730. A flag prevents the melody from restarting continuously while the control stays high. The flag is reset when the reading falls to 2600 or below, allowing the melody to play again the next time the control enters the high-speed range.

The built-in LED changes state every 1000 ms as a heartbeat. The program uses `millis()` to manage the heartbeat and advance the melody without stopping the main loop, so it can continue reading the potentiometer and updating the LEDs while the melody plays. Once per second, the Serial Monitor reports the ADC reading, the calculated fan LED brightness, and the selected RGB color.

The Wokwi project view shows the running circuit and its output, and the photograph shows the assembled hardware.

![Homework 3 Wokwi code and simulation](Homework%233/photos/schema%20code.jpg)

![Assembled Homework 3 circuit](Homework%233/photos/implementation.jpg)

## Pictures and Videos

[Watch the circuit design video](Homework%233/photos/schema%20work.mp4)

[Watch the implementation video](Homework%233/photos/implement%20vid.mp4)

## Conclusion

This project combines analog input, PWM output, visual status indicators, and sound notifications in one ESP32 controller. The potentiometer controls the simulated fan speed, while the RGB LED and buzzer provide feedback about the selected operating level. The heartbeat and melody are timed without blocking the main loop, allowing the controller to continue responding to changes in the potentiometer.