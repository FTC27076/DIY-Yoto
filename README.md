# DIY Yoto

A screen-free, kid-friendly audio player in the style of a Yoto. Put a card on top and it plays that card's stories or songs. It runs fully offline: no Wi-Fi, no app, no account. All audio lives on a micro SD card, and any 13.56 MHz RFID card or NFC sticker can become a card.

## Features

- Tap a card to play, lift it to pause, put it back to resume where it stopped
- Each card remembers which track it was on
- Two knobs: volume (press to pause) and track skip (press to restart the track)
- Volume cap for small ears
- Rechargeable LiPo with USB-C charging; play while charging
- Deep sleep after 5 minutes idle, wake with either knob
- Low-battery warning and safe shutdown

## Hardware

| Part | What it does |
|---|---|
| ESP32-S3-WROOM-1 (SMD) | Main controller |
| RC522 RFID reader | Reads the cards |
| Adafruit MAX98357A (#3006) | I2S amplifier, drives a 4–8 Ω 3 W speaker |
| Adafruit micro SD breakout (#4682) | Audio storage, 4-bit SDIO |
| TP4056 USB-C module (with protection) | Charges and protects the battery |
| MT3608 boost module | 5 V for the amp (set to 5.0 V before connecting!) |
| MCP1825S-3302E/AB (TO-220) | 3.3 V regulator for the logic |
| 2× EC11 rotary encoders | Volume and track controls |
| USB-C (J16) | Programming and Serial Monitor only |
| 3.7 V 2000 mAh LiPo (Adafruit #2011) | Power |

Everything except the ESP32 module is through-hole or a module on headers, so it's hand-solderable.

### Pin map

| Function | GPIO |
|---|---|
| I2S BCLK / LRC / DIN | 5 / 6 / 7 |
| Amp enable (SD_MODE) | 8 |
| RC522 RST / SS / MOSI / SCK / MISO | 9 / 10 / 11 / 12 / 13 |
| SD CMD / CLK / D0 / D1 / D2 / D3 | 14 / 39 / 40 / 41 / 42 / 21 |
| Volume encoder A / B / push | 15 / 16 / 17 |
| Track encoder A / B / push | 18 / 4 / 2 |
| Battery sense (100k/100k divider) | 1 |
| USB D− / D+ | 19 / 20 |

## Repository contents

| Path | What's in it |
|---|---|
| `DiyYoto/DiyYoto.ino` | Arduino firmware |
| `DiyYoto/README.md` | Full firmware setup, SD card layout, controls and troubleshooting |
| `DiyYoto.pretty/` | Custom KiCad footprints for the four modules |
| `make_footprints.py` | Regenerates those footprints |

## Quick start

1. **Build the board.** Order the PCB, solder the parts, and set the MT3608 to 5.0 V with a multimeter *before* plugging in the amp.
2. **Flash the firmware.** In Arduino IDE 2, install the ESP32 core plus the MFRC522 and ESP32-audioI2S libraries. Set Board to *ESP32S3 Dev Module*, USB CDC On Boot to *Enabled*, and PSRAM to match your module. Switch the player on, plug USB into J16, hold BOOT, tap RESET, release BOOT, then upload.
3. **Prepare the SD card.** Format it FAT32 and create a `/cards` folder.
4. **Add a card.** Tap a blank card. The player creates `/cards/<card UID>/`. Copy MP3s into that folder (they play in alphabetical order), put the SD card back, and tap again.

See `DiyYoto/README.md` for the full details.

## Safety notes

- Check the battery's red wire lines up with **+** on J3 before plugging in; some LiPos are wired backwards.
- Mount the battery where it can't be bent or punctured.
- Never connect either speaker wire to GND; the amp output is bridged.
