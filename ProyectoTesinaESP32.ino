  #include <Wire.h>                                                                          
  #include <WiFi.h>                      
  #include <HTTPClient.h>                                                                    
  #include <ArduinoJson.h>                                                                   
  #include <Adafruit_AHTX0.h>                                                                
  #include <WiFiClientSecure.h>                                                              
  #include <WiFiManager.h>        // tzapu/WiFiManager
                                                                                             
  // ── Configuración ──────────────────────────────────────────────
  const char* API_URL   = "https://api.farmaciaselene.com/api/v1/sensor-readings";          
  const char* DEVICE_ID = "esp32-farmacia-01";              
                                                                                             
  const unsigned long INTERVALO_MS = 30000;  // 30 segundos
                                                                                             
  // ── Pines ──────────────────────────────────────────────────────                         
  #define SDA_PIN    8
  #define SCL_PIN    9
  #define RESET_PIN  0   // Botón BOOT del ESP32
  #define LED_PIN    2   // LED integrado del ESP32
                                                                                             
  // ── Objetos ────────────────────────────────────────────────────
  Adafruit_AHTX0 aht;                                                                        
  WiFiManager    wm;                                                                         
   
  // ── Resetear credenciales (botón BOOT 3 seg) ───────────────────                         
  void checkResetButton() {                                 
    if (digitalRead(RESET_PIN) == LOW) {                                                     
      Serial.println("Botón BOOT presionado — esperando 3 segundos...");                     
      delay(3000);
      if (digitalRead(RESET_PIN) == LOW) {                                                   
        Serial.println("Borrando credenciales WiFi y reiniciando...");                       
        wm.resetSettings();
        delay(500);                                                                          
        ESP.restart();                                                                       
      }
    }                                                                                        
  }                                                         

  // ── Envío a la API ─────────────────────────────────────────────
  void enviarLectura(float temperatura, float humedad) {
    if (WiFi.status() != WL_CONNECTED) return;                                               
   
    WiFiClientSecure client;                                                                 
    client.setInsecure();  // HTTPS sin verificar certificado
                                                                                             
    HTTPClient http;
    http.begin(client, API_URL);                                                             
    http.addHeader("Content-Type", "application/json");     

    JsonDocument doc;
    doc["temperature"] = round(temperatura * 10.0) / 10.0;
    doc["humidity"]    = round(humedad * 10.0) / 10.0;                                       
    doc["device_id"]   = DEVICE_ID;
                                                                                             
    String body;                                            
    serializeJson(doc, body);

    Serial.printf("Enviando → %.1f°C  %.1f%%  ", temperatura, humedad);                      
   
    int codigo = http.POST(body);                                                            
                                                            
    if (codigo == 201 || codigo == 200) {
      Serial.println("OK");
    } else if (codigo > 0) {                                                                 
      Serial.printf("HTTP %d: %s\n", codigo, http.getString().c_str());
    } else {                                                                                 
      Serial.printf("Error: %s\n", http.errorToString(codigo).c_str());
    }                                                                                        
   
    http.end();                                                                              
  }                                                         

  // ── Setup ──────────────────────────────────────────────────────
  void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n── PharmaTrack Sensor v2.0 (WiFiManager) ──");
                                                                                             
    pinMode(RESET_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
                                                                                             
    // Revisar si se quiere resetear antes de conectar                                       
    checkResetButton();
                                                                                             
    // Inicializar sensor                                                                    
    Wire.begin(SDA_PIN, SCL_PIN);
    if (!aht.begin()) {                                                                      
      Serial.println("ERROR: AHT10 no detectado. Verifica el cableado.");
      Serial.println("SDA → GPIO8   SCL → GPIO9   VCC → 3.3V   GND → GND");
      while (true) delay(1000);                                                              
    }
    Serial.println("AHT10 detectado correctamente");                                         
                                                            
    // Configurar WiFiManager                                                                
    wm.setConfigPortalTimeout(180);   // Portal abierto 3 minutos, luego reinicia
    wm.setConnectTimeout(30);         // 30 seg intentando conectar a la red guardada        
    wm.setAPClientCheck(true);        // Mantiene portal abierto mientras haya cliente conectado                                                                                  
                                                                                             
    Serial.println("Conectando al WiFi guardado...");                                        
                                                            
    // autoConnect: si tiene red guardada la usa, si no abre el AP "PharmaTrack-Sensor"      
    if (!wm.autoConnect("PharmaTrack-Sensor")) {
      Serial.println("No se pudo conectar. Reiniciando...");                                 
      delay(3000);                                                                           
      ESP.restart();
    }                                                                                        
                                                            
    Serial.printf("Conectado — IP: %s\n", WiFi.localIP().toString().c_str());                
  }
                                                                                             
  // ── Loop ───────────────────────────────────────────────────────
  unsigned long ultimoEnvio = 0;
                                                                                             
  void loop() {
    // Revisar botón de reset en cada ciclo                                                  
    checkResetButton();                                     
                                                                                             
    // Reconectar si se pierde la señal
    if (WiFi.status() != WL_CONNECTED) {                                                     
      Serial.println("WiFi perdido — reconectando...");     
      WiFi.reconnect();                                                                      
      delay(5000);
      return;                                                                                
    }                                                                                        
   
    unsigned long ahora = millis();                                                          
    if (ahora - ultimoEnvio >= INTERVALO_MS) {              
      ultimoEnvio = ahora;

      sensors_event_t evHumedad, evTemperatura;                                              
      aht.getEvent(&evHumedad, &evTemperatura);
                                                                                             
      float temp = evTemperatura.temperature;               
      float hum  = evHumedad.relative_humidity;

      if (temp > -10 && temp < 85 && hum >= 0 && hum <= 100) {                               
        enviarLectura(temp, hum);
      } else {                                                                               
        Serial.printf("Lectura inválida descartada: %.1f°C  %.1f%%\n", temp, hum);
      }                                                                                      
    }
  } 