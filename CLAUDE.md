# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**PharmaTrack Sensor v2.0** — An ESP32-based IoT device that reads temperature and humidity from an AHT10 sensor and POSTs readings every 30 seconds to a remote REST API (`https://api.farmaciaselene.com/api/v1/sensor-readings/`). Designed for pharmacy environment monitoring.

## Build & Flash (Arduino IDE or arduino-cli)

```bash
# Compile (arduino-cli)
arduino-cli compile --fqbn esp32:esp32:esp32 ProyectoTesinaESP32.ino

# Upload to connected ESP32
arduino-cli upload -p /dev/cu.usbserial-* --fqbn esp32:esp32:esp32 ProyectoTesinaESP32.ino

# Monitor serial output (115200 baud)
arduino-cli monitor -p /dev/cu.usbserial-* --config baudrate=115200
```

Required board package: `esp32:esp32` (Espressif Systems).

## Required Libraries

Install via Arduino Library Manager or `arduino-cli lib install`:

- `ArduinoJson` (Benoit Blanchon)
- `Adafruit AHTX0`
- `WiFiManager` (tzapu/WiFiManager)
- `Adafruit BusIO` (dependency of AHTX0)

## Hardware Wiring

| AHT10 Pin | ESP32 GPIO |
|-----------|------------|
| SDA       | GPIO 8     |
| SCL       | GPIO 9     |
| VCC       | 3.3V       |
| GND       | GND        |

RESET_PIN = GPIO 0 (BOOT button, built-in).

## Architecture

Single `.ino` file with three main responsibilities:

1. **WiFi provisioning** — Uses `WiFiManager` (captive-portal AP `"PharmaTrack-Sensor"`) on first boot or after credentials reset. Portal auto-closes after 3 minutes.
2. **Sensor polling** — `loop()` reads the AHT10 every `INTERVALO_MS` (30 s) and discards out-of-range values (temp: −10…85 °C, humidity: 0…100 %).
3. **API POST** — `enviarLectura()` serializes `{temperature, humidity, device_id}` as JSON and POSTs over HTTPS (certificate verification disabled via `setInsecure()`).

**WiFi reset flow**: holding the BOOT button (GPIO 0) for 3 seconds calls `wm.resetSettings()` + `ESP.restart()`, clearing saved credentials and re-entering provisioning mode.

## Key Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| `API_URL` | `https://api.farmaciaselene.com/api/v1/sensor-readings` | Backend endpoint |
| `DEVICE_ID` | `"esp32-farmacia-01"` | Identifies this unit in the API |
| `INTERVALO_MS` | `30000` | Polling interval (ms) |
