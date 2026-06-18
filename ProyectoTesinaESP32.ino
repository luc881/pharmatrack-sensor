#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_AHTX0.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>

// ── Configuración ──────────────────────────────────────────────
const char* API_URL   = "https://api.farmaciaselene.com/api/v1/sensor-readings/";
const char* DEVICE_ID = "esp32-farmacia-01";
const unsigned long INTERVALO_MS = 30000;

// ── Pines ──────────────────────────────────────────────────────
// Bus 0 (Wire, HW I2C): AHT10 (0x38) + dispTemp (0x3C) — no colisionan
#define SDA_PIN   8
#define SCL_PIN   9
// Display Humedad — Software I2C bit-bang
#define SDA1_PIN  6
#define SCL1_PIN  7
// Display Conexion — Software I2C bit-bang
#define SDA2_PIN  4
#define SCL2_PIN  5
// Boton BOOT
#define RESET_PIN 0

// ── Displays SH1106 128x64 ─────────────────────────────────────
Adafruit_SH1106 dispTemp (-1);                       // HW I2C (Wire)
Adafruit_SH1106 dispHum  (SDA1_PIN, SCL1_PIN, -1);  // SW I2C
Adafruit_SH1106 dispConex(SDA2_PIN, SCL2_PIN, -1);  // SW I2C

// ── Objetos ────────────────────────────────────────────────────
Adafruit_AHTX0 aht;
WiFiManager    wm;
bool apiOk = false;

// ── Helpers de display ─────────────────────────────────────────

// Escribe 'texto' centrado horizontalmente en la fila y, con el tamaño dado
void centrar(Adafruit_SH1106 &d, int16_t y, const char* texto, uint8_t size = 1) {
  d.setTextSize(size);
  int16_t x1, y1;
  uint16_t w, h;
  d.getTextBounds(texto, 0, 0, &x1, &y1, &w, &h);
  d.setCursor((128 - (int16_t)w) / 2, y);
  d.print(texto);
}

// Splash generico — 1 o 2 lineas centradas
void mensaje(Adafruit_SH1106 &d, const char* l1, const char* l2 = nullptr) {
  d.clearDisplay();
  d.setTextColor(WHITE);
  centrar(d, l2 ? 22 : 28, l1);
  if (l2) centrar(d, 38, l2);
  d.display();
}

/*
  Layout dispTemp / dispHum  (128x64):
  ┌──────────────────────────┐
  │      TEMP (C)            │  size=1, y=0
  │──────────────────────────│  linea y=10
  │                          │
  │         23.5             │  size=3 (18x24 px/char), y=18
  │                          │
  └──────────────────────────┘
*/
void mostrarTemperatura(float val) {
  char num[8];
  dtostrf(val, 5, 1, num);

  dispTemp.clearDisplay();
  dispTemp.setTextColor(WHITE);
  centrar(dispTemp, 0, "TEMP (C)");
  dispTemp.drawLine(0, 10, 127, 10, WHITE);
  centrar(dispTemp, 18, num, 3);
  dispTemp.display();
}

void mostrarHumedad(float val) {
  char num[8];
  dtostrf(val, 5, 1, num);

  dispHum.clearDisplay();
  dispHum.setTextColor(WHITE);
  centrar(dispHum, 0, "HUMEDAD (%)");
  dispHum.drawLine(0, 10, 127, 10, WHITE);
  centrar(dispHum, 18, num, 3);
  dispHum.display();
}

/*
  Layout dispConex (128x64):
  ┌──────────────────────────┐
  │           RED            │  y=0
  │──────────────────────────│  y=10
  │ WiFi: OK                 │  y=16
  │ 192.168.x.x              │  y=30
  │ API:  OK                 │  y=46
  └──────────────────────────┘
*/
void mostrarConexion() {
  bool wifiOk = (WiFi.status() == WL_CONNECTED);

  dispConex.clearDisplay();
  dispConex.setTextColor(WHITE);
  dispConex.setTextSize(1);

  centrar(dispConex, 0, "RED");
  dispConex.drawLine(0, 10, 127, 10, WHITE);

  dispConex.setCursor(0, 16);
  dispConex.print("WiFi: ");
  dispConex.println(wifiOk ? "OK" : "---");

  if (wifiOk) {
    dispConex.setCursor(0, 30);
    dispConex.println(WiFi.localIP().toString());
  }

  dispConex.setCursor(0, 46);
  dispConex.print("API:  ");
  dispConex.println(apiOk ? "OK" : "---");

  dispConex.display();
}

