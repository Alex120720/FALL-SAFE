# FALL-SAFE -- Arduino-Based Worker Fall Detection and Emergency Alert System

**Course:** 24ECT56 - Microprocessor and Microcontrollers  
**Department:** Computer Science and Engineering (Cyber Security)  
**Institution:** K.S.R College of Engineering (Autonomous), Tiruchengode, Namakkal-637215  
*(Affiliated to Anna University, Accredited by NAAC with A++ Grade)*

## Team

| Name | Roll Number |
|------|-------------|
| Alex R | 73152426003 |
| Bharathi P | 73152426009 |
| Kavin L P | 73152426026 |

## Introduction

FALL-SAFE is an embedded safety system designed to detect possible falls and send emergency alerts using an ESP32 microcontroller, an ADXL345 accelerometer, and a SIM808 GSM/GPS module.

The system continuously monitors the acceleration of a person using the ADXL345 sensor. When a possible fall is detected based on a freefall followed by a high-impact acceleration, the ESP32 activates a buzzer to provide a local alert. The SIM808 module attempts to obtain the GPS coordinates of the person and sends an emergency SMS to a predefined mobile number. If GPS coordinates are available, the message includes latitude, longitude, UTC time, and a Google Maps location link.

This project demonstrates how IoT-related hardware, motion sensing, GPS positioning, and GSM communication can be combined to support emergency notification systems for workers and individuals.

## Circuit Diagram

The FALL-SAFE system uses an ESP32 microcontroller connected to an ADXL345 accelerometer, SIM808 GSM/GPS module, and buzzer. The ADXL345 measures acceleration through I2C communication. When a possible fall is detected, the ESP32 activates the buzzer and uses the SIM808 module to obtain GPS coordinates and send an emergency SMS with the location details when available.

### Connection Summary

| Component | Pin | ESP32 Pin |
|-----------|-----|-----------|
| ADXL345 VCC | VCC | 3V3 |
| ADXL345 GND | GND | GND |
| ADXL345 SDA | SDA | GPIO21 |
| ADXL345 SCL | SCL | GPIO22 |
| ADXL345 CS | CS | 3V3 |
| ADXL345 SDO | SDO | GND |
| SIM808 TXD | TXD | GPIO16 (RX2) |
| SIM808 RXD | RXD | GPIO17 (TX2) |
| SIM808 GND | GND | GND |
| Buzzer/Driver | IN | GPIO25 |
| Buzzer/Driver VCC | VCC | 3V3 |
| Buzzer/Driver GND | GND | GND |

> **Note:** ESP32 is powered via USB / 5V suitable supply. SIM808 requires a module-rated power supply (do not power from ESP32 3V3). Verify actual board pin labels and voltage compatibility before powering.

## Components

### ESP32 Microcontroller
The ESP32 is a widely used microcontroller development board in embedded systems and IoT applications. It provides processing capabilities, GPIO pins, and serial communication interfaces. In this project, it processes accelerometer readings, detects possible falls, controls the buzzer, and communicates with the SIM808 GSM/GPS module.

### ADXL345 Accelerometer
The ADXL345 is a three-axis digital accelerometer used to measure acceleration along the X, Y, and Z axes. In this project, it continuously monitors movement and provides acceleration data to the ESP32. These readings help identify possible freefall and impact conditions for fall detection.

### SIM808 GSM/GPS Module
The SIM808 is a GSM and GPS module used for cellular communication and location tracking. It helps obtain the geographical coordinates of the person and send emergency SMS notifications. When GPS coordinates are available, the message includes the location details and a Google Maps link.

### Buzzer
A buzzer is an electronic component that produces an audible sound when activated. In this project, it is connected to the ESP32 through GPIO 25 and provides a warning when a possible fall is detected.

## How It Works

1. The ADXL345 continuously reads acceleration data
2. The system calculates the acceleration magnitude in g-force
3. When freefall is detected (acceleration < 0.55g), a freefall window opens
4. If a high impact (> 1.8g) follows within 1200ms, a fall alert is triggered
5. The buzzer activates for 10 seconds
6. The system attempts to get GPS coordinates (up to 3 attempts)
7. An emergency SMS is sent with impact force, GPS coordinates, and a Google Maps link
8. A 30-second cooldown prevents duplicate alerts

## Dependencies

- [Wire.h](https://www.arduino.cc/en/reference/wire) -- I2C communication
- [Adafruit_Sensor.h](https://github.com/adafruit/Adafruit_Sensor) -- Unified sensor abstraction
- [Adafruit_ADXL345_U.h](https://github.com/adafruit/Adafruit_ADXL345) -- ADXL345 driver

## Working Demo

[Watch on YouTube](https://youtu.be/KF_RNg9GUk4)
