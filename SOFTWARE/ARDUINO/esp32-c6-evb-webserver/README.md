# OLIMEX ESP32-C6-EVB Wi-Fi Web Server Demo

Arduino IDE demo for controlling and monitoring an OLIMEX ESP32-C6-EVB over Wi-Fi.

## Features

- Independent ON/OFF buttons for all four relays plus all-on/all-off controls
- Live relay state and relay indicator state (the physical relay LEDs follow the relay drive signals)
- Live status for all four optocoupler inputs
- User LED status and control
- Button 1 sequence: Relay 1, Relay 2, Relay 3, Relay 4, all ON, all OFF, repeat
- UEXT UART terminal, I²C scanner, and SPI hexadecimal transaction tool
- Responsive web page with automatic status refresh
- Fast 200 ms live-status refresh with overlapping-request protection
- Station-mode Wi-Fi with automatic access-point fallback
- No third-party Arduino libraries

## Configure and upload

1. Open `ESP32-C6-EVB-WebServer.ino` in Arduino IDE.
2. To join an existing 2.4 GHz network, enter `WIFI_SSID` and `WIFI_PASSWORD` near the top of the sketch. They are empty in the released source; leave them empty to use access-point mode.
3. Select **OLIMEX ESP32-C6-EVB** as the board.
4. Use the tested settings from Olimex: 921600 upload speed, USB CDC On Boot enabled, 160 MHz CPU, 80 MHz flash, QIO flash mode, 4 MB flash, default 4 MB/SPIFFS partition, Core Debug Level none, Erase All Flash disabled, JTAG disabled, Zigbee disabled.
5. Upload, then open Serial Monitor at 115200 baud. Browse to the printed IP address or `http://esp32-c6-evb.local`.

### Finding the web-page address

Open **Tools → Serial Monitor**, select the board's USB COM port, and set the speed to **115200 baud**. Keep **USB CDC On Boot** enabled. At startup, and again every 30 seconds, the board prints a block similar to:

```text
========== WEB PAGE ==========
Mode: Connected to Wi-Fi router
Network: your-network
Open in browser: http://192.168.1.123/
mDNS address:    http://esp32-c6-evb.local/
Serial speed: 115200 baud
==============================
```

If the board starts its fallback access point, the serial output tells you to connect to **OLIMEX-C6-EVB** and open `http://192.168.4.1/`. Web relay and LED operations are also logged over USB serial.

If the SSID is empty, or station connection fails for 15 seconds, connect a phone/computer to the open **OLIMEX-C6-EVB** access point and browse to `http://192.168.4.1/`.

### mDNS name

The board advertises the hostname **`esp32-c6-evb.local`**, so the normal local-network URL is:

`http://esp32-c6-evb.local/`

mDNS must be supported by the computer or phone, and both devices must be on the same local network. If the name does not resolve, use the numeric IP address printed over USB serial. In fallback access-point mode, `http://192.168.4.1/` is the most reliable address.

## Hardware behavior

| Function | GPIO | Logic |
|---|---:|---|
| Relay 1–4 | 10, 11, 22, 23 | Active high |
| User LED | 8 | Active low |
| Button 1 | 9 | Active low, internal pull-up |
| Optocouplers 1–4 | 1, 2, 3, 15 | Active low, board pull-ups |

## UEXT diagnostics

The web page follows the standard 10-pin OLIMEX UEXT assignment and the board schematic:

| UEXT pin | Signal | ESP32-C6 GPIO | Web function |
|---:|---|---:|---|
| 1 | +3.3 V | — | Power |
| 2 | GND | — | Ground |
| 3 | UART TX | 5 | Send text at 115200 baud, 8N1 |
| 4 | UART RX | 4 | Live receive terminal |
| 5 | I²C SCL | 7 | 100 kHz address scanner |
| 6 | I²C SDA | 6 | 100 kHz address scanner |
| 7 | SPI MISO | 20 | Received transaction data |
| 8 | SPI MOSI | 18 | Transmitted transaction data |
| 9 | SPI CLK | 19 | 1 MHz, mode 0 clock |
| 10 | SPI CS | 21 | Active-low chip select |

SPI transfers accept 1–32 hexadecimal bytes, for example `9F 00 00 00`. The CS line is held low for the entire transaction. UEXT signals use **3.3 V logic**; do not apply 5 V logic to the ESP32-C6 pins.

Relay state is the commanded output state; the board has no relay-contact feedback input. The relay LED display therefore mirrors that same drive signal.

## Safety

The optocoupler inputs are intended for 5–30 V DC and are polarity-sensitive. That voltage range applies only to the isolated inputs, not UEXT. Treat relay contacts and connected loads according to their actual voltage and hazard class. Do not handle energized wiring.

This demonstration has no user authentication. The fallback access point is open, and anyone connected to the same network can operate the outputs. Use it only on a trusted test network, particularly when loads are attached to the relay contacts.

## Reference

Pin assignments and active levels were verified against Olimex's working Arduino demos at commit `7b66d23cabba3c573a1e63d83363675b5fb74f06`. See `REFERENCE_ANALYSIS.md`.

License: GPL-3.0-or-later.