// ── Reset WiFi (BOOT 3 seg) ────────────────────────────────────
void checkResetButton() {
  if (digitalRead(RESET_PIN) == LOW) {
    Serial.println("BOOT presionado — esperando 3 s...");
    delay(3000);
    if (digitalRead(RESET_PIN) == LOW) {
      Serial.println("Borrando credenciales WiFi...");
      wm.resetSettings();
      delay(500);
      ESP.restart();
    }
  }
}

// ── POST a la API ──────────────────────────────────────────────
void enviarLectura(float temp, float hum) {
  if (WiFi.status() != WL_CONNECTED) return;

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, API_URL);
  http.addHeader("Content-Type", "application/json");

  JsonDocument doc;
  doc["temperature"] = round(temp * 10.0) / 10.0;
  doc["humidity"]    = round(hum  * 10.0) / 10.0;
  doc["device_id"]   = DEVICE_ID;
  String body;
  serializeJson(doc, body);

  Serial.printf("Enviando -> %.1f C  %.1f%%  ", temp, hum);
  int codigo = http.POST(body);

  apiOk = (codigo == 200 || codigo == 201);
  if (apiOk)           Serial.println("OK");
  else if (codigo > 0) Serial.printf("HTTP %d: %s\n", codigo, http.getString().c_str());
  else                 Serial.printf("Error: %s\n", http.errorToString(codigo).c_str());

  http.end();
  mostrarConexion();
}

// ── Setup ──────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n-- PharmaTrack Sensor v2.0 --");

  pinMode(RESET_PIN, INPUT_PULLUP);
  checkResetButton();

  // Wire para AHT10 + dispTemp (hardware I2C, mismos pines)
  Wire.begin(SDA_PIN, SCL_PIN);

  // Inicializar los 3 displays
  dispTemp.begin(SH1106_SWITCHCAPVCC, 0x3C);
  dispHum.begin(SH1106_SWITCHCAPVCC, 0x3C);
  dispConex.begin(SH1106_SWITCHCAPVCC, 0x3C);

  mensaje(dispTemp,  "PharmaTrack", "Sensor v2.0");
  mensaje(dispHum,   "PharmaTrack", "Sensor v2.0");
  mensaje(dispConex, "Conectando", "al WiFi...");

  // AHT10 comparte Wire con dispTemp; 0x38 != 0x3C, sin conflicto
  if (!aht.begin()) {
    Serial.println("ERROR: AHT10 no detectado. SDA=GPIO8  SCL=GPIO9");
    mensaje(dispTemp, "ERROR AHT10", "revisa cableado");
    while (true) delay(1000);
  }
  Serial.println("AHT10 OK");

  wm.setConfigPortalTimeout(180);
  wm.setConnectTimeout(30);
  wm.setAPClientCheck(true);
  wm.setAPCallback([](WiFiManager*) {
    mensaje(dispConex, "AP abierto:", "PharmaTrack-Sensor");
  });

  Serial.println("Conectando al WiFi...");
  if (!wm.autoConnect("PharmaTrack-Sensor")) {
    Serial.println("Sin WiFi. Reiniciando...");
    mensaje(dispConex, "Sin WiFi", "Reiniciando...");
    delay(3000);
    ESP.restart();
  }

  Serial.printf("Conectado -- IP: %s\n", WiFi.localIP().toString().c_str());
  mostrarConexion();
}

// ── Loop ───────────────────────────────────────────────────────
unsigned long ultimoEnvio = 0;

void loop() {
  checkResetButton();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi perdido -- reconectando...");
    mostrarConexion();
    WiFi.reconnect();
    delay(5000);
    return;
  }

  unsigned long ahora = millis();
  if (ahora - ultimoEnvio >= INTERVALO_MS) {
    ultimoEnvio = ahora;

    sensors_event_t evHum, evTemp;
    aht.getEvent(&evHum, &evTemp);

    float temp = evTemp.temperature;
    float hum  = evHum.relative_humidity;

    if (temp > -10 && temp < 85 && hum >= 0 && hum <= 100) {
      mostrarTemperatura(temp);
      mostrarHumedad(hum);
      enviarLectura(temp, hum);
    } else {
      Serial.printf("Lectura invalida descartada: %.1f C  %.1f%%\n", temp, hum);
    }
  }
}
