
# Firebird V ATmega2560 Experiments (e-Yantra MOOC)

This repository contains the experiments performed during the **e-Yantra MOOC** using the **Firebird V robot** and **ATmega2560 microcontroller**.  

The experiments focus on understanding the basics of **Embedded Systems**, **microcontroller interfacing**, **ADC**, **Interrupts**, and **PWM motor control** using **Embedded C**.

---

## Hardware Used

- Firebird V Robot
- ATmega2560 Microcontroller
- L293D Motor Driver
- White Line Sensors
- IR Proximity Sensors
- Sharp Distance Sensor
- Buzzer
- Interrupt Switch
- LCD Display

---

## Software Used

- AVR Studio / Atmel Studio
- Embedded C
- UART Serial Terminal
- Firebird V Libraries

---

# Experiments

## Experiment 1: Buzzer Interfacing

**Objective**

Interface the Buzzer with ATmega2560 and control it using Embedded C.

**Task**

- Configure the Buzzer pin as output.
- Turn the buzzer ON and OFF at an interval of **1 second**.
- The process repeats indefinitely.

**Concepts Used**

- GPIO configuration
- Output device interfacing
- Delay generation

---

## Experiment 2: Bar-Graph LEDs with Interrupt Switch

**Objective**

Interface Bar-graph LEDs and Interrupt Switch with the microcontroller.

**Task**

- Toggle the status of **two Bar-graph LEDs**
- LEDs change state depending on whether the **Interrupt Switch is pressed or released**

**Concepts Used**

- Input device interfacing
- Output device control
- Switch handling

---

## Experiment 3: ADC Interfacing (White Line and IR Sensors)

**Objective**

Learn to use the **ADC module of ATmega2560**.

**Task**

- Read **8-bit ADC values** from:
  - White Line Sensors
  - IR Proximity Sensors
- Display values on **LCD**
- Send **Center White Line sensor value** to **UART Serial Terminal**

**Concepts Used**

- ADC configuration
- Sensor interfacing
- LCD display
- UART communication

---

## Experiment 4: ADC with Interrupt (Sharp Sensor)

**Objective**

Use **ADC Conversion Interrupt** for sensor data acquisition.

**Task**

- Read **10-bit ADC value** from **Sharp Distance Sensor**
- Use **ADC interrupt mode**
- Display value on **LCD**
- Send value to **UART Serial Terminal**

**Concepts Used**

- ADC interrupt mode
- Sensor interfacing
- UART communication

---

## Experiment 5: Interrupt Controlled Buzzer

**Objective**

Understand **External Interrupts** on ATmega2560.

**Task**

- Turn ON the **Buzzer**
- When the **Interrupt Switch is pressed**

**Concepts Used**

- External interrupts
- Interrupt service routines (ISR)
- Event driven programming

---

## Experiment 6: Motor Speed Control using PWM

**Objective**

Control the speed of motors using **Phase Correct PWM Mode**.

**Task**

- Interface **L293D Motor Driver**
- Use **Timer PWM mode**
- Increase and decrease **motor speed**

**Concepts Used**

- PWM generation
- Timer configuration
- Motor driver interfacing

---

# Key Concepts Learned

- Embedded C programming
- GPIO interfacing
- ADC sensor interfacing
- Interrupt handling
- UART communication
- PWM motor control
- Firebird V robot architecture

---

# Anand Kumar 

**Government Engineering College Vaishali**  
Electronics and Communication Engineering  

---

# Acknowledgement

This work was completed as part of the **e-Yantra MOOC program** focusing on Embedded Systems and Robotics using the Firebird V platform.
