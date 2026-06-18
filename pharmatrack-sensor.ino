#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_AHTX0.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <U8g2lib.h>

// ── Configuración ──────────────────────────────────────────────
const char* API_URL   = "https://api.farmaciaselene.com/api/v1/sensor-readings/";
const char* DEVICE_ID = "esp32-farmacia-01";
const unsigned long INTERVALO_MS = 30000;

// ── Pines ──────────────────────────────────────────────────────
// Wire (HW I2C 0): AHT10 unicamente
#define SDA_PIN   8
#define SCL_PIN   9
// Display Temperatura — Software I2C
#define SDA0_PIN  25
#define SCL0_PIN  26
// Display Humedad — HW I2C bus 1 (Wire1)
#define SDA1_PIN  6
#define SCL1_PIN  7
// Display Conexion — Software I2C
#define SDA2_PIN  4
#define SCL2_PIN  5
// Boton BOOT
#define RESET_PIN 0

// ── Displays SH1106 128x64 (U8g2) ─────────────────────────────
U8G2_SH1106_128X64_NONAME_F_SW_I2C      dispTemp (U8G2_R0, SCL0_PIN, SDA0_PIN, U8X8_PIN_NONE);
U8G2_SH1106_128X64_NONAME_F_2ND_HW_I2C  dispHum  (U8G2_R0, U8X8_PIN_NONE);
U8G2_SH1106_128X64_NONAME_F_SW_I2C      dispConex(U8G2_R0, SCL2_PIN, SDA2_PIN, U8X8_PIN_NONE);

// ── Objetos ────────────────────────────────────────────────────
Adafruit_AHTX0 aht;
WiFiManager    wm;
bool apiOk = false;

// ── Helpers de display ─────────────────────────────────────────

void centrar(U8G2 &d, int y, const char* s) {
  d.drawStr((128 - d.getStrWidth(s)) / 2, y, s);
}

// Splash generico — 1 o 2 lineas centradas
void mensaje(U8G2 &d, const char* l1, const char* l2 = nullptr) {
  d.clearBuffer();
  d.setFont(u8g2_font_6x10_tf);
  centrar(d, l2 ? 28 : 35, l1);
  if (l2) centrar(d, 42, l2);
  d.sendBuffer();
}

/*
  Layout dispTemp / dispHum (128x64):
  ┌──────────────────────────┐
  │      TEMP (C)            │  6x10, y=10
  │──────────────────────────│  linea y=13
  │                          │
  │         23.5             │  logisoso32, y=54
  │                          │
  └──────────────────────────┘
*/
void mostrarTemperatura(float val) {
  char num[8];
  dtostrf(val, 5, 1, num);

  dispTemp.clearBuffer();
  dispTemp.setFont(u8g2_font_6x10_tf);
  centrar(dispTemp, 10, "TEMP (C)");
  dispTemp.drawHLine(0, 13, 128);
  dispTemp.setFont(u8g2_font_logisoso32_tn);
  centrar(dispTemp, 54, num);
  dispTemp.sendBuffer();
}

void mostrarHumedad(float val) {
  char num[8];
  dtostrf(val, 5, 1, num);

  dispHum.clearBuffer();
  dispHum.setFont(u8g2_font_6x10_tf);
  centrar(dispHum, 10, "HUMEDAD (%)");
  dispHum.drawHLine(0, 13, 128);
  dispHum.setFont(u8g2_font_logisoso32_tn);
  centrar(dispHum, 54, num);
  dispHum.sendBuffer();
}

/*
  Layout dispConex (128x64):
  ┌──────────────────────────┐
  │           RED            │  y=10
  │──────────────────────────│  y=13
  │ WiFi: OK                 │  y=27
  │ 192.168.x.x              │  y=39
  │ API:  OK                 │  y=53
  └──────────────────────────┘
*/
void mostrarConexion() {
  bool wifiOk = (WiFi.status() == WL_CONNECTED);

  dispConex.clearBuffer();
  dispConex.setFont(u8g2_font_6x10_tf);
  centrar(dispConex, 10, "RED");
  dispConex.drawHLine(0, 13, 128);

  dispConex.drawStr(0, 27, "WiFi:");
  dispConex.drawStr(38, 27, wifiOk ? "OK" : "---");
  if (wifiOk)
    dispConex.drawStr(0, 39, WiFi.localIP().toString().c_str());

  dispConex.drawStr(0, 53, "API: ");
  dispConex.drawStr(38, 53, apiOk ? "OK" : "---");

  dispConex.sendBuffer();
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

  // Pequeña espera para que los displays arranquen tras el encendido
  delay(500);

  // Inicializar los 3 displays
  Wire1.begin(SDA1_PIN, SCL1_PIN);

  Serial.println("Iniciando dispTemp (GPIO25/26)...");
  dispTemp.begin();
  mensaje(dispTemp, "PharmaTrack", "Sensor v2.0");
  Serial.println("dispTemp OK");

  Serial.println("Iniciando dispHum (GPIO6/7)...");
  dispHum.begin();
  mensaje(dispHum, "PharmaTrack", "Sensor v2.0");
  Serial.println("dispHum OK");

  Serial.println("Iniciando dispConex (GPIO4/5)...");
  dispConex.begin();
  mensaje(dispConex, "Conectando", "al WiFi...");
  Serial.println("dispConex OK");

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
