# Introduction To Robotics (2026-2027)

Meylis Hudayberdiyev's repository for presenting homework from the "Intro to Robotics" course taken in  3rd year in 2026-2027. Each homework will have a detailed description in the following sections.
__________________________________________________________________________________________________________

## Homework #1

For this homework I had to complete a formular via this link https://forms.gle/35oZKw9QovH9uM5U7. This homework was assigned and successfully submitted through Microsoft Teams on Wednesday 7th of October, 2026 at 20:43. I, personally, did not have any problems completing this formular. Through this homework I got my group where I will continue my studies in "Introduction to Robotics" subject.
__________________________________________________________________________________________________________

## Homework #2

For this homework I had to learn how to work with the Falstad Circuit Simulator and strengthen my knowledge of fundamental electrical components, Ohm’s Law, Kirchhoff’s Voltage Law (KVL), and Kirchhoff’s Current Law (KCL) by creating and analyzing circuits, calculating electrical values, and confirming my theoretical results through simulations.


### First Exercise (Series Circuit)

To complete this exercise I had to create a circuit with a direct current voltage source of 5 V and at least two resistors and one LED in series. Afterwards I had to choose resistor values to get a current of 5-20 mA through LED. And finally I had to calculate the current, voltage drops and power dissipated by each resistor and to verify KVL.

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

![Exercise 1 circuit](Homework%232/ex1/ex1circuit.png)

![Exercise 1 calculations](Homework%232/ex1/ex1sol.jpg)

[Watch the Exercise 1 video](Homework%232/ex1/ex1vid.mov)
__________________________________________________________________________________________________________

### Second Exercise (Parallel Circuit)

To complete this exercise I had to create a circuit with a direct current voltage source of 9 V and at least two parallel branches, each containing resistor and LED. Afterwards I had to choose resistor values to get different currents of 5-20 mA through the LEDs. And finally I had to calculate the currentof each branch, the total current and resistor power dissipation and to verify KCL and KVL.

### Solution

The objective of this experiment is to build and analyze a parallel circuit containing two branches, each consisting of a resistor and an LED connected in series.

The circuit consists of:
- One 9 V DC voltage source.
- One red LED in each branch.
- One 800 Ω resistor in the first branch.
- One 400 Ω resistor in the second branch.

Both branches are connected in parallel across the 9 V supply. Each resistor limits the current flowing through its corresponding LED. Different resistor values produce different branch currents. For the theoretical calculations, the forward voltage of each LED is assumed to be approximately 2 V.

The voltage across each resistor is calculated using:
V<sub>R</sub> = V<sub>S</sub> - V<sub>LED</sub>
Assuming a supply voltage of 9 V and an LED forward voltage of 2 V:
V<sub>R</sub> = 9 - 2 = 7 V
Therefore, the voltage across each resistor is approximately 7 V.

Now I calculate current on each branch.

Using Ohm's Law:

I<sub>1</sub> = V<sub>R</sub> / R<sub>1</sub>

I<sub>1</sub> = 7 / 800

I<sub>1</sub> = 0.00875 A = 8.75 mA

I<sub>2</sub> = V<sub>R</sub> / R<sub>2</sub>

I<sub>2</sub> = 7 / 400

I<sub>2</sub> = 0.0175 A = 17.5 mA

Both branch currents are within the required range of 5-20 mA.

Total current:

I<sub>T</sub> = I<sub>1</sub> + I<sub>2</sub>

I<sub>T</sub> = 8.75 + 17.5 

I<sub>T</sub> = 26.25 mA

Next I calculated the power dissipated by each resistor using **Joule's Law of Heating**:

P = I^2 * R

V = I * R => I = V / R

P = (V / R)^2 * R => P = V^2 / R

P<sub>1</sub> = 7^2 / 800 = 0.06125 W = 61.25 mW

P<sub>2</sub> = 7^2 / 400 = 0.1225 W = 122.5 mW

Both resistors can be implemented using standard 0.25 W resistors.

Kirchhoff's Current Law states that the total current entering a junction equals the total current leaving the junction.
For this circuit:

I<sub>T</sub> = I<sub>1</sub> + I<sub>2</sub>

I<sub>T</sub> = 26.25 mA

I<sub>1</sub> + I<sub>2</sub> = 18.75 + 17.5 = 26.25 mA

Therefore, KCL is satisfied.

Kirchhoff's Voltage Law states that the algebraic sum of the voltages around a closed loop equals zero.
For Branch 1:

V<sub>S</sub> - V<sub>R1</sub> - V<sub>LED1</sub> = 0

9 - 7 - 2 = 0

For Branch 2:
V<sub>S</sub> - V<sub>R2</sub> - V<sub>LED2</sub> = 0

9 - 7 - 2 = 0

Therefore, KVL is satisfied for both branches under the assumed LED forward voltage.

### Pictures and Videos

![Exercise 2 circuit](Homework%232/ex2/ex2circuit.png)

![Exercise 2 calculations](Homework%232/ex2/ex2sol.jpg)

[Watch the Exercise 2 video](Homework%232/ex2/ex2vid.mov)
__________________________________________________________________________________________________________

### Third Exercise (RC Circuit)

