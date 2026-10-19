# Introduction To Robotics (2026-2027)

Repository for presenting homework from the "Intro to Robotics" course taken in  3rd year in 2026-2027. Each homework will have a detailed description in the following sections.

## Homework #1

For this homework I had to complete a formular via this link https://forms.gle/35oZKw9QovH9uM5U7. This homework was assigned and successfully submitted through Microsoft Teams on Wednesday 7th of October, 2026 at 20:43. I, personally, did not have any problems completing this formular. Through this homework I got my group where I will continue my studies in "Introduction to Robotics" subject.

## Homework #2

For this homework I had to learn how to work with the Falstad Circuit Simulator and strengthen my knowledge of fundamental electrical components, Ohm’s Law, Kirchhoff’s Voltage Law (KVL), and Kirchhoff’s Current Law (KCL) by creating and analyzing circuits, calculating electrical values, and confirming my theoretical results through simulations.


### First Exercise

To complete this exercise I had to create a circuit with a direct current voltage source with 5 V and at least two resistors and one LED in series. Afterwards I had to choose resistor values to get a current of 5-20 mA through LED. And finally I had to calculate the current, voltage drops and power dissipated by each resistor and to verify KVL.

### Solution

I started with direct current voltage source with 5 V then connected through wires two resistors and one LED in series. I selected two 150 ohm resistors connected in series. 

Their total resistance is 300 ohms. Assuming the LED has a forward voltage of 2 V, the voltage across the resistors is 5 minus 2, which equals 3 V. Using Ohm’s law **( V = I * R )**, the current is 3 divided by 300, giving **10 mA**. That is how I got the current value.

To determine the allowable resistance range, I used the maximum current of 20 milliamperes to calculate a minimum resistance of 150 ohms, and the minimum current of 5 milliamperes to calculate a maximum resistance of 600 ohms. Therefore, the total resistance must be between 150 and 600 ohms under these assumptions or from 75 to 300 ohms each.

Since both resistors have the same resistance and are connected in series, they share the 3 V equally. So, for the first and second resistors we get V<sub>R1</sub>=V<sub>R2</sub>=0.01*150=**1.5 V** of voltage drops.

To calculate the power dissipated by each resistor I used **Joule's Law of Heating**. Power tells us how much electrical energy each resistor converts into heat per second. So, with this formula: **P=I^2*R** we get: P<sub>R1</sub>=P<sub>R2</sub>= (0.01)^2 * 150 = 0.015 W or 15 mW.

Kirchhoff's Voltage Law states:
_The sum of all voltage rises and drops around a closed loop equals zero._
For our circuit:
V<sub>source</sub>-V<sub>R1</sub>-V<sub>R2</sub>-V<sub>LED</sub>=0
5 - 1.5 - 1.5 - 2 = 0 
5 - 5 = 0 V
KVL is verified!

### Pictures and Videos

![Exercise 1 circuit](Homework%232/ex1.png)

![Exercise 1 calculations](Homework%232/ex1sol.jpg)

[Watch the Exercise 1 video](Homework%232/ex1vid.mov)
