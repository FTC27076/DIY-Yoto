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
## BILL OF MATERIALS

Costs are in USD for one player. Most unit prices are estimates, so check them in each cart.

| Item | Qty | Supplier | Cost/item | Total Cost | link |
|---|---:|---|---:|---:|---|
| U2 – ESP32-S3-WROOM-1-N8R8 module (Espressif) | 1 | DigiKey | $4.50 | $4.50 | [link](https://www.digikey.ca/en/products/detail/espressif-systems/ESP32-S3-WROOM-1-N8R8/15295891) |
| U1 – MCP1825S-3302E/AB 3.3V LDO, TO-220 (Microchip) | 1 | DigiKey | $1.75 | $1.75 | [link](https://www.digikey.ca/en/products/result?keywords=MCP1825S-3302E%2FAB) |
| R1, R2 – 100k 1/4W resistor (Stackpole CF14JT100K, cut tape) | 2 | DigiKey | $0.10 | $0.20 | [link](https://www.digikey.ca/en/products/result?keywords=CF14JT100K) |
| R3–R8 – 10k 1/4W resistor (Stackpole CF14JT10K0, cut tape) | 6 | DigiKey | $0.10 | $0.60 | [link](https://www.digikey.ca/en/products/result?keywords=CF14JT10K0) |
| R9, R10 – 5.1k 1/4W resistor (Stackpole CF14JT5K10, cut tape) | 2 | DigiKey | $0.10 | $0.20 | [link](https://www.digikey.ca/en/products/detail/stackpole-electronics-inc/CF14JT5K10/1830367) |
| C1, C5 – 100µF 16V electrolytic (Panasonic EEU-FR1C101) | 2 | DigiKey | $0.40 | $0.80 | [link](https://www.digikey.ca/en/products/detail/panasonic-industry/EEU-FR1C101/2433525) |
| C2, C3, C9 – 10µF 25V ceramic, 5mm pitch (TDK FG28X5R1E106MRT06) | 3 | LCSC / JLC | $0.20 | $0.60 | [link](https://www.lcsc.com/product-detail/C1620216.html) |
| C4, C6, C7, C10 – 100nF 50V ceramic (KNSCHA 178MU0006) | 4 | LCSC / JLC | $0.03 | $0.12 | [link](https://www.lcsc.com/product-detail/C5632430.html) |
| C8 – 1µF 50V ceramic (KNSCHA 178MU0010) | 1 | LCSC / JLC | $0.05 | $0.05 | [link](https://www.lcsc.com/product-detail/C5632434.html) |
| SW2, SW3 – 6x6mm tactile switch, reset/boot (Omron B3F-1000) | 2 | DigiKey | $0.35 | $0.70 | [link](https://www.digikey.ca/en/products/detail/omron-electronics-inc-emc-div/B3F-1000/33150) |
| SW4, SW5 – rotary encoder with switch (Alps EC11E15244G1) | 2 | LCSC / JLC | $1.50 | $3.00 | [link](https://www.lcsc.com/product-detail/C370970.html) |
| J16 – USB-C receptacle, 16-pin (HRO TYPE-C-31-M-12) | 1 | LCSC / JLC | $0.25 | $0.25 | [link](https://www.lcsc.com/product-detail/C165948.html) |
| J3 – JST-PH 2-pin side-entry socket, battery (JST S2B-PH-K-S) | 1 | LCSC / JLC | $0.10 | $0.10 | [link](https://www.lcsc.com/product-detail/C173752.html) |
| J7 – JST-XH 2-pin socket, power switch (JST B2B-XH-A) | 1 | LCSC / JLC | $0.05 | $0.05 | [link](https://www.lcsc.com/product-detail/C158012.html) |
| MOD1 – 1x8 2.54mm female header (BOOMELE) | 1 | LCSC / JLC | $0.10 | $0.10 | [link](https://www.lcsc.com/product-detail/C27438.html) |
| J12 (M1) – TP4056 USB-C charger module with protection (pack of 5) | 1 | Amazon | $8.00 | $8.00 | [link](https://www.amazon.ca/s?k=TP4056+type-c+protection+module) |
| J13 (M2) – MT3608 boost module (pack of 5) | 1 | Amazon | $8.00 | $8.00 | [link](https://www.amazon.ca/s?k=MT3608+boost+module) |
| J14 (M3) – Adafruit MAX98357A I2S amp breakout (3006) | 1 | DigiKey | $5.95 | $5.95 | [link](https://www.digikey.ca/en/products/detail/adafruit-industries-llc/3006/6058477) |
| J15 (M4) – Adafruit micro SD SPI/SDIO breakout (4682) | 1 | DigiKey | $3.50 | $3.50 | [link](https://www.digikey.ca/en/products/detail/adafruit-industries-llc/4682/12822319) |
| MOD1 – RC522 RFID reader kit with card + fob (ProtoSupplies) | 1 | ProtoSupplies | $2.59 | $2.59 | [link](https://protosupplies.com/product/mfrc522-rfid-reader-key-card-and-key-ring/) |
| Header pins for M1/M2 – 1x40 break-away male header (BOOMELE) | 1 | LCSC / JLC | $0.15 | $0.15 | [link](https://www.lcsc.com/product-detail/C2337.html) |
| LiPo battery 3.7V 2000mAh with JST-PH (Adafruit 2011) | 1 | DigiKey | $12.50 | $12.50 | [link](https://www.digikey.ca/en/products/detail/adafruit-industries-llc/2011/6612469) |
| Speaker 40mm 4Ω 3W (Adafruit 3968) | 1 | DigiKey | $4.95 | $4.95 | [link](https://www.digikey.ca/en/products/detail/adafruit-industries-llc/3968/9745251) |
| Power switch – SPDT panel toggle 5A (E-Switch 100SP1T1B1M2QEH) | 1 | DigiKey | $4.50 | $4.50 | [link](https://www.digikey.ca/en/products/detail/e-switch/100SP1T1B1M2QEH/378821) |
| RFID cards – MIFARE Classic 1K (Adafruit 359) | 5 | DigiKey | $1.95 | $9.75 | [link](https://www.digikey.ca/en/products/detail/adafruit-industries-llc/359/5353589) |
| micro SD card 16–32GB (SanDisk / Samsung) | 1 | Amazon | $8.00 | $8.00 | [link](https://www.amazon.ca/s?k=sandisk+32gb+microsd) |
| Optional – JST-XH plug housing for J7 (JST XHP-2) | 0 | DigiKey | $0.10 | $0.00 | [link](https://www.digikey.ca/en/products/detail/jst-sales-america-inc/XHP-2/555485) |
| Optional – JST-XH crimp contacts (JST SXH-001T-P0.6, cut tape) | 0 | DigiKey | $0.10 | $0.00 | [link](https://www.digikey.ca/en/products/detail/jst-sales-america-inc/SXH-001T-P0.6/455-1135-1-ND/527370) |
| Parts subtotal | | | | $80.91 | |
| Tax (Ontario HST 13%) | | | | $13.77 | |
| **Grand total (USD)** | | | | **~$119.68** | |

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



## Images of final design

<img width="832" height="519" alt="Screenshot 2026-11-05 at 8 46 17 PM" src="https://github.com/user-attachments/assets/402a6c26-89a9-4785-9282-88bedbef2d59" />
<img width="852" height="801" alt="Screenshot 2026-11-05 at 8 45 57 PM" src="https://github.com/user-attachments/assets/39659a93-bb64-4700-aa51-71a6b49c6291" />
