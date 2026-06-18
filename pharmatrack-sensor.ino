#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_AHTX0.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Pines I2C por dispositivo
#define SDA_TEMP   25
#define SCL_TEMP   26
#define SDA_HUM     2
#define SCL_HUM     3
#define SDA_CONEX   4
#define SCL_CONEX   5
#define SDA_AHT     8
#define SCL_AHT     9
#define RESET_PIN   0

const char*         API_URL      = "https://api.farmaciaselene.com/api/v1/sensor-readings";
const char*         DEVICE_ID    = "esp32-farmacia-01";
const unsigned long INTERVALO_MS = 30000;

Adafruit_SSD1306 dispTemp (128, 64, &Wire, -1);
Adafruit_SSD1306 dispHum  (128, 64, &Wire, -1);
Adafruit_SSD1306 dispConex(128, 64, &Wire, -1);
Adafruit_AHTX0   aht;

bool wifiOk = false;
bool apiOk  = false;
bool ahtOk  = false;
unsigned long ultimaLectura = 0;

// Offset X para centrar texto (6 px/car × textSize)
int centrar(const char* s, uint8_t sz) {
  return max(0, (128 - (int)strlen(s) * 6 * sz) / 2);
}

void mostrarTemperatura(float t) {
  char buf[10];
  snprintf(buf, sizeof(buf), "%.1fC", t);
  Wire.end(); Wire.begin(SDA_TEMP, SCL_TEMP);
  dispTemp.clearDisplay();
  dispTemp.setTextColor(SSD1306_WHITE);
  dispTemp.setTextSize(1);
  dispTemp.setCursor(centrar("TEMPERATURA", 1), 2);
  dispTemp.print("TEMPERATURA");
  dispTemp.setTextSize(3);
  dispTemp.setCursor(centrar(buf, 3), 24);
  dispTemp.print(buf);
  dispTemp.display();
}

void mostrarHumedad(float h) {
  char buf[10];
  snprintf(buf, sizeof(buf), "%.1f%%", h);
  Wire.end(); Wire.begin(SDA_HUM, SCL_HUM);
  dispHum.clearDisplay();
  dispHum.setTextColor(SSD1306_WHITE);
  dispHum.setTextSize(1);
  dispHum.setCursor(centrar("HUMEDAD", 1), 2);
  dispHum.print("HUMEDAD");
  dispHum.setTextSize(3);
  dispHum.setCursor(centrar(buf, 3), 24);
  dispHum.print(buf);
  dispHum.display();
}

void mostrarConexion() {
  Wire.end(); Wire.begin(SDA_CONEX, SCL_CONEX);
  dispConex.clearDisplay();
  dispConex.setTextColor(SSD1306_WHITE);
  dispConex.setTextSize(1);
  dispConex.setCursor(centrar("CONEXION", 1), 2);
  dispConex.print("CONEXION");
  dispConex.setTextSize(2);
  dispConex.setCursor(0, 20);
  dispConex.print("WiFi: ");
  dispConex.println(wifiOk ? "OK" : "--");
  dispConex.print("API:  ");
  dispConex.print(apiOk  ? "OK" : "--");
  dispConex.display();
}

void mensaje(const char* linea) {
  const char* titulo = "PharmaTrack";
  Wire.end(); Wire.begin(SDA_TEMP, SCL_TEMP);
  dispTemp.clearDisplay();
  dispTemp.setTextColor(SSD1306_WHITE);
  dispTemp.setTextSize(1);
  dispTemp.setCursor(centrar(titulo, 1), 10);
  dispTemp.print(titulo);
  dispTemp.setCursor(centrar(linea, 1), 40);
  dispTemp.print(linea);
  dispTemp.display();

  Wire.end(); Wire.begin(SDA_HUM, SCL_HUM);
  dispHum.clearDisplay();
  dispHum.setTextColor(SSD1306_WHITE);
  dispHum.setTextSize(1);
  dispHum.setCursor(centrar(titulo, 1), 10);
  dispHum.print(titulo);
  dispHum.setCursor(centrar(linea, 1), 40);
  dispHum.print(linea);
  dispHum.display();

  Wire.end(); Wire.begin(SDA_CONEX, SCL_CONEX);
  dispConex.clearDisplay();
  dispConex.setTextColor(SSD1306_WHITE);
  dispConex.setTextSize(1);
  dispConex.setCursor(centrar(titulo, 1), 10);
  dispConex.print(titulo);
  dispConex.setCursor(centrar(linea, 1), 40);
  dispConex.print(linea);
  dispConex.display();
}

