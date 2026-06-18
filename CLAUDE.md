# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

**PharmaTrack Sensor v2.0** — Firmware para ESP32-C5 que lee temperatura y humedad desde un sensor AHT10 (I2C) y envia las lecturas cada 30 segundos a una API REST via HTTPS. Muestra el estado en 3 pantallas OLED SSD1306. Disenado para monitoreo ambiental en farmacias.

## Build & Flash

```bash
# Compilar
arduino-cli compile --fqbn esp32:esp32:esp32c5 pharmatrack-sensor.ino

# Subir al ESP32-C5
arduino-cli upload -p COM3 --fqbn esp32:esp32:esp32c5 pharmatrack-sensor.ino

# Monitor serial (115200 baud)
arduino-cli monitor -p COM3 --config baudrate=115200
```

Board package requerido: `esp32:esp32` (Espressif Systems).
```bash
arduino-cli core install esp32:esp32
```

## Librerias requeridas

Instalar desde Arduino Library Manager o con `arduino-cli lib install`:

| Libreria | Autor |
|----------|-------|
| `ArduinoJson` | Benoit Blanchon |
| `Adafruit AHTX0` | Adafruit |
| `WiFiManager` | tzapu |
| `Adafruit BusIO` | Adafruit (dependencia de AHTX0) |
| `U8g2` | Oliver Kraus (displays OLED SSD1306) |

## Hardware

**AHT10 → ESP32 (I2C bus 0, Wire):**

| AHT10 | ESP32  |
|-------|--------|
| SDA   | GPIO 8 |
| SCL   | GPIO 9 |
| VCC   | 3.3V   |
| GND   | GND    |

**3x SSD1306 OLED 128x64 — cada display en su propio bus I2C:**

| Display   | Bus             | SDA     | SCL     | Muestra         |
|-----------|-----------------|---------|---------|-----------------|
| dispTemp  | Software I2C    | GPIO 25 | GPIO 26 | Temperatura     |
| dispHum   | Software I2C    | GPIO 6  | GPIO 7  | Humedad         |
| dispConex | Software I2C    | GPIO 4  | GPIO 5  | Estado red/API  |

Los 3 displays tienen direccion I2C `0x3C`. Al estar en buses separados no colisionan.
El AHT10 usa Wire (GPIO 8/9) en forma exclusiva, sin compartir bus con ningun display.

`RESET_PIN = GPIO 0` (boton BOOT integrado en la placa).

## Arquitectura

Archivo unico `pharmatrack-sensor.ino` con cuatro responsabilidades:

1. **Provisioning WiFi** — En el primer arranque (o tras reset), abre un AP llamado `"PharmaTrack-Sensor"` mediante `WiFiManager`. El usuario configura la red via portal cautivo. El portal se cierra a los 3 minutos (`setConfigPortalTimeout(180)`).

2. **Lectura del sensor** — `loop()` llama a `aht.getEvent()` cada `INTERVALO_MS` (30 s). Las lecturas fuera de rango (`temp < -10` o `> 85 C`; `hum < 0` o `> 100 %`) se descartan sin enviar.

3. **POST a la API** — `enviarLectura()` serializa `{temperature, humidity, device_id}` como JSON y hace POST sobre HTTPS con `WiFiClientSecure` + `setInsecure()` (cert. deshabilitado). Actualiza el flag `apiOk` y refresca `dispConex` tras cada envio.

4. **Displays OLED** — `mostrarTemperatura()`, `mostrarHumedad()`, `mostrarConexion()` gestionan cada pantalla con U8g2 (clearBuffer → dibujar → sendBuffer). `centrar()` calcula el offset X para texto centrado en 128 px. `mensaje()` es un splash reutilizado en boot y errores.

**Reset de credenciales WiFi:** mantener BOOT (GPIO 0) presionado 3 s llama a `wm.resetSettings()` + `ESP.restart()`, volviendo al modo de provisioning.

## Constantes configurables

| Constante     | Valor por defecto                                              | Descripcion             |
|---------------|----------------------------------------------------------------|-------------------------|
| `API_URL`     | `https://api.farmaciaselene.com/api/v1/sensor-readings/`      | Endpoint de la API      |
| `DEVICE_ID`   | `"esp32-farmacia-01"`                                          | ID del dispositivo      |
| `INTERVALO_MS`| `30000`                                                        | Intervalo de muestreo   |

## Archivos ignorados por git

- `secrets.h` / `config_local.h` — credenciales o configuracion local sensible
- `build/`, `*.bin`, `*.elf`, `*.map`, `*.hex` — artefactos de compilacion
