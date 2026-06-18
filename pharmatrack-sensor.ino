#include <Wire.h>
#include <U8g2lib.h>

// Display Humedad — GPIO 6 (SDA), GPIO 7 (SCL)
#define SDA_PIN 6
#define SCL_PIN 7

U8G2_SSD1306_128X64_NONAME_F_SW_I2C display(U8G2_R0, SCL_PIN, SDA_PIN, U8X8_PIN_NONE);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n-- Test display GPIO6/GPIO7 --");

  // Escanear I2C para verificar que el display responde
  Wire.begin(SDA_PIN, SCL_PIN);
  Serial.println("Escaneando I2C...");
  bool encontrado = false;
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  Dispositivo en 0x%02X\n", addr);
      encontrado = true;
    }
  }
  if (!encontrado) Serial.println("  Ningun dispositivo encontrado.");

  // Inicializar y mostrar texto
  Serial.println("Iniciando display...");
  display.begin();
  Serial.println("begin() OK");

  display.clearBuffer();
  display.setFont(u8g2_font_6x10_tf);
  display.drawStr(20, 30, "GPIO 6/7 OK");
  display.drawStr(20, 45, "Display HUM");
  display.sendBuffer();
  Serial.println("Texto enviado. Debe verse en pantalla.");
}

void loop() {}