void enviarLectura(float temp, float hum) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.begin(client, API_URL);
  http.addHeader("Content-Type", "application/json");

  JsonDocument doc;
  doc["temperature"] = temp;
  doc["humidity"]    = hum;
  doc["device_id"]   = DEVICE_ID;
  String payload;
  serializeJson(doc, payload);

  int code = http.POST(payload);
  apiOk = (code >= 200 && code < 300);
  Serial.printf("API response: %d\n", code);
  http.end();
  mostrarConexion();
}

void leerYEnviar() {
  Wire.end(); Wire.begin(SDA_AHT, SCL_AHT);
  sensors_event_t humEv, tempEv;
  aht.getEvent(&humEv, &tempEv);

  float t = tempEv.temperature;
  float h = humEv.relative_humidity;

  if (t < -10 || t > 85 || h < 0 || h > 100) {
    Serial.printf("Fuera de rango: %.1fC %.1f%%\n", t, h);
    return;
  }

  Serial.printf("Temp: %.1fC  Hum: %.1f%%\n", t, h);
  mostrarTemperatura(t);
  mostrarHumedad(h);
  if (wifiOk) enviarLectura(t, h);
}

void setup() {
  Serial.begin(115200);
  pinMode(RESET_PIN, INPUT_PULLUP);

  // Inicializar displays
  Wire.end(); Wire.begin(SDA_TEMP, SCL_TEMP);
  dispTemp.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  Wire.end(); Wire.begin(SDA_HUM, SCL_HUM);
  dispHum.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  Wire.end(); Wire.begin(SDA_CONEX, SCL_CONEX);
  dispConex.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  mensaje("Iniciando...");

  // AHT10
  Wire.end(); Wire.begin(SDA_AHT, SCL_AHT);
  ahtOk = aht.begin(&Wire);
  if (!ahtOk) {
    Serial.println("AHT10 no encontrado");
  }

  // WiFi — muestra estado en dispConex mientras conecta
  Wire.end(); Wire.begin(SDA_CONEX, SCL_CONEX);
  dispConex.clearDisplay();
  dispConex.setTextColor(SSD1306_WHITE);
  dispConex.setTextSize(1);
  dispConex.setCursor(centrar("Conectando WiFi", 1), 28);
  dispConex.print("Conectando WiFi");
  dispConex.display();

  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  wifiOk = wm.autoConnect("PharmaTrack-Sensor");

  if (wifiOk) {
    Serial.println("WiFi OK: " + WiFi.localIP().toString());
  } else {
    Serial.println("WiFi timeout — modo sin red");
  }
  mostrarConexion();

  // Primera lectura inmediata
  if (ahtOk) leerYEnviar();
  ultimaLectura = millis();
}

void loop() {
  // Mantener BOOT 3 s → reset credenciales WiFi
  if (digitalRead(RESET_PIN) == LOW) {
    unsigned long t0 = millis();
    while (digitalRead(RESET_PIN) == LOW) {
      if (millis() - t0 > 3000) {
        WiFiManager wm;
        wm.resetSettings();
        Wire.end(); Wire.begin(SDA_CONEX, SCL_CONEX);
        dispConex.clearDisplay();
        dispConex.setTextColor(SSD1306_WHITE);
        dispConex.setTextSize(2);
        dispConex.setCursor(centrar("RESET WiFi", 2), 20);
        dispConex.print("RESET WiFi");
        dispConex.display();
        delay(1000);
        ESP.restart();
      }
    }
  }

  if (ahtOk && millis() - ultimaLectura >= INTERVALO_MS) {
    ultimaLectura = millis();
    leerYEnviar();
  }
}
