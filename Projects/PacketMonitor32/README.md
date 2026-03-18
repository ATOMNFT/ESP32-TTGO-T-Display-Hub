# PacketMonitor32 for TTGO T-Display

[![Platform](https://img.shields.io/badge/platform-ESP32-blue)](#)
[![Display](https://img.shields.io/badge/display-TTGO%20T--Display-orange)](#)
[![Framework](https://img.shields.io/badge/framework-Arduino-green)](#)
[![Library](https://img.shields.io/badge/UI-TFT__eSPI-red)](#)

A compact TTGO T-Display port of Spacehuhn's original **PacketMonitor32** project:

**Original project:** https://github.com/spacehuhn/PacketMonitor32

This version adapts the idea for the **LILYGO TTGO T-Display (ESP32)**, adds a TFT-based UI, on-device controls, deep sleep, a stats page, and external **SPI SD card logging**.

## Features

- Live Wi-Fi packet activity graph
- Stats page with boxed rows
- Short press **GPIO 35** to step channels
- Long hold **GPIO 35** to enter deep sleep
- Short press **GPIO 0** to toggle SD logging
- Double tap **GPIO 0** to switch between graph and stats page
- External **SPI SD card** logging
- TFT display output using **TFT_eSPI**

## Based On

This project is a TTGO T-Display port of Spacehuhn's PacketMonitor32.

- Original repo: https://github.com/spacehuhn/PacketMonitor32
- This port: TTGO T-Display UI, TFT_eSPI support, SPI SD support, sleep handling, and button-driven workflow

## Hardware

- **LILYGO TTGO T-Display**
- **SPI SD card module**
- **MicroSD card**
- Jumper wires

## SD Wiring

This sketch uses an **SPI SD card module** with the following pins:

| SD Module | TTGO T-Display GPIO |
|---|---:|
| CS | 33 |
| SCK / CLK | 25 |
| MISO | 36 |
| MOSI | 26 |
| VCC | 3.3V |
| GND | GND |

## Controls

| Action | Function |
|---|---|
| GPIO 35 short press | Next Wi-Fi channel |
| GPIO 35 long hold | Enter deep sleep |
| GPIO 0 short press | Toggle SD logging |
| GPIO 0 double tap | Switch graph / stats page |

## TFT_eSPI Setup

Before compiling, make sure **TFT_eSPI** is configured for the **TTGO T-Display**.

Typical TTGO T-Display values:

- Driver: **ST7789**
- Width: **135**
- Height: **240**
- MOSI: **19**
- SCLK: **18**
- CS: **5**
- DC: **16**
- RST: **23**
- BL: **4**

Also make sure the correct setup is enabled in:

`TFT_eSPI/User_Setup_Select.h`

## Flashing

1. Install the **ESP32 board package** in Arduino IDE.
2. Install the **TFT_eSPI** library.
3. Place these files in the same sketch folder:
   - `PacketMonitor32.ino`
   - `Buffer.h`
   - `Buffer.cpp`
4. Configure **TFT_eSPI** for the TTGO T-Display.
5. Select the correct ESP32 board in Arduino IDE.
6. Connect the TTGO T-Display over USB.
7. Compile and upload the sketch.
8. Open **Serial Monitor** at **115200 baud** if you want debug output.

## Notes

- If the TFT backlight comes on but the screen stays black, verify the active **TFT_eSPI** setup.
- This build uses **SPI SD**, not **SD_MMC**.
- GPIO 0 is also a boot/strapping pin, so avoid holding it during reset or power-up.
- If SD logging fails, double-check your SPI SD wiring and card formatting.

## Images

![Main Graph View](images/graph.jpg?text=Main+Graph+View)

![Stats Page](images/stats.jpg?text=Stats+Page+or+Hardware+Setup)

## Credits

- **Original PacketMonitor32:** Spacehuhn
