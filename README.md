# PharmaTrack Sensor

Monitor de temperatura y humedad para farmacia basado en ESP32 + AHT10. Lee las condiciones ambientales cada 30 segundos y las envía a una API REST.

## Hardware

| Componente | Descripción |
|------------|-------------|
| ESP32      | Microcontrolador principal |
| AHT10      | Sensor de temperatura y humedad (I²C) |

**Cableado AHT10:**

| AHT10 | ESP32 |
|-------|-------|
| SDA   | GPIO 8 |
| SCL   | GPIO 9 |
| VCC   | 3.3V  |
| GND   | GND   |

## Configuración WiFi

En el primer arranque (o luego de un reset), el ESP32 crea un punto de acceso llamado **`PharmaTrack-Sensor`**. Conectarse a esa red y abrir el portal cautivo para ingresar las credenciales WiFi. El portal se cierra automáticamente a los 3 minutos.

Para borrar las credenciales guardadas, mantener presionado el botón **BOOT (GPIO 0)** durante 3 segundos.

## Librerías necesarias

Instalar desde el Library Manager de Arduino IDE:

- `ArduinoJson` — Benoit Blanchon
- `Adafruit AHTX0`
- `WiFiManager` — tzapu
- `Adafruit BusIO` (dependencia de AHTX0)

## Compilar y subir (arduino-cli)

```bash
# Compilar
arduino-cli compile --fqbn esp32:esp32:esp32 ProyectoTesinaESP32.ino

# Subir
arduino-cli upload -p /dev/cu.usbserial-* --fqbn esp32:esp32:esp32 ProyectoTesinaESP32.ino

# Monitor serial (115200 baud)
arduino-cli monitor -p /dev/cu.usbserial-* --config baudrate=115200
```

## Constantes configurables

| Constante | Valor por defecto | Descripción |
|-----------|-------------------|-------------|
| `API_URL` | `https://api.farmaciaselene.com/api/v1/sensor-readings/` | Endpoint de la API |
| `DEVICE_ID` | `esp32-farmacia-01` | Identificador del dispositivo |
| `INTERVALO_MS` | `30000` | Intervalo de envío en ms |
